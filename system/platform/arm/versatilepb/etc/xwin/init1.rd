@/bin/ipcserv /drivers/versatilepb/fbd  /dev/fb0
@/bin/ipcserv /drivers/displayd         
@/bin/ipcserv /drivers/fontd            

@export UX_ID=0
@/bin/ipcserv /drivers/init_consoled  /dev/klog -l 8
@export KLOG_DEV=/dev/klog