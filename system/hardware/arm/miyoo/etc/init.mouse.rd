/bin/ipcserv /drivers/miyoo/gpio_joykeyb_minid /dev/joykeyb
/bin/ipcserv /drivers/miyoo/gpio_joystickd     /dev/joystick

/bin/ipcserv /drivers/miyoo/fbd                /dev/fb0
/bin/ipcserv /drivers/displayd       /dev/display /dev/fb0
/bin/ipcserv /drivers/fontd          /dev/font

/bin/ipcserv /drivers/timerd         /dev/timer
/bin/ipcserv /drivers/nulld          /dev/null
/bin/ipcserv /drivers/ramfsd         /tmp
/bin/ipcserv /drivers/proc/sysinfod  /proc/sysinfo
/bin/ipcserv /drivers/proc/stated    /proc/state

@/sbin/sessiond &
@/bin/session -r &

/bin/ipcserv /drivers/xserverd       /dev/x
@/sbin/x/xim_none   /dev/joykeyb &
@/sbin/x/xjoymoused /dev/joystick &
@/sbin/x/xim_vkey 560 160&
@/bin/x/xsession &
