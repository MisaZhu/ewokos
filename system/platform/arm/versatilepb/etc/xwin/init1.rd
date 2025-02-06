@/bin/ipcserv /drivers/versatilepb/fbd  /dev/fb0
@/bin/ipcserv /drivers/displayd         
@/bin/ipcserv /drivers/fontd            

@export UX_ID=0
@/bin/ipcserv /drivers/consoled  /dev/klog
@set_stdio /dev/klog
@export KLOG_DEV=/dev/klog