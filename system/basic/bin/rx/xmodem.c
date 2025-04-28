/*	
 * Copyright 2001-2010 Georges Menie (www.menie.org)
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ewoksys/proc.h>


#include "crc16.h"

#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A

#define DLY_1S 200

int _inbyte(char* c, int timeout){
	int ret;
	while(timeout--){
		ret = read(0, c, 1);
		if( ret >= 0){
			return ret;
		}
		proc_usleep(5000);
	}
	return -1;
}

int _inbytes(unsigned char* c, int len,  int timeout){
	int ret;
	while(len > 0 && timeout > 0){
		ret = read(0, c, (len > 128)?128:len);
		if( ret > 0){
			len -= ret;
			c += ret;
			continue;
		}else{
			timeout--;
		}
		proc_usleep(5000);
	}
	return (timeout > 0)?0:-1;
}

 void _outbyte(char c){
	write(1, &c, 1);
 }

 int safe_write(char* data, int size, int fd){
	 while(size > 0){
		int wlen = (size > 1024)?1024:size; 
		int ret = write(fd , data, wlen);
		if(ret > 0){
			size -= ret;
			data += ret;
		}else{
			printf("error:%d\n", ret);
			return ret;
		}
	 }
	return 0;
 }

static int check(const unsigned char *buf, int sz)
{
		unsigned short crc = crc16_ccitt(buf, sz);
		unsigned short tcrc = (buf[sz]<<8)+buf[sz+1];
		if (crc == tcrc)
			return 1;
	return 0;
}

static void flushinput(void)
{
	char c;
	while (_inbyte(&c, DLY_1S) > 0);
}

int xmodemReceive(char* data)
{
	unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
	int bufsz;
	unsigned char trychar = 'C';
	unsigned char packetno = 1;
	int len = 0;
	char c;
	int retry;

	for(;;) {
		for( retry = 0; retry < 3; ++retry) {
			if (trychar) 
				_outbyte(trychar);
			if (_inbyte(&c, DLY_1S) >= 0) {
				switch (c) {
				case SOH:
					bufsz = 128;
					goto start_recv;
				case STX:
					bufsz = 1024;
					goto start_recv;
				case EOT:
					_outbyte(ACK);
					return len; /* normal end */
				case CAN:
					flushinput();
					_outbyte(ACK);
					return -1; /* canceled by remote */
				default:
					break;
				}
			}
		}
		_outbyte(CAN);
		return -2; /* sync error */

	start_recv:
		trychar = 0;
		if (_inbytes(xbuff + 1, bufsz + 4, DLY_1S) < 0){ 
			goto reject;
		}

		if (xbuff[1] == (unsigned char)(~xbuff[2]) && 
			(xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno-1) &&
			check(&xbuff[3], bufsz)) {
			if (xbuff[1] == packetno)	{
					memcpy(data, &xbuff[3], bufsz);
					len += bufsz;
					data += bufsz;
					++packetno;
			}
			_outbyte(ACK);
			continue;
		}
	reject:
		_outbyte(NAK);
	}
}

int main(int argc, char* argv[])
{
	int st;
	int flags = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, flags | O_NONBLOCK);

	if(argc < 2)
		return -1;

	int fd = open(argv[1], O_CREAT|O_WRONLY);
	if(fd < 0){
		printf("open file error: %d", fd);
		return -1;
	}
	
	char* buf = malloc(1024*1024);
	if(!buf){
			printf("Not enough memory\n");	
			close(fd);
	}
	st = xmodemReceive(buf);
	if (st < 0) {
		printf ("Xmodem receive error status: %d\n", st);
	}
	else  {
		printf ("Xmodem successfully received %d bytes\n", st);
		//safe_write(buf, st, fd);
		write(fd, buf, st );
	}
	close(fd);
	free(buf);
	return 0;
}

