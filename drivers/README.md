# 底层驱动层

`drivers/` 放当前平台的 C 驱动实现，允许直接访问 MSP430F5529 寄存器、端口、中断向量和片内外设。

当前职责：

- `platform_config.h` 集中 MSP430F5529 Pocket Kit 的时钟、引脚、片内 Flash 地址和外设参数。
- `board.c/.h` 负责板级启动、时钟、10ms tick、蜂鸣器和基础延时。
- `input_msp430.c` 负责 S1-S4 GPIO 中断、Pad1/Pad2 电容触摸扫描和语义输入事件映射。
- `captouch.c/.h` 负责 Comparator B 电容触摸测量。
- `epd_panel_msp430.c/.h` 负责墨水屏 GPIO、软件 SPI、BUSY 等待、控制器初始化和帧提交。
- `sensors.c/.h`、`uart.c/.h` 负责传感器和串口底层访问。

更换 STM32、ESP32 等平台时，优先替换这一层；应用层不应该跟着硬件引脚变化而修改。
