#!/bin/sh
set -eu
TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_DIR=$(CDPATH= cd -- "$TEST_DIR/../../../../.." && pwd)
TEST_OUT=$(mktemp -d "${TMPDIR:-/tmp}/ewok-usb-read.XXXXXX")
trap 'rm -rf "$TEST_OUT"' EXIT HUP INT TERM
"${CC:-cc}" -std=c99 -g -O1 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$REPO_DIR/system/basic/libc/libewoksys/ewoksys/include" \
    "$TEST_DIR/usb_read_test.c" -o "$TEST_OUT/test"
"$TEST_OUT/test"
