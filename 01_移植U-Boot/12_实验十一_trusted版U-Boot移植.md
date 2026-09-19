# 实验十一 trusted 版 U-Boot 移植——收官：FSBL 从 SPL 换成 TF-A

> **对应课件**：《第3章 移植U-Boot》3.6 节，Slide 82-87
>
> **系列说明**：本系列基于华清远见 FS-MP1A（STM32MP157A）开发板，对应课件《第3章 移植U-Boot》。本文覆盖 Slide 82-87，是 U-Boot 移植系列的**最后一站**。前置：实验十（F-6）已完成——basic 版适配收官，启动日志干净、停在 `STM32MP>`。

## 一、为什么还有个 trusted 版

专栏导论画过 STM32MP1 的两条启动链：

- **basic 链**（实验三至今在用）：`SPL（FSBL）→ U-Boot（SSBL）`——SPL 负责最底层的 DDR、时钟初始化，然后拉起 U-Boot；
- **trusted 链**（本站目标）：`TF-A（FSBL）→ U-Boot（SSBL）`——FSBL 换成 **TF-A**（ARM Trusted Firmware，芯片安全固件），由它完成底层初始化并代管电源、时钟，再移交 U-Boot。产品与安全场景用这条链，这也是出厂系统实际走的路。

U-Boot 本体则分 basic / trusted 两种工作模式（串口横幅里的 `Board: stm32mp1 in basic mode` / `in trusted mode`），对应两套默认配置。trusted 版 U-Boot 不再亲自操作 PMIC、时钟树——这些活 TF-A 干了，U-Boot 经安全通道向它"要"。

好消息是，前面十个实验的成果几乎全部直接继承。课件 Slide 82：

> basic版的移植工作主要是驱动的适配
>
> trusted版直接使用basic版中已适配好的驱动即可
>
> trusted版的移植，只需在图形化配置界面中简单修改配置即可

- **驱动文件、设备树**：都在源码里（F-2/F-4/F-6 的设备树改动、F-5 的驱动文件），任何配置编译时用的是同一份，天然继承；
- **menuconfig 配置**：`.config` 是跟着配置版本走的——trusted 版要从 ST 的 trusted 默认配置重新出发，F-1/F-3/F-5 那三处"只记录在 `.config` 里"的改动要**原样重做一遍**。

这也是当初"先 basic 后 trusted"次序的意义：驱动的难点全部在 basic 阶段解决完，到 trusted 这站只剩配置活。

## 二、实验环境（实际）

| 项目 | 实际值 |
|---|---|
| 虚拟机 | 同实验一（VMware + Ubuntu 20.04，4GB 内存） |
| 源码目录 | `~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01` |
| 分支 | WORKING |
| 工具链 | `/opt/st/stm32mp1/3.1-openstlinux-5.4-dunfell-mp1-20-06-24`（**每个新终端都要重新激活**，`echo $CC` 确认） |
| 新增材料 | 出厂 TF-A：`tf-a-stm32mp157a-fsmp1a-trusted.stm32`（Windows 资料包里有，烧写前经共享文件夹拷进虚拟机） |
| 要新增的文件 | `configs/stm32mp15_fsmp1a_trusted_defconfig`（复制自 ST 原厂 trusted 配置） |
| 串口 | MobaXterm Serial 会话（COM11），115200（同实验一） |
| SD 卡 | `/dev/sdb`（实验四已分好区；本站烧写内容更换，见步骤 6） |

## 三、课件 ↔ 步骤对应表

| 课件 Slide | 内容 | 对应步骤 |
|---|---|---|
| 82 | trusted 思路：驱动沿用，只改配置 | 本文第一节 |
| 83 | 保存 basic 配置 → distclean → 复制并加载 trusted 配置 | 步骤 2 |
| 84 | menuconfig 重做 basic 版改动；编译生成 `u-boot.stm32` | 步骤 3~4 |
| 85 | 烧写：TF-A → sdb1/sdb2，`u-boot.stm32` → sdb3 | 步骤 5~6 |
| 86 | 运行：trusted mode | 步骤 7 |
| 87 | 移植完成与收尾说明 | 本文第八节 |

## 四、实验步骤

### 步骤 1：激活工具链，进入源码目录

```bash
cd ~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01

. /opt/st/stm32mp1/3.1-openstlinux-5.4-dunfell-mp1-20-06-24/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi

echo $CC    # 输出 arm-ostl-linux-gnueabi-gcc ... 才算激活成功
```

