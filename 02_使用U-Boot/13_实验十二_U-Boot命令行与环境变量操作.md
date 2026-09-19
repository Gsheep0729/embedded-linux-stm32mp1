# 实验十二 U-Boot 命令行与环境变量——第 4 章《使用 U-Boot》开篇

> **对应课件**：《第4章 使用U-Boot》4.1~4.3 节，Slide 2-16
>
> **系列说明**：本系列基于华清远见 FS-MP1A（STM32MP157A）开发板，对应课件《第4章 使用U-Boot》。第 3 章《移植U-Boot》的十一个实验把启动链条第三环装到了板上——trusted 版 U-Boot 已能稳定停在 `STM32MP>` 命令行。但能停在命令行只是"装好了"，还谈不上"会用"。第 4 章就是使用篇：U-Boot 的命令行是后续内核移植、根文件系统实验的操作台——环境变量怎么设、文件怎么下载、内核怎么手动启动，全在这里练。本文覆盖 Slide 2-16：进入命令行、help 帮助系统、三条查询命令、环境变量的增删改与 saveenv 保存纪律。前置：第 3 章实验十一已完成（trusted 版收官，上电直达 `STM32MP>`）。

## 一、本篇的特殊之处：不碰源码

第 3 章的每个实验都从"激活工具链、进源码目录"开始；本篇全程**不用虚拟机、不编译、不烧写**——纯粹的板子侧操作，所有动作都发生在串口的 `STM32MP>` 提示符后面。这也是"使用"与"移植"的分野：移植时我们改的是 U-Boot 本身，使用时我们驱使的是已经装好的 U-Boot。

课件 Slide 2 的串口截图里，华清演示板跑的正是 trusted 版 U-Boot（横幅 `Board: stm32mp1 in trusted mode`）——与我们第 3 章收官后的板子状态完全同款，跟着课件走不会有"版本对不上"的困扰。

## 二、实验环境（实际）

| 项目 | 实际值 |
|---|---|
| 板子状态 | 第 3 章收官形态：SD 卡启动（拨码 `101`），trusted 版 U-Boot，`STM32MP>` 稳定可达 |
| 串口 | MobaXterm Serial 会话（COM11），115200（同第 3 章） |
| 环境变量基线 | 实验九设定并 saveenv：`ethaddr`、`ipaddr 192.168.0.8`、`netmask 255.255.255.0`，存于 SD 卡 |
| 本篇新增材料 | 无——不动源码、不编译、不烧写 |

## 三、课件 ↔ 步骤对应表

| 课件 Slide | 内容 | 对应步骤 |
|---|---|---|
| 2 | 上电拦停，进入 U-Boot 命令行 | 步骤 1 |
| 3~6 | help/？ 帮助系统、命令缩写、Tab 补全、十六进制参数 | 步骤 2 |
| 7~10 | 查询命令：bdinfo / printenv / version | 步骤 3 |
| 11~12 | 环境变量三剑客 printenv/setenv/saveenv；修改 bootdelay 实战 | 步骤 4 |
| 13 | 新增环境变量（author） | 步骤 5 |
| 14 | 删除环境变量；"改完必须 saveenv"纪律 | 步骤 6 |
| 15~16 | 重要的环境变量；网络四件套复核 | 步骤 7 |

## 四、实验步骤

### 步骤 1：上电拦停，进入命令行（Slide 2）

板子插好 SD 卡、拨码 `101`、上电（或按复位）。MobaXterm 串口里会依次看到：

1. **TF-A 引导日志**（`NOTICE:` / `INFO:` 开场，实验十一逐行解读过）；
2. **U-Boot 横幅**——`U-Boot 2020.01-stm32mp-r1-g88f08870-dirty (Sep 19 2026 - 14:51:04 +0800)` 一行起，到 `Net:   eth0: ethernet@5800a000` 为止；
3. **倒计时**：`Hit any key to stop autoboot:  0`——**看到这行立刻按 Enter**（实验九练过的动作：看准提示、倒计时内按键）；
4. 停在 `STM32MP>` 提示符。

![串口停在命令行](./13_实验十二_U-Boot命令行与环境变量操作.assets/01_串口停在命令行.png)
> 图：课件 Slide 2——trusted 版 U-Boot 启动串口打印：横幅自报 `in trusted mode`，`MMC: STM32 SD/MMC: 0, STM32 SD/MMC: 1` 双控制器，`Net: eth0`，`Hit any key to stop autoboot: 0` 处按任意键，停在 `STM32MP>`。（华清演示板截图，版本串与时间戳与我们必然不同，认提示符即可。）

错过倒计时也不要紧：trusted 版的 autoboot 只会去扫 mmc 设备（`Boot over mmc0!` → `** Unrecognized filesystem type **`——卡上还没有内核，这是后续章节的课题），不会像 basic 阶段那样挂在网卡上，两行报错后照样落回 `STM32MP>`。想从头看日志，敲 `reset`（或按板上复位键）即可重放一遍。

