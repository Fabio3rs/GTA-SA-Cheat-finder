/*
CLEO Library (c) 2007-2016 Seemann, Alien, Deji

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
// https://github.com/cleolibrary/CLEO4/blob/master/source/crc32.cpp
#include "crc32.h"
#include <ctype.h>

uint32_t crc32FromUpcaseString(const char *str) {
    uint32_t crc = 0xFFFFFFFF;
    while (*str)
        crc = crcTable[(crc ^ toupper(*str++)) & 0xff] ^ (crc >> 8);
    return crc;
}

uint32_t crc32FromUpcaseStdString(const std::string &str) {
    return crc32FromUpcaseString(str.c_str());
}

uint32_t crc32FromString(const char *str) {
    uint32_t crc = 0xFFFFFFFF;
    while (*str)
        crc = crcTable[(crc ^ (*str++)) & 0xff] ^ (crc >> 8);
    return crc;
}

uint32_t crc32FromStringLen(const char *str, uint32_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (unsigned i = 0; i < len; i++)
        crc = crcTable[(crc ^ str[i]) & 0xFF] ^ (crc >> 8);
    return crc;
}

uint32_t crc32FromStdString(const std::string &str) {
    return crc32FromString(str.c_str());
}

uint32_t crc32(const unsigned char *buf, uint32_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (unsigned i = 0; i < len; i++)
        crc = crcTable[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return crc;
}

uint32_t updateCrc32(uint32_t crc, const unsigned char *buf, uint32_t len) {
    for (unsigned i = 0; i < len; i++)
        crc = crcTable[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return crc;
}

uint32_t updateCrc32String(uint32_t crc, const char *str, uint32_t len) {
    for (unsigned i = 0; i < len; i++)
        crc = crcTable[(crc ^ str[i]) & 0xFF] ^ (crc >> 8);
    return crc;
}
