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
@/bin/ipcserv /drivers/consoled   /dev/console1 -i /dev/keyb0
@/bin/bgrun /bin/session -r -t /dev/console1 

@export UX_ID=2
@/bin/ipcserv /drivers/consoled   -i /dev/keyb0
@/bin/bgrun /bin/session -r -t /dev/console2 


#@/bin/load_font

@/bin/ipcserv /drivers/xserverd        /dev/x
@/bin/bgrun /sbin/x/xmouse /dev/mouse0 
@/bin/bgrun /sbin/x/xim_none /dev/keyb0 
@/bin/bgrun /bin/x/xsession misa 

@/bin/bgrun /drivers/virt/virtfsd
