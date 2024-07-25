#include <memory>

#include <llvm/ExecutionEngine/JITLink/JITLinkMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace llvm::orc;

static cl::list<std::string> InputIRFiles(cl::Positional, cl::OneOrMore,
                                          cl::desc("<input IR files>"));

static cl::opt<std::string> EntryPointName("entry", cl::desc("Entry function"),
                                           cl::init("main"));

ExitOnError ExitOnErr;

class JITGraphPrintPlugin : public ObjectLinkingLayer::Plugin {
public:
  void modifyPassConfig(MaterializationResponsibility &MR,
                        jitlink::LinkGraph &LG,
                        jitlink::PassConfiguration &Config) override {

    outs() << "MyPlugin -- Modifying pass config for " << LG.getName() << " ("
           << LG.getTargetTriple().str() << "):\n";

    Config.PostPrunePasses.push_back(printGraph);
  }

  void notifyLoaded(MaterializationResponsibility &MR) override {
    outs() << "Loading object defining " << MR.getSymbols() << '\n';
  }

  Error notifyEmitted(MaterializationResponsibility &MR) override {
    outs() << "Emitted object defining " << MR.getSymbols() << '\n';
    return Error::success();
  }

  Error notifyFailed(MaterializationResponsibility &MR) override {
    return Error::success();
  }

  Error notifyRemovingResources(JITDylib &JD, ResourceKey K) override {
    return Error::success();
  }

  void notifyTransferringResources(JITDylib &JD, ResourceKey DstKey,
                                   ResourceKey SrcKey) override {}

private:
  static void printBlockContent(jitlink::Block &B) {
    constexpr JITTargetAddress LineWidth = 16;

    if (B.isZeroFill()) {
      outs() << "    " << formatv("{0:x16}", B.getAddress()) << ": "
             << B.getSize() << " bytes of zero-fill.\n";
      return;
    }

    ExecutorAddr InitAddr(B.getAddress().getValue() & ~(LineWidth - 1));
    ExecutorAddr StartAddr = B.getAddress();
    ExecutorAddr EndAddr = B.getAddress() + B.getSize();
    auto *Data = reinterpret_cast<const uint8_t *>(B.getContent().data());

    for (ExecutorAddr CurAddr = InitAddr; CurAddr != EndAddr; ++CurAddr) {
      if (CurAddr % LineWidth == 0)
        outs() << "          " << formatv("{0:x16}", CurAddr.getValue())
               << ": ";
      if (CurAddr < StartAddr)
        outs() << "   ";
      else
        outs() << formatv("{0:x-2}", Data[CurAddr - StartAddr]) << " ";
      if (CurAddr % LineWidth == LineWidth - 1)
        outs() << "\n";
    }
    if (EndAddr % LineWidth != 0)
      outs() << "\n";
  }

  static Error printGraph(jitlink::LinkGraph &G) {
    DenseSet<jitlink::Block *> BlocksAlreadyVisited;

    outs() << "Graph \"" << G.getName() << "\"\n";

    for (auto &S : G.sections()) {
      outs() << "  Section " << S.getName() << ":\n";

      for (auto *Sym : S.symbols()) {

        outs() << "    " << formatv("{0:x16}", Sym->getAddress()) << ": ";

        if (Sym->hasName())
          outs() << Sym->getName() << "\n";
        else
          outs() << "<anonymous symbol>\n";

        auto &B = Sym->getBlock();

        if (BlocksAlreadyVisited.count(&B)) {
          outs() << "      Block " << formatv("{0:x16}", B.getAddress())
                 << " already printed.\n";
          continue;
        } else
          outs() << "      Block " << formatv("{0:x16}", B.getAddress())
                 << ":\n";

        outs() << "        Content:\n";
        printBlockContent(B);
        BlocksAlreadyVisited.insert(&B);

        if (!B.edges().empty()) {
          outs() << "        Edges:\n";
          for (auto &E : B.edges()) {
            outs() << "          "
                   << formatv("{0:x16}", B.getAddress() + E.getOffset())
                   << ": kind = " << formatv("{0:d}", E.getKind())
                   << ", addend = " << formatv("{0:x}", E.getAddend())
                   << ", target = ";
            jitlink::Symbol &TargetSym = E.getTarget();
            if (TargetSym.hasName())
              outs() << TargetSym.getName() << "\n";
            else
              outs() << "<anonymous target>\n";
          }
        }
        outs() << "\n";
      }
    }
    return Error::success();
  }
};

int main(int argc, char **argv) {
  InitLLVM X{argc, argv};

  if (InitializeNativeTarget()) {
    errs() << "Unable to initialize native target\n";
    return -1;
  }

  if (InitializeNativeTargetAsmPrinter()) {
    errs() << "Unable to initialize native target asm printer\n";
    return -1;
  }

  cl::ParseCommandLineOptions(argc, argv);

  ExitOnErr.setBanner(std::string(argv[0]) + ": ");

  auto loadModuleFromFile =
      [](const std::string &filename) -> Expected<ThreadSafeModule> {
    auto Ctx = std::make_unique<LLVMContext>();
    SMDiagnostic Err;

    if (auto M = parseIRFile(filename, Err, *Ctx))
      return ThreadSafeModule(std::move(M), std::move(Ctx));

    std::string Msg;
    {
      raw_string_ostream OS(Msg);
      Err.print("", OS);
    }
    return make_error<StringError>(std::move(Msg), inconvertibleErrorCode());
  };

  std::vector<ThreadSafeModule> TSMs;
  for (const std::string &Path : InputIRFiles) {
    TSMs.push_back(ExitOnErr(loadModuleFromFile(Path)));
  }

  auto JIT = ExitOnErr(
      LLJITBuilder{}
          .setObjectLinkingLayerCreator(
              [](ExecutionSession &ES, const Triple &T) {
                auto OBL = std::make_unique<ObjectLinkingLayer>(
                    ES, ExitOnErr(jitlink::InProcessMemoryManager::Create()));

                OBL->addPlugin(std::make_unique<JITGraphPrintPlugin>());

                return OBL;
              })
          .create());

  for (ThreadSafeModule &TSM : TSMs)
    ExitOnErr(JIT->addIRModule(std::move(TSM)));

  auto MainAddr = ExitOnErr(JIT->lookup(EntryPointName));

  auto *Entry = MainAddr.toPtr<int()>();

  outs() << "Result: " << Entry() << "\n";

  return 0;
}