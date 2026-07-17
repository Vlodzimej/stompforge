# StompForge 0.0.1-alpha release checklist

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

Для 0.0.1-alpha выбран AGPL-3.0-or-later. Release собирается без ASIO SDK.

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
- [ ] В release не включены сторонние `.nam` или IR без разрешения.
- [x] README сообщает, что пользователь отвечает за лицензии загружаемых
  моделей и impulse response.

## 4. Техническая проверка

- [x] Чистая Release-конфигурация Windows x64.
- [x] Собраны VST3 и Standalone.
- [x] `StompForgeDspSmokeTest.exe` завершился с кодом 0.
- [x] Проверена загрузка WaveNet `.nam`.
- [ ] Проверены mono/stereo, bypass, разные block size и отсутствие NaN/Inf.
- [x] `git diff --check` не сообщает ошибок.
- [x] `build/`, `dist/` и `.tools/` не индексируются Git.

## 5. Упаковка

- [x] Installer собран из `Packaging/StompForge.iss`.
- [ ] Установлены VST3 и Standalone на чистой Windows-машине/VM.
- [ ] Проверено повторное сканирование в поддерживаемой DAW.
- [x] `THIRD_PARTY_NOTICES.md` установлен рядом со Standalone.
- [x] Имя installer: `StompForge-0.0.1-alpha-Windows-x64-Setup.exe`.
- [x] Вычислен SHA-256 installer и опубликован в `SHA256SUMS.txt`.

## 6. Публикация

- [x] `CHANGELOG.md` соответствует фактическому содержимому.
- [ ] Создан подписанный или annotated tag `v0.0.1-alpha`.
- [ ] GitHub Release помечен как pre-release.
- [ ] Приложены installer, portable VST3 archive, checksum и known limitations.
- [ ] Сохранена копия исходников, соответствующая опубликованным бинарникам.
