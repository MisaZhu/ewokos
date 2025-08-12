
#ifndef __KEY_CODE_H__
#define __KEY_CODE_H__
#include <stdint.h>
#include <unistd.h>



/**
 * Modifier masks - used for the first byte in the HID report.
 * NOTE: The second byte in the report is reserved, 0x00
 */
#define KEY_MOD_LCTRL  0x01
#define KEY_MOD_LSHIFT 0x02
#define KEY_MOD_LALT   0x04
#define KEY_MOD_LMETA  0x08
#define KEY_MOD_RCTRL  0x10
#define KEY_MOD_RSHIFT 0x20
#define KEY_MOD_RALT   0x40
#define KEY_MOD_RMETA  0x80

/**
 * Scan codes - last N slots in the HID report (usually 6).
 * 0x00 if no key pressed.
 * 
 * If more than N keys are pressed, the HID reports 
 * KEY_ERR_OVF in all slots to indicate this condition.
 */

#define KEY_NONE 0x00 // No key pressed
#define KEY_ERR_OVF 0x01 //  Keyboard Error Roll Over - used for all slots if too many keys are pressed ("Phantom key")
// 0x02 //  Keyboard POST Fail
// 0x03 //  Keyboard Error Undefined
#define KEY_A 0x04 // Keyboard a and A
#define KEY_B 0x05 // Keyboard b and B
#define KEY_C 0x06 // Keyboard c and C
#define KEY_D 0x07 // Keyboard d and D
#define KEY_E 0x08 // Keyboard e and E
#define KEY_F 0x09 // Keyboard f and F
#define KEY_G 0x0a // Keyboard g and G
#define KEY_H 0x0b // Keyboard h and H
#define KEY_I 0x0c // Keyboard i and I
#define KEY_J 0x0d // Keyboard j and J
#define KEY_K 0x0e // Keyboard k and K
#define KEY_L 0x0f // Keyboard l and L
#define KEY_M 0x10 // Keyboard m and M
#define KEY_N 0x11 // Keyboard n and N
#define KEY_O 0x12 // Keyboard o and O
#define KEY_P 0x13 // Keyboard p and P
#define KEY_Q 0x14 // Keyboard q and Q
#define KEY_R 0x15 // Keyboard r and R
#define KEY_S 0x16 // Keyboard s and S
#define KEY_T 0x17 // Keyboard t and T
#define KEY_U 0x18 // Keyboard u and U
#define KEY_V 0x19 // Keyboard v and V
#define KEY_W 0x1a // Keyboard w and W
#define KEY_X 0x1b // Keyboard x and X
#define KEY_Y 0x1c // Keyboard y and Y
#define KEY_Z 0x1d // Keyboard z and Z

#define KEY_1 0x1e // Keyboard 1 and !
#define KEY_2 0x1f // Keyboard 2 and @
#define KEY_3 0x20 // Keyboard 3 and #
#define KEY_4 0x21 // Keyboard 4 and $
#define KEY_5 0x22 // Keyboard 5 and %
#define KEY_6 0x23 // Keyboard 6 and ^
#define KEY_7 0x24 // Keyboard 7 and &
#define KEY_8 0x25 // Keyboard 8 and *
#define KEY_9 0x26 // Keyboard 9 and (
#define KEY_0 0x27 // Keyboard 0 and )

#define KEY_ENTER 0x28 // Keyboard Return (ENTER)
#define KEY_ESC 0x29 // Keyboard ESCAPE
#define KEY_BACKSPACE 0x2a // Keyboard DELETE (Backspace)
#define KEY_TAB 0x2b // Keyboard Tab
#define KEY_SPACE 0x2c // Keyboard Spacebar
#define KEY_MINUS 0x2d // Keyboard - and _
#define KEY_EQUAL 0x2e // Keyboard = and +
#define KEY_LEFTBRACE 0x2f // Keyboard [ and {
#define KEY_RIGHTBRACE 0x30 // Keyboard ] and }
#define KEY_BACKSLASH 0x31 // Keyboard \ and |
#define KEY_HASHTILDE 0x32 // Keyboard Non-US # and ~
#define KEY_SEMICOLON 0x33 // Keyboard ; and :
#define KEY_APOSTROPHE 0x34 // Keyboard ' and "
#define KEY_GRAVE 0x35 // Keyboard ` and ~
#define KEY_COMMA 0x36 // Keyboard , and <
#define KEY_DOT 0x37 // Keyboard . and >
#define KEY_SLASH 0x38 // Keyboard / and ?
#define KEY_CAPSLOCK 0x39 // Keyboard Caps Lock

#define KEY_F1 0x3a // Keyboard F1
#define KEY_F2 0x3b // Keyboard F2
#define KEY_F3 0x3c // Keyboard F3
#define KEY_F4 0x3d // Keyboard F4
#define KEY_F5 0x3e // Keyboard F5
#define KEY_F6 0x3f // Keyboard F6
#define KEY_F7 0x40 // Keyboard F7
#define KEY_F8 0x41 // Keyboard F8
#define KEY_F9 0x42 // Keyboard F9
#define KEY_F10 0x43 // Keyboard F10
#define KEY_F11 0x44 // Keyboard F11
#define KEY_F12 0x45 // Keyboard F12

