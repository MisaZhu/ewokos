/bin/ipcserv /drivers/versatilepb/ttyd       /dev/tty0

/bin/ipcserv /drivers/timerd                 /dev/timer
/bin/ipcserv /drivers/nulld                  /dev/null
/bin/ipcserv /drivers/ramfsd                 /tmp
/bin/ipcserv /drivers/proc/sysinfod          /proc/sysinfo
/bin/ipcserv /drivers/proc/stated            /proc/state

/bin/ipcserv /drivers/versatilepb/smc91c111d /dev/eth0
/bin/ipcserv /drivers/netd             /dev/net0 /dev/eth0

@/sbin/sessiond &
@/bin/session -r &
@/sbin/telnetd &