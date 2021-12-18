# LROne
Warning: This program uses Unix terminal features and may not display output correctly on Windows Command Prompt. However file I/O should work.

## Building
This program can be built with CMake or the Makefile such as
```
make -j
```

## Grammar Format
+ The first line of grammar file is a list of terminals separated by space. (Not including $)
+ The second line of grammar file is a lit of non-terminals separated by space. (Not including augmented grammar's start symbol)
+ Following lines each represent a production. First element is the LHS of the production and the following elements are RHS. The arrow is ommited to make parsing the grammar file simpler as it does not add any new information.
+ First production's LHS becomes start symbol.
+ Empty

## Running
```
./lrone -h
```
Example grammars are provided in examples/ directory.
Input terminals are written separated by space.

LR(1) grammars
```
./lrone -g examples/grammar0.txt -s "a b"
./lrone -g examples/grammar0.txt -s "a c d b"
./lrone -g examples/grammar1.txt -s "c c d c d"
./lrone -g examples/grammar2.txt -s "id * ( id + id )"
./lrone -g examples/grammar3.txt -s "id * ( id + id )"
```
Non-LR(1) grammars
```
./lrone -g examples/grammar4.txt
./lrone -g examples/grammar5.txt
```
Fixed LR(1) grammar
```
./lrone -g examples/grammar6.txt -s "if cond then if cond then stmt end else stmt end" -l 30
```

# Performance & tracing

+ The -b flag runs the program in benchmark mode without output to avoid delay caused by I/O.
+ The program has built-in profiling. The -p option can be used to save the timing data to a file to be later visualized with Chromium's built-in profiler (chrome://tracing).
```
./lrone -g examples/grammar6.txt -s "if cond then if cond then stmt else stmt end end" -p profile.json
```