**实际执行结果**：待补充

### 步骤 2：help 帮助系统与命令行小抄（Slide 3~6）

U-Boot 命令行自带帮助系统，两条入口：

```
STM32MP> help
```

列出这个 U-Boot 支持的**全部**命令（一屏放不下，逐屏翻完）。`help` 与 `?` 等价。

![help命令列表](./13_实验十二_U-Boot命令行与环境变量操作.assets/02_help命令列表.png)
> 图：课件 Slide 3——`STM32MP> help` 输出（节选）：`? - alias for 'help'`、`bdinfo - print Board Info structure`、`bootm - boot application image from memory`、`mmc - MMC sub-system` 等，每行一条"命令名 - 一句话说明"。

查单条命令的详细用法（usage）：

```
STM32MP> ? printenv
```

![查printenv用法](./13_实验十二_U-Boot命令行与环境变量操作.assets/03_查printenv用法.png)
> 图：课件 Slide 4——`? printenv` 输出该命令的两行用法：`printenv [-a]` 打印全部环境变量、`printenv name ...` 打印指定名字的环境变量。

另外三条命令行小抄（课件 Slide 5~6，纯文字但天天要用）：

| 规则 | 说明 |
|---|---|
| **命令可缩写** | 前缀不歧义即可省略后缀：`print` 与 `printenv` 效果完全相同 |
| **Tab 补全** | 输入命令前几个字符按 Tab，自动补全；歧义时连按两次列出候选 |
| **回车重复** | 空命令行直接回车 = 重复执行上一条命令 |
| **数值一律十六进制** | 命令的数值型参数（地址、扇区号）都是十六进制，`0x` 前缀可写可不写——`tftp c2000000 uImage` 里的地址就没写 |

> **命令集差异提示**：第 3 章的 basic 版 U-Boot 裁剪过命令集（`printenv` 曾报 `Unknown command`，要用 `env print` 替代）。trusted 版配置的命令集完整得多，课件演示用的也是 trusted 版——`printenv`、`help` 都应直接可用。万一哪条命令报 `Unknown command`，先试 `env print`，再看 `help` 列表确认它到底在不在。

**实际执行结果**：待补充

### 步骤 3：三条查询命令（Slide 7~10）

4.2 节给出三条"只读"命令，逐条过：

**bdinfo** —— 开发板信息：

```
STM32MP> bdinfo
```

![bdinfo板信息](./13_实验十二_U-Boot命令行与环境变量操作.assets/04_bdinfo板信息.png)
> 图：课件 Slide 8——`bdinfo` 输出（节选）：`boot_params = 0xc0000100`（启动参数地址）、`DRAM bank = 0` 的 `-> start = 0xc0000000`（内存起始）与 `-> size`（内存大小）、`baudrate = 115200` 等。

重点关注 DRAM 两行：`start = 0xc0000000` 是 DDR 的物理起始地址——后面 tftp/ext4load 往内存下载文件，地址都从这一带起步；`size` 一行课件截图是 `0x40000000`（1GB 机型），**我们的 512MB 板应显示 `0x20000000`**——实验十一日志里 TF-A 报的 `Memory size = 0x20000000 (512 MB)` 正是同一件事，两个数字可以互证。

**printenv**（简写 `print`）—— 查看环境变量（4.3 节的主角，这里先见一面）：

```
STM32MP> print
```

![print查看环境变量](./13_实验十二_U-Boot命令行与环境变量操作.assets/05_print查看环境变量.png)
> 图：课件 Slide 9——`print` 输出环境变量（节选）：`arch=arm`、`baudrate=115200`、`board=stm32mp1` 等，一屏看不完。课件演示板的 `board_name=stm32mp157d-atk`，我们的板子由自编 U-Boot 报 DK1 设备树信息，变量值不同属正常。

**version** —— 版本信息：

```
STM32MP> version
```

![version版本信息](./13_实验十二_U-Boot命令行与环境变量操作.assets/06_version版本信息.png)
> 图：课件 Slide 10——`version` 输出三行：U-Boot 版本串、交叉编译器版本、链接器版本。

我们的版本串会带着自己的哈希与构建时间（第 3 章收官时卡上镜像为 `2020.01-stm32mp-r1-g88f08870-dirty (Sep 19 2026 - 14:51:04 +0800)`），与课件截图的 `2020.01-stm32mp-r1 (Nov 24 2020 ...)` 必然不同——看结构，不看数值。这条命令还能反查"卡上跑的到底是哪次构建"，第 3 章的版本串接力就是靠它（SPL/U-Boot 横幅）做的。

**实际执行结果**：待补充

