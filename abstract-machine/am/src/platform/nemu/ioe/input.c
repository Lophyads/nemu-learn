#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t am_scancode = inl(KBD_ADDR); // 编译后变成访问内存的加载指令,nemu解析指令会调用map_read走到具体的device
  kbd->keydown = (am_scancode & KEYDOWN_MASK) > 0;
  kbd->keycode = kbd->keydown ? am_scancode ^ KEYDOWN_MASK : am_scancode;
}
