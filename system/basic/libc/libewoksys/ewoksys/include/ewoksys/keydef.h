#ifndef KEYDEF_H
#define KEYDEF_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONSOLE_LEFT        8
#define KEY_TAB             9
#define KEY_ENTER           13
#define KEY_ESC             27
#define KEY_SPACE           32

#define KEY_BACKSPACE       127

#define KEY_RIGHT           4
#define KEY_UP              5
#define KEY_LEFT            19
#define KEY_DOWN            24

#define KEY_POWER           26
#define KEY_HOME            0xF0 
#define KEY_END             0xF1

#define KEY_CTRL            0x11
#define KEY_LSHIFT          0xA2
#define KEY_RSHIFT          0xA3

#define JOYSTICK_A        0xE2
#define JOYSTICK_B        0xE3
#define JOYSTICK_L1       0xE4
#define JOYSTICK_L2       0xE5
#define JOYSTICK_MODE     0xE6
#define JOYSTICK_R1       0xE7
#define JOYSTICK_R2       0xE8
#define JOYSTICK_SELECT   0xE9
#define JOYSTICK_START    0xEA 
#define JOYSTICK_THUMBL   0xEB
#define JOYSTICK_THUMBR   0xEC

#define JOYSTICK_X        0xED
#define JOYSTICK_Y        0xEE

#define JOYSTICK_UP        KEY_UP 
#define JOYSTICK_DOWN      KEY_DOWN
#define JOYSTICK_LEFT      KEY_LEFT
#define JOYSTICK_RIGHT     KEY_RIGHT
#define JOYSTICK_PRESS     KEY_ENTER

#ifdef __cplusplus
}
#endif

#endif
