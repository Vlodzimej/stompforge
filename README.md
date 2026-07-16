# StompForge

Версия: **0.0.1**. Автор: **Vlodzimej Garlic**.

Кроссплатформенный гитарный эффект VST3/Standalone на JUCE 8. Виртуальный
педалборд поддерживает матрицу от 1x1 до 4x3, пустые ячейки, добавление модулей
из каталога и изменение порядка перетаскиванием.
Долгое нажатие на карточку открывает каталог замены по категориям Dynamic,
Distortion, Modulation, Amp, Reverb и Delay.

## Эффекты

- STARGATE: noise gate с порогом от -80 до -20 dB.
- DEIMOS-1: circuit-inspired модель транзисторного предусилителя, кремниевого
  диодного ограничения и пассивной Tone-секции с 4-кратным oversampling.
- MARS-8: модель лампового преампа, tone stack, фазоинвертора, двухтактного
  оконечного усилителя, выходного трансформатора и динамического проседания питания.
- CERES-2: аналоговый BBD-inspired chorus с треугольным LFO и управлением Rate,
  Depth и Mix.
- VOID CHAMBER с управлением Size, Damping и Mix.
- PULSAR с управлением Time, Feedback и Mix.
- LUNER: хроматический гитарный тюнер с индикацией ближайшей ноты и отклонения
  в центах. Доступен в категории Utils.
- Переключаемый cabsim с усреднённой АЧХ кабинета 4x12 включён по умолчанию.
- FREQUENCY: трёхполосный активный EQ из категории EQ.
- Общий выходной уровень, автоматизация параметров и сохранение состояния DAW.
- Входной gain от -24 до +24 dB. В Standalone первый моно-вход направляется в
  оба выходных канала до обработки.
- Mono и stereo.

DEIMOS-1 и MARS-8 предназначены для обработки в реальном времени. Это
circuit-inspired DSP-модели, а не покомпонентные SPICE-симуляции. Для дальнейшей
калибровки требуются измеренные АЧХ, спектры и re-amp записи эталонных устройств.

## Сборка на Windows

Понадобятся Git, CMake 3.22+ и Visual Studio 2022 с компонентом
**Desktop development with C++**.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Для ASIO в Standalone укажите корень установленного ASIO SDK:

```powershell
cmake -S . -B build -DASIO_SDK_PATH="C:\SDKs\asiosdk"
cmake --build build --config Release
```

В корне SDK должен находиться `common/iasiodrv.h`. Для VST3 аудиодрайвер и
размер блока задаются самой DAW.

Артефакты:

- `build/StompForge_artefacts/Release/VST3/StompForge.vst3`
- `build/StompForge_artefacts/Release/Standalone/StompForge.exe`

## Сборка на macOS

```bash
cmake -S . -B build -G Xcode
cmake --build build --config Release
```

Перед коммерческим релизом необходимо выбрать подходящий план лицензирования JUCE.
