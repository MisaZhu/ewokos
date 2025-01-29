/bin/ipcserv /drivers/versatilepb/ttyd       /dev/tty0
@set_stdio /dev/tty0

/bin/ipcserv /drivers/timerd                 /dev/timer

/bin/ipcserv /drivers/nulld                  /dev/null
/bin/ipcserv /drivers/ramfsd                 /tmp

/bin/ipcserv /sbin/sessiond
/bin/session -r &