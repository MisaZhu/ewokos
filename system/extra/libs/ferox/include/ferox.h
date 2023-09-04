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

#ifndef FEROX_H
#define FEROX_H

#define FEROX_STANDALONE 

#include <float.h>
#include <openlibm.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef FEROX_STANDALONE
    #include "raylib.h"
#else
    #ifndef PI
        #define PI 3.14159265358979323846f
    #endif

    #define DEG2RAD (PI / 180.0f)
    #define RAD2DEG (180.0f / PI)

    /* A struct that represents a two-dimensional vector. */
    typedef struct Vector2 {
        float x;
        float y;
    } Vector2;

    /* A struct that represents a rectangle. */
    typedef struct Rectangle {
        float x;
        float y;
        float width;
        float height;
    } Rectangle;

#ifdef __cplusplus
extern "C" {
#endif
    /* Returns `true` if `rec1` collides with `rec2`. */
    static bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
        return ((rec1.x + rec1.width) - rec2.x) >= 0 && ((rec2.x + rec2.width) - rec1.x) >= 0
            && ((rec1.y + rec1.height) - rec2.y) >= 0 && ((rec2.y + rec2.height) - rec1.y) >= 0;
    }
#ifdef __cplusplus
}
#endif

#endif

/* | Macro Definitions (Global) | */

#ifdef _MSC_VER
    #define FR_API_INLINE __forceinline
#elif defined(__GNUC__)
    #if defined(__STRICT_ANSI__)
        #define FR_API_INLINE __inline__ __attribute__((always_inline))
    #else
        #define FR_API_INLINE inline __attribute__((always_inline))
    #endif
#else
    #define FR_API_INLINE inline
#endif

#define FR_STRUCT_ZERO(T)                         ((T) { 0 })

/* | Macro Definitions (Configuration) | */

#define FR_BROADPHASE_CELL_SIZE                   3.2f
#define FR_BROADPHASE_INVERSE_CELL_SIZE           (1.0f / (FR_BROADPHASE_CELL_SIZE))

#define FR_DEBUG_CIRCLE_SEGMENT_COUNT             32

#define FR_DYNAMICS_CORRECTION_DEPTH_SCALE        0.24f
#define FR_DYNAMICS_CORRECTION_DEPTH_THRESHOLD    0.02f
#define FR_DYNAMICS_DYNAMIC_FRICTION_MULTIPLIER   0.85f

#define FR_GEOMETRY_MAX_VERTEX_COUNT              8

#define FR_GLOBAL_PIXELS_PER_METER                16.0f

#define FR_WORLD_ACCUMULATOR_LIMIT                200.0
//#define FR_WORLD_DEFAULT_GRAVITY                  ((Vector2) { .y = 9.8f })
#define FR_WORLD_DEFAULT_GRAVITY                  ((Vector2) { 0, 9.8f })
#define FR_WORLD_MAX_BODY_COUNT                   288
#define FR_WORLD_MAX_ITERATIONS                   12

/* | Data Type Definitions | */

/* A struct that represents the type of a rigid body. */
typedef enum frBodyType {
    FR_BODY_UNKNOWN = -1,
    FR_BODY_STATIC,
    FR_BODY_KINEMATIC,
    FR_BODY_DYNAMIC
} frBodyType;

/* An enum that represents the property flag of a rigid body. */
typedef enum frBodyFlag {
    FR_FLAG_NONE = 0x00,
    FR_FLAG_INFINITE_MASS = 0x01,
    FR_FLAG_INFINITE_INERTIA = 0x02
} frBodyFlag;

/* A data type that represents the property flags of a rigid body. */
typedef uint8_t frBodyFlags;

/* A struct that represents the type of a collision shape. */
typedef enum frShapeType {
    FR_SHAPE_UNKNOWN,
    FR_SHAPE_CIRCLE,
    FR_SHAPE_POLYGON
} frShapeType;

/* A struct that represents the material of a collision shape. */
typedef struct frMaterial {
    float density;
    float restitution;
    float staticFriction;
    float dynamicFriction;
} frMaterial;

/* 
    A struct that represents the position (in meters) 
    and rotation (in radians) of an object. 
*/
typedef struct frTransform {
    Vector2 position;
    float rotation;
    struct {
        bool valid;
        float sinA;
        float cosA;
    } cache;
} frTransform;

/* A struct that represents the vertices of a polygon. */
typedef struct frVertices {
    Vector2 data[FR_GEOMETRY_MAX_VERTEX_COUNT];
    int count;
} frVertices;

