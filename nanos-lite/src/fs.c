#include <fs.h>
#include <klib.h>
typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset;
} Finfo;


size_t ramdisk_read(void *buf, size_t offset, size_t len);

size_t ramdisk_write(const void *buf, size_t offset, size_t len);

static inline size_t min(size_t a, size_t b) {
    return a < b ? a : b;
}


size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t serial_write(const void *buf, size_t offset, size_t len);

size_t timer_read(void *buf, size_t offset, size_t len);

size_t events_read(void *buf, size_t offset, size_t len);

// size_t evt_read(void *buf, size_t offset, size_t len);

// size_t evt_write(const void *buf, size_t offset, size_t len);

size_t fbctl_write(const void *buf, size_t offset, size_t len);

size_t dispinfo_read(void *buf, size_t offset, size_t len);

size_t fb_write(const void *buf, size_t offset, size_t len);

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
  [FD_FBCTL] = {"/dev/fbctl", 0, 0, invalid_read, fbctl_write},
  [FD_FB] = {"/dev/fb", 0, 0, invalid_read, fb_write},
  [FD_TIMER] = {"timer", 0, 0, timer_read, invalid_write},
  [FD_KEYBOARD] = {"/dev/events", 0, 0, events_read, invalid_write},
  [FD_DISPINFO] = {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write},
#include "files.h"
};


int fs_open(const char *pathname, int flags, int mode){
  int file_table_n = (sizeof(file_table) / sizeof(Finfo));
  for (int i = 0; i < file_table_n; i++) {
    if (strcmp(file_table[i].name, pathname) == 0) {
      file_table[i].open_offset = 0;
      return i;
    }
  }
  // printf("file_table_n=%d,pathname=%s\n",file_table_n, pathname);
  panic("should not reach here");
}

size_t fs_read(int fd, void *buf, size_t len){
  Finfo* file = &file_table[fd];
  // Log("%d %d %d", len, file->size, file->open_offset);
  // assert(len <= file->size - file->open_offset);
  int ret = 0;
  if (!file->read) {
    len = min(len, file->size - file->open_offset);
    ramdisk_read(buf, file->disk_offset + file->open_offset, len);
    ret = len;
  }
  else {
    ret = file->read(buf, file->disk_offset + file->open_offset, len);
  }
  fs_lseek(fd, ret, SEEK_CUR);
  return ret;
}
size_t fs_write(int fd, const void *buf, size_t len){
  Finfo* file = &file_table[fd];
  // Log("%d %d %d", len, file->size, file->open_offset);
  int ret = 0;
  if (!file->write) {
    assert(len <= file->size - file->open_offset);
    ramdisk_write(buf, file->disk_offset + file->open_offset, len);
    ret = 0;
  }
  else {
    ret = file->write(buf, file->disk_offset + file->open_offset, len);
  }
  fs_lseek(fd, ret, SEEK_CUR);
  return ret;
}
size_t fs_lseek(int fd, size_t offset, int whence){
  Finfo* file = &file_table[fd];
  switch (whence) {
      case SEEK_SET: file->open_offset = offset; break;
      case SEEK_CUR: file->open_offset += offset; break;
      case SEEK_END: file->open_offset = file->size + offset; break;
      default: {
        panic("should not reach here");  // 错误
        return -1;
      }
  }
  file->open_offset = min(file->open_offset, file->size);
  return file->open_offset;  // 返回新的偏移
}
int fs_close(int fd){
  return 0;
}

void init_fs() {
  // TODO: initialize the size of /dev/fb
  int fd = fs_open("/proc/dispinfo", 0, 0);
  char buf[64];
  fs_read(fd, buf, 64);
  int screen_w;
  int screen_h;
  // printf("buf : %s\n", buf);
  sscanf(buf, "WIDTH: %d\nHEIGHT: %d\n", &screen_w, &screen_h);
  printf("init_fs WIDTH : %d HEIGHT : %d\n", screen_w, screen_h);
  file_table[FD_FB].size = screen_w * screen_h;
}
