# Changelog

Все заметные изменения StompForge фиксируются в этом файле. Формат основан на
Keep a Changelog; версия продукта следует Semantic Versioning.

## [Unreleased]

### Добавлено

- CMake-конфигурация iOS/iPadOS для Standalone-приложения и встроенного
  AUv3-расширения.
- App Group storage для общих NAM-моделей и cabinet IR между приложением и
  AUv3; на остальных платформах используется управляемый каталог StompForge.
- Инструкция Apple mobile build с параметрами подписи и отдельными release
  gates.

## [0.0.1-alpha] — 2026-07-17

Первая публичная alpha-версия. Числовая версия бинарников и установщика —
`0.0.1`, канал выпуска — `alpha`.

### Добавлено

- Настраиваемая матрица педалборда от 1x1 до 4x3 с двенадцатью физическими
  слотами, пустыми ячейками, drag-and-drop и сохранением состояния.
- Каталог Dynamic, Distortion, Modulation, Amp, Reverb, Delay, EQ, Cabs и Utils.
- STARGATE, DEIMOS-1, FREQUENCY, CERES-2, MARS-8, VULCAN-5, VOID CHAMBER,
  PULSAR и LUNER.
- IMPULSE с загрузкой WAV/AIFF, managed fallback cache, Low/High Cut, Level и
  Mix.
- MODELER на NeuralAmpModelerCore v0.5.4: загрузка `.nam`, ресемплинг к частоте
  хоста, независимое stereo-состояние, Input, Output и Mix.
- По три основных регулятора в карточке эффекта и окно `MORE` для остальных
  параметров.
- Вертикальные Input/Output фейдеры с отображением текущего уровня.
- Windows x64 VST3, Standalone и Inno Setup installer.
- Новая иконка приложения из исходного Adobe Illustrator artwork.

### Изменено

- Сигнальная цепочка визуально проходит через пустые ячейки и выбирает
  корректные стороны модулей при горизонтальном, вертикальном и диагональном
  размещении.
- Поток педалборда направлен от нижнего правого входа к верхнему левому выходу.
- Активный IMPULSE подавляет CABSIM amp-модулей; попытка включить CABSIM
  показывает предупреждение.

### Исправлено

- NAM architecture parsers принудительно сохраняются при линковке VST3 и
  Standalone, включая WaveNet, LSTM и SlimmableContainer.
- Устранено постепенное исчезновение сигнала в нелинейных модулях.
- Standalone направляет аппаратный вход 1 в оба канала stereo-шины.
- Исправлено восстановление отсутствующих параметров старых DAW-сессий.
