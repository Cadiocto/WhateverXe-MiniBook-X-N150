# CHUWI MiniBook X N150 — графический стек и автоповорот MIPI DSI

## Проверенная конфигурация

- CHUWI MiniBook X N150, Intel N150
- macOS Monterey 12.7.6 (21H1320)
- Встроенная MIPI DSI панель: 1920×1200, 50 Hz
- IOKit framebuffer: `AppleIntelFramebuffer@0`
- Проверенная трансформация: `IOFBTransform = 80` (270°)

Результат относится к конкретному ноутбуку и этой конфигурации.

## Что входит в комплект

- `WhateverXe.kext` — версия 0.8.0, DEBUG build, устанавливался в EFI.
- `AppleIntelTGLCompat.kext` — версия 0.8.0, DEBUG build, отдельный компонент совместимости WhateverXe в macOS.
- `DSIRotate.c` — userspace helper для установки/проверки поворота.
- `org.whateverxe.chuwi-dsi-rotation.plist` — LaunchAgent для запуска helper при входе пользователя.

> Важно: эти DEBUG kext bundles взяты из диагностического архива конкретной системы. Они не являются универсальными драйверами для других моделей, поколений Intel или версий macOS. Перед заменой kext обязательно сохраните полную EFI и исходные компоненты, подготовьте резервный способ загрузки. Не заменяйте системные Apple graphics kext.

## Этап 1. Подготовка и резервное копирование

1. Убедитесь, что устройство и macOS соответствуют конфигурации выше.
2. Сделайте копию всей EFI, включая OpenCore и config.plist.
3. Сохраните текущие версии обоих устанавливаемых kext отдельно.
4. Подготовьте USB-носитель или другой проверенный способ загрузки с резервной EFI.
5. Не продолжайте, если нет возможности вернуть предыдущую EFI.

## Этап 2. Обновление графического стека

В ходе подготовки графики были заменены два компонента:

1. `WhateverXe.kext` — в EFI (для OpenCore типичный путь: `EFI/OC/Kexts/WhateverXe.kext`).
2. `AppleIntelTGLCompat.kext` — отдельный compatibility kext, установленный в macOS.

В архиве диагностической сборки оба bundle имеют версию 0.8.0 и DEBUG build. Bundle identifiers:

- `org.b00t0x.WhateverXe`
- `org.b00t0x.AppleIntelTGLCompat`

Для WhateverXe в OpenCore проверьте Kernel → Add: BundlePath, ExecutablePath, PlistPath и Enabled должны соответствовать реальной структуре bundle. Не оставляйте одновременно дублирующие версии одного kext.

AppleIntelTGLCompat — это не системный `AppleIntelTGLGraphics.kext` и не основание удалять или подменять системные Apple драйверы. Перед заменой compatibility kext выясните фактический путь текущей установленной копии и используемый на конкретной системе способ установки.

После загрузки проверьте:

```bash
kmutil showloaded | grep -Ei 'WhateverXe|AppleIntelTGLCompat'
```

Не переходите к настройке DSI, если графический стек не загружается штатно.

**Ограничение исходных данных:** переданный диагностический ZIP содержит сами DEBUG bundles и диагностические материалы, но не содержит полной истории команд установки AppleIntelTGLCompat. Поэтому точная команда и путь установки на исходной системе документально не подтверждены. Не используйте случайные команды установки из других конфигураций.

## Этап 3. Проверка DSI и доступности transform

Выполните:

```bash
ioreg -lw0 -p IOService -r -n AppleIntelFramebuffer@0 | grep -E 'IOFBProbeOptions|IOFBTransform'
```

В нашем тесте `IOFBProbeOptions = 1025` (`0x401`), в том числе присутствовал флаг `0x400` (`kIOFBSetTransform`).

Проверенные значения:

- 48 (90°) — изображение оказалось перевёрнутым, значение отвергнуто.
- 80 (270°) — правильная физическая ориентация относительно клавиатуры.

Не переносите значение 80 на другое устройство без проверки.

## Этап 4. Сборка и установка DSIRotate

Исходник: [DSIRotate.c](../tools/dsi-rotation/DSIRotate.c).

Соберите helper на целевой macOS:

```bash
clang -Wall -Wextra -O2 DSIRotate.c -o wex-dsi-rotate \
  -framework CoreGraphics -framework CoreFoundation -framework IOKit
```

