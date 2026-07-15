# Guitar Forge

Кроссплатформенный гитарный эффект VST3/Standalone на JUCE 8.

## Эффекты

- Noise gate: порог от -80 до -20 dB
- Drive: мягкое аналогоподобное насыщение `tanh`
- Mix: параллельное смешивание clean/drive
- Bass / Mid / Treble: трёхполосный активный EQ
- Output: итоговая компенсация уровня
- Автоматизация всех параметров и сохранение состояния в проекте DAW
- Mono и stereo

## Сборка на Windows

Понадобятся Git, CMake 3.22+ и Visual Studio 2022 с компонентом **Desktop development with C++**.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

VST3 будет в `build/GuitarForge_artefacts/Release/VST3/Guitar Forge.vst3`.
Standalone-приложение — в `build/GuitarForge_artefacts/Release/Standalone/`.

Скопируйте `.vst3` в `C:\Program Files\Common Files\VST3`, затем пересканируйте плагины в DAW.

## Сборка на macOS

```bash
cmake -S . -B build -G Xcode
cmake --build build --config Release
```

> JUCE распространяется по собственной лицензии. Перед коммерческим релизом выберите подходящий план JUCE и замените тестовые идентификаторы производителя/плагина в `CMakeLists.txt`.
