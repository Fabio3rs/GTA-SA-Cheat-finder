// (c) 2020 Fabio R. Sluzala
// This code is licensed under MIT license (see LICENSE for details)
// Inspired on LINK/2012 GTA SA cheat finder
#include <iostream>
#include <array>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <mutex>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstring>
#include <immintrin.h>
#include "crc32.h"

template<unsigned int num, class T>
class CircleMTIO
{
    std::array<T, num>                          elements;
    std::array<std::atomic<bool>, num>          elements_ready;

    std::atomic<int>                            reading_point;
    std::atomic<int>                            writing_point;

public:
    std::pair<T*, int> new_write()
    {
        int a = writing_point.fetch_add(1);

        if (writing_point >= num)
        {
            writing_point = 0;
        }

        if (a >= num)
        {
            throw std::runtime_error("atomic operation bug ");
        }

        return std::pair<T*, int>(&elements[a], a);
    }

    std::pair<T*, bool> next()
    {
        int r = reading_point;

        if (!elements_ready[r])
        {
            return std::pair<T*, bool>(nullptr, false);
        }

        if (r == writing_point)
        {
            return std::pair<T*, bool>(nullptr, false);
        }

        reading_point++;

        if (reading_point >= num)
        {
            reading_point = 0;
        }

        elements_ready[r] = false;

        return std::pair<T*, bool>(&elements[r], true);
    }

    void set_ready(int a)
    {
        elements_ready[a] = true;
    }

    CircleMTIO()
    {
        reading_point = 0;
        writing_point = 0;

        for (auto &b : elements_ready)
        {
            b = false;
        }
    }
} __attribute__ ((aligned(64)));

struct collision_data
{
    uint32_t hash;
    uintptr_t thread_id;
    std::chrono::time_point<std::chrono::high_resolution_clock> when;
    char str[128];

    collision_data()
    {
        hash = 0;
        thread_id = 0;
        str[0] = 0;
    }
};

CircleMTIO<256, collision_data> collisions;

void register_collision(uint32_t hash, std::chrono::time_point<std::chrono::high_resolution_clock> when, uintptr_t id, const char *str, int strsize)
{
    auto cols = collisions.new_write();
    collision_data &col = *cols.first;
    col.hash = hash;
    col.when = when;
    col.thread_id = id;
    int size = strsize > sizeof(col.str)? (sizeof(col.str) - 1) : strsize;
    strncpy(col.str, str, size);
    col.str[size] = 0;
    collisions.set_ready(cols.second);
}

struct permdata
{
    int len, perm;

    permdata()
    {
        len = perm = 0;

        perm = -1;
        len = 1;
    }
};

permdata lastpdata;
std::mutex assignpermmutx;

template<class T>
constexpr size_t round_up8(const T &a)
{
    size_t s = a.size() / 8;

    if (a.size() % 8 != 0)
        s++;
    
    return s * 8;
}

const std::string perm_list = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::unique_ptr<char[]> perm_list_roundup(std::make_unique<char[]>(round_up8(perm_list)));
/*
Each thread process all the permutations with certain length starting with some letter
*/
permdata assignthreadnewperm(int len, int perm)
{
    std::lock_guard<std::mutex> lck(assignpermmutx);

    ++lastpdata.perm;
    if (lastpdata.perm >= perm_list.size())
    {
        ++lastpdata.len;
        lastpdata.perm = 0;
    }

    return lastpdata;
}

// https://stackoverflow.com/questions/19559808/constexpr-initialization-of-array-to-sort-contents
template<class T, std::size_t N>
struct c_array
{
    T arr[N];

    constexpr T const& operator[](std::size_t p) const
    { return arr[p]; }

    constexpr T & operator[](std::size_t p)
    { return arr[p]; }

    constexpr T const* begin() const
    { return arr+0; }
    constexpr T const* end() const
    { return arr+N; }

    constexpr size_t size() const
    {
        return N;
    }
};

template<class T>
struct c_array<T, 0> {};


struct optliststruct
{
    int pos;
    int end;

    constexpr optliststruct() : pos(-1), end(0)
    {

    }
};

