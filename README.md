# GTA SA Cheat Finder

This program permutates the alphabet to discover CRC32 hash collisions for GTA SA Cheats.

It uses all the CPU threads available (some branches perform different from others).

## Branches

### main
- commom C++ code, should be compatible with any CPU

### Constexpr_optimizations
- testing constexpr

### SIMD_Highcache
- using AVX/AVX2 SIMD instructions manually and constexpr as much as possible

## Compiling
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

For testing purposes:
```
g++ -march=native -mtune=native -Ofast main.cpp crc32.cpp -lpthread -fgraphite-identity -floop-nest-optimize -fdevirtualize-at-ltrans -fipa-pta -fno-semantic-interposition -fuse-linker-plugin -falign-functions=32 -fPIC -fvisibility=hidden -flto
```

```
g++-9 -march=native -mtune=native -Ofast main.cpp crc32.cpp -lpthread -fgraphite-identity -floop-nest-optimize -fdevirtualize-at-ltrans -fipa-pta -funroll-loops -floop-optimize -fno-semantic-interposition -fuse-linker-plugin -falign-functions=32 -fPIC -flto
```

More info about the topic: https://www.mixmods.com.br/2020/06/como-cheats-gta-foram-descobertos.html
