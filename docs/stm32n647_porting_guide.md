# STM32N647 硬件适配说明

## 一、目标板信息

| 项目 | 说明 |
|------|------|
| 开发板 | 正点原子 DNN647 |
| 芯片 | STM32N647X0H3Q（Cortex-M55） |
| SRAM | 4.2MB @ 0x34000000 |
| Flash | 无内部 Flash，使用 DEV Boot 模式 |
| 调试口 | SWD: PA13/PA14 |
| 串口 | USART1: PE5=TX(AF7), PE6=RX(AF7), 经 CH340 转 USB |
| 时钟 | HSI 64MHz 内部（无外部 HSE） |
| LED | PG10 + PE10 |
| Boot 模式 | DEV Boot（BOOT1=1） |

> STM32N647 与 STM32N657 共用 datasheet，SRAM/核心相同，代码复用 STM32N657 配置。

## 二、适配概述

适配工作分三个层面：

1. **芯片层**（`chip/stm32n6/`）：寄存器定义、时钟初始化、GPIO、UART 驱动
2. **板级层**（`board/stm32n6_alientek_dnn647/`）：板级初始化、内存布局、defconfig
3. **Manifest 映射**（`contest2026_463_tongyuanjiang.xml`）：通过 `<linkfile>` 将仓内代码软链到 openvela 编译树

所有改动均在专属仓 `contest2026_463_tongyuanjiang/` 内，不修改 nuttx/apps/vendor 等公共仓库。

## 三、关键配置项

### 3.1 ARCH_CHIP_CUSTOM 机制

openvela 提供 `ARCH_CHIP_CUSTOM` 机制，允许将芯片层代码放在自定义目录：

```kconfig
CONFIG_ARCH_CHIP_CUSTOM=y
CONFIG_ARCH_CHIP_CUSTOM_NAME="stm32n6"
CONFIG_ARCH_CHIP_CUSTOM_DIR="/home/kkk/openvela/contest2026_463_tongyuanjiang/chip/stm32n6"
```

> **踩坑**：`ARCH_CHIP_CUSTOM_DIR` 必须用**绝对路径**。CMake 的 `get_filename_component` 对相对路径解析有 bug，会导致路径拼接错误。

### 3.2 内存布局

```kconfig
CONFIG_RAM_START=0x34000400
CONFIG_RAM_SIZE=4193280
```

SRAM 起始 0x34000000，前 1KB 保留给 M33 向量表，用户空间从 0x34000400 开始，可用 4193280 字节（约 4.0MB）。

### 3.3 串口控制台

```kconfig
CONFIG_STM32_USART1=y
CONFIG_STM32N6_USART1_SERIALDRIVER=y
CONFIG_USART1_SERIAL_CONSOLE=y
CONFIG_USART1_BAUD=115200
```

USART1 时钟源选择 HSI 64MHz，波特率 115200，引脚 PE5(TX)/PE6(RX) 复用 AF7。

## 四、目录结构

```
contest2026_463_tongyuanjiang/
├── chip/stm32n6/                    # 芯片层代码
│   ├── Kconfig                      # 芯片 Kconfig（符号用 STM32N6_ 前缀避免冲突）
│   ├── include/stm32n6/             # 寄存器头文件
│   ├── stm32_start.c                # 时钟/电源初始化
│   ├── stm32_lowputc.c              # 底层 UART 输出
│   ├── stm32_serial.c              # 串口驱动（upper half）
│   ├── stm32_gpio.c                 # GPIO 驱动
│   └── ...
├── board/stm32n6_alientek_dnn647/  # 板级代码
│   ├── configs/nsh/defconfig       # NSH 配置
│   ├── src/                         # 板级初始化源码
│   └── scripts/                     # 链接脚本
├── docs/                            # 适配文档
│   └── stm32n647_porting_guide.md  # 本文档
├── contest2026_463_tongyuanjiang.xml # repo manifest
└── README.md                        # 仓库说明
```

## 五、编译与烧录

### 5.1 编译

```bash
cd ~/openvela
./build.sh vendor/openvela/boards/contest2026_463_board/configs/nsh/ --cmake -j$(nproc)
```

产物：
- `cmake_out/contest2026_463_board_nsh/nuttx.bin`（烧录用二进制）
- `cmake_out/contest2026_463_board_nsh/nuttx`（ELF，调试用）

