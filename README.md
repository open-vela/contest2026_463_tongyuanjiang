# openvela STM32N647 DNN647 硬件适配

## 作品名

**openvela 移植至正点原子 DNN647 开发板（STM32N647X0H3Q, Cortex-M55）**

## 简介

本项目将 openvela 操作系统移植至正点原子 DNN647 开发板，完成系统启动引导、UART 控制台、GPIO/LED、I2C、SPI 等基础外设驱动适配，并在硬件上验证通过。移植遵循 openvela manifest + linkfile 架构，板级与芯片层代码均位于本专属仓，通过 linkfile 映射至 openvela 编译树。

## 选题方向

**新硬件适配赛道** — 基于 STM32N647（ARM Cortex-M55）的 openvela BSP 适配。

## 目录结构

```
contest2026_463_tongyuanjiang/
├── contest2026_463_tongyuanjiang.xml   # manifest，引用 openvela.xml + linkfile 映射
├── README.md                            # 本文件
├── docs/
│   └── stm32n647_porting_guide.md      # 适配指南文档
├── chip/
│   ├── include/stm32n6/                 # 芯片头文件 (chip.h, irq.h, stm32n6xx_irq.h)
│   └── stm32n6/                         # 芯片层驱动源码
│       ├── stm32_start.c                # 芯片启动入口
│       ├── stm32_gpio.c                 # GPIO 驱动
│       ├── stm32_serial.c               # UART 串口驱动
│       ├── stm32_irq.c                  # 中断控制
│       ├── stm32n6xx_rcc.c              # 时钟配置 (RCC)
│       ├── stm32_pwr.c                  # 电源管理
│       ├── stm32_idle.c                 # 空闲任务
│       ├── stm32_timerisr.c             # 系统定时器
│       ├── stm32_lowputc.c              # 底层串口输出
│       ├── stm32_rcc.c                  # RCC 辅助
│       ├── hardware/                    # 寄存器定义头文件
│       ├── Kconfig                      # 芯片 Kconfig 配置
│       └── CMakeLists.txt              # 芯片层 CMake
├── board/
│   └── stm32n6_alientek_dnn647/         # 板级代码
│       ├── include/board.h              # 板级配置（时钟、SRAM、LED 引脚）
│       ├── src/
│       │   ├── stm32n6_alientek_dnn647.h  # 板级头文件
│       │   ├── stm32_boot.c              # 启动初始化
│       │   ├── stm32_bringup.c           # 板级 bringup（注册 userleds/i2c0/spi0）
│       │   ├── stm32_autoleds.c          # LED 自动状态指示
│       │   ├── stm32_userleds.c          # 用户 LED 驱动 (/dev/userleds)
│       │   ├── stm32_i2cbitbang.c        # I2C4 bitbang 驱动
│       │   ├── stm32_spibitbang.c        # SPI5 bitbang 驱动
│       │   ├── stm32_appinit.c           # 应用初始化
│       │   └── etc/init.d/               # 启动脚本 (rcS, rc.sysinit)
│       ├── configs/nsh/defconfig        # NSH 配置
│       ├── scripts/flash.ld             # 链接脚本
│       └── CMakeLists.txt              # 板级 CMake（含 vela_nuttx.bin 生成）
├── app/
│   ├── hello_app/                       # 示例应用
│   ├── sensor_app/                      # 陀螺仪 I2C 传感器采集 (QMI8658A)
│   ├── gpio_app/                        # GPIO 读写 shell 命令
│   └── spi_app/                         # SPI 寄存器读写 shell 命令
├── quickapp/
│   └── hello_quickapp/                  # 快速应用示例
└── logs/
    └── README.md                        # AI Coding 日志说明
```

## 硬件信息

| 项目 | 说明 |
|------|------|
| 开发板 | 正点原子 DNN647 |
| MCU | STM32N647X0H3Q |
| 内核 | ARM Cortex-M55 |
| SRAM | 4MB (0x34000000 - 0x343FFFFF) |
| 内部 Flash | 无（代码运行于 SRAM） |
| BOOT 模式 | DEV Boot (BOOT1=1) |
| 调试接口 | ST-Link via SWD |

