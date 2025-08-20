@/bin/ipcserv /drivers/versatilepb/fbd  /dev/fb0
@/bin/ipcserv /drivers/displayd         
@/bin/ipcserv /drivers/fontd            

@export UX_ID=0
@/bin/ipcserv /sbin/splashd
@/bin/splash -i /usr/system/images/logos/ewokos.png -m "start..."