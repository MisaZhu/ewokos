#include <ewoksys/session.h>
#include <ewoksys/ipc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


static inline int get_splashd_pid(void) {
	return ipc_serv_get("sys.splashd");
}

static int splash(uint32_t persantage, const char* msg, const char* img_fname) {
	int pid = get_splashd_pid();
	if(pid < 0)
		return -1;

	proto_t in;
	PF->format(&in, "i,s,s", persantage, msg, img_fname);

	int res = ipc_call_wait(pid, 0, &in);
	PF->clear(&in);
	return res;
}

static uint32_t _persantage = 0;
static const char* _img_fname = "";
static const char* _msg = "";

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "p:m:i:");
		if(c == -1)
			break;

		switch (c) {
		case 'm':
			_msg = optarg;
			break;
		case 'p':
			_persantage = atoi(optarg);
			break;
		case 'i':
			_img_fname = optarg;
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
    _persantage = 0;
    _img_fname = "";
    _msg = "";

	doargs(argc, argv);
	splash(_persantage, _msg, _img_fname);
    return 0;
}