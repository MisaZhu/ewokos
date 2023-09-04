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

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

/* | `utils` 모듈 매크로 정의... | */

#define TWO_PI (2.0f * PI)

/* | `utils` 모듈 상수... | */

static const float INVERSE_TWO_PI = (1.0f / TWO_PI);

/* | `utils` 모듈 함수... | */

/* 각도 `angle` (단위: rad.)을 정규화하여, 구간 `[center - π, center + π]`에 포함되도록 한다. */
float frNormalizeAngle(float angle, float center) {
    if (angle > center - PI && angle < center + PI) return angle;

    return angle - (TWO_PI * floorf((angle + PI - center) * INVERSE_TWO_PI));
}

/* 부동 소수점 값 `f1`이 `f2`와 근접한 값인지 확인한다. */
bool frNumberApproxEquals(float f1, float f2) {
    return fabsf(f1 - f2) <= fmaxf(1.0f, fmaxf(f1, f2)) * FLT_EPSILON;
}