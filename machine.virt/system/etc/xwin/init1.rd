@/bin/ipcserv /drivers/displayd         
@/bin/ipcserv /drivers/virt/fbd  /dev/fb0
@/bin/ipcserv /drivers/fontd            

@export UX_ID=0
@/bin/ipcserv /drivers/consoled  /dev/klog
