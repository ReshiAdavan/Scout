# Review guide

Review PRs #9–#21 in ascending order. Each has one commit relative to its named base, and each later PR depends on its predecessor. No PR has been merged. Retarget dependent PRs as their bases are merged; do not merge a later branch directly into main without reviewing its accumulated dependencies.

## Local verification completed

- `make -C mfind test`: 24 C++ unit tests and nine Python integration scripts.
- Differential integration: 48 seeded combinations of case sensitivity, word boundaries and priority modes against an independent literal-search oracle, plus retained-index eviction checks.
- Cache/watch integration: cold/warm equivalence, cache hits, truncated-snapshot recovery, unchanged-round silence, deletion events and interrupt shutdown.
- Output integration: independently parsed JSON/CSV, quoting, comma paths, CRLF, UTF-8, profile identities, Boolean predicates and consistent statistics.
- Range integration: cold/warm UTF-16 coordinates with astral characters, CRLF/lone CR and encoded file URIs.
- ThreadSanitizer: C++ suite and watch/cache/matching integration scripts passed.
- AddressSanitizer/UndefinedBehaviorSanitizer: differential, watch/cache and range integration scripts passed.
- Benchmark: 24 cold/warm/mode/workload combinations checked against exact reference results; sample committed under bench/.
- `git diff --check`: passed.

Sanitizer builds used Clang C++17, `-pthread -O1 -g`, and either `-fsanitize=thread` or `-fsanitize=address,undefined`. CLI builds compile mfind.cpp, core/fileCache.cpp and helpers/ignoreRules.cpp from mfind/ with `-I. -Icore -Ihelpers -Iqueues -Iwalkers -Iworkers`; pass the resulting executable to the Python scripts. Unit builds use the test sources listed in the Makefile. Sanitizer findings are platform/workload-specific evidence, not a proof of race or memory safety for every input.

## Decisions to review

1. One-shot search is the new default; --watch restores long-running behavior.
2. Matching/word semantics are explicitly ASCII byte-oriented. LSP output uses UTF-16 positions but is not a protocol server.
3. Longest-per-start semantics replace the ambiguous same-start/same-end wording in the original local proposal. Canonical results stay reusable across priority modes.
4. Nested .gitignore support excludes global/info excludes; .scoutignore is an additional exclusion layer. Symlinks are skipped.
5. Cache fingerprints use FNV-1a, and content is reread for warm verification. Snapshots are opt-in, versioned and atomically replaced; no power-loss durability guarantee.
6. Index retention is estimated payload, not total memory. Files are currently read into memory for coordinates and complete results are retained for output/watch comparison.
7. Invalid UTF-8 bytes are represented as U+00XX in JSON; LSP endpoints must be valid UTF-8 boundaries.
8. Profiles use a small versioned quoted-field format, with explicit CLI precedence. Repeated labels for one literal count as separate labeled occurrences.

The pre-existing untracked test harness and required <fstream> include were incorporated as the foundation in #9. The existing local CI draft was expanded in #21. CLAUDE.md, the proposed ADR and hook scripts remain outside the commits. The final branch removes the tracked generated binary after a combined check demonstrated that it could run stale code.

## CI-discovered watch race

A later macOS CI run exposed deletion between directory discovery and scanning. Watch mode now retries invalidated filesystem snapshots and preserves the last published result set until a complete round succeeds. Snapshot bounds are checked before coordinate/word access. A regression test performs 100 create/delete/write cycles and verifies that the watcher survives and emits the final removal. One-shot scans still report concurrent modifications as errors.
