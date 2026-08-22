/*
 * Intel ACPI Component Architecture
 * Template for [APIC] ACPI Table (static data table)
 * Format: [ByteLength]  FieldName : HexFieldValue
 */
[0004]                          Signature : "APIC"    [Multiple APIC Description Table (MADT)]
[0004]                       Table Length : 00000078
[0001]                           Revision : 07
[0001]                           Checksum : 00
[0006]                             Oem ID : "BALCI "
[0008]                       Oem Table ID : "BTBTBTBT"
[0004]                       Oem Revision : 00000001
[0004]                    Asl Compiler ID : "INTL"
[0004]              Asl Compiler Revision : 20230628

[0004]                 Local Apic Address : 00000000
[0004]              Flags (decoded below) : 00000001
                      PC-AT Compatibility : 1

[0001]                      Subtable Type : 01 [I/O APIC]
[0001]                             Length : 0C
[0001]                        I/O Apic ID : 01
[0001]                           Reserved : 00
[0004]                            Address : 00000000
[0004]                          Interrupt : 00000000

[0001]                      Subtable Type : 09 [Processor Local x2APIC]
[0001]                             Length : 10
[0002]                           Reserved : 0000
[0004]                Processor x2Apic ID : 00000000
[0004]              Flags (decoded below) : 00000001
                        Processor Enabled : 1
[0004]                      Processor UID : 00000000

[0001]                      Subtable Type : 09 [Processor Local x2APIC]
[0001]                             Length : 10
[0002]                           Reserved : 0000
[0004]                Processor x2Apic ID : 00000001
[0004]              Flags (decoded below) : 00000001
                        Processor Enabled : 1
[0004]                      Processor UID : 00000001

[0001]                      Subtable Type : 09 [Processor Local x2APIC]
[0001]                             Length : 10
[0002]                           Reserved : 0000
[0004]                Processor x2Apic ID : 00000002
[0004]              Flags (decoded below) : 00000001
                        Processor Enabled : 1
[0004]                      Processor UID : 00000002

[0001]                      Subtable Type : 09 [Processor Local x2APIC]
[0001]                             Length : 10
[0002]                           Reserved : 0000
[0004]                Processor x2Apic ID : 00000003
[0004]              Flags (decoded below) : 00000001
                        Processor Enabled : 1
[0004]                      Processor UID : 00000003