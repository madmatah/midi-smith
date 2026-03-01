FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

ARG ARM_GCC_URL=https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      cmake ninja-build git g++ \
      clang-format python3-pip python3-venv \
      curl xz-utils \
 && python3 -m pip install --break-system-packages cpplint gcovr \
 && curl -fSL "$ARM_GCC_URL" | tar -xJ -C /opt \
 && rm -rf /var/lib/apt/lists/*

ENV PATH="/opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin:${PATH}"
