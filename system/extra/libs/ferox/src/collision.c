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

/* | `collision` 모듈 함수... | */

/* 도형 `s1`의 AABB가 `s2`의 AABB와 충돌하는지 확인한다. */
static bool frCheckCollisionAABB(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* 다각형 `s`에서 벡터 `v`와의 내적이 가장 큰 꼭짓점의 인덱스를 반환한다. */
static int frGetPolygonFurthestIndex(frShape *s, frTransform tx, Vector2 v);

/* 도형 `s`에서 벡터 `v`와 가장 수직에 가까운 변을 반환한다. */
static frVertices frGetShapeSignificantEdge(frShape *s, frTransform tx, Vector2 v);

/* 다각형 `s1`에서 `s2`로의 충돌 방향과 깊이를 계산한다. */
static int frGetSeparatingAxisIndex(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2, 
    float *depth
);

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclesSAT(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2
);

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 다각형 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclePolySAT(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2
);

/* 최적화된 분리 축 정리를 이용하여, 다각형 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionPolysSAT(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2
);

/* 최적화된 분리 축 정리를 이용하여, 도형 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionSAT(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2
);

/* 선분 `e`에서 벡터 `v`와의 내적이 `minDot`보다 작은 부분을 모두 잘라낸다. */
static frVertices frClipEdgeWithAxis(frVertices e, Vector2 v, float minDot);

/* `o1`에서 `v1` 방향으로 진행하는 광선이 `o2`에서 `v2` 방향으로 진행하는 광선과 만나는지 계산한다. */
static bool frComputeIntersectionRays(
    Vector2 o1, Vector2 v1, 
    Vector2 o2, Vector2 v2, 
    float *distance
);

/* `o`에서 `v` 방향으로 진행하는 광선이 중심점이 `c`이고 반지름이 `r`인 원과 만나는지 계산한다. */
static bool frComputeIntersectionRayCircle(
    Vector2 o, Vector2 v, 
    Vector2 c, float r, 
    float *distance
);

/* 도형 `s1`에서 `s2`로의 충돌을 계산한다. */
frCollision frComputeShapeCollision(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    return frCheckCollisionAABB(s1, tx1, s2, tx2)
        ? frComputeCollisionSAT(s1, tx1, s2, tx2)
        : FR_STRUCT_ZERO(frCollision);
}

/* 강체 `b1`에서 `b2`로의 충돌을 계산한다. */
frCollision frComputeBodyCollision(frBody *b1, frBody *b2) {
    if (b1 == NULL || b2 == NULL) return FR_STRUCT_ZERO(frCollision);
    
    return frComputeShapeCollision(
        frGetBodyShape(b1),
        frGetBodyTransform(b1),
        frGetBodyShape(b2),
        frGetBodyTransform(b2)
    );
}

/* 도형 `s`에 광선을 투사한다. */
frRaycastHit frComputeShapeRaycast(frShape *s, frTransform tx, frRay ray) {
    if (s == NULL || frGetShapeType(s) == FR_SHAPE_UNKNOWN) return FR_STRUCT_ZERO(frRaycastHit);

    frRaycastHit result = { .shape = s, .distance = FLT_MAX };

    ray.direction = frVec2Normalize(ray.direction);

    float distance = FLT_MAX;
    
    if (frGetShapeType(s) == FR_SHAPE_CIRCLE) {
        bool intersects = frComputeIntersectionRayCircle(
            ray.origin, ray.direction,
            tx.position, frGetCircleRadius(s),
            &distance
        );
        
        result.check = (distance >= 0.0f) && (distance <= ray.maxDistance);
        result.inside = (distance < 0.0f);
        
        if (result.check) {
            result.distance = distance;
            
            result.point = frVec2Add(
                ray.origin, 
                frVec2ScalarMultiply(ray.direction, result.distance)
            );

            result.normal = frVec2LeftNormal(frVec2Subtract(ray.origin, result.point));
        }
        
        return result;
    } else if (frGetShapeType(s) == FR_SHAPE_POLYGON) {
        int intersectionCount = 0;
        
        frVertices vertices = frGetPolygonVertices(s);
        
        // 다각형의 변 중에 광선과 교차하는 변이 존재하는지 확인한다.
        for (int j = vertices.count - 1, i = 0; i < vertices.count; j = i, i++) {
            Vector2 v1 = frVec2Transform(vertices.data[i], tx);
            Vector2 v2 = frVec2Transform(vertices.data[j], tx);

            Vector2 diff = frVec2Subtract(v1, v2);
            
            bool intersects = frComputeIntersectionRays(
                ray.origin, ray.direction, 
                v2, diff, 
                &distance
            );
            
            if (intersects && distance <= ray.maxDistance) {
                if (result.distance > distance) {
                    result.distance = distance;
                    
                    result.point = frVec2Add(
                        ray.origin, 
                        frVec2ScalarMultiply(ray.direction, result.distance)
                    );

                    result.normal = frVec2LeftNormal(diff);
                }
                
                intersectionCount++;
            }
        }
        
        result.inside = (intersectionCount & 1);
        result.check = (!result.inside) && (intersectionCount > 0);
        
        return result;
    }
}

/* 강체 `b`에 광선을 투사한다. */
frRaycastHit frComputeBodyRaycast(frBody *b, frRay ray) {
    if (b == NULL || frGetBodyShape(b) == NULL) return FR_STRUCT_ZERO(frRaycastHit);
    
    frRaycastHit result = frComputeShapeRaycast(
        frGetBodyShape(b), frGetBodyTransform(b), 
        ray
    );

    result.body = b;

    return result;
}

/* 도형 `s1`의 AABB가 `s2`의 AABB와 충돌하는지 확인한다. */
static bool frCheckCollisionAABB(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2) {
    return CheckCollisionRecs(frGetShapeAABB(s1, tx1), frGetShapeAABB(s2, tx2));
}

/* 다각형 `s`에서 벡터 `v`와의 내적이 가장 큰 꼭짓점의 인덱스를 반환한다. */
static int frGetPolygonFurthestIndex(frShape *s, frTransform tx, Vector2 v) {
    if (frGetShapeType(s) != FR_SHAPE_POLYGON) return -1;
    
    frVertices vertices = frGetPolygonVertices(s);
    
    float maxDot = -FLT_MAX;
    int result = 0;

    v = frVec2Rotate(v, -tx.rotation);
    
    for (int i = 0; i < vertices.count; i++) {
        float dot = frVec2DotProduct(vertices.data[i], v);
        
        if (maxDot < dot) {
            maxDot = dot;
            result = i;
        }
    }
    
    return result;
}

/* 도형 `s`에서 벡터 `v`와 가장 수직에 가까운 변을 반환한다. */
static frVertices frGetShapeSignificantEdge(frShape *s, frTransform tx, Vector2 v) {
    if (s == NULL || frGetShapeType(s) == FR_SHAPE_UNKNOWN) return FR_STRUCT_ZERO(frVertices);

    frVertices result = FR_STRUCT_ZERO(frVertices);
    
    if (frGetShapeType(s) == FR_SHAPE_CIRCLE) {
        result.data[0] = frVec2Transform(frVec2ScalarMultiply(v, frGetCircleRadius(s)), tx);
        result.count = 1;
        
        return result;
    } else if (frGetShapeType(s) == FR_SHAPE_POLYGON) {
        int furthestIndex = frGetPolygonFurthestIndex(s, tx, v);
        
        frVertices vertices = frGetPolygonVertices(s);

        int prevIndex = (furthestIndex == 0) ? vertices.count - 1 : furthestIndex - 1;
        int nextIndex = (furthestIndex == vertices.count - 1) ? 0 : furthestIndex + 1;

        Vector2 furthestVertex = frVec2Transform(vertices.data[furthestIndex], tx);

        Vector2 prevVertex = frVec2Transform(vertices.data[prevIndex], tx);
        Vector2 nextVertex = frVec2Transform(vertices.data[nextIndex], tx);

        Vector2 leftV = frVec2Normalize(frVec2Subtract(furthestVertex, prevVertex));
        Vector2 rightV = frVec2Normalize(frVec2Subtract(furthestVertex, nextVertex));

        if (frVec2DotProduct(leftV, v) < frVec2DotProduct(rightV, v)) {
            result.data[0] = prevVertex;
            result.data[1] = furthestVertex;
        } else {
            result.data[0] = furthestVertex;
            result.data[1] = nextVertex;
        }
        
        result.count = 2;

        return result;
    }
}

/* 다각형 `s1`에서 `s2`로의 충돌 방향과 깊이를 계산한다. */
static int frGetSeparatingAxisIndex(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2, 
    float *depth
) {
    if (frGetShapeType(s1) != FR_SHAPE_POLYGON || frGetShapeType(s2) != FR_SHAPE_POLYGON)
        return -1;

    int result = -1;
    
    frVertices normals = frGetPolygonNormals(s1);
    
    float maxDistance = -FLT_MAX;
    
    for (int i = 0; i < normals.count; i++) {
        Vector2 vertex = frVec2Transform(frGetPolygonVertex(s1, i), tx1);
        Vector2 normal = frVec2RotateTx(normals.data[i], tx1);
        
        int furthestIndex = frGetPolygonFurthestIndex(s2, tx2, frVec2Negate(normal));
        Vector2 furthestVertex = frVec2Transform(frGetPolygonVertex(s2, furthestIndex), tx2);
        
        float distance = frVec2DotProduct(normal, frVec2Subtract(furthestVertex, vertex));
        
        if (maxDistance < distance) {
            maxDistance = distance;
            result = i;
        }
    }
    
    if (depth != NULL) *depth = maxDistance;
    
    return result;
}

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclesSAT(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2
) {
    if (frGetShapeType(s1) != FR_SHAPE_CIRCLE || frGetShapeType(s2) != FR_SHAPE_CIRCLE)
        return FR_STRUCT_ZERO(frCollision);

    frCollision result = FR_STRUCT_ZERO(frCollision);

    Vector2 diff = frVec2Subtract(tx2.position, tx1.position);

    float radiusSum = frGetCircleRadius(s1) + frGetCircleRadius(s2);
    float magnitudeSqr = frVec2MagnitudeSqr(diff);
    
    if (radiusSum * radiusSum < magnitudeSqr) return result;

    float diffMagnitude = sqrtf(magnitudeSqr);

    result.check = true;

    if (diffMagnitude > 0.0f) {
        result.direction = frVec2ScalarMultiply(diff, 1.0f / diffMagnitude);
        
        frVertices e = frGetShapeSignificantEdge(s1, tx1, result.direction);
        
        result.points[0] = result.points[1] = e.data[0];
        result.depths[0] = result.depths[1] = radiusSum - diffMagnitude;
    } else {
        result.direction = (Vector2) { .x = 1.0f };
        
        frVertices e = frGetShapeSignificantEdge(s1, tx1, result.direction);
        
        result.points[0] = result.points[1] = e.data[0];
        result.depths[0] = result.depths[1] = frGetCircleRadius(s1);
    }

    result.count = 1;
    
    return result;
}

/* 최적화된 분리 축 정리를 이용하여, 원 `s1`에서 다각형 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionCirclePolySAT(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2
) {
    if (frGetShapeType(s1) == FR_SHAPE_UNKNOWN || frGetShapeType(s2) == FR_SHAPE_UNKNOWN 
        || frGetShapeType(s1) == frGetShapeType(s2))
        return FR_STRUCT_ZERO(frCollision);

    frCollision result = FR_STRUCT_ZERO(frCollision);
        
    frShape *circle = s1, *polygon = s2;
    frTransform circleTx = tx1, polygonTx = tx2;
    
    if (frGetShapeType(s1) == FR_SHAPE_POLYGON && frGetShapeType(s2) == FR_SHAPE_CIRCLE) {
        circle = s2, polygon = s1;
        circleTx = tx2, polygonTx = tx1;
    }

    frVertices vertices = frGetPolygonVertices(polygon);
    frVertices normals = frGetPolygonNormals(polygon);

    // 다각형의 꼭짓점과 법선 벡터를 변환하는 대신, 원의 중심을 다각형을 기준으로 변환한다.
    Vector2 center = frVec2Rotate(
        frVec2Subtract(circleTx.position, polygonTx.position), 
        -polygonTx.rotation
    );

    float radius = frGetCircleRadius(circle), maxDistance = -FLT_MAX;
    int normalIndex = -1;

    // 다각형에서 원의 중심과 거리가 가장 가까운 변을 찾는다.
    for (int i = 0; i < vertices.count; i++) {
        float distance = frVec2DotProduct(
            normals.data[i], 
            frVec2Subtract(center, vertices.data[i])
        );
        
        // 꼭짓점 하나라도 원의 바깥쪽에 있다면, 계산을 종료한다.
        if (distance > radius) return result;
        
        if (maxDistance < distance) {
            maxDistance = distance;
            normalIndex = i;
        }
    }

    // 다각형이 원의 중심을 포함하고 있다면, 계산을 종료한다.
    if (maxDistance < 0.0f) {
        result.check = true;

        result.direction = frVec2Negate(frVec2RotateTx(normals.data[normalIndex], polygonTx));

        if (frVec2DotProduct(frVec2Subtract(tx2.position, tx1.position), result.direction) < 0.0f)
            result.direction = frVec2Negate(result.direction);

        result.points[0] = result.points[1] = frVec2Add(
            circleTx.position,
            frVec2ScalarMultiply(result.direction, -radius)
        );

        result.depths[0] = result.depths[1] = radius - maxDistance;

        result.count = 1;
    } else {
        Vector2 v1 = (normalIndex > 0)
            ? vertices.data[normalIndex - 1]
            : vertices.data[vertices.count - 1];
        Vector2 v2 = vertices.data[normalIndex];

        /*
            1. 원이 `v1`을 포함하는 선분과 만나는가?
            2. 원이 `v2`를 포함하는 선분과 만나는가?
            3. 원이 `v1`과 `v2` 사이의 선분과 만나는가?

            => 벡터의 내적을 통해 확인할 수 있다.
        */
        Vector2 diff1 = frVec2Subtract(center, v1), diff2 = frVec2Subtract(center, v2);

        float v1Dot = frVec2DotProduct(diff1, frVec2Subtract(v2, v1));
        float v2Dot = frVec2DotProduct(diff2, frVec2Subtract(v1, v2));

        if (v1Dot <= 0.0f) {
            float magnitudeSqr = frVec2MagnitudeSqr(diff1);

            if (magnitudeSqr > radius * radius) return result;

            result.direction = frVec2Normalize(frVec2RotateTx(frVec2Negate(diff1), polygonTx));

            if (frVec2DotProduct(frVec2Subtract(tx2.position, tx1.position), result.direction) < 0.0f)
                result.direction = frVec2Negate(result.direction);
            
            result.points[0] = result.points[1] = frVec2Transform(v1, polygonTx);
            result.depths[0] = result.depths[1] = radius - sqrtf(magnitudeSqr);
        } else if (v2Dot <= 0.0f) {
            float magnitudeSqr = frVec2MagnitudeSqr(diff2);

            if (magnitudeSqr > radius * radius) return result;
            
            result.direction = frVec2Normalize(frVec2RotateTx(frVec2Negate(diff2), polygonTx));

            if (frVec2DotProduct(frVec2Subtract(tx2.position, tx1.position), result.direction) < 0.0f)
                result.direction = frVec2Negate(result.direction);

            result.points[0] = result.points[1] = frVec2Transform(v2, polygonTx);
            result.depths[0] = result.depths[1] = radius - sqrtf(magnitudeSqr);
        } else {
            Vector2 normal = normals.data[normalIndex];

            if (frVec2DotProduct(diff1, normal) > radius) return result;

            result.direction = frVec2Negate(frVec2RotateTx(normal, polygonTx));

            if (frVec2DotProduct(frVec2Subtract(tx2.position, tx1.position), result.direction) < 0.0f)
                result.direction = frVec2Negate(result.direction);

            result.points[0] = result.points[1] = frVec2Add(
                circleTx.position,
                frVec2ScalarMultiply(result.direction, -radius)
            );

            result.depths[0] = result.depths[1] = radius - maxDistance;
        }

        result.check = true;

        result.count = 1;
    }

    return result;
}

