#### Build & Run

```
$ xcrun clang++ -std=c++20 -O3 -framework Cocoa ./VideoEditor.mm -o ./VideoEditor
$ ./VideoEditor ./src.mov,320-291,292-351,350-321 ./dst.mov
```

#### Arguments

1st argument is commands.  
2nd argument is output path. (optional)

#### Commands

| operator | operation |
| -------- | --------- |
| -        | range     |
| x        | repeat    |



