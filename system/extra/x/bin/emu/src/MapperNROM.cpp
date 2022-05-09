#include "MapperNROM.h"
#include <stdio.h>

namespace sn
{
    MapperNROM::MapperNROM(Cartridge &cart) :
        Mapper(cart, Mapper::NROM)
    {
        if (cart.getROM().size() == 0x4000) //1 bank
        {
            m_oneBank = true;
        }
        else //2 banks
        {
            m_oneBank = false;
        }

        if (cart.getVROM().size() == 0)
        {
            m_usesCharacterRAM = true;
            m_characterRAM.resize(0x2000);
            printf( "Uses character RAM\n");
        }
        else
            m_usesCharacterRAM = false;
    }

    Byte MapperNROM::readPRG(Address addr)
    {
        if (!m_oneBank)
            return m_cartridge.getROM()[addr - 0x8000];
        else //mirrored
            return m_cartridge.getROM()[(addr - 0x8000) & 0x3fff];
    }

    void MapperNROM::writePRG(Address addr, Byte value)
    {
		(void)addr;
		(void)value;
    }

    const Byte* MapperNROM::getPagePtr(Address addr)
    {
        if (!m_oneBank)
            return &m_cartridge.getROM()[addr - 0x8000];
        else //mirrored
            return &m_cartridge.getROM()[(addr - 0x8000) & 0x3fff];
    }

    Byte MapperNROM::readCHR(Address addr)
    {
        if (m_usesCharacterRAM)
            return m_characterRAM[addr];
        else
            return m_cartridge.getVROM()[addr];
    }

    void MapperNROM::writeCHR(Address addr, Byte value)
    {
        if (m_usesCharacterRAM)
            m_characterRAM[addr] = value;
        else
            printf( "Read-only CHR memory write attempt at %08x\n",addr);
    }
}
