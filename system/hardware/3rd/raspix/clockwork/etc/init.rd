/bin/ipcserv /drivers/clockwork/fbd         /dev/fb0
/bin/ipcserv /drivers/displayd              /dev/display /dev/fb0
/bin/ipcserv /drivers/fontd                 /dev/font

/bin/ipcserv /drivers/consoled              /dev/console0
@set_stdio /dev/console0

/bin/ipcserv /drivers/clockwork/powerd      /dev/power0
/bin/ipcserv /drivers/raspix/uartd          /dev/tty0
/bin/ipcserv /drivers/timerd                /dev/timer

/bin/ipcserv /drivers/clockwork/usbd        /dev/hid0
/bin/ipcserv /drivers/raspix/hid_keybd      /dev/keyb0
/bin/ipcserv /drivers/raspix/hid_moused     /dev/mouse0
/bin/ipcserv /drivers/raspix/hid_joystickd  /dev/joystick0

/bin/ipcserv /drivers/ramfsd                /tmp
/bin/ipcserv /drivers/nulld                 /dev/null
/bin/ipcserv /drivers/proc/sysinfod         /proc/sysinfo
/bin/ipcserv /drivers/proc/stated           /proc/state

#/bin/ipcserv /drivers/raspix/wland          /dev/wl0
#/bin/ipcserv /drivers/netd                  /dev/net0 /dev/wl0
#@/bin/telnetd &

/bin/ipcserv /sbin/sessiond
#/bin/session -r &

#/bin/load_font
/bin/ipcserv /drivers/xserverd              /dev/x

@/sbin/x/xjoystickd /dev/joystick0 &
@/sbin/x/xmoused /dev/mouse0 &
@/sbin/x/xim_none &

@/bin/x/xsession misa &
