#include <NDL.h>
#include <SDL.h>
#include <string.h>
#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

#define keyname_sz (sizeof(keyname) /  sizeof(char*))

static uint8_t keystate[keyname_sz];

int pool_event_handler(SDL_Event *ev){
  char buf[64];
  int nlen = 0;
  if(nlen = NDL_PollEvent(buf, sizeof(buf)))
  {
    buf[nlen] = '\0';
    // printf("pool_event_handler buf=%s\n", buf);
    if (buf[1] == 'd')
    {
      ev->type = SDL_KEYDOWN;
    }
    else {
      ev->type = SDL_KEYUP;
    }
    buf[nlen - 1] = '\0';
    for(int i = 0; i < keyname_sz; ++ i){
      // printf("keyname[i]=%s\n", keyname[i]);
      if (strcmp(buf + 3, keyname[i]) == 0) {
        ev->key.keysym.sym = i; 
        keystate[i] = ev->type;
        return 1;
      }
    }
    ev->type = SDL_USEREVENT;
  }
  return 0;
}
int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
    return pool_event_handler(ev);
}

int SDL_WaitEvent(SDL_Event *event) {
  while(pool_event_handler(event) != 1)
  {
  }
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return &keystate;
}