#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <ewoksys/md5.h>
#include <ewoksys/mstr.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/keydef.h>
#include <ewoksys/session.h>
#include <ewoksys/klog.h>

#define USER_NUM_MAX 64
#define GROUP_NUM_MAX 64

static session_info_t _users[USER_NUM_MAX];
static int _user_num = 0;
static session_group_t _groups[GROUP_NUM_MAX];
static int _group_num = 0;

static int load_session_db(void);

static void skip_line(int fd) {
    char c;
    while(read(fd, &c, 1) == 1) {
        if(c == '\n')
            return;
    }
}

static char skip_space(int fd) {
    char c;
    while(true) {
        if(read(fd, &c , 1) != 1)
            break;
        if(c == '#') {
            skip_line(fd);
            continue;
        }
        if(c != ' ' && c != '\t' && c != '\r' &&  c != '\n')
            return c;
    }
    return 0;
}

static int read_value(int fd, char* val, uint32_t len, bool start) {
    str_t* s = str_new("");
    char c = 0;
    uint32_t copy_len;

    if(s == NULL || val == NULL || len == 0) {
        str_free(s);
        return -1;
    }

    if(start) {
        c = skip_space(fd);
        if(c == 0) {
            str_free(s);
            return -1;
        }
        str_addc(s, c);
    }

    while(true) {
        if(read(fd, &c , 1) != 1) {
            break;
        }
        if(c == ':' || c == '\r' || c == '\n')
            break;

        str_addc(s, c);
    }
    memset(val, 0, len);
    copy_len = s->len;
    if(copy_len >= len)
        copy_len = len - 1;
    memcpy(val, s->cstr, copy_len);
    val[copy_len] = 0;
    str_free(s);
    return 0;
}

static int read_user_item(int fd, session_info_t* info) {
    memset(info, 0, sizeof(session_info_t));
    if(read_value(fd, info->user, SESSION_USER_MAX, true) != 0)
        return -1;

    char id[32];
    if(read_value(fd, id, 32, false) != 0)
        return -1;
    info->gid = atoi(id);
    
    if(read_value(fd, id, 32, false) != 0)
        return -1;
    info->uid = atoi(id);

    if(read_value(fd, info->home, SESSION_HOME_MAX, false) != 0)
        return -1;

    if(read_value(fd, info->cmd, SESSION_CMD_MAX, false) != 0)
        return -1;

    if(read_value(fd, info->password, SESSION_PSWD_MAX, false) != 0)
        return -1;
    return 0;
}

static int read_group_item(int fd, session_group_t* info) {
    char val[32];
    char ignore[64];

    memset(info, 0, sizeof(session_group_t));
    if(read_value(fd, info->group, SESSION_GROUP_MAX, true) != 0)
        return -1;

    if(read_value(fd, ignore, sizeof(ignore), false) != 0)
        return -1;

    if(read_value(fd, val, sizeof(val), false) != 0)
        return -1;
    info->gid = atoi(val);

    /* Ignore the member list for now. */
    read_value(fd, ignore, sizeof(ignore), false);
    return 0;
}

static int read_user_info(void) {
    int fd = open("/etc/passwd", O_RDONLY);
    if(fd < 0) {
        fprintf(stderr, "Error, open password file failed!\n");
        return -1;
    }
    _user_num = 0;
    memset(_users, 0, sizeof(_users));

    while(_user_num < USER_NUM_MAX) {
        session_info_t* info = &_users[_user_num];
        if(read_user_item(fd, info) != 0)
            break;
        _user_num++;
    }
    close(fd);
    return 0;
}

static int read_group_info(void) {
    int fd = open("/etc/group", O_RDONLY);
    if(fd < 0) {
        fprintf(stderr, "Warning, open group file failed!\n");
        _group_num = 0;
        memset(_groups, 0, sizeof(_groups));
        return 0;
    }
    _group_num = 0;
    memset(_groups, 0, sizeof(_groups));

    while(_group_num < GROUP_NUM_MAX) {
        session_group_t* info = &_groups[_group_num];
        if(read_group_item(fd, info) != 0)
            break;
        _group_num++;
    }
    close(fd);
    return 0;
}

static int load_session_db(void) {
    if(read_user_info() != 0)
        return -1;
    return read_group_info();
}

static session_info_t* check(const char* user, const char* password, int* res) {
    int i;
    *res = 0;
    for(i=0; i<_user_num; i++) {
        session_info_t* info = &_users[i];
        if(strcmp(info->user, user) == 0) {
            if(info->password[0] == 0)
                return info;
            const char* md5 = md5_encode_str((uint8_t*)password, strlen(password));
            if(strcmp(info->password, md5) == 0) 
                return info;
            else {
                *res = SESSION_ERR_PWD; //Wrong password
                return NULL;
            }
        }
    }
    *res = SESSION_ERR_USR; //user not existed
    return NULL;
}

static session_info_t* get_by_name(const char* user) {
    int i;
    for(i=0; i<_user_num; i++) {
        session_info_t* info = &_users[i];
        if(strcmp(info->user, user) == 0) {
            return info;
        }
    }
    return NULL;
}

