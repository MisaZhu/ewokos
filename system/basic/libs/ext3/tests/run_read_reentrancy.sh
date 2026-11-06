#!/bin/sh
set -eu
TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_DIR=$(CDPATH= cd -- "$TEST_DIR/../../../../.." && pwd)
TEST_OUT=$(mktemp -d "${TMPDIR:-/tmp}/ewok-ext3-read.XXXXXX")
trap 'rm -rf "$TEST_OUT"' EXIT HUP INT TERM
mkdir -p "$TEST_OUT/ewoksys"
# Only pathname helpers are needed from VFS; keep host ABI types separate.
cat > "$TEST_OUT/ewoksys/vfs.h" <<'EOF'
#include <stdint.h>
#define FS_FULL_NAME_MAX 512
const char* vfs_dir_name(const char*, char*, uint32_t);
const char* vfs_file_name(const char*, char*, uint32_t);
EOF
: > "$TEST_OUT/ewokos_config.h"
"${CC:-cc}" -std=c99 -g -O1 -Wall -Wextra \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$TEST_OUT" -I"$TEST_DIR/../include" \
    -I"$REPO_DIR/system/basic/libs/sd/include" \
    -I"$REPO_DIR/system/basic/libc/libewoksys/ewoksys/include" \
    -DEXT3_SOURCE="\"${EXT3_SOURCE:-$TEST_DIR/../src/ext3fs.c}\"" \
    "$TEST_DIR/read_reentrancy_test.c" "$TEST_DIR/../src/jbd.c" \
    -o "$TEST_OUT/read_reentrancy_test"
"$TEST_OUT/read_reentrancy_test"
