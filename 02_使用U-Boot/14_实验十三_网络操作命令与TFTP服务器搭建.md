# 实验十三 网络操作命令——PC 上搭 TFTP 服务器，板子第一次从网络拉文件

> **对应课件**：《第4章 使用U-Boot》4.4 节，Slide 17-22
>
> **系列说明**：本系列基于华清远见 FS-MP1A（STM32MP157A）开发板，对应课件《第4章 使用U-Boot》。第 3 章实验九让板子 ping 通了 PC，但那只是确认"链路通"；本篇把这条链路铺成正经的开发通道——PC 上搭一台 TFTP 服务器，板子用 `tftp` 命令把 PC 上的文件拉进内存。这条通道是后续所有内核实验的命脉：第 5 章自己编的 Linux 内核，就是从这条路下载上板调试的。本文覆盖 Slide 17-22：ping 复核、serverip 落地、TFTP 服务器搭建、tftp 下载实战、dhcp 与 nfs 认知。前置：实验十二（bootdelay 已放宽到 5 秒，拦停从容）。

## 一、网络命令四条与我们的拓扑

4.4 节给出四条网络命令：

| 命令 | 用途 | 本篇安排 |
|---|---|---|
| `ping` | 检测板子与 PC 的连通性（只能板子 ping PC） | 步骤 1 动手 |
| `dhcp` | 从路由器的 DHCP 服务自动获取 ipaddr/netmask/gatewayip | 步骤 5 认知 |
| `tftp` | 把 PC 上 TFTP 服务器的文件下载到板子内存（DRAM） | 步骤 2~4 重头戏 |
| `nfs` | 从 NFS 服务器下载文件到 DRAM（须给完整路径） | 步骤 6 认知 |

课件的环境是"板子接路由器 + Ubuntu 虚拟机里开 TFTP/NFS 服务器"；我们的环境是**网线直连**（板子网口 —— PC 的 USB 网卡"以太网 7"，实验九的拓扑），TFTP 服务器直接搭在 **Windows** 上。拓扑不同，思路不变：板子和服务器在一个网段，服务器上放着待下载的文件。

PC 侧两个 IP 是既有事实（实验九配好，不用再动）：

- "以太网 7"（ASIX USB 网卡）：`192.168.0.100`，掩码 `255.255.255.0`；
- 板子：`ipaddr 192.168.0.8`、`netmask 255.255.255.0`、`ethaddr` 已设并存盘。

本篇要新增的只有一件事：让板子知道"服务器是谁"——`serverip`。

## 二、实验环境（实际）

| 项目 | 实际值 |
|---|---|
| 板子状态 | trusted 版 U-Boot，上电倒计时 5 秒（实验十二成果），`STM32MP>` 可达 |
| 串口 | MobaXterm Serial 会话（COM11），115200 |
| 环境变量基线 | `ethaddr` / `ipaddr 192.168.0.8` / `netmask 255.255.255.0` 已设并 saveenv（实验九设定、换板后按附录重设，实验十二步骤 7 复核仍在）；`serverip` **出厂默认带着 `192.168.1.1`**（实验十二实测，不是空值）——本篇要把它改成 PC 的 `192.168.0.100` |
| PC 侧网络 | USB 网卡"以太网 7" = `192.168.0.100/24`（实验九配好） |
| 本篇新增材料 | `D:\桌面文件\资料\嵌入式linux\官方系统内核和设备树.zip`（内含 `uImage`，7,546,640 字节）；Windows 侧 TFTP 服务器软件 tftpd64 |

> **开工自检（10 秒）**：上电先看 `Hit any key to stop autoboot:` 后面那个数字——若是 **0**（换过板子、重新分区烧写后最容易回到 0），先补一句 `setenv bootdelay 5` + `saveenv`（`env set` / `env save` 等价写法）再 `reset`，往后每一步拦停才来得及按 Enter。

## 三、课件 ↔ 步骤对应表

| 课件 Slide | 内容 | 对应步骤 |
|---|---|---|
| 17 | 网络命令总览 | —（本节第一节） |
| 18 | ping 检测连通性 | 步骤 1 |
| 16 末尾例子的落地 | setenv serverip + saveenv | 步骤 2 |
| —（课件默认 Ubuntu 侧服务器已就绪） | Windows 侧搭建 TFTP 服务器 | 步骤 3 |
| 20 | tftp 下载文件到 DRAM | 步骤 4 |
| 21 | tftp 的 "Permission denied" 排查 | 步骤 4 末尾排查表 |
| 19 | dhcp 自动获取 IP（路由器场景） | 步骤 5 |
| 22 | nfs 下载（须完整路径） | 步骤 6 |

