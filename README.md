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
- [X] "boyer-moore" on file specified to find instances where keywords exists
- [X] read file into chunks
- [X] recursively detect presence of keyword across a whole directory
- [ ] "replace feature"
- [ ] deploy worker threads and run boyer moore in parallel across the file (tbd)

multi-keyword search
- [ ] "aho-corasick" to determine existence of multiple keywords
- [ ] read file into chunks
- [ ] "replace feature"

file search
- [ ] "fuzzy-search" over entire dir