/* 최적화된 분리 축 정리를 이용하여, 다각형 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionPolysSAT(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2
) {
    if (frGetShapeType(s1) != FR_SHAPE_POLYGON || frGetShapeType(s2) != FR_SHAPE_POLYGON)
        return FR_STRUCT_ZERO(frCollision);

    frCollision result = FR_STRUCT_ZERO(frCollision);
    
    float depth1 = FLT_MAX, depth2 = FLT_MAX;
    
    int index1 = frGetSeparatingAxisIndex(s1, tx1, s2, tx2, &depth1);
    if (depth1 >= 0.0f) return result;
    
    int index2 = frGetSeparatingAxisIndex(s2, tx2, s1, tx1, &depth2);
    if (depth2 >= 0.0f) return result;
    
    Vector2 direction = (depth1 > depth2) 
        ? frVec2RotateTx(frGetPolygonNormal(s1, index1), tx1)
        : frVec2RotateTx(frGetPolygonNormal(s2, index2), tx2);
    
    if (frVec2DotProduct(frVec2Subtract(tx2.position, tx1.position), direction) < 0.0f) 
        direction = frVec2Negate(direction);
        
    result.check = true;
    
    // 회전 변환이 적용된 벡터는 회전 변환을 적용하기 전의 벡터와 크기가 같다.
    result.direction = direction;
    
    frVertices e1 = frGetShapeSignificantEdge(s1, tx1, direction);
    frVertices e2 = frGetShapeSignificantEdge(s2, tx2, frVec2Negate(direction));
    
    frVertices refEdge = e1, incEdge = e2;
    frTransform refTx = tx1, incTx = tx2;

    float dot1 = frVec2DotProduct(frVec2Subtract(e1.data[1], e1.data[0]), direction);
    float dot2 = frVec2DotProduct(frVec2Subtract(e2.data[1], e2.data[0]), direction);

    if (fabsf(dot1) > fabsf(dot2)) {
        refEdge = e2;
        incEdge = e1;

        refTx = tx2;
        incTx = tx1;
    }

    Vector2 refV = frVec2Normalize(frVec2Subtract(refEdge.data[1], refEdge.data[0]));

    float refDot1 = frVec2DotProduct(refEdge.data[0], refV);
    float refDot2 = frVec2DotProduct(refEdge.data[1], refV);

    incEdge = frClipEdgeWithAxis(incEdge, refV, refDot1);
    incEdge = frClipEdgeWithAxis(incEdge, frVec2Negate(refV), -refDot2);

    Vector2 refNormal = frVec2RightNormal(refV);

    float max_depth = frVec2DotProduct(refEdge.data[0], refNormal);

    result.points[0] = incEdge.data[0], result.points[1] = incEdge.data[1];

    result.depths[0] = frVec2DotProduct(result.points[0], refNormal) - max_depth;
    result.depths[1] = frVec2DotProduct(result.points[1], refNormal) - max_depth;

    result.count = 2;

    if (result.depths[0] < 0.0f) {
        result.points[0] = result.points[1];
        result.depths[0] = result.depths[1];

        result.count = 1;
    } else if (result.depths[1] < 0.0f) {
        result.points[1] = result.points[0];
        result.depths[1] = result.depths[0];

        result.count = 1;
    }

    return result;
}

/* 최적화된 분리 축 정리를 이용하여, 도형 `s1`에서 `s2`로의 충돌을 계산한다. */
static frCollision frComputeCollisionSAT(
    frShape *s1, frTransform tx1, 
    frShape *s2, frTransform tx2
) {
    frShapeType type1 = frGetShapeType(s1);
    frShapeType type2 = frGetShapeType(s2);
    
    if (type1 == FR_SHAPE_UNKNOWN || type2 == FR_SHAPE_UNKNOWN)
        return FR_STRUCT_ZERO(frCollision);
    else if (type1 == FR_SHAPE_CIRCLE && type2 == FR_SHAPE_CIRCLE)
        return frComputeCollisionCirclesSAT(s1, tx1, s2, tx2);
    else if (type1 == FR_SHAPE_CIRCLE && type2 == FR_SHAPE_POLYGON
        || type1 == FR_SHAPE_POLYGON && type2 == FR_SHAPE_CIRCLE)
        return frComputeCollisionCirclePolySAT(s1, tx1, s2, tx2);
    else if (type1 == FR_SHAPE_POLYGON && type2 == FR_SHAPE_POLYGON)
        return frComputeCollisionPolysSAT(s1, tx1, s2, tx2);
}

