// (c) 2020 Fabio R. Sluzala
// This code is licensed under MIT license (see LICENSE for details)
// Inspired on LINK/2012 GTA SA cheat finder
#include "crc32.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <immintrin.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

template <class T, std::size_t N> struct c_array_str {
    T arr[N];

    constexpr T const &operator[](std::size_t p) const { return arr[p]; }

    constexpr T &operator[](std::size_t p) { return arr[p]; }

    constexpr T const *begin() const { return arr + 0; }
    constexpr T const *end() const { return arr + (N - 1); }

    constexpr size_t size() const { return N - 1; }
};

template <class T> struct c_array_str<T, 0> {};

template <class T> struct singlevarnofsh { T a; } __attribute__((aligned(64)));

template <size_t num, class T> class CircleMTIO {
    std::array<T, num> elements;
    std::array<singlevarnofsh<std::atomic<bool>>, num> elements_ready;

    std::atomic<size_t> reading_point;
    std::atomic<size_t> writing_point;

  public:
    std::pair<T *, size_t> new_write() {
        size_t a = writing_point.fetch_add(1);

        if (writing_point >= num) {
            writing_point = 0;
        }

        if (a >= num) {
            // throw std::runtime_error("atomic operation bug ");
        }

        return std::pair<T *, size_t>(&elements[a], a);
    }

    std::pair<T *, bool> next() {
        size_t r = reading_point;

        if (!elements_ready[r].a) {
            return std::pair<T *, bool>(nullptr, false);
        }

        if (r == writing_point) {
            return std::pair<T *, bool>(nullptr, false);
        }

        reading_point++;

        if (reading_point >= num) {
            reading_point = 0;
        }

        elements_ready[r].a = false;

        return std::pair<T *, bool>(&elements[r], true);
    }

    void set_ready(size_t a) { elements_ready[a].a = true; }

    CircleMTIO() {
        reading_point = 0;
        writing_point = 0;

        for (auto &b : elements_ready) {
            b.a = false;
        }
    }
} __attribute__((aligned(64)));

struct collision_data {
    uint32_t hash;
    uintptr_t thread_id;
    std::chrono::time_point<std::chrono::high_resolution_clock> when;
    uint8_t str[100];

    collision_data() {
        hash = 0;
        thread_id = 0;
        str[0] = 0;
    }
};

static_assert(sizeof(collision_data) == 128, "sizeof(collision_data) != 128");

static CircleMTIO<256, collision_data> collisions;

template <class T>
inline void register_collision(
    uint32_t hash,
    std::chrono::time_point<std::chrono::high_resolution_clock> when,
    uintptr_t id, const T &pd, int strsize) {
    auto cols = collisions.new_write();
    collision_data &col = *cols.first;
    col.hash = hash;
    col.when = when;
    col.thread_id = id;
    std::copy(reinterpret_cast<const uint64_t *>(pd.data()) + 0,
              reinterpret_cast<const uint64_t *>(pd.data()) +
                  (pd.size() + sizeof(uint64_t) / 2) / sizeof(uint64_t),
              reinterpret_cast<uint64_t *>(col.str));
    col.str[strsize] = 0xFF;
    collisions.set_ready(cols.second);
}

struct permdata {
    unsigned int len{}, perm{};
};

static std::atomic<unsigned int> last_permpoint;

template <class T> static constexpr size_t round_up8(const T &a) {
    size_t s = a.size() / 8;

    if (a.size() % 8 != 0)
        s++;

    return s * 8;
}

static constexpr c_array_str<char, 27> perm_list{
    {"ABCDEFGHIJKLMNOPQRSTUVWXYZ"}};
static const std::unique_ptr<uint32_t[]>
    perm_list_roundup(std::make_unique<uint32_t[]>(round_up8(perm_list)));
/*
Each thread process all the permutations with certain length starting with some
letter
*/
static permdata assignthreadnewperm(int len, int perm) {
    unsigned int a = last_permpoint.fetch_add(1);

    permdata lastpdata{a / static_cast<unsigned int>(perm_list.size()) + 2,
                       a % static_cast<unsigned int>(perm_list.size())};

    return lastpdata;
}

// https://stackoverflow.com/questions/19559808/constexpr-initialization-of-array-to-sort-contents
template <class T, std::size_t N> struct  alignas(alignof(__m256i)) c_array {
    T arr[N]{};

    constexpr T const &operator[](std::size_t p) const { return arr[p]; }

    constexpr T &operator[](std::size_t p) { return arr[p]; }

    constexpr T const *begin() const { return arr + 0; }
    constexpr T const *end() const { return arr + N; }

    constexpr size_t size() const { return N; }
};

