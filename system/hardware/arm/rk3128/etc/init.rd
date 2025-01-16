/bin/ipcserv /drivers/rk3128/rk_uartd       /dev/tty0

/bin/ipcserv /drivers/rk3128/fbd            /dev/fb0
/bin/ipcserv /drivers/displayd             /dev/display /dev/fb0
/bin/ipcserv /drivers/fontd                /dev/font
/bin/ipcserv /drivers/consoled             0

/bin/ipcserv /drivers/rk3128/gpio_joystickd  /dev/joystick
/bin/ipcserv /drivers/vjoystickd             /dev/vjoystick /dev/joystick

/bin/ipcserv /drivers/timerd               /dev/timer
/bin/ipcserv /drivers/nulld                /dev/null
/bin/ipcserv /drivers/ramfsd               /tmp
/bin/ipcserv /drivers/proc/sysinfod        /proc/sysinfo
/bin/ipcserv /drivers/proc/stated          /proc/state

/bin/ipcserv /sbin/sessiond
#/bin/session -r &

/sbin/x/xim_none   /dev/vjoystick &
/sbin/x/xjoymoused /dev/vjoystick &

/bin/load_font
/bin/ipcserv /drivers/xserverd             /dev/x
@/sbin/x/xim_vkey 560 168&
@/bin/x/xsession misa &