Установите исполняемый файл:

```bash
sudo mkdir -p /usr/local/libexec
sudo install -m 755 wex-dsi-rotate /usr/local/libexec/wex-dsi-rotate
```

Helper работает только с найденным встроенным дисплеем, путь которого содержит `/IGPU@2/AppleIntelFramebuffer@0`. Он проверяет наличие бита `0x400` в `IOFBProbeOptions`. Если `IOFBTransform` уже равен 80, helper ничего повторно не меняет. Иначе он вызывает `IOServiceRequestProbe`, проверяет новое значение и повторяет попытки до 30 раз с интервалом 2 секунды.

Helper не меняет EFI, kext, boot arguments или системные настройки безопасности.

## Этап 5. Установка LaunchAgent

Файл: [org.whateverxe.chuwi-dsi-rotation.plist](../tools/dsi-rotation/org.whateverxe.chuwi-dsi-rotation.plist).

Скопируйте plist:

```bash
mkdir -p "$HOME/Library/LaunchAgents"
cp org.whateverxe.chuwi-dsi-rotation.plist \
  "$HOME/Library/LaunchAgents/org.whateverxe.chuwi-dsi-rotation.plist"
chmod 644 "$HOME/Library/LaunchAgents/org.whateverxe.chuwi-dsi-rotation.plist"
```

Зарегистрируйте LaunchAgent:

```bash
launchctl bootstrap "gui/$(id -u)" \
  "$HOME/Library/LaunchAgents/org.whateverxe.chuwi-dsi-rotation.plist"
```

Если запись уже зарегистрирована, сначала выгрузите её:

```bash
launchctl bootout "gui/$(id -u)/org.whateverxe.chuwi-dsi-rotation" 2>/dev/null || true
```

Затем повторите `bootstrap`.

Путь установленного helper в plist: `/usr/local/libexec/wex-dsi-rotate`. LaunchAgent запускает его при входе пользователя в систему.

## Этап 6. Проверка после входа и перезагрузки

Проверьте журнал:

```bash
cat /tmp/wex-dsi-rotation.log
```

Проверьте transform:

```bash
ioreg -lw0 -p IOService -r -n AppleIntelFramebuffer@0 | grep IOFBTransform
```

Проверьте LaunchAgent:

```bash
launchctl print "gui/$(id -u)/org.whateverxe.chuwi-dsi-rotation" \
  | grep -E 'state =|runs =|last exit code'
```

Ожидаемые признаки:

- `IOFBTransform = 80`
- `DSI already has desired transform: 80`
- `last exit code = 0`

`state = not running` после выполнения — нормально: helper одноразовый и завершается, а не остаётся постоянно запущенным.

## Отключение автозапуска и откат

Отключить LaunchAgent и удалить его plist:

```bash
launchctl bootout "gui/$(id -u)/org.whateverxe.chuwi-dsi-rotation"
rm "$HOME/Library/LaunchAgents/org.whateverxe.chuwi-dsi-rotation.plist"
```

Удалить helper:

```bash
sudo rm -f /usr/local/libexec/wex-dsi-rotate
```

Для отката графического стека восстановите сохранённые исходные версии WhateverXe и AppleIntelTGLCompat по процедуре, применимой к вашей EFI/macOS. Не удаляйте системные Apple kext.

## Результат тестирования

На тестовом ноутбуке transform 80 обеспечил правильную ориентацию. После установки LaunchAgent настройка сохранялась после полной перезагрузки и входа пользователя. Пользователь также перестал наблюдать ранее замеченные постоянные графические артефакты и рябь при обычном использовании.

Это наблюдение на одной конфигурации, а не доказательство, что причиной исчезновения артефактов был именно transform: до этого менялись компоненты графического стека.

## Контрольные суммы бинарных файлов из диагностического архива

- `WhateverXe.kext` / executable `WhateverXe`, version 0.8.0, SHA-256: `6860584faad93490edfec124c6120add5877cff0cd4409808ae05bea1b0576dc`
- `AppleIntelTGLCompat.kext` / executable `AppleIntelTGLCompat`, version 0.8.0, SHA-256: `3e2320415da70fdeb7ebb03c98eeaa9e6697835d5cd24a85c006c1d3076a32cd`

Хэши относятся к файлам `Contents/MacOS/<executable>`, а не ко всему каталогу bundle.
