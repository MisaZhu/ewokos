#include <keyb/keyb.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define KEY_REPEAT_TIMEOUT	50
#define KEY_HOLD_TIMEOUT	150
#define KEY_TIMER	        10000 //100 fps

#define KEY_MAX_ONE_TIME    6

typedef struct {
	uint8_t key;
	int state;
	int sm;
	uint64_t timer;
}key_state_t;

enum {
	KS_IDLE = 0,
	KS_HOLD,
	KS_REPEAT,
	KS_RELEASE
};

key_state_t _key_state[KEYB_EVT_MAX];

static key_state_t* match_key(uint8_t key){
    for(size_t i = 0; i < sizeof(_key_state)/sizeof(key_state_t); i++){
        if(_key_state[i].key == key){
            return &_key_state[i];
        }
    }

    for(size_t i = 0; i < sizeof(_key_state)/sizeof(key_state_t); i++){
        if(_key_state[i].key == 0){
            _key_state[i].key = key;
            _key_state[i].sm = KS_IDLE;
            _key_state[i].timer = 0;
            _key_state[i].state = KEYB_STATE_PRESS;
            return &_key_state[i]; 
        }
    }
    return NULL;
}

static void update_key_state(char *keys, int size){
    for(int i = 0; i < size ; i++){
        key_state_t *ks = match_key(keys[i]);
        if(ks){
            ks->state = KEYB_STATE_PRESS; 
        }
    }		
}

static int key_state_machine(keyb_evt_t* evts, uint8_t num){
    uint8_t n = 0;
    for(uint8_t i = 0;
            i < sizeof(_key_state)/sizeof(key_state_t) && n < num;
            i++){
        key_state_t *ks = &_key_state[i];
        
        if(ks->key == 0)
            continue;

        if(ks->state == KEYB_STATE_RELEASE){
            evts[n].key = ks->key;
            evts[n].state = KEYB_STATE_RELEASE;	
            ks->key = 0;
            ks->sm = KS_RELEASE;
            n++;
            continue;
        }

        switch(ks->sm){
            case KS_IDLE:
                evts[n].key = ks->key;
                evts[n].state = KEYB_STATE_PRESS;	
                ks->sm = KS_HOLD;
                ks->timer = kernel_tic_ms(0);
                n++;
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
                    evts[n].key = ks->key;
                    evts[n].state = KEYB_STATE_PRESS;	
                    n++;
                }
                break;
            default:
                ks->sm = KS_IDLE;
                break;
        }
        ks->state = KEYB_STATE_RELEASE;
    }	
    return n;
}

int keyb_read(int keyb_fd, keyb_evt_t* evts, uint8_t num) {
	if(core_get_active_ux(0) != core_get_ux_env())
        return 0;

	char v[KEYB_EVT_MAX] = { 0 };
	int rd = read(keyb_fd, v, KEYB_EVT_MAX);
	update_key_state(v, rd);
	return key_state_machine(evts, num); 
}

#ifdef __cplusplus
}
#endif