@/bin/ipcserv /drivers/orangepi/uartd /dev/tty0

@export KLOG_DEV=/dev/tty0
@set_stdio /dev/tty0

@echo "+---------------------------------------+\n"
@echo "|  < EwokOS MicroKernel >               |\n"
@echo "+---------------------------------------+\n"

@/bin/ipcserv /drivers/timerd
@/bin/ipcserv /drivers/nulld          /dev/null
@/bin/ipcserv /drivers/ramfsd         /tmp
/bin/shell
@/bin/bgrun /bin/session -r -t /dev/tty0
