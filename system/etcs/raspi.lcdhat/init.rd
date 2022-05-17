/bin/rundev /drivers/raspix/lcdhatd       /dev/fb0
/bin/rundev /drivers/raspix/mini_uartd    /dev/tty0
/bin/rundev /drivers/displayd             /dev/display /dev/fb0
/bin/rundev /drivers/consoled             /dev/console0 /dev/display
$

#/bin/rundev /drivers/raspix/mbox_actledd  /dev/actled

/bin/rundev /drivers/nulld                /dev/null
/bin/rundev /drivers/ramfsd               /tmp
/bin/rundev /drivers/proc/sysinfod        /proc/sysinfo
/bin/rundev /drivers/proc/stated          /proc/state

/bin/rundev /drivers/raspix/hat13_joystickd     /dev/joystick
/bin/rundev /drivers/raspix/hat13_joykeybd      /dev/keyb0 revx
/bin/rundev /drivers/xserverd             /dev/x

@/sbin/x/xjoystickd /dev/joystick &
@/sbin/x/xim_vkey &
@/bin/x/launcher &

@/bin/session &