constexpr c_array<uint32_t, 87> cheatArray()
{
    /*
    0xDE4B237D, 0xB22A28D1, 0x5A783FAE, 0xEECCEA2B, 0x42AF1E28, 0x555FC201, 0x2A845345, 0xE1EF01EA, 0x771B83FC, 0x5BF12848,
        0x44453A17, 0xFCFF1D08, 0xB69E8532, 0x8B828076, 0xDD6ED9E9, 0xA290FD8C, 0x3484B5A7, 0x43DB914E, 0xDBC0DD65, 0x00000000, 0xD08A30FE, 0x37BF1B4E,
        0xB5D40866, 0xE63B0D99, 0x675B8945, 0x4987D5EE, 0x2E8F84E8, 0x1A9AA3D6, 0xE842F3BC, 0x0D5C6A4E, 0x74D4FCB1, 0xB01D13B8, 0x66516EBC, 0x4B137E45,
        0x00000000, 0x78520E33, 0x3A577325, 0xD4966D59, 0x5FD1B49D, 0xA7613F99, 0x1792D871, 0xCBC579DF, 0x4FEDCCFF, 0x44B34866, 0x2EF877DB, 0x2781E797,
        0x2BC1A045, 0xB2AFE368, 0xFA8DD45B, 0x8DED75BD, 0x1A5526BC, 0xA48A770B, 0xB07D3B32, 0x80C1E54B, 0x5DAD0087, 0x7F80B950, 0x6C0FA650, 0xF46F2FA4,
        0x70164385, 0x00000000, 0x885D0B50, 0x151BDCB3, 0xADFA640A, 0xE57F96CE, 0x040CF761, 0xE1B33EB9, 0xFEDA77F7, 0x8CA870DD, 0x9A629401, 0xF53EF5A5,
        0xF2AA0C1D, 0xF36345A8, 0x8990D5E1, 0xB7013B1B, 0xCAEC94EE, 0x31F0C3CC, 0xB3B3E72A, 0xC25CDBFF, 0xD5CF4EFF, 0x680416B1, 0xCF5FDA18, 0xF01286E9,
        0xA841CC0A, 0x31EA09CF, 0xE958788A, 0x02C83A7C, 0xE49C3ED4, 0x171BA8CC, 0x86988DAE, 0x2BDD2FA1, 0x00000000, 0x00000000
    */
   
    c_array<uint32_t, 87> result{{ 0xDE4B237D, 0xB22A28D1, 0x5A783FAE, 0xEECCEA2B, 0x42AF1E28, 0x555FC201, 0x2A845345, 0xE1EF01EA, 0x771B83FC, 0x5BF12848,
        0x44453A17, 0xFCFF1D08, 0xB69E8532, 0x8B828076, 0xDD6ED9E9, 0xA290FD8C, 0x3484B5A7, 0x43DB914E, 0xDBC0DD65, 0xD08A30FE, 0x37BF1B4E,
        0xB5D40866, 0xE63B0D99, 0x675B8945, 0x4987D5EE, 0x2E8F84E8, 0x1A9AA3D6, 0xE842F3BC, 0x0D5C6A4E, 0x74D4FCB1, 0xB01D13B8, 0x66516EBC, 0x4B137E45,
        0x78520E33, 0x3A577325, 0xD4966D59, 0x5FD1B49D, 0xA7613F99, 0x1792D871, 0xCBC579DF, 0x4FEDCCFF, 0x44B34866, 0x2EF877DB, 0x2781E797,
        0x2BC1A045, 0xB2AFE368, 0xFA8DD45B, 0x8DED75BD, 0x1A5526BC, 0xA48A770B, 0xB07D3B32, 0x80C1E54B, 0x5DAD0087, 0x7F80B950, 0x6C0FA650, 0xF46F2FA4,
        0x70164385, 0x885D0B50, 0x151BDCB3, 0xADFA640A, 0xE57F96CE, 0x040CF761, 0xE1B33EB9, 0xFEDA77F7, 0x8CA870DD, 0x9A629401, 0xF53EF5A5,
        0xF2AA0C1D, 0xF36345A8, 0x8990D5E1, 0xB7013B1B, 0xCAEC94EE, 0x31F0C3CC, 0xB3B3E72A, 0xC25CDBFF, 0xD5CF4EFF, 0x680416B1, 0xCF5FDA18, 0xF01286E9,
        0xA841CC0A, 0x31EA09CF, 0xE958788A, 0x02C83A7C, 0xE49C3ED4, 0x171BA8CC, 0x86988DAE, 0x2BDD2FA1
    }};
    
    // constexpr sort
    for (int i = 0; i < result.size(); i++)
    {
        for (int j = i + 1; j < result.size(); j++)
        {
            if (result[i] > result[j])
            {
                auto a = std::move(result[i]);
                result[i] = std::move(result[j]);
                result[j] = std::move(a);
            }
        }
    }

    return result;
}

