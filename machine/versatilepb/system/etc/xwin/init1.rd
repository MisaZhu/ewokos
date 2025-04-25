@/bin/ipcserv /drivers/versatilepb/fbd  /dev/fb0
@/bin/ipcserv /drivers/displayd         
@/bin/ipcserv /drivers/fontd            
@/bin/load_font

@export UX_ID=0
@/bin/ipcserv /drivers/consoled  /dev/klog
@export KLOG_DEV=/dev/klog