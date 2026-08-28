#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ewoksys/vfs.h>

DIR* opendir(const char* name) {
    fsinfo_t info;
    if(vfs_get_by_name(name, &info) != 0)
        return NULL;
    if(!FS_IS_TYPE(info.type, FS_TYPE_DIR)) {
        errno = ENOTDIR;
        return NULL;
    }
    
    uint32_t num = 0;
    fsinfo_t* kids = vfs_kids(&info, &num);
    DIR* ret = (DIR*)malloc(sizeof(DIR));
    if(ret == NULL) {
        errno = ENOMEM;
        if(kids != NULL)
            free(kids);
        return NULL;
    }

    memset(ret, 0, sizeof(DIR));
    ret->kids = kids;
    ret->num = num;
    ret->offset = 0;
    ret->fd = -1;
    strncpy(ret->name, name, FS_NODE_NAME_MAX - 1);
    return ret;
}
