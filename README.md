# LCALauncher

Кастомный современный лаунчер для **Minecraft 1.21.1 NeoForge** с поддержкой своей системы авторизации и автоматической синхронизации модов.

Написан на **C++20** и **Qt 6 (QML / Qt Quick)**.

---

## 🛠️ Системные требования

* **Компилятор C++20**:
  * Windows: MSVC 2022 (v143) или MinGW GCC 13+
  * macOS: Apple Clang 15+ (Xcode 15+)
  * Linux: GCC 12+ или Clang 15+
* **CMake**: 3.22 или новее
* **Qt**: 6.5 или новее (модули: `Core`, `Gui`, `Quick`, `Qml`, `QuickControls2`, `Network`, `Concurrent`)
* **Java Runtime**: Java 21 (требуется для игры Minecraft 1.21.1)

---

## 🚀 Инструкция по сборке

### 🍎 macOS
1. Установите Qt 6 и Ninja через Homebrew:
   ```bash
   brew install qt ninja
   ```
2. Сконфигурируйте и соберите проект:
   ```bash
   cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```
3. Для сборки готового `.dmg`:
   ```bash
   macdeployqt build/LCALauncher.app -qmldir=qml -dmg
   ```

---

### 🪟 Windows
1. Установите **Qt 6 (MSVC 2022 64-bit)** через официальный Qt Online Installer (например, в `C:\Qt\6.8.0\msvc2022_64`).
2. В командной строке разработчика (x64 Native Tools Command Prompt for VS 2022):
   ```cmd
   cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
   cmake --build build --config Release
   ```
3. Для сборки переносимой папки со всеми DLL:
   ```cmd
   mkdir dist
   copy build\Release\LCALauncher.exe dist\
   windeployqt --qmldir qml dist\LCALauncher.exe
   ```

---

### 🐧 Linux (Ubuntu / Debian / Arch)
1. Установите зависимости:
   * **Ubuntu/Debian**:
     ```bash
     sudo apt update
     sudo apt install -y build-essential cmake ninja-build \
         qt6-base-dev qt6-declarative-dev qml6-module-qtquick-controls \
         qml6-module-qtquick-layouts libgl1-mesa-dev
     ```
   * **Arch Linux**:
     ```bash
     sudo pacman -S base-devel cmake ninja qt6-base qt6-declarative
     ```
2. Сконфигурируйте и соберите проект:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```
3. Запуск бинарника:
   ```bash
   ./build/lcalauncher
   ```

---

## 📦 Автоматическая CI/CD сборка

В репозитории настроен GitHub Actions workflow (`.github/workflows/build.yml`), который при каждом пуше автоматически собирает и упаковывает релизы для Windows, macOS и Linux.
