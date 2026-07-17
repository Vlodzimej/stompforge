# Сборка для Apple mobile

Русский | [English](README.md)

StompForge использует общий JUCE processor/editor для iOS/iPadOS
Standalone-приложения и встроенного AUv3-расширения. При
`CMAKE_SYSTEM_NAME=iOS` CMake создаёт обе цели; Windows продолжает собирать
VST3 и Standalone.

## Требования

- macOS с актуальным Xcode;
- CMake 3.22 или новее;
- Apple Development Team для физических устройств;
- зарегистрированные identifiers:
  - `STOMPFORGE_IOS_BUNDLE_ID`;
  - `STOMPFORGE_IOS_APP_GROUP_ID`.

Значения по умолчанию: `audio.stompforge.app` и
`group.audio.stompforge.shared`.

## Simulator

```bash
cmake -S . -B build-ios \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0

cmake --build build-ios \
  --config Debug \
  --target StompForge_Standalone \
  -- -sdk iphonesimulator
```

Для физического устройства добавьте при конфигурации:

```bash
-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="Apple Development" \
-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="<TEAM_ID>"
```

Сборка `StompForge_Standalone` также собирает и встраивает
`StompForge_AUv3`. Entitlements приложения и расширения создаются из общей
App Group конфигурации.

UI работает только в landscape. MODELER временно отключён на iOS/iPadOS до
появления поддерживаемой mobile-интеграции NAM.

## Общее хранилище

- IR: `StompForge/ImpulseResponses`;
- NAM на поддерживаемых платформах: `StompForge/NAMModels`.

На iOS каталоги находятся в App Group container, доступном приложению и AUv3.

## Непройденные release gates

- подготовить App Store icon без alpha channel;
- проверить interruptions и audio route changes;
- проверить touch drag-and-drop, long press и поддерживаемые размеры окна;
- профилировать oversampled effects на реальных iPhone/iPad;
- протестировать несколько AUv3 instances в production host;
- выполнить Archive и Validate в Xcode.
