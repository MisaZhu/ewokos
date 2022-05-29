/bin/rundev /drivers/miyoo/ms_uartd       /dev/tty0
/bin/rundev /drivers/miyoo/fbd            /dev/fb0 640 480
/bin/rundev /drivers/displayd             /dev/display   /dev/fb0
#/bin/rundev /drivers/consoled            /dev/console0  /dev/display
$

/bin/rundev /drivers/nulld                /dev/null
/bin/rundev /drivers/ramfsd               /tmp
/bin/rundev /drivers/proc/sysinfod        /proc/sysinfo
/bin/rundev /drivers/proc/stated          /proc/state


/bin/rundev /drivers/miyoo/gpio_joykeybd  /dev/joykeyb
/bin/rundev /drivers/xserverd             /dev/x

@/sbin/x/xim_none               /dev/joykeyb &
@/sbin/x/xim_vkey &
@/bin/x/launcher &
@/bin/session &
