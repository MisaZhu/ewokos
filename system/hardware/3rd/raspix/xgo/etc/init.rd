/bin/ipcserv /drivers/xgo/spilcdd    /dev/fb0
/bin/ipcserv /drivers/displayd       /dev/display /dev/fb0
/bin/ipcserv /drivers/fontd          /dev/font

/bin/ipcserv /drivers/consoled       0
@export INIT_OUT_DEV=/dev/console0

/bin/ipcserv /drivers/timerd         /dev/timer
#/bin/ipcserv /drivers/raspix/uartd   /dev/tty0
#/bin/ipcserv /drivers/xgo/xgo_buttond    /dev/keyb0

/bin/ipcserv /sbin/sessiond

/bin/ipcserv /drivers/xserverd       /dev/x

@/bin/x/xsession misa &
