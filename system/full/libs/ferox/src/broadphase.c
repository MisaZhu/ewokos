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
#include "stb_ds.h"

/* | `broadphase` 모듈 자료형 정의... | */

/* 공간 해시맵의 키와 값을 나타내는 구조체. */
typedef struct frSpatialEntry {
    int key;
    int *values;
} frSpatialEntry;

/* 공간 해시맵을 나타내는 구조체. */
typedef struct frSpatialHash {
    Rectangle bounds;
    float cellSize;
    float inverseCellSize;
    frSpatialEntry *entries;
    int *queryCache;
} frSpatialHash;

/* | `broadphase` 모듈 함수... | */

/* 공간 해시맵 `hash`에서 벡터 `v`와 대응하는 키를 반환한다. */
static FR_API_INLINE int frComputeSpatialHashKey(frSpatialHash *hash, Vector2 v);

/* C 표준 라이브러리의 `qsort()` 함수 호출에 사용되는 비교 함수이다. */
static int frQuickSortCallback(const void *x, const void *y);

/* 경계 범위가 `bounds`이고 각 셀의 크기가 `cellSize`인 공간 해시맵의 메모리 주소를 반환한다. */
frSpatialHash *frCreateSpatialHash(Rectangle bounds, float cellSize) {
    if (cellSize <= 0.0f) return NULL;
    
    frSpatialHash *result = calloc(1, sizeof(*result));
    
    result->bounds = bounds;
    result->cellSize = cellSize;
    result->inverseCellSize = 1.0f / cellSize;
    
    return result;
}

/* 공간 해시맵 `hash`에 할당된 메모리를 해제한다. */
void frReleaseSpatialHash(frSpatialHash *hash) {
    if (hash == NULL) return; 
    
    for (int i = 0; i < hmlen(hash->entries); i++)
        arrfree(hash->entries[i].values);
    
    hmfree(hash->entries);

    free(hash);
}

/* 공간 해시맵 `hash`에 직사각형 `rec`로 생성한 키와 `value`를 추가한다. */
void frAddToSpatialHash(frSpatialHash *hash, Rectangle rec, int value) {
    if (hash == NULL || !CheckCollisionRecs(hash->bounds, rec)) return;
    
    int x0 = frComputeSpatialHashKey(hash, (Vector2) { .x = rec.x }); 
    int x1 = frComputeSpatialHashKey(hash, (Vector2) { .x = rec.x + rec.width });
    
    int y0 = frComputeSpatialHashKey(hash, (Vector2) { .y = rec.y });
    int y1 = frComputeSpatialHashKey(hash, (Vector2) { .y = rec.y + rec.height });
    
    for (int y = y0; y <= y1; y += hash->bounds.width) {
        for (int x = x0; x <= x1; x++) {
            const int key = x + y;

            frSpatialEntry *entry = hmgetp_null(hash->entries, key);
            
            if (entry != NULL) {
                arrput(entry->values, value);
            } else {
                frSpatialEntry newEntry = { .key = key };

                arrput(newEntry.values, value);
                hmputs(hash->entries, newEntry);
            }
        }
    }
}

/* 공간 해시맵 `hash`의 모든 키와 값을 제거한다. */
void frClearSpatialHash(frSpatialHash *hash) {
    if (hash == NULL) return;
    
    for (int i = 0; i < hmlen(hash->entries); i++)
        arrsetlen(hash->entries[i].values, 0);
}

/* 공간 해시맵 `hash`에서 키가 `key`인 값을 제거한다. */
void frRemoveFromSpatialHash(frSpatialHash *hash, int key) {
    if (hash == NULL) return;
    
    frSpatialEntry entry = hmgets(hash->entries, key);
    arrfree(entry.values);
    
    hmdel(hash->entries, key);
}

/* 공간 해시맵 `hash`에서 직사각형 `rec`와 경계 범위가 겹치는 모든 강체의 인덱스를 반환한다. */
void frQuerySpatialHash(frSpatialHash *hash, Rectangle rec, int **result) {
    if (hash == NULL || result == NULL || !CheckCollisionRecs(hash->bounds, rec)) return;
    
    int x0 = frComputeSpatialHashKey(hash, (Vector2) { .x = rec.x });
    int x1 = frComputeSpatialHashKey(hash, (Vector2) { .x = rec.x + rec.width });
    
    int y0 = frComputeSpatialHashKey(hash, (Vector2) { .y = rec.y });
    int y1 = frComputeSpatialHashKey(hash, (Vector2) { .y = rec.y + rec.height });

    const int yOffset = (int) hash->bounds.width;

    for (int y = y0; y <= y1; y += yOffset) {
        for (int x = x0; x <= x1; x++) {
            const int key = x + y;

            // 주어진 셀과 만나는 모든 강체의 인덱스를 구한다.
            frSpatialEntry *entry = hmgetp_null(hash->entries, key);

            if (entry != NULL) {
                for (int j = 0; j < arrlen(entry->values); j++)
                    arrput(*result, entry->values[j]);
            }
        }
    }

    const size_t oldLength = arrlen(*result);

    if (oldLength <= 1) return;
    
    // 결과 배열을 먼저 퀵-정렬한다.
    qsort(*result, oldLength, sizeof(**result), frQuickSortCallback);

    size_t newLength = 0;
    
    // 결과 배열에서 중복 값을 제거한다.
    for (int i = 0; i < oldLength; i++)
        if ((*result)[i] != (*result)[i + 1])
            (*result)[newLength++] = (*result)[i];
    
    // 결과 배열의 길이를 다시 설정한다.
    arrsetlen(*result, newLength);
}

/* 공간 해시맵 `hash`의 경계 범위를 반환한다. */
Rectangle frGetSpatialHashBounds(frSpatialHash *hash) {
    return (hash != NULL) ? hash->bounds : FR_STRUCT_ZERO(Rectangle);
}

/* 공간 해시맵 `hash`의 각 셀의 크기를 반환한다. */
float frGetSpatialHashCellSize(frSpatialHash *hash) {
    return (hash != NULL) ? hash->cellSize : 0.0f;
}

/* 공간 해시맵 `hash`의 경계 범위를 `bounds`로 설정한다. */
void frSetSpatialHashBounds(frSpatialHash *hash, Rectangle bounds) {
    if (hash != NULL) hash->bounds = bounds;
}

/* 공간 해시맵 `hash`의 각 셀의 크기를 `cellSize`로 설정한다. */
void frSetSpatialHashCellSize(frSpatialHash *hash, float cellSize) {
    if (hash != NULL) hash->cellSize = cellSize;
}

/* 공간 해시맵 `hash`에서 벡터 `v`와 대응하는 키를 반환한다. */
static FR_API_INLINE int frComputeSpatialHashKey(frSpatialHash *hash, Vector2 v) {
    if (hash == NULL) return -1;

    const int xIndex = v.x * hash->inverseCellSize;
    const int yIndex = v.y * hash->inverseCellSize;

    return yIndex * (int) hash->bounds.width + xIndex;
}

/* C 표준 라이브러리의 `qsort()` 함수 호출에 사용되는 비교 함수이다. */
static int frQuickSortCallback(const void *x, const void *y) {
    int nx = *(const int *) x, ny = *(const int *) y;
    
    return (nx > ny) - (nx < ny);
}
