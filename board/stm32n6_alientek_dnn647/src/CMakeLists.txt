# ##############################################################################
# STM32N6 Alientek DNN647 board src CMakeLists.txt
# ##############################################################################

set(SRCS stm32_boot.c stm32_bringup.c stm32_appinit.c)

if(CONFIG_ARCH_LEDS)
  list(APPEND SRCS stm32_autoleds.c)
else()
  list(APPEND SRCS stm32_userleds.c)
endif()

target_sources(board PRIVATE ${SRCS})

set_property(GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/flash.ld")
