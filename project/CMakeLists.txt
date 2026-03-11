cmake_minimum_required(VERSION 3.20)

project(template LANGUAGES C ASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(USER_DIR "${ROOT_DIR}/user")
set(LIB_DIR "${ROOT_DIR}/Libraries")
set(LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/STM32G431RBTX_FLASH.ld")
set(STARTUP_FILE "${LIB_DIR}/startup/startup_stm32g431xx.s")
set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

file(GLOB HAL_SOURCES CONFIGURE_DEPENDS
    "${LIB_DIR}/hal/stm32g4xx_hal*.c"
)

list(REMOVE_ITEM HAL_SOURCES
    "${LIB_DIR}/hal/stm32g4xx_hal_msp_template.c"
    "${LIB_DIR}/hal/stm32g4xx_hal_timebase_tim_template.c"
)

file(GLOB USER_SOURCES CONFIGURE_DEPENDS
    "${USER_DIR}/*.c"
)

set(SOURCE_FILES
    ${HAL_SOURCES}
    "${LIB_DIR}/hal/stm32g4xx_ll_fmc.c"
    "${LIB_DIR}/hal/stm32g4xx_ll_usb.c"
    "${LIB_DIR}/stm32g4/system_stm32g4xx.c"
    ${STARTUP_FILE}
    ${USER_SOURCES}
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".elf"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${USER_DIR}"
    "${LIB_DIR}/cmsis"
    "${LIB_DIR}/hal"
    "${LIB_DIR}/hal/Legacy"
    "${LIB_DIR}/stm32g4"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    USE_HAL_DRIVER
    STM32G431xx
)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -g3
)

target_link_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -nostartfiles
    -static
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map
    -Wl,--print-memory-usage
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    c
    m
    gcc
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}>
)
