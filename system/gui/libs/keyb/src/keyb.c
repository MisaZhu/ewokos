#include <keyb/keyb.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define KEY_REPEAT_TIMEOUT	50
#define KEY_HOLD_TIMEOUT	100
#define KEY_TIMER	        3000 //300 ps

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

key_state_t _key_state[KEY_MAX_ONE_TIME];

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

static void key_state_machine(key_evt_handle_t handle_func, void* p){
    for(size_t i = 0; i < sizeof(_key_state)/sizeof(key_state_t); i++){
        key_state_t *ks = &_key_state[i];
        
        if(ks->key == 0)
            continue;

        if(ks->state == KEYB_STATE_RELEASE){
            handle_func(ks->key, KEYB_STATE_RELEASE, p);	
            ks->key = 0;
            ks->sm = KS_RELEASE;
            continue;
        }

        switch(ks->sm){
            case KS_IDLE:
                handle_func(ks->key, KEYB_STATE_PRESS, p);
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
                    handle_func(ks->key, KEYB_STATE_PRESS, p);	
                }
                break;
            default:
                ks->sm = KS_IDLE;
                break;
        }
        ks->state = KEYB_STATE_RELEASE;
    }	
}

void keyb_read(int keyb_fd, key_evt_handle_t handle_func, void* p) {
	char v[KEY_MAX_ONE_TIME] = { 0 };
	int rd = read(keyb_fd, v, KEY_MAX_ONE_TIME);
	update_key_state(v, rd);
	key_state_machine(handle_func, p);
    usleep(KEY_TIMER);
}

#ifdef __cplusplus
}
#endif