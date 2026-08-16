#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ewoksys/vfs.h>
#include <ewoksys/session.h>

static int parse_user(const char* spec, int* uid, int* gid, int use_default_gid) {
    session_info_t sinfo;
    char c = spec[0];

    if(c >= '0' && c <= '9') {
        *uid = atoi(spec);
        if(!use_default_gid)
            return 0;
        if(session_get_by_uid(*uid, &sinfo) != 0) {
            printf("UID [%d] not exist!\n", *uid);
            return -1;
        }
        *gid = sinfo.gid;
        return 0;
    }

    if(session_get_by_name(spec, &sinfo) != 0) {
        printf("User [%s] not exist!\n", spec);
        return -1;
    }
    *uid = sinfo.uid;
    if(use_default_gid)
        *gid = sinfo.gid;
    return 0;
}

static int parse_group(const char* spec, int* gid) {
    session_group_t ginfo;
    char c = spec[0];

    if(c >= '0' && c <= '9') {
        *gid = atoi(spec);
        return 0;
    }

    if(session_get_group_by_name(spec, &ginfo) != 0) {
        printf("Group [%s] not exist!\n", spec);
        return -1;
    }
    *gid = ginfo.gid;
    return 0;
}

int main(int argc, char* argv[]) {
    if(argc < 3) {
        printf("Usage: chown <user>[:group] <fname>\n");
        return -1;
    }

    int uid = 0;
    int gid = 0;
    char owner[SESSION_USER_MAX] = {0};
    char group[SESSION_GROUP_MAX] = {0};
    const char* spec = argv[1];
    const char* sep = strchr(spec, ':');
    char fullname[FS_FULL_NAME_MAX+1] = {0};
    struct stat st;

    vfs_fullname(argv[2], fullname, FS_FULL_NAME_MAX);
    if(stat(fullname, &st) != 0) {
        printf("Can't stat [%s]!\n", fullname);
        return -1;
    }

    if(sep != NULL) {
        size_t owner_len = (size_t)(sep - spec);
        size_t group_len = strlen(sep + 1);
        if(owner_len >= sizeof(owner) ||
                group_len == 0 || group_len >= sizeof(group)) {
            printf("Invalid owner/group spec [%s]!\n", spec);
            return -1;
        }
        memcpy(group, sep + 1, group_len + 1);
        if(owner_len == 0)
            uid = st.st_uid;
        else {
            memcpy(owner, spec, owner_len);
            owner[owner_len] = 0;

            if(parse_user(owner, &uid, &gid, 0) != 0)
                return -1;
        }
        if(parse_group(group, &gid) != 0)
            return -1;
    }
    else if(parse_user(spec, &uid, &gid, 1) != 0) {
        return -1;
    }

    if(chown(fullname, uid, gid) != 0) {
        printf("Can't chown [%s]!\n", fullname);
        return -1;
    }
    return 0;
}
