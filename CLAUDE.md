# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 仓库性质

嵌入式 Linux 课程资料库（正点原子 FS-MP1A / STM32MP157A 开发板，共 10 章）。不是代码项目，没有构建/测试流程。核心资产是 `PPT/*.md`（由课件 PPT 转换的图文完整版）及 `PPT/images/chap01~10/` 配图。

- `PPT/*.md` + `PPT/images/` 是**git 跟踪的唯一内容**，也是知识的权威载体（描述行中逐字转录了截图里的 menuconfig 路径、配置项、命令行等）。
- 原始课件 `.dps`/`.pptx` 保留在磁盘上但被 git 忽略。若 md 描述存疑，以原 PPT 截图为准。
- `01实验/` 存放实验产物与实验指导（工作流和环境见下方「实验部分」）；其中大体积安装包（`*.zip`/`*.tar.xz`）及解压出的 SDK 目录（`en.SDK-*/`）被忽略，实验代码可正常跟踪。

## md 文件结构与约定

- 每页幻灯片一个 section，以 `<!-- Slide number: N -->` 分隔，N 与原 PPT 页码对应，勿改动。
- 每个图片引用 `![chapNN/slideXXX_KK.ext](images/chapNN/slideXXX_KK.ext)` 的**紧下一行**必须是 `> 图：` 描述行——这是全仓库的固定格式，编辑时保持二者相邻。
- 图片按内容 md5 去重：文件名中 `slideXXX` 是该图**首次出现**的页码，`KK` 是全章出现顺序序号（不是页内序号）。同一文件可被多页引用。
- 编码 UTF-8（无 BOM）、LF 行尾。控制台里中文显示乱码通常是 GBK 显示问题，文件本身没问题——不要因此"修复"文件编码。

## PPT → Markdown 转换管线（新增章节时使用）

`.dps` 是 WPS 扩展名，但文件实际是改名的 MS PowerPoint 97-2003 (.ppt)（OLE2 复合文档，含 `PowerPoint Document` 流），PowerPoint 无法直接按 .dps 扩展名打开：

1. 复制为 `.ppt` 扩展名：`cp "${f%.dps}.ppt"`
2. PowerShell COM 批量转 `.pptx`（`pp.Presentations.Open($f, $true, $false, $false)` + `SaveAs($out, 24)`，目录路径经环境变量传入避免中文参数乱码）
3. markitdown 转 md，**必须加 UTF-8 环境变量**（否则 GBK 输出导致全文乱码）：
   ```bash
   PYTHONUTF8=1 PYTHONIOENCODING=utf-8 /c/Users/GY/AppData/Local/Programs/Python/Python314/python -m markitdown "文件.pptx" > "文件.md"
   ```
4. 用 python-pptx 按幻灯片顺序提取图片（group 递归、placeholder 含 `.image` 的也算图片，按 md5 去重），并核对每页 md 引用数与实际图片数——**markitdown 会随机丢弃部分图片**（本次 555 页丢了 30 张），核对后按页补插引用。
5. 逐张查看图片补写 `> 图：` 描述，截图中的菜单路径/配置项/命令需逐字转录。

## 校验

改动 md 后应确认：旧格式死链（`![图片 x](图片x.jpg)`）为 0、每条引用下一行是 `> 图：` 行、引用文件均存在、`<!-- Slide number:` 数量不变。

## 实验部分（01实验/）

FS-MP1A（STM32MP157A）实验课：在 VirtualBox 的 Ubuntu 20.04 虚拟机里做交叉编译，把 U-Boot/内核烧到开发板。当前实验对应课件《第3章 移植U-Boot》，按 Slide 范围拆成多个实验。

### 实验工作流