#define KEY_SYSRQ 0x46 // Keyboard Print Screen
#define KEY_SCROLLLOCK 0x47 // Keyboard Scroll Lock
#define KEY_PAUSE 0x48 // Keyboard Pause
#define KEY_INSERT 0x49 // Keyboard Insert
#define KEY_HOME 0x4a // Keyboard Home
#define KEY_PAGEUP 0x4b // Keyboard Page Up
#define KEY_DELETE 0x4c // Keyboard Delete Forward
#define KEY_END 0x4d // Keyboard End
#define KEY_PAGEDOWN 0x4e // Keyboard Page Down
#define KEY_RIGHT 0x4f // Keyboard Right Arrow
#define KEY_LEFT 0x50 // Keyboard Left Arrow
#define KEY_DOWN 0x51 // Keyboard Down Arrow
#define KEY_UP 0x52 // Keyboard Up Arrow

#define KEY_NUMLOCK 0x53 // Keyboard Num Lock and Clear
#define KEY_KPSLASH 0x54 // Keypad /
#define KEY_KPASTERISK 0x55 // Keypad *
#define KEY_KPMINUS 0x56 // Keypad -
#define KEY_KPPLUS 0x57 // Keypad +
#define KEY_KPENTER 0x58 // Keypad ENTER
#define KEY_KP1 0x59 // Keypad 1 and End
#define KEY_KP2 0x5a // Keypad 2 and Down Arrow
#define KEY_KP3 0x5b // Keypad 3 and PageDn
#define KEY_KP4 0x5c // Keypad 4 and Left Arrow
#define KEY_KP5 0x5d // Keypad 5
#define KEY_KP6 0x5e // Keypad 6 and Right Arrow
#define KEY_KP7 0x5f // Keypad 7 and Home
#define KEY_KP8 0x60 // Keypad 8 and Up Arrow
#define KEY_KP9 0x61 // Keypad 9 and Page Up
#define KEY_KP0 0x62 // Keypad 0 and Insert
#define KEY_KPDOT 0x63 // Keypad . and Delete

#define KEY_102ND 0x64 // Keyboard Non-US \ and |
#define KEY_COMPOSE 0x65 // Keyboard Application
#define KEY_POWER 0x66 // Keyboard Power
#define KEY_KPEQUAL 0x67 // Keypad =

#define KEY_F13 0x68 // Keyboard F13
#define KEY_F14 0x69 // Keyboard F14
#define KEY_F15 0x6a // Keyboard F15
#define KEY_F16 0x6b // Keyboard F16
#define KEY_F17 0x6c // Keyboard F17
#define KEY_F18 0x6d // Keyboard F18
#define KEY_F19 0x6e // Keyboard F19
#define KEY_F20 0x6f // Keyboard F20
#define KEY_F21 0x70 // Keyboard F21
#define KEY_F22 0x71 // Keyboard F22
#define KEY_F23 0x72 // Keyboard F23
#define KEY_F24 0x73 // Keyboard F24

#define KEY_OPEN 0x74 // Keyboard Execute
#define KEY_HELP 0x75 // Keyboard Help
#define KEY_PROPS 0x76 // Keyboard Menu
#define KEY_FRONT 0x77 // Keyboard Select
#define KEY_STOP 0x78 // Keyboard Stop
#define KEY_AGAIN 0x79 // Keyboard Again
#define KEY_UNDO 0x7a // Keyboard Undo
#define KEY_CUT 0x7b // Keyboard Cut
#define KEY_COPY 0x7c // Keyboard Copy
#define KEY_PASTE 0x7d // Keyboard Paste
#define KEY_FIND 0x7e // Keyboard Find
#define KEY_MUTE 0x7f // Keyboard Mute
#define KEY_VOLUMEUP 0x80 // Keyboard Volume Up
#define KEY_VOLUMEDOWN 0x81 // Keyboard Volume Down
// 0x82  Keyboard Locking Caps Lock
// 0x83  Keyboard Locking Num Lock
// 0x84  Keyboard Locking Scroll Lock
#define KEY_KPCOMMA 0x85 // Keypad Comma
// 0x86  Keypad Equal Sign
#define KEY_RO 0x87 // Keyboard International1
#define KEY_KATAKANAHIRAGANA 0x88 // Keyboard International2
#define KEY_YEN 0x89 // Keyboard International3
#define KEY_HENKAN 0x8a // Keyboard International4
#define KEY_MUHENKAN 0x8b // Keyboard International5
#define KEY_KPJPCOMMA 0x8c // Keyboard International6
// 0x8d  Keyboard International7
// 0x8e  Keyboard International8
// 0x8f  Keyboard International9
#define KEY_HANGEUL 0x90 // Keyboard LANG1
#define KEY_HANJA 0x91 // Keyboard LANG2
#define KEY_KATAKANA 0x92 // Keyboard LANG3
#define KEY_HIRAGANA 0x93 // Keyboard LANG4
#define KEY_ZENKAKUHANKAKU 0x94 // Keyboard LANG5
// 0x95  Keyboard LANG6
// 0x96  Keyboard LANG7
// 0x97  Keyboard LANG8
// 0x98  Keyboard LANG9
// 0x99  Keyboard Alternate Erase
// 0x9a  Keyboard SysReq/Attention
// 0x9b  Keyboard Cancel
// 0x9c  Keyboard Clear
// 0x9d  Keyboard Prior
// 0x9e  Keyboard Return
// 0x9f  Keyboard Separator
// 0xa0  Keyboard Out
// 0xa1  Keyboard Oper
// 0xa2  Keyboard Clear/Again
// 0xa3  Keyboard CrSel/Props
// 0xa4  Keyboard ExSel

