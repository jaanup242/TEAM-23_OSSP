/* memtrace CLI: reads a simple command script on stdin, prints JSON on stdout. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "memtrace.h"
#define MAX_ACC 5000

static int fail(const char *m) { printf("{\"error\":\"%s\"}\n", m); return 1; }
static int num(const char *s, long long *o) {
    char *e;
    errno = 0;
    *o = strtoll(s, &e, 10);
    return *s && e != s && !*e && errno != ERANGE;
}

int main(void) {
    char line[160], cmd[16], a[48], b[48];
    long long ps = 0, np = 0, cap = 0, v, w, acc[MAX_ACC], mp[MAX_PAGES][2];
    int lru = -1, nacc = 0, nmap = 0;
    while (fgets(line, sizeof line, stdin)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] != '\n' && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            return fail("command line too long");
        }
        int n = sscanf(line, "%15s %47s %47s", cmd, a, b);
        if (n < 1) continue;
        if (n >= 2 && !strcmp(cmd, "pagesize")) { if (!num(a, &ps)) return fail("bad pagesize"); }
        else if (n >= 2 && !strcmp(cmd, "pages")) { if (!num(a, &np)) return fail("bad pages"); }
        else if (n >= 2 && !strcmp(cmd, "tlb")) { if (!num(a, &cap)) return fail("bad tlb"); }
        else if (n >= 2 && !strcmp(cmd, "policy")) lru = !strcmp(a, "LRU") ? 1 : !strcmp(a, "FIFO") ? 0 : -1;
        else if (n >= 3 && !strcmp(cmd, "map")) {
            if (!num(a, &v) || !num(b, &w) || nmap >= MAX_PAGES) return fail("bad map");
            if (v < 0 || v >= MAX_PAGES || w < 0 || w > INT_MAX) return fail("page or frame out of range");
            mp[nmap][0] = v; mp[nmap][1] = w; nmap++;
        } else if (n >= 2 && !strcmp(cmd, "access")) {
            if (!num(a, &v) || nacc >= MAX_ACC) return fail("bad access");
            acc[nacc++] = v;
        } else return fail("unknown command");
    }
    if (ps < 2 || ps > 65536 || (ps & (ps - 1))) return fail("page size must be a power of two from 2 to 65536");
    if (np < 1 || np > MAX_PAGES) return fail("pages must be 1 to 1024");
    if (cap < 1 || cap > MAX_TLB) return fail("TLB entries must be 1 to 64");
    if (lru < 0) return fail("policy must be LRU or FIFO");
    if (nacc < 1) return fail("no addresses given");

    Sim s; sim_init(&s, ps, (int)np, (int)cap, lru);
    for (int i = 0; i < nmap; i++) {
        int rc = pt_map(&s, (int)mp[i][0], (int)mp[i][1]);
        if (rc == -1) return fail("page or frame out of range");
        if (rc == -2) return fail("a page is mapped twice");
        if (rc == -3) return fail("a frame is used by two pages");
    }
    printf("{\"config\":{\"pageSize\":%lld,\"pages\":%lld,\"tlb\":%lld,\"policy\":\"%s\"},\"steps\":[", ps, np, cap, lru ? "LRU" : "FIFO");
    for (int i = 0; i < nacc; i++) {
        Step r = sim_translate(&s, acc[i]);
        printf("%s{\"va\":%lld,\"page\":%d,\"offset\":%lld,\"result\":\"%s\",\"frame\":%d,\"pa\":%lld,\"evicted\":%d,\"tlb\":[",
               i ? "," : "", r.va, r.page, r.offset, result_name(r.res), r.frame, r.pa, r.evicted);
        for (int j = 0; j < s.tlb.n; j++) printf("%s[%d,%d]", j ? "," : "", s.tlb.e[j].page, s.tlb.e[j].frame);
        printf("]}");
    }
    printf("],\"stats\":{\"hits\":%ld,\"misses\":%ld,\"faults\":%ld,\"invalid\":%ld}}\n", s.hits, s.misses, s.faults, s.invalid);
    return 0;
}