template <std::size_t N> struct c_boolset {
    bool arr[N];

    constexpr c_boolset() : arr{0} {
        for (size_t i = 0; i < N; i++) {
            arr[i] = false;
        }
    }

    constexpr bool const &operator[](std::size_t p) const { return arr[p]; }

    constexpr bool &operator[](std::size_t p) { return arr[p]; }

    constexpr bool const *begin() const { return arr + 0; }
    constexpr bool const *end() const { return arr + N; }

    constexpr size_t size() const { return N; }
};

template <class T> struct c_array<T, 0> {};

struct optliststruct {
    int pos;
    int end;

    constexpr optliststruct() : pos(-1), end(0) {}
};

static constexpr c_array<uint32_t, 87> cheatArray() {
    /*
    0xDE4B237D, 0xB22A28D1, 0x5A783FAE, 0xEECCEA2B, 0x42AF1E28, 0x555FC201,
    0x2A845345, 0xE1EF01EA, 0x771B83FC, 0x5BF12848, 0x44453A17, 0xFCFF1D08,
    0xB69E8532, 0x8B828076, 0xDD6ED9E9, 0xA290FD8C, 0x3484B5A7, 0x43DB914E,
    0xDBC0DD65, 0x00000000, 0xD08A30FE, 0x37BF1B4E, 0xB5D40866, 0xE63B0D99,
    0x675B8945, 0x4987D5EE, 0x2E8F84E8, 0x1A9AA3D6, 0xE842F3BC, 0x0D5C6A4E,
    0x74D4FCB1, 0xB01D13B8, 0x66516EBC, 0x4B137E45, 0x00000000, 0x78520E33,
    0x3A577325, 0xD4966D59, 0x5FD1B49D, 0xA7613F99, 0x1792D871, 0xCBC579DF,
    0x4FEDCCFF, 0x44B34866, 0x2EF877DB, 0x2781E797, 0x2BC1A045, 0xB2AFE368,
    0xFA8DD45B, 0x8DED75BD, 0x1A5526BC, 0xA48A770B, 0xB07D3B32, 0x80C1E54B,
    0x5DAD0087, 0x7F80B950, 0x6C0FA650, 0xF46F2FA4, 0x70164385, 0x00000000,
    0x885D0B50, 0x151BDCB3, 0xADFA640A, 0xE57F96CE, 0x040CF761, 0xE1B33EB9,
    0xFEDA77F7, 0x8CA870DD, 0x9A629401, 0xF53EF5A5, 0xF2AA0C1D, 0xF36345A8,
    0x8990D5E1, 0xB7013B1B, 0xCAEC94EE, 0x31F0C3CC, 0xB3B3E72A, 0xC25CDBFF,
    0xD5CF4EFF, 0x680416B1, 0xCF5FDA18, 0xF01286E9, 0xA841CC0A, 0x31EA09CF,
    0xE958788A, 0x02C83A7C, 0xE49C3ED4, 0x171BA8CC, 0x86988DAE, 0x2BDD2FA1,
    0x00000000, 0x00000000
    */

    c_array<uint32_t, 87> result{
        {0xDE4B237D, 0xB22A28D1, 0x5A783FAE, 0xEECCEA2B, 0x42AF1E28, 0x555FC201,
         0x2A845345, 0xE1EF01EA, 0x771B83FC, 0x5BF12848, 0x44453A17, 0xFCFF1D08,
         0xB69E8532, 0x8B828076, 0xDD6ED9E9, 0xA290FD8C, 0x3484B5A7, 0x43DB914E,
         0xDBC0DD65, 0xD08A30FE, 0x37BF1B4E, 0xB5D40866, 0xE63B0D99, 0x675B8945,
         0x4987D5EE, 0x2E8F84E8, 0x1A9AA3D6, 0xE842F3BC, 0x0D5C6A4E, 0x74D4FCB1,
         0xB01D13B8, 0x66516EBC, 0x4B137E45, 0x78520E33, 0x3A577325, 0xD4966D59,
         0x5FD1B49D, 0xA7613F99, 0x1792D871, 0xCBC579DF, 0x4FEDCCFF, 0x44B34866,
         0x2EF877DB, 0x2781E797, 0x2BC1A045, 0xB2AFE368, 0xFA8DD45B, 0x8DED75BD,
         0x1A5526BC, 0xA48A770B, 0xB07D3B32, 0x80C1E54B, 0x5DAD0087, 0x7F80B950,
         0x6C0FA650, 0xF46F2FA4, 0x70164385, 0x885D0B50, 0x151BDCB3, 0xADFA640A,
         0xE57F96CE, 0x040CF761, 0xE1B33EB9, 0xFEDA77F7, 0x8CA870DD, 0x9A629401,
         0xF53EF5A5, 0xF2AA0C1D, 0xF36345A8, 0x8990D5E1, 0xB7013B1B, 0xCAEC94EE,
         0x31F0C3CC, 0xB3B3E72A, 0xC25CDBFF, 0xD5CF4EFF, 0x680416B1, 0xCF5FDA18,
         0xF01286E9, 0xA841CC0A, 0x31EA09CF, 0xE958788A, 0x02C83A7C, 0xE49C3ED4,
         0x171BA8CC, 0x86988DAE, 0x2BDD2FA1}};

    // constexpr sort
    for (size_t i = 0; i < result.size(); i++) {
        for (size_t j = i + 1; j < result.size(); j++) {
            if (result[i] > result[j]) {
                auto a = std::move(result[i]);
                result[i] = std::move(result[j]);
                result[j] = std::move(a);
            }
        }
    }

    return result;
}

