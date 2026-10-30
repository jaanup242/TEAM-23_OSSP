# MemTrace – Virtual Memory & TLB Analyzer

C engine (page table, TLB with LRU/FIFO, translation) + Python bridge + browser UI.

```
browser (web/index.html) --HTTP/JSON--> server/server.py --stdin/stdout--> build/memtrace (C)
```

## Run on Ubuntu (or Ubuntu in WSL)
```bash
sudo apt update && sudo apt install -y build-essential python3
make          # builds build/memtrace and build/test_engine
make test     # 10 C unit tests
make run      # starts http://localhost:8000
```
Open http://localhost:8000. Use `PORT=9000 make run` to change the port.

## Layout
- `include/memtrace.h` – shared types and API
- `src/tlb.c`, `src/page_table.c`, `src/translation.c` – the engine
- `src/main.c` – CLI that reads a command script on stdin and prints JSON
- `tests/test_engine.c` – unit tests
- `server/server.py` – serves the UI, validates input, runs the C binary
- `web/index.html` – the interface

## CLI without the web UI
```bash
printf 'pagesize 1024\npages 8\ntlb 2\npolicy LRU\nmap 0 5\nmap 1 2\naccess 100\naccess 200\n' | ./build/memtrace
```
