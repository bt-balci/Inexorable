#include <stdint.h>
#include <stdarg.h>

#define COM1 0x402

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
    for (uint32_t i = 0; str[i] != 0; i++)
        serial_putc(str[i]);
}

static void serial_putn(uint32_t value, int base, int width, char pad)
{
    char buffer[32];
    int i = 0;

    if (value == 0) {
        buffer[i++] = '0';
    } else {
        while (value > 0) {
            int digit = value % base;
            buffer[i++] = (digit < 10) ? '0' + digit : 'A' + digit - 10;
            value /= base;
        }
    }

    while (i < width) {
        serial_putc(pad);
        width--;
    }

    while (i > 0)
        serial_putc(buffer[--i]);
}

static void serial_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            serial_putc(*p);
            continue;
        }

        if (*(p + 1) == '%') {
            serial_putc('%');
            p++;
            continue;
        }

        int width = 0;
        while (*(p + 1) >= '0' && *(p + 1) <= '9') {
            width = width * 10 + (*(p + 1) - '0');
            p++;
        }

        p++;

        switch (*p) {
            case 's': {
                char *s = va_arg(args, char*);
                serial_write(s ? s : "(null)");
                break;
            }
            case 'c': {
                int c = va_arg(args, int);
                serial_putc((char)c);
                break;
            }
            case 'd':
            case 'i': {
                int d = va_arg(args, int);
                if (d < 0) {
                    serial_putc('-');
                    d = -d;
                }
                serial_putn((uint32_t)d, 10, width, '0');
                break;
            }
            case 'u': {
                unsigned int u = va_arg(args, unsigned int);
                serial_putn(u, 10, width, '0');
                break;
            }
            case 'x':
            case 'X': {
                unsigned int x = va_arg(args, unsigned int);
                serial_putn(x, 16, width, '0');
                break;
            }
            case 'p': {
                void *ptr = va_arg(args, void*);
                serial_write("0x");
                int digits = 2 * sizeof(uintptr_t);
                serial_putn((uintptr_t)ptr, 16, digits, '0');
                break;
            }
            default:
                serial_putc('%');
                if (width > 0)
                    serial_putn((uint32_t)width, 10, 0, '0');
                serial_putc(*p);
                break;
        }
    }

    va_end(args);
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

static void print_oem_id(const uint8_t *oem_id)
{
    for (int i = 0; i < 6; i++)
        serial_putc((char)oem_id[i]);
}

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
    int count, i;

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

void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info)
{
    uintptr_t rsdp_addr, madt_addr;
    uintptr_t pos, end;

    (void)multiboot_magic;

    rsdp_addr = find_rsdp((uintptr_t)multiboot_info);

    serial_printf("RSDP found at %p\n", (void*)rsdp_addr);

    if (rsdp_addr) {
        acpi_rsdp *rsdp = (acpi_rsdp *)rsdp_addr;
        serial_printf("RSDP OEM ID: ");
        print_oem_id(rsdp->oem_id);
        serial_printf("\n");

        madt_addr = find_madt(rsdp_addr);

        serial_printf("MADT found at %p\n", (void*)madt_addr);

        if (madt_addr) {
            acpi_madt *madt = (acpi_madt *)madt_addr;
            serial_printf("MADT OEM ID: ");
            print_oem_id(madt->header.oem_id);
            serial_printf("\n");

            pos = (uintptr_t)madt->entries;
            end = madt_addr + madt->header.length;

            while (pos + 2 <= end) {
                uint8_t type = *(uint8_t *)pos;
                uint8_t len  = *(uint8_t *)(pos + 1);
                if (len < 2)
                    break;

                if (type == 0) {
                    uint8_t uid     = *(uint8_t *)(pos + 2);
                    uint8_t apic_id = *(uint8_t *)(pos + 3);
                    serial_printf("LAPIC UID=%u APICID=%u\n", uid, apic_id);
                }
                else if (type == 9) {
                    uint32_t x2apic_id = *(uint32_t *)(pos + 4);
                    uint32_t acpi_id   = *(uint32_t *)(pos + 8);
                    serial_printf("x2APIC UID=%u APICID=%u\n", acpi_id, x2apic_id);
                }

                pos += len;
            }
        }
    }

    for (;;)
        __asm__ volatile ("hlt");
}