template <uint32_t OPTSIZE, uint32_t START, uint32_t DIVISOR>
static constexpr c_array<optliststruct, OPTSIZE>
gentblvec(const c_array<uint32_t, 87> &ct) {
    c_array<optliststruct, OPTSIZE> tblvec;

    for (size_t i = 0, size = ct.size(); i < size; i++) {
        uint32_t BASE = ct[i];

        auto &op = tblvec[BASE / DIVISOR];

        if (op.pos == -1) {
            op.pos = i;
            op.end = i;
        }

        op.end++;
    }

    return tblvec;
}

static constexpr uint32_t real_divisor(const uint32_t t) {
    uint32_t result = 1;

    while (result < t)
        result <<= 1;

    return result;
}

constexpr static c_array<uint32_t, 87> cheatTable = cheatArray();

constexpr static uint32_t LAST = cheatTable[cheatTable.size() - 1];
constexpr static uint32_t DIFF = (LAST - cheatTable[0]);
constexpr static uint32_t START = cheatTable[0],
                          DIVISOR = real_divisor(LAST / 40960);
constexpr static size_t OPTSIZE = LAST / DIVISOR + 1;

constexpr static c_array<optliststruct, OPTSIZE> tblvec =
    gentblvec<OPTSIZE, START, DIVISOR>(cheatTable);

static const __m256i min = _mm256_set1_epi32(START), max = _mm256_set1_epi32(LAST);

struct alignas(alignof(__m256i)) m256istruct {
    union {
        __m256i a;
        uint32_t b[8];
    };

    constexpr m256istruct() : a{0} {}
};

static inline uint32_t *pm256s_toui(m256istruct *r) { return (uint32_t *)r; }

template <size_t OPTSIZE, uint32_t START, uint32_t DIVISOR, class T, class D>
static constexpr c_array<uint32_t, OPTSIZE> gentblsimdvec(const T &tblvec,
                                                          const D &ct) {
    c_array<uint32_t, OPTSIZE> result;

    for (size_t i = 0, size = OPTSIZE; i < size; i++) {
        if (tblvec[i].pos == -1) {
            result[i] = 0;
            continue;
        }
        
        int cnt = 0;
        for (size_t j = tblvec[i].pos, end = tblvec[i].end; j != end;
             j++) {
            if (ct[j] != 0) {
                result[i] = ct[j];
                cnt++;
            }
        }

        if (cnt > 1) {
            std::cout << "cnt > 1" << std::endl;

            for (size_t j = tblvec[i].pos, end = tblvec[i].end; j != end; j++) {
                std::cout << std::hex << ct[j] << std::dec << " ";
            }
            std::cout << std::endl;
        }
    }

    return result;
}

template <size_t OPTSIZE, uint32_t START, uint32_t DIVISOR, class T, class D>
static constexpr c_boolset<OPTSIZE> gentblttable(const T &tblvec, const D &ct) {
    c_boolset<OPTSIZE> result;

    for (size_t i = 0, size = OPTSIZE; i < size; i++) {
        result[i] = false;

        if (tblvec[i].pos == -1) {
            continue;
        }

        for (size_t j = tblvec[i].pos, end = tblvec[i].end, l = 0; j != end;
             j++, l++) {
            result[i] = true;
        }
    }

    return result;
}

