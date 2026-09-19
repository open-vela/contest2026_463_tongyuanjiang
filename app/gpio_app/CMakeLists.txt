# Contest 2026 team 463 gpio app (shell command for GPIO read/write/mode)
if(CONFIG_LVX_USE_DEMO_CONTEST2026_463_GPIO_APP)
  nuttx_add_application(
    NAME gpio_app
    SRCS gpio_app_main.c
    STACKSIZE 2048
    INCLUDE_DIRECTORIES
      ${NUTTX_DIR}/../contest2026_463_tongyuanjiang/chip/stm32n6)
endif()
