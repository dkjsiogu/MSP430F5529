# MSP430F5529 温度记录仪

这是一个基于 MSP430F5529 Pocket Kit 的温度采集、显示、记录和报警项目。系统采集片内温度、外接 NTC 热敏电阻温度和 TMP421 本地温度，并通过墨水屏显示当前数据和历史数据。项目支持定时采样、片内 Flash 存储、按键设置参数和蜂鸣器报警。

## 功能特性

- 温度采集：采集 MSP430 片内 DIE 温度、NTC 温度和 TMP421 本地温度。
- 墨水屏显示：显示当前温度、报警阈值、报警状态、设置界面和历史记录。
- 历史记录：温度样本保存到片内 Flash，可按设置限制存储条数。
- SD 卡资源：支持 32GB FAT32 SDHC 卡读写，图片、字库和阅读文本放在 `IMG/`、`TEXT/` 目录并由 `ASSET.IDX` 索引。
- 参数设置：通过 S1-S4 按键设置采样间隔、报警阈值、存储条数和报警时长。
- 蜂鸣器报警：温度超过阈值后，通过 P3.6 输出方波驱动无源蜂鸣器。
- 低功耗等待：主循环空闲时进入 LPM0，由采样定时器、按键中断或串口接收唤醒。

## 技术栈

- MCU：TI MSP430F5529
- 开发环境：Code Composer Studio
- 语言：C
- 显示：SSD1673 墨水屏驱动，软件 SPI
- 传感器：MSP430 片内温度传感器、NTC ADC、TMP421 I2C
- 存储：MSP430 片内 Flash、FatFs FAT32 SD 卡
- 交互：Pocket Kit S1-S4 按键、UART 接收命令
- 报警：P3.6 PWM/方波驱动无源蜂鸣器

## 代码结构

```text
.
├── main.c                 主循环，显式初始化资源层并调度采样、显示、按键、Flash 和低功耗
├── app_config.h           全局硬件引脚和应用参数配置
├── app_types.h            温度样本、历史记录等公共数据结构
├── app_state.c/.h         应用设置管理和 Info Flash 持久化
├── app_resources.c/.h     应用资源门面，向主循环和渲染层提供图片、字库和文本接口
├── board.c/.h             时钟、GPIO、采样定时器、蜂鸣器、心跳 LED
├── buttons.c/.h           S1-S4 按键采集、去抖和设置状态机
├── sensors.c/.h           DIE、NTC、TMP421 温度采集
├── epaper.c/.h            墨水屏驱动和页面渲染，只消费资源帧和阅读页结果
├── flash_log.c/.h         温度历史记录 Flash 存取
├── sd_assets.c/.h         FAT32 SD 卡索引、图片、字库和文本的底层加载
├── text_reader.c/.h       BOOK.TXT UTF-8 解码、分页和上一页偏移栈
├── fatfs/                 Pocket Kit SD SPI 适配后的 FatFs 文件系统
├── serial_control.c/.h    串口接收命令处理
├── uart.c/.h              UART1 接收中断封装
├── format.c/.h            小型字符串格式化工具
├── tools/                 图片/GIF 到墨水屏帧资源的转换脚本
├── sdcard/                可直接复制到 SD 卡根目录的示例资源
├── lnk_msp430f5529.cmd    MSP430F5529 链接脚本
└── targetConfigs/         CCS 目标配置
```

## 按键说明

主界面：

- S1：进入全屏 GIF 播放页面，S1/S2 在该页面切换上一个/下一个动图。
- S2：进入 SD 卡文本阅读页面，S1/S2 在该页面翻到上一页/下一页。
- S3：进入设置界面。
- S4：手动执行一次全屏刷新。

设置选择界面：

- S1/S2：上下选择参数。
- S4：进入当前参数编辑。
- S3：返回主界面。

参数编辑界面：

- S1：增加当前参数。
- S2：降低当前参数。
- S4：确认并回到参数选择界面。
- S3：返回主界面。

当前可设置参数：

- `SAMPLE`：定时采集间隔。
- `ALM TEMP`：报警温度阈值。
- `STORE`：历史数据存储条数。
- `ALM TIME`：报警鸣叫时长。
- `HOURGLASS`：沙漏动画周期和 TMP 平均温度统计窗口。

## 硬件连接