## 四、实验步骤

### 步骤 1：ping 复核链路（Slide 18）

上电，倒计时 5 秒内按 Enter 拦停，进 `STM32MP>`：

> 倒计时还是眨眼就没（`Hit any key to stop autoboot:  0` 显示 0）说明 `bootdelay` 没落成 5——先补一句再往下做：`setenv bootdelay 5` + `saveenv`（`env set` / `env save` 等价写法），`reset` 后窗口就有 5 秒。换过板子或换过卡之后尤其容易回到 0，见第 3 章末《附录 换电脑 / 换板子后，快速恢复到实验十一结束状态》。

```
STM32MP> ping 192.168.0.100
```

预期输出（实验九见过同款）：

```
ethernet@5800a000 Waiting for PHY auto negotiation to complete......... done
Using ethernet@5800a000 device
host 192.168.0.100 is alive
```

![ping通PC](./14_实验十三_网络操作命令与TFTP服务器搭建.assets/01_ping通PC.png)
> 图：课件 Slide 18——`ping 192.168.1.249` 的输出：`Waiting for PHY auto negotiation to complete......... done` 后 `host 192.168.1.249 is alive`。（课件 IP 是它的路由器环境，我们打自己的 `192.168.0.100`。）

`is alive` = 物理链路、MAC、IP、子网掩码四件事全部就位。注意 ping 只有板子打 PC 这一个方向，PC ping 板子无响应（U-Boot 不回 ICMP echo），这不是故障。

**实际执行结果**：待补充

### 步骤 2：设定 serverip 并保存

课件 4.3 节 Slide 16 那组网络变量例子，在实验十二只做了"print 复核"，本篇落地真正要用的那条：

```
STM32MP> setenv serverip 192.168.0.100
STM32MP> saveenv
STM32MP> print serverip
```

`saveenv` 后应见实验九同款两条：`Writing to MMC(0)... OK` + `Writing to redundant MMC(0)... OK`（主副本 + 冗余副本）。`serverip` 从此存盘——后续实验篇篇依赖它，设一次管到底。

**实际执行结果**：待补充

### 步骤 3：PC 侧搭建 TFTP 服务器（tftpd64）

课件假设 Ubuntu 里开 TFTP 服务；我们的服务器在 Windows 上，用经典小工具 **tftpd64**（绿色免安装）：

1. **准备下载目录**：建 `D:\tftpboot`，把测试文件放进去——解压 `D:\桌面文件\资料\嵌入式linux\官方系统内核和设备树.zip`（右键 → 全部解压，解出来会套一层同名文件夹），把里面的 `uImage`（7,546,640 字节）拷到 `D:\tftpboot` 根下，**不要套子目录**（TFTP 按文件名直接取，不递归找）。同目录里那份 `stm32mp157a-fsmp1a-mipi050.dtb` 本篇用不到，先留在解压处，实验十六再取。
2. **下载 tftpd64**：官网 tftpd32.jounin.net 下载 tftpd64 便携版 zip，解压即用，运行 `tftpd64.exe`。
3. **配置两处**（主界面即可）：
   - **Current Directory**（当前目录）：选 `D:\tftpboot`——这就是 TFTP 的根目录，板子要的文件都在这里找；
   - **Server interfaces**（服务接口）：下拉**只勾 `192.168.0.100`**——PC 上网卡多，选错了板子连的就是错误接口，这是本篇最常见翻车点。
4. **防火墙放行**：首次运行 tftpd64 会弹"Windows 安全中心警报"，**勾选"专用网络"后点"允许访问"**。如果没弹或漏点了：控制面板 → Windows Defender 防火墙 → 允许应用通过防火墙 → 找到 tftpd64 勾上"专用"。TFTP 走 UDP 69 端口，被防火墙拦的症状是板子 `tftp` 命令卡在 `Loading:` 不动（实验九的 ICMP 被拦是同款剧情）。

配置完成后 tftpd64 保持运行，主界面日志区随后能看到板子来访的记录。

**实际执行结果**：待补充

### 步骤 4：tftp 下载实战（Slide 20）

板子侧，把 PC 上的 uImage 拉到 DRAM 的 `0xc2000000` 处（课件同款地址，第 5 章沿用）：