- 每个实验一份指导文件 `01实验/NN_实验N_内容.md`（博客命名规则见下），范围对应课件的具体 Slide；开头有"课件 ↔ 步骤"对应表，正文为：实验目的 → 实验环境 → 实验步骤 → 注意事项 → 完成标志 → 后续实验预告。
- 实验开始前：写指导（命令 + 预期输出 + 注意事项），各步骤留"**实际执行结果**：待补充"占位。
- 用户在虚拟机里执行，把终端输出贴回对话（原始记录由用户存到 `testN.md`），完成后把真实输出合并进指导文件：替换"待补充"、状态改 ✅、勾选完成标志——二者合二为一。
- 用户会自己往指导文件里插截图（`实验N ...assets/` 目录）并编辑文件——**每次编辑指导文件前必须重新 Read**，文件经常在对话间隙被用户更新。
- 用户偏好：指导文件只写正确流程，走弯路的尝试不写入（实验一的 SDK 误装目录已按要求删除）；但无害报错的解释（如 gc 冲突）可以保留。
- **博客文档图片规范（Typora 规则）**：博客发布的 md（01实验 各指导/导论 + 根目录专栏总导论）图片一律复制到 `./${md文件名}.assets/` 文件夹，引用写 `./${md文件名}.assets/xxx.png`——**禁止 `../PPT/images` 跨目录引用**（博客平台不解析）；PPT/*.md 仍用 `images/chapNN/` 共享图库不动。新写博客文档时先建 assets 文件夹再写引用。
- **博客文档命名规则（用户指定，2026-09-16）**：博客 md 及 assets 文件名**不含空格**，统一 `NN_短标题.md`——NN 为两位序号，按专栏全局阅读顺序递增（文件名排序即阅读顺序，方便索引）；下划线分词（避免与 U-Boot、F-1 内部连字符混淆）；assets 文件夹严格同名加 `.assets`。已定名：根目录 `00_专栏总导论_嵌入式Linux全景与学习路线.md`；01实验：`01_实验导论_U-Boot基础与移植全景.md`、`02_实验一_安装交叉编译工具链.md`、`03_实验二_获取U-Boot源码并打ST补丁.md`、`04_实验三_basic版U-Boot配置与首次编译.md`。后续（实验四、F 系列、trusted、第4~10章实验）从 05 续编。testN.md、PPT/*.md、CLAUDE.md 不适用此规则。

### 实验环境（截至 2026-09-15，有变化以用户终端为准）

- 虚拟机：VirtualBox + Ubuntu 20.04（focal），主机名 `cnu-virtual-machine`，用户 `cnu`，内存 4GB（编译用 `-j2` 上限）。
- 共享文件夹挂载点换过（实验一 `~/Desktop/share/Linux/Test1` → 现在 `~/Desktop/LINUX-gy/Test2`），新实验先看用户终端提示符确认路径。
- 工具链：`/opt/st/stm32mp1/3.1-openstlinux-5.4-dunfell-mp1-20-06-24`；激活：`source /stm32env`（符号链接可能未建）或直接 `. /opt/st/stm32mp1/3.1-openstlinux-5.4-dunfell-mp1-20-06-24/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi`；**每个新终端窗口都要重新激活**，`echo $CC` 验证。
- git 已配置（Gsheep0729 / 2697438381@qq.com）。
- U-Boot 源码（已打 6 个 ST 补丁，WORKING 分支 7 条提交）：`~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01`。
- basic 版编译：`make -j2 all DEVICE_TREE=stm32mp157a-fsmp1a` → 产物 `u-boot-spl.stm32`（FSBL）+ `u-boot.img`（SSBL）。
- 串口：VirtualBox 抓板载 ST-Link 虚拟串口，115200；当前板子跑出厂固件（trusted 模式，DK1 设备树）。

### 实验进度（2026-09-15）

- ✅ 实验一 Slide 28-31（安装 SDK 工具链 + 辅助工具）；✅ 实验二 Slide 32-34（解压源码 + git am 6 个 ST 补丁）；✅ 实验三 Slide 35-42（basic 版配置 + 设备树 + 首次编译）。
- ⬜ 下一步：Slide 43 起 SD 卡分区烧写 + 首次启动（拨码 101）；预期电源初始化失败不断复位（正常），随后进入 F-1~F-6 迭代修复（F-1 电源 → F-2 SD 卡 CD 引脚 → F-3 关 ADC → F-4 关 LTDC → F-5 国产网卡 → F-6 eMMC）；最后移植 trusted 版。
- 后续实验材料在 `D:\桌面文件\资料\嵌入式linux`：`tf-a-stm32mp157a-fsmp1a-trusted.stm32`（trusted 版 TF-A）、`u-boot-网卡-MAE0621A驱动.rar`（F-5 的 phy.c/maxio.c/dwc_eth_qos.c）、`u-boot电源配置-设备树.dts`（F-1 参考）、`官方系统内核和设备树.zip`、kernel 相关 zip、buildroot/busybox 等。

### 实验踩坑记录（避免重复踩）

- SDK 解压目录里**没有 `install.sh`**，安装脚本是长名字的 `st-image-weston-*.sh`；安装目标目录必须直接回车用默认 `/opt/st/...`（vboxsf 共享文件夹不支持符号链接，工具链不能装在里面）。
- `git init` 必须在解压出的 `u-boot-stm32mp-2020.01` 子目录里执行；在外层 `r0` 目录执行会把补丁文件本身当源码提交，且 `../*.patch` 找不到补丁（`rm -rf .git` 后重来）。
- `cc1: error: bad value ('generic-armv7-a') for '-mtune=' switch` = 当前终端没激活工具链，make 落到了 x86 宿主机 gcc（报错列出的 -mtune 参数全是 Intel/AMD CPU 即铁证）→ `source` 工具链后续编即可，无需 clean。
- `fatal: 已经有一个 gc 正运行`、ST 补丁的空白字符警告（`new blank line at EOF`）均无害，忽略。
- 编译在共享文件夹里能跑但慢，Ctrl+C 后 `make -j2` 可安全续编（进度保留）；若报符号链接/权限类错误，把源码移到虚拟机本地磁盘（如 `~/FS-MP1A/`）再编（挪动后需 `make distclean` + 重新 defconfig）。
