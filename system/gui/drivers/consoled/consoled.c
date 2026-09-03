#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <gterminal/gterminal.h>
#include <display/display.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/keydef.h>
#include <displayman/displayman.h>
#include <graph/graph_png.h>
#include <tinyjson/tinyjson.h>
#include <ewoksys/klog.h>
#include <ewoksys/sys.h>
#include <ewoksys/syscall.h>
#include <ewoksys/core.h>
#include <ewoksys/charbuf.h>
#include <sysinfo.h>
#include <font/font.h>
#include <keyb/keyb.h>
#include <ewoksys/tty.h>

typedef struct {
    const char* id;
    const char* display_dev;
    uint32_t    display_index;
    display_t    display;
    graph_t*    g;
    gterminal_t terminal;
    graph_t*    icon;
} fb_console_t;

static int32_t read_config(fb_console_t* console, const char* fname) {
    json_var_t *conf_var = json_parse_file(fname);	

    console->terminal.fg_color = 0xffcccccc;
    console->terminal.bg_color = 0xff000000;

    const char* font_fname = json_get_str_def(conf_var, "font", DEFAULT_SYSTEM_FONT);
    if(console->terminal.font != NULL)
        font_free(console->terminal.font);
    console->terminal.font = font_new(font_fname, true);

    const char* v = json_get_str_def(conf_var, "icon", "");
    if(v[0] != 0) 
        console->icon = png_image_new(v);

    console->terminal.bg_color = json_get_int_def(conf_var, "bg_color", 0xff000000);
    console->terminal.fg_color = json_get_int_def(conf_var, "fg_color", 0xffcccccc);
    console->terminal.font_size = json_get_int_def(conf_var, "font_size", 12);;
    console->terminal.char_space = json_get_int_def(conf_var, "char_space", 0);;
    console->terminal.line_space = json_get_int_def(conf_var, "line_space", 0);;

    if(conf_var != NULL)
        json_var_unref(conf_var);
    return 0;
}

static void init_graph(fb_console_t* console) {
    console->g = display_fetch_graph(&console->display);
    graph_clear(console->g, 0xff000000);
}

static int init_console(fb_console_t* console, const char* display_dev, const uint32_t display_index) {
    memset(console, 0, sizeof(fb_console_t));
    font_init();

    console->display_dev = display_dev;
    console->display_index = display_index;
    if(displayman_open(display_dev, console->display_index, &console->display) != 0)
        return -1;
    init_graph(console);
    gterminal_init(&console->terminal);
    read_config(console, "/etc/console.json");
    return 0;
}

static void close_console(fb_console_t* console) {
    if(console->g != NULL)
        graph_free(console->g);

    display_close(&console->display);
    if(console->icon != NULL)
        graph_free(console->icon);
    gterminal_close(&console->terminal);
    font_quit();
}

static int reset_console(fb_console_t* console) {
    gterminal_resize(&console->terminal, console->g->w, console->g->h);
    return 0;
}

static void draw_bg(fb_console_t* console) {
    graph_t* g = console->g;
    graph_clear(g, console->terminal.bg_color);
}

static void flush(fb_console_t* console) {
    draw_bg(console);

    if(console->display_index == 0) {
        //draw cores
        if(console->icon != NULL) {
            sys_info_t sys_info;
            sys_get_sys_info(&sys_info);
            if(sys_info.cores > 1) {
                for(uint32_t i=0; i<sys_info.cores; i++) {
                    graph_blt_alpha(console->icon, 
                            0, 0, console->icon->w, console->icon->w,
                            console->g,
                            console->g->w - console->icon->w * (i+1),
                            console->g->h - console->icon->h,
                            console->icon->w, console->icon->h, 
                            0xff);
                }	
            }
        }
    }

    gterminal_paint(&console->terminal, console->g, 0, 0, console->g->w, console->g->h);
    display_flush(&console->display, true);
}

static bool _flush = true;
static int _disp_index = 0;
static charbuf_t* _buffer = NULL;   /* bytes pending for read() (declared early:
                                       the line discipline and CPR callback use it) */
static tty_state_t _tty;            /* shared terminal line discipline */
static vdevice_t* _dev = NULL;      /* lets output_callback wake readers */

/* gterminal's replies to terminal queries (e.g. the cursor-position report vi
 * asks for) arrive here and are queued as input for the reader. */
static void console_output_cb(void* p, const char* buf, int size) {
    (void)p;
    for(int i = 0; i < size; i++)
        charbuf_push(_buffer, buf[i], true);
    if(_dev != NULL)
        vfs_wakeup(_dev->mnt_info.node, VFS_EVT_RD);
}

/* Line-discipline emit callbacks: queue bytes for read(), echo to the screen. */
static void console_emit_read(void* arg, const char* buf, int size) {
    (void)arg;
    for(int i = 0; i < size; i++)
        charbuf_push(_buffer, buf[i], true);
}

static void console_emit_echo(void* arg, const char* buf, int size) {
    fb_console_t* console = (fb_console_t*)arg;
    if(console == NULL || size <= 0)
        return;
    _flush = true;
    gterminal_put(&console->terminal, buf, size);
}

static int console_write(vdevice_t* dev,
        int fd,
        int from_pid,
        fsinfo_t* info,
        const void* buf,
        int size,
        int offset,
        void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)offset;

    _flush = true;
    fb_console_t* console = (fb_console_t*)p;
    if(size <= 0 || console->g == NULL)
        return 0;

    const char* pb = (const char*)buf;
    if((_tty.tio.c_oflag & OPOST) == 0) {
        gterminal_put(&console->terminal, pb, size);
    }
    else {
        /* OPOST may expand NL->CRLF, so give tty_output room to grow. */
        char* tmp = (char*)malloc((size_t)size * 2);
        if(tmp != NULL) {
            int m = tty_output(&_tty, pb, size, tmp, size * 2);
            gterminal_put(&console->terminal, tmp, m);
            free(tmp);
        }
        else {
            gterminal_put(&console->terminal, pb, size);
        }
    }
    return size;
}