/* A struct that represents a collision shape. */
typedef struct frShape frShape;

/* A struct that represents a rigid body. */
typedef struct frBody frBody;

/* A struct that represents information for a set of two rigid bodies. */
typedef struct frSolverCache {
    frBody *bodies[2];
    /* TODO: ... */
} frSolverCache;

/* A struct that represents the details of a collision. */
typedef struct frCollision {
    bool check;           // Returns `true` if two collision shapes collide with each other.
    frSolverCache cache;  // The struct that contains rigid bodies that collided with each other.
    Vector2 direction;    // The direction of the collision in a unit vector form.
    Vector2 points[2];    // The points of the collision between two collision shapes.
    float depths[2];      // The penetration depths of the collision.
    int count;            // The number of points for this collision.
} frCollision;

/* A callback which will be executed when a collision event occurs. */
typedef void (*frCollisionCallback)(frCollision *collision);

/* A struct that represents the collision event handler of a world. */
typedef struct frCollisionHandler {
    frCollisionCallback preSolve;
    frCollisionCallback postSolve;
} frCollisionHandler;

/* A struct that represents a ray. */
typedef struct frRay {
    Vector2 origin;
    Vector2 direction;
    float maxDistance;
    bool closest;
} frRay;

/* A struct that represents the details of a raycast hit. */
typedef struct frRaycastHit {
    bool check;          // Returns `true` if the ray collides with `shape` or `body`.
    union {
        frShape *shape; 
        frBody *body;
    };                   // The collision shape or the body that was hit by the raycast.
    Vector2 point;       // The point at which the raycast hit `shape` or `body`.
    Vector2 normal;      // The normal vector of the raycast hit.
    float distance;      // The distance from the ray's starting point to `shape` or `body`.
    bool inside;         // Returns `true` if the ray's starting point is inside `shape` or `body`.
} frRaycastHit;

/* A struct that represents a spatial hash. */
typedef struct frSpatialHash frSpatialHash;

/* A struct that represents the world that holds rigid bodies. */
typedef struct frWorld frWorld;

