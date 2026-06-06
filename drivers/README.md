# 底层驱动层

这个目录作为底层 C 驱动的迁移边界。第一阶段先保留现有 C 驱动文件位置，避免一次性改动 FatFs、墨水屏和中断向量相关路径。

后续迁移建议：

- GPIO、时钟、蜂鸣器、定时器归入 `drivers/board/`
- SD SPI 和 FatFs 适配归入 `drivers/storage/`
- 传感器 ADC/I2C 归入 `drivers/sensors/`
- 墨水屏硬件时序归入 `drivers/display/`

底层驱动保持 C ABI；上层通过 `middleware/` 的 C++ 适配层调用。
