#include <setenv.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef setenv
extern char ** environ;
// 自定义的 setenv 函数实现
int setenv(const char *name, const char *value) {
    // 检查环境变量名是否为空
    if (name == NULL || *name == '\0' || strchr(name, '=') != NULL) {
        return -1;
    }

    // 检查是否需要覆盖现有环境变量
    char *existing = getenv(name);

    // 构建新的环境变量字符串
    size_t name_len = strlen(name);
    size_t value_len = strlen(value);
    size_t new_env_len = name_len + value_len + 2; // 加上 '=' 和 '\0'
    char *new_env = (char *)malloc(new_env_len);
    if (new_env == NULL) {
        return -1;
    }
    snprintf(new_env, new_env_len, "%s=%s", name, value);

    // 如果已有同名环境变量，先删除它
    if (existing != NULL) {
        char **ep;
        for (ep = environ; *ep != NULL; ++ep) {
            if (strncmp(*ep, name, name_len) == 0 && (*ep)[name_len] == '=') {
                // 找到匹配的环境变量，将后续的环境变量向前移动
                while (*(ep + 1) != NULL) {
                    *ep = *(ep + 1);
                    ep++;
                }
                *ep = NULL;
                break;
            }
        }
    }

    // 计算环境变量数组的长度
    int env_count = 0;
    for (char **ep = environ; *ep != NULL; ++ep) {
        env_count++;
    }

    // 重新分配环境变量数组的内存，为新的环境变量腾出空间
    environ = (char **)realloc(environ, (env_count + 2) * sizeof(char *));
    if (environ == NULL) {
        free(new_env);
        return -1;
    }

    // 将新的环境变量添加到数组末尾
    environ[env_count] = new_env;
    environ[env_count + 1] = NULL;

    return 0;
}
#endif