- S1：P1.2，内部上拉，按下为低电平。
- S2：P1.3，内部上拉，按下为低电平。
- S3：P2.3，内部上拉，按下为低电平。
- S4：P2.6，内部上拉，按下为低电平。
- 蜂鸣器：P3.6 输出方波。无源蜂鸣器需要按 Pocket Kit 文档连接跳线，将 3 和 5 相连、4 和 6 相连。
- 墨水屏：使用 P1.4、P2.2、P2.7、P3.2、P3.3、P3.4。
- SD 卡：使用 UCB1 SPI，P4.0 为 CS，P4.1 为 SIMO，P4.2 为 SOMI，P4.3 为 CLK。
- NTC：P6.5 / A5。
- TMP421：I2C 连接，程序会检测 TMP421 地址。

## SD 卡资源

工程已经接入 ChaN FatFs R0.08b，并把底层初始化改成 SDv2/SDHC 需要的 `CMD8 + ACMD41(HCS) + CMD58` 流程，因此 32GB FAT32 卡可以按块地址读写。

使用方式：

- 将 `sdcard` 目录下的 `ASSET.IDX`、`IMG`、`TEXT` 复制到 FAT32 SD 卡根目录。
- 插入 SD 卡后烧录程序，主界面沙漏、`郑` 字、S1 GIF 页面和 S2 阅读页面都会从 SD 卡读取资源。
- GIF 页面左上角显示 `SD` 表示当前帧来自 SD 卡；如果 SD 卡或文件不可用，会显示 `NO SD`。
- 图片和文字资源不再编译进片内 Flash，Flash 主要保存程序和应用参数。
- 串口发送 `w` 会在 SD 卡根目录创建或追加 `SDTEST.TXT`，用于验证 FAT32 写入路径。

SD 卡目录约定：

```text
ASSET.IDX              根资源索引
IMG/MASCOT.BIN         S1 全屏 GIF 帧资源
IMG/HOURGLAS.BIN       主界面沙漏帧资源，8.3 文件名
IMG/GIF2.BIN           可选扩展动图，存在时会被写入索引并可在 GIF 页面翻页
IMG/GIF3.BIN           可选扩展动图
IMG/GIF4.BIN           可选扩展动图
TEXT/FONT24.BIN        24x24 点阵字库，用于主界面“郑”
TEXT/FONT16.BIN        16x16 点阵字库，用于阅读页
TEXT/BOOK.TXT          UTF-8 小说文本，文件较大，不纳入 Git
```

重新生成 SD 图片资源示例：

```powershell
python tools\image_to_frames.py --prepare-sd-layout sdcard --only-binary
python tools\image_to_frames.py C:\Users\Administrator\Downloads\500000002.gif --binary-output sdcard\IMG\MASCOT.BIN --only-binary --width 150 --height 122 --max-frames 6 --mono-mode subject
python tools\image_to_frames.py --demo-hourglass --binary-output sdcard\IMG\HOURGLAS.BIN --only-binary --width 48 --height 66 --x 198 --y 12 --max-frames 12
python tools\image_to_frames.py --font-output sdcard\TEXT\FONT24.BIN --font-chars 郑 --only-binary
python tools\image_to_frames.py --text-input sdcard\TEXT\BOOK_SOURCE.TXT --text-output sdcard\TEXT\BOOK.TXT --only-binary
python tools\image_to_frames.py --font-output sdcard\TEXT\FONT16.BIN --font-chars-file sdcard\TEXT\BOOK.TXT --font-width 16 --font-height 16 --font-size 17 --font-threshold 150 --font-direct-index --only-binary
```

## 构建方式

在 CCS 中导入工程后直接构建，或在命令行使用当前 Debug 目录下的 makefile：

```powershell
cd E:\code\ccs\GPIO\LED\Debug
& 'D:\ccs\ccs\utils\bin\gmake.exe' all
```

构建产物：

```text
Debug\1. LED.out
```

## 低功耗说明

主循环在没有待处理墨水屏刷新任务时进入 `LPM0`。`Timer0_A0`、`PORT1/PORT2` 按键中断和 `USCI_A1` 串口接收中断会在退出中断时清除 `LPM0_bits`，唤醒主循环继续处理任务。由于当前墨水屏动画追求刷新流畅度，实际省电效果会受到持续刷新影响。

## 注意事项

- `.out`、`.obj`、`.map`、`*_linkInfo.xml` 等构建产物已通过 `.gitignore` 忽略。
- `.launches/` 中通常含有本机路径，不纳入版本管理。
- `Debug/` 下保留 CCS 自动生成的 makefile 规则，便于命令行复现构建。
