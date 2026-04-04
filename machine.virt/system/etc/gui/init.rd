@export TZ=CST-8

@/bin/ipcserv /drivers/logd /dev/log

@/bin/ipcserv /drivers/virt/ttyd         /dev/tty0
@/bin/ipcserv /sbin/sessiond
@/bin/bgrun /bin/session -r -t /dev/tty0 

@/bin/ipcserv /drivers/displayd        
@/bin/ipcserv /drivers/virt/fbd      /dev/fb0
@/bin/ipcserv /drivers/fontd           

@export UX_ID=0
@/bin/ipcserv /drivers/consoled        -u 0
@set_stdio /dev/console0

@/bin/ipcserv /drivers/timerd          

@/bin/ipcserv /drivers/virt/snd /dev/sound0

@/bin/ipcserv /drivers/virt/net /dev/eth0
@/bin/ipcserv /drivers/netd /dev/net0 /dev/eth0

@/bin/ipcserv /drivers/timed    /dev/time

@/bin/ipcserv /drivers/virt/keybd   /dev/keyb0
@/bin/ipcserv /drivers/virt/moused  /dev/mouse0

@/bin/ipcserv /drivers/ramfsd          /tmp
@/bin/ipcserv /drivers/nulld           /dev/null

@export UX_ID=1
@/bin/ipcserv /drivers/consoled        -i /dev/keyb0
@/bin/bgrun /bin/session -r -t /dev/console1 
@setux 0 1
