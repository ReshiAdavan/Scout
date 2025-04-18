## findr

lil side project for implementing:
- file search in dir
- keyword search
- *multi*-keyword search

## setup

- clone
- run find with 
    - `clang++ -std=c++17 -stdlib=libc++ find.cpp -o find`
    - `./find . Alice`

- run mfind with
    - `clang++ -std=c++17 -stdlib=libc++ mfind.cpp -o mfind`
    - `./mfind . "Alice" "library" "book"`

## todos

find
- [X] boyer-moore
- [ ] case-insensitive match
- [ ] whole-word match
- [ ] line number tracking
- [ ] highlight match in context

multifind
- [X] aho-corasick
- [ ] case-insensitive match
- [ ] grouped color-coded output
- [ ] keyword tagging or ID support
- [ ] longest match / all match priority toggle
- [ ] match stats per file/keyword
- [ ] export results (e.g. JSON, CSV)
- [ ] watch mode (live file changes)

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

general
- [X] chunking
- [X] recursive find in directory
- [ ] parallelization (file-level threading)
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
