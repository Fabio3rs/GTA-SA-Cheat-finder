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

void findcollisions_mthread(uint32_t hash, int length, std::string perm_list, uintptr_t thread_id)
{
    if (perm_list.size() == 0)
        return;

    if (length > 127)
        length = 127;

    std::array<uint32_t, 128> hashbylen;
    char str[128] = { 0 };

    std::sort(perm_list.begin(), perm_list.end());

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

                if (resulthash == hash)
                {
                    // complete the string
                    str[i] = perm_list[j];

                    // Send to IO
                    auto nowtime = std::chrono::high_resolution_clock::now();
                    register_collision(hash, nowtime, thread_id, str, i + 1);
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
                    temp.append(12 - temp.size(), ' ');
                    temp += "  #";
                    temp += std::to_string(io.thread_id);
                    temp += "  ";
                    buffer += temp;

                    buffer.append(32 - temp.size(), ' ');

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
    int max_length = 16;

    if (argc > 1)
    {
        max_length = 7;
        std::cout << "max_length = " << max_length << std::endl;
    }

    iothreadShouldContinue = true;

    io_collisions.reserve(16);

    int threads = std::thread::hardware_concurrency();
    std::vector<std::thread> thrds;
    thrds.reserve(threads);

    auto starttime = std::chrono::high_resolution_clock::now();
    std::thread iothred(io_thread, starttime);

    std::cout << "TIME          THREAD            STRING" << std::endl;
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

