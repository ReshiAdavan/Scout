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

## deep dive

keyword search
- [X] boyer-moore

multi-keyword search
- [X] aho-corasick

file search
- [ ] fuzzy-search

replace
- [ ] boyer-moore replace
- [ ] aho-corasick replace

general
- [X] chunking
- [X] recursive find in directory
- [ ] parallelization