// 0xb0  Keypad 00
// 0xb1  Keypad 000
// 0xb2  Thousands Separator
// 0xb3  Decimal Separator
// 0xb4  Currency Unit
// 0xb5  Currency Sub-unit
#define KEY_KPLEFTPAREN 0xb6 // Keypad (
#define KEY_KPRIGHTPAREN 0xb7 // Keypad )
// 0xb8  Keypad {
// 0xb9  Keypad }
// 0xba  Keypad Tab
// 0xbb  Keypad Backspace
// 0xbc  Keypad A
// 0xbd  Keypad B
// 0xbe  Keypad C
// 0xbf  Keypad D
// 0xc0  Keypad E
// 0xc1  Keypad F
// 0xc2  Keypad XOR
// 0xc3  Keypad ^
// 0xc4  Keypad %
// 0xc5  Keypad <
// 0xc6  Keypad >
// 0xc7  Keypad &
// 0xc8  Keypad &&
// 0xc9  Keypad |
// 0xca  Keypad ||
// 0xcb  Keypad :
// 0xcc  Keypad #
// 0xcd  Keypad Space
// 0xce  Keypad @
// 0xcf  Keypad !
// 0xd0  Keypad Memory Store
// 0xd1  Keypad Memory Recall
// 0xd2  Keypad Memory Clear
// 0xd3  Keypad Memory Add
// 0xd4  Keypad Memory Subtract
// 0xd5  Keypad Memory Multiply
// 0xd6  Keypad Memory Divide
// 0xd7  Keypad +/-
// 0xd8  Keypad Clear
// 0xd9  Keypad Clear Entry
// 0xda  Keypad Binary
// 0xdb  Keypad Octal
// 0xdc  Keypad Decimal
// 0xdd  Keypad Hexadecimal

#define KEY_LEFTCTRL 0xe0 // Keyboard Left Control
#define KEY_LEFTSHIFT 0xe1 // Keyboard Left Shift
#define KEY_LEFTALT 0xe2 // Keyboard Left Alt
#define KEY_LEFTMETA 0xe3 // Keyboard Left GUI
#define KEY_RIGHTCTRL 0xe4 // Keyboard Right Control
#define KEY_RIGHTSHIFT 0xe5 // Keyboard Right Shift
#define KEY_RIGHTALT 0xe6 // Keyboard Right Alt
#define KEY_RIGHTMETA 0xe7 // Keyboard Right GUI

#define KEY_MEDIA_PLAYPAUSE 0xe8
#define KEY_MEDIA_STOPCD 0xe9
#define KEY_MEDIA_PREVIOUSSONG 0xea
#define KEY_MEDIA_NEXTSONG 0xeb
#define KEY_MEDIA_EJECTCD 0xec
#define KEY_MEDIA_VOLUMEUP 0xed
#define KEY_MEDIA_VOLUMEDOWN 0xee
#define KEY_MEDIA_MUTE 0xef
#define KEY_MEDIA_WWW 0xf0
#define KEY_MEDIA_BACK 0xf1
#define KEY_MEDIA_FORWARD 0xf2
#define KEY_MEDIA_STOP 0xf3
#define KEY_MEDIA_FIND 0xf4
#define KEY_MEDIA_SCROLLUP 0xf5
#define KEY_MEDIA_SCROLLDOWN 0xf6
#define KEY_MEDIA_EDIT 0xf7
#define KEY_MEDIA_SLEEP 0xf8
#define KEY_MEDIA_COFFEE 0xf9
#define KEY_MEDIA_REFRESH 0xfa
#define KEY_MEDIA_CALC 0xfb


