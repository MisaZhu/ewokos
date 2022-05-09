#include "Cartridge.h"
#include <stdio.h>
#include <string.h>

namespace sn
{
    Cartridge::Cartridge() :
        m_nameTableMirroring(0),
        m_mapperNumber(0),
        m_extendedRAM(false)
    {

    }
    const std::vector<Byte>& Cartridge::getROM()
    {
        return m_PRG_ROM;
    }

    const std::vector<Byte>& Cartridge::getVROM()
    {
        return m_CHR_ROM;
    }

    Byte Cartridge::getMapper()
    {
        return m_mapperNumber;
    }

    Byte Cartridge::getNameTableMirroring()
    {
        return m_nameTableMirroring;
    }

    bool Cartridge::hasExtendedRAM()
    {
        return m_extendedRAM;
    }

    bool Cartridge::loadFromFile(char* path)
    {
        FILE *romFile = fopen(path, "r");
        if (!romFile)
        {
            printf("Could not open ROM file from path: %s\n", path);
            return false;
        }

        std::vector<Byte> header;
        printf("Reading ROM from path: %s\n" , path);

        //Header
		header.resize(0x10);
        if (!fread(reinterpret_cast<char*>(&header[0]), header.size(), 1, romFile))
        {
            printf("Reading iNES header failed.\n");
            return false;
        }
        if (strncmp(reinterpret_cast<char*>(&header[0]), "NES\x1A", 4))
        {
            printf("Not a valid iNES image. Magic number\n ");
            return false;
        }

        printf("Reading header, it dictates: \n");

        Byte banks = header[4];
        printf("16KB PRG-ROM Banks: %d\n" ,banks);
        if (!banks)
        {
            printf("ROM has no PRG-ROM banks. Loading ROM failed. \n");
            return false;
        }

        Byte vbanks = header[5];
        printf("8KB CHR-ROM Banks: %d\n" , vbanks);

        m_nameTableMirroring = header[6] & 0xB;
        printf("Name Table Mirroring: %d", m_nameTableMirroring);

        m_mapperNumber = ((header[6] >> 4) & 0xf) | (header[7] & 0xf0);
		printf("Mapper #: %d\n" ,m_mapperNumber);

        m_extendedRAM = header[6] & 0x2;
        printf("Extended (CPU) RAM: %d\n" , m_extendedRAM);

        if (header[6] & 0x4)
        {
            printf("Trainer is not supported.\n");
            return false;
        }

        if ((header[0xA] & 0x3) == 0x2 || (header[0xA] & 0x1))
        {
            printf("PAL ROM not supported.\n");
            return false;
        }
        else
			printf("ROM is NTSC compatible.\n");

        //PRG-ROM 16KB banks
        m_PRG_ROM.resize(0x4000 * banks);
        if (!fread(reinterpret_cast<char*>(&m_PRG_ROM[0]), 0x4000 * banks, 1, romFile))
        {
            printf("Reading PRG-ROM from image file failed. \n");
            return false;
        }

        //CHR-ROM 8KB banks
        if (vbanks)
        {
            m_CHR_ROM.resize(0x2000 * vbanks);
            if (!fread(reinterpret_cast<char*>(&m_CHR_ROM[0]), 0x2000 * vbanks, 1, romFile))
            {
                printf("Reading CHR-ROM from image file failed. \n");
                return false;
            }
        }
        else
            printf("Cartridge with CHR-RAM.\n");
        return true;
    }
}
