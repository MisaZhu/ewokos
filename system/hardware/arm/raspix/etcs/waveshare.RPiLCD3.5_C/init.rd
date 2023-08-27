/bin/rundev /drivers/timerd               /dev/timer
#/bin/rundev /drivers/raspix/mini_uartd    /dev/tty0
/bin/rundev /drivers/fontd                /dev/font
/bin/rundev /drivers/raspix/rpi_lcdd      /dev/rpi_lcd
/bin/rundev /drivers/displayd             /dev/display /dev/rpi_lcd
/bin/rundev /drivers/consoled             /dev/console0 /dev/display
$

/bin/rundev /drivers/nulld                /dev/null
/bin/rundev /drivers/ramfsd               /tmp
/bin/rundev /drivers/proc/sysinfod        /proc/sysinfo
/bin/rundev /drivers/proc/stated          /proc/state

/bin/rundev /drivers/xserverd             /dev/x

#@/bin/session &

@/sbin/x/xtouchd /dev/rpi_lcd &
@/sbin/x/xim_vkey &
@/bin/x/launcher &
