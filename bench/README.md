# Benchmark methodology

Run `make -C mfind build`, then `python3 bench/benchmark.py --binary mfind/mfind`. Use `--scale N` for larger corpora. Output is JSON with platform/compiler information and per-run metrics.

Four deterministic ASCII workloads cover 50 small files, a roughly 256 KiB file with boundary-spanning matches, dense overlaps, and 100 distinct patterns. Each runs all three priority modes cold and warm (24 runs). An independent Python literal matcher validates exact (file, byte offset, keyword) tuples before results are accepted. The reference is a correctness oracle, not a performance competitor.

Timing includes process startup, fingerprinting, matching, coordinate generation, serialization and cache I/O. Throughput divides input bytes by end-to-end elapsed time. POSIX wait4 measures peak RSS per process (macOS bytes/Linux KiB normalized). Cold means no Scout index; it does not flush the OS page cache. Warm retains the Scout index but still fingerprints content. Watch latency includes a 50 ms polling interval and output observation overhead.

The checked-in sample was measured on the development Mac at scale 1. Its six watch changes took approximately 52–56 ms, and per-run peak RSS ranged approximately 1.4–5.7 MiB. These tiny workloads are smoke measurements; use larger scales and multiple repetitions for performance decisions. There are no hard timing gates in CI and no claim of superiority to other tools. Full result records are in sample-macos.json.
