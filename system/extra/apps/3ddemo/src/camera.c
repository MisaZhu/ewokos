#include "camera.h"

camera_t camera = {
    .position = { 0, 0, 0 },
    .direction = { 0, 0, 1 },
    .forward_velocity = { 0, 0, 0 },
    .yaw = 0
};
