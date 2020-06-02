#include <iostream>
#include <array>
#include <vector>
#include <cstdint>
#include <algorithm>
#include "crc32.h"

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
                uint32_t resulthash = updateCrc32String(hashbase, &str[i], sizeof(str[i]));
                std::cout << str << " 0x" << std::hex << hashbase << " 0x" << std::endl;
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

int main(int argc, char *argv[])
{
    findcollisions(0, 4, "ABCDEFGHIJKLMNOPQRTUVWXYZ");
    

    return 0;    
}

