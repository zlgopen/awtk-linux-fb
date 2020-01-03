
#include "fb_info.h"

int main(int argc, char* argv[]) {
  fb_info_t fb;
  const char* fbname = "/dev/fb0";

  if(argc != 2) {
    printf("Usage: %s fbdevice\n", argv[0]);
    return 0;
  }

  fbname = argv[1];
  memset(&fb, 0x00, sizeof(fb));

  if(fb_open(&fb, fbname) == 0) {

    fb_close(&fb);
  }

  return 0;
}

