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
#include "crc32.h"

std::mutex printmutex;

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

std::string io_buffer;
std::vector<collision_data> io_collisions;

void register_collision(uint32_t hash, std::chrono::time_point<std::chrono::high_resolution_clock> when, uintptr_t id, const char *str, int strsize)
{
    std::lock_guard<std::mutex> lck(printmutex);
    io_collisions.push_back({});
    
    collision_data &col = io_collisions.back();
    col.hash = hash;
    col.when = when;
    col.thread_id = id;
    int size = strsize > sizeof(col.str)? (sizeof(col.str) - 1) : strsize;
    strncpy(col.str, str, size);
    col.str[size] = 0;
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

/*
Each thread process all the permutations with certain length starting with some letter
*/
permdata assignthreadnewperm(int len, int perm, const std::string &perm_list)
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

void findcollisions_mthread(uint32_t hash, int length, const std::string &perm_list, uintptr_t thread_id)
{
    if (perm_list.size() == 0)
        return;

    if (length > 31)
        length = 31;

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
    std::array<uint32_t, 87> cheatTable = { 0xDE4B237D, 0xB22A28D1, 0x5A783FAE, 0xEECCEA2B, 0x42AF1E28, 0x555FC201, 0x2A845345, 0xE1EF01EA, 0x771B83FC, 0x5BF12848,
        0x44453A17, 0xFCFF1D08, 0xB69E8532, 0x8B828076, 0xDD6ED9E9, 0xA290FD8C, 0x3484B5A7, 0x43DB914E, 0xDBC0DD65, 0xD08A30FE, 0x37BF1B4E,
        0xB5D40866, 0xE63B0D99, 0x675B8945, 0x4987D5EE, 0x2E8F84E8, 0x1A9AA3D6, 0xE842F3BC, 0x0D5C6A4E, 0x74D4FCB1, 0xB01D13B8, 0x66516EBC, 0x4B137E45,
        0x78520E33, 0x3A577325, 0xD4966D59, 0x5FD1B49D, 0xA7613F99, 0x1792D871, 0xCBC579DF, 0x4FEDCCFF, 0x44B34866, 0x2EF877DB, 0x2781E797,
        0x2BC1A045, 0xB2AFE368, 0xFA8DD45B, 0x8DED75BD, 0x1A5526BC, 0xA48A770B, 0xB07D3B32, 0x80C1E54B, 0x5DAD0087, 0x7F80B950, 0x6C0FA650, 0xF46F2FA4,
        0x70164385, 0x885D0B50, 0x151BDCB3, 0xADFA640A, 0xE57F96CE, 0x040CF761, 0xE1B33EB9, 0xFEDA77F7, 0x8CA870DD, 0x9A629401, 0xF53EF5A5,
        0xF2AA0C1D, 0xF36345A8, 0x8990D5E1, 0xB7013B1B, 0xCAEC94EE, 0x31F0C3CC, 0xB3B3E72A, 0xC25CDBFF, 0xD5CF4EFF, 0x680416B1, 0xCF5FDA18, 0xF01286E9,
        0xA841CC0A, 0x31EA09CF, 0xE958788A, 0x02C83A7C, 0xE49C3ED4, 0x171BA8CC, 0x86988DAE, 0x2BDD2FA1
    };

    std::sort(cheatTable.begin(), cheatTable.end());

    std::array<uint32_t, 8> hashr;
    
    std::fill(hashr.begin(), hashr.end(), 0u);
    const int CSIZE = (cheatTable.size() + hashr.size() / 2) / hashr.size();

    const uint32_t LAST = cheatTable[cheatTable.size() - 1];
    const uint32_t DIFF = (LAST - cheatTable[0]);
    const uint32_t START = cheatTable[0], DIVISOR = (DIFF / cheatTable.size());
    const uint32_t OPTSIZE = LAST / DIVISOR + 1;

    struct optliststruct
    {
        int pos;
        int end;

        optliststruct()
        {
            pos = -1;
            end = 0;
        }
    };

    std::vector<optliststruct> tblvec;

    tblvec.reserve(OPTSIZE);

    {
        for (int i = 0; i < OPTSIZE; i++)
        {
            tblvec.push_back({});
        }

        /*std::lock_guard<std::mutex> lck(printmutex);
        std::cout << std::hex << "BASE: " << cheatTable[0] << " MAX " << cheatTable[cheatTable.size() - 1] << " DIFF "
            << DIFF << "  " << DIVISOR << std::endl;*/

        int handle = 0;

        for (int i = 0, size = cheatTable.size(); i < size; i++)
        {
            uint32_t BASE = cheatTable[i] - START;

            auto &op = tblvec[BASE / DIVISOR];

            if (op.pos == -1)
            {
                op.pos = i;
                op.end = i;
            }
            
            op.end++;

            hashr[handle] |= cheatTable[i];

            if (i != 0 && i % CSIZE == 0)
            {
                ++handle;
            }
        }
    }

    std::array<uint32_t, 32> hashbylen;
    char str[32] = { 0 };

    uint32_t hashtotal = 0;

    for (uint32_t h : cheatTable)
    {
        hashtotal |= h;
    }

    //std::sort(perm_list.begin(), perm_list.end());

    permdata pd = assignthreadnewperm(0, 0, perm_list);

    for (int i = pd.len; i < length; /*i++*/)
    {
        i = pd.len;

        str[0] = perm_list[pd.perm];
        hashbylen[0] = crc32Char(perm_list[pd.perm]);

        for (int j = 1; j < i; j++)
        {
            str[j] = perm_list[0];
            hashbylen[j] = updateCrc32Char(hashbylen[j - 1], perm_list[0]);
        }

        int imone = i - 1;

        while (true)
        {
            uint32_t hashbase = hashbylen[imone]/*crc32FromStringLen(str, i)*/;
            
            for (int j = 0; j < perm_list.size(); j++)
            {
                uint32_t resulthash = updateCrc32Char(hashbase, perm_list[j]);

                if ((hashtotal & resulthash) == resulthash && resulthash >= cheatTable[0] && resulthash <= cheatTable[cheatTable.size() - 1])
                {
                    bool findval = false;
                    
                    {
                        uint32_t B = resulthash - START;
                        B /= DIVISOR;

                        if (tblvec[B].pos != -1)
                        {
                            for (int dc = tblvec[B].pos, end = tblvec[B].end; dc != end; dc++)
                            {
                                if (cheatTable[dc] == resulthash)
                                {
                                    findval = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (findval)
                    //if (resulthash == hash)
                    {
                        // complete the string
                        str[i] = perm_list[j];

                        // Send to IO
                        auto nowtime = std::chrono::high_resolution_clock::now();
                        register_collision(resulthash, nowtime, thread_id, str, i + 1);
                    }
                }
            }
            
            bool next = false;

            for (int l = i - 1; l >= 0; l--)
            {
                if (l == 0)
                {
                    pd = assignthreadnewperm(i, 0, perm_list);
                    i = pd.len;
                    break;
                }

                auto it = std::find(perm_list.begin(), perm_list.end(), str[l]);

                ++it;

                if (it != perm_list.end())
                {
                    str[l] = *it;
                    hashbylen[l] = updateCrc32Char(hashbylen[l - 1], *it);

                    for (int j = l + 1; j < i; j++)
                    {
                        str[j] = perm_list[0];
                        hashbylen[j] = updateCrc32Char(hashbylen[j - 1], perm_list[0]);
                    }
                    
                    next = true;
                    break;
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
            std::lock_guard<std::mutex> lck(printmutex);

            if (io_collisions.size() > 0)
            {
                buffer.clear();

                for (collision_data &io : io_collisions)
                {
                    temp.clear();
                    std::chrono::duration<double> diff = io.when - start;

                    temp += std::to_string(diff.count());
                    temp.append(14 - temp.size(), ' ');
                    temp += "  #";
                    temp += std::to_string(io.thread_id);
                    temp += "  ";
                    buffer += temp;
                    
                    buffer.append(24 - temp.size(), ' ');
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

                io_collisions.clear();
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

    iothreadShouldContinue = true;

    io_collisions.reserve(64);

    int threads = std::thread::hardware_concurrency();
    std::vector<std::thread> thrds;
    thrds.reserve(threads);

    auto starttime = std::chrono::high_resolution_clock::now();
    std::thread iothred(io_thread, starttime);

    std::cout << "TIME          THREAD    HASH        STRING" << std::endl;
    for (int i = 0; i < threads; i++)
    {
        thrds.push_back(std::thread(findcollisions_mthread, 0xDE4B237D, max_length, "ABCDEFGHIJKLMNOPQRTUVWXYZ", i + 1));
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

