# CHUWI ES8326 ACPI diagnostic draft

This isolated branch runs ACPICA iasl on a macOS Intel GitHub Actions runner.
The workflow compiles the DSL to AML, disassembles the AML back to DSL, and
uploads logs and outputs for review.

**Review-only output. Do not install the AML in EFI.** Successful compilation
does not prove that resources are correct, that the codec is powered, or that
VoodooI2C will instantiate an I2C client.

Source: CHUWI-ES8326-DIAGNOSTIC/SSDT-ES8326-Test.dsl
Workflow: .github/workflows/es8326-acpi-compile.yml
