SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

NETD_OBJS = $(ARCH)/netd.o $(ARCH)/intr.o $(ARCH)/sched.o $(ARCH)/tap.o $(ARCH)/task.o\
				$(ARCH)/stack/loopback.o	\
				$(ARCH)/stack/icmp.o \
				$(ARCH)/stack/net.o \
				$(ARCH)/stack/ether.o \
				$(ARCH)/stack/arp.o \
				$(ARCH)/stack/ip.o \
				$(ARCH)/stack/udp.o \
				$(ARCH)/stack/tcp.o  \
				$(ARCH)/stack/util.o \
				$(ARCH)/stack/sock.o \
				$(ARCH)/stack/dhcp.o 

NETD = $(TARGET_DIR)/drivers/netd

$(NETD): $(NETD_OBJS) \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(NETD_OBJS) -o $(NETD) $(LDFLAGS) -ltinyjson $(EWOK_LIBC)

clean:
	rm -f $(NETD_OBJS)
