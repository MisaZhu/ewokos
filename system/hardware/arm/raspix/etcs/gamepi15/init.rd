/bin/ipcserv /drivers/raspix/uartd          /dev/tty0
/bin/ipcserv /drivers/raspix/lcdhatd        /dev/fb0

#left button as joystick pins: up down left right press
/bin/ipcserv /drivers/raspix/gpio_joystickd /dev/joystick 5 6 13 16 19 

#right button as keyboard pins: up down left right press
/bin/ipcserv /drivers/raspix/gpio_joykeybd  /dev/joykeyb  12 21 15 20 26 

/bin/ipcserv /drivers/fontd                 /dev/font /usr/system/fonts/system.ttf
/bin/ipcserv /drivers/consoled              /dev/console0

/bin/ipcserv /drivers/timerd                /dev/timer
/bin/ipcserv /drivers/nulld                 /dev/null
/bin/ipcserv /drivers/ramfsd                /tmp
/bin/ipcserv /drivers/proc/sysinfod         /proc/sysinfo
/bin/ipcserv /drivers/proc/stated           /proc/state

/bin/ipcserv /drivers/displayd              /dev/display /dev/fb0
/bin/ipcserv /drivers/xserverd              /dev/x
/bin/ipcserv /sbin/x/xwm_opencde

#/bin/ipcserv /drivers/xconsoled             /dev/console0

@/sbin/x/xjoystickd             /dev/joystick revx &
@/sbin/x/xim_none               /dev/joykeyb &
@/sbin/x/xim_vkey &

@/sbin/sessiond &

@/bin/x/menubar &
@/bin/x/launcher &
@/bin/session &