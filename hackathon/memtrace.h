#ifndef MEMTRACE_H
#define MEMTRACE_H
#define MAX_PAGES 1024
#define MAX_TLB 64

typedef enum { HIT, MISS, FAULT, INVALID } Result;
typedef struct { int page, frame; } TlbEntry;
/* e[0] is the next victim: least recently used (LRU) or oldest (FIFO). */
typedef struct { TlbEntry e[MAX_TLB]; int n, cap, lru; } Tlb;
typedef struct {
    long long ps; int np;
    int frame[MAX_PAGES];            /* page table: frame number, or -1 if unmapped */
    Tlb tlb;
    long hits, misses, faults, invalid;
} Sim;
typedef struct { long long va; int page; long long offset; Result res; int frame; long long pa; int evicted; } Step;

void tlb_init(Tlb *t, int cap, int lru);
int  tlb_lookup(Tlb *t, int page);               /* frame or -1 */
int  tlb_insert(Tlb *t, int page, int frame);    /* evicted page or -1 */
void pt_init(Sim *s);
int  pt_map(Sim *s, int page, int frame);        /* 0 ok, -1 range, -2 page dup, -3 frame dup */
int  pt_lookup(const Sim *s, int page);          /* frame or -1 */
void sim_init(Sim *s, long long ps, int np, int cap, int lru);
Step sim_translate(Sim *s, long long va);
const char *result_name(Result r);
#endif
