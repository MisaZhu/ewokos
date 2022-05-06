!/drivers/screend              /dev/scr0 /dev/fb0
!/drivers/consoled             /dev/console0 /dev/scr0
$

!/drivers/nulld                /dev/null
!/drivers/ramfsd               /tmp
!/drivers/proc/sysinfod        /proc/sysinfo
!/drivers/proc/stated          /proc/state


!/drivers/xserverd             /dev/x
/bin/session
/sbin/x/xmoused /dev/mouse0
/sbin/x/xim_none
/sbin/x/xim_vkey
/bin/x/launcher
