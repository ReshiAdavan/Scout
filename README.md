## findr

lil side project for implementing:
- *multi*-keyword search

## setup

- clone
- run mfind with
    - `clang++ -std=c++17 -pthread  mfind.cpp -I. -Iwalkers -Iqueues -Iworkers -Ihelpers -Iaho -o mfind`
    - `./mfind . "Alice" "library" "book"`

## todos

multifind
- [X] aho-corasick
- [ ] case-insensitive match
- [X] grouped color-coded output
- [ ] keyword tagging or ID support
- [ ] longest match / all match priority toggle
- [ ] match stats per file/keyword
- [ ] export results (e.g. JSON, CSV)
- [ ] watch mode (live file changes)

general
- [X] chunking
- [X] recursive find in directory
- [X] parallelization
    - [X] file-level threading
    - [ ] chunk-level threading
- [ ] extension filters (e.g., `.txt`, `.cpp`)
- [ ] ignore rules support (`.gitignore`)
- [ ] cache file hashes to skip unchanged files
- [ ] in-memory index for very large codebases
- [ ] LSP-compatible output formatting
- [ ] plugin system or extensible backend

other
- [ ] minimal GUI wrapper
- [ ] serve search via API (search as a service)
- [ ] git diff aware searching

## structure

- walkers/      Spawns walker threads, traverses dirs, pushes files
- queues/	    Holds concurrent fileQueue + matchQueue impls
- workers/	    Thread pool to pop files, run searchFile() using Aho-Corasick
- output/	    One thread dequeues matches and prints them out
- aho/    	    Reusable Aho-Corasick engine
- helpers/	    Format/context/extension utils

## archive

find
- [X] boyer-moore
- [ ] case-insensitive match
- [ ] whole-word match
- [ ] line number tracking
- [ ] highlight match in context

search
- [X] fuzzy-search
- [ ] match highlighting in file path
- [ ] score file types by relevance
- [ ] return top-N best matches
- [ ] support basename-only matching
- [ ] frecency-aware sorting
- [ ] Levenshtein-based fallback

replace
- [ ] boyer-moore replace
- [ ] aho-corasick replace
- [ ] dry-run preview mode
- [ ] diff-style preview
- [ ] replace with context
- [ ] write to temp/backup path
