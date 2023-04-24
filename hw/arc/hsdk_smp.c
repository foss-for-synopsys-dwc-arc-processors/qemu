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
#include "qemu/units.h"
#include "qapi/error.h"
#include "boot.h"
#include "hw/boards.h"
#include "hw/char/serial.h"
#include "exec/address-spaces.h"
#include "sysemu/reset.h"
#include "sysemu/sysemu.h"
#include "hw/arc/cpudevs.h"
#include "hw/sysbus.h"
#include "arconnect.h"

#define HSDK_RAM_BASE      0x80000000
#define HSDK_IO_BASE       0xf0000000
#define HSDK_IO_SIZE       0x10000000

/* UART */
#define HSDK_UART_NUMBER   1
#define HSDK_UART_OFFSET   0x5000
#define HSDK_UART_BASE     (HSDK_IO_BASE + HSDK_UART_OFFSET)
#define HSDK_UART_IRQ      (EXT_IRQ + 6)
#define HSDK_UART_SIZE     0x2000

/* Test Device */
#define HSDK_TEST_OFFSET   0xa00000
#define HSDK_TEST_BASE     (HSDK_IO_BASE + HSDK_TEST_OFFSET)

static void hsdk_init(MachineState *machine)
{
    static struct arc_boot_info boot_info;
    unsigned int smp_cpus = machine->smp.cpus;
    ARCCPU *cpu = NULL, *boot_cpu = NULL;
    MemoryRegion *system_memory = get_system_memory();
    MemoryRegion *system_ram;
    MemoryRegion *system_ram0;
    MemoryRegion *system_io;
    int n;

    boot_info.ram_start = HSDK_RAM_BASE;
    boot_info.ram_size = machine->ram_size;
    boot_info.kernel_filename = machine->kernel_filename;
    boot_info.kernel_cmdline = machine->kernel_cmdline;

    for (n = 0; n < smp_cpus; n++) {
        Object *cpuobj = object_new(machine->cpu_type);
        cpu = ARC_CPU(cpuobj);
        if (cpu == NULL) {
            fprintf(stderr, "Unable to find CPU definition!\n");
            exit(1);
        }

        /* Set the initial CPU properties. */
        cpu->core_id = n;
        qdev_realize(DEVICE(cpuobj), NULL, &error_fatal);
        object_unref(cpuobj);

	/* Initialize internal devices. */
        cpu_arc_pic_init(cpu);
        cpu_arc_clock_init(cpu);
        qemu_register_reset(arc_cpu_reset, cpu);

	if (!n)
            boot_cpu = cpu;
    }

    /* Init system DDR */
    system_ram = g_new(MemoryRegion, 1);
    memory_region_init_ram(system_ram, NULL, "arc.ram", machine->ram_size,
                           &error_fatal);
    memory_region_add_subregion(system_memory, HSDK_RAM_BASE, system_ram);

    system_ram0 = g_new(MemoryRegion, 1);
    memory_region_init_ram(system_ram0, NULL, "arc.ram0", 0x1000000,
                           &error_fatal);
    memory_region_add_subregion(system_memory, 0, system_ram0);

    /* Init IO area */
    system_io = g_new(MemoryRegion, 1);
    memory_region_init_io(system_io, NULL, NULL, NULL, "arc.io",
                          HSDK_IO_SIZE);
    memory_region_add_subregion(system_memory, HSDK_IO_BASE, system_io);

    for (n = 0; n < HSDK_UART_NUMBER; n++) {
        serial_mm_init(system_io, HSDK_UART_OFFSET + HSDK_UART_SIZE * n, 2,
                       boot_cpu->env.irq[HSDK_UART_IRQ + n], 115200,
		       serial_hd(n), DEVICE_NATIVE_ENDIAN);
    }

    sysbus_create_simple("snps-plat-test", HSDK_TEST_BASE, NULL);
    arc_load_kernel(boot_cpu, &boot_info);
}

static void hsdk_machine_init(MachineClass *mc)
{
    mc->desc = "ARC HSDK SMP Machine";
    mc->init = hsdk_init;
    mc->min_cpus = 4;
    mc->max_cpus = 4;
    mc->default_cpus = 4;
    mc->default_ram_size = 2 * GiB;
    mc->default_cpu_type = ARC_CPU_TYPE_NAME("archs");
}

DEFINE_MACHINE("hsdk", hsdk_machine_init)


/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
