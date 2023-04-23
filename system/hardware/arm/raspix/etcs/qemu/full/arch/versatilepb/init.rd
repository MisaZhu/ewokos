/bin/rundev /drivers/versatilepb/fbd       /dev/fb0
/bin/rundev /drivers/versatilepb/ttyd      /dev/tty0
/bin/rundev /drivers/displayd             /dev/display /dev/fb0
/bin/rundev /drivers/consoled             /dev/console0 /dev/display

$

/bin/rundev /drivers/nulld                /dev/null
/bin/rundev /drivers/ramfsd               /tmp
/bin/rundev /drivers/proc/sysinfod        /proc/sysinfo
/bin/rundev drivers/proc/stated           /proc/state
/bin/rundev /drivers/timerd               /dev/timer

/bin/rundev /drivers/versatilepb/ps2keybd   /dev/keyb0
/bin/rundev /drivers/versatilepb/ps2moused  /dev/mouse0


/bin/rundev /drivers/xserverd             /dev/x

@/sbin/x/xmoused /dev/mouse0 &
@/sbin/x/xim_none /dev/keyb0 esc_home &
@/sbin/x/xim_vkey &
@/bin/x/launcher &

@/bin/session &
