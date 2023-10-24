/bin/rundev /drivers/raspix/uartd    /dev/tty0
#/bin/rundev /drivers/raspix/soundd   /dev/sound
/bin/rundev /drivers/raspix/usbd          /dev/touch0
/bin/rundev /drivers/raspix/fbd      /dev/fb0

/bin/rundev /drivers/fontd           /dev/font /usr/system/fonts/system.ttf
/bin/rundev /drivers/consoled        /dev/console0

/bin/rundev /drivers/timerd               /dev/timer
/bin/rundev /drivers/ramfsd               /tmp

#/bin/rundev /drivers/nulld                /dev/null
#/bin/rundev /drivers/proc/sysinfod        /proc/sysinfo
#/bin/rundev /drivers/proc/stated          /proc/state

/bin/rundev /drivers/displayd        /dev/display /dev/fb0
/bin/rundev /drivers/xserverd        /dev/x

#/bin/rundev /drivers/xconsoled       /dev/console0

#@/sbin/x/xmoused /dev/mouse0 &
@/sbin/x/xim_none &
@/sbin/x/xim_vkey 600 160&

@/bin/session &
@/bin/x/launcher &