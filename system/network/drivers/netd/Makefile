SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

NETD_OBJS = netd.o intr.o sched.o tap.o task.o\
				stack/loopback.o	\
				stack/icmp.o \
				stack/net.o \
				stack/ether.o \
				stack/arp.o \
				stack/ip.o \
				stack/udp.o \
				stack/tcp.o  \
				stack/util.o \
				stack/sock.o \
				stack/dhcp.o 

NETD = $(TARGET_DIR)/drivers/netd

$(NETD): $(NETD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(NETD_OBJS) -o $(NETD) $(LDFLAGS) -ltinyjson $(EWOK_LIBC)

clean:
	rm -f $(NETD_OBJS)
