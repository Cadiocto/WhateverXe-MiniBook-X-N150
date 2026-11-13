/*
 * CHUWI MiniBook X N150 — ES8326 I2C namespace visibility test
 * COMPILE/REVIEW ONLY. Do NOT install this AML in EFI.
 *
 * This intentionally contains only the known I2C address (0x19) and
 * the I2C1 controller path. GPIO power/reset/interrupt resources are
 * omitted until verified against the machine's actual ACPI tables.
 */
DefinitionBlock ("", "SSDT", 2, "CHUWI", "ES8326T", 0x00000002)
{
    External (\_SB.PC00.I2C1, DeviceObj)

    Scope (\_SB.PC00.I2C1)
    {
        Device (E832)
        {
            Name (_HID, "ESSX8326")
            Name (_CID, "ESSX8326")
            Name (_DDN, "ES8326 I2C visibility test")
            Name (_UID, One)

            Method (_STA, 0, NotSerialized)
            {
                Return (0x0F)
            }

            Name (_CRS, ResourceTemplate ()
            {
                I2cSerialBusV2 (0x0019, ControllerInitiated, 0x00061A80,
                    AddressingMode7Bit, "\\_SB.PC00.I2C1",
                    0x00, ResourceConsumer, , Exclusive)
            })
        }
    }
}
