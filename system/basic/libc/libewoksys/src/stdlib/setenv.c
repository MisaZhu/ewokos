#include <setenv.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#undef setenv

extern char ** environ;

// 静态指针，用于存储我们自己分配的环境变量数组
static char **env_buffer = NULL;
static int env_capacity = 0;
static int env_count = 0;

// 初始化环境变量缓冲区
static int init_env_buffer(void) {
    if (env_buffer != NULL) {
        return 0;
    }
    
    // 计算当前环境变量数量
    env_count = 0;
    for (char **ep = environ; *ep != NULL; ++ep) {
        env_count++;
    }
    
    // 初始容量为当前数量 + 16
    env_capacity = env_count + 16;
    
    // 分配新的缓冲区
    env_buffer = (char **)malloc(env_capacity * sizeof(char *));
    if (env_buffer == NULL) {
        return -1;
    }
    
    // 复制现有的环境变量指针
    for (int i = 0; i < env_count; i++) {
        env_buffer[i] = environ[i];
    }
    env_buffer[env_count] = NULL;
    
    // 更新 environ 指向我们的缓冲区
    environ = env_buffer;
    
    return 0;
}

// 扩展环境变量缓冲区
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

// 自定义的 setenv 函数实现
int setenv(const char *name, const char *value) {
    //klog("setenv: %s=%s\n", name, value);
    // 检查环境变量名是否为空
    if (name == NULL || *name == '\0' || strchr(name, '=') != NULL) {
        return -1;
    }

    // 初始化环境变量缓冲区
    if (init_env_buffer() != 0) {
        return -1;
    }

    // 构建新的环境变量字符串
    size_t name_len = strlen(name);
    size_t value_len = strlen(value);
    size_t new_env_len = name_len + value_len + 2; // 加上 '=' 和 '\0'
    char *new_env = (char *)malloc(new_env_len);
    if (new_env == NULL) {
        return -1;
    }
    snprintf(new_env, new_env_len, "%s=%s", name, value);

    // 查找是否已有同名环境变量
    int existing_index = -1;
    for (int i = 0; i < env_count; i++) {
        if (strncmp(env_buffer[i], name, name_len) == 0 && env_buffer[i][name_len] == '=') {
            existing_index = i;
            break;
        }
    }

    if (existing_index >= 0) {
        // 替换现有的环境变量
        free(env_buffer[existing_index]);
        env_buffer[existing_index] = new_env;
    } else {
        // 添加新的环境变量
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
