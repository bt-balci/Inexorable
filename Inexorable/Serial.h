#pragma once

#include <stdint.h>
#include <stddef.h>

void SerialInit(void);
void SerialPutc(char c);
void SerialPuts(const char *s);
void SerialPrint(const char *fmt, ...);
void SerialPrintHex(const char *prefix, uint64_t val);
