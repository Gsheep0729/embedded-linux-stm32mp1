# 实验二 获取 U-Boot 源码并打 ST 补丁——把"别人的代码"理清楚

> **系列说明**：本系列基于正点原子 FS-MP1A（STM32MP157A）开发板，对应课件《第3章 移植U-Boot》。本文覆盖 Slide 32-34。前置：实验一的工具链已装好（`/opt/st/stm32mp1/...`），git 已配置。

## 一、为什么不直接下载个 U-Boot 就开改？

先讲清楚这个实验的逻辑。我们最终要的 U-Boot 源码，其实是**三层叠加**的结果：

1. **DENX 社区的官方主线源码**（`u-boot-stm32mp-2020.01.tar.gz`）——通用的、支持几百块板子的 U-Boot，但它不认识 STM32MP1 这颗芯片；
2. **ST 的厂商适配补丁**（6 个 `.patch` 文件）——ST 工程师为 STM32MP1 系列写的驱动、设备树、配置，以补丁形式发布；
3. **我们自己的移植修改**（后续实验）——针对 FS-MP1A 这块具体板子的调整。

为什么不把 2、3 直接揉进源码里发一个"整合包"？因为分层有分层的规矩：

- **责任清晰**：出了问题，`git log` 一看就知道每行代码是谁写的——是 DENX 的、ST 的、还是我自己的；
- **可以回退**：自己改坏了，`git reset` 一下就回到 ST 补丁打完的干净状态；
- **可升级**：ST 明年发布 r2 版本补丁，我们能清楚地知道"新补丁"和"我的修改"之间的边界。

而这一切的前提是：**用 git 管理这个源码树**。这就是本实验的真正主题——不只是"打补丁"，而是把源码变成一个有历史、可追溯的 git 仓库。

## 二、补丁文件名里藏着的"说明书"

拿到 r0 目录，先别急着执行命令，`ls` 看一眼——光看 6 个补丁的文件名，就能知道 ST 改了哪些层面：

```
0001-ARM-v2020.01-stm32mp-r1-MACHINE.patch        ← 平台级：SoC 本身的支持
0002-ARM-v2020.01-stm32mp-r1-BOARD.patch          ← 板级：ST 自家开发板的适配
0003-ARM-v2020.01-stm32mp-r1-MISC-DRIVERS.patch   ← 各种外设驱动
0004-ARM-v2020.01-stm32mp-r1-DEVICETREE.patch     ← 设备树
0005-ARM-v2020.01-stm32mp-r1-CONFIG.patch         ← 默认配置
0099-Add-external-var-to-allow-build-of-new-devicetree-fi.patch   ← 允许编译"清单外"的新设备树
```

前 5 个补丁的编号 0001~0005 是有顺序的，必须按序打；编号特意跳到 **0099** 的那个值得单独说——它的作用是让 U-Boot 支持在 `make` 命令行用 `DEVICE_TREE=xxx` 指定一个"官方设备树清单里没有"的新设备树。**这正是实验三要用的**：我们要编译 `stm32mp157a-fsmp1a` 这样一个 ST 清单里不存在的设备树，全靠这个补丁开了口子。看不懂某步没关系，但打过的每个补丁，后面都会兑现它的价值。

另外两个文件也别忽略：

| 文件 | 说明 |
|---|---|
| `u-boot-stm32mp-2020.01-r0.tar.gz` | DENX 官方 U-Boot 源码压缩包（第 1 层） |
| `series` | 补丁顺序清单——上面说的"必须按序打"，顺序就记在这里 |
| `README.HOW_TO.txt` | ST 自己写的操作手册！本实验步骤 3 的命令（4.2 节）就来自它 |
| `Makefile.sdk` | ST 提供的一键编译 Makefile（本系列不直接用它，知道即可） |

顺便建立一个重要观念：**`README.HOW_TO.txt` 是厂商亲笔写的正确姿势**。很多人拿到源码包直接跳过英文说明去抄博客，其实第一步应该是读它——嵌入式开发里，读英文文档不是加分项，是基本功。

## 三、实验环境（实际）

| 项目 | 实际值 |
|---|---|
| 虚拟机 | 同实验一（`cnu-virtual-machine`，Ubuntu 20.04） |
| 工作目录 | `~/Desktop/LINUX-gy/Test2`（本实验中途更换过共享文件夹挂载，原为 `~/Desktop/share/Linux/Test2`，不影响命令本身） |
| 工具链 | `/opt/st/stm32mp1/3.1-openstlinux-5.4-dunfell-mp1-20-06-24`（实验一安装） |
| git | 已安装并配置（实验一步骤 2） |

> **为什么工作目录不能带中文和空格？** U-Boot 的构建系统（Make/Kconfig）和各种脚本对空格、非 ASCII 路径的支持参差不齐，中文路径随时可能让编译莫名其妙地失败。养成习惯：**所有源码、工作目录只用英文、不用空格**。

---

## 四、实验步骤（真实记录）

### 步骤 1：解压 en.SOURCES 源码包