### 步骤 2：配置切换——basic 存档、清场、trusted 上场（Slide 83）

依次执行四条命令：

```bash
cp .config configs/stm32mp15_fsmp1a_basic_defconfig
```

把当前 basic 配置存回 defconfig（实验九收尾做过的话，这条是幂等的，再跑无害）。

```bash
make distclean
```

清掉 `.config` 和全部编译产物。**别慌**：源码改动在 git 里、basic 配置在 `configs/stm32mp15_fsmp1a_basic_defconfig` 里，一个字节都丢不了——想回 basic，`make stm32mp15_fsmp1a_basic_defconfig` 再 `make` 即可。这就是 F-1 立下"改过 menuconfig 必须 `cp .config configs/...`"规矩的回报时刻。

```bash
cp configs/stm32mp15_trusted_defconfig configs/stm32mp15_fsmp1a_trusted_defconfig
```

复制一份 ST 原厂板的 trusted 配置作为 FS-MP1A 的（**这是一条命令**，课件特意注明"两行是一条命令"，终端里写不下才折行）。和实验三复制 basic 配置的操作完全同构。

```bash
make stm32mp15_fsmp1a_trusted_defconfig
```

加载 trusted 配置。自检：

```bash
ls configs/stm32mp15_fsmp1a_*    # 应有两个：basic 与 trusted
```

**实际执行结果**：四条命令依次执行完成（本站未单独截图）。旁证有二：`ls configs/stm32mp15_fsmp1a_*` 应见 basic 与 trusted 两个文件；步骤 3 的三处 `.config` 自检全部命中——说明加载的已是 trusted 配置基底。

### 步骤 3：menuconfig 把 basic 版的配置改动原样重做（Slide 84）

```bash
make menuconfig
```

按 basic 版改过的地方照改一遍，一共三站四处：

| 改动 | menuconfig 路径 | 来自 |
|---|---|---|
| 关掉 `Enable support for STMicroelectronics STPMIC1 PMIC` | `Device Drivers --->` → `Power --->` | F-1（实验五） |
| 关掉 `adc - Access Analog to Digital Converters info and data` | `Command line interface --->` → `Device access commands --->` | F-3（实验七） |
| 关掉 `Enable ADC drivers using Driver Model` | `Device Drivers --->` | F-3（实验七） |
| 勾选 `supports the Maxio MAEXXXX PHY` | `Device Drivers --->` → `Ethernet PHY (physical media interface) support --->` | F-5（实验九） |

![menuconfig 找到 STPMIC1 实测](./12_实验十一_trusted版U-Boot移植.assets/01_menuconfig找到STPMIC1实测.png)
> 图：menuconfig 实测——`Device Drivers → Power` 子菜单里的 `[*] Enable support for STMicroelectronics STPMIC1 PMIC`（高亮行，basic 阶段 F-1 关掉的就是它）；留意紧邻其下还有一条同以 PALMAS 结尾的 TI 电源芯片项，别按错行。

按 N 取消勾选、空格是勾选，退出时保存确认框选 `<Yes>`。自检（三条都要过）：

```bash
grep "STPMIC1" .config                                 # 期望：# CONFIG_PMIC_STPMIC1 is not set
grep -E "^# CONFIG_(CMD_)?ADC is not set" .config      # 期望：恰好两行
grep "MAXIO" .config                                   # 期望：CONFIG_PHY_MAXIO=y
```

再把 trusted 配置固化（新规矩的第一次执行，今后改 menuconfig 都照此办理）：

```bash
cp .config configs/stm32mp15_fsmp1a_trusted_defconfig
```

> **F-2/F-4/F-6 为什么不用重做？** 那三站改的是设备树文件——文件躺在源码里，basic、trusted 两种配置编译时读的是同一份，天然继承。要重做的只有"只记录在 `.config` 里"的配置改动，即 F-1、F-3、F-5。

**实际执行结果**：三处自检全部命中：

![三站四处自检实测](./12_实验十一_trusted版U-Boot移植.assets/02_menuconfig自检实测.png)
> 图：自检实测——`# CONFIG_PMIC_STPMIC1 is not set`（F-1）、`# CONFIG_CMD_ADC is not set` 与 `# CONFIG_ADC is not set` 恰好两行（F-3）、`CONFIG_PHY_MAXIO=y`（F-5）：trusted 配置的三站四处全部就位。

### 步骤 4：编译，产出 `u-boot.stm32`（Slide 84）

