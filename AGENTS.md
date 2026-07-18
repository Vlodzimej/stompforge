# StompForge — контекст и инструкции для Codex

## Проект

StompForge — кроссплатформенный гитарный аудиоэффект на JUCE 8 в форматах
VST3 и Standalone. Пользователь размещает эффекты в матрице до 4x3, меняет
параметры и переставляет модули drag-and-drop. Рабочая папка исторически
называется `GuitarForge`, но имя продукта и всех целей — `StompForge`.

Стек: C++20, JUCE 8.0.10, CMake, APVTS, `juce_dsp`, Windows/Visual Studio 2022
и iOS/iPadOS/Xcode.
Поддерживаются mono и stereo с одинаковым числом входных и выходных каналов.
Standalone опционально использует ASIO через `ASIO_SDK_PATH`; VST3 получает
аудио и размер блока от DAW.

Текущая версия продукта: `0.0.2-alpha` (числовая версия бинарников `0.0.2`).
Автор и publisher: `Vlodzimej Garlic`.
Windows-установщик собирается из `Packaging/StompForge.iss` в каталог `dist/`.

## Структура

- `CMakeLists.txt` — конфигурация сборки.
- `cmake/PlatformConfig.cmake` — выбор desktop или iOS форматов и Apple
  capabilities.
- `Source/AssetRepository.*` — импорт IR/NAM в управляемое хранилище; на iOS
  используется общий App Group container.
- `Source/Effects.*` — интерфейс DSP-модуля и реализации эффектов.
- `Source/PluginProcessor.*` — APVTS, аудиотракт, порядок и состояние.
- `Source/PluginEditor.*` — интерфейс, параметры и drag-and-drop.
- `Tests/DspSmokeTest.cpp` — длительный offline DSP-тест.
- `Platforms/Apple/README.md` — англоязычная инструкция по сборке
  Standalone/AUv3 и Apple release gates.
- `Assets/stompforge-icon.png` — исходная прозрачная иконка приложения.
- `Assets/stompforge-icon.ico` — многоразмерная иконка установщика Windows.
- `Assets/stompforge-icon-source.ai` — векторный мастер новой иконки.
- `CHANGELOG.md` — англоязычный журнал изменений релизов.
- `README.md` — англоязычное пользовательское описание.
- `RELEASE_NOTES_<version>.md` — англоязычные заметки к выпускам.
- `THIRD_PARTY_NOTICES.md` — обязательные уведомления сторонних компонентов.
- `RELEASE_CHECKLIST.md` — англоязычные release gates, включая выбор лицензии
  JUCE/ASIO.

Внешние изображения скинов не используются. `build/` и `.tools/` не добавлять
в Git.

## Язык документации

`AGENTS.md` ведётся на русском языке. Публичные описательные и релизные файлы
(`README.md`, `Platforms/Apple/README.md`, `CHANGELOG.md`,
`RELEASE_NOTES_<version>.md`, `RELEASE_CHECKLIST.md` и
`THIRD_PARTY_NOTICES.md`) ведутся только на английском языке.

Не создавать параллельные языковые версии и не добавлять к именам файлов
суффиксы `.en` или `.ru`. Внутренние ссылки должны указывать на канонические
имена файлов без языкового суффикса.

## Архитектура DSP

Каждый эффект реализует `EffectModule` с методами `prepare`, `reset`, `process`.
Процессор владеет одиннадцатью экземплярами и обрабатывает активные позиции
настраиваемой матрицы размером до 4x3. Порядок двенадцати физических слотов
упакован в атомарный `juce::uint64` и сохранён в свойстве APVTS `pedalOrder`.
На слот отводится 4 бита; значение `PedalId::empty` может повторяться, остальные
`PedalId` уникальны. Аудиопоток пропускает пустые позиции, не блокируется и не
изменяет ValueTree.

Цепочка по умолчанию:

1. LUNER.
2. STARGATE.
3. DEIMOS-1.
4. VULCAN-5 с включённым cabsim.
5. VOID CHAMBER.
6. PULSAR.

VOID CHAMBER и PULSAR по умолчанию находятся в bypass. FREQUENCY доступен в
категории EQ вне базовой цепочки. Старый `DriveEffect` и параметры `drive`,
`mix`, `driveBypass` сохранены для совместимости.

## Модули

### STARGATE

`StargateEffect`, параметры `gate`, `gateBypass`, порог от -80 до -20 dB.

### DEIMOS-1

`Deimos1Effect`; параметры `ds1Dist`, `ds1Tone`, `ds1Level`, `ds1Bypass` оставлены
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

### FREQUENCY

`FrequencyEffect`: `bass`, `mid`, `treble`, `toneBypass`. Частоты 180 Гц, 850 Гц
(Q 0,8) и 3,5 кГц.

### Modulation, Reverb и Delay

