#!/bin/bash
set -e

RPI_HOST="${RPI_HOST:-admin@rpi3-amp}"
RPI_PATH="~/dev/rpi3-amp-rust"

echo "=== Deploying to Raspberry Pi ==="
echo "Target: $RPI_HOST:$RPI_PATH"

if [ ! -f "target/aarch64-unknown-none/release/kernel_core3.img" ]; then
    echo "Error: kernel_core3.img not found. Run ./build.sh first!"
    exit 1
fi

scp target/aarch64-unknown-none/release/kernel_core3.img \
    "$RPI_HOST:$RPI_PATH"

echo ""
echo "=== Deployment successful! ==="
echo "To run on Raspberry Pi:"
echo "  ssh $RPI_HOST"
echo "  cd $RPI_PATH"
echo "  sudo ./core3_loader_v2 kernel_core3.img"
