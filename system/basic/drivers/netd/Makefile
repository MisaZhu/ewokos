include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

NETD_OBJS = netd.o intr.o sched.o tap.o\
				stack/loopback.o stack/icmp.o stack/net.o stack/ether.o stack/arp.o stack/ip.o stack/udp.o stack/tcp.o  stack/util.o stack/sock.o 

NETD = $(TARGET_DIR)/drivers/netd

$(NETD): $(NETD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(NETD_OBJS) -o $(NETD) $(LDFLAGS) -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(NETD_OBJS)