`Ceres2Effect`, `VoidChamberEffect`, `PulsarEffect` используют стабильные APVTS-параметры
с префиксами `chorus`, `reverb`, `delay`. Delay выделяет кольцевые буферы только
в `prepare()`. Базовый каталог содержит по одному эффекту каждой категории.

CERES-2 моделирует 1024-ступенчатую BBD-линию: входной high-pass, три
однополюсных anti-alias фильтра примерно 4,82/1,94/4,82 кГц, ограничение и
квантование переноса заряда, дробную задержку с центром 4,6 мс, треугольный LFO
интегратор-компараторного типа и симметричный reconstruction-фильтр. Rate
охватывает примерно 0,25–3,2 Гц, Depth — до ±1,8 мс. Не заменять треугольный
LFO синусоидальным без отдельного режима совместимости.

### LUNER

`LunerEffect` — проходной хроматический тюнер из категории Utils. Анализирует
моно-сумму входа в окне 2048 отсчётов после двукратной децимации, использует
нормализованную автокорреляцию в диапазоне 65–1200 Гц и публикует ближайшую MIDI
ноту, частоту, отклонение ±50 cents и confidence через атомарные значения.
Аудиосигнал не изменяет. UI показывает центральную шкалу: низкий тон растёт
влево, высокий — вправо; цвет меняется от красного к ярко-зелёному у центра.

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

### MODELER

`ModelerEffect` использует NeuralAmpModelerCore v0.5.4. Параметры:
`modelerInput`, `modelerOutput`, `modelerMix`, `modelerBypass`. Модель `.nam`
загружается вне аудиопотока, проверяется как mono-in/mono-out и публикуется
атомарно; stereo обрабатывается двумя независимыми экземплярами. При
несовпадении sample rate используется pinned AudioDSPTools resampler.
Translation units с NAM config parsers регистрируются статически, поэтому
финальная линковка VST3/Standalone обязана сохранять весь shared-code archive.

## APVTS и realtime

Существующие parameter ID не переименовывать без явной миграции состояния.
Параметры DSP читать через `getRawParameterValue()` и атомарные значения.

- Не использовать mutex, I/O или выделение памяти в `processBlock()`.
- Все буферы, oversampling и фильтры готовить в `prepareToPlay()`/`prepare()`.
- Сглаживать изменения gain и параметры, вызывающие щелчки.
- Проверять mono, stereo, bypass, сохранение состояния и разные block size.
- Не допускать NaN/Inf, постепенного затухания в ноль и неограниченного роста.

Параметр `input` задаёт входной gain -24…+24 dB и применяется перед всей
цепочкой со сглаживанием. В Standalone входной канал 1 копируется в оба канала
стереошины до DSP, чтобы незадействованный аппаратный вход 2 не добавлял шум.

## UI

`StompForgeAudioProcessorEditor` — `DragAndDropContainer`, а `PedalCard` —
`DragAndDropTarget`. Перетаскивание начинается в верхней области карточки,
drop вызывает `processor.movePedal()`, затем `layoutPedals()`. Матрица
настраивается от 1x1 до 4x3, поддерживает пустые ячейки, добавление коротким
кликом/long-tap и очистку из меню. Long-tap 650 мс открывает иерархическое меню
Dynamic, Distortion, Modulation, Amp, Reverb, Delay, EQ, Utils. Выбор уже активного эффекта
меняет два слота местами. Карточки рисуются средствами JUCE без фото.
Кнопка `ON` визуально инвертирует сохранённый bypass-параметр: тёмно-красная
подсветка означает включённую обработку, нейтральный тёмный фон — bypass.
На iOS/iPadOS UI работает только в landscape, использует touch-oriented меню
и кнопку выбора аппаратного buffer size. MODELER на iOS временно недоступен.
В UI не отображать расчётную надпись `LATENCY`.

## Сборка и Git

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

В этой среде CMake находится в `.tools`. ASIO SDK установлен в
`.tools/asio-sdk`; при конфигурации передавать `-DASIO_SDK_PATH=...`.

Перед завершением:

1. Выполнить полную Release-сборку и `Tests/DspSmokeTest.cpp`, включая загрузку
   официальной тестовой `.nam`.
2. Выполнить `git diff --check`.
3. Проверить, что `build/`, `dist/` и `.tools/` не индексируются.
4. Перед публичным релизом пройти `RELEASE_CHECKLIST.md`, подтвердить выбранную
   JUCE licence и отдельно проверить ASIO licence для ASIO-enabled Standalone.
5. Убедиться, что `THIRD_PARTY_NOTICES.md` включён в installer/package.
6. Не создавать коммит, тег или GitHub Release без явного запроса пользователя.

Основная ветка — `master`. Сохранять существующие незакоммиченные изменения и
не выполнять destructive-команды без явного разрешения. В начале новой задачи
сначала читать этот файл и проверять `git status`.
