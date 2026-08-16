#include <common.h>
#include "utils.h"
#define IRINGBUF_SZ 30
char iringbuf[IRINGBUF_SZ][128];
size_t irbuf_tail = -1;
size_t irbuf_head = 0;
void write_iringbuf(char* s){
    irbuf_tail = irbuf_tail + 1;
    if (irbuf_head != 0) irbuf_head = (irbuf_head + 1) % IRINGBUF_SZ;
    if (irbuf_tail >= IRINGBUF_SZ) irbuf_tail = 0, irbuf_head ++;
    strcpy(iringbuf[irbuf_tail], s);
}

void print_iringbuf(){
    if (irbuf_tail == -1) return;
    int sz = irbuf_tail < irbuf_head ? IRINGBUF_SZ : irbuf_tail - irbuf_head + 1;
    for(size_t i = 0; i < sz; ++ i)
    {
        if (i == sz - 1) memcpy(iringbuf[(irbuf_head + i) % IRINGBUF_SZ], "-->", 3);
        Log("%s", iringbuf[(irbuf_head + i) % IRINGBUF_SZ]);
    }
}