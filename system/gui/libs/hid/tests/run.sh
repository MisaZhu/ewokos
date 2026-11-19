#!/bin/sh
# Host tests for the multi-touch USB touch path (no hardware needed).
set -eu
cd "$(dirname "$0")"

cc -std=c99 -Wall -Wextra -Werror -I../include \
  ../src/hid_report.c hid_touch_parse_test.c -o hid_touch_parse_test
./hid_touch_parse_test
rm -f hid_touch_parse_test

cc -std=c99 -Wall -Wextra -Werror -I../include \
  ../src/hid_mt.c hid_mt_test.c -o hid_mt_test
./hid_mt_test
rm -f hid_mt_test