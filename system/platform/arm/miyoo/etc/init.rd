@/bin/ipcserv /drivers/miyoo/fbd      /dev/fb0
@/bin/ipcserv /drivers/displayd       
@/bin/ipcserv /drivers/fontd          

@export UX_ID=0
@/bin/ipcserv /drivers/init_consoled  /dev/klog -l 8
@export KLOG_DEV=/dev/klog
@set_stdio /dev/klog

#@/bin/ipcserv /drivers/miyoo/ms_uartd /dev/tty0

@/bin/ipcserv /drivers/miyoo/gpio_joystickd     /dev/joystick
@/bin/ipcserv /drivers/vjoystickd               /dev/vjoystick /dev/joystick
#@/bin/ipcserv /drivers/miyoo/audctrl            /dev/sound

@/bin/ipcserv /drivers/timerd         
@/bin/ipcserv /drivers/nulld          /dev/null
@/bin/ipcserv /drivers/ramfsd         /tmp

@/bin/ipcserv /sbin/sessiond
#@/bin/bgrun /bin/session -r -t /dev/tty0 

@/bin/ipcserv /drivers/xserverd       /dev/x
@/bin/bgrun /sbin/x/xim_none   /dev/vjoystick 
@/bin/bgrun /sbin/x/xmouse    /dev/vjoystick 
@/bin/bgrun /sbin/x/xim_vkey 460 120

#@/bin/load_font

@/bin/bgrun /bin/x/xsession misa 
