/bin/rundev /drivers/versatilepb/ttyd       /dev/tty0
/bin/rundev /drivers/versatilepb/ps2keybd   /dev/keyb0
/bin/rundev /drivers/versatilepb/ps2moused  /dev/mouse0
/bin/rundev /drivers/versatilepb/smc91c111d /dev/eth0
/bin/rundev /drivers/versatilepb/fbd        /dev/fb0

/bin/rundev /drivers/fontd                  /dev/font /usr/system/fonts/system.ttf
/bin/rundev /drivers/consoled               /dev/console0

/bin/rundev /drivers/timerd                 /dev/timer
/bin/rundev /drivers/nulld                  /dev/null
/bin/rundev /drivers/ramfsd                 /tmp
/bin/rundev /drivers/proc/sysinfod          /proc/sysinfo
/bin/rundev /drivers/proc/stated            /proc/state

/bin/rundev /drivers/displayd               /dev/display /dev/fb0
/bin/rundev /drivers/xserverd               /dev/x

#/bin/rundev /drivers/xconsoled              /dev/console0

@/sbin/x/xmoused /dev/mouse0 &
@/sbin/x/xim_none /dev/keyb0 &

@/bin/x/launcher &
@/bin/session &