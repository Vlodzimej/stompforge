# Changelog

## Unreleased

- Empty cells now open the effect menu on a short click as well as long-press.
  Signal routing traverses every intermediate empty-cell centre and selects
  left, right, top or bottom pedal ports according to the route direction.
- Added Clear current cell to the long-press menu, allowed grid sizes down to
  1x1, improved the matrix preview, and replaced knob-to-knob signal lines with
  rounded orthogonal Bezier cable routing between pedal input/output ports.
- Added persistent empty pedalboard slots with drop targets, long-press effect
  insertion, and a directional signal cable from the right-side input through
  active effects to the left-side output. Legacy six-slot sessions migrate to
  the new twelve-position representation.
- Reworked the pedalboard layout to flow from the bottom-right input to the
  top-left output, swapped the global Input/Output controls, and added a saved
  visual grid selector supporting matrices up to 4 columns by 3 rows.
- Added the IMPULSE cabinet IR player in the new Cabs category, with WAV/AIFF
  loading, managed fallback cache, session restore, Level, Low/High Cut and Mix.
- Added the VULCAN-5 amp: Clean/Crunch/Lead relay voicings, 12AX7 stages,
  cold-biased 6L6GC output, Presence, Resonance, supply sag and 4x12 cabsim.

Все заметные изменения StompForge фиксируются в этом файле.

## [0.0.1] — 2026-07-16

Первый публичный прототип.

### Добавлено

- Модульный педалборд на шесть слотов с обработкой слева направо.
- Изменение порядка модулей drag-and-drop.
- Long-tap каталог с категориями Dynamic, Distortion, Modulation, Amp, Reverb,
  Delay, EQ и Utils.
- STARGATE — noise gate.
- DEIMOS-1 — circuit-inspired distortion с 4-кратным oversampling.
- CERES-2 — BBD-inspired chorus с треугольным LFO.
- MARS-8 — модель лампового усилителя с блоком питания, оконечным каскадом и
  переключаемым cabsim 4x12.
- VOID CHAMBER — алгоритмический reverb.
- PULSAR — digital delay.
- FREQUENCY — трёхполосный EQ.
- LUNER — хроматический тюнер с отображением ноты и отклонения в центах.
- Входной и выходной gain со сглаживанием.
- Поддержка VST3 и Standalone; ASIO доступен в Windows Standalone.
- Сохранение параметров и порядка модулей в состоянии DAW.
- Windows x64 installer для VST3 и Standalone.

### Исправлено

- Устранено постепенное исчезновение сигнала в нелинейных модулях.
- В Standalone вход 1 направляется в оба канала, исключая шум незадействованного
  второго аппаратного входа.
- Исправлено отображение подзаголовка при несовпадении кодировок.

### Оформление

- Добавлена фирменная иконка приложения и установщика.
- Кнопки `ON` показывают состояние эффекта: красная подсветка для включённой
  обработки и тёмный фон для bypass.
