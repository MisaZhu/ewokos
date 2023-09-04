/*
    Copyright (c) 2021-2022 jdeokkim

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "ferox.h"

/* | `timer` 모듈 매크로 정의... | */

#if defined(_WIN32)
    #define NOGDI
    #define NOUSER
#endif

#define SOKOL_TIME_IMPL
#include "sokol_time.h"

/* | `timer` 모듈 변수... | */

/* 단조 시계의 초기화 여부를 나타내는 변수. */
static bool initialized = false;

/* | `timer` 모듈 함수... | */

/* 단조 시계를 초기화한다. */
void frInitClock(void) {
    if (!initialized) {
        initialized = true;
        stm_setup();
    }
} 

/* 단조 시계의 현재 시각 (단위: ms)을 반환한다. */
double frGetCurrentTime(void) {
    if (!initialized) frInitClock();
    
    return stm_ms(stm_now());
}

/* 단조 시계의 시각 `newTime`과 `oldTime`의 차이를 반환한다. */
double frGetTimeDifference(double newTime, double oldTime) {
    return newTime - oldTime;
}

/* 단조 시계의 현재 시각과 `old_time`과의 차이를 반환한다. */
double frGetTimeSince(double oldTime) {
    return frGetTimeDifference(frGetCurrentTime(), oldTime);
}
