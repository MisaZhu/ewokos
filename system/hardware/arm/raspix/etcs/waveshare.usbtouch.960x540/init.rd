rundev /drivers/timerd                /dev/timer
#rundev /drivers/raspix/uartd         /dev/tty0

rundev /drivers/raspix/fbd            /dev/fb0 960 540
#rundev /drivers/raspix/fbd           /dev/fb0 1920 1080
rundev /drivers/fontd                 /dev/font
rundev /drivers/displayd              /dev/display /dev/fb0
rundev /drivers/consoled              /dev/console0 /dev/display 0
$

rundev /drivers/nulld                /dev/null
rundev /drivers/ramfsd               /tmp
rundev /drivers/proc/sysinfod        /proc/sysinfo
rundev /drivers/proc/stated          /proc/state

rundev /drivers/raspix/usbd          /dev/touch0
rundev /drivers/xserverd             /dev/x

@/bin/session &

@/sbin/x/xtouchd &
#@/sbin/x/xim_none &
@/sbin/x/xim_vkey &
@/bin/x/launcher &

