#include <stdio.h>
#include "memtrace.h"
static int fails = 0;
#define CHECK(name, cond) do { if (cond) printf("PASS  %s\n", name); else { printf("FAIL  %s\n", name); fails++; } } while (0)

static void mk(Sim *s, int cap, int lru) {
    sim_init(s, 1024, 8, cap, lru);
    pt_map(s, 0, 5); pt_map(s, 1, 2); pt_map(s, 2, 7); pt_map(s, 3, 1);
}
static void run(Sim *s, const long long *a, int n, Result *out) { for (int i = 0; i < n; i++) out[i] = sim_translate(s, a[i]).res; }

int main(void) {
    Sim s; Step r; Result o[8];
    mk(&s, 3, 1); r = sim_translate(&s, 2500);
    CHECK("2500 -> page 2, offset 452, physical 7620", r.page == 2 && r.offset == 452 && r.pa == 7620);

    mk(&s, 3, 1); long long a1[] = {100, 200}; run(&s, a1, 2, o);
    CHECK("same page twice: MISS then HIT", o[0] == MISS && o[1] == HIT);

    mk(&s, 3, 1); long long a2[] = {100, 200, 1100, 100, 2100}; run(&s, a2, 5, o);
    CHECK("guide sequence: 2 hits, 3 misses", s.hits == 2 && s.misses == 3);

    mk(&s, 3, 1);
    CHECK("negative address rejected", sim_translate(&s, -1).res == INVALID);
    CHECK("address beyond virtual space rejected", sim_translate(&s, 8 * 1024).res == INVALID);

    mk(&s, 3, 1);
    CHECK("unmapped page faults and is not cached", sim_translate(&s, 5000).res == FAULT && s.tlb.n == 0 && sim_translate(&s, 5000).res == FAULT);

    sim_init(&s, 1024, 8, 2, 1); pt_map(&s, 0, 5); pt_map(&s, 1, 2); pt_map(&s, 2, 7);
    long long a3[] = {0, 1024, 0, 2048, 1024}; run(&s, a3, 5, o);
    CHECK("LRU cap 2, pages 0,1,0,2,1: M M H M M", o[0]==MISS && o[1]==MISS && o[2]==HIT && o[3]==MISS && o[4]==MISS);

    sim_init(&s, 1024, 8, 2, 1); pt_map(&s, 0, 5); pt_map(&s, 1, 2); pt_map(&s, 2, 7);
    long long a4[] = {0, 1024, 0, 2048}; run(&s, a4, 4, o);
    CHECK("LRU evicted page 1, TLB holds 0 and 2", s.tlb.n == 2 && s.tlb.e[0].page == 0 && s.tlb.e[1].page == 2);

    sim_init(&s, 1024, 8, 2, 0); pt_map(&s, 0, 5); pt_map(&s, 1, 2); pt_map(&s, 2, 7);
    long long a5[] = {0, 1024, 0, 2048, 0}; run(&s, a5, 5, o);
    CHECK("FIFO evicts oldest page 0 even after a hit", o[2] == HIT && o[4] == MISS);

    sim_init(&s, 1024, 4, 2, 1); pt_map(&s, 0, 3);
    CHECK("duplicate page / duplicate frame rejected", pt_map(&s, 0, 4) == -2 && pt_map(&s, 1, 3) == -3);

    printf("%s\n", fails ? "SOME TESTS FAILED" : "ALL TESTS PASSED");
    return fails != 0;
}
