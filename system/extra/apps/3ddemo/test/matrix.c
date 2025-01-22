#include <math.h>

#include "matrix.h"
#include "stdio.h"

mat4_t mat4_identity(void) {
  // | 1 0 0 0 |
  // | 0 1 0 0 |
  // | 0 0 1 0 |
  // | 0 0 0 1 |

  mat4_t eye = { // eye for identity
    .m = { // can skip the .m = part - it's just for clarity
      {1, 0, 0, 0}, 
      {0, 1, 0, 0}, 
      {0, 0, 1, 0}, 
      {0, 0, 0, 1}
    }
  };
  return eye;
}

vec4_t mat4_mul_vec4(mat4_t m, vec4_t v){
  // Example of this multiplication (values can be all different):
  // | sx 0 0 0 |   | x |   | x'|
  // | 0 sy 0 0 | X | y | = | y'|
  // | 0 0 sz 0 |   | z |   | z'|
  // | 0 0 0  1 |   | 1 |   | 1 |

  vec4_t result;

  result.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w;
  result.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w;
  result.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w;
  result.w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w;

  return result;
}

mat4_t mat4_mul_mat4(mat4_t a, mat4_t b) {
  mat4_t m;
  for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
          m.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
      }
  }
  return m;
}

mat4_t mat4_make_scale(float sx, float sy, float sz){
  // | sx 0 0 0 |
  // | 0 sy 0 0 |
  // | 0 0 sz 0 |
  // | 0 0 0 1 |

  mat4_t m = mat4_identity();

  m.m[0][0] = sx;
  m.m[1][1] = sy;
  m.m[2][2] = sz;

  return m;
}

mat4_t mat4_make_translation(float tx, float ty, float tz){
  // | 1 0 0 tx |
  // | 0 1 0 ty |
  // | 0 0 1 tz | 
  // | 0 0 0  1 | 

  mat4_t m = mat4_identity();

  m.m[0][3] = tx;
  m.m[1][3] = ty;
  m.m[2][3] = tz;

  return m;
}

mat4_t mat4_make_rotation_x(float angle) {
  float c = cos(angle);
  float s = sin(angle);
  // | 1  0  0  0 |
  // | 0  c -s  0 |
  // | 0  s  c  0 |
  // | 0  0  0  1 |
  mat4_t m = mat4_identity();
  m.m[1][1] = c;
  m.m[1][2] = -s;
  m.m[2][1] = s;
  m.m[2][2] = c;
  return m;
}

mat4_t mat4_make_rotation_y(float angle) {
  float c = cos(angle);
  float s = sin(angle);
  // |  c  0  s  0 |
  // |  0  1  0  0 |
  // | -s  0  c  0 |
  // |  0  0  0  1 |
  mat4_t m = mat4_identity();
  m.m[0][0] = c;
  m.m[0][2] = s;
  m.m[2][0] = -s;
  m.m[2][2] = c;
  return m;
}

mat4_t mat4_make_rotation_z(float angle) {
  float c = cos(angle);
  float s = sin(angle);
  // | c -s  0  0 |
  // | s  c  0  0 |
  // | 0  0  1  0 |
  // | 0  0  0  1 |
  mat4_t m = mat4_identity();
  m.m[0][0] = c;
  m.m[0][1] = -s;
  m.m[1][0] = s;
  m.m[1][1] = c;
  return m;
}

//https://www.youtube.com/watch?v=U0_ONQQ5ZNM - more explanation of this those matrices
//https://www.youtube.com/watch?v=vu1VNKHfzqQ here too - start with this one

vec4_t mat4_mul_vec4_project(mat4_t mat_proj, vec4_t v) {
  // multiply the projection matrix by our original vector
  vec4_t result = mat4_mul_vec4(mat_proj, v);

  // perform perspective divide with original z-value that is now stored in w
  if (result.w != 0.0) {
      result.x /= result.w;
      result.y /= result.w;
      result.z /= result.w;
  }
  return result;
}