```
STM32MP> tftp c2000000 uImage
```

预期输出：

```
Using ethernet@5800a000 device
TFTP from server 192.168.0.100; our IP address is 192.168.0.8
Filename 'uImage'.
Load address: 0xc2000000
Loading: ################################################################
         ################################################################
         ...
done
Bytes transferred = 7546640 (732710 hex)
```

![tftp下载uImage](./14_实验十三_网络操作命令与TFTP服务器搭建.assets/02_tftp下载uImage.png)
> 图：课件 Slide 20——`tftp c2000000 uImage` 下载成功的完整回显：`TFTP from server ...; our IP address is ...`、`Filename 'uImage'.`、`Load address: 0xc2000000`、进度条、`Bytes transferred = ...`。

三处对账，一处都不能错：

- `TFTP from server 192.168.0.100; our IP address is 192.168.0.8`——两个 IP 与步骤 2/实验九的设定一致；
- `Bytes transferred = 7546640`——与 PC 上 `uImage` 的文件大小（右键属性）**一字不差**，这是"完整下载"的唯一铁证；
- `732710 hex`——就是 7,546,640 的十六进制，顺带复习"U-Boot 数值一律十六进制"。

几 MB 的文件几秒钟传完。这个 7.2 MiB 的内核镜像下载到 `0xc2000000` 之后**本篇不用管它**——实验十六会在同一地址把它点火。

**卡住/报错的排查表**：

| 现象 | 原因与解法 |
|---|---|
| `Loading:` 后光标久闪不动 | 十有八九是防火墙拦了 UDP 69——回步骤 3 第 4 小步补放行 |
| `TFTP error: 'Permission denied' (0)` | 课件 Slide 21 的场景（下图）：服务器侧文件/目录无读权限。Linux 服务器用 `chmod` 补权限；Windows 侧 tftpd64 一般不报这个，先查 Current Directory 是否选对、文件是否真的在 `D:\tftpboot` 根下 |
| `TFTP error: 'File not found'` | 文件名大小写要对（`uImage`，TFTP 区分大小写）；文件没放根目录 |

![tftp权限错误](./14_实验十三_网络操作命令与TFTP服务器搭建.assets/03_tftp权限错误.png)
> 图：课件 Slide 21——tftp 报 `TFTP error: 'Permission denied' (0)` 后 `Starting again` 的样子。课件给出的两个可能原因：文件所在目录无 read 权限、文件本身无 read 权限，服务器上 `chmod` 解决。

**实际执行结果**：待补充

### 步骤 5：dhcp——知道它是干嘛的（Slide 19）

`dhcp` 的适用场景是"板子接路由器，路由器开着 DHCP 服务"：一条命令自动把 `ipaddr`、`netmask`、`gatewayip` 全拿到，省掉手工 setenv。课件截图里 `DHCP client bound to address 192.168.1.7 (787 ms)` 就是这个效果：

![dhcp自动获取IP](./14_实验十三_网络操作命令与TFTP服务器搭建.assets/04_dhcp自动获取IP.png)
> 图：课件 Slide 19——`dhcp` 输出 `BOOTP broadcast 1 / 2 / 3` 后 `DHCP client bound to address 192.168.1.7 (787 ms)`，IP 是路由器分派的。

![查dhcp用法](./14_实验十三_网络操作命令与TFTP服务器搭建.assets/05_查dhcp用法.png)
> 图：课件 Slide 19——`? dhcp` 给出的用法：`dhcp [loadAddress] [[hostIPaddr:]bootfilename]`——它本质上还能借 TFTP 顺带下载并启动内核（"boot image via network using DHCP/TFTP protocol"）。

**我们的直连拓扑上敲 `dhcp` 会怎样**：网线那头是 PC，PC 上没有 DHCP 服务，板子会 `BOOTP broadcast 1、2、3…` 广播几轮后超时放弃退出。**这是预期行为，不是故障**——想看到课件同款的"自动分到 IP"，把板子接到开了 DHCP 的路由器下即可（拿到什么 IP 与本系列的 `192.168.0.x` 静态规划无关，实验完改回静态值即可）。本步骤敲一次、观察到广播超时、能讲清"为什么课件能拿到我们拿不到"，就达标。

**实际执行结果**：待补充

### 步骤 6：nfs——知道它和 tftp 的差别（Slide 22）

`nfs` 同样能把 PC 上的文件下载到 DRAM，两点不同：

