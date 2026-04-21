# D11_SA

<img width="1280" height="696" alt="photo_2026-04-20_00-44-40" src="https://github.com/user-attachments/assets/17ed60d8-4158-4f00-abe4-d9b2775012fc" />

English | [Русский](https://github.com/multimaks2/D11_SA/README-ru.md)

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



