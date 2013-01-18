# 指定驱动模块支持的CPU类型（EMSYS_AT91RM9200，EMSYS_AT91SAM9260，EMSYS_AT91SAM9263）
#CPUARCH = EMSYS_AT91RM9200
#CPUARCH = EMSYS_AT91SAM9260
CPUARCH = EMSYS_AT91SAM9263

# 指定库名称
LIBSNAME = EMZY_ARM_ODM

# 指定库程序包含的功能组件
#库包含串口操作相关函数（若不需要，则#注释该选项）
CFG_USART_LIB=y

#库包含ADC操作相关函数（若不需要，则#注释该选项）
CFG_ADC_LIB=y

#库包含DAC操作相关函数（若不需要，则#注释该选项）
CFG_DAC_LIB=y

#库包含GPIO端口操作相关函数（若不需要，则#注释该选项）
CFG_GPIO_LIB=y

#库包含总线端口操作相关函数（若不需要，则#注释该选项）
CFG_PORT_LIB=y

#库包含日期/时间操作相关函数（若不需要，则#注释该选项）
CFG_DATETIM_LIB=y

#库包含定时器操作相关函数（若不需要，则#注释该选项）
CFG_TIMER_LIB=y

#库包含高精度定时器操作相关函数（若不需要，则#注释该选项，注需硬件支持）
CFG_HIGHTIMER_LIB=y

#库包含加密芯片操作相关函数（若不需要，则#注释该选项）
#CFG_SERIALID_LIB=y

#库包含温度传感器操作相关函数（若不需要，则#注释该选项）
CFG_TEMPER_LIB=y

#库包含PWM输出操作相关函数（若不需要，则#注释该选项）
#CFG_PWM_LIB=y

#库包含PI采样操作相关函数（若不需要，则#注释该选项）
#CFG_PULSEI_LIB=y

#库包含看门狗操作相关函数（若不需要，则#注释该选项）
CFG_WDT_LIB=y

#库包含自定义键盘操作相关函数（若不需要，则#注释该选项）
#CFG_KBD_LIB=y
