@export TZ=CST-8

@/bin/ipcserv /sbin/splashd -w 320 -h 240 -f 12 -d
@/bin/splash -i /usr/system/images/logos/ewokos.png -m "start..."

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

@/bin/splash -m "start /dev/sound0" -p 63
@/bin/ipcserv /drivers/virt/snd /dev/sound0

@/bin/splash -m "start /dev/eth0" -p 65
@/bin/ipcserv /drivers/virt/net /dev/eth0

@/bin/splash -m "start /dev/net0" -p 68
@/bin/ipcserv /drivers/netd /dev/net0 /dev/eth0

@/bin/splash -m "start /dev/timed" -p 70
@/bin/ipcserv /drivers/timed    /dev/time

#@/bin/splash -m "start telnetd" -p 72
#@/bin/bgrun /sbin/telnetd

@/bin/splash -m "start /dev/null" -p 73
@/bin/ipcserv /drivers/nulld           /dev/null

@/bin/splash -m "mount /tmp" -p 74
@/bin/ipcserv /drivers/ramfsd          /tmp

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
#@/bin/bgrun /bin/x/xsession

#@/bin/bgrun /drivers/virt/virtfsd
