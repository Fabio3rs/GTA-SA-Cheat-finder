This program permutates the alphabet to discover CRC32 hash collisions for GTA SA Cheats.

It uses all the CPU threads available.

*TODO command line*

Compile with:

Normal:
```
cd src
g++ -march=native -mtune=native -O2 main.cpp crc32.cpp -lpthread
```

Maybe cause bugs, but faster in theory:
```
cd src
g++ -march=native -mtune=native -Ofast main.cpp crc32.cpp -lpthread
```

More info about the topic: https://www.mixmods.com.br/2020/06/como-cheats-gta-foram-descobertos.html
