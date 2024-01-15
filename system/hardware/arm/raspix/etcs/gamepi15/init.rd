/bin/rundev /drivers/raspix/uartd          /dev/tty0
/bin/rundev /drivers/raspix/lcdhatd        /dev/fb0

#left button as joystick pins: up down left right press
/bin/rundev /drivers/raspix/gpio_joystickd /dev/joystick 5 6 13 16 19 

#right button as keyboard pins: up down left right press
/bin/rundev /drivers/raspix/gpio_joykeybd  /dev/joykeyb  12 21 15 20 26 

/bin/rundev /drivers/fontd                 /dev/font /usr/system/fonts/system.ttf
/bin/rundev /drivers/consoled              /dev/console0

/bin/rundev /drivers/timerd                /dev/timer
/bin/rundev /drivers/nulld                 /dev/null
/bin/rundev /drivers/ramfsd                /tmp
/bin/rundev /drivers/proc/sysinfod         /proc/sysinfo
/bin/rundev /drivers/proc/stated           /proc/state

/bin/rundev /drivers/displayd              /dev/display /dev/fb0
/bin/rundev /drivers/xserverd              /dev/x

#/bin/rundev /drivers/xconsoled             /dev/console0

@/sbin/x/xjoystickd             /dev/joystick revx &
@/sbin/x/xim_none               /dev/joykeyb &
@/sbin/x/xim_vkey &

@/sbin/sessiond &

@/bin/x/menubar &
@/bin/x/launcher &
@/bin/session &