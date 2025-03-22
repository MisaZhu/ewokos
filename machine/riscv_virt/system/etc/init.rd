@/bin/ipcserv /drivers/virt/virt_uartd   /dev/tty0

$

@/bin/ipcserv /drivers/timerd               
@/bin/ipcserv /drivers/nulld                /dev/null
@/bin/ipcserv /drivers/ramfsd               /tmp

@/bin/bgrun /bin/session -r
