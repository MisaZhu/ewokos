@echo "+---------------------------------------+\n"
@echo "|  < EwokOS MicroKernel >               |\n" 
@echo "+---------------------------------------+\n"

@/bin/splash -i /usr/system/images/logos/apple_old.png -m "/drivers/timerd" -p 20
@/bin/ipcserv /drivers/timerd                 

@/bin/splash  -m "config keyboard" -p 30
@/bin/ipcserv /drivers/versatilepb/ps2keybd   /dev/keyb0
@/bin/ipcserv /drivers/vkeybd   /dev/vkeyb
@/bin/splash  -m "config mouse" -p 40
@/bin/ipcserv /drivers/versatilepb/ps2moused  /dev/mouse0

#@/bin/ipcserv /drivers/versatilepb/smc91c111d /dev/eth0
#@/bin/ipcserv /drivers/netd             /dev/net0 /dev/eth0
#@/bin/bgrun /sbin/telnetd 

@/bin/splash  -m "config powerd" -p 50
@/bin/ipcserv /drivers/versatilepb/powerd     /dev/power0

@/bin/splash  -m "config nulld" -p 60
@/bin/ipcserv /drivers/nulld           /dev/null
@/bin/splash  -m "config ramfsd" -p 70
@/bin/ipcserv /drivers/ramfsd          /tmp

@/bin/splash  -m "config console" -p 80
@export UX_ID=1
@/bin/ipcserv /drivers/consoled   /dev/console0 -i /dev/vkeyb
@/bin/bgrun /bin/session -r -t /dev/console0


@/bin/splash  -m "config fonts"
@/bin/load_font

@/bin/splash  -m "config X" -p 100
@/bin/ipcserv /drivers/xserverd        /dev/x
@/bin/bgrun /sbin/x/xmouse /dev/mouse0 
@/bin/bgrun /sbin/x/xim_none /dev/vkeyb
@/bin/bgrun /bin/x/xsession misa 