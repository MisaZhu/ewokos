/bin/rundev /drivers/timerd               /dev/timer
/bin/rundev /drivers/raspix/uartd         /dev/tty0
/bin/rundev /drivers/fontd                /dev/font
/bin/rundev /drivers/raspix/fbd           /dev/fb0 320 1480 3
/bin/rundev /drivers/displayd             /dev/display /dev/fb0
/bin/rundev /drivers/consoled             /dev/console0 /dev/display 0

$

/bin/rundev /drivers/nulld                /dev/null
/bin/rundev /drivers/ramfsd               /tmp
/bin/rundev /drivers/proc/sysinfod        /proc/sysinfo
/bin/rundev /drivers/proc/stated          /proc/state
/bin/rundev /drivers/raspix/usbd          /dev/touch0

/bin/rundev /drivers/xserverd             /dev/x /dev/display 0

@/bin/session &

@/sbin/x/xtouchd &
@/sbin/x/xim_vkey 600 160 &
@/bin/x/launcher &

