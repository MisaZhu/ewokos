#ifndef EMULATOR_H
#define EMULATOR_H

#include "CPU.h"
#include "PPU.h"
#include "MainBus.h"
#include "PictureBus.h"
#include "Controller.h"

namespace sn
{
    const int NESVideoWidth = ScanlineVisibleDots;
    const int NESVideoHeight = VisibleScanlines;

    class Emulator
    {
    public:
        Emulator();
		bool loadGame(char* rom_path);
		void setFrameBuffer(int w, int h, void* buf);
        void run(int cycle);
        void setKey(uint32_t key);

    private:
        void DMA(Byte page);

        MainBus m_bus;
        PictureBus m_pictureBus;
        CPU m_cpu;
        PPU m_ppu;
        Cartridge m_cartridge;
        Mapper *m_mapper;

        Controller m_controller1, m_controller2;

        VirtualScreen m_emulatorScreen;
        float m_screenScale;
    };
}
#endif // EMULATOR_H
