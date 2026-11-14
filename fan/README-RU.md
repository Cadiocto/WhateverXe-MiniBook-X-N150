# CHUWIFanProbe 0.1.0

Диагностический kext для Chuwi MiniBook X N150 / macOS Monterey.

- Находит ACPI-устройство PTID.
- Вызывает только `PTID.OSDD()`.
- Печатает тип и содержимое результата в kernel log.
- Не вызывает WPMD, _FSL, ECWT и не пишет в Embedded Controller.
- Целевой ABI: x86_64, macOS 12+.

Это диагностическая сборка, не драйвер управления вентилятором и пока не публикует SMC-ключи.
