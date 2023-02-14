```
$ xcrun clang++ -std=c++20 -O3 -framework Cocoa ./VideoEditor.mm -o ./VideoEditor
$ ./VideoEditor ./src.mov,320-291,292-351,350-321 ./dst.mov
```