/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "boot.h"
#include "hw/boards.h"
#include "hw/char/serial.h"
#include "exec/address-spaces.h"
#include "sysemu/reset.h"
#include "sysemu/sysemu.h"
#include "hw/arc/cpudevs.h"
#include "hw/pci-host/gpex.h"
#include "hw/sysbus.h"

#define VIRT_RAM_BASE      0x80000000
#define VIRT_RAM_SIZE      0x80000000
#define VIRT_IO_BASE       0xf0000000
#define VIRT_IO_SIZE       0x10000000
#define VIRT_UART0_OFFSET  0x0
#define VIRT_UART0_IRQ     24

/* VirtIO */
#define VIRT_VIRTIO_NUMBER 5
#define VIRT_VIRTIO_OFFSET 0x100000
#define VIRT_VIRTIO_BASE   (VIRT_IO_BASE + VIRT_VIRTIO_OFFSET)
#define VIRT_VIRTIO_SIZE   0x2000
#define VIRT_VIRTIO_IRQ    31

/* PCI */
#define VIRT_PCI_ECAM_BASE 0xe0000000
#define VIRT_PCI_ECAM_SIZE 0x01000000
#define VIRT_PCI_MMIO_BASE 0xd0000000
#define VIRT_PCI_MMIO_SIZE 0x10000000
#define VIRT_PCI_PIO_BASE  0xc0000000
#define VIRT_PCI_PIO_SIZE  0x00004000
#define PCIE_IRQ           40  /* IRQs 40-43 as GPEX_NUM_IRQS=4 */

static void create_pcie(ARCCPU *cpu)
{
    hwaddr base_ecam = VIRT_PCI_ECAM_BASE;
    hwaddr size_ecam = VIRT_PCI_ECAM_SIZE;
    hwaddr base_pio  = VIRT_PCI_PIO_BASE;
    hwaddr size_pio  = VIRT_PCI_PIO_SIZE;
    hwaddr base_mmio = VIRT_PCI_MMIO_BASE;
    hwaddr size_mmio = VIRT_PCI_MMIO_SIZE;

    MemoryRegion *ecam_alias;
    MemoryRegion *ecam_reg;
    MemoryRegion *pio_alias;
    MemoryRegion *pio_reg;
    MemoryRegion *mmio_alias;
    MemoryRegion *mmio_reg;

    DeviceState *dev;
    int i;

    dev = qdev_new(TYPE_GPEX_HOST);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    /* Map only the first size_ecam bytes of ECAM space. */
    ecam_alias = g_new0(MemoryRegion, 1);
    ecam_reg = sysbus_mmio_get_region(SYS_BUS_DEVICE(dev), 0);
    memory_region_init_alias(ecam_alias, OBJECT(dev), "pcie-ecam",
                             ecam_reg, 0, size_ecam);
    memory_region_add_subregion(get_system_memory(), base_ecam, ecam_alias);

    /*
     * Map the MMIO window into system address space so as to expose
     * the section of PCI MMIO space which starts at the same base address
     * (ie 1:1 mapping for that part of PCI MMIO space visible through
     * the window).
     */
    mmio_alias = g_new0(MemoryRegion, 1);
    mmio_reg = sysbus_mmio_get_region(SYS_BUS_DEVICE(dev), 1);
    memory_region_init_alias(mmio_alias, OBJECT(dev), "pcie-mmio",
                             mmio_reg, base_mmio, size_mmio);
    memory_region_add_subregion(get_system_memory(), base_mmio, mmio_alias);

    /* Map IO port space. */
    pio_alias = g_new0(MemoryRegion, 1);
    pio_reg = sysbus_mmio_get_region(SYS_BUS_DEVICE(dev), 2);
    memory_region_init_alias(pio_alias, OBJECT(dev), "pcie-pio",
                             pio_reg, 0, size_pio);
    memory_region_add_subregion(get_system_memory(), base_pio, pio_alias);

    /* Connect IRQ lines. */
    for (i = 0; i < GPEX_NUM_IRQS; i++) {
        sysbus_connect_irq(SYS_BUS_DEVICE(dev), i, cpu->env.irq[PCIE_IRQ + i]);
        gpex_set_irq_num(GPEX_HOST(dev), i, PCIE_IRQ + i);
    }
}

static void virt_init(MachineState *machine)
{
    static struct arc_boot_info boot_info;
    unsigned int smp_cpus = machine->smp.cpus;
    MemoryRegion *system_memory = get_system_memory();
    MemoryRegion *system_ram;
    MemoryRegion *system_io;
    ARCCPU *cpu = NULL;
    int n;

    boot_info.ram_start = VIRT_RAM_BASE;
    boot_info.ram_size = VIRT_RAM_SIZE;
    boot_info.kernel_filename = machine->kernel_filename;
    boot_info.kernel_cmdline = machine->kernel_cmdline;

    for (n = 0; n < smp_cpus; n++) {
        cpu = ARC_CPU(cpu_create("archs-" TYPE_ARC_CPU));
        if (cpu == NULL) {
            fprintf(stderr, "Unable to find CPU definition!\n");
            exit(1);
        }

       /* Initialize internal devices. */
        cpu_arc_pic_init(cpu);
        cpu_arc_clock_init(cpu);

        qemu_register_reset(arc_cpu_reset, cpu);
    }

    /* Init system DDR */
    system_ram = g_new(MemoryRegion, 1);
    memory_region_init_ram(system_ram, NULL, "arc.ram", VIRT_RAM_SIZE,
                           &error_fatal);
    memory_region_add_subregion(system_memory, VIRT_RAM_BASE, system_ram);

    /* Init IO area */
    system_io = g_new(MemoryRegion, 1);
    memory_region_init_io(system_io, NULL, NULL, NULL, "arc.io",
                          VIRT_IO_SIZE);
    memory_region_add_subregion(system_memory, VIRT_IO_BASE, system_io);

    serial_mm_init(system_io, VIRT_UART0_OFFSET, 2,
                   cpu->env.irq[VIRT_UART0_IRQ], 115200, serial_hd(0),
                   DEVICE_NATIVE_ENDIAN);

    for (n = 0; n < VIRT_VIRTIO_NUMBER; n++) {
        sysbus_create_simple("virtio-mmio",
                             VIRT_VIRTIO_BASE + VIRT_VIRTIO_SIZE * n,
                             cpu->env.irq[VIRT_VIRTIO_IRQ + n]);
    }

    create_pcie(cpu);

    arc_load_kernel(cpu, &boot_info);
}

static void virt_machine_init(MachineClass *mc)
{
    mc->desc = "ARC Virtual Machine";
    mc->init = virt_init;
    mc->max_cpus = 1;
    mc->is_default = true;
}

DEFINE_MACHINE("virt", virt_machine_init)


/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