```bash
make -j2 all DEVICE_TREE=stm32mp157a-fsmp1a
```

编译完成，顶层目录下生成 **`u-boot.stm32`**——它就是 trusted 版 U-Boot 的可执行文件（SSBL）：

```bash
ls -l u-boot.stm32
```

![编译产物 u-boot.stm32](./12_实验十一_trusted版U-Boot移植.assets/03_编译产物u-boot.stm32.png)
> 图：课件 Slide 84——trusted 版编译成功后，u-boot 顶层目录执行 `ls` 的结果：`api`、`arch`、`board` 等目录与 `u-boot`、`u-boot.bin`、`u-boot.cfg`、`u-boot.dtb` 等文件并列，红框标出的 **`u-boot.stm32`** 即 trusted 版 U-Boot 的可执行文件。

留意两处与 basic 版编译的不同：

- **这次没有 `u-boot-spl.stm32`**：trusted 配置下 SPL 不参与构建——FSBL 的活整个交给 TF-A，U-Boot 直接以打包好的 `u-boot.stm32` 面世；
- 设备树里那大段时钟树、PMIC 标记（`&rcc` 的 `st,clksrc/st,clkdiv`、`&pmic` 的 `u-boot,dm-pre-reloc`、`&sdmmc1` 的 `u-boot,dm-spl` 等）在文件里都包在 `#ifndef CONFIG_STM32MP1_TRUSTED` 条件段里——trusted 编译时自动剔除。电源、时钟改由 TF-A 负责，U-Boot 不再亲自操心，这是两种模式在设备树层面的分水岭。

**实际执行结果**：

![编译收尾实测](./12_实验十一_trusted版U-Boot移植.assets/04_编译收尾实测.png)
> 图：编译收尾实测——`MKIMAGE u-boot.stm32`（绿框）是 trusted 版的最终打包步骤，收尾不见 SPL 的 `MKIMAGE spl/u-boot-spl.stm32`——SPL 确实缺席了这次构建；`ls -l u-boot.stm32` 报 855605 字节、14:51 生成。

### 步骤 5：把出厂 TF-A 拷进虚拟机（Slide 85）

`tf-a-stm32mp157a-fsmp1a-trusted.stm32` 在 Windows 资料包（`D:\桌面文件\资料\嵌入式linux`）里。我们**不再移植 TF-A**，直接用开发板出厂的这份（华清已按 FS-MP1A 配置好）。把它经共享文件夹拷进虚拟机，放到源码顶层目录（与 `u-boot.stm32` 并排，方便下一步 dd）：

```bash
cp ~/Desktop/LINUX-gy/Test2/tf-a-stm32mp157a-fsmp1a-trusted.stm32 .
ls -l tf-a-stm32mp157a-fsmp1a-trusted.stm32 u-boot.stm32
```

（路径按你实际放置的位置调整。）

**实际执行结果**：

![TF-A 拷入实测](./12_实验十一_trusted版U-Boot移植.assets/05_TF-A拷入实测.png)
> 图：实测——出厂 TF-A（241984 字节，资料包原始日期 2025-12-25）与 `u-boot.stm32`（855605 字节，14:51）已在源码顶层并排就位，只等烧写。

### 步骤 6：烧写——灌的东西换了（Slide 85）

```bash
lsblk                                   # 先确认 SD 卡仍是 /dev/sdb
sudo dd if=tf-a-stm32mp157a-fsmp1a-trusted.stm32 of=/dev/sdb1 conv=fdatasync
sudo dd if=tf-a-stm32mp157a-fsmp1a-trusted.stm32 of=/dev/sdb2 conv=fdatasync
sudo dd if=u-boot.stm32 of=/dev/sdb3 conv=fdatasync
sudo eject /dev/sdb                     # 弹出 SD 卡（命令行版"安全弹出"），之后直接拔卡插板
```

最后一条 `sudo eject /dev/sdb` 是"弹出 SD 卡"的命令行等价：`dd` 的 `conv=fdatasync` 已保证数据落盘，`eject` 再把分区卸载干净、让系统松开设备——此后直接拔卡插板即可，不必再去文件管理器手动点弹出。（若 eject 后读卡器从虚拟机断开，下次烧写前用「虚拟机 → 可移动设备」重新连上即可。）

与 basic 版的烧写命令对比，变化全在"灌什么"：

