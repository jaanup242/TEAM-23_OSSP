#include "memtrace.h"

void pt_init(Sim *s) { for (int i = 0; i < MAX_PAGES; i++) s->frame[i] = -1; }

int pt_map(Sim *s, int page, int frame) {
    if (page < 0 || page >= s->np || frame < 0) return -1;
    if (s->frame[page] != -1) return -2;
    for (int i = 0; i < s->np; i++) if (s->frame[i] == frame) return -3;
    s->frame[page] = frame;
    return 0;
}

int pt_lookup(const Sim *s, int page) { return s->frame[page]; }
