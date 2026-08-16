#include <common.h>
#include <sys/time.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif
extern int fg_cb;
#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

static char evt_buff[64];
static char fbctl_buff[64]; // fame buff 存放画布大小
static int screen_w = 800;
static int screen_h = 600;
size_t serial_write(const void *buf, size_t offset, size_t len) {
  // yield();
  for (int i = 0; i < len; ++ i){
    putch(*(char*)buf);
    buf ++;
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  // yield();
  AM_INPUT_KEYBRD_T event = io_read(AM_INPUT_KEYBRD);
  char* data = buf;
  if(event.keycode == AM_KEY_NONE) {
    return 0;
  }
  int temp_len = strlen(keyname[event.keycode]);
  if(4 + temp_len > len) {
    return 0;
  }
  if(event.keydown) {
    data[0] = 'k';
    data[1] = 'd';
    data[2] = ' ';

    if (event.keycode >= 2 && event.keycode <= 4){
      fg_cb = event.keycode - 1; // 更换前台进程号，下一次硬件中断到来会切换到该前台进程
    }
  } else {
    data[0] = 'k';
    data[1] = 'u';
    data[2] = ' ';
  }
  memcpy(data + 3, keyname[event.keycode], temp_len);
  data[3 + temp_len] = '\n';
  return 4 + temp_len;

  // AM_INPUT_KEYBRD_T event = io_read(AM_INPUT_KEYBRD);
  // char* data = buf;
  // if(event.keycode == AM_KEY_NONE || !event.keydown) {
  //   return 0;
  // }
  // memcpy(data, keyname[event.keycode], strlen(keyname[event.keycode]));
  // // printf("event read=%s\n", data);
  // return strlen(keyname[event.keycode]);
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T info = io_read(AM_GPU_CONFIG);
  char* data = buf;
  screen_w = info.width;
  screen_h = info.height;
  // printf("dispinfo_read WIDTH: %d\nHEIGHT: %d\n\n", screen_w, screen_h);
  sprintf(data, "WIDTH: %d\nHEIGHT: %d\n", info.width, info.height);
  return sizeof(data);
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  // yield();
  // int x, y; void *pixels; int w, h; bool sync
  // printf("fb_write offset=%d screen_w=%d screen_h=%d len=%d\n", offset, offset % screen_w, offset / screen_w, len);
  io_write(AM_GPU_FBDRAW, offset % screen_w, offset / screen_w, (void*)buf, len, 1, 1);
  return 0;
}

size_t timer_read(void *buf, size_t offset, size_t len) {
  uint64_t us = io_read(AM_TIMER_UPTIME).us;
  struct timeval *tv = (struct timeval*)buf;
  tv->tv_sec = us / 1000000;
  tv->tv_usec = us % 1000000;
  return 0;
}

// size_t evt_read(void *buf, size_t offset, size_t len) {
//   if (len > 64){
//     Log("evt_read ERROR len=%d", len);
//     return 0;
//   }
//   char* data = buf;
//   memcpy(data, evt_buff, len);
//   // data[len] = '\0';
//   return len;
// }

// size_t evt_write(const void *buf, size_t offset, size_t len) {
//   if (len > 64){
//     Log("evt_write ERROR len=%d", len);
//     return 0;
//   }
//   *evt_buff = *(char*)buf;
//   return len;
// }

size_t fbctl_write(const void *buf, size_t offset, size_t len) {
  *fbctl_buff = *(char*)buf;
  sprintf(evt_buff, "mmap ok\n"); // 直接往事件文件写画布申请成功。。。
  return len;
}


void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
