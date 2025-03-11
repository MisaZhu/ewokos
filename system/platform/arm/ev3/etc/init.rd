@/bin/ipcserv /drivers/ev3/uartd         /dev/tty0
@/bin/ipcserv /sbin/sessiond
@/bin/bgrun /bin/session -r -t /dev/tty0 

@/bin/ipcserv /drivers/ev3/fbd      /dev/fb0
@/bin/ipcserv /drivers/displayd        
@/bin/ipcserv /drivers/fontd           

@export UX_ID=0
@/bin/ipcserv /drivers/consoled        -u 0
@set_stdio /dev/console0

@/bin/ipcserv /drivers/timerd          
@/bin/ipcserv /drivers/ramfsd          /tmp
@/bin/ipcserv /drivers/nulld           /dev/null


@export UX_ID=1
@/bin/ipcserv /drivers/consoled
@/bin/bgrun /bin/session -r -t /dev/console1 
@setux 1
