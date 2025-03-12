@/bin/ipcserv /drivers/ev3/fbd      /dev/fb0
@/bin/ipcserv /drivers/displayd        
@/bin/ipcserv /drivers/fontd           

@export UX_ID=0
@/bin/ipcserv /drivers/consoled        -u 0
@set_stdio /dev/console0

@/bin/ipcserv /drivers/ev3/gpio_joystickd     /dev/joystick

@/bin/ipcserv /drivers/vjoystickd               /dev/vjoystick /dev/joystick
@/bin/ipcserv /drivers/joymoused               /dev/mouse0 /dev/vjoystick

@/bin/ipcserv /drivers/timerd         

@/bin/ipcserv /sbin/sessiond

@/bin/ipcserv /drivers/xserverd       /dev/x
@/bin/bgrun /sbin/x/xim_none   /dev/vjoystick 
@/bin/bgrun /sbin/x/xmouse    /dev/mouse0 

@/bin/bgrun /bin/x/xsession misa 
