/*
 * QEMU PCI test device
 *
 * Copyright (c) 2022 Synopsys, Inc.
 * Author: Jose Abreu <joabreu@synopsys.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/qdev-properties.h"
#include "qemu/event_notifier.h"
#include "qemu/module.h"
#include "sysemu/kvm.h"
#include "qom/object.h"

#define SNPS_MAXSIZE        (4096 * 1024)
#define SNPS_MAX_BARS       10

typedef struct SnpsTestBar {
    uint32_t regs[SNPS_MAXSIZE];
    PCIDevice *pci_dev;
    void *d;
} SnpsTestBar;

typedef struct SnpsTestState {
    PCIDevice parent_obj;
    SnpsTestBar bars[SNPS_MAX_BARS];
    MemoryRegion mmio[SNPS_MAX_BARS];
    uint64_t membar_size;
    uint16_t vendor_id;
    uint16_t device_id;
    uint64_t bar_size;
    uint16_t n_bars;
} SnpsTestState;

static Property snps_test_properties[] = {
    DEFINE_PROP_SIZE("membar", SnpsTestState, membar_size, 0),
    DEFINE_PROP_UINT16("vendor-id", SnpsTestState, vendor_id, 0x16c3),
    DEFINE_PROP_UINT16("device-id", SnpsTestState, device_id, 0x0001),
    DEFINE_PROP_SIZE("size", SnpsTestState, bar_size, SNPS_MAXSIZE),
    DEFINE_PROP_UINT16("bars", SnpsTestState, n_bars, 1),
    DEFINE_PROP_END_OF_LIST(),
};

#define TYPE_SNPS_TEST      "snps-test"
OBJECT_DECLARE_SIMPLE_TYPE(SnpsTestState, SNPS_TEST)

static void
snps_test_reset(SnpsTestState *d)
{
    return;
}

static uint64_t
snps_test_read(void *opaque, hwaddr addr, unsigned size)
{
    SnpsTestBar *d = opaque;

    if (addr % 4)
        return 0;
    if ((addr / 4) >= SNPS_MAXSIZE)
        return 0;

    return d->regs[addr / 0x4];
}

static void
snps_test_write(void *opaque, hwaddr addr, uint64_t val,
                     unsigned size)
{
    SnpsTestBar *d = opaque;

    if (addr % 4)
        return;
    if ((addr / 4) >= SNPS_MAXSIZE)
        return;
    d->regs[addr / 0x4] = val;
}

static const MemoryRegionOps snps_test_mmio_ops = {
    .read = snps_test_read,
    .write = snps_test_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void snps_test_realize(PCIDevice *pci_dev, Error **errp)
{
    SnpsTestState *d = SNPS_TEST(pci_dev);
    uint8_t *pci_conf;
    int i;

    pci_conf = pci_dev->config;
    pci_set_word(pci_conf + PCI_VENDOR_ID, d->vendor_id);
    pci_set_word(pci_conf + PCI_SUBSYSTEM_VENDOR_ID, d->vendor_id);
    pci_set_word(pci_conf + PCI_DEVICE_ID, d->device_id);
    pci_set_word(pci_conf + PCI_SUBSYSTEM_ID, d->device_id);
    pci_conf[PCI_INTERRUPT_PIN] = 0x1; /* Int-A */

    for (i = 0; i < d->n_bars; i++) {
        d->bars[i].pci_dev = pci_dev;
        d->bars[i].d = d;
        memory_region_init_io(&d->mmio[i], OBJECT(d), &snps_test_mmio_ops,
                              &d->bars[i], "snps-test-mmio", d->bar_size);
        pci_register_bar(pci_dev, i, PCI_BASE_ADDRESS_SPACE_MEMORY, &d->mmio[i]);
    }
}

static void
snps_test_uninit(PCIDevice *dev)
{
    SnpsTestState *d = SNPS_TEST(dev);
    snps_test_reset(d);
}

static void qdev_snps_test_reset(DeviceState *dev)
{
    SnpsTestState *d = SNPS_TEST(dev);
    snps_test_reset(d);
}

static void snps_test_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->realize = snps_test_realize;
    k->exit = snps_test_uninit;
    k->vendor_id = 0x16c3;
    k->device_id = 0x0001;
    k->revision = 0x00;
    k->class_id = PCI_CLASS_OTHERS;
    dc->desc = "Synopsys Test Device";
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    dc->reset = qdev_snps_test_reset;
    device_class_set_props(dc, snps_test_properties);
}

static const TypeInfo snps_test_info = {
    .name           = TYPE_SNPS_TEST,
    .parent         = TYPE_PCI_DEVICE,
    .instance_size  = sizeof(SnpsTestState),
    .class_init     = snps_test_class_init,
    .interfaces = (InterfaceInfo[]) {
        { INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { },
    },
};

static void snps_test_register_types(void)
{
    type_register_static(&snps_test_info);
}
type_init(snps_test_register_types)