| 分区 | basic 版烧的 | trusted 版烧的 |
|---|---|---|
| sdb1（fsbl1） | 我们的 `u-boot-spl.stm32` | 出厂 `tf-a-...-trusted.stm32` |
| sdb2（fsbl2） | 同上（冗余备份） | 同上（冗余备份） |
| sdb3（ssbl） | `u-boot.img` | `u-boot.stm32` |

TF-A 照样烧两份——与实验四 SPL 烧两份同理：fsbl1/fsbl2 互为备份，第一份损坏时引导程序自动启用第二份。SPL 自此退出本系列的舞台。

> 卡上的 basic 版镜像被这次烧写覆盖了。想回 basic 版：`make stm32mp15_fsmp1a_basic_defconfig` → `make` → 重烧三条 basic 镜像即可，源码与配置都在版本库里。

**实际执行结果**：

```
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$ lsblk
NAME   MAJ:MIN RM   SIZE RO TYPE MOUNTPOINT
loop0    7:0    0     4K  1 loop /snap/bare/5
loop1    7:1    0  63.8M  1 loop /snap/core20/2769
loop2    7:2    0  63.8M  1 loop /snap/core20/2922
loop3    7:3    0 248.8M  1 loop /snap/gnome-3-38-2004/99
loop4    7:4    0  91.7M  1 loop /snap/gtk-common-themes/1535
loop5    7:5    0  48.4M  1 loop /snap/snapd/26382
loop6    7:6    0 531.5M  1 loop /snap/gnome-42-2204/263
loop7    7:7    0 349.7M  1 loop /snap/gnome-3-38-2004/143
loop8    7:8    0  12.2M  1 loop /snap/snap-store/1216
loop9    7:9    0    74M  1 loop /snap/core22/2955
loop10   7:10   0  65.2M  1 loop /snap/gtk-common-themes/1519
loop11   7:11   0  54.2M  1 loop /snap/snap-store/558
loop12   7:12   0  50.3M  1 loop /snap/snapd/27738
sda      8:0    0   100G  0 disk 
├─sda1   8:1    0   512M  0 part /boot/efi
├─sda2   8:2    0     1K  0 part 
└─sda5   8:5    0  99.5G  0 part /
sdb      8:16   1  29.8G  0 disk 
├─sdb1   8:17   1   256K  0 part 
├─sdb2   8:18   1   256K  0 part 
├─sdb3   8:19   1     2M  0 part 
├─sdb4   8:20   1    64M  0 part 
└─sdb5   8:21   1  29.7G  0 part 
sr0     11:0    1   3.2G  0 rom  /media/cnu/Ubuntu 20.04.4 LTS amd64
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$ sudo dd if=tf-a-stm32mp157a-fsmp1a-trusted.stm32 of=/dev/sdb1 conv=fdatasync
[sudo] cnu 的密码： 
记录了472+1 的读入
记录了472+1 的写出
241984字节（242 kB，236 KiB）已复制，0.282562 s，856 kB/s
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$ sudo dd if=tf-a-stm32mp157a-fsmp1a-trusted.stm32 of=/dev/sdb2 conv=fdatasync
记录了472+1 的读入
记录了472+1 的写出
241984字节（242 kB，236 KiB）已复制，0.272525 s，888 kB/s
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$ sudo dd if=u-boot.stm32 of=/dev/sdb3 conv=fdatasync
记录了1671+1 的读入
记录了1671+1 的写出
855605字节（856 kB，836 KiB）已复制，0.976941 s，876 kB/s
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$ 

```

### 步骤 7：上电验证（Slide 86）

板子断电 → 插卡 → 拨码 `101` → 上电，看 MobaXterm 串口。

这一站的里程碑有**两个**，按出场顺序：

1. **开机第一段日志换了主角**：不再以 `U-Boot SPL ...` 横幅开场，而是 TF-A 的引导日志（`NOTICE:` / `INFO:` 开头的若干行——实验四之前板子跑出厂系统时见过的那套开场白）。FSBL 换人，看得见。
2. **U-Boot 横幅之后**：

```
Board: stm32mp1 in trusted mode (st,stm32mp157a-dk1)
```

`trusted mode` 一出现，trusted 版移植即告成功。其余成果应全线保持：`MMC:   STM32 SD/MMC: 0, STM32 SD/MMC: 1`（F-6 在）、`Net:   eth0: ethernet@5800a000`（F-5 在）、无电源报错（F-1 在）、无 ADC 报错（F-3 在），最终停在 `STM32MP>`。