static inline void hashsfill(uint32_t hs, std::array<m256istruct, 4> &ahash) {
    __m256i crc = _mm256_set1_epi32(hs);
    __m256i crcshifted = _mm256_srli_epi32(crc, 8);
    __m256i __mm256xFF = _mm256_set1_epi32(0xFF);

    int i[8], o[8];
    for (size_t j = 0, kc = 0; j < perm_list.size(); j += 8, kc++) {
        __m256i chars =
            _mm256_lddqu_si256((const __m256i *)&perm_list_roundup[j]);
        __m256i crcxor = _mm256_xor_si256(crc, chars);

        __m256i pos = _mm256_and_si256(crcxor, __mm256xFF);
        _mm256_storeu_si256((__m256i *)i, pos);

        size_t l = (perm_list.size() - j) >= 8 ? 8 : perm_list.size() - j;
        for (size_t k = 0; k < l; k++) {
            o[k] = crcTable[i[7 - k]];
        }

        __m256i ifromtbl = _mm256_lddqu_si256((__m256i *)o);
        _mm256_storeu_si256(&(ahash[kc].a),
                            _mm256_xor_si256(ifromtbl, crcshifted));
    }
}

static inline std::bitset<32> hashcmp(std::array<m256istruct, 4> &ahash) {
    std::bitset<32> result;

    int bit = 0;
    for (size_t i = 0; i < ahash.size(); i++) {
        __m256i minr = _mm256_min_epu32(ahash[i].a, max);
        __m256i maxr = _mm256_max_epu32(ahash[i].a, min);

        __m256i minrcmp = _mm256_cmpgt_epi32(minr, max);
        __m256i maxrcmp = _mm256_cmpgt_epi32(maxr, min);

        int bits =
            _mm256_movemask_epi8(minrcmp) & _mm256_movemask_epi8(maxrcmp);

        for (int valm = bit + 32; bit < valm; bit++) {
            result[bit] = (bits & 7);
            bits >>= 8;
        }
    }

    return result;
}

constexpr static c_array<uint32_t, OPTSIZE> tblvecsimd =
    gentblsimdvec<OPTSIZE, START, DIVISOR, c_array<optliststruct, OPTSIZE>>(
        tblvec, cheatTable);
constexpr static c_boolset<OPTSIZE> tblttable =
    gentblttable<OPTSIZE, START, DIVISOR, c_array<optliststruct, OPTSIZE>>(
        tblvec, cheatTable);

static void findcollisions_mthread(uint32_t hash, size_t length,
                                   uintptr_t thread_id) {
    if (perm_list.size() == 0)
        return;

    if (length > 31)
        length = 31;

    std::array<std::array<m256istruct, 4>, 32> hashsbylen;
    std::array<uint8_t, 32> permlen;

    std::fill(permlen.begin(), permlen.end(), 0);

    permdata pd = assignthreadnewperm(0, 0);

    hashsfill(0xFFFFFFFF, hashsbylen[0]);

    for (size_t i = pd.len; i < length; /*i++*/) {
        i = pd.len;
        permlen[0] = pd.perm;

        for (size_t j = 1; j < i; j++) {
            size_t jm1 = j - 1;
            hashsfill(pm256s_toui(hashsbylen[jm1].data())[permlen[jm1]],
                      hashsbylen[j]);
        }

        size_t imone = i - 1;

        while (true) {
            for (size_t ji = 0; ji < perm_list.size(); ji++) {
                uint32_t hashbase = pm256s_toui(hashsbylen[imone].data())[ji];

                for (size_t j = 0; j < perm_list.size(); j++) {
                    uint32_t resulthash =
                        updateCrc32Char(hashbase, perm_list[j]);

                    if (resulthash != 0) {
                        uint32_t B = resulthash;
                        B /= DIVISOR;

                        if (!tblttable[B])
                            continue;

                        if (tblvecsimd[B] == resulthash) {
                            [[unlikely]];
                            // complete the string
                            permlen[imone] = ji;
                            permlen[i] = j;

                            // Send to IO
                            auto nowtime =
                                std::chrono::high_resolution_clock::now();
                            register_collision(resulthash, nowtime, thread_id,
                                               permlen, i + 1);
                        }
                    }
                }
            }

            bool next = false;
            size_t imtw = imone - 1;

            for (size_t l = imtw; l >= 0; l--) {
                if (l == 0) {
                    pd = assignthreadnewperm(i, 0);
                    i = pd.len;
                    break;
                }

                permlen[l]++;

                if (permlen[l] != perm_list.size()) {
                    size_t lp1 = (l + 1);

                    for (size_t j = lp1; j < i; j++) {
                        size_t jm1 = j - 1;
                        hashsfill(
                            pm256s_toui(hashsbylen[jm1].data())[permlen[jm1]],
                            hashsbylen[j]);
                    }

                    next = true;
                    break;
                } else {
                    permlen[l] = 0;
                }
            }

            if (!next)
                break;
        }
    }
}

