!/drivers/screend              /dev/scr0 /dev/fb0
!/drivers/consoled             /dev/console0 /dev/scr0
$

#!/drivers/raspix/mbox_actledd  /dev/actled

!/drivers/nulld                /dev/null
!/drivers/ramfsd               /tmp
!/drivers/proc/sysinfod        /proc/sysinfo
!/drivers/proc/stated          /proc/state

#pins: up down left right press
!/drivers/raspix/gpio_joystickd  /dev/joystick 5 6 13 16 19 

!/drivers/xserverd             /dev/x

/bin/session
/sbin/x/xjoystickd /dev/joystick revx
/sbin/x/xim_vkey
/bin/x/launcher
