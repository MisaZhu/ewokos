#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <ewoksys/keydef.h>
#include <ewoksys/vdevice.h>
#include <x/xwin.h>

static void input(int32_t x_pid, char c, int state) {
	xevent_t ev;
	ev.type = XEVT_IM;
	ev.value.im.value = c;
	ev.state = state;

	proto_t in;
	PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
	dev_cntl_by_pid(x_pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

static void prompt(void) {
	printf( "type '~' to exit. 'ESC' as HOME.\n"
			"+----------------------------------------+\n"
			"|                                        |\n"
			"|        [up]                   [x]      |\n"
			"|                                        |\n"
			"|  [left]    [right]        [y]     [a]  |\n"
			"|                                        |\n"
			"|       [down]                  [b]      |\n"
			"|                                        |\n"
			"+----------------------------------------+\n");
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int32_t x_pid = dev_get_pid("/dev/x");
	if(x_pid < 0) {
		printf("Error: no X found!\n");
		return -1;
	}
	prompt();

	while(1) {
		uint8_t ks[3];
		int32_t sz = read(0, ks, 3);
		if(sz > 0) {
			uint8_t c = ks[0];
			if(sz == 3) {
				c = ks[2];
				if(c == 65)
					input(x_pid, KEY_UP, XIM_STATE_PRESS);
				else if(c == 66)
					input(x_pid, KEY_DOWN, XIM_STATE_PRESS);
				else if(c == 68)
					input(x_pid, KEY_LEFT, XIM_STATE_PRESS);
				else if(c == 67)
					input(x_pid, KEY_RIGHT, XIM_STATE_PRESS);
				continue;
			}

			if(c == '~')
				break;

			if(c == 27) //esc
				c = KEY_HOME;
			else if(c == 'a')
				c = KEY_BUTTON_A;
			else if(c == 'b')
				c = KEY_BUTTON_B;
			else if(c == 'x')
				c = KEY_BUTTON_X;
			else if(c == 'y')
				c = KEY_BUTTON_Y;
			else if(c == 's')
				c = KEY_BUTTON_START;
			else if(c == 'l')
				c = KEY_BUTTON_SELECT;
			input(x_pid, c, XIM_STATE_PRESS);
			input(x_pid, c, XIM_STATE_RELEASE);
		}
		proc_usleep(100000);
	}
	close(x_pid);
	return 0;
}
