#include <setenv.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#undef setenv

extern char ** environ;

// Static buffer used to store the environment array we allocate ourselves.
static char **env_buffer = NULL;
static int env_capacity = 0;
static int env_count = 0;

static char *dup_env_entry(const char *src) {
    size_t len;
    char *dst;

    if (src == NULL) {
        return NULL;
    }

    len = strlen(src) + 1;
    dst = (char *)malloc(len);
    if (dst == NULL) {
        return NULL;
    }
    memcpy(dst, src, len);
    return dst;
}

// Initialize the environment buffer.
static int init_env_buffer(void) {
    char **old_environ = environ;

    if (env_buffer != NULL) {
        return 0;
    }
    
    // Count the current number of environment entries.
    env_count = 0;
    if (old_environ != NULL) {
        for (char **ep = old_environ; *ep != NULL; ++ep) {
            env_count++;
        }
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
        env_buffer[i] = dup_env_entry(old_environ[i]);
        if (env_buffer[i] == NULL) {
            while (i > 0) {
                i--;
                free(env_buffer[i]);
            }
            free(env_buffer);
            env_buffer = NULL;
            env_capacity = 0;
            env_count = 0;
            return -1;
        }
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

int __ewok_setenv_impl(const char *name, const char *value, int overwrite) {
    size_t name_len;
    size_t value_len;
    size_t new_env_len;
    char *new_env;
    int existing_index;

    if (name == NULL || *name == '\0' || strchr(name, '=') != NULL) {
        errno = EINVAL;
        return -1;
    }
    if (value == NULL) {
        value = "";
    }

    if (init_env_buffer() != 0) {
        errno = ENOMEM;
        return -1;
    }

    name_len = strlen(name);
    existing_index = -1;
    for (int i = 0; i < env_count; i++) {
        if (strncmp(env_buffer[i], name, name_len) == 0 && env_buffer[i][name_len] == '=') {
            existing_index = i;
            break;
        }
    }

    if (existing_index >= 0 && overwrite == 0) {
        return 0;
    }

    value_len = strlen(value);
    new_env_len = name_len + value_len + 2;
    new_env = (char *)malloc(new_env_len);
    if (new_env == NULL) {
        errno = ENOMEM;
        return -1;
    }
    snprintf(new_env, new_env_len, "%s=%s", name, value);

    if (existing_index >= 0) {
        free(env_buffer[existing_index]);
        env_buffer[existing_index] = new_env;
        return 0;
    }

    if (expand_env_buffer() != 0) {
        free(new_env);
        errno = ENOMEM;
        return -1;
    }
    env_buffer[env_count] = new_env;
    env_count++;
    env_buffer[env_count] = NULL;
    return 0;
}

int setenv(const char *name, const char *value, ...) {
    return __ewok_setenv_impl(name, value, 1);
}
