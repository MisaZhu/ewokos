#
# Makefile
#
# USPi - An USB driver for Raspberry Pi written in C
# Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be src/useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

BUILD_DIR=../../../../build

OBJS	= src/uspilibrary.o \
	  src/dwhcidevice.o src/dwhciregister.o src/dwhcixferstagedata.o \
	  src/usbconfigparser.o src/usbdevice.o src/usbdevicefactory.o src/usbendpoint.o src/usbrequest.o src/usbstandardhub.o \
	  src/devicenameservice.o src/macaddress.o src/usbfunction.o src/smsc951x.o src/lan7800.o src/string.o src/util.o \
	  src/usbmassdevice.o \
	  src/dwhciframeschednper.o src/dwhciframeschedper.o src/keymap.o src/usbkeyboard.o \
	  src/dwhcirootport.o src/usbmouse.o \
	  src/dwhciframeschednsplit.o src/usbgamepad.o src/synchronize.o src/usbstring.o src/usbmidi.o

LIB_USPI=$(BUILD_DIR)/lib/libuspi.a

$(LIB_USPI): $(OBJS) headers
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

headers:
	cp -r include/* $(BUILD_DIR)/include

include Rules.mk

clean:
	rm -f $(LIB_USPI) $(OBJS)
