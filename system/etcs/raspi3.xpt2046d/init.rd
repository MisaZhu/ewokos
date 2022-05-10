!/drivers/displayd              /dev/display /dev/fb0
!/drivers/consoled             /dev/console0 /dev/display
$

#!/drivers/raspix/mbox_actledd  /dev/actled

!/drivers/nulld                /dev/null
!/drivers/ramfsd               /tmp
!/drivers/proc/sysinfod        /proc/sysinfo
!/drivers/proc/stated          /proc/state

!/drivers/raspix/xpt2046d      /dev/touch0
!/drivers/xserverd             /dev/x

/bin/session
/sbin/x/xtouchd
/sbin/x/xim_vkey
/bin/x/launcher