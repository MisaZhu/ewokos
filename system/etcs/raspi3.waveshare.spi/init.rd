/bin/rundev /drivers/displayd              /dev/display /dev/fb0
/bin/rundev /drivers/consoled             /dev/console0 /dev/display
$

#/bin/rundev /drivers/raspix/mbox_actledd  /dev/actled

/bin/rundev /drivers/nulld                /dev/null
/bin/rundev /drivers/ramfsd               /tmp
/bin/rundev /drivers/proc/sysinfod        /proc/sysinfo
/bin/rundev /drivers/proc/stated          /proc/state

/bin/rundev /drivers/xserverd             /dev/x

@/bin/session &
@/sbin/x/xim_vkey &
@/bin/x/launcher &
