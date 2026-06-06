# 中间连接层

`middleware/` 负责把应用层需要的能力整理成稳定接口，让应用层看到“输入事件、显示服务、资源、记录存储”，而不是看到具体芯片寄存器或文件系统细节。

当前职责：

- `input.h` 定义可移植语义输入事件，具体输入来源由平台驱动映射。
- `app_types.h` 定义跨层数据契约，`system_config.h` 定义公共时基，`display_config.h` 定义显示几何和刷新时序。
- `epaper.c/.h` 提供显示服务和页面渲染入口，面板 GPIO/SPI/BUSY 时序下沉到 `drivers/`。
- `app_resources.c/.h`、`sd_assets.c/.h`、`text_reader.c/.h` 负责 SD 资源门面、资源索引和文本分页。
- `settings_store.c/.h`、`flash_log.c/.h` 负责设置存储和历史记录存储后端。
- `freertos/` 保存 FreeRTOS 内核、MSP430X 移植层和项目 RTOS hooks。

中间层可以依赖底层 C 驱动和 FatFs，但应用层应通过这里的接口访问能力。