### 步骤 4：setenv 修改 bootdelay + saveenv 保存（Slide 11~12）

4.3 节的三剑客分工：`printenv` 查、`setenv` 改、`saveenv` 存。先拿 `bootdelay`（autoboot 倒计时秒数）练手：

```
STM32MP> setenv bootdelay 5
STM32MP> saveenv
```

`setenv env_name env_value`——环境变量的值都是字符串；`bootdelay 5` 的 `5` 也是字符串 `"5"`，只是 U-Boot 用它时按数字解释。`saveenv` 把**整套**环境变量写回 flash（我们板上即 SD 卡的 ssbl 分区）：

![setenv改bootdelay](./13_实验十二_U-Boot命令行与环境变量操作.assets/07_setenv改bootdelay.png)
> 图：课件 Slide 12——`setenv bootdelay 5` 后执行 `saveenv`，输出 `Saving Environment to MMC... Writing to redundant MMC(1)... OK`。（华清演示板从 eMMC 启动所以写 MMC(1)；我们从 SD 卡启动，应见 `Writing to MMC(0)` 与 `Writing to redundant MMC(0)` 两条——主副本 + 冗余副本，实验九设网络变量时见过。）

敲 `reset` 复位，拦停——这回倒计时从容地数 `5、4、3、2、1、0`，五秒窗口，以后拦停再也不用手忙脚乱。

**实际执行结果**：待补充

### 步骤 5：新增环境变量（Slide 13）

`setenv` 同一条命令，换个用法就是"新增"——名字不存在就创建：

```
STM32MP> setenv author 'console=ttySTM0,115200 root=/dev/mmcblk2p2 rootwait rw'
STM32MP> saveenv
```

![新增author变量](./13_实验十二_U-Boot命令行与环境变量操作.assets/08_新增author变量.png)
> 图：课件 Slide 13——`setenv author '...'` 新建环境变量，值含空格必须用引号引起来，随后 saveenv 保存。

两个语法点：

- **值里有空格，必须整体加引号**（单引号双引号都行）——否则 U-Boot 会把第二段当成另一条命令的参数；
- `author` 这个值长得像内核启动参数（bootargs 风格），但它此刻**没有任何作用**——我们只是借它练语法。bootargs 什么时候真正上场，是第 5 章启动内核时的事。

用 `print author` 验证：

![author变量生效](./13_实验十二_U-Boot命令行与环境变量操作.assets/09_author变量生效.png)
> 图：课件 Slide 13——`print` 输出中红框标出新建的 `author=console=ttySTM0,11520 root=/dev/mmcblk2p2 rootwait rw`，与相邻变量并排出现。（课件截图里波特率少了一个 0，值本身无意义，不必纠结。）

**实际执行结果**：待补充

### 步骤 6：删除环境变量 + "重启即失"对照实验（Slide 14）

**删除**：给变量赋空值即删——`setenv` 后面只跟名字、不跟值：

```
STM32MP> setenv author
STM32MP> saveenv
```

![删除author变量](./13_实验十二_U-Boot命令行与环境变量操作.assets/10_删除author变量.png)
> 图：课件 Slide 14——`setenv author`（赋空值即删除）后 saveenv 保存。

接下来做一组**对照实验**，把 4.3 节最重要的一句纪律钉死——课件原话：*对环境变量进行修改后，要用 saveenv 命令把修改保存到 flash，否则修改只在内存中，重启会丢失*。空口无凭，亲测一次：

```
STM32MP> setenv testvar hello          # 只进内存，故意不 saveenv
STM32MP> print testvar                 # hello——内存里确实有
STM32MP> reset                         # 复位（或按板上复位键），倒计时内按 Enter 拦停
STM32MP> print testvar                 ## Error: "testvar" not defined——丢了
STM32MP> print author                  ## Error: "author" not defined——删除操作还在
STM32MP> print bootdelay               # 5——步骤 4 的修改也还在
```

三个结果对照着看：

| 变量 | 做过什么 | saveenv 过吗 | 重启后 |
|---|---|---|---|
| `testvar` | 新增 | 没有 | **消失** |
| `author` | 删除 | 有（步骤 6 开头那条） | 维持删除 |
| `bootdelay` | 改成 5 | 有（步骤 4） | 维持 5 |

一句话结论：**内存里的环境变量是"草稿"，saveenv 才是"存盘"，reset/断电就是"关机不保存"**。以后每次 setenv 完，先问自己一句"要重启后生效吗"——要，就 saveenv。

**实际执行结果**：待补充

### 步骤 7：重要的环境变量与网络四件套复核（Slide 15~16）

课件 4.3 节末尾点名了一批"重要的环境变量"，逐个认识（不必动手改）：

