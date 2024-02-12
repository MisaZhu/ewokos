#/bin/ipcserv /drivers/rk3128/gpio_joykeybd  /dev/joykeyb
/bin/ipcserv /drivers/rk3128/gpio_joykeyb_minid  /dev/joykeyb
/bin/ipcserv /drivers/rk3128/gpio_joystickd  /dev/joystick
/bin/ipcserv /drivers/rk3128/fbd            /dev/fb0

/bin/ipcserv /drivers/fontd                /dev/font /usr/system/fonts/system.ttf
/bin/ipcserv /drivers/consoled             /dev/console0

/bin/ipcserv /drivers/timerd               /dev/timer
/bin/ipcserv /drivers/nulld                /dev/null
/bin/ipcserv /drivers/ramfsd               /tmp
/bin/ipcserv /drivers/proc/sysinfod        /proc/sysinfo
/bin/ipcserv /drivers/proc/stated          /proc/state

/bin/ipcserv /drivers/displayd             /dev/display /dev/fb0
/bin/ipcserv /drivers/xserverd             /dev/x
/bin/ipcserv /sbin/x/xwm_opencde

#/bin/ipcserv /drivers/xconsoled           /dev/console0

@/sbin/x/xim_none                         /dev/joykeyb &
@/sbin/x/xjoymoused /dev/joystick &
@/sbin/x/xim_vkey 560 168&

@/sbin/sessiond &

@/bin/x/menubar &
@/bin/x/launcher &
@/bin/session &