static void findcollisions(uint32_t hash, size_t length,
                           std::string perm_list) {
    if (perm_list.size() == 0)
        return;

    char str[128] = {0};

    std::sort(perm_list.begin(), perm_list.end());
    std::cout << "Permutation list " << perm_list << std::endl;

    for (size_t i = 1; i < length; i++) {
        for (size_t j = 0; j < i; j++)
            str[j] = perm_list[0];

        while (true) {
            uint32_t hashbase = crc32FromStringLen(str, i);

            for (size_t j = 0; j < perm_list.size(); j++) {
                str[i] = perm_list[j];
                uint32_t resulthash = updateCrc32String(hashbase, &str[i], 1);

                if (resulthash == hash)
                    std::cout << str << " 0x" << std::hex << hashbase << " 0x"
                              << resulthash << std::endl;
            }

            bool next = false;

            for (size_t l = i - 1; l >= 0; l--) {
                auto it = std::find(perm_list.begin(), perm_list.end(), str[l]);

                if (it == perm_list.end()) {
                    std::cout << "error";
                }

                ++it;
                if (it != perm_list.end()) {
                    str[l] = *it;

                    for (size_t j = l + 1; j < i; j++)
                        str[j] = perm_list[0];

                    next = true;
                    break;
                }
            }

            if (!next)
                break;
        }
    }
}

static std::atomic<bool> iothreadShouldContinue;

/**/
static void
io_thread(std::chrono::time_point<std::chrono::high_resolution_clock> start) {
    std::string buffer;
    std::string temp;
    temp.reserve(64);
    buffer.reserve(2048);

    char btmp[32] = {0};

    while (iothreadShouldContinue) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        {
            // if (io_collisions.size() > 0)
            {
                buffer.clear();

                for (auto nxt = collisions.next(); nxt.second;
                     nxt = collisions.next()) {
                    auto &io = *nxt.first;
                    temp.clear();
                    std::chrono::duration<double> diff = io.when - start;

                    temp += std::to_string(diff.count());
                    temp.append(24 - temp.size(), ' ');
                    temp += "  #";
                    temp += std::to_string(io.thread_id);
                    temp += "  ";
                    buffer += temp;

                    buffer.append(36 - temp.size(), ' ');
                    temp.clear();

                    sprintf(btmp, "%.8X", io.hash);

                    temp += btmp;
                    temp += "  ";
                    buffer += temp;

                    buffer.append(12 - temp.size(), ' ');

                    temp.clear();
                    uint8_t *p = io.str;
                    while (*p != 0xFF) {
                        temp += perm_list[*p++];
                    }
                    std::reverse(temp.begin(), temp.end());
                    buffer += temp;
                    buffer += "\n";
                }

                // io_collisions.clear();
            }
        }

        if (buffer.size() > 0) {
            std::cout << buffer;
            buffer.clear();
        }
    }
}

int main(int argc, char *argv[]) {
    int max_length = 31;

    if (argc > 1) {
        max_length = 7;
        std::cout << "max_length = " << max_length << std::endl;
    }

    memset(perm_list_roundup.get(), 0,
           round_up8(perm_list) * sizeof(perm_list_roundup[0]));

    for (size_t i = 0; i < perm_list.size(); i++) {
        perm_list_roundup[i] = (unsigned char)(perm_list[i]);
    }

    std::reverse(perm_list_roundup.get(), perm_list_roundup.get() + 8);
    std::reverse(perm_list_roundup.get() + 8, perm_list_roundup.get() + 16);
    std::reverse(perm_list_roundup.get() + 16, perm_list_roundup.get() + 24);
    std::reverse(perm_list_roundup.get() + 24, perm_list_roundup.get() + 32);

    iothreadShouldContinue = true;

    int threads = std::thread::hardware_concurrency();
    std::vector<std::thread> thrds;
    thrds.reserve(threads);

    auto starttime = std::chrono::high_resolution_clock::now();
    std::thread iothred(io_thread, starttime);

    std::cout << "TIME                      THREAD    HASH        STRING"
              << std::endl;
    for (int i = 0; i < threads; i++) {
        thrds.push_back(
            std::thread(findcollisions_mthread, 0xDE4B237D, max_length, i + 1));
    }

    // findcollisions(0xDE4B237D, 16, "ABCDEFGHIJKLMNOPQRTUVWXYZ");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    for (auto &t : thrds) {
        if (t.joinable())
            t.join();
    }

    iothreadShouldContinue = false;

    if (iothred.joinable())
        iothred.join();

    return 0;
}
