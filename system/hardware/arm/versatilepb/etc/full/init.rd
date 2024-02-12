/bin/ipcserv /drivers/timerd          /dev/timer
/bin/ipcserv /drivers/versatilepb/ttyd       /dev/tty0

/bin/ipcserv /drivers/versatilepb/fbd        /dev/fb0
/bin/ipcserv /drivers/displayd        /dev/display /dev/fb0

#/bin/ipcserv /drivers/fontd           /dev/font /usr/system/fonts/system.ttf /usr/system/fonts/system_cn.ttf
/bin/ipcserv /drivers/fontd           /dev/font /usr/system/fonts/system.ttf
/bin/ipcserv /drivers/consoled        /dev/console0

/bin/ipcserv /drivers/versatilepb/ps2keybd   /dev/keyb0
/bin/ipcserv /drivers/versatilepb/ps2moused  /dev/mouse0

/bin/ipcserv /drivers/versatilepb/powerd     /dev/power0
#/bin/ipcserv /drivers/versatilepb/smc91c111d /dev/eth0
#/bin/ipcserv /drivers/netd             /dev/net0 /dev/eth0

/bin/ipcserv /drivers/nulld           /dev/null
/bin/ipcserv /drivers/ramfsd          /tmp
/bin/ipcserv /drivers/proc/sysinfod   /proc/sysinfo
/bin/ipcserv /drivers/proc/stated     /proc/state

@/sbin/sessiond &
@/bin/session &

@/bin/ipcserv /drivers/xserverd        /dev/x
@/sbin/x/xmoused /dev/mouse0 &
@/sbin/x/xim_none /dev/keyb0 &

#@export XTHEME=openlook
#@/bin/ipcserv /sbin/x/xwm_openlook

@export XTHEME=opencde
@/bin/ipcserv /sbin/x/xwm_opencde

@/bin/x/menubar &
@/bin/x/launcher &