![trusted 版串口日志](./12_实验十一_trusted版U-Boot移植.assets/06_trusted版串口日志.png)
> 图：课件 Slide 86——trusted 版 U-Boot 的串口输出：`Board: stm32mp1 in trusted mode (st,stm32mp157a-dk1)`、`DRAM: 512 MiB`、`MMC: STM32 SD/MMC: 0, STM32 SD/MMC: 1`、`Loading Environment from MMC... OK`、`Net: eth0: ethernet@5800a000`、`Hit any key to stop autoboot: 0`，进入 `STM32MP>` 命令行。

> 课件截图的版本串是 `2020.01-stm32mp-r1-gae7d1c12 (Jun 15 2023 ...)`——那是华清演示时的构建；我们编出来的版本串哈希与时间戳必然不同，正常。看准的是 `in trusted mode` 那一行，不是哈希。按版本串接力规律，本次构建应从 `g88f08870`（F-6 提交）起跳；且 trusted 流程的配置改动落在 `.config`（不入库）与**新增的** defconfig（提交前只是 untracked 文件）里，都不弄脏工作区——版本串预计**不带 `-dirty`**，与 basic 阶段 F 系列构建全带 `-dirty` 形成有趣对照。

**实际执行结果**：

```
NOTICE:  CPU: STM32MP157AAA Rev.Z
NOTICE:  Model: HQYJ FS-MP1A Discovery Board
INFO:    Reset reason (0x15):
INFO:      Power-on Reset (rst_por)
INFO:    Using SDMMC
INFO:      Instance 1
INFO:    Boot used partition fsbl1
NOTICE:  BL2: v2.2-r1.0(debug):a70053f
NOTICE:  BL2: Built : 09:55:29, Nov  5 2020
INFO:    Using crypto library 'stm32_crypto_lib'
INFO:    BL2: Doing platform setup
INFO:    RAM: DDR3-DDR3L 16bits 533000Khz
INFO:    Memory size = 0x20000000 (512 MB)
INFO:    BL2 runs SP_MIN setup
INFO:    BL2: Loading image id 4
INFO:    Loading image id=4 at address 0x2ffed000
INFO:    Image id=4 loaded: 0x2ffed000 - 0x2ffff000
INFO:    BL2: Loading image id 5
INFO:    Loading image id=5 at address 0xc0100000
INFO:    STM32 Image size : 855349
INFO:    Image id=5 loaded: 0xc0100000 - 0xc01d0d35
WARNING: Skip signature check (header option)
NOTICE:  ROTPK is not deployed on platform. Skipping ROTPK verification.
NOTICE:  BL2: Booting BL32
INFO:    Entry point address = 0x2ffed000
INFO:    SPSR = 0x1d3
INFO:    Cannot find st,stpmic1 node in DT
NOTICE:  SP_MIN: v2.2-r1.0(debug):a70053f
NOTICE:  SP_MIN: Built : 09:55:29, Nov  5 2020
INFO:    ARM GICv2 driver initialized
INFO:    stm32mp IWDG1 (12): Secure
INFO:    ETZPC: CRYP1 (9) could be non secure
INFO:    SP_MIN: Initializing runtime services
INFO:    SP_MIN: Preparing exit to normal world


U-Boot 2020.01-stm32mp-r1-g88f08870-dirty (Sep 19 2026 - 14:51:04 +0800)

CPU: STM32MP157AAA Rev.Z
Model: STMicroelectronics STM32MP157A-DK1 Discovery Board
Board: stm32mp1 in trusted mode (st,stm32mp157a-dk1)
DRAM:  512 MiB
Clocks:
- MPU : 650 MHz
- MCU : 208.878 MHz
- AXI : 266.500 MHz
- PER : 24 MHz
- DDR : 533 MHz
WDT:   Started with servicing (32s timeout)
NAND:  0 MiB
MMC:   STM32 SD/MMC: 0, STM32 SD/MMC: 1
Loading Environment from MMC... OK
In:    serial
Out:   serial
Err:   serial
Net:   eth0: ethernet@5800a000
Hit any key to stop autoboot:  0
Boot over mmc0!
switch to partitions #0, OK
mmc0 is current device
** Unrecognized filesystem type **
STM32MP>
```

逐行看这份日志——这是一场"两个世界"的接力：

**TF-A 段**（`NOTICE:` / `INFO:` 开场，没有 SPL 横幅——FSBL 换人，看得见）：

