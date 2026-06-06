################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
rtos_hooks.obj: ../middleware/freertos/rtos_hooks.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'MSP430 Compiler - building file: "$<"'
	"D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="D:/ccs/ccs/ccs_base/msp430/include" --include_path="E:/code/ccs/GPIO/LED" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos/FreeRTOS-Kernel/include" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X" --include_path="D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --define=__MSP430F5529__ -g --opt_level=3 --opt_for_speed=0 --gen_data_subsections=on --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="rtos_hooks.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

list.obj: ../middleware/freertos/FreeRTOS-Kernel/list.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'MSP430 Compiler - building file: "$<"'
	"D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="D:/ccs/ccs/ccs_base/msp430/include" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos/FreeRTOS-Kernel/include" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X" --include_path="D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --define=__MSP430F5529__ -g --opt_level=3 --opt_for_speed=0 --gen_data_subsections=on --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="list.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

tasks.obj: ../middleware/freertos/FreeRTOS-Kernel/tasks.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'MSP430 Compiler - building file: "$<"'
	"D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="D:/ccs/ccs/ccs_base/msp430/include" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos/FreeRTOS-Kernel/include" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X" --include_path="D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --define=__MSP430F5529__ -g --opt_level=3 --opt_for_speed=0 --gen_data_subsections=on --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="tasks.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

port.obj: ../middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X/port.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'MSP430 Compiler - building file: "$<"'
	"D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="D:/ccs/ccs/ccs_base/msp430/include" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos/FreeRTOS-Kernel/include" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X" --include_path="D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --define=__MSP430F5529__ -g --opt_level=3 --opt_for_speed=0 --gen_data_subsections=on --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="port.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

portext.obj: ../middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X/portext.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'MSP430 Compiler - building file: "$<"'
	"D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="E:/code/ccs/GPIO/LED/middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X" --define=__MSP430F5529__ "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'MSP430 Compiler - building file: "$<"'
	"D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="D:/ccs/ccs/ccs_base/msp430/include" --include_path="E:/code/ccs/GPIO/LED" --include_path="D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --define=__MSP430F5529__ -g --opt_level=3 --opt_for_speed=0 --gen_data_subsections=on --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.cpp $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'MSP430 Compiler - building file: "$<"'
	"D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="D:/ccs/ccs/ccs_base/msp430/include" --include_path="E:/code/ccs/GPIO/LED" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos/FreeRTOS-Kernel/include" --include_path="E:/code/ccs/GPIO/LED/middleware/freertos/FreeRTOS-Kernel/portable/CCS/MSP430X" --include_path="D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --define=__MSP430F5529__ -g --opt_level=3 --opt_for_speed=0 --gen_data_subsections=on --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


