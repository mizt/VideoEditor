## VideoEditor

#### Build

```
$ xcrun clang++ -std=c++20 -O3 -framework Cocoa ./VideoEditor.mm -o ./VideoEditor
```

#### Run

```
$ ./VideoEditor ./src.mov,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59 ./docs/00.mov
$ ./VideoEditor ./src.mov,{0-59} ./docs/01.mov
$ ./VideoEditor ./src.mov,{59-0} ./docs/02.mov
$ ./VideoEditor ./src.mov,{15-44},{44-15} ./docs/03.mov
$ ./VideoEditor ./src.mov,{{0-29}+15} ./docs/04.mov
$ ./VideoEditor ./src.mov,{0-59}x3 ./docs/05.mov
$ ./VideoEditor ./src.mov,2x{0-29}+1 ./docs/06.mov
$ ./VideoEditor ./src.mov,{3x{0-19}+2}x3 ./docs/07.mov
```

#### Arguments

1st argument is commands.  
2nd argument is output path. (optional)

#### Commands

[https://mizt.github.io/VideoEditor/](https://mizt.github.io/VideoEditor/)

#### Search Path

* ./
* ~/Movies/
* ~/Downloads/
* ~/Documents/