static int _keyb_fd = -1;
static const char* _keyb_dev = "";

static int console_read(vdevice_t* dev,
        int fd,
        int from_pid,
        fsinfo_t* info,
        void* buf,
        int size,
        int offset,
        void* p) {

    (void)dev;
    tty_set_foreground(&_tty, from_pid);
    char c;
    int res = charbuf_pop(_buffer, &c);

    if(res != 0)
        return VFS_ERR_RETRY;
    ((char*)buf)[0] = c;
    return 1;
}

static uint32_t console_check_poll_events(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)p;

    /* Level-triggered: report readable whenever bytes are pending. Without this
     * poll() only sees the edge from vfs_wakeup(), so a full-screen app that
     * reads one byte at a time (e.g. vi's cursor-position probe) can miss the
     * rest of a multi-byte reply and time out. */
    if(!charbuf_is_empty(_buffer))
        return VFS_EVT_RD;
    return 0;
}

static int console_loop(vdevice_t* dev, void* p) {
    (void)dev;
    static bool shift = false;
    static bool ctrl = false;

    fb_console_t* console = (fb_console_t*)p;

    ipc_disable();
    if(_flush) {
        flush(console);
        _flush = false;
    }

    if(_keyb_fd < 0) {
        if(_keyb_dev[0] != 0)
            _keyb_fd = open(_keyb_dev, O_RDONLY | O_NONBLOCK);
    }

    if(_keyb_fd > 0) {
        keyb_evt_t evts[KEYB_EVT_MAX];
        int n = keyb_read(_keyb_fd, evts, KEYB_EVT_MAX);
        for(int i=0; i<n; i++) {
            keyb_evt_t* ev = &evts[i];
            if(ev->state == KEYB_STATE_PRESS) {
                if(ev->key == KEY_LSHIFT || ev->key == KEY_RSHIFT) {
                    shift = true;
                }
                else if(ev->key == KEY_CTRL) {
                    ctrl = true;
                }
            }
            else {
                if(ev->key == KEY_LSHIFT || ev->key == KEY_RSHIFT) {
                    shift = false;
                }
                else if(ev->key == KEY_CTRL) {
                    ctrl = false;
                }
            }

            uint8_t c;
            if(shift)
                c = keyb_shift_value(ev->key);
            else if(ctrl)
                c = keyb_ctrl_value(ev->key);
            else
                c = ev->key;

            if(ev->state != KEYB_STATE_PRESS || c >= 128)
                continue;

            if(ev->key == KEY_LSHIFT || ev->key == KEY_RSHIFT || ev->key == KEY_CTRL)
                continue;

            if(c == KEY_UP) {
                gterminal_scroll(&console->terminal, -1);
                _flush = true;	
            }
            else if(c == KEY_DOWN) {
                gterminal_scroll(&console->terminal, 1);
                _flush = true;	
            }
            else if(c == KEY_LEFT) {
                if(console->terminal.font_size > 5)
                    console->terminal.font_size--;
                gterminal_resize(&console->terminal, console->g->w, console->g->h);
                _flush = true;	
            }
            else if(c == KEY_RIGHT) {
                if(console->terminal.font_size < 99)
                    console->terminal.font_size++;
                gterminal_resize(&console->terminal, console->g->w, console->g->h);
                _flush = true;	
            }
            else {
                gterminal_scroll(&console->terminal, 0);
                char ch = (char)c;
                tty_input(&_tty, &ch, 1,
                        console_emit_read, NULL,
                        console_emit_echo, console);
            }
        }
        if(n > 0)
            vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
    }

    ipc_enable();
    usleep(30000);
    return 0;
}

static int console_dcntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
    (void)dev;
    (void)from_pid;
    fb_console_t* console = (fb_console_t*)p;
    /* Report the live textgrid geometry so TIOCGWINSZ answers without a probe. */
    if(console != NULL)
        tty_set_winsize(&_tty,
                (unsigned short)console->terminal.rows,
                (unsigned short)console->terminal.cols);
    return tty_dev_cntl(&_tty, cmd, in, ret);
}

static const char* _mnt_point = "";
static int doargs(int argc, char* argv[]) {
    int c = 0;
    while (c != -1) {
        c = getopt (argc, argv, "i:d:u:");
        if(c == -1)
            break;

        switch (c) {
        case 'i':
            _keyb_dev = optarg;
            break;
        case 'd':
            _disp_index = atoi(optarg);
            break;
        default:
            c = -1;
            break;
        }
    }
    return optind;
}

int main(int argc, char** argv) {
    _disp_index = 0;
    _buffer = charbuf_new(0);
    tty_init(&_tty);
    int argind = doargs(argc, argv);

    char mnt_point[128] = {0};
    if(argind < argc)
        strncpy(mnt_point, argv[argind], 127);
    else
        snprintf(mnt_point, 127, "/dev/console0");

    _keyb_fd = -1;
    const char* display_dev = "/dev/displayman";

    fb_console_t _console;
    init_console(&_console, display_dev, _disp_index);
    reset_console(&_console);
    _console.terminal.output_callback = console_output_cb;
    _console.terminal.output_callback_arg = NULL;

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "console");
    dev.write = console_write;
    dev.read = console_read;
    dev.extra_data = &_console;
    dev.loop_step = console_loop;
    dev.check_poll_events = console_check_poll_events;
    dev.dev_cntl = console_dcntl;
    _dev = &dev;

    device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666, false);
    close_console(&_console);
    charbuf_free(_buffer);
    return 0;
}
