# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(infrared C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME} 
    barcode.c
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_adc 
)

# target_link_libraries(${PROJECT_NAME} 
#     # boot_stage2
#   
#   hardware_base
#   hardware_claim
#   hardware_clocks
#   hardware_divider
#   hardware_dma
#   hardware_exception
#   hardware_flash
#   hardware_gpio
#   hardware_i2c
#   hardware_interp
#   hardware_irq
#   hardware_pio
#   hardware_pll
#   hardware_pwm
#   hardware_resets
#   hardware_rtc
#   hardware_spi
#   hardware_sync
#   hardware_timer
#   hardware_uart
#   hardware_vreg
#   hardware_watchdog
#   hardware_xosc
#   pico_bit_ops
#   pico_bootrom
#   pico_bootsel_via_double_reset
#   pico_cxx_options
#   pico_divider
#   pico_double
# # pico_fix
#   pico_float
#   pico_int64_ops
#   pico_malloc
#   pico_mem_ops
#   pico_multicore
#   pico_platform
#   pico_printf
#   pico_runtime
#   pico_standard_link
#   pico_stdio
# # pico_stdio_semihosting
# # pico_stdio_uart
# # pico_stdio_usb
#   pico_stdlib
#   pico_unique_id  
# )

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)