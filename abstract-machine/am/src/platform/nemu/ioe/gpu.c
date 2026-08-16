#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)
void __am_gpu_init() {
  int i;
  uint32_t screen_wh = inl(VGACTL_ADDR);
  uint32_t h = screen_wh & 0xffff;
  uint32_t w = screen_wh >> 16;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i ++) fb[i] = 0;
  outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = inw(VGACTL_ADDR+2), .height = inw(VGACTL_ADDR),
    .vmemsz = 0
  };
}
// struct AM_GPU_FBDRAW_T{int x, y; void *pixels; int w, h; bool sync}
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  // 机器用的是小端存储,低位在低地址,高位在高地址
  uint32_t w = inw(VGACTL_ADDR+2);
  for(int i = 0 ; i < ctl->h; ++ i){ // 行
    for(int j = 0; j < ctl->w; ++ j){ // 列
      int x = ctl->x + j;
      int y = ctl->y + i;
      outl(FB_ADDR + (y * w + x) * 4, ((uint32_t*)ctl->pixels)[i * ctl->w + j]);
    }
  }
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
