
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "vector.h"
#include "mesh.h"
#include "matrix.h"
#include "light.h"
#include "texture.h"
#include "triangle.h"
#include "camera.h"
#include "clipping.h"

#define MAX_TRIANGLES 10000

float *z_buffer;
mat4_t proj_matrix;
mat4_t view_matrix;
mat4_t world_matrix;
triangle_t triangles_to_render[MAX_TRIANGLES];
int num_triangles_to_render;
int window_width;
int window_height;

void clear_z_buffer(){
	for(int i = 0; i<  window_width * window_height; i++)
        z_buffer[i] = 1.0f;
}



void setup(int width, int height){
  window_width = width;
  window_height = height;
  // Allocate the required memory in bytes to hold the color buffer
  z_buffer = (float*) malloc(sizeof(float) * window_width * window_height);

  // Initialize the perspective projection matrix
  float aspect_y = (float)window_height / (float)window_width;
  float aspect_x = (float)window_width / (float)window_height;
  float fov_y = 3.141592 / 3.0; // the same as 180/3, or 60deg
  float fov_x = atan(tan(fov_y / 2) * aspect_x) * 2;
  float z_near = 1.0;
  float z_far = 20.0;


  // Initialize frustum planes with a point and a normal
  init_frustum_planes(fov_x, fov_y, z_near, z_far);

  proj_matrix = mat4_make_projection(fov_y, aspect_x, z_near, z_far);

  clear_z_buffer();
}

