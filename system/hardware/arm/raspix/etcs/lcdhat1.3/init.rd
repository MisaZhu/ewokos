/bin/ipcserv /drivers/raspix/uartd         /dev/tty0

/bin/ipcserv /drivers/raspix/lcdhatd        /dev/fb0 
/bin/ipcserv /drivers/displayd              /dev/display /dev/fb0
/bin/ipcserv /drivers/fontd                 /dev/font

/bin/ipcserv /drivers/consoled              /dev/console0
@export INIT_OUT_DEV=/dev/console0

/bin/ipcserv /drivers/raspix/hat13_joykeybd /dev/keyb0
/bin/ipcserv /drivers/vjoystickd               /dev/vjoystick /dev/keyb0
#/bin/ipcserv /drivers/raspix/hat13_joystickd /dev/joystick

/bin/ipcserv /drivers/timerd                /dev/timer
/bin/ipcserv /drivers/nulld                 /dev/null
/bin/ipcserv /drivers/ramfsd                /tmp
/bin/ipcserv /drivers/proc/sysinfod         /proc/sysinfo
/bin/ipcserv /drivers/proc/stated           /proc/state

/bin/ipcserv /sbin/sessiond

/sbin/x/xim_none   /dev/vjoystick &
/sbin/x/xjoymoused /dev/vjoystick &
/sbin/x/xim_vkey &

/bin/load_font
/bin/ipcserv /drivers/xserverd              /dev/x
@/bin/x/xsession misa &