# STM32G431RBT6-Template

一个面向 VSCode 工作流的 STM32G431RBT6 裸机模板工程。

这个模板基于 STM32 HAL，使用 CMake + Ninja 构建，使用 OpenOCD 进行烧录，使用 `cortex-debug` 进行调试。工程目标不是做代码生成器，而是提供一套已经整理好目录、启动链路、构建链路和调试入口的可维护起点。

## 1. 项目定位

本工程适合作为 STM32G431RBT6 新项目的基础模板，特点是：

- 用户代码与官方库分层清晰
- 默认可在 VSCode 中直接执行配置、编译、烧录、调试
- 保留完整 HAL 驱动源码，方便后续按需启用外设
- 默认提供 `syscalls.c` 和 `sysmem.c`，便于 newlib 裸机运行
- 默认启动时钟简单可控，便于先把工程跑通再扩展

当前模板只描述和提供仓库中已经存在的能力，不包含 CubeMX、FreeRTOS、CubeIDE 工程生成流程，也不承诺自动外设生成或板级 BSP 封装。

## 2. 特性概览

- 芯片目标：`STM32G431RBT6`
- 工具链：`arm-none-eabi-gcc`
- 构建系统：`CMake` + `Ninja`
- 调试链路：`OpenOCD` + VSCode `cortex-debug`
- 默认系统时钟：`HSI 16 MHz`
- 默认 HAL 时基：`SysTick`
- 默认构建产物：`elf`、`hex`、`bin`、`map`
- 默认烧录脚本：`cmsis-dap.cfg + stm32g4x.cfg`

## 3. 目录结构

```text
STM32G431RBT6-Template/
├── .vscode/
│   ├── tasks.json
│   ├── launch.json
│   └── settings.json
├── Libraries/
│   ├── cmsis/
│   ├── hal/
│   ├── startup/
│   └── stm32g4/
├── project/
│   ├── CMakeLists.txt
│   ├── arm-gnu-toolchain.cmake
│   ├── STM32G431RBTX_FLASH.ld
│   └── STM32G431.svd
├── user/
│   ├── main.c
│   ├── main.h
│   ├── stm32g4xx_hal_conf.h
│   ├── stm32g4xx_hal_msp.c
│   ├── stm32g4xx_it.c
│   ├── stm32g4xx_it.h
│   ├── syscalls.c
│   └── sysmem.c
└── README.md
```

各目录职责如下：

- `user/`
  放用户层代码。你自己的应用逻辑、外设初始化、中断处理、运行库重定向都应优先放在这里。
- `Libraries/`
  放 ST 官方库和启动文件。通常视为第三方依赖层，除非升级或修复库缺失，不建议频繁改动。
- `project/`
  放构建层配置，包括 CMake、工具链文件、链接脚本和调试用 SVD 文件。
- `.vscode/`
  放 VSCode 任务和调试配置，是日常开发入口。

## 4. 架构设计

### 4.1 分层原则

本模板的核心设计思想是“用户层”和“官方库层”分离：

- `user/` 负责业务代码和板级初始化
- `Libraries/` 负责 CMSIS、HAL、启动文件和芯片头文件
- `project/` 负责把两者组装成一个可执行目标

这种分层的好处是：

- 升级 HAL 或 CMSIS 时影响范围更清晰
- 用户代码不容易和第三方库混在一起
- 迁移到新项目时可以整体复用 `user/ + project/`

### 4.2 启动链路

默认启动链路如下：

```text
startup_stm32g431xx.s
  -> Reset_Handler
  -> SystemInit()                  // Libraries/stm32g4/system_stm32g4xx.c
  -> __libc_init_array()           // C 运行时初始化
  -> main()                        // user/main.c
```

进入 `main()` 后的默认流程是：

```text
HAL_Init()
  -> HAL_MspInit()                 // user/stm32g4xx_hal_msp.c
SystemClock_Config()              // user/main.c
while (1)
```

### 4.3 默认时钟策略

为了降低模板复杂度，当前默认时钟基线是：

- 使用 `HSI`
- 主频 `16 MHz`
- 不启用 `PLL`

对应实现位于 `user/main.c` 中的 `SystemClock_Config()`。如果你后续要切换到外部晶振、PLL 倍频或更高主频，也应从这里改起。

### 4.4 中断与 MSP

- 中断入口声明在 `user/stm32g4xx_it.h`
- 中断处理函数在 `user/stm32g4xx_it.c`
- 全局 MSP 初始化在 `user/stm32g4xx_hal_msp.c`

当前模板默认使用 HAL 的 `SysTick` 作为时基，没有引入 TIM6 时间基模板。这意味着：

