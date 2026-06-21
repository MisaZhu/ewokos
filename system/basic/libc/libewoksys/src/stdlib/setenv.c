#include <setenv.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#undef setenv

extern char ** environ;

// Static buffer used to store the environment array we allocate ourselves.
static char **env_buffer = NULL;
static int env_capacity = 0;
static int env_count = 0;

// Initialize the environment buffer.
static int init_env_buffer(void) {
    if (env_buffer != NULL) {
        return 0;
    }
    
    // Count the current number of environment entries.
    env_count = 0;
    for (char **ep = environ; *ep != NULL; ++ep) {
        env_count++;
    }
    
    // Start with the current size plus some spare room.
    env_capacity = env_count + 16;
    
    // Allocate a new buffer.
    env_buffer = (char **)malloc(env_capacity * sizeof(char *));
    if (env_buffer == NULL) {
        return -1;
    }
    
    // Copy the existing environment pointers.
    for (int i = 0; i < env_count; i++) {
        env_buffer[i] = environ[i];
    }
    env_buffer[env_count] = NULL;
    
    // Redirect environ to our managed buffer.
    environ = env_buffer;
    
    return 0;
}

// Grow the environment buffer when needed.
static int expand_env_buffer(void) {
    if (env_count + 1 >= env_capacity) {
        env_capacity += 16;
        char **new_buffer = (char **)realloc(env_buffer, env_capacity * sizeof(char *));
        if (new_buffer == NULL) {
            return -1;
        }
        env_buffer = new_buffer;
        environ = env_buffer;
    }
    return 0;
}

// Custom setenv implementation.
int setenv(const char *name, const char *value, ...) {
    //klog("setenv: %s=%s\n", name, value);
    // Reject invalid environment variable names.
    if (name == NULL || *name == '\0' || strchr(name, '=') != NULL) {
        return -1;
    }

    // Initialize the environment buffer on first use.
    if (init_env_buffer() != 0) {
        return -1;
    }

    // Build the new NAME=VALUE string.
    size_t name_len = strlen(name);
    size_t value_len = strlen(value);
    size_t new_env_len = name_len + value_len + 2; // Include '=' and '\0'
    char *new_env = (char *)malloc(new_env_len);
    if (new_env == NULL) {
        return -1;
    }
    snprintf(new_env, new_env_len, "%s=%s", name, value);

    // Check whether the variable already exists.
    int existing_index = -1;
    for (int i = 0; i < env_count; i++) {
        if (strncmp(env_buffer[i], name, name_len) == 0 && env_buffer[i][name_len] == '=') {
            existing_index = i;
            break;
        }
    }

    if (existing_index >= 0) {
        // Replace the existing entry.
        free(env_buffer[existing_index]);
        env_buffer[existing_index] = new_env;
    } else {
        // Append a new environment entry.
        if (expand_env_buffer() != 0) {
            free(new_env);
            return -1;
        }
        env_buffer[env_count] = new_env;
        env_count++;
        env_buffer[env_count] = NULL;
    }

    return 0;
}