- `Model: HQYJ FS-MP1A Discovery Board`——华清出厂 TF-A 自报家门"FS-MP1A"；而紧接着的 U-Boot 段报的却是 `STM32MP157A-DK1 Discovery Board`——我们沿用的是 DK1 设备树（课件的做法，F 系列从没改过 Model 字符串）。FSBL 与 SSBL 各说各的，正是"出厂 TF-A + 自移植 U-Boot"这个组合留下的有趣烙印；
- `Boot used partition fsbl1`——TF-A 从 fsbl1（sdb1）起跳，烧的两份里第一份就干了活；
- `BL2: v2.2-r1.0(debug):a70053f`、`Built : 09:55:29, Nov 5 2020`——出厂 TF-A 的版本与构建日期，解读方式与 U-Boot 版本串同理（这份 2020 年的固件，比我们的实验早了六年）；
- `STM32 Image size : 855349`——TF-A 从 ssbl 分区读到的 U-Boot 大小。我们 dd 的 `u-boot.stm32` 是 855605 字节，855605 − 855349 = **256 = STM32 镜像头大小**——数字对上了，加载的正是我们编的那个文件；
- `Cannot find st,stpmic1 node in DT`——眼熟吗？FS-MP1A 板上根本没有 STPMIC1 这颗 PMIC（正是 F-1 的病根），华清改 TF-A 时同样把它的节点删了，这条 INFO 只是"按 DK1 模板找而没找到"的说明，不影响引导；
- `WARNING: Skip signature check`、`ROTPK is not deployed`——出厂镜像没部署签名校验，开发板上的常态，忽略。

**U-Boot 段**：

- 版本串 `g88f08870-dirty (Sep 19 2026 - 14:51:04 +0800)`——哈希 = F-6 提交，**接力预测兑现**；但**带了 `-dirty`，与"无 `-dirty`"的预测不符**。破案在步骤 8：`git status --short` 显示已跟踪文件一个没动、只有两个**未跟踪**新文件（trusted defconfig 与 TF-A 素材）——对照 F-3 那次"变动的只有被忽略的 `.config` → 无 `-dirty`"，可以定位：**本版 U-Boot 的 dirty 判定把未跟踪文件也算数**（被忽略的不算）。这两个 `??` 一天躺在仓库里，版本串就一天带着 `-dirty`；按步骤 8 提交并忽略之后，下次构建就是一个干干净净的新哈希；
- `Board: stm32mp1 in trusted mode (st,stm32mp157a-dk1)`——**本站里程碑达成**；
- `MMC:   STM32 SD/MMC: 0, STM32 SD/MMC: 1`——F-6 的设备树天然继承（aliases 的 `mmc1` 在 `#ifndef` 段之外，trusted 保留）；`Net:   eth0: ethernet@5800a000`——F-5 在；无电源报错（F-1 在）、无 ADC 报错（F-3 在）——全线保持；
- `Loading Environment from MMC... OK`——basic 阶段 `saveenv` 存下的环境（在 ssbl 分区里，与 U-Boot 版本无关）对 trusted 版照样有效；倒计时归零 → `Boot over mmc0!` → `** Unrecognized filesystem type **` 落回 `STM32MP>`——老剧本照旧，卡上没内核仍是后续章节的课题。

![trusted 串口完整日志实测](./12_实验十一_trusted版U-Boot移植.assets/07_串口完整日志实测.png)
> 图：MobaXterm（COM11）实测完整日志——TF-A 的 `NOTICE:`/`INFO:` 开场、U-Boot `in trusted mode`、`MMC: 0, 1` 双控制器、`Net: eth0`，最终停在 `STM32MP>`，与上方文本日志同一次启动。

![串口动图](./12_实验十一_trusted版U-Boot移植.assets/08_串口动图.gif)
> 图：上电到 `STM32MP>` 命令行的启动过程动图——TF-A 引导日志先行，U-Boot 横幅接力登场。

### 步骤 8：收尾——git 提交

本站唯一的源码层产物是新 defconfig，但仓库顶层还躺着一个**不该入库的文件**——步骤 5 拷进来的 TF-A 素材（二进制固件）。git 默认会做 CRLF→LF 换行规范化，对文本无妨、对二进制就是**悄悄改坏数据**，所以提交前先把它请出版本管理：

