#!/bin/sh
set -eu
TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_DIR=$(CDPATH= cd -- "$TEST_DIR/../../.." && pwd)
TEST_OUT=$(mktemp -d "${TMPDIR:-/tmp}/ewok-dma-pin.XXXXXX")
trap 'rm -rf "$TEST_OUT"' EXIT HUP INT TERM
for page_size in 4K 16K 64K; do
    "${CC:-cc}" -std=c99 -g -O1 -Wall -Wextra -Werror \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -DPAGE_SIZE_${page_size}=1 \
        -I"$REPO_DIR/kernel/kernel/include" \
        -I"$REPO_DIR/kernel/lib/include" \
        -I"$REPO_DIR/kernel/platform/aarch64/arch/common/include" \
        -I"$REPO_DIR/system/basic/libc/libewoksys/ewoksys/include" \
        "$TEST_DIR/../src/mm/dma.c" "$TEST_DIR/dma_pin_test.c" \
        -o "$TEST_OUT/dma_pin_test"
    "$TEST_OUT/dma_pin_test"
done
