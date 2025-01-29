/bin/ipcserv /drivers/raspix/uartd         /dev/tty0
@set_stdio /dev/tty0

/bin/ipcserv /drivers/raspix/fbd      /dev/fb0
/bin/ipcserv /drivers/displayd        /dev/display /dev/fb0
/bin/ipcserv /drivers/fontd           /dev/font

/bin/ipcserv /drivers/consoled        -u 0
@set_stdio /dev/console0

/bin/ipcserv /sbin/sessiond
@/bin/session -r -t /dev/tty0 &

/bin/ipcserv /drivers/timerd          /dev/timer
/bin/ipcserv /drivers/ramfsd          /tmp

/bin/ipcserv /drivers/nulld           /dev/null
/bin/ipcserv /drivers/proc/sysinfod   /proc/sysinfo
/bin/ipcserv /drivers/proc/stated     /proc/state

/sbin/x/xim_none &

#/bin/load_font
/bin/ipcserv /drivers/xserverd        /dev/x
@/bin/x/xsession  misa &
