FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

# installation of build dependencies
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    software-properties-common \
    clang-format \
    python3-pip \
    python3-venv \
    texlive-latex-extra \
    texlive-extra-utils \
    texlive-lang-european \
    latexmk \
    openssh-client \
    git \
    libcmocka-dev \
    cmocka-doc \
    cmake \
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y gcc-16 g++-16 \
    && rm -rf /var/lib/apt/lists/*

# installation of pre-commit
RUN pip install --break-system-packages pre-commit
