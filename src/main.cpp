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
#include "crc32.h"

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
std::mutex printmutex;

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

void findcollisions_mthread(uint32_t hash, int length, std::string perm_list, std::chrono::time_point<std::chrono::high_resolution_clock> start)
{
    if (perm_list.size() == 0)
        return;

    std::array<uint32_t, 64> hashbylen;
    char str[128] = { 0 };

    std::sort(perm_list.begin(), perm_list.end());
    int ppos = 0;

    permdata pd = assignthreadnewperm(0, 0, perm_list);
    /*{
        std::lock_guard<std::mutex> lck(printmutex);
        std::cout << pd.len << "  " << pd.perm << std::endl;
    }*/

    for (int i = pd.len; i < length; i++)
    {
        i = pd.len;

        for (int j = 0; j < i; j++)
            str[j] = perm_list[0];
        
        str[0] = perm_list[pd.perm];

        while (true)
        {
            uint32_t hashbase = crc32FromStringLen(str, i);
            
            for (int j = 0; j < perm_list.size(); j++)
            {
                str[i] = perm_list[j];
                uint32_t resulthash = updateCrc32String(hashbase, &str[i], 1);

                if (resulthash == hash)
                {
                    auto nowtime = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> diff = nowtime-start;

                    std::lock_guard<std::mutex> lck(printmutex);
                    std::cout << diff.count() << "      " << str << "\n";
                }
            }
            
            bool next = false;

            for (int l = i - 1; l >= 0; l--)
            {
                if (l == 0)
                {
                    pd = assignthreadnewperm(i, 0, perm_list);
                    break;
                }

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

/**/

int main(int argc, char *argv[])
{
    auto starttime = std::chrono::high_resolution_clock::now();
    int threads = std::thread::hardware_concurrency();
    std::vector<std::thread> thrds;
    thrds.reserve(threads);

    std::cout << "TIME          STRING" << std::endl;
    for (int i = 0; i < threads; i++)
    {
        thrds.push_back(std::thread(findcollisions_mthread, 0xDE4B237D, 16, "ABCDEFGHIJKLMNOPQRTUVWXYZ", starttime));
    }

    //findcollisions(0xDE4B237D, 16, "ABCDEFGHIJKLMNOPQRTUVWXYZ");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    for (auto &t : thrds)
    {
        if (t.joinable())
            t.join();
    }
    return 0;    
}

