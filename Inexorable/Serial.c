#include "Serial.h"
#include <stdarg.h>

#define COM1_BASE 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void SerialInit(void) {
    outb(COM1_BASE + 1, 0x00); // Disable all interrupts
    outb(COM1_BASE + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(COM1_BASE + 0, 0x01); // Set divisor to 1 (lo byte) 115200 baud
    outb(COM1_BASE + 1, 0x00); //                  (hi byte)
    outb(COM1_BASE + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1_BASE + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_BASE + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

void SerialPutc(char c) {
    outb(COM1_BASE, (uint8_t)c);
}

void SerialPuts(const char *s) {
    if (!s) return;
    while (*s) {
        if (*s == '\n') SerialPutc('\r');
        SerialPutc(*s++);
    }
}

static void print_uint(uint64_t n, int base) {
    char buf[32];
    int i = 0;
    const char *digits = "0123456789abcdef";

    if (n == 0) {
        SerialPutc('0');
        return;
    }

    while (n > 0) {
        buf[i++] = digits[n % base];
        n /= base;
    }

    while (i > 0) {
        SerialPutc(buf[--i]);
    }
}

void SerialPrintHex(const char *prefix, uint64_t val) {
    if (prefix) SerialPuts(prefix);
    SerialPuts("0x");
    char buf[16];
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = (val >> (i * 4)) & 0xF;
        buf[15 - i] = (nibble < 10) ? ('0' + nibble) : ('a' + nibble - 10);
    }
    for (int i = 0; i < 16; i++) {
        SerialPutc(buf[i]);
    }
    SerialPutc('\n');
}

void SerialPrint(const char *fmt, ...) {
    if (!fmt) return;
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            if (*p == '\n') SerialPutc('\r');
            SerialPutc(*p);
            continue;
        }

        p++;
        switch (*p) {
            case 's': {
                const char *s = va_arg(args, const char *);
                SerialPuts(s ? s : "(null)");
                break;
            }
            case 'd': {
                int64_t d = va_arg(args, int64_t);
                if (d < 0) {
                    SerialPutc('-');
                    d = -d;
                }
                print_uint((uint64_t)d, 10);
                break;
            }
            case 'u': {
                uint64_t u = va_arg(args, uint64_t);
                print_uint(u, 10);
                break;
            }
            case 'x':
            case 'p': {
                uint64_t x = va_arg(args, uint64_t);
                SerialPuts("0x");
                print_uint(x, 16);
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                SerialPutc(c);
                break;
            }
            case '%': {
                SerialPutc('%');
                break;
            }
            default: {
                SerialPutc('%');
                SerialPutc(*p);
                break;
            }
        }
    }

    va_end(args);
}
