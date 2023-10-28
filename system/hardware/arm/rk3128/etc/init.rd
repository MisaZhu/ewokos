/bin/rundev /drivers/rk3128/rk_uartd       /dev/tty0
#/bin/rundev /drivers/rk3128/gpio_joykeybd  /dev/joykeyb
/bin/rundev /drivers/rk3128/gpio_joykeyb_minid  /dev/joykeyb
/bin/rundev /drivers/rk3128/gpio_joystickd  /dev/joystick
/bin/rundev /drivers/rk3128/fbd            /dev/fb0

/bin/rundev /drivers/fontd                /dev/font /usr/system/fonts/system.ttf
/bin/rundev /drivers/consoled             /dev/console0

/bin/rundev /drivers/timerd               /dev/timer
/bin/rundev /drivers/nulld                /dev/null
/bin/rundev /drivers/ramfsd               /tmp
/bin/rundev /drivers/proc/sysinfod        /proc/sysinfo
/bin/rundev /drivers/proc/stated          /proc/state

/bin/rundev /drivers/displayd             /dev/display /dev/fb0
/bin/rundev /drivers/xserverd             /dev/x

#/bin/rundev /drivers/xconsoled           /dev/console0

@/sbin/x/xim_none                          /dev/joykeyb &
@/sbin/x/xjoystickd /dev/joystick &
@/sbin/x/xim_vkey 560 168&

@/bin/x/launcher &
@/bin/session &