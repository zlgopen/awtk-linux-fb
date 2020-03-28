#include "tkc/platform.h"
#include "tkc/time_now.h"
#include "fb_info.h"

static void fb_draw_test(fb_info_t* fb, uint8_t value) {
  int i = 0;
  int fb_nr = fb_number(fb);
  uint32_t buff_size = fb_size(fb);

  for(i = 0; i < fb_nr; i++) {
    struct fb_var_screeninfo vi = (fb->var);
    uint8_t* buff = fb->fbmem0 + buff_size * i;
    uint32_t start = time_now_ms();
    uint32_t cost = 0;

    memset(buff, value, buff_size); 
    
    vi.yoffset = i * fb_height(fb); 
    vi.yres_virtual = vi.yres * fb_nr;
    
    if (ioctl(fb->fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
      perror("active fb swap failed");
    }
   
    cost = time_now_ms() - start;
    log_info("i=%d: cost=%u fb=%p yoffset=%u size=%u\n", i, cost, buff, vi.yoffset, buff_size);
  }
}

int main(int argc, char* argv[]) {
  fb_info_t fb;
  const char* fbname = "/dev/fb0";
  platform_prepare();

  if(argc != 2) {
    printf("Usage: %s fbdevice\n", argv[0]);
    return 0;
  }

  fbname = argv[1];
  memset(&fb, 0x00, sizeof(fb));

  if(fb_open(&fb, fbname) == 0) {
    int i = 0;
    for(i = 0; i < 1000; i++) {
      fb_draw_test(&fb, i%255);
    }

    fb_close(&fb);
  }

  return 0;
}