void update(void) {
	// Initialize the counter of triangles to render for the current frame
  	num_triangles_to_render = 0;

  	mesh.translation.z = 4;
  	mesh.rotation.y += 0.02;

	static float xoff = 0.01;
	if(mesh.rotation.x > 0.3)
		xoff = -0.01;
	if(mesh.rotation.x < -0.3)
		xoff = 0.01;
	mesh.rotation.x += xoff;
  	//mesh.translation.y +=0.001;

  	// Initialize the target looking at the positive z-axis
  	vec3_t target = { 0, 0, 1 };
  	mat4_t camera_yaw_rotation = mat4_make_rotation_y(camera.yaw);
  	camera.direction = vec3_from_vec4(mat4_mul_vec4(camera_yaw_rotation, vec4_from_vec3(target)));

  	// Offset the camera position in the direction where the camera is pointing at
  	target = vec3_add(camera.position, camera.direction);
  	vec3_t up_direction = { 0, 1, 0 };

  	// Create the view matrix
  	view_matrix = mat4_look_at(camera.position, target, up_direction);

  	// Create matrices that will be used to multiply mesh vertices
  	mat4_t scale_matrix = mat4_make_scale(mesh.scale.x, mesh.scale.y, mesh.scale.z);

  	mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh.rotation.x);
  	mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh.rotation.y);
  	mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh.rotation.z);

  	mat4_t translation_matrix = mat4_make_translation(mesh.translation.x, mesh.translation.y, mesh.translation.z);

    // Loop all triangle faces of our mesh
    extern int num_faces;
    for (int i = 0; i < num_faces; i++) {

        // if(i!= 4) continue;

        face_t mesh_face = mesh.faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh.vertices[mesh_face.a - 1];
        face_vertices[1] = mesh.vertices[mesh_face.b - 1];
        face_vertices[2] = mesh.vertices[mesh_face.c - 1];

        vec3_t transformed_vertices[3];

        // Loop all three vertices of this current face and apply transformations
        for (int j = 0; j < 3; j++) {
            vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

            // World Matrix combining scale, rotation and translation matrices
            world_matrix = mat4_identity();
            // Order matters. First scale, then rotate, and then translate. [T]*[R]*[S]*v
            world_matrix = mat4_mul_mat4(scale_matrix,world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
            world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

            transformed_vertex = mat4_mul_vec4(world_matrix, transformed_vertex);

            // Multiply the view matrix by the vector to transform the scene to camera space
            transformed_vertex = mat4_mul_vec4(view_matrix, transformed_vertex);

            transformed_vertices[j] = vec3_from_vec4(transformed_vertex);
        }

        // Check backface culling
        vec3_t vector_a = transformed_vertices[0]; /*   A   */
        vec3_t vector_b = transformed_vertices[1]; /*  / \  */
        vec3_t vector_c = transformed_vertices[2]; /* C---B */

        // Get the vector subtraction of B-A and C-A
        vec3_t vector_ab = vec3_sub(vector_b, vector_a);
        vec3_t vector_ac = vec3_sub(vector_c, vector_a);
        vec3_normalize(&vector_ab);
        vec3_normalize(&vector_ac);

        // Compute the face normal (using cross product to find perpendicular)
        vec3_t normal = vec3_cross(vector_ab, vector_ac);
        vec3_normalize(&normal);

        // Find the vector between vertex A in the triangle and the camera origin
        vec3_t origin = { 0, 0, 0 };
        vec3_t camera_ray = vec3_sub(origin, vector_a);

        // How aligned the camera ray is with the face normal
        float dot_normal_camera = vec3_dot(normal, camera_ray);

        // Don't render if not facing camera
        if (dot_normal_camera < 0) {
            continue;
        }

       // Create a polygon from the original transformed triangle to be clipped
        polygon_t polygon = polygon_from_triangle(
            transformed_vertices[0],
            transformed_vertices[1],
            transformed_vertices[2],
            mesh_face.a_uv,
            mesh_face.b_uv,
            mesh_face.c_uv
        );

        // Clip the polygon and returns a new polygon with potential new vertices
        clip_polygon(&polygon);

      // Break the clipped polygon apart back into individual triangles
        triangle_t triangles_after_clipping[MAX_NUM_POLY_TRIANGLES];
        int num_triangles_after_clipping = 0;

        triangles_from_polygon(&polygon, triangles_after_clipping, &num_triangles_after_clipping);

        // Loops all the assembled triangles after clipping
        for (int t = 0; t < num_triangles_after_clipping; t++) {
            triangle_t triangle_after_clipping = triangles_after_clipping[t];

            vec4_t projected_points[3];

            // Loop all three vertices to perform projection and conversion to screen space
            for (int j = 0; j < 3; j++) {
                // Project the current vertex using a perspective projection matrix
                projected_points[j] = mat4_mul_vec4(proj_matrix, triangle_after_clipping.points[j]);

                // Perform perspective divide
                if (projected_points[j].w != 0) {
                    projected_points[j].x /= projected_points[j].w;
                    projected_points[j].y /= projected_points[j].w;
                    projected_points[j].z /= projected_points[j].w;
                }

                // Flip vertically since the y values of the 3D mesh grow bottom->up and in screen space y values grow top->down
                projected_points[j].y *= -1;

                // Scale into the view
                projected_points[j].x *= (window_width / 2.0);
                projected_points[j].y *= (window_height / 2.0);

                // Translate the projected points to the middle of the screen
                projected_points[j].x += (window_width / 2.0);
                projected_points[j].y += (window_height / 2.0);
            }

            // Calculate the shade intensity based on how aliged is the normal with the flipped light direction ray
            float light_intensity_factor = -vec3_dot(normal, light.direction);


            // Calculate the triangle color based on the light angle
            uint32_t triangle_color = light_apply_intensity(mesh_face.color, light_intensity_factor);

            // Create the final projected triangle that will be rendered in screen space
            triangle_t triangle_to_render = {
                .points = {
                    { projected_points[0].x, projected_points[0].y, projected_points[0].z, projected_points[0].w },
                    { projected_points[1].x, projected_points[1].y, projected_points[1].z, projected_points[1].w },
                    { projected_points[2].x, projected_points[2].y, projected_points[2].z, projected_points[2].w }
                },
                .texcoords = {
                    { triangle_after_clipping.texcoords[0].u, triangle_after_clipping.texcoords[0].v },
                    { triangle_after_clipping.texcoords[1].u, triangle_after_clipping.texcoords[1].v },
                    { triangle_after_clipping.texcoords[2].u, triangle_after_clipping.texcoords[2].v }
                },
                .color = triangle_color
            };

            // Save the projected triangle in the array of triangles to render
            if (num_triangles_to_render < MAX_TRIANGLES) {
                triangles_to_render[num_triangles_to_render++] = triangle_to_render;
            }
        }

    }


    // Sort the triangle to render by their avg_depth
}

void render(void){
  // Loop all projected triangles and render them

  for (int i = 0; i< num_triangles_to_render; i++){
    triangle_t triangle = triangles_to_render[i];

    // Draw textured triangle
    draw_textured_triangle(
          triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w, triangle.texcoords[0].u, triangle.texcoords[0].v, // vertex A
          triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w, triangle.texcoords[1].u, triangle.texcoords[1].v, // vertex B
          triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w, triangle.texcoords[2].u, triangle.texcoords[2].v, // vertex C
          mesh_texture
      );
  }

  clear_z_buffer();
}

