################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../application/rtos_tasks.cpp \
../main.cpp

ASM_SRCS += \
../middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X/portext.asm

CMD_SRCS += \
../lnk_msp430f5529.cmd 

C_SRCS += \
../middleware/app_resources.c \
../application/app_state.c \
../drivers/board.c \
../application/buttons.c \
../application/epaper.c \
../middleware/flash_log.c \
../middleware/format.c \
../middleware/sd_assets.c \
../drivers/sensors.c \
../application/serial_control.c \
../middleware/text_reader.c \
../drivers/uart.c \
../middleware/freertos/rtos_hooks.c \
../middleware/freertos/FreeRTOS-Kernel/list.c \
../middleware/freertos/FreeRTOS-Kernel/tasks.c \
../middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X/port.c

C_DEPS += \
./app_resources.d \
./app_state.d \
./board.d \
./buttons.d \
./epaper.d \
./flash_log.d \
./format.d \
./sd_assets.d \
./sensors.d \
./serial_control.d \
./text_reader.d \
./uart.d \
./rtos_hooks.d \
./list.d \
./tasks.d \
./port.d

OBJS += \
./app_resources.obj \
./app_state.obj \
./board.obj \
./buttons.obj \
./epaper.obj \
./flash_log.obj \
./format.obj \
./main.obj \
./rtos_tasks.obj \
./sd_assets.obj \
./sensors.obj \
./serial_control.obj \
./text_reader.obj \
./uart.obj \
./rtos_hooks.obj \
./list.obj \
./tasks.obj \
./port.obj \
./portext.obj

CPP_DEPS += \
./rtos_tasks.d \
./main.d

OBJS__QUOTED += \
"app_resources.obj" \
"app_state.obj" \
"board.obj" \
"buttons.obj" \
"epaper.obj" \
"flash_log.obj" \
"format.obj" \
"main.obj" \
"rtos_tasks.obj" \
"sd_assets.obj" \
"sensors.obj" \
"serial_control.obj" \
"text_reader.obj" \
"uart.obj" \
"rtos_hooks.obj" \
"list.obj" \
"tasks.obj" \
"port.obj" \
"portext.obj"

C_DEPS__QUOTED += \
"app_resources.d" \
"app_state.d" \
"board.d" \
"buttons.d" \
"epaper.d" \
"flash_log.d" \
"format.d" \
"sd_assets.d" \
"sensors.d" \
"serial_control.d" \
"text_reader.d" \
"uart.d" \
"rtos_hooks.d" \
"list.d" \
"tasks.d" \
"port.d"

CPP_DEPS__QUOTED += \
"rtos_tasks.d" \
"main.d"

C_SRCS__QUOTED += \
"../middleware/app_resources.c" \
"../application/app_state.c" \
"../drivers/board.c" \
"../application/buttons.c" \
"../application/epaper.c" \
"../middleware/flash_log.c" \
"../middleware/format.c" \
"../middleware/sd_assets.c" \
"../drivers/sensors.c" \
"../application/serial_control.c" \
"../middleware/text_reader.c" \
"../drivers/uart.c" \
"../middleware/freertos/rtos_hooks.c" \
"../middleware/freertos/FreeRTOS-Kernel/list.c" \
"../middleware/freertos/FreeRTOS-Kernel/tasks.c" \
"../middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X/port.c"

CPP_SRCS__QUOTED += \
"../application/rtos_tasks.cpp" \
"../main.cpp"


