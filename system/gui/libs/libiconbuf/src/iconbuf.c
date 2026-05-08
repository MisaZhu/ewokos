#include "iconbuf/iconbuf.h"
#include <ewoksys/vfs.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ICON_BUF_DIR "/tmp/iconbuf"

graph_t* get_icon(const char* icon, uint32_t size) {
    if(icon == NULL)
        return NULL;

    char icon_file[FS_FULL_NAME_MAX+1];
    char icon_path[FS_FULL_NAME_MAX+1];
    if(icon[0] == '/')
        snprintf(icon_file, FS_FULL_NAME_MAX, "%s/%d%s.img", ICON_BUF_DIR, size, icon);
    else
        snprintf(icon_file, FS_FULL_NAME_MAX, "%s/%d/%s.img", ICON_BUF_DIR, size, icon);
    char* dir_name = vfs_dir_name(icon_file, icon_path, FS_FULL_NAME_MAX);

    int rd = 0;
    uint8_t* buf;
    int32_t w, h;
    buf = vfs_readfile(icon_file, &rd);
    if(buf != NULL) {
        if(rd < 8) {
            free(buf);
            return NULL;
        }
        w = *buf;
        h = *(buf+4);
        return graph_new(buf+8, w, h);
    }

    graph_t* img = graph_image_new(icon);
    if(img == NULL)
        return NULL;

    graph_t* ret = NULL;
    uint32_t sz = img->w > img->h ? img->w : img->h;
    if(sz == 0 || size == sz)
        ret = img;
    else {
        ret = graph_scalef_fast(img, ((float)size) / ((float)sz));
        graph_free(img);
    }
    
    if(vfs_get_by_name("/tmp", NULL) != 0 || ret == NULL) {
        return ret;
    }

    if(vfs_get_by_name(dir_name, NULL) != 0) {
		if(vfs_create(dir_name, NULL, FS_TYPE_DIR, 0777, false, true) != 0) {
    		return ret;
        }
	}

    int fd = open(icon_file, O_WRONLY | O_CREAT);
    if(fd < 0) {
        return ret;
    }

    if(write(fd, &ret->w, 4) != 4) {
        close(fd);
        return ret;
    }

    if(write(fd, &ret->h, 4) != 4) {
        close(fd);
        return ret;
    }

    write(fd, ret->buffer, ret->w * ret->h * 4);
    close(fd);
    return ret;
}

#ifdef __cplusplus
}
#endif

