/bin/rundev /drivers/nezha/fbd            /dev/fb0 1920 1080
/bin/rundev /drivers/displayd             /dev/display   /dev/fb0
/bin/rundev /drivers/consoled             /dev/console0  /dev/display
/bin/rundev /drivers/nezha/uart16550d   /dev/tty0
$

/bin/rundev /drivers/timerd               /dev/timer
/bin/rundev /drivers/nulld                /dev/null
/bin/rundev /drivers/ramfsd               /tmp
/bin/rundev /drivers/proc/sysinfod        /proc/sysinfo
/bin/rundev /drivers/proc/stated          /proc/state

#/bin/rundev /drivers/xserverd             /dev/x

#@/bin/x/launcher &
@/bin/session