- `HAL_Delay()` 等基础 HAL 时间函数可以直接工作
- 你不需要先处理定时器时间基迁移问题
- 如果后续要自定义 HAL Tick，再单独替换即可

### 4.5 运行库与重定向

模板默认保留了两个运行库相关文件：

- `user/syscalls.c`
- `user/sysmem.c`

作用分别是：

- `syscalls.c` 提供 newlib 所需的基础系统调用桩函数
- `sysmem.c` 提供 `_sbrk()`，用于堆空间分配

如果你想支持 `printf` 重定向串口，可以实现这两个弱符号：

```c
int __io_putchar(int ch);
int __io_getchar(void);
```

## 5. 构建设计

### 5.1 CMake 负责什么

真正的编译和链接逻辑在 `project/CMakeLists.txt`，不是在 VSCode 任务里。

当前 CMake 做了这些事情：

- 指定目标语言为 `C` 和 `ASM`
- 指定目标芯片宏 `STM32G431xx`
- 指定 `cortex-m4 + hard-float` 编译参数
- 收集 `Libraries/hal` 下全部 `stm32g4xx_hal*.c`
- 显式排除 HAL 模板源文件
- 额外编译 `stm32g4xx_ll_fmc.c` 和 `stm32g4xx_ll_usb.c`
- 引入系统文件、启动文件和 `user/` 源码
- 使用 `STM32G431RBTX_FLASH.ld` 作为链接脚本
- 构建完成后生成 `elf`、`hex`、`bin` 和 `map`

默认目标文件名是：

- `build/template.elf`
- `build/template.hex`
- `build/template.bin`
- `build/template.map`

### 5.2 为什么会看到内存占用和 size 输出

构建结束时，你会在终端中看到类似这样的信息：

```text
Memory region         Used Size  Region Size  %age Used
...
text    data     bss     dec     hex filename
...
```

这是模板有意保留的后处理输出，用来帮助你快速观察：

- Flash 占用
- RAM 占用
- 最终 `elf` 的段大小

来源是两部分：

- 链接参数中的 `-Wl,--print-memory-usage`
- 构建后的 `arm-none-eabi-size --format=berkeley`

如果你不想显示这两段输出，可以在 `project/CMakeLists.txt` 中删除对应配置。

## 6. 环境要求

建议至少准备以下工具：

- `CMake`
- `Ninja`
- `arm-none-eabi-gcc`
- `OpenOCD`
- `VSCode`

推荐安装的 VSCode 扩展：

- `ms-vscode.cmake-tools`
- `marus25.cortex-debug`
- `llvm-vs-code-extensions.vscode-clangd`

编译器最好能直接在 `PATH` 中找到。当前工具链文件会优先通过 `find_program()` 查找 `arm-none-eabi-gcc`、`arm-none-eabi-objcopy` 和 `arm-none-eabi-size`。如果你使用自定义安装位置，也可以在 `project/arm-gnu-toolchain.cmake` 中固定路径。

## 7. 快速开始

### 7.1 用 VSCode 打开工程

直接用 VSCode 打开仓库根目录：

```text
STM32G431RBT6-Template/
```

### 7.2 配置工程

执行任务：

```text
CMake Configure
```

它会等价执行：

```bash
cmake \
  -DCMAKE_TOOLCHAIN_FILE=${workspaceFolder}/project/arm-gnu-toolchain.cmake \
  -DCMAKE_SYSTEM_NAME=Generic \
  -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
  -GNinja \
  -S project \
  -B build
```

执行成功后会生成 `build/` 目录。

### 7.3 编译工程

执行任务：

```text
CMake Build
```

成功后可在 `build/` 中看到：

```text
template.elf
template.hex
template.bin
template.map
```

## 8. VSCode 全流程教程

### 8.1 Configure

任务名：

```text
CMake Configure
```

用途：

- 生成 Ninja 构建文件
- 检查工具链、链接脚本和源码组织是否正确

常见失败点：

- `arm-none-eabi-gcc` 找不到
- `CMake` 或 `Ninja` 未安装
- 工具链文件路径写错

### 8.2 Build

任务名：

```text
CMake Build
```

用途：

- 编译所有源文件
- 链接生成 `template.elf`
- 后处理生成 `hex/bin/map`

预期结果：

- `build/template.elf` 存在
- 终端输出内存占用和 size 信息

### 8.3 Flash

任务名：

```text
Flash
```

默认使用：

- 接口脚本：`interface/cmsis-dap.cfg`
- 目标脚本：`target/stm32g4x.cfg`

等价命令：

```bash
openocd \
  -f interface/cmsis-dap.cfg \
  -f target/stm32g4x.cfg \
  -c "program build/template.elf verify reset exit"
```

