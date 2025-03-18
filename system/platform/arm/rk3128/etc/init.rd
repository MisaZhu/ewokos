@/bin/ipcserv /drivers/rk3128/rk_uartd       /dev/tty0

@/bin/ipcserv /drivers/rk3128/fbd            /dev/fb0
@/bin/ipcserv /drivers/displayd             
@/bin/ipcserv /drivers/fontd                

@export UX_ID=0
@/bin/ipcserv /drivers/consoled             /dev/klog
@export KLOG_DEV=/dev/klog
@set_stdio /dev/klog

@/bin/ipcserv /drivers/rk3128/gpio_joystickd  /dev/joystick
@/bin/ipcserv /drivers/vjoystickd             /dev/vjoystick /dev/joystick

@/bin/ipcserv /drivers/timerd               
@/bin/ipcserv /drivers/nulld                /dev/null
@/bin/ipcserv /drivers/ramfsd               /tmp

@/bin/ipcserv /sbin/sessiond
#@/bin/bgrun /bin/session -r 

@/bin/bgrun /sbin/x/xim_none   /dev/vjoystick 
@/bin/bgrun /sbin/x/xmouse     /dev/vjoystick 

#@/bin/load_font
@/bin/ipcserv /drivers/xserverd             /dev/x
@/bin/bgrun /sbin/x/xim_vkey 560 168
@/bin/bgrun /bin/x/xsession misa 
