/bin/ipcserv /drivers/nezha/fbd            /dev/fb0 1920 1080
/bin/ipcserv /drivers/displayd             /dev/display   /dev/fb0
/bin/ipcserv /drivers/consoled             /dev/console0  /dev/display
/bin/ipcserv /drivers/nezha/uart16550d   /dev/tty0
$

/bin/ipcserv /drivers/timerd               /dev/timer
/bin/ipcserv /drivers/nulld                /dev/null
/bin/ipcserv /drivers/ramfsd               /tmp
/bin/ipcserv /drivers/proc/sysinfod        /proc/sysinfo
/bin/ipcserv /drivers/proc/stated          /proc/state

#/bin/ipcserv /drivers/xserverd             /dev/x

#@/bin/x/launcher &
@/bin/session
