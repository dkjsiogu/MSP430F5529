################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
fatfs/%.obj: ../fatfs/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'MSP430 Compiler - building file: "$<"'
	"D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="D:/ccs/ccs/ccs_base/msp430/include" --include_path="E:/code/ccs/GPIO/LED" --include_path="E:/code/ccs/GPIO/LED/application" --include_path="E:/code/ccs/GPIO/LED/drivers" --include_path="E:/code/ccs/GPIO/LED/middleware" --include_path="D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --define=__MSP430F5529__ -g --opt_level=3 --opt_for_speed=0 --gen_data_subsections=on --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="fatfs/$(basename $(<F)).d_raw" --obj_directory="fatfs" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


