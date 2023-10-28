/bin/rundev /drivers/miyoo/ms_uartd           /dev/tty0
#/bin/rundev /drivers/miyoo/gpio_joykeybd      /dev/joykeyb
/bin/rundev /drivers/miyoo/gpio_joykeyb_minid /dev/joykeyb
/bin/rundev /drivers/miyoo/gpio_joystickd     /dev/joystick
/bin/rundev /drivers/miyoo/audctrl            /dev/sound
/bin/rundev /drivers/miyoo/fbd                /dev/fb0

/bin/rundev /drivers/fontd          /dev/font /usr/system/fonts/system.ttf
/bin/rundev /drivers/consoled       /dev/console0

/bin/rundev /drivers/timerd         /dev/timer
/bin/rundev /drivers/nulld          /dev/null
/bin/rundev /drivers/ramfsd         /tmp
/bin/rundev /drivers/proc/sysinfod  /proc/sysinfo
/bin/rundev /drivers/proc/stated    /proc/state

/bin/rundev /drivers/displayd       /dev/display /dev/fb0
/bin/rundev /drivers/xserverd       /dev/x

#/bin/rundev /drivers/xconsoled      /dev/console0

@/sbin/x/xim_none   /dev/joykeyb &
@/sbin/x/xjoystickd /dev/joystick &
@/sbin/x/xim_vkey 560 160&

@/bin/x/launcher &
@/bin/session &