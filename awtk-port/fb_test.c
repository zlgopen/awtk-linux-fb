#include "tkc/platform.h"
#include "tkc/time_now.h"
#include "tkc/utils.h"
#include "fb_info.h"
#include <signal.h>

static int s_ttyfd = -1;
static bool_t s_quited = FALSE;
static uint32_t s_buff_index = 0;

static void fb_draw_test(fb_info_t* fb, color_t color) {

  uint32_t cost = 0;
  int fb_nr = fb_number(fb);
  uint32_t fb_bpp = fb_bpp(fb);
  uint32_t start = time_now_ms();
  uint32_t buff_size = fb_size(fb);
  struct fb_var_screeninfo vi = (fb->var);
  uint32_t size = fb->var.xres_virtual * fb_height(fb);
  uint8_t* buff = fb->fbmem0 + buff_size * s_buff_index;

  if (fb_bpp == 16) {
    uint16_t c = ((color.rgba.r >> 3) << 11) | ((color.rgba.g >> 2) << 5) | (color.rgba.b >> 3);
    tk_memset16((uint16_t*)buff, c, size);
  } else if (fb_bpp == 32) {
    uint32_t c = ((color.rgba.r << 24) | (color.rgba.g << 16) | (color.rgba.b << 8) | 0xff);
    tk_memset32((uint32_t*)buff, c, size);
  } else if (fb_bpp == 24) {
    int32_t i = 0;
    for (i = 0; i < size; i++) {
      *buff++ = color.rgba.r;
      *buff++ = color.rgba.g;
      *buff++ = color.rgba.b;
    }
  } else {
    log_error(" fb_test don't supported \r\n");
  }

  if (fb_nr > 1) {
    vi.yoffset = s_buff_index * fb_height(fb);
    vi.yres_virtual = vi.yres * fb_nr;

    if (ioctl(fb->fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
      perror("active fb swap failed");
    }
    cost = time_now_ms() - start;
    log_info("i=%d: cost=%u fb=%p yoffset=%u size=%u\n", s_buff_index, cost, buff, vi.yoffset, buff_size);
    
    s_buff_index++;
    if (s_buff_index >= fb_nr) {
      s_buff_index = 0;
    }
  } else {
    fb_sync(fb);
  }
}

static void on_fb_test_exit(void) {
  s_quited = TRUE;
  sleep_ms(10);
  if (s_ttyfd >= 0) {
    ioctl(s_ttyfd, KDSETMODE, KD_TEXT);
  }
  log_info("on_fb_test_exit \r\n");
}

static void on_signal_int(int sig) {
  on_fb_test_exit();
}

int main(int argc, char* argv[]) {
  fb_info_t fb;
  const char* fbname = "/dev/fb0";
  color_t color = color_init(0xff, 0, 0, 0xff);

  platform_prepare();

  atexit(on_fb_test_exit);
  signal(SIGINT, on_signal_int);

  if (argc != 2) {
    log_info("Usage: %s fbdevice\n", argv[0]);
    return 0;
  }

  fbname = argv[1];
  memset(&fb, 0x00, sizeof(fb));

  if (fb_open(&fb, fbname) == 0) {
    int32_t i = 0;
    s_ttyfd = open("/dev/tty1", O_RDWR);
    if (s_ttyfd >= 0) {
      ioctl(s_ttyfd, KDSETMODE, KD_GRAPHICS);
    }

    // fix FBIOPUT_VSCREENINFO block issue when run in vmware double fb mode
    if (check_if_run_in_vmware() && fb_number((&fb)) > 1) {
      log_info("run in vmware and fix FBIOPUT_VSCREENINFO block issue\n");
      fb.var.activate = FB_ACTIVATE_INV_MODE;
      fb.var.pixclock = 60;
    }

    for (i = 0; i < 0xff * 2; i++) {
      if (s_quited) {
        break;
      }
      fb_draw_test(&fb, color);

      if (i > 0xff) {
        uint32_t c = (i - 0xff);
        color.color = (c << 16) | ((0xff - c) << 8) | 0xff0000ff;
      } else {
        color.color = (i << 8) | 0xff0000ff;
      }

      log_info("i:%d, color:(%d, %d, %d, %d) \r\n", i, color.rgba.r, color.rgba.g, color.rgba.b, color.rgba.a);
      sleep_ms(100);
    }
    log_info(" fb_test finished and quited \r\n");
    fb_close(&fb);
  }

  return 0;
}
