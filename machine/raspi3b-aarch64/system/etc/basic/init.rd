@/bin/ipcserv /drivers/raspix/uartd         /dev/tty0
@set_stdio /dev/tty0

@/bin/ipcserv /drivers/timerd          

@/bin/ipcserv /drivers/ramfsd          /tmp
@/bin/ipcserv /drivers/nulld           /dev/null

@/bin/ipcserv /sbin/sessiond
@/bin/bgrun /bin/session -r -t /dev/tty0 