/* 선분 `e`에서 벡터 `v`와의 내적이 `minDot`보다 작은 부분을 모두 잘라낸다. */
static frVertices frClipEdgeWithAxis(frVertices e, Vector2 v, float minDot) {
    frVertices result = FR_STRUCT_ZERO(frVertices);
    
    float p1Dot = frVec2DotProduct(e.data[0], v) - minDot;
    float p2Dot = frVec2DotProduct(e.data[1], v) - minDot;
    
    bool insideP1 = (p1Dot >= 0.0f), insideP2 = (p2Dot >= 0.0f);
    
    if (insideP1 && insideP2) {
        result.data[0] = e.data[0];
        result.data[1] = e.data[1];
        
        result.count = 2;
    } else {
        Vector2 edgeV = frVec2Subtract(e.data[1], e.data[0]);
        
        const float distance = p1Dot / (p1Dot - p2Dot);
        
        Vector2 midpoint = frVec2Add(
            e.data[0], 
            frVec2ScalarMultiply(edgeV, distance)
        );
        
        if (insideP1 && !insideP2) {
            result.data[0] = e.data[0];
            result.data[1] = midpoint;

            result.count = 2;
        } else if (!insideP1 && insideP2) {
            result.data[0] = e.data[1];
            result.data[1] = midpoint;

            result.count = 2;
        }
    }
    
    return result;
}

