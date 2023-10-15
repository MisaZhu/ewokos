/bin/rundev /drivers/timerd               /dev/timer
/bin/rundev /drivers/nulld                /dev/null
/bin/rundev /drivers/ramfsd               /tmp
/bin/rundev /drivers/proc/sysinfod        /proc/sysinfo
/bin/rundev /drivers/proc/stated           /proc/state

/bin/rundev /drivers/displayd             /dev/display /dev/fb0
#/bin/rundev /drivers/fontd                /dev/font /user/system/fonts/system.ttf /user/system/fonts/system_cn.ttf
/bin/rundev /drivers/fontd                /dev/font /user/system/fonts/system.ttf
/bin/rundev /drivers/xserverd             /dev/x
/bin/rundev /drivers/xconsoled             /dev/console0