template<uint32_t OPTSIZE, uint32_t START, uint32_t DIVISOR>
constexpr c_array<optliststruct, OPTSIZE> gentblvec(const c_array<uint32_t, 87> &ct)
{
    c_array<optliststruct, OPTSIZE> tblvec;

    for (int i = 0, size = ct.size(); i < size; i++)
    {
        uint32_t BASE = ct[i] - START;

        auto &op = tblvec[BASE / DIVISOR];

        if (op.pos == -1)
        {
            op.pos = i;
            op.end = i;
        }
            
        op.end++;
    }

    return tblvec;
}


constexpr c_array<uint32_t, 87> cheatTable = cheatArray();

constexpr uint32_t LAST = cheatTable[cheatTable.size() - 1];
constexpr uint32_t DIFF = (LAST - cheatTable[0]);
constexpr uint32_t START = cheatTable[0], DIVISOR = (DIFF / cheatTable.size());
constexpr uint32_t OPTSIZE = LAST / DIVISOR + 1;

constexpr c_array<optliststruct, OPTSIZE> tblvec = gentblvec<OPTSIZE, START, DIVISOR>(cheatTable);

struct m256istruct
{
    __m256i a;

    m256istruct() : a{0}
    {

    }
};

inline uint32_t *pm256s_toui(m256istruct *r)
{
    return (uint32_t*)r;
}

template<uint32_t OPTSIZE, uint32_t START, uint32_t DIVISOR, class T, class D>
constexpr c_array<m256istruct, OPTSIZE> gentblsimdvec(const T &tblvec, const D &ct)
{
    c_array<m256istruct, OPTSIZE> result;

    for (int i = 0, size = OPTSIZE; i < size; i++)
    {
        if (tblvec[i].pos == -1)
        {
            int *a = (int*)(&result[i].a);

            for (int j = 0; j < 8; j++)
            {
                a[j] = 0;
            }

            continue;
        }

        int *temp = (int*)(&result[i].a);

        for (int j = tblvec[i].pos, end = tblvec[i].end, l = 0; j != end; j++, l++)
        {
            temp[l] = ct[j];
        }
    }

    return result;
}

inline void hashsfill(uint32_t hs, std::array<m256istruct, 4> &ahash)
{
    __m256i tmp256i = _mm256_set1_epi32(hs);
    for (int j = 0, kc = 0; j < perm_list.size(); j += 8, kc++)
    {
        ahash[kc].a = SIMDupdateCrc32Char(tmp256i, &perm_list_roundup[j]);
    }
}

void findcollisions_mthread(uint32_t hash, int length, uintptr_t thread_id)
{
    if (perm_list.size() == 0)
        return;

    if (length > 31)
        length = 31;

    const c_array<m256istruct, OPTSIZE> tblvecsimd = gentblsimdvec<OPTSIZE, START, DIVISOR, c_array<optliststruct, OPTSIZE>>(tblvec, cheatTable);
    
    std::array<std::array<m256istruct, 4>, 32> hashsbylen;
    std::array<uint8_t, 32> permlen;

    std::fill(permlen.begin(), permlen.end(), 0);

    char str[32] = { 0 };

    permdata pd = assignthreadnewperm(0, 0);

    hashsfill(0xFFFFFFFF, hashsbylen[0]);

    for (int i = pd.len; i < length; /*i++*/)
    {
        i = pd.len;
        permlen[0] = pd.perm;

        memset(&str, perm_list_roundup[0], i);
        str[0] = perm_list_roundup[pd.perm];

        for (int j = 1; j < i; j++)
        {
            int jm1 = j - 1;
            hashsfill(pm256s_toui(hashsbylen[jm1].data())[permlen[jm1]], hashsbylen[j]);
        }

        int imone = i - 1;

        while (true)
        {
            //for (int ji = 0; ji < perm_list.size(); ji++)
            {
                uint32_t hashbase = pm256s_toui(hashsbylen[imone].data())[permlen[imone]];
                
                for (int j = 0; j < perm_list.size(); j++)
                {
                    uint32_t resulthash = updateCrc32Char(hashbase, perm_list_roundup[j]);

                    if (resulthash >= cheatTable[0] && resulthash <= cheatTable[cheatTable.size() - 1])
                    {
                        __m256i __resulthash = _mm256_set1_epi32(static_cast<int>(resulthash));
                        uint32_t B = resulthash - START;
                        B /= DIVISOR;

                        __m256i r = _mm256_cmpeq_epi32(__resulthash, tblvecsimd[B].a);
                        int findeq = _mm256_movemask_epi8(r);

                        if (findeq != 0)
                        {
                            // complete the string
                            str[imone] = perm_list_roundup[permlen[imone]];
                            str[i] = perm_list_roundup[j];

                            // Send to IO
                            auto nowtime = std::chrono::high_resolution_clock::now();
                            register_collision(resulthash, nowtime, thread_id, str, i + 1);
                        }
                    }
                }
            }
            
            bool next = false;

            for (int l = i - 1; l >= 0; l--)
            {
                if (l == 0)
                {
                    pd = assignthreadnewperm(i, 0);
                    i = pd.len;
                    break;
                }

                permlen[l]++;

                if (permlen[l] != perm_list.size())
                {
                    str[l] = perm_list_roundup[permlen[l]];

                    int lp1 = (l + 1);
                    memset(&str[lp1], perm_list_roundup[0], i - lp1);

                    for (int j = lp1; j < i; j++)
                    {
                        int jm1 = j - 1;
                        hashsfill(pm256s_toui(hashsbylen[jm1].data())[permlen[jm1]], hashsbylen[j]);
                    }
                    
                    next = true;
                    break;
                }
                else
                {
                    permlen[l] = 0;
                }
            }
            
            if (!next)
                break;
        }
    }
}