uint8_t KEY_A_MAKE[] = {1, 0x1C};
uint8_t KEY_B_MAKE[] = {1, 0x32};
uint8_t KEY_C_MAKE[] = {1, 0x21};
uint8_t KEY_D_MAKE[] = {1, 0x23};
uint8_t KEY_E_MAKE[] = {1, 0x24};
uint8_t KEY_F_MAKE[] = {1, 0x2B};
uint8_t KEY_G_MAKE[] = {1, 0x34};
uint8_t KEY_H_MAKE[] = {1, 0x33};
uint8_t KEY_I_MAKE[] = {1, 0x43};
uint8_t KEY_J_MAKE[] = {1, 0x3B};
uint8_t KEY_K_MAKE[] = {1, 0x42};
uint8_t KEY_L_MAKE[] = {1, 0x4B};
uint8_t KEY_M_MAKE[] = {1, 0x3A};
uint8_t KEY_N_MAKE[] = {1, 0x31};
uint8_t KEY_O_MAKE[] = {1, 0x44};
uint8_t KEY_P_MAKE[] = {1, 0x4D};
uint8_t KEY_Q_MAKE[] = {1, 0x15};
uint8_t KEY_R_MAKE[] = {1, 0x2D};
uint8_t KEY_S_MAKE[] = {1, 0x1B};
uint8_t KEY_T_MAKE[] = {1, 0x2C};
uint8_t KEY_U_MAKE[] = {1, 0x3C};
uint8_t KEY_V_MAKE[] = {1, 0x2A};
uint8_t KEY_W_MAKE[] = {1, 0x1D};
uint8_t KEY_X_MAKE[] = {1, 0x22};
uint8_t KEY_Y_MAKE[] = {1, 0x35};
uint8_t KEY_Z_MAKE[] = {1, 0x1A};
uint8_t KEY_0_MAKE[] = {1, 0x45};
uint8_t KEY_1_MAKE[] = {1, 0x16};
uint8_t KEY_2_MAKE[] = {1, 0x1E};
uint8_t KEY_3_MAKE[] = {1, 0x26};
uint8_t KEY_4_MAKE[] = {1, 0x25};
uint8_t KEY_5_MAKE[] = {1, 0x2E};
uint8_t KEY_6_MAKE[] = {1, 0x36};
uint8_t KEY_7_MAKE[] = {1, 0x3D};
uint8_t KEY_8_MAKE[] = {1, 0x3E};
uint8_t KEY_9_MAKE[] = {1, 0x46};
uint8_t KEY_TILDE_MAKE[] = {1, 0x0E};
uint8_t KEY_DASH_MAKE[] = {1, 0x4E};
uint8_t KEY_EQUAL_MAKE[] = {1, 0x55};
uint8_t KEY_BKSLASH_MAKE[] = {1, 0x5D};
uint8_t KEY_BKSPACE_MAKE[] = {1, 0x66};
uint8_t KEY_SPACE_MAKE[] = {1, 0x29};
uint8_t KEY_TAB_MAKE[] = {1, 0x0D};
uint8_t KEY_CAPS_MAKE[] = {1, 0x58};
uint8_t KEY_LSHIFT_MAKE[] = {1, 0x12};
uint8_t KEY_LCTRL_MAKE[] = {1, 0x14};
uint8_t KEY_LGUI_MAKE[] = {2, 0xE0, 0x1F};
uint8_t KEY_LALT_MAKE[] = {1, 0x11};
uint8_t KEY_RSHIFT_MAKE[] = {1, 0x59};
uint8_t KEY_RCTRL_MAKE[] = {2, 0xE0, 0x14};
uint8_t KEY_RGUI_MAKE[] = {2, 0xE0, 0x27};
uint8_t KEY_RALT_MAKE[] = {2, 0xE0, 0x11};
uint8_t KEY_APPS_MAKE[] = {2, 0xE0, 0x2F};
uint8_t KEY_ENTER_MAKE[] = {1, 0x5A};
uint8_t KEY_ESC_MAKE[] = {1, 0x76};
uint8_t KEY_F1_MAKE[] = {1, 0x05};
uint8_t KEY_F2_MAKE[] = {1, 0x06};
uint8_t KEY_F3_MAKE[] = {1, 0x04};
uint8_t KEY_F4_MAKE[] = {1, 0x0C};
uint8_t KEY_F5_MAKE[] = {1, 0x03};
uint8_t KEY_F6_MAKE[] = {1, 0x0B};
uint8_t KEY_F7_MAKE[] = {1, 0x83};
uint8_t KEY_F8_MAKE[] = {1, 0x0A};
uint8_t KEY_F9_MAKE[] = {1, 0x01};
uint8_t KEY_F10_MAKE[] = {1, 0x09};
uint8_t KEY_F11_MAKE[] = {1, 0x78};
uint8_t KEY_F12_MAKE[] = {1, 0x07};
uint8_t KEY_PRTSC_MAKE[] = {4, 0xE0, 0x12, 0xE0, 0x7C};
uint8_t KEY_SCROLL_MAKE[] = {1, 0x7E};
uint8_t KEY_PAUSE_MAKE[] = {8, 0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77};
uint8_t KEY_LEFTSQB_MAKE[] = {1, 0x54};
uint8_t KEY_INSERT_MAKE[] = {2, 0xE0, 0x70};
uint8_t KEY_HOME_MAKE[] = {2, 0xE0, 0x6C};
uint8_t KEY_PGUP_MAKE[] = {2, 0xE0, 0x7D};
uint8_t KEY_DELETE_MAKE[] = {2, 0xE0, 0x71};
uint8_t KEY_END_MAKE[] = {2, 0xE0, 0x69};
uint8_t KEY_PGDN_MAKE[] = {2, 0xE0, 0x7A};
uint8_t KEY_UP_MAKE[] = {2, 0xE0, 0x75};
uint8_t KEY_LEFT_MAKE[] = {2, 0xE0, 0x6B};
uint8_t KEY_DOWN_MAKE[] = {2, 0xE0, 0x72};
uint8_t KEY_RIGHT_MAKE[] = {2, 0xE0, 0x74};
uint8_t KEY_NUM_MAKE[] = {1, 0x77};
uint8_t KEY_PADFWSLASH_MAKE[] = {2, 0xE0, 0x4A};
uint8_t KEY_PADASTERISK_MAKE[] = {1, 0x7C};
uint8_t KEY_PADMINUS_MAKE[] = {1, 0x7B};
uint8_t KEY_PADPLUS_MAKE[] = {1, 0x79};
uint8_t KEY_PADEN_MAKE[] = {2, 0xE0, 0x5A};
uint8_t KEY_PADDOT_MAKE[] = {1, 0x71};
uint8_t KEY_PAD0_MAKE[] = {1, 0x70};
uint8_t KEY_PAD1_MAKE[] = {1, 0x69};
uint8_t KEY_PAD2_MAKE[] = {1, 0x71};
uint8_t KEY_PAD3_MAKE[] = {1, 0x7A};
uint8_t KEY_PAD4_MAKE[] = {1, 0x6B};
uint8_t KEY_PAD5_MAKE[] = {1, 0x73};
uint8_t KEY_PAD6_MAKE[] = {1, 0x74};
uint8_t KEY_PAD7_MAKE[] = {1, 0x6C};
uint8_t KEY_PAD8_MAKE[] = {1, 0x75};
uint8_t KEY_PAD9_MAKE[] = {1, 0x7D};
uint8_t KEY_RIGHTSQB_MAKE[] = {1, 0x5B};
uint8_t KEY_SEMICOLON_MAKE[] = {1, 0x4C};
uint8_t KEY_APOSTROPHE_MAKE[] = {1, 0x52};
uint8_t KEY_COMMA_MAKE[] = {1, 0x41};
uint8_t KEY_PERIOD_MAKE[] = {1, 0x49};
uint8_t KEY_FWSLASH_MAKE[] = {1, 0x4A};
uint8_t KEY_EURO1_MAKE[] = {1, 0x5D};
uint8_t KEY_EURO2_MAKE[] = {1, 0x61};

