/bin/ipcserv /drivers/versatilepb/ttyd /dev/tty0
@set_stdio /dev/tty0

/bin/ipcserv /drivers/versatilepb/fbd  /dev/fb0
/bin/ipcserv /drivers/displayd         /dev/display /dev/fb0
/bin/ipcserv /drivers/fontd            /dev/font

/bin/ipcserv /drivers/consoled -m /dev/klog -u 0
@set_stdio  /dev/klog
@set_stderr /dev/klog

@echo "+---------------------------------------+\n"
@echo "|  < EwokOS MicroKernel >               |\n" 
@echo "+---------------------------------------+\n"

/bin/ipcserv /drivers/timerd                 /dev/timer

/bin/ipcserv /drivers/versatilepb/ps2keybd   /dev/keyb0
/bin/ipcserv /drivers/versatilepb/ps2moused  /dev/mouse0

#/bin/ipcserv /drivers/versatilepb/smc91c111d /dev/eth0
#/bin/ipcserv /drivers/netd             /dev/net0 /dev/eth0
#/sbin/telnetd &

/bin/ipcserv /drivers/versatilepb/powerd     /dev/power0

/bin/ipcserv /drivers/nulld           /dev/null
/bin/ipcserv /drivers/ramfsd          /tmp
/bin/ipcserv /drivers/proc/sysinfod   /proc/sysinfo
/bin/ipcserv /drivers/proc/stated     /proc/state

/bin/ipcserv /sbin/sessiond

/bin/ipcserv /drivers/consoled   -i /dev/keyb0 -m /dev/console1 -u 1
/bin/ipcserv /drivers/consoled   -i /dev/keyb0 -m /dev/console2 -u 2

/bin/session -r -t /dev/console1 &
/bin/setux 1
/bin/session -r -t /dev/console2 &
/bin/session -r -t /dev/tty0 &

#/bin/load_font

/bin/ipcserv /drivers/xserverd        /dev/x
/sbin/x/xmoused /dev/mouse0 &
/sbin/x/xim_none /dev/keyb0 &

@/bin/x/xsession misa &