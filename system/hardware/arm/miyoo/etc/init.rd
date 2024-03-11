/bin/ipcserv /drivers/miyoo/gpio_joystickd     /dev/joystick
/bin/ipcserv /drivers/vjoystickd               /dev/vjoystick /dev/joystick
#/bin/ipcserv /drivers/miyoo/audctrl            /dev/sound

/bin/ipcserv /drivers/miyoo/fbd                /dev/fb0
/bin/ipcserv /drivers/displayd       /dev/display /dev/fb0
/bin/ipcserv /drivers/fontd          /dev/font /usr/system/fonts/system.ttf

/bin/ipcserv /drivers/timerd         /dev/timer
/bin/ipcserv /drivers/nulld          /dev/null
/bin/ipcserv /drivers/ramfsd         /tmp
/bin/ipcserv /drivers/proc/sysinfod  /proc/sysinfo
/bin/ipcserv /drivers/proc/stated    /proc/state

/bin/ipcserv /drivers/xserverd       /dev/x

@/sbin/sessiond &
@/bin/session -r &

@/sbin/x/xim_none   /dev/vjoystick &
@/sbin/x/xjoymoused /dev/vjoystick &
@/sbin/x/xim_vkey 460 120&
@/bin/x/xsession &
