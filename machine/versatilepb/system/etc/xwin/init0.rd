@/bin/ipcserv /drivers/logd  /dev/log

@/bin/ipcserv /drivers/displayd         
@/bin/ipcserv /drivers/versatilepb/fbd  /dev/fb0
@/bin/ipcserv /drivers/fontd -l -o

@export UX_ID=0
@/bin/ipcserv /sbin/splashd -w 320 -h 240 -f 14
@/bin/splash -i /usr/system/images/logos/ewokos.png -m "start..."