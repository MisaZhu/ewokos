/bin/ipcserv /drivers/raspix/uartd         /dev/tty0

/bin/ipcserv /drivers/waveshare/rpi_lcdd   /dev/fb0
/bin/ipcserv /drivers/displayd             /dev/display /dev/fb0
/bin/ipcserv /drivers/fontd                /dev/font

/bin/ipcserv /drivers/consoled              0
@set_stdio /dev/console0

/bin/ipcserv /drivers/timerd               /dev/timer

/bin/ipcserv /drivers/nulld                /dev/null
/bin/ipcserv /drivers/ramfsd               /tmp
/bin/ipcserv /drivers/proc/sysinfod        /proc/sysinfo
/bin/ipcserv /drivers/proc/stated          /proc/state

/bin/ipcserv /sbin/sessiond
#/bin/session -r &

/bin/ipcserv /drivers/xserverd             /dev/x

@/sbin/x/xtouchd &
@/sbin/x/xim_vkey &

@/bin/x/xsession misa &