- 前置更重：PC 上要搭的是 **NFS 服务器**（不是 TFTP）；
- 命令更啰嗦：必须给出文件在服务器上的**完整路径**，课件原话"使用不太方便"：

```
STM32MP> nfs 0xc2000000 192.168.1.249:/home/zuozhongkai/linux/nfs/uImage
```

![nfs下载示例](./14_实验十三_网络操作命令与TFTP服务器搭建.assets/06_nfs下载示例.png)
> 图：课件 Slide 22——`nfs 0xc2000000 192.168.1.249:/home/zuozhongkai/linux/nfs/uImage`：注意命令里带着服务器上的绝对路径 `/home/zuozhongkai/linux/nfs/uImage`。

本系列**暂不搭建** NFS 服务器：Windows 侧搭 NFS 成本高，而"把文件从 PC 弄进板子内存"这件事 `tftp` 已经完全覆盖。`nfs` 到此认识格式、记住差异即可；哪天后续章节真的要用 NFS 挂载，再回头搭不迟。

**实际执行结果**：待补充

## 五、注意事项

1. **防火墙是本篇第一反派**：ICMP 拦 ping（实验九）、UDP 69 拦 tftp（本篇），同一面墙。tftp 卡住先查防火墙，再查别的。
2. **Server interfaces 必须选 `192.168.0.100`**：PC 上物理网卡、虚拟网卡一大把，tftpd64 默认可能监听在别的接口上——板子那边表现为连不上或超时。
3. **文件放 `D:\tftpboot` 根目录**、文件名一字不差（区分大小写）：tftpd64 只在当前目录找文件，不递归子目录。
4. **`tftp` 的地址参数是十六进制**：`c2000000` 即 `0xC2000000`，`0x` 可省——从本篇起这个地址约定（内核 → `c2000000`、设备树 → `c4000000`）全系列统一，第 5 章沿用。
5. **`dhcp` 超时不是故障**：直连拓扑没有 DHCP 服务器，广播几轮放弃是正常收场。
6. **终端粘贴用右键**：MobaXterm 里 Ctrl+C 是中断、Ctrl+V 不粘贴（实验九的教训）。

## 六、怎么验证

两条硬判据：

1. `ping 192.168.0.100` 回 `host 192.168.0.100 is alive`；
2. `tftp c2000000 uImage` 回 `Bytes transferred = 7546640`——与 PC 侧文件属性里的字节数一致。

软判据：`print serverip` 显示已保存；能口头讲清 dhcp 在我们拓扑为何超时、nfs 与 tftp 的两点差别。

不达标时的排查：

| 现象 | 先查什么 |
|---|---|
| ping 不通 | 网线两端插紧？实验九后 PC 网卡 IP 有没有被改动？"以太网 7"还是 `192.168.0.100` 吗 |
| ping 通但 tftp 卡住 | 防火墙（注意事项 1）、tftpd64 是否在运行、Server interfaces 是否勾对 |
| `File not found` | 文件名大小写、文件是否在 `D:\tftpboot` 根目录 |
| 字节数对不上 | 传输被打断——重跑一遍 `tftp`；仍不对则对比 PC 侧文件是否完整解压 |

## 七、实验完成标志

- `ping 192.168.0.100` 通（步骤 1，待实测）
- `serverip` 已设并 saveenv，`print serverip` 复验在（步骤 2，待实测）——注意板子上 `serverip` 出厂自带 `192.168.1.1`，是"改"不是"补"，见实验十二步骤 7 实测
- PC 侧 tftpd64 运行中，`D:\tftpboot` 根下有 `uImage`（步骤 3，待实测）
- `tftp c2000000 uImage` 下载成功，`Bytes transferred = 7546640`（步骤 4，待实测）
- `dhcp` 直连拓扑下广播超时——现象亲测、原因说得清（步骤 5，待实测）
- nfs 与 tftp 的差异、"Permission denied" 的排查思路说得出（步骤 4~6，待实测）

## 八、下一步：eMMC 和 SD 卡操作命令

下一篇覆盖 4.5 节（Slide 23-34）：`mmc` 命令族——`mmc info` / `mmc list` / `mmc dev` / `mmc part` / `mmc read`。第 3 章实验十只摸过 eMMC 的芯片信息，本篇要把它的**分区表**摸出来——这张"地图"是实验十五（ext4 文件操作）和实验十七（从 eMMC 启动）的入场券。
