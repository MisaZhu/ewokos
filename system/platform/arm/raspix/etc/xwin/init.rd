/bin/ipcserv /drivers/raspix/uartd         /dev/tty0
/bin/ipcserv /sbin/sessiond
/bin/session -r -t /dev/tty0 &

/bin/ipcserv /drivers/raspix/fbd      /dev/fb0
/bin/ipcserv /drivers/displayd        
/bin/ipcserv /drivers/fontd           

export UX_ID=0
/bin/ipcserv /drivers/consoled        
@set_stdio /dev/console0

/bin/ipcserv /drivers/timerd          

/bin/ipcserv /drivers/raspix/usbd        /dev/hid0
/bin/ipcserv /drivers/raspix/hid_keybd   /dev/keyb0

export UX_ID=1
/bin/ipcserv /drivers/consoled        -i /dev/keyb0
@/bin/session -r -t /dev/console1 &

/bin/ipcserv /drivers/ramfsd          /tmp
/bin/ipcserv /drivers/nulld           /dev/null

/sbin/x/xim_none &

#/bin/load_font
/bin/ipcserv /drivers/xserverd        /dev/x

@/bin/x/xsession  misa &
