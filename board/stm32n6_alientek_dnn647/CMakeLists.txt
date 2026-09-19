# ##############################################################################
# STM32N6 Alientek DNN647 board CMakeLists.txt
# ##############################################################################

add_subdirectory(src)

# Generate vela_nuttx.bin (openvela official naming standard)
# Porting guide requires: vela_nuttx.bin as final firmware binary.
#
# NuttX provides a standard post-build mechanism:
# Top-level CMakeLists.txt line 672 checks if(TARGET nuttx_post_build),
# then: add_dependencies(nuttx_post_build nuttx_post)
#       add_custom_target(post_build ALL DEPENDS nuttx_post_build)
#
# Build chain:
#   nuttx (ELF) -> nuttx-bin (generates nuttx.bin) -> nuttx_post
#   -> nuttx_post_build (copies nuttx.bin to vela_nuttx.bin) -> post_build (ALL)
add_custom_target(nuttx_post_build
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_BINARY_DIR}/nuttx.bin
          ${CMAKE_BINARY_DIR}/vela_nuttx.bin
  COMMENT "Generating vela_nuttx.bin")
