# Contest 2026 team 463 sensor app (QMI8658A)
if(CONFIG_LVX_USE_DEMO_CONTEST2026_463_SENSOR_APP)
  nuttx_add_application(
    NAME sensor_app
    SRCS sensor_app_main.c
    STACKSIZE 4096)
endif()