## 外设适配

| 外设 | 引脚 | 接口 | 设备节点 | 状态 |
|------|------|------|----------|------|
| UART 控制台 | PE5(TX)/PE6(RX), AF7, 115200 | USART1 | /dev/console | 已验证 |
| LED1 (红) | PG10, Active Low | GPIO | /dev/userleds | 已验证 |
| LED2 (绿) | PE10, Active Low | GPIO | /dev/userleds | 已验证 |
| I2C | PE13(SCL)/PE14(SDA), 开漏 | I2C4 bitbang | /dev/i2c0 | 已验证 |
| SPI | PE15(SCK)/PH7(MOSI)/PH8(MISO)/PH6(CS) | SPI5 bitbang | /dev/spi0 | 已验证 |

## 编译

### 环境要求

- openvela 工程已通过 `repo init` + `repo sync` 拉取
- 交叉编译工具链：arm-none-eabi-gcc 13.4.0（openvela 预置）

### 编译命令

```bash
cd ~/openvela
./build.sh vendor/openvela/boards/contest2026_463_board/configs/nsh/ --cmake -j3
```

### 编译产物

| 文件 | 说明 | 路径 |
|------|------|------|
| `vela_nuttx.bin` | 最终固件二进制（openvela 标准命名） | `cmake_out/contest2026_463_board_nsh/vela_nuttx.bin` |
| `nuttx` | ELF 文件（含调试符号，用于调试） | `cmake_out/contest2026_463_board_nsh/nuttx` |
| `System.map` | 符号表 | `cmake_out/contest2026_463_board_nsh/System.map` |

> `vela_nuttx.bin` 通过 NuttX 标准 `nuttx_post_build` 机制自动生成。

## 烧录与运行

STM32N6 无内部 Flash，代码运行于 SRAM。BOOT1=1（DEV Boot 模式）。

### 烧录方式：STM32CubeIDE Debug（已验证）

1. 设置 BOOT1=1（DEV Boot 模式），连接 ST-Link
2. 打开 STM32CubeIDE，配置 Debug Configurations
3. 加载 `nuttx` ELF 文件至 SRAM
4. 点击 Debug，自动设置 PC 和 SP，停于入口
5. 点击 Resume 运行
6. 串口连接：PE5(TX)/PE6(RX) via CH340, 波特率 115200

### 串口验证

成功启动后显示：

```
userleds registered at /dev/userleds
I2C0 registered (bitbang, PE13=SCL, PE14=SDA)
SPI0 registered (bitbang, PE15=SCK, PH7=MOSI, PH8=MISO, PH6=CS)

NuttShell (NSH)
dnn647-ap>
```

> 注：STM32N6 SRAM 执行特性下，`vela_nuttx.bin` 需通过 IDE 设置 PC 后运行，不支持独立 RESET 启动。

## 运行验证

## xTS 认证测试

参考 [openvela xTS 测试用例](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/test_dev_guide/openvela_xts_test_cases.md)，本项目执行了以下测试项：

### 1.1.4 Kernel-ostest 测试

启用 `CONFIG_TESTING_OSTEST=y`，在 NSH 中执行 `ostest`，NuttX 内核综合测试全部通过：

