cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(flashprint C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(btpwmled
    main.c
)

pico_btstack_make_gatt_header(btpwmled INTERFACE "${CMAKE_CURRENT_LIST_DIR}/notify.gatt")

pico_enable_stdio_usb(btpwmled 1)
pico_enable_stdio_uart(btpwmled 0)
pico_add_extra_outputs(btpwmled)

target_include_directories(btpwmled PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR} 
    ${CMAKE_CURRENT_BINARY_DIR}/generated/btpwmled_gatt_header
)

target_link_libraries(
    btpwmled
    pico_cyw43_arch_none
    pico_stdlib
    hardware_pwm
    pico_btstack_ble
    pico_btstack_cyw43
)
