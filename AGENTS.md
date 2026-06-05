# 项目关键记忆

这个文件用于上下文丢失后的快速恢复。接手时先读 `AGENTS.md`、`README.md`，再执行 `git status --short`，不要回退用户未说明要回退的改动。

## 用户要求

- 交流、注释、提交信息优先使用中文。
- Git 提交人保持 `dkjsiogu <15715022545@163.com>`。
- 用户追求的是接近产品级的嵌入式工程：硬件可验证、显示效果可接受、响应速度直接、资源和代码结构清晰。
- 不接受把功能随手塞进最近的文件。新增功能前先判断模块边界是否健康。

## 项目目标

- 平台：MSP430F5529 Pocket Kit，CCS 工程路径通常是 `E:\code\ccs\GPIO\LED`。
- 功能：采集 DIE 片内温度、NTC 热敏温度、TMP421 本地温度；墨水屏显示；片内 Flash 历史记录；SD 卡图片/GIF/字库/文本资源；S1-S4 按键交互；P3.6 方波驱动无源蜂鸣器。
- 蜂鸣器硬件需要按 Pocket Kit 文档接跳线：3 和 5 相连，4 和 6 相连。

## 当前架构边界

- `main.c` 是显式调度入口，负责初始化和主循环，不隐藏关键子系统。
- `app_resources.c/.h` 是应用资源门面。上层只依赖它读取图片帧、字形和文本，不直接依赖 SD/FatFs。
- `sd_assets.c/.h` 是底层 FAT32 SD 资源索引和文件读取实现，直接处理 `ASSET.IDX`、`IMG/`、`TEXT/`。
- `text_reader.c/.h` 负责 `BOOK.TXT` 的 UTF-8 解码、分页、上一页偏移栈；`epaper.c` 不再解析文本。
- `epaper.c/.h` 负责墨水屏驱动和页面渲染，只消费资源帧、字形行和阅读页结果，不直接调用 `sd_assets`。
- `buttons.c/.h` 负责 S1-S4 去抖、事件和页面/设置状态机。
- `app_state.c/.h` 负责设置参数和 Info Flash 持久化。
- `flash_log.c/.h` 负责温度历史记录，不要把实时显示链路强行绕进 Flash。

## 显示和交互现状

- 墨水屏驱动以 SSD1673 路径为主，保留备用驱动。
- 用户非常在意刷新残影、白底/灰底和闪烁。优先保持局部刷新和统一渲染缓冲，避免无意义全刷。
- 主界面：S1 进入全屏 GIF 页面，S2 进入文本阅读页面，S3 进入设置页面，S4 手动触发类似上电后的全屏刷新。
- GIF 页面：S1/S2 切换上一个/下一个动图，S3 返回主界面。
- 文本阅读页：S1/S2 上一页/下一页，S3 返回主界面。
- 设置页：S1/S2 选择或调整，S4 进入/确认参数，S3 返回主界面。

## SD 卡资源约定

- 支持 32GB FAT32 SDHC。资源不再编译进 `.c/.h`，Flash 主要保存程序、配置和历史记录。
- SD 根目录约定：
  - `ASSET.IDX`
  - `IMG/MASCOT.BIN`
  - `IMG/HOURGLAS.BIN`
  - `IMG/GIF2.BIN`
  - `IMG/GIF3.BIN`
  - `IMG/GIF4.BIN`
  - `TEXT/FONT24.BIN`
  - `TEXT/FONT16.BIN`
  - `TEXT/BOOK.TXT`
- `TEXT/BOOK.TXT` 很大，通常不纳入 Git。
- 资源生成脚本主要是 `tools/image_to_frames.py`，支持 GIF、沙漏 demo、字库、SD layout 和 direct-index 字库格式。

## 构建和验证

- 命令行构建：

```powershell
cd E:\code\ccs\GPIO\LED\Debug
D:\ccs\ccs\utils\bin\gmake.exe all
```

- 最近一次架构整理后构建通过，只有 TI ULP 低功耗建议。
- 最近 map 里 RAM 用量约 `0x1758`，剩余约 `0x08a8`。继续加功能时要注意 RAM，不要缓存整帧大图或整本小说。

## Git 约定

- 分支：`main`。
- 远端：`origin https://github.com/dkjsiogu/MSP430F5529.git`。
- 提交信息用中文。
- 推送前先构建或至少说明未构建原因。
- CCS 的 `Debug/makefile`、`Debug/subdir_vars.mk` 可能被构建工具改出行尾空格或换行符噪声，提交前检查 diff，不要把无意义噪声混进功能提交。
