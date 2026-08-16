#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static int evt_fd = -1;
static int fbctl = -1;

uint32_t NDL_GetTicks() {
  struct timeval now;
  gettimeofday(&now, NULL);
  return (now.tv_sec * 1000000 + now.tv_usec) / 1000;
}

int NDL_PollEvent(char *buf, int len) {
  // if (evt_fd < 0) evt_fd = open("/dev/evt", O_RDONLY);
  // if (evt_fd < 0) return 0;
  return read(evtdev, buf, len);
}

void NDL_OpenCanvas(int *w, int *h) {
  printf("NDL_OpenCanvas begin\n");
  char buf[64];
  int fb_dispinfo = open("/proc/dispinfo", 0);
  
  read(fb_dispinfo, buf, 64);
  sscanf(buf, "WIDTH: %d\nHEIGHT: %d\n", &screen_w, &screen_h);
  printf("screen_w=%d screen_h=%d\n", screen_w, screen_h);
  // if (getenv("NWM_APP")) {
    fbctl = open("/dev/fbctl", 0);
    fbdev = open("/dev/fb", 0);
    if (!(*w))
    {
       *w = screen_w; 
    }
    if (!(*h)){
      *h = screen_h;
    }
    
    int len = sprintf(buf, "%d %d", *w, *h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    memset(buf, 0, sizeof(buf));
    int nlen = 0;
    while (1) {
      // 3 = evtdev
      // printf("wait read evtdev=%d\n", evtdev);
      // int nread = read(evtdev, buf + offset, sizeof(buf) - offset - 1);
      // if (nread <= 0) continue;
      // // offset += nread;
      // buf[offset + nread] = '\0';
      // printf("get read buff=%s\n", buf);
      // if (strcmp(buf, "SPACE") == 0) break;
      if (nlen = NDL_PollEvent(buf, sizeof(buf))){
        buf[nlen] = '\0';
        printf("get read buff=%s\n", buf + 3);
        if (buf[1] == 'd')
        {
          if(strcmp(buf + 3, "SPACE\n") == 0) break;
        }
      }
    }
    close(fbctl);
    printf("NDL_OpenCanvas done\n");
  // }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
    // int bpp = sizeof(uint32_t);          // bytes per pixel
    for (int i = 0; i < h; ++i) {
        // 字节偏移 = (行号 * 屏幕宽度 + 列号) * 每像素字节数
        int offset = ((y + i) * screen_w + x);
        lseek(fbdev, offset, SEEK_SET);
        write(fbdev, pixels + i * w, w);
    }
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  // if (getenv("NWM_APP")) {
  // evtdev = fileno(stdin);
  // }
  evtdev = open("/dev/events", O_RDONLY);
  return 0;
}

void NDL_Quit() {
}
