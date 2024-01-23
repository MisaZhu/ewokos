#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <ewoksys/keydef.h>
#include <ewoksys/kernel_tic.h>
#include <x/xwin.h>
#include <string.h>
#include <ewoksys/timer.h>

#define KEY_REPEAT_TIMEOUT	60
#define KEY_HOLD_TIMEOUT	100
#define KEY_TIMER	        10000 //100 ps

typedef struct {
	uint8_t key;
	int state;
	int sm;
	uint64_t timer;
}KeyState_t;

enum {
	KS_IDLE = 0,
	KS_HOLD,
	KS_REPEAT,
	KS_RELEASE
};

class XIM {
	int x_pid;
	int keybFD;
	bool escHome;
	KeyState_t keyState[6];

	KeyState_t* match_key(uint8_t key){
		for(size_t i = 0; i < sizeof(keyState)/sizeof(KeyState_t); i++){
			if(keyState[i].key == key){
				return &keyState[i];
			}
		}

		for(size_t i = 0; i < sizeof(keyState)/sizeof(KeyState_t); i++){
			if(keyState[i].key == 0){
				keyState[i].key = key;
				keyState[i].sm = KS_IDLE;
				keyState[i].timer = 0;
				keyState[i].state = XIM_STATE_PRESS;
				return &keyState[i]; 
			}
		}
		return NULL;
	}

	void update_key_state(char *keys, int size){
		for(int i = 0; i < size ; i++){
			KeyState_t *ks = match_key(keys[i]);
			if(ks){
				ks->state = XIM_STATE_PRESS; 
			}
		}		
	}

	void key_state_machine(){
		for(size_t i = 0; i < sizeof(keyState)/sizeof(KeyState_t); i++){
			KeyState_t *ks = &keyState[i];
			
			if(ks->key == 0)
				continue;

			if(ks->state == XIM_STATE_RELEASE){
				input(ks->key, XIM_STATE_RELEASE);	
				ks->key = 0;
				ks->sm = KS_RELEASE;
				continue;
			}

			switch(ks->sm){
				case KS_IDLE:
					input(ks->key, XIM_STATE_PRESS);
					ks->sm = KS_HOLD;
					ks->timer = kernel_tic_ms(0);
					break;
				case KS_HOLD:
					if(kernel_tic_ms(0) - ks->timer > KEY_HOLD_TIMEOUT){
						ks->timer = kernel_tic_ms(0);
						ks->sm = KS_REPEAT;	
					}
					break;
				case KS_REPEAT:
					if(kernel_tic_ms(0) - ks->timer > KEY_REPEAT_TIMEOUT){
						ks->timer = kernel_tic_ms(0);
						input(ks->key, XIM_STATE_PRESS);	
					}
					break;
				default:
					ks->sm = KS_IDLE;
					break;
			}
			ks->state = XIM_STATE_RELEASE;
		}	
	}

	void input(char c, int state = XIM_STATE_PRESS) {
		xevent_t ev;
		ev.type = XEVT_IM;
		if((c == KEY_ESC || c == KEY_BUTTON_SELECT) && escHome)
			c = KEY_HOME;
		ev.value.im.value = c;
		ev.state = state;

		proto_t in;
		PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
		dev_cntl_by_pid(x_pid, X_DCNTL_INPUT, &in, NULL);
		PF->clear(&in);
	}


public:
	inline XIM(const char* keyb_dev, bool escHome) {
		x_pid = -1;
		keybFD = -1;
		this->escHome = escHome;

		for(size_t i = 0; i < sizeof(keyState)/sizeof(KeyState_t); i++){
			memset(&keyState[i], 0, sizeof(KeyState_t));
		}

		while(true) {
			keybFD = open(keyb_dev, O_RDONLY);
			if(keybFD > 0)
				break;
			proc_usleep(300000);
		}
	}

	inline ~XIM() {
		if(keybFD < 0)
			return;
		::close(keybFD);
	}

	void read(void) {
		if(x_pid < 0)
			x_pid = dev_get_pid("/dev/x");
		if(x_pid <= 0 || keybFD < 0)
			return;

		char v[6];
		int rd = ::read(keybFD, &v, 6);
		update_key_state(v, rd);
		key_state_machine();
	}
};

int main(int argc, char* argv[]) {
	const char* keyb_dev = "/dev/keyb0";
	if(argc > 1)
		keyb_dev = argv[1];

	bool escHome = false;
	if(argc > 2 && strcmp(argv[2], "esc_home") == 0) {
		escHome = true;
	}

	XIM xim(keyb_dev, escHome);
	while(true) {
		xim.read();
		proc_usleep(KEY_TIMER);
	}
	return 0;
}
