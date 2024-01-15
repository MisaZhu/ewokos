/bin/rundev /drivers/raspix/uartd         /dev/tty0
/bin/rundev /drivers/raspix/xpt2046d      /dev/touch0
/bin/rundev /drivers/raspix/fbd           /dev/fb0

/bin/rundev /drivers/fontd                /dev/font /usr/system/fonts/system.ttf
/bin/rundev /drivers/consoled             /dev/console0

/bin/rundev /drivers/timerd               /dev/timer
/bin/rundev /drivers/nulld                /dev/null
/bin/rundev /drivers/ramfsd               /tmp
/bin/rundev /drivers/proc/sysinfod        /proc/sysinfo
/bin/rundev /drivers/proc/stated          /proc/state

/bin/rundev /drivers/displayd             /dev/display /dev/fb0
/bin/rundev /drivers/xserverd             /dev/x

#/bin/rundev /drivers/xconsoled             /dev/console0

@/sbin/x/xtouchd &
@/sbin/x/xim_vkey &

@/sbin/sessiond &

@/bin/x/menubar &
@/bin/x/launcher &
@/bin/session &