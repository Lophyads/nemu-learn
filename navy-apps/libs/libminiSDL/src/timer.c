#include <NDL.h>
#include <sdl-timer.h>
#include <stdio.h>
static uint32_t boot_time = 0;
SDL_TimerID SDL_AddTimer(uint32_t interval, SDL_NewTimerCallback callback, void *param) {
  return NULL;
}

int SDL_RemoveTimer(SDL_TimerID id) {
  return 1;
}

uint32_t SDL_GetTicks() {
  uint32_t now = NDL_GetTicks();
  if(boot_time == 0) boot_time = now;
  return now - boot_time;
}

void SDL_Delay(uint32_t ms) {
}
