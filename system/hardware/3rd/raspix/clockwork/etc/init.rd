#/bin/ipcserv /drivers/raspix/soundd  /dev/sound

/bin/ipcserv /drivers/raspix/clockwork_fbd      /dev/fb0
/bin/ipcserv /drivers/displayd              /dev/display /dev/fb0
/bin/ipcserv /drivers/fontd           /dev/font
/bin/ipcserv /drivers/consoled        /dev/console0

/bin/ipcserv /drivers/timerd                /dev/timer
/bin/ipcserv /drivers/raspix/clockwork_usbd /dev/hid0
/bin/ipcserv /drivers/raspix/hid_keybd      /dev/keyb0
/bin/ipcserv /drivers/raspix/hid_moused     /dev/mouse0
/bin/ipcserv /drivers/raspix/hid_joystickd  /dev/joystick0

/bin/ipcserv /drivers/ramfsd                /tmp
@/sbin/sessiond &

/bin/ipcserv /drivers/nulld                 /dev/null
/bin/ipcserv /drivers/proc/sysinfod         /proc/sysinfo
/bin/ipcserv /drivers/proc/stated           /proc/state

/bin/ipcserv /drivers/xserverd              /dev/x
/bin/ipcserv /sbin/x/xwm_opencde

#/bin/ipcserv /drivers/xconsoled             /dev/console0

@/sbin/x/xjoystickd /dev/joystick0 &
@/sbin/x/xmoused /dev/mouse0 &
@/sbin/x/xim_none &

@/bin/session -r &
@/bin/x/menubar &
@/bin/x/launcher &
