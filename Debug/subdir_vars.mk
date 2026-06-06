################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../application/FirmwareApp.cpp \
../main.cpp

ASM_SRCS += \
../middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X/portext.asm

CMD_SRCS += \
../lnk_msp430f5529.cmd 

C_SRCS += \
../app_resources.c \
../app_state.c \
../board.c \
../buttons.c \
../epaper.c \
../flash_log.c \
../format.c \
../sd_assets.c \
../sensors.c \
../serial_control.c \
../text_reader.c \
../uart.c \
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
./FirmwareApp.obj \
./main.obj \
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
./FirmwareApp.d \
./main.d

OBJS__QUOTED += \
"app_resources.obj" \
"app_state.obj" \
"board.obj" \
"buttons.obj" \
"epaper.obj" \
"flash_log.obj" \
"format.obj" \
"FirmwareApp.obj" \
"main.obj" \
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
"FirmwareApp.d" \
"main.d"

C_SRCS__QUOTED += \
"../app_resources.c" \
"../app_state.c" \
"../board.c" \
"../buttons.c" \
"../epaper.c" \
"../flash_log.c" \
"../format.c" \
"../sd_assets.c" \
"../sensors.c" \
"../serial_control.c" \
"../text_reader.c" \
"../uart.c" \
"../middleware/freertos/rtos_hooks.c" \
"../middleware/freertos/FreeRTOS-Kernel/list.c" \
"../middleware/freertos/FreeRTOS-Kernel/tasks.c" \
"../middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X/port.c"

CPP_SRCS__QUOTED += \
"../application/FirmwareApp.cpp" \
"../main.cpp"


