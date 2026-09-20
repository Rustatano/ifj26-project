#!/usr/bin/env bash
set -e

echo "Installing tools and dependencies."
echo "======================================================"

# tools
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  software-properties-common \
  clang-format \
  python3-pip \
  python3-venv \
  latexmk \
  texlive-latex-extra \
  texlive-lang-european

# gcc-16
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y gcc-16 g++-16

# pre-commit
pipx install pre-commit
pre-commit install
pre-commit install --hook-type commit-msg

echo "======================================================"
echo "Setup complete."