| 变量 | 作用 | 备注 |
|---|---|---|
| `bootcmd` | 启动命令——autoboot 倒计时归零后执行的就是它 | 第 3 章的 `Boot over mmc0!` 就来自它；4.7 节（下一篇）重点 |
| `bootargs` | 启动参数——传给 Linux 内核的命令行 | 第 5 章启动内核时才真正用到 |
| `ipaddr` | 开发板 IP | 实验九已设 |
| `serverip` | 电脑（服务器）IP | tftp 下载时的对端 |
| `netmask` | 子网掩码——须使 ipaddr 与 serverip 同网段 | 实验九已设 |
| `gatewayip` | 网关 IP | 直连 PC 的拓扑用不上 |
| `ethaddr` | 网卡 MAC 地址——不设网卡不工作 | 实验九设过；**write-once**，本篇练习别碰它 |

网络四件套在第 3 章实验九已经亲手设过并 ping 通了 PC——这里只需 `print` 复核它们还在：

```
STM32MP> print ipaddr netmask ethaddr
```

课件 Slide 16 的 setenv 例子（serverip/ipaddr/netmask 三连 + saveenv）与实验九的操作完全同款，本文不再重复操作；等下一篇搭好 TFTP 服务器、真要下载文件时，再按需更新 `serverip`。

**实际执行结果**：待补充

## 五、注意事项

1. **拦停姿势**：看准 `Hit any key to stop autoboot` 按 Enter（任意键皆可）；本篇把 bootdelay 调到 5 之后，窗口宽裕得多。错过也只是多走一段 autoboot 报错，`reset` 重来即可。
2. **saveenv 纪律**：改完要重启生效的，必须 saveenv；`saveenv` 落盘的是**整套**环境（主 + 冗余两份副本），不是只存最后一条。
3. **`ethaddr` 不当练习品**：它有 write-once 属性（`mo` 双标志），设过一次后直接 `setenv` 会被拒（`Can't overwrite`）；真要改得 `env default -a` 清整套环境重来（实验九的解法）。练习增删改用 `author`、`testvar` 这类自造变量最安全。
4. **数值参数一律十六进制**：从下一篇的 `tftp c2000000 uImage` 开始满屏都是地址，记住 `0x` 可省、字母不分大小写。
5. **终端粘贴用右键**：MobaXterm 里 Ctrl+C 是中断、Ctrl+V 不粘贴（第 3 章实验九的教训）。
6. **课件截图的数值别硬对**：演示板是 1GB eMMC 机型（DRAM size `0x40000000`、saveenv 写 MMC(1)、`board_name=...-atk`），我们是 512MB SD 卡启动（`0x20000000`、MMC(0)、DK1 设备树信息）——对结构不对数值，对不上就是正常。

## 六、怎么验证

三条判据同时成立：

1. `help` 能列出命令全集、`？ printenv` 能给出用法——帮助系统在；
2. `bdinfo` 的 DRAM `size = 0x20000000`、`version` 报出我们的构建版本串——查询命令在；
3. 对照实验完整：`author` 增了能 print 到、删了报 not defined（saveenv 过，重启仍在）；`testvar` 没存盘，重启即失——**saveenv 纪律不是背出来的，是测出来的**。

不达标时的排查：

| 现象 | 先查什么 |
|---|---|
| `printenv` 报 `Unknown command` | 确认板子跑的是 trusted 版（横幅 `in trusted mode`）；basic 版命令集裁剪过，用 `env print` 替代 |
| `saveenv` 报错或写不进 | 环境存于 SD 卡 ssbl 分区——卡插好了吗？`mmc dev` 看当前设备 |
| `setenv` 设 `ethaddr` 被拒 | write-once 正常行为，不是故障；要改见注意事项 3 |
| 倒计时只有 1 秒来不及拦 | `bootdelay` 还是原值——步骤 4 做完了吗（做了要 saveenv 才落盘） |

## 七、实验完成标志

- [ ] 上电能拦停进 `STM32MP>`（步骤 1）
- [ ] `help` / `？ printenv` 实测通过（步骤 2）
- [ ] `bdinfo` / `printenv` / `version` 三条查询实测（步骤 3）
- [ ] `bootdelay` 改 5 + saveenv + 复位复验倒计时（步骤 4）
- [ ] `author` 新增 → print 验证 → 删除 → print 报 not defined（步骤 5~6）
- [ ] `testvar` 未保存重启即失——对照实验亲测（步骤 6）
- [ ] 网络四件套 `print` 复核仍在（步骤 7）

## 八、下一步：网络操作命令

下一篇覆盖 4.4 节（Slide 17-22）：`ping` 回顾、`dhcp` 自动取 IP、`tftp` 把 PC 上的文件下载到板子内存、`nfs` 网络文件系统下载。tftp 是重头——它需要先在 PC 侧搭一台 TFTP 服务器；这条路打通之后，第 5 章的"网络加载自己编的 Linux 内核"就是水到渠成的事。