```bash
git status --short      # 应见两个未跟踪文件：configs/stm32mp15_fsmp1a_trusted_defconfig、tf-a-...-trusted.stm32
echo "tf-a-stm32mp157a-fsmp1a-trusted.stm32" >> .gitignore    # 二进制素材不入库
git add -A              # .gitignore 已生效：进库的只有 defconfig 和 .gitignore
git commit -m "trusted: 新增 FS-MP1A trusted 版 defconfig"
git show --stat HEAD    # 复核：恰好 2 个文件——defconfig 与 .gitignore
```

**实际执行结果**：

实际执行时多了一个小插曲，恰好把本节开头的警告坐实：第一轮忘了先写 `.gitignore` 就 `git add -A`，TF-A 二进制被一起暂存——`add` 阶段弹出的 `warning: …中的 CRLF 将被 LF 替换` 就是"git 正在把这个'文本'做换行规范化"的现场信号。好在提交还没推送，就地修正：`git rm --cached` 把它请出暂存区（`--cached` 只动索引、磁盘文件原样保留），补写 `.gitignore`，再用 `git commit --amend --no-edit` 把上一个提交**原地改写**（`amend` 是"重做刚提交"的正规工具，改写会生成新哈希、旧的作废，且只对尚未推送的提交安全）。修正后的完整执行记录：

```
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$ git rm --cached tf-a-stm32mp157a-fsmp1a-trusted.stm32
rm 'tf-a-stm32mp157a-fsmp1a-trusted.stm32'
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$ echo "tf-a-stm32mp157a-fsmp1a-trusted.stm32" >> .gitignore
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$ git add .gitignore
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$ git commit --amend --no-edit
[WORKING e97b6818] trusted: 新增 FS-MP1A trusted 版 defconfig
 Date: Sat Sep 19 15:19:37 2026 +0800
 2 files changed, 1617 insertions(+)
 create mode 100644 configs/stm32mp15_fsmp1a_trusted_defconfig
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$ git show --stat HEAD
commit e97b68186d58478b75a176a6d7eb786e874d0a1c (HEAD -> WORKING)
Author: Gsheep0729 <2697438381@qq.com>
Date:   Sat Sep 19 15:19:37 2026 +0800

    trusted: 新增 FS-MP1A trusted 版 defconfig

 .gitignore                                 |    1 +
 configs/stm32mp15_fsmp1a_trusted_defconfig | 1616 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 2 files changed, 1617 insertions(+)
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$ git status --short
cnu@cnu-virtual-machine:~/Desktop/LINUX-gy/Test2/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/u-boot-stm32mp-2020.01-r0/u-boot-stm32mp-2020.01$
```

两条复核全部达标：`git show --stat HEAD` 恰 2 个文件——`configs/stm32mp15_fsmp1a_trusted_defconfig`（1616 行，`create mode` 里有且只有它）与 `.gitignore`（新增 1 行），合计 1617 insertions，TF-A 二进制不在其中；`git status --short` 无输出，工作区回干净。提交定档 **`e97b6818`**——WORKING 分支的新尖端，U-Boot 版本串接力再添一站：g3f0216e7 → g2224655f → g8de188df → g1ac3a506 → ec8c29dd → dd36022e → 88f08870 → e97b6818。`??` 清零之后，下次构建的版本串预计就是不带 `-dirty` 的 `ge97b6818`。

## 五、注意事项

1. **`make distclean` 不用怕**：清的是 `.config` 与编译产物；源码改动在 git、配置在两个 defconfig 里，随时可回。
2. **menuconfig 三站四处缺一不可**：电源（F-1）、ADC 两项（F-3）、网卡（F-5）；退出必须保存；改完 `cp .config configs/stm32mp15_fsmp1a_trusted_defconfig` 固化。
3. **设备树一概不用动**：F-2/F-4/F-6 的改动对两种模式同时生效——这是"驱动与配置分离"带来的红利。
4. **烧写对象变了**：sdb1/sdb2 → 出厂 TF-A；sdb3 → `u-boot.stm32`（不是 `u-boot.img`）；没有 SPL 这一环。拿旧命令顺手一烧，板子还是 basic 的老样子。
5. **TF-A 文件经共享文件夹进虚拟机**，`cp` 到哪都行，`dd` 时路径写对即可。
6. **回 basic 版的路一直都在**：`make stm32mp15_fsmp1a_basic_defconfig` → `make` → 重烧三条 basic 镜像。
7. **拨码开关不用变**：拨 `101` 选的是"从 SD 卡启动"，FSBL 是 SPL 还是 TF-A 不影响这个选择。