/* `o1`에서 `v1` 방향으로 진행하는 광선이 `o2`에서 `v2` 방향으로 진행하는 광선과 만나는지 계산한다. */
static bool frComputeIntersectionRays(
    Vector2 o1, Vector2 v1, 
    Vector2 o2, Vector2 v2, 
    float *distance
) {
    bool result = true;
    
    float cross = frVec2CrossProduct(v1, v2);
    
    // 두 광선은 평행하거나 일직선 상에 있다.
    if (frNumberApproxEquals(cross, 0.0f)) {
        *distance = 0.0f;
        
        return false;
    }
    
    Vector2 diff = frVec2Subtract(o2, o1);

    float inverseCross = 1.0f / cross;

    float t = frVec2CrossProduct(diff, v2) * inverseCross;
    float u = frVec2CrossProduct(diff, v1) * inverseCross;

    // 두 광선은 평행하지 않으면서 서로 만나지 않는다.
    if (t < 0.0f || u < 0.0f || u > 1.0f) result = false;

    *distance = t;

    return result;
}

/* `o`에서 `v` 방향으로 진행하는 광선이 중심점이 `c`이고 반지름이 `r`인 원과 만나는지 계산한다. */
static bool frComputeIntersectionRayCircle(
    Vector2 o, Vector2 v, 
    Vector2 c, float r, 
    float *distance
) {
    bool result = true;
    
    Vector2 diff = frVec2Subtract(c, o);
    
    float dot = frVec2DotProduct(diff, v);
    
    float heightSqr = frVec2MagnitudeSqr(diff) - (dot * dot);
    float baseSqr = (r * r) - heightSqr;
    
    // 광선과 원은 서로 만나지 않는다.
    if (dot < 0.0f || baseSqr < 0.0f) result = false;
    
    *distance = dot - sqrtf(baseSqr);
    
    return result;
}
