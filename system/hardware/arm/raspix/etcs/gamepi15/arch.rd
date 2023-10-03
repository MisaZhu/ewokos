/bin/rundev /drivers/raspix/lcdhatd       /dev/fb0 240 240 3
/bin/rundev /drivers/raspix/uartd         /dev/tty0
#/bin/rundev /drivers/raspix/mbox_actledd  /dev/actled

#left button as joystick pins: up down left right press
/bin/rundev /drivers/raspix/gpio_joystickd  /dev/joystick 5 6 13 16 19 

#right button as keyboard pins: up down left right press
/bin/rundev /drivers/raspix/gpio_joykeybd   /dev/joykeyb  12 21 15 20 26 