// translate and scale projected vector to -1 to 1 range
mat4_t mat4_make_ortho(float l, float b, float n, float r, float t, float f) {
  // translate to origin and scale to -1 to 1 range -> width, height, depth should be 2

  // Coordinates of center of the provided cube are:
  float c_x = (l + r) / 2;
  float c_y = (b + t) / 2;
  float c_z = (n + f) / 2;

  // To translate the cube to the origin, we need to subtract the center coordinates from each vertex
  // | 1 0 0 -c_x |
  // | 0 1 0 -c_y |
  // | 0 0 1 -c_z |
  // | 0 0 0    1 |
  mat4_t trans = mat4_identity();
  trans.m[0][3] = -c_x;
  trans.m[1][3] = -c_y;
  trans.m[2][3] = -c_z;

  // Current width, height, depth of the cube:
  float width = r - l;
  float height = t - b;
  float depth = f - n;

  // To scale the cube to have width, height, depth of 2 (be from -1 to 1)
  // we need to multiply each vertex by 2/width, 2/height, 2/depth
  // | 2/width 0        0        0 |
  // | 0       2/height  0       0 |
  // | 0       0        2/depth  0 |
  // | 0       0        0        1 |

  mat4_t scale = mat4_identity();
  scale.m[0][0] = 2.0 / width;
  scale.m[1][1] = 2.0 / height;
  scale.m[2][2] = 2.0 / depth;

  // Now we need to combine the two matrices into one
  return mat4_mul_mat4(trans, scale);
}

mat4_t mat4_make_perspective(float n, float f) {
  // | n  0       0     0 |
  // | 0  n       0     0 |
  // | 0  0   (f+n) (-fn) |
  // | 0  0       1     0 |

  mat4_t m = {{{ 0 }}};
  m.m[0][0] = n;
  m.m[1][1] = n;
  m.m[2][2] = f + n;
  m.m[2][3] = -f * n;
  m.m[3][2] = 1.0;

  return m;
} 

mat4_t mat4_make_projection(float fov, float aspect_ratio, float near, float far) {
  // Orthographic matrix X Perspective matrix

  float r = near * tan(fov / 2.0) * aspect_ratio; // aspect_ratio = width / height
  float t = near * tan(fov / 2.0);
  float f = far;

  float l = -r;
  float b = -t;
  float n = near;

  mat4_t t_ortho = mat4_make_ortho(l, b, n, r, t, f);
  mat4_t t_perspective = mat4_make_perspective(near, far);

  mat4_t m = mat4_mul_mat4(t_ortho, t_perspective);
  return m;
}

mat4_t mat4_make_perspective_old(float fov, float aspect, float znear, float zfar){
  // | (w/h)*1/tan(fov/2)             0              0                 0 |
  // |                  0  1/tan(fov/2)              0                 0 |
  // |                  0             0     zf/(zf-zn)  (-zf*zn)/(zf-zn) |
  // |                  0             0              1                 0 |
  mat4_t m = {{{ 0 }}};
  m.m[0][0] = aspect * (1 / tan(fov / 2));
  m.m[1][1] = 1 / tan(fov / 2);
  m.m[2][2] = zfar / (zfar - znear);
  m.m[2][3] = (-zfar * znear) / (zfar - znear);
  m.m[3][2] = 1.0;
  return m;
}

mat4_t mat4_look_at(vec3_t eye, vec3_t target, vec3_t up) {
    // Compute the forward (z), right (x), and up (y) vectors
    vec3_t z = vec3_sub(target, eye);
    vec3_normalize(&z);
    vec3_t x = vec3_cross(up, z);
    vec3_normalize(&x);
    vec3_t y = vec3_cross(z, x);

    // | x.x   x.y   x.z  -dot(x,eye) |
    // | y.x   y.y   y.z  -dot(y,eye) |
    // | z.x   z.y   z.z  -dot(z,eye) |
    // |   0     0     0            1 |
    mat4_t view_matrix = {{
        { x.x, x.y, x.z, -vec3_dot(x, eye) },
        { y.x, y.y, y.z, -vec3_dot(y, eye) },
        { z.x, z.y, z.z, -vec3_dot(z, eye) },
        {   0,   0,   0,                 1 }
    }};
    return view_matrix;
}
