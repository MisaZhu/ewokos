/bin/ipcserv /drivers/miyoo/fbd      /dev/fb0
/bin/ipcserv /drivers/displayd       /dev/display /dev/fb0
/bin/ipcserv /drivers/fontd          /dev/font

export UX_ID=0
/bin/ipcserv /drivers/consoled  /dev/klog
@export KLOG_DEV=/dev/klog
@set_stdio /dev/klog

#/bin/ipcserv /drivers/miyoo/ms_uartd /dev/tty0

/bin/ipcserv /drivers/miyoo/gpio_joystickd     /dev/joystick
/bin/ipcserv /drivers/vjoystickd               /dev/vjoystick /dev/joystick
#/bin/ipcserv /drivers/miyoo/audctrl            /dev/sound

/bin/ipcserv /drivers/timerd         /dev/timer
/bin/ipcserv /drivers/nulld          /dev/null
/bin/ipcserv /drivers/ramfsd         /tmp

/bin/ipcserv /sbin/sessiond
#/bin/session -r -t /dev/tty0 &

/bin/ipcserv /drivers/consoled  /dev/console1 -i /dev/joystick
/bin/session -r -t /dev/console1 &
/bin/setux 1

/bin/ipcserv /drivers/xserverd       /dev/x
/sbin/x/xim_none   /dev/vjoystick &
/sbin/x/xjoymoused /dev/vjoystick &
/sbin/x/xim_vkey 460 120&

#/bin/load_font

@/bin/x/xsession misa &
