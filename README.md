```
clang++ -lncurses -std=c++17 -stdlib=libc++ -g main.cpp -o main
```

```
adb logcat -b all -v long -T 100 | ./main
```

- `-v long`: Output "long" format, required.
- `-T 100`: Optional, start from last 100 logs.
- `-b all`: Optional, output all buffer.