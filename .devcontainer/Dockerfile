FROM fedora:40 as base

SHELL ["/bin/bash", "-c"]

# Base setup
RUN dnf upgrade -y \
    && dnf install -y sudo \
    && groupadd dev \
    && useradd -g dev -m -s /bin/bash orphee \
    && echo "orphee ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

FROM base

USER orphee:dev

# NeoVim
RUN sudo dnf install -y neovim

# Misc
RUN sudo dnf install -y git gh curl wget bat tar zlib zip unzip openssh gnupg python3 python3-pip bash-completion

# LLVM toolchain
## LLVM perse can make cross platform builds
RUN sudo dnf install -y llvm clang clang-tools-extra lld lldb mlir

# LLVM devel
RUN sudo dnf install -y llvm-devel zlib-devel libffi-devel mlir-devel

# Build tools
RUN sudo dnf install -y cmake ninja-build

WORKDIR /workspace
