#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while(s[len] !='\0') ++ len;
  return len;
  // panic("Not implemented");
}

char *strcpy(char *dst, const char *src) {
  size_t i;
  for(i = 0; src[i] != '\0'; i ++)
    dst[i] = src[i];
  dst[i] = '\0';
  return dst;
  // panic("Not implemented");
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dst[i] = src[i];
    for ( ; i < n; i++)
        dst[i] = '\0';
    return dst;
  // panic("Not implemented");
}

char *strcat(char *dst, const char *src) {
   size_t dest_len = strlen(dst);
    size_t i;
    for (i = 0 ; src[i] != '\0' ; i++)
        dst[dest_len + i] = src[i];
    dst[dest_len + i] = '\0';
    return dst;
  // panic("Not implemented");
}

int strcmp(const char *s1, const char *s2) {
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);
  size_t i;
  for(i = 0; i <= len1 && i <= len2; i ++){
    if(s1[i] != s2[i]) return s1[i] - s2[i];
  }
  return 0;
  // panic("Not implemented");
}

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);
  size_t i;
  for(i = 0; i < n && i < len1 && i < len2; i ++){
    if(s1[i] != s2[i]) return s1[i] < s2[i];
  }
  return (len1 == len2) ? 0 : len1 < len2;
  // panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void *memmove(void *dst, const void *src, size_t n) {
   unsigned char *d = dst;
    const unsigned char *s = src;
    if (d < s) {
        // 从前往后复制
        while (n--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        // 从后往前复制
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *d = out;
    const unsigned char *s = in;
    while (n--) {
        *d++ = *s++;
    }
    return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = s1, *p2 = s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

#endif
