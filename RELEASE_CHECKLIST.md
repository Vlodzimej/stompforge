# StompForge 0.0.2-alpha release checklist

[English](RELEASE_CHECKLIST.en.md)

Дата аудита: 2026-07-17. Этот документ — инженерный checklist, а не юридическая
консультация.

## 1. Блокирующие лицензионные решения

- [x] Выбран основной лицензионный маршрут StompForge:
  - **Commercial JUCE route** — закрытый/не-AGPL продукт; владелец и все
    Framework Users имеют подходящие действующие JUCE 8 seats согласно
    revenue/funding и условиям распространения.
  - **Open-source route** — весь соответствующий продукт и исходники
    распространяются на условиях AGPLv3 с выполнением требований о source,
    notices и network use.
- [x] В корне добавлен соответствующий `LICENSE` или сохранено документальное
  подтверждение commercial JUCE licence до публикации бинарников.
- [ ] Для ASIO-enabled Standalone принят Steinberg ASIO proprietary agreement
  либо выбран совместимый GPLv3/open-source маршрут.
- [x] Если ASIO licence не подтверждена, release пересобран без
  `ASIO_SDK_PATH`.

Для 0.0.2-alpha выбран AGPL-3.0-or-later. Публичный release собирается без
ASIO SDK.

## 2. Совместимость компонентов

| Компонент | Лицензия | Результат |
|---|---|---|
| NeuralAmpModelerCore v0.5.4 | MIT | Совместима с commercial и AGPL route при сохранении notice |
| NeuralAmpModelerPlugin | MIT | Использован как reference; код plugin напрямую не включён |
| AudioDSPTools | MIT; отдельные embedded notices | Совместима при сохранении notices |
| Eigen | преимущественно MPL-2.0 | Допускает larger work; сохранять license/source notices |
| nlohmann/json | MIT | Совместима при сохранении notice |
| JUCE 8.0.10 | AGPLv3 или JUCE 8 EULA | Выбран открытый маршрут AGPLv3 |
| VST3 SDK из JUCE | MIT | Совместима |
| ASIO SDK, если включён | proprietary Steinberg или GPLv3 | Требует отдельного выбора |

## 3. Права на контент

- [ ] Подтверждены права на `Assets/stompforge-icon-source.ai`.
- [x] В release не включены сторонние `.nam` или IR без разрешения.
- [x] README сообщает, что пользователь отвечает за лицензии загружаемых
  моделей и impulse response.

## 4. Техническая проверка

- [x] Чистая Release-конфигурация Windows x64 без ASIO SDK.
- [x] Собраны VST3 и Standalone версии 0.0.2.
- [x] `StompForgeDspSmokeTest.exe` завершился с кодом 0.
- [x] Проверена загрузка WaveNet `.nam` и linked-channel режим.
- [x] Проверены mono/stereo, bypass, разные block size и отсутствие NaN/Inf.
- [x] `git diff --check` не сообщает ошибок.
- [x] `build/`, `dist/` и `.tools/` не индексируются Git.
- [x] iOS/iPadOS targets помечены как experimental и не входят в бинарные
  файлы этого релиза.

## 5. Упаковка

- [x] Installer собран из `Packaging/StompForge.iss` с помощью Inno Setup 6.7.3.
- [ ] Установлены VST3 и Standalone на чистой Windows-машине/VM.
- [ ] Проверено повторное сканирование в поддерживаемой DAW.
- [x] `THIRD_PARTY_NOTICES.md` и `LICENSE` включены в installer и portable
  package.
- [x] Имя installer: `StompForge-0.0.2-alpha-Windows-x64-Setup.exe`.
- [x] Создан portable package
  `StompForge-0.0.2-alpha-Windows-x64-Portable.zip`.
- [x] Вычислены SHA-256 и создан `SHA256SUMS.txt`.

## 6. Публикация

- [x] Русская и английская документация соответствует фактическому содержимому.
- [x] `CHANGELOG.md` и `CHANGELOG.en.md` соответствуют release notes.
- [ ] Создан annotated tag `v0.0.2-alpha`.
- [ ] GitHub Release помечен как pre-release.
- [ ] Приложены installer, portable VST3 archive, checksum и known limitations.
- [ ] Сохранена копия исходников, соответствующая опубликованным бинарникам.
