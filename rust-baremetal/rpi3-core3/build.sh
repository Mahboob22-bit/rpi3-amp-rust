#!/bin/bash
set -e

echo "=== Building Rust Bare-Metal Binary ==="
cargo build --release

echo ""
echo "=== Converting to raw binary ==="
rust-objcopy --strip-all -O binary \
    target/aarch64-unknown-none/release/rpi3-core3 \
    target/aarch64-unknown-none/release/kernel_core3.img

echo ""
echo "Binary info:"
ls -lh target/aarch64-unknown-none/release/kernel_core3.img

echo ""
echo "=== Ready to deploy ==="
echo "To copy to Raspberry Pi, run:"
echo "  scp target/aarch64-unknown-none/release/kernel_core3.img admin@rpi3-amp:~/rpi3-amp-project/rpi3-amp-rust/raspberry-pi/core-loader/"
