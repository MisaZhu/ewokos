/*
 * pci.h: ECAM config-space access for the QEMU virt PCIe host bridge.
 *
 * vm.dts: pcie@10000000 is a pci-host-ecam-generic controller with its
 * 16MB ECAM window at 0x3f000000 (bus-range 0-15) and the 32-bit MMIO
 * window at 0x10000000. QEMU assigns BAR addresses at machine init, so
 * enumeration only has to read config space and enable decoding.
 */
#ifndef __ARCH_VIRT_PCI_H__
#define __ARCH_VIRT_PCI_H__

#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/ewokdef.h>

#define VIRT_PCI_ECAM_PHY  0x3f000000u
#define VIRT_PCI_ECAM_SIZE (16u * 1024u * 1024u)
#define VIRT_PCI_WIN_PHY   0x10000000u
#define VIRT_PCI_WIN_SIZE  (16u * 1024u * 1024u)

/* map the ECAM window and the 32-bit MMIO window; 0 = ok, -1 = no map.
   Idempotent. */
int virt_pci_init(void);

/* ECAM config space accessors (bus 0-15, off dword-aligned for 32) */
uint32_t virt_pci_cfg_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off);
uint16_t virt_pci_cfg_read16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off);
void virt_pci_cfg_write16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint16_t val);

/* scan bus 0 for the first xHCI controller (class 0x0C, subclass 0x03,
   prog-if 0x30): enables memory + bus-master decoding and returns the
   BAR0 physical address (inside the MMIO window), or 0 when none. */
uint32_t virt_pci_find_xhci(void);

/* virtual base covering the MMIO window (valid after virt_pci_init):
   xHCI registers of a device whose BAR0 == phy live at
   virt_pci_win_base() + (phy - VIRT_PCI_WIN_PHY) */
ewokos_addr_t virt_pci_win_base(void);

#endif /* __ARCH_VIRT_PCI_H__ */