如果你的调试器不是 CMSIS-DAP，或者你的硬件接法不是当前这套 OpenOCD 配置，需要自行修改 `.vscode/tasks.json`。

### 8.4 Debug

调试配置名：

```text
Debug with OpenOCD
```

默认行为：

- 使用 `build/template.elf` 作为调试目标
- 使用 `project/STM32G431.svd` 提供寄存器视图
- 在 `main` 处停下

如果使用这套调试配置，请确认：

- OpenOCD 能正常连接目标板
- `cmsis-dap.cfg` 与你的调试器匹配
- 芯片确实是 G4 系列目标

## 9. CLI 备用命令

如果你不想依赖 VSCode，也可以直接使用命令行：

### 9.1 配置

```bash
cmake \
  -DCMAKE_TOOLCHAIN_FILE=project/arm-gnu-toolchain.cmake \
  -DCMAKE_SYSTEM_NAME=Generic \
  -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
  -GNinja \
  -S project \
  -B build
```

### 9.2 编译

```bash
cmake --build build --target all
```

### 9.3 烧录

```bash
openocd \
  -f interface/cmsis-dap.cfg \
  -f target/stm32g4x.cfg \
  -c "program build/template.elf verify reset exit"
```

## 10. 扩展指南

### 10.1 往哪里写业务代码

优先把你的代码放进 `user/`，不要直接散落到 `Libraries/` 里。

推荐做法：

- 新增业务模块：`user/xxx.c`、`user/xxx.h`
- 在 `main.c` 中做系统级初始化和主循环调度
- 在 `stm32g4xx_it.c` 中添加中断响应
- 在 `stm32g4xx_hal_msp.c` 中补充外设底层时钟和 GPIO 初始化

### 10.2 如何启用新的 HAL 模块

如果你要使用某个 HAL 外设模块，需要至少检查两件事：

- `user/stm32g4xx_hal_conf.h` 中是否启用了对应 `HAL_xxx_MODULE_ENABLED`
- 你的用户代码里是否真正完成了外设初始化

注意：模板当前保留的是“完整 HAL 源码 + 用户按需启用”的思路，不代表每个外设已经完成板级初始化。

### 10.3 如何修改系统时钟

修改 `user/main.c` 中的：

```c
static void SystemClock_Config(void);
```

当前模板默认是最简单的 `HSI 16 MHz`。如果你后续要改为：

- 外部晶振
- PLL 倍频
- 更高主频

都应该从这个函数开始改。

### 10.4 如何做 printf 重定向

实现 `user/syscalls.c` 中依赖的弱符号即可：

```c
int __io_putchar(int ch);
int __io_getchar(void);
```

最常见的做法是把 `__io_putchar()` 对接到 UART 发送函数。

## 11. 常见问题

### 11.1 提示找不到 arm-none-eabi-gcc

原因通常是：

- 工具链未安装
- 工具链未加入 `PATH`
- `project/arm-gnu-toolchain.cmake` 没有找到你的安装位置

优先建议把 ARM GNU Toolchain 加入 `PATH`。

### 11.2 OpenOCD 连不上板子

先检查：

- 调试器是否真的是 CMSIS-DAP
- 接口脚本是否需要改成 ST-Link、J-Link 等其他配置
- 目标板供电是否正常
- SWD/JTAG 连线是否正确

当前模板的默认脚本不保证适配所有探针。

### 11.3 为什么 printf 没有输出

因为模板只提供了系统调用桩，不会自动帮你绑定串口。

你还需要：

- 初始化串口
- 实现 `__io_putchar()`
- 如果需要输入，再实现 `__io_getchar()`

### 11.4 为什么构建后会打印内存占用信息

这是模板故意保留的功能，方便观察程序尺寸，不是编译报错。

如果你不喜欢，可以删掉 `project/CMakeLists.txt` 中：

- `-Wl,--print-memory-usage`
- `arm-none-eabi-size --format=berkeley`

### 11.5 为什么 Libraries/hal 下有很多文件，但不是全部都参与构建

模板当前策略是：

- 默认编译所有 `stm32g4xx_hal*.c`
- 排除 HAL 模板源文件
- 额外只补少量 HAL 依赖的 LL 源文件

这样可以在保留完整 HAL 能力的同时，避免把整套 LL 模块一起拖进默认构建。

## 12. 推荐使用方式

如果你准备基于这个模板开始一个新项目，建议按下面顺序推进：

1. 先确认 `CMake Configure` 和 `CMake Build` 能通过
2. 再确认 `Flash` 和 `Debug with OpenOCD` 与你的探针匹配
3. 然后只在 `user/` 中增加业务模块
4. 最后再逐步替换默认时钟、串口日志、外设初始化

这样能最大限度避免“工程骨架没稳住就开始堆业务代码”的问题。
