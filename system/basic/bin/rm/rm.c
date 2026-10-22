#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/vfs.h>

static int remove_path(char* path, int recursive) {
    fsinfo_t info;
    if(vfs_get_by_name(path, &info) != 0) {
        fprintf(stderr, "rm: can't access [%s]\n", path);
        return -1;
    }

    if(!FS_IS_TYPE(info.type, FS_TYPE_DIR)) {
        if(unlink(path) == 0)
            return 0;
        fprintf(stderr, "rm: can't remove [%s]\n", path);
        return -1;
    }
    if(!recursive) {
        fprintf(stderr, "rm: [%s] is a directory (use -r)\n", path);
        return -1;
    }

    /* Keep a snapshot: removing children changes the live directory list. */
    uint32_t num = (uint32_t)-1;
    fsinfo_t* kids = vfs_kids(&info, &num);
    if(kids == NULL && num != 0) {
        fprintf(stderr, "rm: can't read directory [%s]\n", path);
        return -1;
    }

    int ret = 0;
    size_t len = strlen(path);
    for(uint32_t i = 0; i < num; i++) {
        const char* name = kids[i].name;
        if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;
        if(name[0] == 0 || strchr(name, '/') != NULL ||
                len + 1 + strlen(name) >= FS_FULL_NAME_MAX) {
            fprintf(stderr, "rm: invalid or too long child path [%s/%s]\n", path, name);
            ret = -1;
            continue;
        }
        path[len] = '/';
        strcpy(path + len + 1, name);
        if(remove_path(path, recursive) != 0)
            ret = -1;
        path[len] = 0;
    }
    free(kids);

    if(ret == 0 && rmdir(path) != 0) {
        fprintf(stderr, "rm: can't remove directory [%s]\n", path);
        ret = -1;
    }
    return ret;
}

int main(int argc, char* argv[]) {
    int recursive = 0;
    int option;
    while((option = getopt(argc, argv, "r")) != -1) {
        if(option != 'r') {
            fprintf(stderr, "Usage: rm [-r] [--] <path>...\n");
            return -1;
        }
        recursive = 1;
    }
    if(optind >= argc) {
        fprintf(stderr, "Usage: rm [-r] [--] <path>...\n");
        return -1;
    }

    int ret = 0;
    char fullname[FS_FULL_NAME_MAX];
    char cwd[FS_FULL_NAME_MAX];
    for(int i = optind; i < argc; i++) {
        const char* name = argv[i];
        if(name[0] == 0) {
            fprintf(stderr, "rm: empty path\n");
            ret = -1;
            continue;
        }
        while(name[0] == '.' && name[1] == '/') {
            name += 2;
            while(name[0] == '/')
                name++;
        }
        int n;
        if(name[0] == '/')
            n = snprintf(fullname, sizeof(fullname), "%s", name);
        else {
            if(getcwd(cwd, sizeof(cwd)) == NULL) {
                fprintf(stderr, "rm: can't get current directory\n");
                ret = -1;
                continue;
            }
            n = snprintf(fullname, sizeof(fullname), "%s%s%s", cwd,
                    strcmp(cwd, "/") == 0 ? "" : "/", name);
        }
        if(n < 0 || (size_t)n >= sizeof(fullname)) {
            fprintf(stderr, "rm: path too long [%s]\n", argv[i]);
            ret = -1;
            continue;
        }
        while(n > 1 && fullname[n - 1] == '/')
            fullname[--n] = 0;
        const char* base = strrchr(fullname, '/');
        base = base == NULL ? fullname : base + 1;
        if(strcmp(fullname, "/") == 0 || strcmp(base, ".") == 0 ||
                strcmp(base, "..") == 0 || name[0] == 0) {
            fprintf(stderr, "rm: refusing to remove [%s]\n", argv[i]);
            ret = -1;
            continue;
        }
        if(remove_path(fullname, recursive) != 0)
            ret = -1;
    }
    return ret;
}
