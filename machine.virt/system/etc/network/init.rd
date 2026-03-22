@export TZ=CST-8

@/bin/ipcserv /drivers/logd /dev/log

@/bin/ipcserv /drivers/virt/ttyd /dev/tty0

@/bin/ipcserv /drivers/timerd          
@/bin/ipcserv /drivers/ramfsd          /tmp
@/bin/ipcserv /drivers/nulld           /dev/null

@/bin/ipcserv /drivers/virt/net /dev/eth0
@/bin/ipcserv /drivers/netd /dev/net0 /dev/eth0
@/bin/ipcserv /drivers/timed    /dev/time

@/bin/ipcserv /sbin/sessiond
@/bin/bgrun /bin/session -r -t /dev/tty0 