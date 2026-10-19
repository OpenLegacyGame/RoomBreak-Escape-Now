# 自动通关与连续演示

仓库包含全部 30 关的自动操作路线：

| 文件 | 用途 |
| --- | --- |
| [run-main-routes.py](run-main-routes.py) | 每关启动一个新游戏进程，依次验证全部 30 关；失败即停止 |
| [run-menu-demo.py](run-menu-demo.py) | 从标题画面开始，连续完成选关、剧情、解谜、结算和下一关 |
| [VisualProbe.cpp](VisualProbe.cpp) | 各关的物品操作、机关输入、动作小游戏与通关检查 |
| [MainFlowProbe.cpp](MainFlowProbe.cpp) | 主菜单等界面流程的额外检查 |

脚本通过游戏内的操作接口驱动路线。运行时会打开游戏窗口，需要图形桌面；请等待自动操作完成。路线验证以脱出为目标，完整的金钥匙收集方法见[五语言攻略](../walkthrough/zh-Hans.md)。

## 1. 编译带自动操作功能的版本

发行安装包默认关闭 `RB_VISUAL_PROBE`，需要从源码单独编译。下面的 macOS 方案使用 Python 3 和[构建说明](../BUILDING.md)中的工具链；Python 脚本只依赖标准库。

在仓库根目录运行：

```sh
bash scripts/prepare-engine.sh
export AX_ROOT="$PWD/external/axmol"
cmake -S . -B build/macos-routes -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  -DRB_VISUAL_PROBE=ON
cmake --build build/macos-routes --parallel
```

已有准备好的引擎时，将 `AX_ROOT` 指向它即可。PowerShell 7 的 `pwsh` 必须在 `PATH` 中。此处直接使用 CMake；普通 `build-macos.sh` 会显式关闭自动操作功能。

可执行文件是：

```text
build/macos-routes/bin/RoomBreakEscapeNow/RoomBreak Escape Now.app/Contents/MacOS/RoomBreak Escape Now
```

`--binary` 要传入这个内部可执行文件，不能只传 `.app` 目录。其他桌面平台同样需要启用 `RB_VISUAL_PROBE`，并传入实际的可执行文件；本说明的运行验证采用 macOS。

## 2. 逐关验证全部 30 关

在新的终端中，从仓库根目录运行：

```sh
python3 tests/run-main-routes.py \
  --binary "build/macos-routes/bin/RoomBreakEscapeNow/RoomBreak Escape Now.app/Contents/MacOS/RoomBreak Escape Now" \
  --output "build/route-results-01"
```

每关最多等待 720 秒，遇到失败或超时立即停止。全部通过后终端显示 `30/30 PASS`。脚本使用各关独立的新会话，主要使用简体中文，第 6 章第 3 关使用英语。

输出包括：

- `results.json`：每关状态、退出码、耗时、截图路径。
- `progress.txt`：运行中的简短进度。
- `run-info.json`：开始时间、程序路径和 SHA-256。
- `episodeN-stageM/run.log`：对应关卡日志。
- `episodeN-stageM/*clear.png`：通关截图。

每次使用一个**尚不存在的输出目录**。脚本会拒绝覆盖已有目录。运行前清除自行设置的 `RB_ROUTE_DEMO` 环境变量，以免切换为连续演示模式。

## 3. 从主菜单连续演示

```sh
python3 tests/run-menu-demo.py \
  --binary "build/macos-routes/bin/RoomBreakEscapeNow/RoomBreak Escape Now.app/Contents/MacOS/RoomBreak Escape Now" \
  --output "build/menu-demo-01"
```

连续演示包含开场与结尾播放、对话等待和菜单切换，耗时比逐关验证更长。它使用简体中文并开启音乐和音效。新演示存档放在输出目录的 `progress-save.json`，运行日志为 `run.log`；每关截图在各自子目录，状态在 `results.json` 和 `progress.txt`。

全部完成后输出 `30/30 PASS`。可以按 `Ctrl+C` 停止连续演示；若游戏意外停住，查看日志末尾的关卡和步骤。脚本本身没有全程超时限制。

## 4. 只运行一关

单关环境变量使用数据库的章节编号。它与选关画面前 3 章的顺序不同：

| 选关画面章节 | 环境变量中的 EPISODE |
| --- | --- |
| 1 | 3 |
| 2 | 1 |
| 3 | 2 |
| 4 | 4 |
| 5 | 5 |
| 6 | 6 |

例如，运行选关画面的 **第 3 章第 5 关**：

```sh
mkdir -p build/single-3-5
RB_VISUAL_PROBE_OUTPUT="$PWD/build/single-3-5" \
RB_PROBE_EPISODE2_STAGE5=1 \
RB_PROBE_LANGUAGE=0 \
"build/macos-routes/bin/RoomBreakEscapeNow/RoomBreak Escape Now.app/Contents/MacOS/RoomBreak Escape Now" \
  > build/single-3-5/run.log 2>&1
```

使用干净终端，避免残留其他 `RB_PROBE_*` 或 `RB_ROUTE_DEMO` 设置。语言编号：`0` 简体中文、`1` 韩语、`2` 英语、`3` 日语。逐关脚本和连续演示脚本会自行设置语言。

成功日志为 `RB_PROBE_EPISODE2_STAGE5 PASS`，并生成通关截图。只有程序正常退出、出现对应 `PASS`、且保存通关截图，才算该路线验证完成。程序只打开主菜单时，先检查是否编译了 `RB_VISUAL_PROBE=ON`。

## 窗口与安全区域回归

启用 `RB_VISUAL_PROBE=ON` 后，桌面版可运行连续窗口尺寸检查：

```sh
mkdir -p build/viewport-check
RB_VIEWPORT_PROBE="$PWD/build/viewport-check" \
  "build/macos-routes/bin/RoomBreakEscapeNow/RoomBreak Escape Now.app/Contents/MacOS/RoomBreak Escape Now"
```

检查包含不同宽高比、物件点击、按钮边缘位置、F11/Esc 全屏切换、镜头移动途中缩放、拖动边界和留边点击；输出 `checks.txt` 与各尺寸截图。全屏检查需要显示器处于唤醒状态。

追加 `RB_VIEWPORT_SMALL_ROOM=1` 可检查小房间留边；追加 `RB_VIEWPORT_INSETS=40,0,40,16` 可模拟左、上、右、下安全边距（窗口坐标单位）。模拟安全区域仅用于开发构建，设备系统返回的真实安全区域仍需在设备上验证。
