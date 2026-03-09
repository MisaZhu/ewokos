@/bin/ipcserv /drivers/logd  /dev/log

@export TZ=CST-8

@/bin/ipcserv /drivers/virt/ttyd /dev/tty0
@/bin/ipcserv /sbin/sessiond
@/bin/bgrun /bin/session -r -t /dev/tty0 
