# Source/minGameplayFramework

本目录是本项目的 **gameplay 代码根**，也是反射 codegen 的 `SRC_ROOT`（见根目录 `CMakeLists.txt`
里的 `minengine_add_reflection_codegen(... SRC_ROOT "${GAME_MODULE_SRC_ROOT}" ...)`）。

约定：

- 带 `ME_CLASS` / `ME_STRUCT` 等反射元数据的头文件必须放在本目录（含任意子目录）下，
  codegen 会递归扫描 `*.h` / `*.hpp`；放在别处不会被收集，生成的注册表里也就没有对应类型。
- 新增加的 `.cpp` 需要显式加入根目录 `CMakeLists.txt` 中 `add_library(${GAME_MODULE_NAME} ...)`
  的源文件列表（当前含 `Module.cpp` 与 `GameInstance/MinGameInstance.cpp`）。
- 生成产物写入仓库根的 `Generated/Reflection/`，不要手工编辑，也不要提交为手写源文件。
- 本目录对游戏宿主与编辑器宿主都可见：`target_include_directories` 以 `PUBLIC` 暴露本目录。
