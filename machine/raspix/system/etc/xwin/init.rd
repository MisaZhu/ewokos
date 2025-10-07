@/bin/ipcserv /drivers/logd /dev/log

@/bin/ipcserv /drivers/raspix/uartd         /dev/tty0
@/bin/ipcserv /sbin/sessiond
@/bin/bgrun /bin/session -r -t /dev/tty0 

@/bin/ipcserv /drivers/displayd        
@/bin/ipcserv /drivers/raspix/fbd      /dev/fb0
@/bin/ipcserv /drivers/fontd           

@export UX_ID=0
@/bin/ipcserv /drivers/consoled        
@set_stdio /dev/console0

@/bin/ipcserv /drivers/timerd          

@/bin/ipcserv /drivers/ramfsd          /tmp
@/bin/ipcserv /drivers/nulld           /dev/null

#@/bin/ipcserv /drivers/raspix/wland          /dev/wl0
#@/bin/ipcserv /drivers/netd                  /dev/net0 /dev/wl0
#@/bin/bgrun /sbin/telnetd

@/bin/load_font
@/bin/ipcserv /drivers/xserverd        /dev/x
@/bin/bgrun /bin/x/xsession  misa 
