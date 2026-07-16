# StompForge — контекст и инструкции для Codex

## Проект

StompForge — кроссплатформенный гитарный аудиоэффект на JUCE 8 в форматах
VST3 и Standalone. Пользователь подключает до трёх модулей, меняет параметры и
переставляет модули drag-and-drop слева направо. Рабочая папка исторически
называется `GuitarForge`, но имя продукта и всех целей — `StompForge`.

Стек: C++20, JUCE 8.0.10, CMake, APVTS, `juce_dsp`, Windows/Visual Studio 2022.
Поддерживаются mono и stereo с одинаковым числом входных и выходных каналов.
Standalone опционально использует ASIO через `ASIO_SDK_PATH`; VST3 получает
аудио и размер блока от DAW.

## Структура

- `CMakeLists.txt` — конфигурация сборки.
- `Source/Effects.*` — интерфейс DSP-модуля и реализации эффектов.
- `Source/PluginProcessor.*` — APVTS, аудиотракт, порядок и состояние.
- `Source/PluginEditor.*` — интерфейс, параметры и drag-and-drop.
- `Tests/DspSmokeTest.cpp` — длительный offline DSP-тест.
- `README.md` — пользовательское описание.

Внешние изображения скинов не используются. `build/` и `.tools/` не добавлять
в Git.

## Архитектура DSP

Каждый эффект реализует `EffectModule` с методами `prepare`, `reset`, `process`.
Процессор владеет четырьмя экземплярами, но последовательно обрабатывает ровно
три выбранных модуля. Порядок упакован в атомарный `juce::uint32` и сохранён в
свойстве APVTS `pedalOrder`. Порядок должен содержать три допустимых уникальных
`PedalId`; аудиопоток не блокируется и не изменяет ValueTree.

Цепочка по умолчанию:

1. Noise Gate.
2. DIST-1.
3. MARS-8 с включённым cabsim 4x12.

Третий слот переключается между MARS-8 и Tone Shaper. Старый `DriveEffect` и
параметры `drive`, `mix`, `driveBypass` сохранены для совместимости.

## Модули

### Noise Gate

`GateEffect`, параметры `gate`, `gateBypass`, порог от -80 до -20 dB.

### DIST-1

`Dist1Effect`; параметры `ds1Dist`, `ds1Tone`, `ds1Level`, `ds1Bypass` оставлены
стабильными для совместимости проектов DAW. Circuit-inspired модель ранней
аналоговой ревизии с TA7136 и 4x oversampling:

1. Входной RC high-pass около 7,2 Гц.
2. Инвертирующий Q2 с мягкой компрессией и спадом около 1,35 кГц.
3. Частотно-зависимая обратная связь Dist с перегибом около 339 Гц.
4. Фильтр перед ограничителями около 7,2 кГц.
5. Противопараллельные кремниевые диоды с коленом около 0,62 В.
6. Пассивные Tone-ветви: low-pass 234 Гц и high-pass 1,06 кГц.
7. Level и выходной DC blocker.

Все одиночные RC-цепи реализовывать фильтрами первого порядка. Не заменять их
резонансными biquad: это вызывает фазовое вычитание и исчезновение сигнала.

### Tone Shaper

`ToneEffect`: `bass`, `mid`, `treble`, `toneBypass`. Частоты 180 Гц, 850 Гц
(Q 0,8) и 3,5 кГц.

### MARS-8

`Mars8Effect`; стабильные параметры совместимости: `jcmPreamp`, `jcmBass`,
`jcmMiddle`, `jcmTreble`, `jcmMaster`, `jcmPresence`, `jcmSag`, `jcmCab`,
`jcmBypass`. Модель включает каскады 12AX7, cold clipper 10 кОм, cathode
follower, пассивный tone stack, long-tail phase inverter, push-pull EL34,
выходной трансформатор, отрицательную обратную связь/Presence и envelope-sag.
Cabsim — усреднённая многополюсная модель кабинета 4x12, не микрофонный IR.

Между нелинейными каскадами обязательны разделительные DC blockers. Их
отсутствие накапливает смещение и запирает waveshaper. Низкочастотные RC-фильтры
на высокой частоте oversampling должны быть однополюсными: низкочастотный
biquad с `float` терял точность и через доли секунды обнулял сигнал.

## APVTS и realtime

Существующие parameter ID не переименовывать без явной миграции состояния.
Параметры DSP читать через `getRawParameterValue()` и атомарные значения.

- Не использовать mutex, I/O или выделение памяти в `processBlock()`.
- Все буферы, oversampling и фильтры готовить в `prepareToPlay()`/`prepare()`.
- Сглаживать изменения gain и параметры, вызывающие щелчки.
- Проверять mono, stereo, bypass, сохранение состояния и разные block size.
- Не допускать NaN/Inf, постепенного затухания в ноль и неограниченного роста.

## UI

`StompForgeAudioProcessorEditor` — `DragAndDropContainer`, а `PedalCard` —
`DragAndDropTarget`. Перетаскивание начинается в верхней области карточки,
drop вызывает `processor.movePedal()`, затем `layoutPedals()`. Одновременно
активно не более трёх карточек. Карточки рисуются средствами JUCE без фото.

## Сборка и Git

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

В этой среде CMake находится в `.tools`. ASIO SDK установлен в
`.tools/asio-sdk`; при конфигурации передавать `-DASIO_SDK_PATH=...`.

Перед завершением:

1. Выполнить полную Release-сборку и `Tests/DspSmokeTest.cpp`.
2. Выполнить `git diff --check`.
3. Проверить, что `build/` и `.tools/` не индексируются.
4. Не создавать коммит без явного запроса пользователя.

Основная ветка — `master`. Сохранять существующие незакоммиченные изменения и
не выполнять destructive-команды без явного разрешения. В начале новой задачи
сначала читать этот файл и проверять `git status`.