| 测试项 | 结果 | 说明 |
|--------|------|------|
| stdio 标准I/O | PASS | printf/fprintf/stderr |
| 参数传递 | PASS | argc/argv 验证 |
| getopt/getopt_long | PASS | 命令行解析 |
| libc 测试 | PASS | 标准库函数 |
| setvbuf 缓冲 | PASS | 全/行/无缓冲模式 |
| /dev/null | PASS | 零字节读，1024 字节写 |
| **FPU 浮点运算** | PASS | 16 轮双线程 FPU 上下文切换 |
| task_restart | PASS | 任务重启与环境保持 |
| waitpid | PASS | 子进程等待 (3 进程) |
| **mutex 互斥锁** | PASS | 32 轮双线程，0 errors |
| **timed mutex** | PASSED | 超时获取锁 |
| cancel 线程取消 | PASS | 正常/异步/detached/不可取消 |
| robust 健壮锁 | PASS | 0 errors |
| **semaphore 信号量** | PASS | 3 线程优先级链 |
| timed semaphore | PASS | 超时等待 |
| **condition variable** | PASS | 32 轮，0 errors |
| pthread_exit | PASS | 线程退出清理 |
| pthread_rwlock | PASS | 读写锁 |
| timed wait | PASS | 超时条件变量等待 |
| **message queue** | PASS | 10 条消息收发，0 errors |
| timed message queue | PASS | 超时收发 |
| sigprocmask | SUCCESS | 信号掩码 |
| **signal handler** | PASS | 信号中断 sem_wait |
| nested signal handler | PASS | 嵌套信号 3720 次 |
| **spinlock 自旋锁** | PASS | 10000000 次，192678 op/s |

> 结论：NuttX 内核在 STM32N647（Cortex-M55）上运行稳定可靠，调度、锁、信号、消息队列、FPU 上下文切换等核心机制均验证通过。

### 其他已验证项

| xTS 测试项 | 验证方式 | 结果 |
|-----------|---------|------|
| 1.3.6 GPIO 功能测试 | `gpio_app` + `leds` 命令 | PASS — IO 配置/读写正常 |
| 1.3.7 I2C 功能测试 | `i2c dev` + `sensor_app` | PASS — 扫描到 5 设备，QMI8658A 数据采集正常 |
| 1.3.7 SPI 功能测试 | `spi_app probe/test` | PASS — 20/20 传输成功，无卡死/超时 |
| 1.3.10 UART 功能测试 | NSH 控制台 | PASS — 115200 波特率稳定通信 |
| 1.2.3 RAM 资源占用 | SRAM 341KB / 4095KB (8.15%) | PASS |


### 1. GPIO 驱动验证

```
dnn647-ap> leds                    # 启动 LED 周期闪烁（500ms）
dnn647-ap> gpio_app mode PG10 out # 配置引脚为输出
dnn647-ap> gpio_app write PG10 1  # 写高电平
dnn647-ap> gpio_app read PG10     # 读电平
dnn647-ap> gpio_app mode PE14 in pullup  # 配置为输入+上拉
```

### 2. I2C 驱动验证

```
dnn647-ap> i2c bus                # 查看 I2C 总线
dnn647-ap> i2c dev                 # 扫描设备（检测到 0x50 EEPROM, 0x6a QMI8658A 等）
dnn647-ap> sensor_app              # 读取 QMI8658A 陀螺仪数据（WHO_AM_I + 20 次采样）
```

### 3. SPI 驱动验证

```
dnn647-ap> spi_app probe          # 探测 SPI 总线设备
dnn647-ap> spi_app read 0x9F      # 读 JEDEC ID
dnn647-ap> spi_app write 0x01 0x42  # 写寄存器
dnn647-ap> spi_app test           # 稳定性测试（20 次交换，无卡死/超时）
```

## AI Coding 使用说明

本项目在开发过程中使用了 AI 辅助编程工具（TRAE）进行代码生成、调试和文档编写。AI 辅助主要应用于：

- 芯片层驱动代码（GPIO、RCC、PWR、Serial、Timer）编写与调试
- 板级 bringup 代码和 I2C/SPI bitbang 驱动实现
- CMake 构建配置和 manifest linkfile 映射
- 应用层 shell 命令（sensor_app、gpio_app、spi_app）开发
- 编译错误排查和硬件调试问题分析

### AI Coding 日志说明

本项目**未导出 AI 对话日志**，自愿放弃 AI Coding 日志相关评分。`logs/` 目录仅保留说明文件。

## 许可证

Apache License 2.0