### 5.2 烧录

STM32N6 无内部 Flash，使用 DEV Boot 模式（BOOT1=1）：

1. **CubeIDE Debug 模式**（推荐）：加载 `nuttx` ELF，通过 SWD 下载到 SRAM 并运行
2. **CubeProgrammer**：只能下载，不能自动运行，需配合 openocd 或手动复位

### 5.3 串口连接

- CH340 TX → PE6 (USART1 RX)
- CH340 RX → PE5 (USART1 TX)
- GND 共地
- 上位机：115200, 8N1, 无流控

## 六、踩坑记录

### 6.1 Kconfig 符号名冲突（最严重）

**现象**：defconfig 中设置 `CONFIG_STM32_USART1_SERIALDRIVER=y`，但编译后 .config 中该符号始终为 `n`，`stm32_serial.c` 不编译，串口无输出。

**根因**：标准 NuttX 的 `nuttx/arch/arm/src/stm32/Kconfig` 中，`STM32_USART1_SERIALDRIVER` 定义在 `choice` 块内。即使 `ARCH_CHIP_STM32` 未设置，kconfiglib 仍解析所有 Kconfig 文件，将该符号标记为 **choice symbol**。对于 choice symbol，`default y` 和 defconfig `=y` 赋值均无效。

**解决**：将自定义 Kconfig 中的符号重命名为 `STM32N6_USART1_SERIALDRIVER`（加前缀避免冲突），同步更新源码中的 `#ifdef` 引用。

### 6.2 ARCH_CHIP_CUSTOM_DIR 相对路径 bug

**现象**：CMake 编译时找不到芯片层源文件。

**根因**：openvela CMake 的 `get_filename_component` 对相对路径解析有 bug。

**解决**：defconfig 中 `ARCH_CHIP_CUSTOM_DIR` 必须用绝对路径。

### 6.3 头文件路径缺失

**现象**：编译报错找不到 `<arch/armv8-m/nvicpri.h>`。

**根因**：openvela fork 的 NuttX 没有 `arch/arm/include/armv8-m` 目录。

**解决**：将 `#include <arch/armv8-m/nvicpri.h>` 改为 `#include <arch/arm_m/nvicpri.h>`。

### 6.4 缺少 board_app_initialize

**现象**：链接报错 `undefined reference to board_app_initialize`。

**解决**：新建 `board/src/stm32_appinit.c`，定义 `board_app_initialize()` 函数，并在 `board/src/CMakeLists.txt` 中添加该源文件。

### 6.5 include 软链接

**现象**：编译找不到 `<arch/chip/irq.h>` 等头文件。

**解决**：在 `chip/stm32n6/` 下创建 `include` 软链接指向 `../include/stm32n6`。

### 6.6 STM32N6 无内部 Flash

**说明**：STM32N6 系列没有片内 Flash，代码必须下载到 SRAM 运行。必须使用 DEV Boot 模式（BOOT1=1），CubeProgrammer 只能下载不能运行，需用 CubeIDE Debug 或 openocd 启动执行。

## 七、当前状态

- [x] L0：系统启动，NSH 串口控制台可用，`nsh>` 提示符正常
- [x] procfs 已启用（`/proc`）
- [x] NSH 内置命令可用（help, ls, cat, free, ps 等）
- [ ] LED 驱动（board.h 中 LED 引脚仍为 Nucleo 默认配置，需更新为 PG10/PE10）
- [ ] 文件系统（RomFS/TmpFS）
- [ ] 网络栈（如板载以太网/WiFi）

## 八、关键文件索引

| 文件 | 说明 |
|------|------|
| `chip/stm32n6/Kconfig` | 芯片 Kconfig，符号用 STM32N6_ 前缀 |
| `chip/stm32n6/stm32_start.c` | 时钟初始化，USART1 时钟源选择 HSI |
| `chip/stm32n6/stm32_lowputc.c` | 底层 UART 输出，波特率计算 |
| `chip/stm32n6/stm32_serial.c` | 串口驱动 upper half |
| `board/.../configs/nsh/defconfig` | 板级配置 |
| `contest2026_463_tongyuanjiang.xml` | manifest，linkfile 映射定义 |