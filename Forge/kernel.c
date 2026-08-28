#include <stdint.h>
#define COM1 0x3F8U

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a" (value), "Nd" (port));
}

static void serial_putc(char c)
{
    outb(COM1, (uint8_t)c);
}

static void serial_write(const char *str)
{
    for (uint32_t i = 0; str[i] != 0; i++
        serial_putc(str[i]]);
}

static void serial_hex8(uint8_t value)
{
    static const char d[] = "0123456789ABCDEF";
    char b[3] = { d[(value >> 4) & 0xF], d[value & 0xF], '\0' };
    serial_write(b);
}

static void serial_hex32(uint32_t value)
{
    static const char d[] = "0123456789ABCDEF";
    char b[9];
    int i;
    for (i = 7; i >= 0; i--) {
        b[i] = d[value & 0xF];
        value >>= 4;
    }
    b[8] = '\0';
    serial_write(b);
}

typedef struct {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) multiboot_tag;

typedef struct {
    uint8_t signature[8];
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
} __attribute__((packed)) acpi_rsdp;

typedef struct {
    uint8_t signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt;

typedef struct {
    acpi_sdt header;
    uint32_t entries[];
} __attribute__((packed)) acpi_rsdt;

typedef struct {
    acpi_sdt header;
    uint64_t entries[];
} __attribute__((packed)) acpi_xsdt;

typedef struct {
    acpi_sdt header;
    uint32_t local_apic_address;
    uint32_t flags;
    uint8_t entries[];
} __attribute__((packed)) acpi_madt;

static uintptr_t find_rsdp(uintptr_t mb_info)
{
    uintptr_t end = mb_info + *(uint32_t *)mb_info;
    uintptr_t pos = mb_info + 8;

    while (pos + 8 <= end) {
        multiboot_tag *tag = (multiboot_tag *)pos;
        if (tag->type == 0)
            break;
        if (tag->type == 14)
            return *(uint32_t *)(pos + 8);
        if (tag->type == 15)
            return (uintptr_t)*(uint64_t *)(pos + 8);
        pos = (pos + tag->size + 7) & ~(uintptr_t)7;
    }
    return 0;
}

static uintptr_t find_madt(uintptr_t rsdp_addr)
{
    acpi_rsdp *rsdp = (acpi_rsdp *)rsdp_addr;
    int count;
    int i;

    if (rsdp->revision >= 2 && rsdp->xsdt_address) {
        acpi_xsdt *xsdt = (acpi_xsdt *)(uintptr_t)rsdp->xsdt_address;
        count = (xsdt->header.length - sizeof(acpi_sdt)) / 8;
        for (i = 0; i < count; i++) {
            uintptr_t entry = (uintptr_t)xsdt->entries[i];
            acpi_sdt *t = (acpi_sdt *)entry;
            if (t->signature[0] == 'A' && t->signature[1] == 'P' &&
                t->signature[2] == 'I' && t->signature[3] == 'C')
                return entry;
        }
    } else {
        acpi_rsdt *rsdt = (acpi_rsdt *)(uintptr_t)rsdp->rsdt_address;
        count = (rsdt->header.length - sizeof(acpi_sdt)) / 4;
        for (i = 0; i < count; i++) {
            uintptr_t entry = rsdt->entries[i];
            acpi_sdt *t = (acpi_sdt *)entry;
            if (t->signature[0] == 'A' && t->signature[1] == 'P' &&
                t->signature[2] == 'I' && t->signature[3] == 'C')
                return entry;
        }
    }
    return 0;
}

static void print_oem_id(uint8_t *id)
{
    int i;
    for (i = 0; i < 6; i++)
        serial_putc(id[i]);
}

void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info)
{
    uintptr_t rsdp_addr;
    uintptr_t madt_addr;
    uintptr_t pos;
    uintptr_t end;

    (void)multiboot_magic;

    serial_write("Hello World\n");

    rsdp_addr = find_rsdp((uintptr_t)multiboot_info);
    if (rsdp_addr) {
        acpi_rsdp *rsdp = (acpi_rsdp *)rsdp_addr;
        serial_write("RSDP OEM ID: ");
        print_oem_id(rsdp->oem_id);
        serial_write("\n");

        madt_addr = find_madt(rsdp_addr);
        if (madt_addr) {
            acpi_madt *madt = (acpi_madt *)madt_addr;
            serial_write("MADT OEM ID: ");
            print_oem_id(madt->header.oem_id);
            serial_write("\n");

            pos = (uintptr_t)madt->entries;
            end = madt_addr + madt->header.length;
            while (pos + 2 <= end) {
                uint8_t type = *(uint8_t *)pos;
                uint8_t len = *(uint8_t *)(pos + 1);
                if (len < 2)
                    break;
                if (type == 0) {
                    serial_write("LAPIC UID=");
                    serial_hex8(*(uint8_t *)(pos + 2));
                    serial_write(" APICID=");
                    serial_hex8(*(uint8_t *)(pos + 3));
                    serial_write("\n");
                } else if (type == 9) {
                    serial_write("x2APIC UID=");
                    serial_hex32(*(uint32_t *)(pos + 4));
                    serial_write(" APICID=");
                    serial_hex32(*(uint32_t *)(pos + 8));
                    serial_write("\n");
                }
                pos += len;
            }
        }
    }

    for (;;)
        __asm__ volatile ("hlt");
}
