```
$ xcrun clang++ -std=c++20 -O3 -I../../../libs -framework Cocoa ./VideoEditor.mm -o ./VideoEditor
$ ./VideoEditor ./src.mov,320-291,292-351,350-321 ./out.mov
```