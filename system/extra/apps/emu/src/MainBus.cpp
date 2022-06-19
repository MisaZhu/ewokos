#include "MainBus.h"
#include <cstring>
#include <stdio.h>

namespace sn
{
    MainBus::MainBus() :
        m_RAM(0x800, 0),
        m_mapper(nullptr)
    {
    }

    Byte MainBus::read(Address addr)
    {
        if (addr < 0x2000)
            return m_RAM[addr & 0x7ff];
        else if (addr < 0x4020)
        {
            if (addr < 0x4000) //PPU registers, mirrored
            {
                auto it = m_readCallbacks.find(static_cast<IORegisters>(addr & 0x2007));
                if (it != m_readCallbacks.end())
                    return (it -> second)();
                    //Second object is the pointer to the function object
                    //Dereference the function pointer and call it
                else
                    printf( "No read callback registered for I/O register at: %08x\n", addr );
            }
            else if (addr < 0x4018 && addr >= 0x4014) //Only *some* IO registers
            {
                auto it = m_readCallbacks.find(static_cast<IORegisters>(addr));
                if (it != m_readCallbacks.end())
                    return (it -> second)();
                    //Second object is the pointer to the function object
                    //Dereference the function pointer and call it
                else
                    printf( "No read callback registered for I/O register at: %08x\n" , addr );
            }
            else
                printf( "Read access attempt at: %08x\n" , addr );
        }
        else if (addr < 0x6000)
        {
            printf( "Expansion ROM read attempted. This is currently unsupported\n" );
        }
        else if (addr < 0x8000)
        {
            if (m_mapper->hasExtendedRAM())
            {
                return m_extRAM[addr - 0x6000];
            }
        }
        else
        {
            return m_mapper->readPRG(addr);
        }
        return 0;
    }

    void MainBus::write(Address addr, Byte value)
    {
        if (addr < 0x2000)
            m_RAM[addr & 0x7ff] = value;
        else if (addr < 0x4020)
        {
            if (addr < 0x4000) //PPU registers, mirrored
            {
                auto it = m_writeCallbacks.find(static_cast<IORegisters>(addr & 0x2007));
                if (it != m_writeCallbacks.end())
                    (it -> second)(value);
                    //Second object is the pointer to the function object
                    //Dereference the function pointer and call it
                //else
               //     printf( "No write callback registered for I/O register at: %08x\n", addr );
            }
            else if (addr < 0x4017 && addr >= 0x4014) //only some registers
            {
                auto it = m_writeCallbacks.find(static_cast<IORegisters>(addr));
                if (it != m_writeCallbacks.end())
                    (it -> second)(value);
                    //Second object is the pointer to the function object
                    //Dereference the function pointer and call it
                //else
                    //printf( "No write callback registered for I/O register at: %08x\n", addr);
            }
            //else
                //printf( "Write access attmept at: %08x\n" , addr );
        }
        else if (addr < 0x6000)
        {
            printf( "Expansion ROM access attempted. This is currently unsupported\n" );
        }
        else if (addr < 0x8000)
        {
            if (m_mapper->hasExtendedRAM())
            {
                m_extRAM[addr - 0x6000] = value;
            }
        }
        else
        {
            m_mapper->writePRG(addr, value);
        }
    }

    const Byte* MainBus::getPagePtr(Byte page)
    {
        Address addr = page << 8;
        if (addr < 0x2000)
            return &m_RAM[addr & 0x7ff];
        else if (addr < 0x4020)
        {
            printf( "Register address memory pointer access attempt\n" );
        }
        else if (addr < 0x6000)
        {
            printf( "Expansion ROM access attempted, which is unsupported\n" );
        }
        else if (addr < 0x8000)
        {
            if (m_mapper->hasExtendedRAM())
            {
                return &m_extRAM[addr - 0x6000];
            }
        }
        else
        {
        }
        return nullptr;
    }

    bool MainBus::setMapper(Mapper* mapper)
    {
        m_mapper = mapper;

        if (!mapper)
        {
            printf( "Mapper pointer is nullptr\n" );
            return false;
        }

        if (mapper->hasExtendedRAM())
            m_extRAM.resize(0x2000);

        return true;
    }

    bool MainBus::setWriteCallback(IORegisters reg, std::function<void(Byte)> callback)
    {
        if (!callback)
        {
            printf( "callback argument is nullptr\n" );
            return false;
        }
        return m_writeCallbacks.emplace(reg, callback).second;
    }

    bool MainBus::setReadCallback(IORegisters reg, std::function<Byte(void)> callback)
    {
        if (!callback)
        {
            printf( "callback argument is nullptr\n" );
            return false;
        }
        return m_readCallbacks.emplace(reg, callback).second;
    }

}
