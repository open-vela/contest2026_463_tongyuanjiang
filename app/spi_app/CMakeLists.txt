# Contest 2026 team 463 spi app (shell command for SPI register read/write)
if(CONFIG_LVX_USE_DEMO_CONTEST2026_463_SPI_APP)
  nuttx_add_application(
    NAME spi_app
    SRCS spi_app_main.c
    STACKSIZE 2048)
endif()
