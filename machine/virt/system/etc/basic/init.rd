@/bin/ipcserv /drivers/virt/ttyd    /dev/tty0
@/bin/ipcserv /drivers/virt/fbd		/dev/fb0
@set_stdio /dev/tty0

@/bin/ipcserv /drivers/timerd                 

@/bin/ipcserv /drivers/nulld                  /dev/null
@/bin/ipcserv /drivers/ramfsd                 /tmp

@/bin/ipcserv /sbin/sessiond
@/bin/bgrun /bin/session -r 
