/*
 * pci.c: ECAM config-space enumeration for the QEMU virt PCIe host.
 *
 * Both windows are mapped once at fixed offsets above the shared mmio
 * virtual base: SYS_MEM_MAP accepts any phy_base (the virt kernel's
 * whitelist only bounds the size), so the mapping does not depend on
 * the 64MB mmio window declared in hw_info_arch.c.
 */
#include <arch/virt/pci.h>
#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/ewokdef.h>
#include <ewoksys/mmio.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>
#include <ewoksys/klog.h>

#define PCI_CMD_MEM_ENABLE 0x0002
#define PCI_CMD_BUS_MASTER 0x0004

#define PCI_CLASS_SERIAL_BUS 0x0C
#define PCI_SUBCLASS_USB     0x03
#define PCI_PROGIF_XHCI      0x30

/* offsets above sysinfo.mmio.v_base; the main mmio window occupies
   [0, 64MB), keep clear of it */
#define PCI_ECAM_VOFF 0x30000000u
#define PCI_WIN_VOFF  0x08000000u

static ewokos_addr_t _ecam_base = 0;
static ewokos_addr_t _win_base = 0;

int virt_pci_init(void) {
    sys_info_t sysinfo;

    if (_ecam_base != 0 && _win_base != 0) {
        return 0;
    }
    if (sys_get_sys_info(&sysinfo) != 0) {
        return -1;
    }
    _ecam_base = sysinfo.mmio.v_base + PCI_ECAM_VOFF;
    _win_base = sysinfo.mmio.v_base + PCI_WIN_VOFF;

    if (syscall3(SYS_MEM_MAP, _ecam_base, VIRT_PCI_ECAM_PHY,
            VIRT_PCI_ECAM_SIZE) != _ecam_base) {
        klog("virt_pci: ecam map failed\n");
        _ecam_base = 0;
        _win_base = 0;
        return -1;
    }
    if (syscall3(SYS_MEM_MAP, _win_base, VIRT_PCI_WIN_PHY,
            VIRT_PCI_WIN_SIZE) != _win_base) {
        klog("virt_pci: mmio window map failed\n");
        _ecam_base = 0;
        _win_base = 0;
        return -1;
    }
    return 0;
}

ewokos_addr_t virt_pci_win_base(void) {
    return _win_base;
}

static ewokos_addr_t cfg_addr(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off) {
    return _ecam_base +
            ((ewokos_addr_t)bus << 20) +
            ((ewokos_addr_t)dev << 15) +
            ((ewokos_addr_t)func << 12) +
            (off & 0xFFu);
}

uint32_t virt_pci_cfg_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off) {
    if (_ecam_base == 0) {
        return 0xFFFFFFFFu;
    }
    return get32(cfg_addr(bus, dev, func, off & ~0x3u));
}

uint16_t virt_pci_cfg_read16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off) {
    uint32_t v = virt_pci_cfg_read32(bus, dev, func, off);
    return (uint16_t)((v >> ((off & 0x2u) * 8u)) & 0xFFFFu);
}

void virt_pci_cfg_write16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint16_t val) {
    uint32_t shift = (off & 0x2u) * 8u;
    uint32_t reg = virt_pci_cfg_read32(bus, dev, func, off);
    if (_ecam_base == 0) {
        return;
    }
    reg &= ~(0xFFFFu << shift);
    reg |= ((uint32_t)val << shift);
    put32(cfg_addr(bus, dev, func, off & ~0x3u), reg);
}

uint32_t virt_pci_find_xhci(void) {
    if (_ecam_base == 0) {
        return 0;
    }
    for (uint8_t dev = 0; dev < 32; ++dev) {
        for (uint8_t func = 0; func < 8; ++func) {
            uint32_t id = virt_pci_cfg_read32(0, dev, func, 0x00);
            uint32_t class_reg;
            uint16_t cmd;
            uint32_t bar0;

            if ((id & 0xFFFFu) == 0xFFFFu) {
                if (func == 0) {
                    break; /* nothing here, skip remaining functions */
                }
                continue;
            }
            class_reg = virt_pci_cfg_read32(0, dev, func, 0x08);
            if ((class_reg >> 24) != PCI_CLASS_SERIAL_BUS ||
                    ((class_reg >> 16) & 0xFFu) != PCI_SUBCLASS_USB ||
                    ((class_reg >> 8) & 0xFFu) != PCI_PROGIF_XHCI) {
                continue;
            }
            /* BAR0 must be a 32-bit memory BAR inside the window */
            bar0 = virt_pci_cfg_read32(0, dev, func, 0x10);
            if ((bar0 & 0x1u) != 0 || (bar0 & ~0xFu) == 0) {
                continue;
            }
            bar0 &= ~0xFu;
            if (bar0 < VIRT_PCI_WIN_PHY ||
                    bar0 >= VIRT_PCI_WIN_PHY + VIRT_PCI_WIN_SIZE) {
                klog("virt_pci: xhci bar0=%08x outside mapped window\n", bar0);
                continue;
            }
            cmd = virt_pci_cfg_read16(0, dev, func, 0x04);
            cmd |= PCI_CMD_MEM_ENABLE | PCI_CMD_BUS_MASTER;
            virt_pci_cfg_write16(0, dev, func, 0x04, cmd);
            klog("virt_pci: xhci at %02x:%02x.%x bar0=%08x\n",
                    0, dev, func, bar0);
            return bar0;
        }
    }
    return 0;
}
