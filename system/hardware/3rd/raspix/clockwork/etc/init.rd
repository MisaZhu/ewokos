/bin/rundev /drivers/raspix/powerd  /dev/power0
/bin/rundev /drivers/raspix/uartd   /dev/tty0
#/bin/rundev /drivers/raspix/soundd  /dev/sound

/bin/rundev /drivers/raspix/clockwork_fbd      /dev/fb0
/bin/rundev /drivers/displayd              /dev/display /dev/fb0
#/bin/rundev /drivers/fontd           /dev/font /usr/system/fonts/system.ttf /usr/system/fonts/system_cn.ttf
/bin/rundev /drivers/fontd           /dev/font /usr/system/fonts/system.ttf
/bin/rundev /drivers/consoled        /dev/console0

/bin/rundev /drivers/raspix/clockwork_usbd /dev/hid0
/bin/rundev /drivers/raspix/hid_keybd      /dev/keyb0
/bin/rundev /drivers/raspix/hid_moused     /dev/mouse0
/bin/rundev /drivers/raspix/hid_joystickd  /dev/joystick0

/bin/rundev /drivers/timerd                /dev/timer
/bin/rundev /drivers/ramfsd                /tmp

/bin/rundev /drivers/nulld                 /dev/null
/bin/rundev /drivers/proc/sysinfod         /proc/sysinfo
/bin/rundev /drivers/proc/stated           /proc/state

/bin/rundev /drivers/xserverd              /dev/x

#/bin/rundev /drivers/xconsoled             /dev/console0

@/sbin/x/xjoystickd /dev/joystick0 &
@/sbin/x/xmoused /dev/mouse0 &
@/sbin/x/xim_none &

@/bin/session &
@/bin/x/menubar &
@/bin/x/launcher &
