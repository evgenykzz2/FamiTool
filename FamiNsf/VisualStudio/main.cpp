#include <iostream>
#include <vector>
#include <stdint.h>
#include "../FamiNsf/src/FamiNsf.h"

int main()
{
    FILE* file = 0;
    //fopen_s(&file, "../test_music/Super Mario Bros. (1985-09-13)(Nintendo EAD)(Nintendo).nsf", "rb");
    //fopen_s(&file, "../test_music/Battletoads & Double Dragon - The Ultimate Team (1993-06)(Rare)(Tradewest).nsf", "rb");
    //fopen_s(&file, "../test_music/Robocop 3 (1992-08)(Probe)(Ocean).nsf", "rb");
    fopen_s(&file, "../test_music/DuckTales [Wanpaku Duck Yume Bouken] (1989-09)(Capcom).nsf", "rb");
    if (!file)
    {
        std::cout << "Can't load nsf file" << std::endl;
        return -1;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    std::vector<uint8_t> nsf(file_size);
    size_t read_size = fread(nsf.data(), 1, file_size, file);
    fclose(file);
    if (read_size != file_size)
    {
        std::cout << "Can't read nsf file" << std::endl;
        return -1;
    }

    FamiNsf nsf_player;
    nsf_player.Load(nsf.data(), nsf.size());
    nsf_player.Init(6, 0);

    //while (true)
    for (int i = 0; i < 60*60; ++i)
        nsf_player.PlayFrame();

    return 0;
}