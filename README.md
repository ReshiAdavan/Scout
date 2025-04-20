## findr

lil side project for implementing:
- *multi*-keyword search

## setup

- clone
- populate `keywords.txt` with the keywords you want to query for
- configure `.scoutignore` with the files you want to omit for search
- run mfind with
    - `clang++ -std=c++17 -pthread mfind.cpp core/fileCache.cpp helpers/ignoreRules.cpp -I. -Icore -Ihelpers -Iqueues -Iwalkers -Iworkers -o mfind`
    - `./mfind .`

## todos

multifind
- [X] aho-corasick
- [ ] case-insensitive match
- [X] grouped color-coded output
- [ ] longest match / all match priority toggle
- [ ] keyword tagging or ID support
- [ ] match stats per file/keyword
- [ ] export results (e.g. JSON, CSV)
- [X] watch mode (live file changes)

general
- [X] chunking
- [X] recursive find in directory
- [X] parallelization
    - [X] file-level threading
    - [X] chunk-level threading
- [X] extension filters (e.g., `.txt`, `.cpp`)
- [X] ignore rules support (`.gitignore`)
- [X] cache file hashes to skip unchanged files
- [ ] in-memory index for very large codebases
- [ ] LSP-compatible output formatting

## structure

- walkers/      Spawns walker threads, traverses dirs, pushes files
- queues/	    Holds concurrent fileQueue + matchQueue impls
- workers/	    Thread pool to pop files, run searchFile() using Aho-Corasick
- core/    	    Aho-Corasick engine + file caching
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
