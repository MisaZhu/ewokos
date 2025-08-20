@/bin/splash -i /usr/system/images/logos/ewokos.png -m "/drivers/tty_uart" -p 5 
@/bin/ipcserv /drivers/versatilepb/ttyd /dev/tty0

@/bin/splash -i /usr/system/images/logos/ewokos.png -m "config sessiond" -p 8
@/bin/ipcserv /sbin/sessiond

@/bin/splash -i /usr/system/images/logos/ewokos.png -m "/dev/tty0" -p 10
@/bin/bgrun /bin/session -r -t /dev/tty0 
