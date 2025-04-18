## findr

lil side project for implementing:
- file search in dir
- keyword search
- *multi*-keyword search

## setup
- clone
- run with 
    - `clang++ main.cpp -o findr`
    - `./findr`

## deep dive

keyword search
- "boyer-moore" on file specified to find instances where keywords exists
- replace tbd
- for large files we want to load as chunks, deal with words over chunks somehow

multi keyword search
- "aho-corasick" to determine existence of multiple keywords
- replace tbd
- for large files we load as chunks, figure out the in b/w

file search
- "fuzzy-search" over entire dir
- 
