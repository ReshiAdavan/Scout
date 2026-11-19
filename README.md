# Scout

A C++17 literal multi-keyword search CLI using Aho–Corasick, parallel scanning, content fingerprints and reusable per-file match indexes. Supports one-shot searches, watch deltas, named search profiles, structured exports and editor coordinates.

## Build and test

Requires a POSIX environment (tested locally on macOS), Clang with C++17, make, and Python 3 for integration tests/benchmarks.

```sh
cd mfind
make build
make test
./mfind --help
```

Generated executables are built locally. `make test` runs the C++ unit suite and all Python integration tests.

## Search

Put one nonempty literal per line in `keywords.txt`; CRLF files are accepted and duplicate literals are ignored. Paths below are relative to the current directory unless a profile supplies its root.

```sh
./mfind /path/to/project --keywords keywords.txt
./mfind /path/to/project --ignore-case --whole-word --extension cpp
./mfind /path/to/project --format json --stats
./mfind /path/to/project --format csv
./mfind /path/to/project --format editor
./mfind /path/to/project --format lsp
./mfind /path/to/project --watch --poll-ms 250 --format jsonl
./mfind /path/to/project --cache /path/to/project/.cache/scout.index --index-mb 64
./mfind /path/to/project --require-all malloc --require-all throw
```

One-shot search is now the default; existing daemon users should add `--watch`. Exit codes: **0** matches, **1** no matches, **2** error. Watch exits 0 on Ctrl-C/SIGTERM. Errors go to stderr. `--stats` adds JSON statistics on stderr. `--threads N` controls concurrent file jobs; `--color auto|always|never` controls text coloring. Machine formats contain no ANSI codes.

`--require-all ID` / `--require-any ID` are repeatable, mutually exclusive file predicates. IDs are literal keywords in plain files or configured IDs in profiles. Predicates run after match selection; accepted files emit all their selected matches.

## Matching and coordinates

- `--ignore-case` folds ASCII A–Z only; original bytes/offsets are preserved.
- `--whole-word` treats ASCII letters, digits and underscore as word bytes.
- `--match-mode all` keeps every match (default).
- `--match-mode longest-per-start` keeps the longest match at each starting byte.
- `--match-mode leftmost-longest` greedily keeps non-overlapping leftmost matches; equal-length ties follow keyword order.
- Text/editor/JSON/CSV line and column values are one-based **byte** coordinates; byteOffset is zero-based. These formats split lines on LF, including CRLF.
- `--format lsp` emits batch Location-shaped JSON with encoded file URIs and zero-based UTF-16 ranges, handling LF, CRLF and CR. It is an output adapter, not a language server. Invalid UTF-8 match boundaries fail explicitly.

JSON returns `{ "matches": [...] }`; CSV includes a header; JSONL emits one record per match. In watch mode JSONL emits `{ "event": "added"|"removed", "match": ... }`. Unchanged rounds are quiet. Context/label changes produce removal/addition events. Watch supports text, editor and JSONL formats.

## Profiles

```text
version 1
profile migration
root "src"
extension ".cpp"
ignore-case false
whole-word true
match-mode "all"
keyword "legacy-client" "migration" "OldClient"
keyword "old-call" "deprecated-api" "old_call"
```

```sh
./mfind --config scout.conf --profile migration --format json
```

`keyword` fields are ID, tag, literal. Quoted fields support spaces and escaped quotes/backslashes. Root paths are relative to the configuration file. Explicit CLI options override profile settings; `--keywords` replaces the profile's keyword list. IDs must be unique within a profile; multiple IDs may label the same literal. Configurations reload each watch round. See [profile semantics](docs/changes/SC-18.md).

## Ignore rules and caching

Nested `.gitignore` files apply within their directories. Root `.scoutignore` adds exclusions. Traversal prunes ignored trees and skips symbolic links, `.git`, `.cache` and `.match_index`. Supported glob/negation semantics and limitations are [documented here](docs/changes/SC-13.md); Git global excludes and `.git/info/exclude` are not consulted. Traversal/read failures are errors rather than silently incomplete searches.

Warm searches verify content fingerprints and reuse canonical matches. This avoids rematching but still reads files to fingerprint them. Fingerprints use noncryptographic FNV-1a. Optional `--cache FILE` snapshots are versioned, query/root-specific and atomically replaced. Invalid or incompatible snapshots rebuild. Legacy caches are not reused.

`--index-mb` bounds estimated retained match payload, **not total RSS**. Current results, per-file snapshots, containers and workers use additional memory. Evicted entries are rescanned; output is never truncated. Changed files are currently read into memory for coordinates. See [cache and watch design](docs/changes/SC-20.md).

## Review series

All changes are proposed for owner review; this list does not imply they are merged. Review in order. Each PR after the first targets its predecessor so the diff contains one change.

| PR | Scope |
|---|---|
| [SC-10 / #9](https://github.com/ReshiAdavan/Scout/pull/9) | Chunk boundaries, offsets, concurrency and test foundation |
| [SC-11 / #10](https://github.com/ReshiAdavan/Scout/pull/10) | Rebuild-safe matcher and consistent index updates |
| [SC-12 / #11](https://github.com/ReshiAdavan/Scout/pull/11) | Keyword replacement, deletions and safe shutdown |
| [SC-13 / #12](https://github.com/ReshiAdavan/Scout/pull/12) | Ignore traversal and content fingerprints |
| [SC-14 / #13](https://github.com/ReshiAdavan/Scout/pull/13) | One-shot CLI, exit codes and deterministic output |
| [SC-15 / #14](https://github.com/ReshiAdavan/Scout/pull/14) | JSON, CSV, JSONL and byte coordinates |
| [SC-16 / #15](https://github.com/ReshiAdavan/Scout/pull/15) | Case-insensitive and whole-word matching |
| [SC-17 / #16](https://github.com/ReshiAdavan/Scout/pull/16) | Longest and leftmost match modes |
| [SC-18 / #17](https://github.com/ReshiAdavan/Scout/pull/17) | Profiles, keyword IDs and tags |
| [SC-19 / #18](https://github.com/ReshiAdavan/Scout/pull/18) | Boolean file queries and statistics |
| [SC-20 / #19](https://github.com/ReshiAdavan/Scout/pull/19) | Bounded retention, persistence and watch deltas |
| [SC-21 / #20](https://github.com/ReshiAdavan/Scout/pull/20) | UTF-16 editor ranges |
| [SC-22 / #21](https://github.com/ReshiAdavan/Scout/pull/21) | Full-stack regression tests, CI, benchmarks and documentation |

The original unchecked README features are covered by SC-15 through SC-21. Scope remains literal search: Unicode case folding, a full LSP server, OS event watchers and a hard process-memory cap are not implemented.

## Benchmarks

From the repository root, after building:

```sh
python3 bench/benchmark.py --binary mfind/mfind > benchmark-results.json
```

The benchmark checks exact results against an independent literal-search reference before reporting cold/warm timings, throughput, per-process peak RSS and watch-change latency. It covers small files, larger files, dense overlapping matches and many keywords across all match modes. `--scale N` increases corpus sizes. [Methodology and sample results](bench/README.md) explain limits; no universal speed claim is made.
