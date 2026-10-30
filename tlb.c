#include <string.h>
#include "memtrace.h"

void tlb_init(Tlb *t, int cap, int lru) { t->n = 0; t->cap = cap; t->lru = lru; }

int tlb_lookup(Tlb *t, int page) {
    for (int i = 0; i < t->n; i++) {
        if (t->e[i].page != page) continue;
        int f = t->e[i].frame;
        if (t->lru) {                              /* move to most-recent end */
            TlbEntry x = t->e[i];
            memmove(&t->e[i], &t->e[i + 1], (size_t)(t->n - i - 1) * sizeof x);
            t->e[t->n - 1] = x;
        }
        return f;
    }
    return -1;
}

int tlb_insert(Tlb *t, int page, int frame) {
    int ev = -1;
    if (t->n >= t->cap) {                          /* full: evict front entry */
        ev = t->e[0].page;
        memmove(&t->e[0], &t->e[1], (size_t)(t->n - 1) * sizeof(TlbEntry));
        t->n--;
    }
    t->e[t->n].page = page; t->e[t->n].frame = frame; t->n++;
    return ev;
}
