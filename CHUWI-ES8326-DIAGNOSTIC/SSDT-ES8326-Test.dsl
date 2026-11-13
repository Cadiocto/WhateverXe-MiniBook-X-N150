/*
 * CHUWI MiniBook X N150 — ES8326 ACPI visibility diagnostic
 * SOURCE-ONLY DRAFT. Generated AML is for review only; do NOT install in EFI.
 */
DefinitionBlock ("", "SSDT", 2, "CHUWI", "ES8326T", 0x00000001)
{
    External (\_SB.GPI0, DeviceObj)
    External (\_SB.PC00.I2C1, DeviceObj)
    External (\_SB.GNUM, MethodObj, 1)

    Scope (\_SB.PC00.I2C1)
    {
        Device (E832)
        {
            Name (_HID, "ESSX8326")
            Name (_CID, "ESSX8326")
            Name (_DDN, "ES8326 ACPI visibility test")
            Name (_UID, 2)
            Name (_ADR, Zero)
            Name (_DEP, Package () { \_SB.GPI0, \_SB.PC00.I2C1 })

            Method (_STA, 0, NotSerialized) { Return (0x0F) }

            Method (_CRS, 0, NotSerialized)
            {
                Name (SBFB, ResourceTemplate ()
                {
                    I2cSerialBusV2 (0x0019, ControllerInitiated, 0x00061A80,
                        AddressingMode7Bit, "\\_SB.PC00.I2C1",
                        0x00, ResourceConsumer, , Exclusive)
                })
                Name (SBFP, ResourceTemplate ()
                {
                    GpioIo (Exclusive, PullDefault, 0x0000, 0x0000,
                        IoRestrictionOutputOnly, "\\_SB.GPI0", 0x00,
                        ResourceConsumer, , ) { 0x0000 }
                })
                Name (SBFG, ResourceTemplate ()
                {
                    GpioInt (Edge, ActiveBoth, ExclusiveAndWake, PullNone,
                        0x0000, "\\_SB.GPI0", 0x00, ResourceConsumer, , )
                    { 0x0000 }
                })
                CreateWordField (SBFG, 0x17, INTP)
                CreateWordField (SBFP, 0x17, PWRP)
                PWRP = \_SB.GNUM (0x090E000D)
                INTP = \_SB.GNUM (0x090E0011)
                Return (ConcatenateResTemplate (SBFB,
                    ConcatenateResTemplate (SBFG, SBFP)))
            }
        }
    }
}
