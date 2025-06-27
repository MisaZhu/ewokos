#include <string.h>
#include <arch/vpb/framebuffer.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>
#include <sysinfo.h>

static fbinfo_t _fb_info;
int32_t vpb_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	memset(&_fb_info, 0, sizeof(fbinfo_t));

	if(mmio_map() == 0)
		return -1;
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);

  memset(&_fb_info, 0, sizeof(fbinfo_t));
	dep = 32;
  _fb_info.width = w;
  _fb_info.height = h;
  _fb_info.vwidth = w;
  _fb_info.vheight = h;
  _fb_info.depth = dep;
  _fb_info.pitch = 0;
  _fb_info.xoffset = 0;
  _fb_info.yoffset = 0;
  _fb_info.pointer = sysinfo.gpu.v_base;

  if(w == 640 && h == 480) {
    put32((_mmio_base | 0x1c), 0x2c77);
    put32((_mmio_base | 0x00120000), 0x3f1f3f9c);
    put32((_mmio_base | 0x00120004), 0x090b61df);
    put32((_mmio_base | 0x00120008), 0x067f1800);
  }
  else if(w == 800 && h == 600) {
    put32((_mmio_base | 0x1c), 0x2cac);
    put32((_mmio_base | 0x00120000), 0x1313a4c4);
    put32((_mmio_base | 0x00120004), 0x0505f6f7);
    put32((_mmio_base | 0x00120008), 0x071f1800);
  }
  else {
    //1024x768
    w = 1024;
    h = 768;
    _fb_info.width = w;
    _fb_info.height = h;
    _fb_info.vwidth = w;
    _fb_info.vheight = h;
    put32((_mmio_base | 0x00120000), 0x3F << 2);
    put32((_mmio_base | 0x00120004), 767);
  }
  put32((_mmio_base | 0x00120010), _fb_info.pointer - sysinfo.kernel_base);
  put32((_mmio_base | 0x00120018), 0x092b);

  _fb_info.size = _fb_info.width * _fb_info.height * (_fb_info.depth/8);
	_fb_info.size_max = 4*1024*1024;
  syscall3(SYS_MEM_MAP, (ewokos_addr_t)_fb_info.pointer, (ewokos_addr_t)_fb_info.pointer-sysinfo.kernel_base, (ewokos_addr_t)_fb_info.size_max);
  return 0;
}

fbinfo_t* vpb_get_fbinfo(void) {
	return &_fb_info;
}
