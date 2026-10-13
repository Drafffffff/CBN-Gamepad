# CBN Gamepad

基于 Cataclysm: Bright Nights 最新稳定版 `v0.13.0`（`e7b497637d45`）移植的原生 SDL3 手柄界面。交互参考 [CDDA Gamepad](https://github.com/Drafffffff/CDDA-Gamepad)。已使用 SteamGridDB 的 Bright Nights 专属竖版与横版封面（Purity3556）。

这是修改过的游戏客户端，不是数据模组；输入捕获和原生渲染需要修改客户端。

安装在 `~/Games/cbn-gamepad-stable`，Steam 名称为 **CBN Gamepad**，使用独立存档、配置和运行库。原 CDDA 安装保留。不要直接覆盖 CDDA 存档；两个项目的数据格式和内容已经分化。

## 操作

| 控制 | 行为 |
|---|---|
| 左摇杆＋RT | 按方向移动；RT 单独按下等待 |
| 右摇杆按下 | 查看周围；进入查看后使用方向控制 |
| LT＋右摇杆按下 | 附近物品列表；在列表按 A 进入高级物品管理 |
| A / B / X / Y | 地图中分别为调查、抓取、砸毁、投掷；菜单按实际底栏提示 |
| 十字键 | 当前页面的快捷操作或菜单方向导航 |
| LT＋任一摇杆 | 打开轮盘，LB/RB 切换探索、物品、生存、信息四类 |
| 松开 LT | 执行选项；摇杆回中后松开，或先按 B，均取消 |
| LT＋☰ | 当前页面完整手柄快捷键列表，可进入原键盘设置 |
| LT＋返回键 | 缩小地图；放大地图在“信息”轮盘中 |

底栏直接读取当前 `input_context` 的注册动作及实际绑定。轮盘也只显示当前可用动作，并去除直接按键已覆盖的动作。打开轮盘时捕获输入，地图光标不会跟随移动。新增键名使用 `PAD_` 前缀，避免 BN 共享键名表覆盖原键盘的 `A`、`UP` 等键。

## 屏幕配置

- Bazzite：独立桌面配置，字体单元 8×16、缩放 2、全屏（4K 预设 480×135 个物理字体单元）。4K 不会自动启用紧凑侧栏。
- Steam Deck：提供 `profile-small`，用于先前的 640×360 游戏窗口／480×360 外接显示器；字体单元 8×12。
- 游戏窗口不大于 640×480 时，默认选用 32 列紧凑侧栏，保留状态、小地图和日志。底部两行有独立空间。
- 两台设备使用相同游戏二进制和素材，配置独立。Deck 安装时将小屏预设作为初始 `profile`。

## 便携包首次安装

将包解压到 `~/Games`，执行 `~/Games/cbn-gamepad-stable/install-device.sh small`（Steam Deck）或 `desktop`（Bazzite）。添加 `~/Games/cbn-gamepad-stable/launch.sh` 为 Steam 非 Steam 游戏，名称使用 **CBN Gamepad**。安装器只补齐缺少的配置，已有玩家设置保留。

## 构建和验证

已在 Fedora 43 容器、Clang 21.1.8、SDL3 环境构建游戏和测试程序。使用绝对编译器路径，避免 CMake 更换编译器时清空其他选项。

```sh
cmake -S . -B out/gamepad -G Ninja \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE='-O1 -DNDEBUG' \
  -DTILES=ON -DCURSES=OFF -DSOUND=ON -DTESTS=ON \
  -DUSE_PREFIX_DATA_DIR=OFF -DUSE_HOME_DIR=OFF \
  -DLANGUAGES=zh_CN -DBUILD_SHADERCROSS=OFF -DSHADER_TARGETS=spirv \
  -DLUA_DOCS_ON_BUILD=OFF -DLINKER=mold -DJSON_FORMAT=ON
cmake --build out/gamepad -j10
CATA_TEST_COMPUTE_ACCELERATION=cpu SDL_VIDEODRIVER=dummy \
  SDL_AUDIODRIVER=dummy out/gamepad/tests/cata_test-tiles '[controller]'
```

`BUILD_SHADERCROSS=OFF` 时须提供该版本的预编译 SPIR-V 着色器。此次从同一上游版本的官方 Linux 包复制 `data/shaders`；官方包同时提供图块、音效和已编译汉化。`controller-zh_CN.po` 只覆盖新增界面，使用 `msgcat --use-first` 合并到官方词库。

`prepare-runtime.sh` 整理目录，`bundle-runtime.py` 在构建容器内收集 SDL3、C++ 等依赖，不替换宿主 glibc；当前包要求 glibc ≥2.38。`create-profiles.py` 仅用于首次生成配置，会写入 CBN 预设，已有玩家配置不要重复执行。

`controller-smoke.c` 是仅供隔离测试进程使用的 SDL3 虚拟手柄夹具，截取实际游戏渲染。不得注入玩家正在运行的游戏。验证覆盖帮助、四类轮盘、取消、查看、附近物品和高级物品管理；`CBN_SMOKE_MAP=1` 检查大地图轮盘，`CBN_SMOKE_MENU_ONLY=1` 仅检查主菜单和帮助。

代码和素材继续遵循上游仓库的许可证；未替换上游内容署名。原有系统级屏幕翻译插件独立于本客户端，新增手柄界面使用原生简体中文。
