/bin/ipcserv /drivers/virt/virt_uartd   /dev/tty0

$

/bin/ipcserv /drivers/timerd               /dev/timer
/bin/ipcserv /drivers/nulld                /dev/null
/bin/ipcserv /drivers/ramfsd               /tmp

@/bin/session -r
