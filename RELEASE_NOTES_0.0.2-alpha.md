# StompForge 0.0.2-alpha

[English](RELEASE_NOTES_0.0.2-alpha.en.md)

Второй alpha-выпуск StompForge улучшает подготовку проекта к iPhone/iPad и
сенсорное управление, сохраняя проверенные Windows x64 VST3 и Standalone.

## Основное

- Экспериментальные iOS/iPadOS Standalone и AUv3 targets в общем CMake-проекте.
- Адаптивный landscape pedalboard, touch-меню эффектов и выбор buffer size.
- App Group storage для общих ресурсов приложения и AUv3.
- Автоматическое сохранение состояния iOS Standalone.
- Улучшенная linked-channel обработка NAM в Standalone.
- Новая цепочка по умолчанию: LUNER, STARGATE, DEIMOS-1, VULCAN-5,
  VOID CHAMBER и PULSAR; reverb/delay начинают в bypass.
- Удалена расчётная надпись `LATENCY`.

## Совместимость

- Форматы релиза: Windows x64 VST3 и Standalone.
- Стабильные APVTS parameter IDs сохранены.
- MODELER доступен в Windows VST3/Standalone, но временно отключён на
  iOS/iPadOS.
- Выбранные IR/NAM импортируются в управляемое хранилище StompForge.

## Известные ограничения

- Это тестовый alpha-выпуск, не предназначенный для критичных live-сценариев.
- iOS/iPadOS targets не прошли подпись и тестирование на физическом устройстве.
- Загрузка большой `.nam` может кратковременно задержать UI.
- Бинарники не подписаны цифровой подписью.
- Публичные Windows-бинарники собраны без ASIO SDK.

## Файлы релиза

- `StompForge-0.0.2-alpha-Windows-x64-Setup.exe`
- `StompForge-0.0.2-alpha-Windows-x64-Portable.zip`
- `SHA256SUMS.txt`

StompForge и соответствующий исходный код распространяются по
AGPL-3.0-or-later.
