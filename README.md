# D11_SA
<img width="1922" height="1112" alt="{91C40C1D-1479-4AA6-84D7-58CFC08A2CAB}" src="https://github.com/user-attachments/assets/6448af0c-fd39-4c9d-96c3-89f5c13d65b3" />


## Русский

**Платформа:** 64-bit (**x64**) **Windows**, рендер **Direct3D 11**.

**Сборка:** решение под **Visual Studio Version 17** (см. `Build/D11_SA.sln`), набор инструментов **v145**, конфигурации **Debug|x64** и **Release|x64**.

**Сторонние зависимости (vendor):**

| Компонент | Источник |
|-----------|----------|
| **discord-rpc** | [multitheftauto/discord-rpc](https://github.com/multitheftauto/discord-rpc) |
| **gtatools** (`Vendor/gtatools-master`, upstream master) | [alemariusnexus/gtatools](https://github.com/alemariusnexus/gtatools) |
| **Dear ImGui 1.92.7** | [ocornut/imgui](https://github.com/ocornut/imgui) |

**Задача проекта:** поэтапно **воссоздать сцену GTA San Andreas** на своём рендере — в перспективе **погода**, **визуальные эффекты** и остальное окружение, характерное для игры; развитие идёт от загрузки данных карты и объектов к более полному совпадению с оригиналом.

**Сейчас реализовано:**

- **Парсинг данных:** **DFF**, **IDE**, **IPL**, **TXD** (текстурные архивы) и связанные форматы/цепочки загрузки (в т.ч. обход `gta.dat`, **IMG** и пр.).
- **Сцена:** загрузка **моделей в сцену** и вывод в **рендер** (геометрия из потока данных игры).
- **Текстуры на моделях:** в основном работает **наложение текстур**; известны **редкие баги UV** (развёртка/карта координат может отображаться некорректно) — заметно в частности на **пользовательских картах**.
- **Не сделано:** **альфа-прозрачность** текстур на моделях (прозрачные материалы как в игре пока не воспроизводятся).

**Управление:** свободная **камера** (и интерфейс отладки на ImGui).

---

## English

**Platform:** **x64** **Windows**, rendering via **Direct3D 11**.

**Build:** solution targets **Visual Studio Version 17** (see `Build/D11_SA.sln`), toolset **v145**, configurations **Debug|x64** and **Release|x64**.

**Third-party (vendor) dependencies:**

| Component | Source |
|-----------|--------|
| **discord-rpc** | [multitheftauto/discord-rpc](https://github.com/multitheftauto/discord-rpc) |
| **gtatools** (`Vendor/gtatools-master`, upstream master) | [alemariusnexus/gtatools](https://github.com/alemariusnexus/gtatools) |
| **Dear ImGui 1.92.7** | [ocornut/imgui](https://github.com/ocornut/imgui) |

**Project goal:** iteratively **rebuild the GTA San Andreas scene** on a custom renderer — toward **weather**, **visual effects**, and other world features over time, starting from map/object data and moving closer to the original experience.

**Current implementation:**

- **Data parsing:** **DFF**, **IDE**, **IPL**, **TXD** (texture dictionaries), and related loading paths (including `gta.dat`, **IMG**, etc.).
- **Scene:** **models are loaded into the scene** and **drawn** by the renderer.
- **Texturing:** **texture mapping on meshes** largely works; **occasional UV/layout bugs** (wrong mapping) show up rarely — notably on some **custom maps** such as **MTA Province**.
- **Not implemented:** **alpha transparency** for textures on models (transparent materials are not reproduced yet).

**Controls:** free **camera** (plus ImGui debugging UI).



