#include <stdio.h>
#include <sys/time.h>

#include "Emulator.h"

namespace sn
{
    Emulator::Emulator() :
        m_cpu(m_bus),
        m_ppu(m_pictureBus, m_emulatorScreen)
    {
        if(!m_bus.setReadCallback(PPUSTATUS, [&](void) {return m_ppu.getStatus();}) ||
            !m_bus.setReadCallback(PPUDATA, [&](void) {return m_ppu.getData();}) ||
            !m_bus.setReadCallback(JOY1, [&](void) {return m_controller1.read();}) ||
            !m_bus.setReadCallback(JOY2, [&](void) {return m_controller2.read();}) ||
            !m_bus.setReadCallback(OAMDATA, [&](void) {return m_ppu.getOAMData();}))
        {
            printf( "Critical error: Failed to set I/O callbacks\n" );
        }


        if(!m_bus.setWriteCallback(PPUCTRL, [&](Byte b) {m_ppu.control(b);}) ||
            !m_bus.setWriteCallback(PPUMASK, [&](Byte b) {m_ppu.setMask(b);}) ||
            !m_bus.setWriteCallback(OAMADDR, [&](Byte b) {m_ppu.setOAMAddress(b);}) ||
            !m_bus.setWriteCallback(PPUADDR, [&](Byte b) {m_ppu.setDataAddress(b);}) ||
            !m_bus.setWriteCallback(PPUSCROL, [&](Byte b) {m_ppu.setScroll(b);}) ||
            !m_bus.setWriteCallback(PPUDATA, [&](Byte b) {m_ppu.setData(b);}) ||
            !m_bus.setWriteCallback(OAMDMA, [&](Byte b) {DMA(b);}) ||
            !m_bus.setWriteCallback(JOY1, [&](Byte b) {m_controller1.strobe(b); m_controller2.strobe(b);}) ||
            !m_bus.setWriteCallback(OAMDATA, [&](Byte b) {m_ppu.setOAMData(b);}))
        {
            printf( "Critical error: Failed to set I/O callbacks\n" );
        }

        m_ppu.setInterruptCallback([&](){ m_cpu.interrupt(CPU::NMI); });
    }

	bool Emulator::loadGame(char* rom_path){
     if (!m_cartridge.loadFromFile(rom_path))
            return false;

        m_mapper = Mapper::createMapper(static_cast<Mapper::Type>(m_cartridge.getMapper()),
                                        m_cartridge,
                                        [&](){ m_pictureBus.updateMirroring(); });
        if (!m_mapper)
        {
            printf( "Creating Mapper failed. Probably unsupported.\n" );
            return false;
        }

        if (!m_bus.setMapper(m_mapper) ||
            !m_pictureBus.setMapper(m_mapper))
            return false;

        m_cpu.reset();
        m_ppu.reset();
		return true;
	}

	void Emulator::setFrameBuffer(int w, int h, void* buf){
        m_emulatorScreen.update(buf, w,  h);
	}

    void Emulator::run(int cycle)
    {
        while (cycle--)
        {
            //PPU
            m_ppu.step();
            m_ppu.step();
            m_ppu.step();
            //CPU
            m_cpu.step();

        }
		//m_emulatorScreen.draw();
    }

    void Emulator::DMA(Byte page)
    {
        m_cpu.skipDMACycles();
        auto page_ptr = m_bus.getPagePtr(page);
        m_ppu.doDMA(page_ptr);
    }

    void Emulator::setKey(uint32_t key){
        m_controller1.setKeyStatus(key);
    }

}
