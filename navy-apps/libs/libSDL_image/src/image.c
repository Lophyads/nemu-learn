#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

SDL_Surface* IMG_Load(const char *filename) {
  // 1. 打开文件
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "IMG_Load: Cannot open file %s\n", filename);
        return NULL;
    }

    // 2. 获取文件大小
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 3. 分配内存并读取整个文件
    unsigned char *buf = (unsigned char*)malloc(fsize);
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    fread(buf, 1, fsize, fp);
    fclose(fp);

    // 4. 交给 STBIMG_LoadFromMemory 解码
    SDL_Surface *surface = STBIMG_LoadFromMemory(buf, fsize);
    free(buf);  // 文件数据已解码，可以释放

    if (!surface) {
        fprintf(stderr, "IMG_Load: Failed to decode image %s\n", filename);
    }

    return surface;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}