static session_info_t* get_by_uid(int32_t uid) {
    int i;
    for(i=0; i<_user_num; i++) {
        session_info_t* info = &_users[i];
        if(info->uid == uid) {
            return info;
        }
    }
    return NULL;
}

static session_group_t* get_group_by_name(const char* group) {
    int i;
    for(i=0; i<_group_num; i++) {
        session_group_t* info = &_groups[i];
        if(strcmp(info->group, group) == 0) {
            return info;
        }
    }
    return NULL;
}

static session_group_t* get_group_by_gid(int32_t gid) {
    int i;
    for(i=0; i<_group_num; i++) {
        session_group_t* info = &_groups[i];
        if(info->gid == gid) {
            return info;
        }
    }
    return NULL;
}

static void ensure_home_dir(const session_info_t* sinfo) {
    if(sinfo->home[0] == 0)
        return;

    /* create parent directories if needed (e.g. /tmp/home for /tmp/home/guest) */
    char path[SESSION_HOME_MAX];
    strncpy(path, sinfo->home, SESSION_HOME_MAX - 1);
    path[SESSION_HOME_MAX - 1] = 0;
    for(char* p = path + 1; *p != 0; p++) {
        if(*p == '/') {
            *p = 0;
            if(access(path, F_OK) != 0)
                mkdir(path, 0755);
            *p = '/';
        }
    }

    if(access(sinfo->home, F_OK) != 0) {
        if(mkdir(sinfo->home, 0750) != 0)
            return;
        /* set directory permission and ownership */
        chmod(sinfo->home, 0750);
        chown(sinfo->home, sinfo->uid, sinfo->gid);
    }
}

static session_info_t* secure_session(const session_info_t* sinfo) {
    ensure_home_dir(sinfo);

    static session_info_t to;
    memcpy(&to, sinfo, sizeof(session_info_t));
    memset(to.password, 0, SESSION_PSWD_MAX);
    return &to;
}

static void do_session_get_by_uid(int pid, proto_t* in, proto_t* out) {
    int32_t uid = proto_read_int(in);
    session_info_t* sinfo = get_by_uid(uid);

    PF->clear(out)->addi(out, -1);
    if(sinfo == NULL)
            return;
    sinfo = secure_session(sinfo);
    PF->clear(out)->addi(out, 0)->add(out, sinfo, sizeof(session_info_t));
}

static void do_session_get_by_name(int pid, proto_t* in, proto_t* out) {
    const char* name = proto_read_str(in);
    session_info_t* sinfo = get_by_name(name);

    PF->clear(out)->addi(out, -1);
    if(sinfo == NULL)
            return;
    sinfo = secure_session(sinfo);
    PF->clear(out)->addi(out, 0)->add(out, sinfo, sizeof(session_info_t));
}

static void do_session_check(int pid, proto_t* in, proto_t* out) {
    const char* name = proto_read_str(in);
    const char* passwd = proto_read_str(in);
    int res = 0;
    session_info_t* sinfo = check(name, passwd, &res);
    PF->clear(out)->addi(out, res);
    if(sinfo == NULL)
            return;

    sinfo = secure_session(sinfo);
    PF->clear(out)->addi(out, 0)->add(out, sinfo, sizeof(session_info_t));
}

static void do_session_get_group_by_gid(int pid, proto_t* in, proto_t* out) {
    (void)pid;
    int32_t gid = proto_read_int(in);
    session_group_t* ginfo = get_group_by_gid(gid);

    PF->clear(out)->addi(out, -1);
    if(ginfo == NULL)
        return;
    PF->clear(out)->addi(out, 0)->add(out, ginfo, sizeof(session_group_t));
}

static void do_session_get_group_by_name(int pid, proto_t* in, proto_t* out) {
    (void)pid;
    const char* name = proto_read_str(in);
    session_group_t* ginfo = get_group_by_name(name);

    PF->clear(out)->addi(out, -1);
    if(ginfo == NULL)
        return;
    PF->clear(out)->addi(out, 0)->add(out, ginfo, sizeof(session_group_t));
}

static void do_session_reload(int pid, proto_t* in, proto_t* out) {
    (void)pid;
    (void)in;
    int res = load_session_db();
    PF->clear(out)->addi(out, res);
}

static void handle_ipc(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
    (void)p;
    pid = proc_getpid(pid);

    switch(cmd) {
    case SESSION_CHECK: 
        do_session_check(pid, in, out);
        return;
    case SESSION_GET_BY_UID: 
        do_session_get_by_uid(pid, in, out);
        return;
    case SESSION_GET_BY_NAME: 
        do_session_get_by_name(pid, in, out);
        return;
    case SESSION_GET_GROUP_BY_GID:
        do_session_get_group_by_gid(pid, in, out);
        return;
    case SESSION_GET_GROUP_BY_NAME:
        do_session_get_group_by_name(pid, in, out);
        return;
    case SESSION_SET:
        do_session_reload(pid, in, out);
        return;
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if(load_session_db() != 0)
        return -1;

    if(ipc_serv_reg(IPC_SERV_SESSIOND) != 0) {
        slog("reg sessiond ipc_serv error!\n");
        return -1;
    }

    ipc_serv_run(handle_ipc, NULL, NULL, 0);

    while(1) {
        usleep(100000);
    }
    return 0;
}
