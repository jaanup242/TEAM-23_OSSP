#include <string.h>
#include "memtrace.h"

void sim_init(Sim *s, long long ps, int np, int cap, int lru) {
    memset(s, 0, sizeof *s);
    s->ps = ps; s->np = np;
    pt_init(s); tlb_init(&s->tlb, cap, lru);
}

Step sim_translate(Sim *s, long long va) {
    Step r = { va, -1, -1, INVALID, -1, -1, -1 };
    if (va < 0 || va >= s->ps * s->np) { s->invalid++; return r; }
    r.page = (int)(va / s->ps);
    r.offset = va % s->ps;
    int f = tlb_lookup(&s->tlb, r.page);
    if (f >= 0) { r.res = HIT; s->hits++; }
    else if ((f = pt_lookup(s, r.page)) >= 0) {     /* TLB miss, valid PTE */
        r.res = MISS; s->misses++;
        r.evicted = tlb_insert(&s->tlb, r.page, f);
    } else { r.res = FAULT; s->faults++; return r; } /* never cache unmapped pages */
    r.frame = f;
    r.pa = (long long)f * s->ps + r.offset;
    return r;
}

const char *result_name(Result r) {
    static const char *n[] = { "HIT", "MISS", "FAULT", "INVALID" };
    return n[r];
}
