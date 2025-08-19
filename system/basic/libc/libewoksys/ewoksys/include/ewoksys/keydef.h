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

#define KEY_CTRL            0xF2
#define KEY_LSHIFT          0xA2
#define KEY_RSHIFT          0xA3

#define JOYSTICK_A        97
#define JOYSTICK_B        98
#define JOYSTICK_L1       102
#define JOYSTICK_L2       104
#define JOYSTICK_MODE     110
#define JOYSTICK_R1       103
#define JOYSTICK_R2       105
#define JOYSTICK_SELECT   109
#define JOYSTICK_START    108
#define JOYSTICK_THUMBL   106
#define JOYSTICK_THUMBR   107

#define JOYSTICK_X        120
#define JOYSTICK_Y        121

#define JOYSTICK_UP        KEY_UP 
#define JOYSTICK_DOWN      KEY_DOWN
#define JOYSTICK_LEFT      KEY_LEFT
#define JOYSTICK_RIGHT     KEY_RIGHT
#define JOYSTICK_PRESS     KEY_ENTER

#ifdef __cplusplus
}
#endif

#endif