## 六、怎么验证 trusted 版移植成功

### 第一层：配置自检（半分钟）

```bash
grep "STPMIC1" .config                               # # CONFIG_PMIC_STPMIC1 is not set
grep -E "^# CONFIG_(CMD_)?ADC is not set" .config    # 两行
grep "MAXIO" .config                                 # CONFIG_PHY_MAXIO=y
ls -l u-boot.stm32                                   # 产物存在
grep "MAXIO\|STPMIC1\|_ADC" configs/stm32mp15_fsmp1a_trusted_defconfig   # 步骤 3 的 cp 之后：三处改动都在
```

### 第二层：上电实测（唯一的"真的生效"）

三条判据同时成立：

1. 开机第一段日志是 TF-A 的（`NOTICE:` / `INFO:` 开场），没有 `U-Boot SPL` 横幅；
2. U-Boot 横幅后出现 `Board: stm32mp1 in trusted mode (st,stm32mp157a-dk1)`；
3. `MMC:` 行两个控制器、`Net:` 行正常，无电源/ADC 报错，停在 `STM32MP>`。

不达标时的排查顺序：

| 现象 | 先查什么 |
|---|---|
| 还是 `in basic mode` | sdb3 烧的还是旧的 `u-boot.img`——重烧 `u-boot.stm32`；或者配置没切（`make stm32mp15_fsmp1a_trusted_defconfig` 跑了吗） |
| 开机还是 `U-Boot SPL` 横幅 | sdb1/sdb2 里还是旧 SPL——TF-A 的两条 dd 没做或没成功 |
| TF-A 日志之后卡住、不出 U-Boot 横幅 | sdb3 是否烧了 `u-boot.stm32`（拿错成 `u-boot.img` 最常见）；TF-A 文件是否拷完整（`ls -l` 看大小是否与 Windows 侧一致） |
| `MMC:` 只有一个控制器 | F-6 的设备树改动还在吗（`git log --oneline` 应见 F-6 提交；两种模式用同一份设备树） |
| 串口完全没有输出 | 回到实验四步骤 8 的排查（供电、拨码、串口） |

### 最后一道验证：提交本身

`git show --stat HEAD` 复核：本次应**只有 1 个新文件**（`configs/stm32mp15_fsmp1a_trusted_defconfig`）。

## 七、实验完成标志

- `configs/stm32mp15_fsmp1a_trusted_defconfig` 已建立，内含 F-1/F-3/F-5 的三处配置改动（步骤 2~3 实测，三处自检全中）
- 编译产出 `u-boot.stm32`（855605 字节），SD 卡已按新组合烧写：TF-A（241984 字节）→ sdb1/sdb2，`u-boot.stm32` → sdb3（步骤 4~6 实测）
- 串口依次出现：TF-A 引导日志 → U-Boot 横幅 → `Board: stm32mp1 in trusted mode (st,stm32mp157a-dk1)` → `MMC: STM32 SD/MMC: 0, STM32 SD/MMC: 1` → `Net: eth0` → `STM32MP>` 命令行（步骤 7 实测，版本串 `g88f08870-dirty`，14:51:04 构建）
- 已完成 git 提交 `e97b6818`：恰 2 个文件（trusted defconfig 1616 行 + `.gitignore` 1 行，合计 1617 insertions），TF-A 素材已 ignore、未入库，`git status --short` 无输出、工作区干净（步骤 8 实测，2026-09-19）

## 八、下一步：U-Boot 移植收官

课件 Slide 87：

> 至此，u-boot移植完成！！！
>
> 我们仅移植了一个可以启动Linux内核的u-boot，有些设备没有驱动，无法正常工作，如需使用某一特定设备，还要移植其驱动；u-boot的默认环境变量也没进行修改，可自行查阅资料进行修改。

回头看这十一个实验：从装交叉编译工具链、拿源码打补丁，到 basic 版配置出第一个镜像、SD 卡分区烧写，再到按串口报错逐站修完 F-1~F-6，最后换 TF-A 领队上 trusted——第 3 章《移植U-Boot》走到了终点。

课件的收尾提醒同样清晰：这个 U-Boot 只是"能启动 Linux 内核"的程度，个别设备仍无驱动、默认环境变量也没调——这些属于"使用"层面的功课。U-Boot 怎么用（环境变量、bootcmd/ bootargs、从 eMMC 或网络引导内核），正是第 4 章《使用U-Boot》的主题，专栏后续篇目继续。
