!/drivers/dispmand              /dev/dispman /dev/fb0
!/drivers/consoled             /dev/console0 /dev/dispman
$

#!/drivers/raspix/mbox_actledd  /dev/actled

!/drivers/nulld                /dev/null
!/drivers/ramfsd               /tmp
!/drivers/proc/sysinfod        /proc/sysinfo
!/drivers/proc/stated          /proc/state

#left button as joystick pins: up down left right press
!/drivers/raspix/gpio_joystickd  /dev/joystick 5 6 13 16 19 

#right button as keyboard pins: up down left right press
!/drivers/raspix/gpio_joykeybd   /dev/joykeyb  12 21 15 20 26 

!/drivers/xserverd             /dev/x

/bin/session
/sbin/x/xjoystickd             /dev/joystick revx
/sbin/x/xim_none               /dev/joykeyb
/sbin/x/xim_vkey
/bin/x/launcher
