# ORC

## NOTE

After initializing the `nativeTarget` and the `nativeTargetAsmPrinter` more
options are added to the command line.

## Adding code to ORC

```c++
JIT->addIRModule()
```

```c++
JIT->addObjectFile()
```

## TODO

link with orc-rt - have access to stdlib

```
clang main.c -c -S -emit-llvm -o <filename.ll>
```
