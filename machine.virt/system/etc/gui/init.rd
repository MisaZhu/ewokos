@/bin/ipcserv /drivers/virt/ttyd /dev/tty0
@/bin/ipcserv /sbin/sessiond
@/bin/bgrun /bin/session -r -t /dev/tty0 

@/bin/ipcserv /drivers/virt/fbd  /dev/fb0
@/bin/ipcserv /drivers/displayd        
@/bin/ipcserv /drivers/fontd            

@export UX_ID=0
@/bin/ipcserv /drivers/consoled  /dev/klog
@set_stdio /dev/klog

@echo "+---------------------------------------+\n"
@echo "|  < EwokOS MicroKernel >               |\n" 
@echo "+---------------------------------------+\n"

@/bin/ipcserv /drivers/timerd                 

@/bin/ipcserv /drivers/virt/keybd   /dev/keyb0
@/bin/ipcserv /drivers/virt/moused  /dev/mouse0

#@/bin/ipcserv /drivers/virt/smc91c111d /dev/eth0
#@/bin/ipcserv /drivers/netd             /dev/net0 /dev/eth0
#@/bin/bgrun /sbin/telnetd 

@/bin/ipcserv /drivers/nulld           /dev/null
@/bin/ipcserv /drivers/ramfsd          /tmp


@export UX_ID=1
@/bin/ipcserv /drivers/consoled   -i /dev/keyb0
@/bin/bgrun /bin/session -r -t /dev/console1 
@setux 1

@export UX_ID=2
@/bin/ipcserv /drivers/consoled   -i /dev/keyb0
@/bin/bgrun /bin/session -r -t /dev/console2 
