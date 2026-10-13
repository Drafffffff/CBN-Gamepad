# CBN Gamepad Stable

基于 Cataclysm: Bright Nights **v0.13.0** 的非官方原生手柄分支，参考 [CDDA Gamepad](https://github.com/Drafffffff/CDDA-Gamepad)。手柄移植仍为预览版。

下载 [Releases](https://github.com/Drafffffff/CBN-Gamepad/releases) 中的 Linux x86_64 便携包，解压到可写目录，运行 `./install-device.sh desktop`（Bazzite）或 `small`（小屏预设），然后运行 `./launch.sh`。将启动脚本添加为 Steam 非 Steam 游戏，使用原生 Linux 运行和标准 Gamepad 模板。程序要求 glibc 2.38 或更新及兼容系统库。

配置与存档在包内 `profile/`，首次启动自动补齐配置，已有配置保留。不要覆盖 CDDA 存档。

左摇杆预选八方向邻格，RT 确认移动，回中时 RT 等待；A 按所选格执行可用交互；LT 加摇杆打开四分类轮盘，LB/RB 切换，松开 LT 执行，B 取消；LT + Start 打开完整动作列表。各页面以底栏提示为准。

本版修复松键排队、旧方向连发、失焦残留输入及手柄热插拔后底栏遮挡日志；左摇杆选格时优先显示邻格光标，回中后恢复右摇杆平移产生的视野中心标记；桌面侧栏采用状态、放大小地图、方位、日志的排列。游戏和测试程序构建通过，控制器测试 10 个用例、2231 项断言通过。Bazzite 实机已验证基础操作；小屏预设尚未在本版 Steam Deck 实机复测。车辆完整驾驶/维修、全部战斗装填组合、模组特殊交互及中文屏幕键盘仍需完整验证。

[详细操作与构建说明](build-scripts/gamepad/README.zh-CN.md)。游戏和素材保留上游许可证与署名。封面可在 [SteamGridDB](https://www.steamgriddb.com/game/5470558) 找到，作者 Purity3556。
