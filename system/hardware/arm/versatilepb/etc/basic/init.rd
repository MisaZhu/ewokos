/bin/rundev /drivers/versatilepb/ttyd       /dev/tty0

/bin/rundev /drivers/timerd                 /dev/timer
/bin/rundev /drivers/nulld                  /dev/null
/bin/rundev /drivers/ramfsd                 /tmp
#/bin/rundev /drivers/proc/sysinfod          /proc/sysinfo
/bin/rundev /drivers/proc/stated            /proc/state

@/sbin/sessiond &

@/bin/session &