uint8_t KEY_A_BREAK[] = {2, 0xF0, 0x1C};
uint8_t KEY_B_BREAK[] = {2, 0xF0, 0x32};
uint8_t KEY_C_BREAK[] = {2, 0xF0, 0x21};
uint8_t KEY_D_BREAK[] = {2, 0xF0, 0x23};
uint8_t KEY_E_BREAK[] = {2, 0xF0, 0x24};
uint8_t KEY_F_BREAK[] = {2, 0xF0, 0x2B};
uint8_t KEY_G_BREAK[] = {2, 0xF0, 0x34};
uint8_t KEY_H_BREAK[] = {2, 0xF0, 0x33};
uint8_t KEY_I_BREAK[] = {2, 0xF0, 0x43};
uint8_t KEY_J_BREAK[] = {2, 0xF0, 0x3B};
uint8_t KEY_K_BREAK[] = {2, 0xF0, 0x42};
uint8_t KEY_L_BREAK[] = {2, 0xF0, 0x4B};
uint8_t KEY_M_BREAK[] = {2, 0xF0, 0x3A};
uint8_t KEY_N_BREAK[] = {2, 0xF0, 0x31};
uint8_t KEY_O_BREAK[] = {2, 0xF0, 0x44};
uint8_t KEY_P_BREAK[] = {2, 0xF0, 0x4D};
uint8_t KEY_Q_BREAK[] = {2, 0xF0, 0x15};
uint8_t KEY_R_BREAK[] = {2, 0xF0, 0x2D};
uint8_t KEY_S_BREAK[] = {2, 0xF0, 0x1B};
uint8_t KEY_T_BREAK[] = {2, 0xF0, 0x2C};
uint8_t KEY_U_BREAK[] = {2, 0xF0, 0x3C};
uint8_t KEY_V_BREAK[] = {2, 0xF0, 0x2A};
uint8_t KEY_W_BREAK[] = {2, 0xF0, 0x1D};
uint8_t KEY_X_BREAK[] = {2, 0xF0, 0x22};
uint8_t KEY_Y_BREAK[] = {2, 0xF0, 0x35};
uint8_t KEY_Z_BREAK[] = {2, 0xF0, 0x1A};
uint8_t KEY_0_BREAK[] = {2, 0xF0, 0x45};
uint8_t KEY_1_BREAK[] = {2, 0xF0, 0x16};
uint8_t KEY_2_BREAK[] = {2, 0xF0, 0x1E};
uint8_t KEY_3_BREAK[] = {2, 0xF0, 0x26};
uint8_t KEY_4_BREAK[] = {2, 0xF0, 0x25};
uint8_t KEY_5_BREAK[] = {2, 0xF0, 0x2E};
uint8_t KEY_6_BREAK[] = {2, 0xF0, 0x36};
uint8_t KEY_7_BREAK[] = {2, 0xF0, 0x3D};
uint8_t KEY_8_BREAK[] = {2, 0xF0, 0x3E};
uint8_t KEY_9_BREAK[] = {2, 0xF0, 0x46};
uint8_t KEY_TILDE_BREAK[] = {2, 0xF0, 0x0E};
uint8_t KEY_DASH_BREAK[] = {2, 0xF0, 0x4E};
uint8_t KEY_EQUAL_BREAK[] = {2, 0xF0, 0x55};
uint8_t KEY_BKSLASH_BREAK[] = {2, 0xF0, 0x5D};
uint8_t KEY_BKSPACE_BREAK[] = {2, 0xF0, 0x66};
uint8_t KEY_SPACE_BREAK[] = {2, 0xF0, 0x29};
uint8_t KEY_TAB_BREAK[] = {2, 0xF0, 0x0D};
uint8_t KEY_CAPS_BREAK[] = {2, 0xF0, 0x58};
uint8_t KEY_LSHIFT_BREAK[] = {2, 0xF0, 0x12};
uint8_t KEY_LCTRL_BREAK[] = {2, 0xF0, 0x14};
uint8_t KEY_LGUI_BREAK[] = {3, 0xE0, 0xF0, 0x1F};
uint8_t KEY_LALT_BREAK[] = {2, 0xF0, 0x11};
uint8_t KEY_RSHIFT_BREAK[] = {2, 0xF0, 0x59};
uint8_t KEY_RCTRL_BREAK[] = {3, 0xE0, 0xF0, 0x14};
uint8_t KEY_RGUI_BREAK[] = {3, 0xE0, 0xF0, 0x27};
uint8_t KEY_RALT_BREAK[] = {3, 0xE0, 0xF0, 0x11};
uint8_t KEY_APPS_BREAK[] = {3, 0xE0, 0xF0, 0x2F};
uint8_t KEY_ENTER_BREAK[] = {2, 0xF0, 0x5A};
uint8_t KEY_ESC_BREAK[] = {2, 0xF0, 0x76};
uint8_t KEY_F1_BREAK[] = {2, 0xF0, 0x05};
uint8_t KEY_F2_BREAK[] = {2, 0xF0, 0x06};
uint8_t KEY_F3_BREAK[] = {2, 0xF0, 0x04};
uint8_t KEY_F4_BREAK[] = {2, 0xF0, 0x0C};
uint8_t KEY_F5_BREAK[] = {2, 0xF0, 0x03};
uint8_t KEY_F6_BREAK[] = {2, 0xF0, 0x0B};
uint8_t KEY_F7_BREAK[] = {2, 0xF0, 0x83};
uint8_t KEY_F8_BREAK[] = {2, 0xF0, 0x0A};
uint8_t KEY_F9_BREAK[] = {2, 0xF0, 0x01};
uint8_t KEY_F10_BREAK[] = {2, 0xF0, 0x09};
uint8_t KEY_F11_BREAK[] = {2, 0xF0, 0x78};
uint8_t KEY_F12_BREAK[] = {2, 0xF0, 0x07};
uint8_t KEY_PRTSC_BREAK[] = {4, 0xE0, 0xF0, 0x7C, 0xE0, 0xF0, 0x12};
uint8_t KEY_SCROLL_BREAK[] = {2, 0xF0, 0x7E};
uint8_t KEY_PAUSE_BREAK[] = {8, 0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77};
uint8_t KEY_LEFTSQB_BREAK[] = {2, 0xF0, 0x54};
uint8_t KEY_INSERT_BREAK[] = {3, 0xE0, 0xF0, 0x70};
uint8_t KEY_HOME_BREAK[] = {3, 0xE0, 0xF0, 0x6C};
uint8_t KEY_PGUP_BREAK[] = {3, 0xE0, 0xF0, 0x7D};
uint8_t KEY_DELETE_BREAK[] = {3, 0xE0, 0xF0, 0x71};
uint8_t KEY_END_BREAK[] = {3, 0xE0, 0xF0, 0x69};
uint8_t KEY_PGDN_BREAK[] = {3, 0xE0, 0xF0, 0x7A};
uint8_t KEY_UP_BREAK[] = {3, 0xE0, 0xF0, 0x75};
uint8_t KEY_LEFT_BREAK[] = {3, 0xE0, 0xF0, 0x6B};
uint8_t KEY_DOWN_BREAK[] = {3, 0xE0, 0xF0, 0x72};
uint8_t KEY_RIGHT_BREAK[] = {3, 0xE0, 0xF0, 0x74};
uint8_t KEY_NUM_BREAK[] = {2, 0xF0, 0x77};
uint8_t KEY_PADFWSLASH_BREAK[] = {3, 0xE0, 0xF0, 0x4A};
uint8_t KEY_PADASTERISK_BREAK[] = {2, 0xF0, 0x7C};
uint8_t KEY_PADMINUS_BREAK[] = {2, 0xF0, 0x7B};
uint8_t KEY_PADPLUS_BREAK[] = {2, 0xF0, 0x79};
uint8_t KEY_PADEN_BREAK[] = {3, 0xE0, 0xF0, 0x5A};
uint8_t KEY_PADDOT_BREAK[] = {2, 0xF0, 0x71};
uint8_t KEY_PAD0_BREAK[] = {2, 0xF0, 0x70};
uint8_t KEY_PAD1_BREAK[] = {2, 0xF0, 0x69};
uint8_t KEY_PAD2_BREAK[] = {2, 0xF0, 0x71};
uint8_t KEY_PAD3_BREAK[] = {2, 0xF0, 0x7A};
uint8_t KEY_PAD4_BREAK[] = {2, 0xF0, 0x6B};
uint8_t KEY_PAD5_BREAK[] = {2, 0xF0, 0x73};
uint8_t KEY_PAD6_BREAK[] = {2, 0xF0, 0x74};
uint8_t KEY_PAD7_BREAK[] = {2, 0xF0, 0x6C};
uint8_t KEY_PAD8_BREAK[] = {2, 0xF0, 0x75};
uint8_t KEY_PAD9_BREAK[] = {2, 0xF0, 0x7D};
uint8_t KEY_RIGHTSQB_BREAK[] = {2, 0xF0, 0x5B};
uint8_t KEY_SEMICOLON_BREAK[] = {2, 0xF0, 0x4C};
uint8_t KEY_APOSTROPHE_BREAK[] = {2, 0xF0, 0x52};
uint8_t KEY_COMMA_BREAK[] = {2, 0xF0, 0x41};
uint8_t KEY_PERIOD_BREAK[] = {2, 0xF0, 0x49};
uint8_t KEY_FWSLASH_BREAK[] = {2, 0xF0, 0x4A};
uint8_t KEY_EURO1_BREAK[] = {2, 0xF0, 0x5D};
uint8_t KEY_EURO2_BREAK[] = {2, 0xF0, 0x61};

