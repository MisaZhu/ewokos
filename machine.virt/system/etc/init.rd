@export UX_ID=0
@/bin/ipcserv /sbin/splashd -w 320 -h 240 -f 14
@/bin/splash -i /usr/system/images/logos/ewokos.png -m "start..."

@/bin/splash -m "start /dev/console0" -p 20
@export UX_ID=1
@/bin/ipcserv /drivers/consoled  /dev/console0
@set_stdio /dev/console0

@echo "+---------------------------------------+\n"
@echo "|  < EwokOS MicroKernel >               |\n" 
@echo "+---------------------------------------+\n"

@/bin/splash -m "start /dev/timer" -p 30
@/bin/ipcserv /drivers/timerd                 

@/bin/splash -m "start /dev/power0" -p 40
@/bin/ipcserv /drivers/virt/powerd  /dev/power0

@/bin/splash -m "start /dev/keyb0" -p 50
@/bin/ipcserv /drivers/virt/keybd   /dev/keyb0

@/bin/splash -m "start /dev/mouse0" -p 60
@/bin/ipcserv /drivers/virt/moused  /dev/mouse0

#@/bin/ipcserv /drivers/virt/smc91c111d /dev/eth0
#@/bin/ipcserv /drivers/netd             /dev/net0 /dev/eth0
#@/bin/bgrun /sbin/telnetd 

@/bin/splash -m "start /dev/null" -p 70
@/bin/ipcserv /drivers/nulld           /dev/null

@/bin/splash -m "mount /tmp" -p 70
@/bin/ipcserv /drivers/ramfsd          /tmp

@/bin/splash -m "start /dev/console1" -p 75
@export UX_ID=2
@/bin/ipcserv /drivers/consoled   /dev/console1 -i /dev/keyb0
@/bin/bgrun /bin/session -r -t /dev/console1 

@/bin/splash -m "load fonts" -p 80
@/bin/load_font

@/bin/splash -m "start /dev/x" -p 85
@/bin/ipcserv /drivers/xserverd        /dev/x

@/bin/splash -m "run xmouse" -p 90
@/bin/bgrun /sbin/x/xmouse /dev/mouse0 

@/bin/splash -m "run xim_none" -p 95
@/bin/bgrun /sbin/x/xim_none /dev/keyb0 

@/bin/splash -m "startx" -p 100
@/bin/bgrun /bin/x/xsession misa 

#@/bin/bgrun /drivers/virt/virtfsd