void findcollisions(uint32_t hash, int length, std::string perm_list)
{
    if (perm_list.size() == 0)
        return;

    std::array<uint32_t, 64> hashbylen;
    char str[128] = { 0 };

    std::sort(perm_list.begin(), perm_list.end());
    std::cout << "Permutation list " << perm_list << std::endl;
    int ppos = 0;

    for (int i = 1; i < length; i++)
    {
        for (int j = 0; j < i; j++)
            str[j] = perm_list[0];

        while (true)
        {
            uint32_t hashbase = crc32FromStringLen(str, i);
            
            for (int j = 0; j < perm_list.size(); j++)
            {
                str[i] = perm_list[j];
                uint32_t resulthash = updateCrc32String(hashbase, &str[i], 1);

                if (resulthash == hash)
                    std::cout << str << " 0x" << std::hex << hashbase << " 0x" << resulthash << std::endl;
            }
            
            bool next = false;

            for (int l = i - 1; l >= 0; l--)
            {
                auto it = std::find(perm_list.begin(), perm_list.end(), str[l]);

                if (it == perm_list.end())
                {
                    std::cout << "error";
                }

                ++it;
                if (it != perm_list.end())
                {
                    str[l] = *it;

                    for (int j = l + 1; j < i; j++)
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

std::atomic<bool> iothreadShouldContinue;

/**/
void io_thread(std::chrono::time_point<std::chrono::high_resolution_clock> start)
{
    std::string buffer;
    std::string temp;
    temp.reserve(64);
    buffer.reserve(2048);

    char btmp[32] = { 0 };

    while (iothreadShouldContinue)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        
        {
            //if (io_collisions.size() > 0)
            {
                buffer.clear();

                for (auto nxt = collisions.next(); nxt.second; nxt = collisions.next())
                {
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

                    temp = io.str;
                    std::reverse(temp.begin(), temp.end());
                    buffer += temp;
                    buffer += "\n";
                }

                //io_collisions.clear();
            }
        }

        if (buffer.size() > 0)
        {
            std::cout << buffer;
            buffer.clear();
        }
    }
}

int main(int argc, char *argv[])
{
    int max_length = 31;

    if (argc > 1)
    {
        max_length = 7;
        std::cout << "max_length = " << max_length << std::endl;
    }

    memset(perm_list_roundup.get(), 0, round_up8(perm_list));
    memcpy(perm_list_roundup.get(), perm_list.c_str(), perm_list.size());

    iothreadShouldContinue = true;

    int threads = std::thread::hardware_concurrency();
    std::vector<std::thread> thrds;
    thrds.reserve(threads);

    auto starttime = std::chrono::high_resolution_clock::now();
    std::thread iothred(io_thread, starttime);

    std::cout << "TIME                      THREAD    HASH        STRING" << std::endl;
    for (int i = 0; i < threads; i++)
    {
        thrds.push_back(std::thread(findcollisions_mthread, 0xDE4B237D, max_length, i + 1));
    }

    //findcollisions(0xDE4B237D, 16, "ABCDEFGHIJKLMNOPQRTUVWXYZ");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    for (auto &t : thrds)
    {
        if (t.joinable())
            t.join();
    }

    iothreadShouldContinue = false;

    if (iothred.joinable())
        iothred.join();
    
    return 0;
}