uint8_t KEY_ACK[] = {1, 0xFA};
uint8_t KEY_BATCOMPLETE[] = {1, 0xAA};
uint8_t KEY_ID[] = {2, 0xAB, 0x83};

uint8_t KEY_SCANCODE_2[] = {1, 0x02};
uint8_t KEY_ECHO[] = {1, 0xEE};
uint8_t KEY_ERROR[] = {1, 0xFE};

const uint8_t *const ModtoPS2_MAKE[] =
{
        KEY_LCTRL_MAKE,
        KEY_LSHIFT_MAKE,
        KEY_LALT_MAKE,
        KEY_LGUI_MAKE,
        KEY_RCTRL_MAKE,
        KEY_RSHIFT_MAKE,
        KEY_RALT_MAKE,
        KEY_RGUI_MAKE
};

const uint8_t *const ModtoPS2_BREAK[] =
{
        KEY_LCTRL_BREAK,
        KEY_LSHIFT_BREAK,
        KEY_LALT_BREAK,
        KEY_LGUI_BREAK,
        KEY_RCTRL_BREAK,
        KEY_RSHIFT_BREAK,
        KEY_RALT_BREAK,
        KEY_RGUI_BREAK
};

const uint8_t *const HIDtoPS2_Make[] =
    {
        NULL,
        NULL,
        NULL,
        NULL,
        KEY_A_MAKE,
        KEY_B_MAKE,
        KEY_C_MAKE,
        KEY_D_MAKE,
        KEY_E_MAKE,
        KEY_F_MAKE,
        KEY_G_MAKE,
        KEY_H_MAKE,
        KEY_I_MAKE,
        KEY_J_MAKE,
        KEY_K_MAKE,
        KEY_L_MAKE,
        KEY_M_MAKE, // 10
        KEY_N_MAKE,
        KEY_O_MAKE,
        KEY_P_MAKE,
        KEY_Q_MAKE,
        KEY_R_MAKE,
        KEY_S_MAKE,
        KEY_T_MAKE,
        KEY_U_MAKE,
        KEY_V_MAKE,
        KEY_W_MAKE,
        KEY_X_MAKE,
        KEY_Y_MAKE,
        KEY_Z_MAKE,
        KEY_1_MAKE,
        KEY_2_MAKE,
        KEY_3_MAKE, //20
        KEY_4_MAKE,
        KEY_5_MAKE,
        KEY_6_MAKE,
        KEY_7_MAKE,
        KEY_8_MAKE,
        KEY_9_MAKE,
        KEY_0_MAKE,
        KEY_ENTER_MAKE,
        KEY_ESC_MAKE,
        KEY_BKSPACE_MAKE,
        KEY_TAB_MAKE,
        KEY_SPACE_MAKE,
        KEY_DASH_MAKE,
        KEY_EQUAL_MAKE,
        KEY_LEFTSQB_MAKE,
        KEY_RIGHTSQB_MAKE, //30
        KEY_BKSLASH_MAKE,
        KEY_EURO1_MAKE,
        KEY_SEMICOLON_MAKE,
        KEY_APOSTROPHE_MAKE,
        KEY_TILDE_MAKE,
        KEY_COMMA_MAKE,
        KEY_PERIOD_MAKE,
        KEY_FWSLASH_MAKE,
        KEY_CAPS_MAKE,
        KEY_F1_MAKE,
        KEY_F2_MAKE,
        KEY_F3_MAKE,
        KEY_F4_MAKE,
        KEY_F5_MAKE,
        KEY_F6_MAKE,
        KEY_F7_MAKE, //40
        KEY_F8_MAKE,
        KEY_F9_MAKE,
        KEY_F10_MAKE,
        KEY_F11_MAKE,
        KEY_F12_MAKE,
        KEY_PRTSC_MAKE,
        KEY_SCROLL_MAKE,
        KEY_PAUSE_MAKE,
        KEY_INSERT_MAKE,
        KEY_HOME_MAKE,
        KEY_PGUP_MAKE,
        KEY_DELETE_MAKE,
        KEY_END_MAKE,
        KEY_PGDN_MAKE,
        KEY_RIGHT_MAKE,
        KEY_LEFT_MAKE, // 50
        KEY_DOWN_MAKE,
        KEY_UP_MAKE,
        KEY_NUM_MAKE,
        KEY_PADFWSLASH_MAKE,
        KEY_PADASTERISK_MAKE,
        KEY_PADMINUS_MAKE,
        KEY_PADPLUS_MAKE,
        KEY_PADEN_MAKE,
        KEY_PAD1_MAKE,
        KEY_PAD2_MAKE,
        KEY_PAD3_MAKE,
        KEY_PAD4_MAKE,
        KEY_PAD5_MAKE,
        KEY_PAD6_MAKE,
        KEY_PAD7_MAKE,
        KEY_PAD8_MAKE, // 60
        KEY_PAD9_MAKE,
        KEY_PAD0_MAKE,
        KEY_PADDOT_MAKE,
        KEY_EURO2_MAKE,
        KEY_APPS_MAKE,
        NULL,
        KEY_EQUAL_MAKE
};


