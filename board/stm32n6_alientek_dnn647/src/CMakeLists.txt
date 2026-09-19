# ##############################################################################
# STM32N6 Alientek DNN647 board src CMakeLists.txt
# ##############################################################################

set(SRCS stm32_boot.c stm32_bringup.c stm32_appinit.c)

# autoleds: OS 状态自动指示（IDLE/IRQ/PANIC）
if(CONFIG_ARCH_LEDS)
  list(APPEND SRCS stm32_autoleds.c)
endif()

# userleds: /dev/userleds 设备节点，供 leds 命令和 shell 读写
# 当 ARCH_LEDS=y 且 USERLED_LOWER=y 时两者共存
if(CONFIG_USERLED_LOWER OR NOT CONFIG_ARCH_LEDS)
  list(APPEND SRCS stm32_userleds.c)
endif()

if(CONFIG_I2C_BITBANG)
  list(APPEND SRCS stm32_i2cbitbang.c)
endif()

if(CONFIG_SPI_BITBANG)
  list(APPEND SRCS stm32_spibitbang.c)
endif()

target_sources(board PRIVATE ${SRCS})

# 打包 etc/init.d/ 启动脚本为 ROMFS，自动挂载到 /etc
if(CONFIG_ETC_ROMFS)
  nuttx_add_romfs(
    NAME etc
    MOUNTPOINT /etc
    RCSRCS etc/init.d/rcS etc/init.d/rc.sysinit
    PATH ${CMAKE_CURRENT_SOURCE_DIR}/etc)
  target_link_libraries(board PRIVATE romfs_etc)
endif()

set_property(GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/flash.ld")