把 `en.SOURCES-stm32mp1-openstlinux-5-4-dunfell-mp1-20-06-24.tar.xz` 复制到共享文件夹后，在虚拟机中解压。`tar -xf` 会自动识别 xz 压缩格式：

```bash
cd ~/Desktop/LINUX-gy/Test2
tar -xf en.SOURCES-stm32mp1-openstlinux-5-4-dunfell-mp1-20-06-24.tar.xz
cd stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0
```

解压出的路径很有 Yocto 特色：`sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0`——ST 是按"目标架构/包名-r版本号"组织源码的，后面的 `r0` 目录就是本实验的工作目录。

`ls` 实际输出（与课件截图一致）：

```
0001-ARM-v2020.01-stm32mp-r1-MACHINE.patch
0002-ARM-v2020.01-stm32mp-r1-BOARD.patch
0003-ARM-v2020.01-stm32mp-r1-MISC-DRIVERS.patch
0004-ARM-v2020.01-stm32mp-r1-DEVICETREE.patch
0005-ARM-v2020.01-stm32mp-r1-CONFIG.patch
0099-Add-external-var-to-allow-build-of-new-devicetree-fi.patch
Makefile.sdk
README.HOW_TO.txt
series
u-boot-stm32mp-2020.01-r0.tar.gz
```

### 步骤 2：解压官方源码并进入源码目录

**做什么**：把第 1 层（DENX 官方源码）解开。

```bash
tar xfz u-boot-stm32mp-2020.01-r0.tar.gz
cd u-boot-stm32mp-2020.01/
```

> 注意：`-z` 显式声明 gzip 解压；解压出的目录是 `u-boot-stm32mp-2020.01`（**没有 r0 后缀**），别在 r0 目录里直接操作——补丁必须打在源码目录里，而不是装着补丁的 r0 目录里。

实际执行记录（进入源码目录后 `ls` 可见完整的 U-Boot 源码树）：

```
$ tar xfz u-boot-stm32mp-2020.01-r0.tar.gz
$ ls
0001-ARM-v2020.01-stm32mp-r1-MACHINE.patch
...
u-boot-stm32mp-2020.01
u-boot-stm32mp-2020.01-r0.tar.gz
$ cd u-boot-stm32mp-2020.01/
$ ls
api    common     doc      examples  Kconfig      Makefile  scripts
arch   config.mk  drivers  fs        lib          net       test
board  configs    dts      include   Licenses     post      tools
cmd    disk       env      Kbuild    MAINTAINERS  README
```

源码树结构先眼熟几个目录，实验三马上会用到：`configs/`（各板子的默认配置）、`arch/arm/dts/`（ARM 设备树）、`arch/arm/mach-stm32mp/`（STM32MP1 平台代码）。

### 步骤 3：把源码变成 git 仓库——记下"最初的模样"

**为什么要 git init？** 马上要打的 6 个补丁会改动几千个文件。如果不先用 git 给纯净源码拍一张"快照"，之后我们既说不清"哪些改动是 ST 的"，也没法在改坏时一键回退。所以：**先把纯净源码提交成基线，再打补丁**——每一层变化都是一条提交，历史一目了然。

```bash
test -d .git || git init . && git add . && git commit -m "U-Boot source code" && git gc
```

这条长命令逐段拆开看：

| 片段 | 作用 |
|---|---|
| `test -d .git \|\|` | 目录里已经有 `.git` 就跳过初始化——**幂等**设计，命令重复执行也不会出错 |
| `git init .` | 把当前目录初始化为 git 仓库 |
| `git add .` | 把全部源码加入暂存区（上万个文件，稍等片刻） |
| `git commit -m "U-Boot source code"` | 提交为基线 |
| `git gc` | 压缩整理仓库对象（上万个文件第一次提交后值得做） |

实际执行记录（节选——`git add .` 会输出几千行 `create mode ...`，此处只留尾部）：

```
[master （根提交） 52ec4091] U-Boot source code
 ...
 create mode 100755 tools/zynqmp_psu_init_minimize.sh
 create mode 100644 tools/zynqmpbif.c
 create mode 100644 tools/zynqmpimage.c
 create mode 100644 tools/zynqmpimage.h
fatal: 已经有一个 gc 正运行在机器 'cnu-virtual-machine' pid 7260（如果不是，使用 --force）
```

**末尾那行 fatal 要不要慌？** 不用。提交上万个文件时，git 检测到仓库对象很多，**自动触发了后台 gc**；命令链末尾我们手动执行的 `git gc` 属于重复劳动，被 git 拒绝了。提交本身早已成功（`[master （根提交） 52ec4091]`）。

这里教一个通用判断方法：**报错不一定意味着失败，先看关键动作（commit）有没有完成**。`fatal:` 开头的红色文字很吓人，但读一遍内容——"已经有一个 gc 正在运行"——就知道它只是"这活儿有人干了，不用你干"。

### 步骤 4：切出 WORKING 工作分支

```bash
git checkout -b WORKING
```

实际输出：

```
切换到一个新分支 'WORKING'
```

**为什么要专门切一个分支？** 分工约定：

- `master` 分支永远保持"官方源码 + ST 补丁"的**原始状态**，是参考基准；
- 后续我们所有的移植修改都在 `WORKING` 分支上进行，随便折腾。

