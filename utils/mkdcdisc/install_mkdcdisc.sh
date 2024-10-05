#!/bin/bash

# Clone mkdcdisc repository and build the utility
REPO_URL="https://gitlab.com/simulant/mkdcdisc"
CLONE_DIR="mkdcdisc"

if ! command -v meson &> /dev/null
then
  echo "Meson is not installed. Please install it before running this script."
  exit 0
fi

# Check if the repository already exists
if [ -d "$CLONE_DIR" ]; then
  echo "Repository already exists. Pulling latest changes..."
  cd $CLONE_DIR && git pull
else
  echo "Cloning repository..."
  git clone $REPO_URL
  cd $CLONE_DIR
fi

# Build the utility
echo "Setting up build directory..."
meson setup builddir

echo "Compiling the utility..."
meson compile -C builddir

echo "Build completed. mkdcdisc is ready to use."