const uint8_t *const HIDtoPS2_Break[] =
{
        NULL,
        NULL,
        NULL,
        NULL,
        KEY_A_BREAK,
        KEY_B_BREAK,
        KEY_C_BREAK,
        KEY_D_BREAK,
        KEY_E_BREAK,
        KEY_F_BREAK,
        KEY_G_BREAK,
        KEY_H_BREAK,
        KEY_I_BREAK,
        KEY_J_BREAK,
        KEY_K_BREAK,
        KEY_L_BREAK,
        KEY_M_BREAK, // 10
        KEY_N_BREAK,
        KEY_O_BREAK,
        KEY_P_BREAK,
        KEY_Q_BREAK,
        KEY_R_BREAK,
        KEY_S_BREAK,
        KEY_T_BREAK,
        KEY_U_BREAK,
        KEY_V_BREAK,
        KEY_W_BREAK,
        KEY_X_BREAK,
        KEY_Y_BREAK,
        KEY_Z_BREAK,
        KEY_1_BREAK,
        KEY_2_BREAK,
        KEY_3_BREAK, //20
        KEY_4_BREAK,
        KEY_5_BREAK,
        KEY_6_BREAK,
        KEY_7_BREAK,
        KEY_8_BREAK,
        KEY_9_BREAK,
        KEY_0_BREAK,
        KEY_ENTER_BREAK,
        KEY_ESC_BREAK,
        KEY_BKSPACE_BREAK,
        KEY_TAB_BREAK,
        KEY_SPACE_BREAK,
        KEY_DASH_BREAK,
        KEY_EQUAL_BREAK,
        KEY_LEFTSQB_BREAK,
        KEY_RIGHTSQB_BREAK, //30
        KEY_BKSLASH_BREAK,
        KEY_EURO1_BREAK,
        KEY_SEMICOLON_BREAK,
        KEY_APOSTROPHE_BREAK,
        KEY_TILDE_BREAK,
        KEY_COMMA_BREAK,
        KEY_PERIOD_BREAK,
        KEY_FWSLASH_BREAK,
        KEY_CAPS_BREAK,
        KEY_F1_BREAK,     // = {1, 0x05};
        KEY_F2_BREAK,     // = {1, 0x06};
        KEY_F3_BREAK,     // = {1, 0x04};
        KEY_F4_BREAK,     // = {1, 0x0C};
        KEY_F5_BREAK,     // = {1, 0x03};
        KEY_F6_BREAK,     // = {1, 0x0B};
        KEY_F7_BREAK,     // = {1, 0x83}; 40
        KEY_F8_BREAK,     // = {1, 0x0A};
        KEY_F9_BREAK,     // = {1, 0x01};
        KEY_F10_BREAK,    // = {1, 0x09};
        KEY_F11_BREAK,    // = {1, 0x78};
        KEY_F12_BREAK,    // = {1, 0x07};
        KEY_PRTSC_BREAK,  // = {4, 0xE0, 0x12, 0xE0, 0x7C};
        KEY_SCROLL_BREAK, // = {1, 0x7E};
        KEY_PAUSE_BREAK,  // = {8, 0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77};
        KEY_INSERT_BREAK, // = {2, 0xE0, 0x70};
        KEY_HOME_BREAK,   // = {2, 0xE0, 0x6C};
        KEY_PGUP_BREAK,   // = {2, 0xE0, 0x7D};
        KEY_DELETE_BREAK, // = {2, 0xE0, 0x71};
        KEY_END_BREAK,    // = {2, 0xE0, 0x69};
        KEY_PGDN_BREAK,   // = {2, 0xE0, 0x7A};
        KEY_RIGHT_BREAK,
        KEY_LEFT_BREAK, // 50
        KEY_DOWN_BREAK,
        KEY_UP_BREAK,
        KEY_NUM_BREAK,         // = {1, 0x77};
        KEY_PADFWSLASH_BREAK,  // = {2, 0xE0, 0x4A};
        KEY_PADASTERISK_BREAK, // = {1, 0x7C};
        KEY_PADMINUS_BREAK,    // = {1, 0x7B};
        KEY_PADPLUS_BREAK,     // = {1, 0x79};
        KEY_PADEN_BREAK,       // = {2, 0xE0, 0x5A};
        KEY_PAD1_BREAK,        // = {1, 0x69};
        KEY_PAD2_BREAK,        // = {1, 0x71};
        KEY_PAD3_BREAK,        // = {1, 0x7A};
        KEY_PAD4_BREAK,        // = {1, 0x6B};
        KEY_PAD5_BREAK,        // = {1, 0x73};
        KEY_PAD6_BREAK,        // = {1, 0x74};
        KEY_PAD7_BREAK,        // = {1, 0x6C};
        KEY_PAD8_BREAK,        // = {1, 0x75}; 60
        KEY_PAD9_BREAK,        // = {1, 0x7D};
        KEY_PAD0_BREAK,        // = {1, 0x70};
        KEY_PADDOT_BREAK,
        KEY_EURO2_BREAK,
        KEY_APPS_BREAK,
        NULL,
        KEY_EQUAL_BREAK
};
#endif