想看"我到底改了什么"？一条 `git diff master` 全部列出。这就是前面说的"分层管理"落到实处的样子。

### 步骤 5：按顺序打入全部补丁

```bash
for p in `ls -1 ../*.patch`; do git am $p; done
```

这条命令的几个细节：

- `../*.patch`——补丁在**上一级目录**（r0 目录）里，所以要在源码目录里引用 `..`；
- `ls -1` 每行一个文件名，天然按 0001→0005→0099 排序，正好满足"按序打补丁"的要求；
- 用的是 **`git am`** 而不是 `patch` 或 `git apply`——`git am` 读取的是 `git format-patch` 格式的补丁，里面带着原作者、提交说明，打完补丁等于把 ST 工程师的提交**原样收编进我们的仓库**。这就是实验一提前配置 `user.name`/`user.email` 的原因：补丁的作者是 ST，但"把补丁提交进仓库"这个动作的 committer 是我们。

实际输出（6 个补丁全部应用成功，无 conflict）：

```
应用：ARM v2020.01-stm32mp-r1 MACHINE
应用：ARM v2020.01-stm32mp-r1 BOARD
应用：ARM v2020.01-stm32mp-r1 MISC-DRIVERS
应用：ARM v2020.01-stm32mp-r1 DEVICETREE
.git/rebase-apply/patch:5202: new blank line at EOF.
+
.git/rebase-apply/patch:7606: new blank line at EOF.
+
warning: 2 行新增了空白字符误用。
应用：ARM v2020.01-stm32mp-r1 CONFIG
应用：Add external var to allow build of new devicetree file
```

中间那几行 `new blank line at EOF` / `空白字符误用` **是警告不是错误**——只是说补丁文件末尾多了个空行，属于 ST 补丁自带的格式瑕疵，不影响内容，忽略即可。**`应用：` 六行才是关键**，六行齐全 = 六个补丁全部干净落地。

### 步骤 6：验证补丁结果

**信任但要验证**——补丁打完必须自己确认三件事：

```bash
git log --oneline | head -10
git branch
git status
```

实际输出（✅ 全部符合预期）：

```
$ git log --oneline | head -10
3f0216e7 Add external var to allow build of new devicetree file
e78f0877 ARM v2020.01-stm32mp-r1 CONFIG
70c67989 ARM v2020.01-stm32mp-r1 DEVICETREE
44fb1e3f ARM v2020.01-stm32mp-r1 MISC-DRIVERS
536afa06 ARM v2020.01-stm32mp-r1 BOARD
0ae7f7a9 ARM v2020.01-stm32mp-r1 MACHINE
52ec4091 U-Boot source code
$ git branch
* WORKING
  master
$ git status
位于分支 WORKING
无文件要提交，干净的工作区
```

逐项核对：

1. `git log`：**7 条提交**——最底下是源码基线（52ec4091），往上是 6 个补丁按序叠放，层次和我们计划的完全一致；
2. `git branch`：当前在 `* WORKING`，master 保持原始状态；
3. `git status`：工作区干净，没有半途而废的残留。

---

## 五、实验结果

| 检查项 | 结果 |
|---|---|
| en.SOURCES 解压，r0 目录内容与课件一致（10 个文件） | ✅ |
| 官方源码解压出 `u-boot-stm32mp-2020.01`，源码树完整 | ✅ |
| git init + 源码基线提交（52ec4091） | ✅（末尾 gc fatal 无害） |
| WORKING 分支创建，master 保持原始状态 | ✅ |
| 6 个 ST 补丁全部 `git am` 成功，无 conflict | ✅（空白警告无害） |
| `git log` 共 7 条提交，工作区干净 | ✅ |

## 六、实验完成标志

- [x] 解压出 `u-boot-stm32mp-2020.01-r0`，目录内容与课件清单一致
- [x] `git init` + 源码基线 commit 成功
- [x] WORKING 分支上 6 个补丁全部 `git am` 成功，无 conflict
- [x] `git log --oneline` 可见 1 条源码提交 + 6 条补丁提交

## 七、踩坑与经验

1. **要在源码目录（`u-boot-stm32mp-2020.01`）里操作，不是 r0 目录**：git 仓库建在源码目录里，补丁才打得进去；在 r0 目录里 `git init` 再 `git am ../xxx.patch` 是打不到源码上的（首次执行最容易犯）。
2. **补丁必须按 0001→0099 的顺序打**：顺序记在 `series` 文件里，`for p in $(ls -1 ../*.patch)` 的排序天然正确。
3. **报错先读内容再慌**：`gc 正运行` 的 fatal、`new blank line at EOF` 的 warning 都是无害信息，看关键动作（commit、`应用：`）是否完成。

## 八、下一步

现在源码已是"官方底座 + ST 适配"的完整状态，但它认识的板子里**没有 FS-MP1A**。下一实验要做的，就是告诉 U-Boot："我这块板子长什么样"——复制 defconfig、复制设备树、注册进编译清单，然后完成首次编译。请看《实验三 basic 版 U-Boot 配置与首次编译》。
