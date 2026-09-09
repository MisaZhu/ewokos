#!/bin/bash
# Boot machine.virt, log in as guest (empty password), launch a program,
# capture the serial console.
# usage: run_qemu_test.sh "<command>" [wait_after]
cd /Users/mingmingzhu/work/ewokos/machine.virt/kernel || exit 1
CMD="${1:-/apps/xfilemanager/xfilemanager}"
WAIT="${2:-30}"
rm -f /tmp/qin /tmp/qout
mkfifo /tmp/qin
sleep 3600 > /tmp/qin &            # keep the fifo's write end open
KEEPER=$!
make run < /tmp/qin > /tmp/qout 2>&1 &
QEMU=$!
sleep 24
printf 'guest\n' > /tmp/qin
sleep 3
printf '\n' > /tmp/qin             # empty password
sleep 5
printf '%s\n' "$CMD" > /tmp/qin
sleep "$WAIT"
kill $QEMU $KEEPER 2>/dev/null
wait $QEMU 2>/dev/null
echo "=== console tail ==="
sed -n '/login:/,$p' /tmp/qout | tail -50