#ifdef __cplusplus
extern "C" {
#endif

/* | Module Functions (`broadphase`) | */

/* Creates a new spatial hash from the given bounds and the size of each cell. */
frSpatialHash *frCreateSpatialHash(Rectangle bounds, float cellSize);

/* Releases the memory allocated for `hash`. */
void frReleaseSpatialHash(frSpatialHash *hash);

/* Generates a new key from `rec` and inserts the key-`value` pair to `hash`. */
void frAddToSpatialHash(frSpatialHash *hash, Rectangle rec, int value);

/* Removes all key-value pairs from `hash`. */
void frClearSpatialHash(frSpatialHash *hash);

/* Removes a `key`-value pair from `hash`. */
void frRemoveFromSpatialHash(frSpatialHash *hash, int key);

/* Returns the values of all pairs that collides with `rec` in `hash`. */
void frQuerySpatialHash(frSpatialHash *hash, Rectangle rec, int **result);

/* Returns the bounds of `hash`. */
Rectangle frGetSpatialHashBounds(frSpatialHash *hash);

/* Returns the size of each cell of `hash`. */
float frGetSpatialHashCellSize(frSpatialHash *hash);

/* Sets the bounds of `hash` to `bounds`. */
void frSetSpatialHashBounds(frSpatialHash *hash, Rectangle bounds);

/* Sets the size of each cell of `hash` to `cellSize`. */
void frSetSpatialHashCellSize(frSpatialHash *hash, float cellSize);

/* | Module Functions (`collision`) | */

/* 
    Computes the collision between `s1` with position and rotation from `tx1` and `s2` 
    with position and rotation from `tx2`.
*/
frCollision frComputeShapeCollision(frShape *s1, frTransform tx1, frShape *s2, frTransform tx2);

/* Computes the collision between `b1` and `b2`. */
frCollision frComputeBodyCollision(frBody *b1, frBody *b2);

/* Casts a `ray` to the collision shape `s`. */
frRaycastHit frComputeShapeRaycast(frShape *s, frTransform tx, frRay ray);

/* Casts a `ray` to the rigid body `b`. */
frRaycastHit frComputeBodyRaycast(frBody *b, frRay ray);

/* | Module Functions (`debug`) | */

#ifndef FEROX_STANDALONE
    /* Draws an arrow that points from `p1` to `p2`. */
    void frDrawArrow(Vector2 p1, Vector2 p2, float thick, Color color);

    /* Draws the collision shape of `b`. */
    void frDrawBody(frBody *b, Color color);

    /* Draws the border of the collision shape of `b`. */
    void frDrawBodyLines(frBody *b, float thick, Color color);

    /* Draws the AABB and the center of mass of `b`. */
    void frDrawBodyAABB(frBody *b, float thick, Color color);

    /* Draws the properties of `b`. */
    void frDrawBodyProperties(frBody *b, Color color);

    /* Draws the border of `hm`. */
    void frDrawSpatialHash(frSpatialHash *hm, float thick, Color color);
#endif

/* | Module Functions (`dynamics`) | */

/* Creates a new rigid body from the given type, flags, and position (in meters). */
frBody *frCreateBody(frBodyType type, frBodyFlags flags, Vector2 p);

/* 
    Creates a new rigid body from the given type, flags, position (in meters), 
    and a collision shape.
*/
frBody *frCreateBodyFromShape(frBodyType type, frBodyFlags flags, Vector2 p, frShape *s);

/* Releases the memory allocated for `b`. */
void frReleaseBody(frBody *b);

/* Attaches the collision shape `s` to `b`. */
void frAttachShapeToBody(frBody *b, frShape *s);

/* Detaches the collision shape `s` from `b`. */
void frDetachShapeFromBody(frBody *b);

/* Returns the size of the struct `frBody`. */
size_t frGetBodyStructSize(void);

/* Returns the type of `b`. */
frBodyType frGetBodyType(frBody *b);

/* Returns the property flags of `b`. */
frBodyFlags frGetBodyFlags(frBody *b);

/* Returns the material of `b`. */
frMaterial frGetBodyMaterial(frBody *b);

/* Returns the mass of `b`. */
float frGetBodyMass(frBody *b);

/* Returns the inverse mass of `b`. */
float frGetBodyInverseMass(frBody *b);

/* Returns the moment of inertia for `b`. */
float frGetBodyInertia(frBody *b);

/* Returns the inverse moment of inertia for `b`. */
float frGetBodyInverseInertia(frBody *b);

/* Returns the velocity of `b`. */
Vector2 frGetBodyVelocity(frBody *b);

/* Returns the angular velocity of `b`. */
float frGetBodyAngularVelocity(frBody *b);

/* Returns the gravity scale of `b`. */
float frGetBodyGravityScale(frBody *b);

/* Returns the position (in meters) and rotation (in radians) of `b`. */
frTransform frGetBodyTransform(frBody *b);

/* Returns the position (in meters) of `b`. */
Vector2 frGetBodyPosition(frBody *b);

/* Returns the rotation (in radians) of `b`. */
float frGetBodyRotation(frBody *b);

/* Returns the collision shape of `b`. */
frShape *frGetBodyShape(frBody *b);

/* Returns the AABB (Axis-Aligned Bounding Box) of `b`. */
Rectangle frGetBodyAABB(frBody *b);

/* Returns the user data of `b`. */
void *frGetBodyUserData(frBody *b);

/* Converts the world coordinates `p` to a position relative to `b`'s transform value. */
Vector2 frGetLocalPoint(frBody *b, Vector2 p);

/* Converts the position relative to `b`'s transform value `p` to world coordinates. */
Vector2 frGetWorldPoint(frBody *b, Vector2 p);

/* Sets the type of `b` to `type`. */
void frSetBodyType(frBody *b, frBodyType type);

/* Sets the property flags of `b` to `flags`. */
void frSetBodyFlags(frBody *b, frBodyFlags flags);

/* Sets the velocity of `b` to `v`. */
void frSetBodyVelocity(frBody *b, Vector2 v);

/* Sets the angular velocity of `b` to `a`. */
void frSetBodyAngularVelocity(frBody *b, double a);

/* Sets the gravity scale value of `b` to `scale`. */
void frSetBodyGravityScale(frBody *b, float scale);

/* Sets the transform value of `b` to `tx`. */ 
void frSetBodyTransform(frBody *b, frTransform tx);

/* Sets the position of `b` to `p`. */
void frSetBodyPosition(frBody *b, Vector2 p);

/* Sets the rotation of `b` to `rotation`. */
void frSetBodyRotation(frBody *b, float rotation);

/* Sets the user data of `b` to `data`. */
void frSetBodyUserData(frBody *b, void *data);

/* Clears all forces that are applied to `b`. */
void frClearBodyForces(frBody *b);

/* Applies a `gravity` vector to `b`. */
void frApplyGravity(frBody *b, Vector2 gravity);

/* Applies an `impulse` to `b`. */
void frApplyImpulse(frBody *b, Vector2 impulse);

/* Applies a torque `impulse` to `b`. */
void frApplyTorqueImpulse(frBody *b, Vector2 p, Vector2 impulse);

/* Integrates the position of `b` with `dt`. */
void frIntegrateForBodyPosition(frBody *b, double dt);

/* Integrates the velocities of `b` with `dt`. */
void frIntegrateForBodyVelocities(frBody *b, double dt);

/* Resolves the collision between two rigid bodies. */
void frResolveCollision(frCollision *collision);

/* Corrects the positions of two rigid bodies. */
void frCorrectBodyPositions(frCollision *collision, float inverseDt);

/* | Module Functions (`geometry`) | */

/* 
    Creates a new circle-shaped collision shape 
    from the given material and radius (in meters).
*/
frShape *frCreateCircle(frMaterial material, float radius);

/* 
    Creates a new rectangle-shaped collision shape 
    from the given material, width and height (in meters). 
*/
frShape *frCreateRectangle(frMaterial material, float width, float height);

/* 
    Creates a new polygon-shaped collision shape 
    from the given material and vertices (in meters).
*/
frShape *frCreatePolygon(frMaterial material, frVertices vertices);

/* Creates an empty shape. */
frShape *frCreateShape(void);

/* Returns a clone of `s`. */
frShape *frCloneShape(frShape *s);

/* Releases the memory allocated for `s`. */
void frReleaseShape(frShape *s);

/* Returns the size of the struct `frShape`. */
size_t frGetShapeStructSize(void);

/* Returns the type of `s`. */
frShapeType frGetShapeType(frShape *s);

/* Returns the material of `s`. */
frMaterial frGetShapeMaterial(frShape *s);

/* Returns the area of `s`. */
float frGetShapeArea(frShape *s);

/* Returns the mass of `s`. */
float frGetShapeMass(frShape *s);

/* Returns the moment of inertia for `s`. */
float frGetShapeInertia(frShape *s);

/* Returns the AABB (Axis-Aligned Bounding Box) of `s`. */
Rectangle frGetShapeAABB(frShape *s, frTransform tx);

/* Returns the radius of the `s`. */
float frGetCircleRadius(frShape *s);

/* Returns the width and height of `s`. */
Vector2 frGetRectangleDimensions(frShape *s);

/* Returns the `i + 1`th vertex of `s`. */
Vector2 frGetPolygonVertex(frShape *s, int i);

/* Returns the `i + 1`th normal of `s`. */
Vector2 frGetPolygonNormal(frShape *s, int i);

/* Returns the vertices of `s`. */
frVertices frGetPolygonVertices(frShape *s);

/* Returns the normals of `s`. */
frVertices frGetPolygonNormals(frShape *s);

/* Returns `true` if `s` is a rectangular collision shape. */
bool frIsShapeRectangle(frShape *s);

/* Sets the radius of `s` to `radius`. */
void frSetCircleRadius(frShape *s, float radius);

/* Sets the dimensions (width and height) for `s` to `v`. */
void frSetRectangleDimensions(frShape *s, Vector2 v);

/* Sets the vertices of `s` to `vertices`. */
void frSetPolygonVertices(frShape *s, frVertices vertices);

/* Sets the material of `s` to `material`. */
void frSetShapeMaterial(frShape *s, frMaterial material);

/* Sets the type of `s` to `type`. */
void frSetShapeType(frShape *s, frShapeType type);

/* Returns `true` if `s` contains the point `p`. */
bool frShapeContainsPoint(frShape *s, frTransform tx, Vector2 p);

/* | Module Functions (`timer`) | */

/* Initializes a monotonic clock. */
void frInitClock(void);

/* Returns the current time (in milliseconds) of the monotonic clock. */
double frGetCurrentTime(void);

/* Returns the difference between `newTime` and `oldTime`. */
double frGetTimeDifference(double newTime, double oldTime);

/* Returns the difference between the current time and `oldTime`. */
double frGetTimeSince(double oldTime);

/* | Module Functions (`utils`) | */

/* Normalizes `angle` (in radians) to range `[center - π/2, center + π/2]`. */
float frNormalizeAngle(float angle, float center);

/* Returns `true` if `f1` and `f2` are approximately equal. */
bool frNumberApproxEquals(float f1, float f2);

/* | Module Functions (`world`) | */

/* Creates a new world from the given `gravity` vector and world `bounds` in meters. */
frWorld *frCreateWorld(Vector2 gravity, Rectangle bounds);

/* Removes all rigid bodies from `world`, then releases the memory allocated for `world`. */
void frReleaseWorld(frWorld *world);

/* Adds `b` to `world`. */
bool frAddToWorld(frWorld *world, frBody *b);

/* Removes all rigid bodies from `world`. */
void frClearWorld(frWorld *world);

/* Removes `b` from `world`. */
bool frRemoveFromWorld(frWorld *world, frBody *b);

/* Returns the size of the struct `frWorld`. */
size_t frGetWorldStructSize(void);

/* Returns the rigid body at the `index` in `world`'s array of rigid bodies. */
frBody *frGetWorldBody(frWorld *world, int index);

/* Returns the collision event handler for `world`. */
frCollisionHandler frGetWorldCollisionHandler(frWorld *world);

/* Returns the length of `world`'s array of rigid bodies. */
int frGetWorldBodyCount(frWorld *world);

/* Returns the bounds of `world`. */
Rectangle frGetWorldBounds(frWorld *world);

/* Returns the spatial hash of `world`. */
frSpatialHash *frGetWorldSpatialHash(frWorld *world);

/* Returns the gravity vector of `world`. */
Vector2 frGetWorldGravity(frWorld *world);

/* Returns `true` if `b` collides with the bounds of `world`. */
bool frIsInWorldBounds(frWorld *world, frBody *b);

/* Sets the world bounds of `world` to `bounds`. */
void frSetWorldBounds(frWorld *world, Rectangle bounds);

/* Sets the collision event handler for `world` to `handler`. */
void frSetWorldCollisionHandler(frWorld *world, frCollisionHandler handler);

/* Sets the gravity vector of `world` to `gravity`. */
void frSetWorldGravity(frWorld *world, Vector2 gravity);

/* Simulates the `world` for the time step `dt` (in milliseconds). */
void frSimulateWorld(frWorld *world, double dt);

/* Query the `world` for rigid bodies that collides with `rec`. */
int frQueryWorldSpatialHash(frWorld *world, Rectangle rec, frBody **bodies);

/* Casts a `ray` to all rigid bodies in `world`. */
int frComputeWorldRaycast(frWorld *world, frRay ray, frRaycastHit *hits);

/* | Inline Functions | */

/* Converts `value` (in pixels) to meters. */
FR_API_INLINE float frNumberPixelsToMeters(float value) {
    return (FR_GLOBAL_PIXELS_PER_METER > 0.0f)
        ? (value / FR_GLOBAL_PIXELS_PER_METER)
        : 0.0f;
}

/* Converts `value` (in meters) to pixels. */
FR_API_INLINE float frNumberMetersToPixels(float value) {
    return (FR_GLOBAL_PIXELS_PER_METER > 0.0f)
        ? (value * FR_GLOBAL_PIXELS_PER_METER)
        : 0.0f;
}

/* Converts the components of `rec` (in pixels) to meters. */
FR_API_INLINE Rectangle frRecPixelsToMeters(Rectangle rec) {
    return (Rectangle) {
        .x = frNumberPixelsToMeters(rec.x),
        .y = frNumberPixelsToMeters(rec.y),
        .width = frNumberPixelsToMeters(rec.width),
        .height = frNumberPixelsToMeters(rec.height)
    };
}

/* Converts the components of `rec` (in meters) to pixels. */
FR_API_INLINE Rectangle frRecMetersToPixels(Rectangle rec) {
    return (Rectangle) {
        .x = frNumberMetersToPixels(rec.x),
        .y = frNumberMetersToPixels(rec.y),
        .width = frNumberMetersToPixels(rec.width),
        .height = frNumberMetersToPixels(rec.height)
    };
}

/* Adds `v1` and `v2`. */
FR_API_INLINE Vector2 frVec2Add(Vector2 v1, Vector2 v2) {
    return (Vector2) { v1.x + v2.x, v1.y + v2.y };
}

/* Subtracts `v2` from `v1`. */
FR_API_INLINE Vector2 frVec2Subtract(Vector2 v1, Vector2 v2) {
    return (Vector2) { v1.x - v2.x, v1.y - v2.y };
}

/* Multiplies `v` by `value`. */
FR_API_INLINE Vector2 frVec2ScalarMultiply(Vector2 v, float value) {
    return (Vector2) { v.x * value, v.y * value };
}

/* Returns the cross product of `v1` and `v2`. */
FR_API_INLINE float frVec2CrossProduct(Vector2 v1, Vector2 v2) {
    // 평면 벡터의 외적은 스칼라 값이다.
    return (v1.x * v2.y) - (v1.y * v2.x);
}

/* Returns the dot product of `v1` and `v2`. */
FR_API_INLINE float frVec2DotProduct(Vector2 v1, Vector2 v2) {
    return (v1.x * v2.x) + (v1.y * v2.y);
}

/* Returns the squared magnitude of `v`. */
FR_API_INLINE float frVec2MagnitudeSqr(Vector2 v) {
    return frVec2DotProduct(v, v);
}

/* Returns the magnitude of `v`. */
FR_API_INLINE float frVec2Magnitude(Vector2 v) {
    return sqrtf(frVec2MagnitudeSqr(v));
}

/* Returns the negated vector of `v`. */
FR_API_INLINE Vector2 frVec2Negate(Vector2 v) {
    return (Vector2) { -v.x, -v.y };
}

/* Converts `v` to a unit vector. */
FR_API_INLINE Vector2 frVec2Normalize(Vector2 v) {
    const float magnitude = frVec2Magnitude(v);
    
    return (magnitude > 0.0f)
        ? frVec2ScalarMultiply(v, 1.0f / magnitude)
        : FR_STRUCT_ZERO(Vector2);
}

/* Returns the angle (in radians) between `v1` and `v2`. */
FR_API_INLINE float frVec2Angle(Vector2 v1, Vector2 v2) {
    return atan2f(v2.y, v2.x) - atan2f(v1.y, v1.x);
}

/* Returns `true` if `v1` and `v2` are approximately equal. */
FR_API_INLINE bool frVec2ApproxEquals(Vector2 v1, Vector2 v2) {
    return frNumberApproxEquals(v1.x, v2.x) && frNumberApproxEquals(v1.y, v2.y);
}

/* Returns the left normal of `v`. */
FR_API_INLINE Vector2 frVec2LeftNormal(Vector2 v) {
    return frVec2Normalize((Vector2) { -v.y, v.x });
}

/* Returns the right normal of `v`. */
FR_API_INLINE Vector2 frVec2RightNormal(Vector2 v) {
    return frVec2Normalize((Vector2) { v.y, -v.x });
}

/* Rotates `v` around the origin. */
FR_API_INLINE Vector2 frVec2Rotate(Vector2 v, float angle) {
    const float sinA = sinf(angle);
    const float cosA = cosf(angle);
    
    return (Vector2) { (v.x * cosA - v.y * sinA), (v.x * sinA + v.y * cosA) };
}

/* Rotates `v` with the properties of `tx`. */
FR_API_INLINE Vector2 frVec2RotateTx(Vector2 v, frTransform tx) {
    Vector2 result = {
        (v.x * tx.cache.cosA - v.y * tx.cache.sinA),
        (v.x * tx.cache.sinA + v.y * tx.cache.cosA)
    };

    if (!tx.cache.valid) result = frVec2Rotate(v, tx.rotation);

    return result;
}

/* Transforms `v` with the properties of `tx`. */
FR_API_INLINE Vector2 frVec2Transform(Vector2 v, frTransform tx) {
    return frVec2Add(tx.position, frVec2RotateTx(v, tx));
}

/* Returns `true` if `v1`, `v2` and `v3` are ordered counter-clockwise. */
FR_API_INLINE bool frVec2CCW(Vector2 v1, Vector2 v2, Vector2 v3) {
    return (v3.y - v1.y) * (v2.x - v1.x) < (v2.y - v1.y) * (v3.x - v1.x);
}

/* Converts the components of `v` (in pixels) to meters. */
FR_API_INLINE Vector2 frVec2PixelsToMeters(Vector2 v) {
    return (FR_GLOBAL_PIXELS_PER_METER > 0.0f)
        ? frVec2ScalarMultiply(v, 1.0f / FR_GLOBAL_PIXELS_PER_METER)
        : FR_STRUCT_ZERO(Vector2);
}

/* Converts the components of `v` (in meters) to pixels. */
FR_API_INLINE Vector2 frVec2MetersToPixels(Vector2 v) {
    return (FR_GLOBAL_PIXELS_PER_METER > 0.0f)
        ? frVec2ScalarMultiply(v, FR_GLOBAL_PIXELS_PER_METER)
        : FR_STRUCT_ZERO(Vector2);
}

#ifdef __cplusplus
}
#endif

#endif
