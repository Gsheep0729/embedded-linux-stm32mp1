# 附录 | FS-MP1A 板上 Qt 开发环境速查：能力边界、快速开始与避坑

> **系列说明**：本篇是**附录**（备查文档，不占专栏序号），服务第 10 章《GUI 应用程序开发》之后的全部板上 Qt 开发——特别是番外篇《板上像素马里奥》、番外篇之二《板上系统监视器》之后想在这块板子上写自己程序的你。读者假设：手里有一块**配置好的板子**（出厂内核 + 带 Qt 的根文件系统，按实验三十六/三十七做完，或按各册换机换板恢复手册恢复出来）。目标：10 分钟建立环境认知、知道这套环境**能干什么不能干什么**、照着命令走通第一个程序；卡住了按第五节「调试方法论」三步破案。

> **素材与出处**：本文零图片纯命令篇，无需任何素材。环境全部由第 6~10 章建出（逐项出处见文末对照表）；"fb0 抓帧看画面"调试法来自番外篇实战。文内 `<共享目录>` = 你的 VMware 共享文件夹（本机示例 `~/Desktop/LINUX-gy/Test2`），`<buildroot源码>` = buildroot-2020.02.6 解压目录，照抄占位符会报"没有那个文件或目录"。

## 一、三层环境全景

| 层 | 机器 / 路径 | 里面有什么 | 建在哪一章 |
|----|-------------|-----------|-----------|
| **开发机** | Ubuntu 虚拟机（仅主机网 `192.168.56.101`，用户 cnu） | buildroot 产物 `output/host/bin/`：交叉 `arm-none-linux-gnueabihf-g++/gdb`、**qmake（Qt 5.12.8）**、sysroot（`output/host/arm-buildroot-linux-gnueabihf/sysroot`）；Qt Creator 已配好 "Buildroot ARM" Kit | 实验二十八/三十六/三十七 |
| **部署通道** | NFS 根 `/home/cnu/nfsboot/rfs-buildroot` | **这个目录就是板子的 `/`**：程序 `sudo cp` 进 `opt/` 后板上立即可见，不用重启板子 | 实验二十七/二十八/二十九 |
| **板子** | 出厂内核 + 新根（启动变量 `fcsys`），IP `192.168.0.8`，root/123 | Qt 运行库在 `/usr/lib`，自己的程序放 `/opt`；显示/触摸驱动**只有出厂内核有**；sshd 已开（PermitRootLogin yes） | 实验二十三/三十六 |

三条铁律：

1. **跑 GUI 一律出厂内核**：U-Boot 拦停后 `run fcsys`。上电默认行为取决于上次 `saveenv` 的 bootcmd（本课历史上指向自制内核 `run mybootnet`）。拿不准就 `uname -a`：编译时间 **Apr 2020 / oe-user = 出厂内核（对）**；Sep 2026 自编串 = 进错内核，黑屏是预期不是故障，复位拦停改跑 fcsys。
2. **交叉编译只能用 buildroot 的 qmake**：`<buildroot源码>/output/host/bin/qmake`。系统 qmake/别的工具链编出来的程序板上缺库。
3. **运行必带 `-platform linuxfb`**：板上只编了 linuxfb 后端，Qt 默认找 eglfs 必报 `Could not find the Qt platform plugin "eglfs"`。

## 二、能力与边界（在有限环境里写程序）

这套环境的每一条边界都实测过（番外篇马里奥就是全程按这张表写出来的）：

| 维度 | 现状 | 写程序时怎么办 |
|------|------|---------------|
| 显示后端 | 只有 **linuxfb**（minimal/offscreen/vnc 不算数）；**无 OpenGL/EGL** → eglfs、QML/QtQuick 整条技术线不可用，纯 CPU 软渲染 | 用 **Widgets + QPainter 自绘**；要流畅动画学番外篇三件套：离屏小 `QImage`（如 240×400）→ 整数倍放大贴屏 → 固定步长逻辑（60 步/秒） |
| 屏幕 | MIPI050 竖屏 **480×854**（RGB565，fb 虚拟宽 512、stride 1024 字节） | 程序里用 `QGuiApplication::primaryScreen()->availableGeometry()` 自适应 + 整数倍缩放居中，别写死分辨率；查询命令见下 |
| 触摸 | Goodix 电容屏走 **evdev**（Qt 自动探测）；**多点可用**；第一触点会自动合成鼠标事件 | 只要点点按按：收鼠标事件就够（calculator 就是这样）；要**双指同按**必须 `setAttribute(Qt::WA_AcceptTouchEvents)` + 处理 `QTouchEvent` 按触点 id 跟踪（番外篇方向盘/A 键是现成样例）；没反应再试 `QT_QPA_GENERIC_PLUGINS=evdevtouch:/dev/input/eventX` |
| 字体 | 根上只有 DejaVu 系西文字体，**没有中文字体** | 界面文字用英文/数字最稳（还自带 FC 复古气质）；一定要中文：拷一个 ttf/otf 上板 + `QFontDatabase::addApplicationFont()` 动态加载；或学番外篇**自绘点阵字库**（零依赖、放大不糊） |
| 音频 | **无声卡驱动**（cs42l51 probe failed、No soundcards found），QtMultimedia 也没编 | 做静音应用；界面上别画喇叭 |
| Qt 模块 | buildroot 只编了 **qt5base**（本课实测 = Core / Gui / Widgets） | `.pro` 写 `QT += widgets` 即可；想用 Network/SQL 等，先 `ls <buildroot源码>/output/host/arm-buildroot-linux-gnueabihf/sysroot/usr/lib/ \| grep Qt5` 核实库存在再用 |
| 网络 | 板子 ↔ Ubuntu（.100，NFS/TFTP 服务器）↔ Windows（.99）局域网互通，**无外网** | 板上程序别指望在线 API；反过来，局域网让调试通道非常稳（ssh / gdbserver / NFS 三条路） |
| 性能 | 双核 Cortex-A7 @800MHz，软件渲染 | 240×400 逻辑分辨率 + 60Hz 定时器毫无压力；**全屏原生分辨率大面积半透明混合每帧重画会掉帧**——按钮/浮层画在最上层、能用小画布就别全屏刷 |
| 存储 | 根文件系统 = NFS（落在 Ubuntu 磁盘），**断电不丢**、空间不限 | 程序放 `/opt`，数据文件也写 `/opt`（番外篇最高分 `/opt/hiscore.txt` 实测持久）；**板子挂根运行时严禁 Ubuntu 侧 `rm -rf` 根目录重部署**——板子会 Stale file handle 只能硬复位；更新一律覆盖式 `cp` / `tar xf` |

就地核实环境的四条命令（板上）：

```
cat /sys/class/graphics/fb0/virtual_size     # 实测 480,854
cat /sys/class/graphics/fb0/bits_per_pixel   # 实测 16（RGB565）
cat /proc/bus/input/devices | grep -i -A4 goodix   # 触摸设备与其 eventX 号
ls /opt                                      # 自己程序的落点（qt-demo 在旁做伴）
```

## 三、快速开始两条路线

### 路线 A：命令行（日常开发/调试推荐，番外篇同款）

工程三件套（`.pro` + 源码）放共享目录，Ubuntu 侧：

```bash
cd <共享目录>/你的工程
B=<buildroot源码>/output/host/bin
$B/qmake 你的工程.pro
make -j2
```

**就地验证**：`file 你的程序` → `ELF 32-bit LSB executable, ARM, EABI5 ... interpreter /lib/ld-linux-armhf.so.3` = ARM 产物实锤；报 x86 = qmake 用错了。

部署（**sudo 规矩**：NFS 根里 root 属主文件多，凡往里拷一律 sudo）：

```bash
echo 123 | sudo -S cp 你的程序 /home/cnu/nfsboot/rfs-buildroot/opt/
```

板上运行（先确认在出厂内核）：

```
/opt/你的程序 -platform linuxfb
```

ssh 一条龙（串口可以让给日志看；Windows 侧 PowerShell/MobaXterm 也能直接 ssh，番外篇实测）：

```bash
ssh root@192.168.0.8 '/opt/你的程序 -platform linuxfb > /tmp/app.log 2>&1 & sleep 3; cat /tmp/app.log; ps | grep 你的程序 | grep -v grep'
```

### 路线 B：Qt Creator 一键部署（演示/截图好看，实验三十七已配好）

Kit "Buildroot ARM" 四项弹药都在；新增工程只要注意两处：**Run 的 Command line arguments 填 `-platform linuxfb`**（不填必报 eglfs 崩溃）；`.pro` 部署路径写 **`target.path = /opt`**（别带子目录，板上 SFTP 建目录会 Failure）。mkspec 手填 `devices/linux-buildroot-g++`。

## 四、调试工具箱

| 工具 | 用法 | 适用 |
|------|------|------|
| **qDebug / 日志重定向** | `nohup /opt/程序 -platform linuxfb > /tmp/app.log 2>&1 &` 然后 `cat /tmp/app.log` | 一切运行期报错（Qt 插件、缺库、断言）都先看这里 |
| **fb0 抓帧看画面** | 见下方三步 | 隔空"看"板子屏幕——不用拍照就能核对渲染，番外篇全程用它验收 |
| **gdbserver 远程断点** | 板上 gdbserver 已装（实验三十六）；Qt Creator 里按实验三十七步骤 6 配置 Remote Debugging | 逻辑 bug 单步调试 |
| **单实例清理** | `killall 程序名` | 重启程序前先杀旧实例，两个 GUI 同开 fb0 会叠影 |
| **清屏** | `dd if=/dev/zero of=/dev/fb0 bs=1024 count=854` | 程序退出后画面残留时擦掉（写满会报 ENOSPC，正常现象同实验三十六） |

**fb0 抓帧三步**（板上 → Windows，零依赖工具）：

```
# ① 板上抓一帧
cat /dev/fb0 > /tmp/fb.raw
# ② 传回开发机（在 Windows/Ubuntu 侧执行）
scp root@192.168.0.8:/tmp/fb.raw .
# ③ 按 RGB565 + stride 1024 转 PNG 后用看图工具打开
```

第 ③ 步的转换脚本（Python，自研工具、按需自取）：

```python
# fb2png.py —— 板子 fb0 帧转换：RGB565、行跨度 1024B（虚拟宽 512）、可见 480x854
# 用法: python fb2png.py fb.raw fb.png ；换屏幕先改 W/H/STRIDE
import struct, zlib, sys

RAW, OUT = sys.argv[1], sys.argv[2]
W, H, STRIDE = 480, 854, 1024

data = open(RAW, 'rb').read()
pix = bytearray(W * H * 3)
for y in range(H):
    row = data[y*STRIDE : y*STRIDE + W*2]
    for x in range(W):
        v = row[x*2] | (row[x*2+1] << 8)
        r = ((v >> 11) & 0x1F) << 3; r |= r >> 5
        g = ((v >> 5) & 0x3F) << 2;  g |= g >> 6
        b = (v & 0x1F) << 3; b |= b >> 5
        i = (y*W + x) * 3
        pix[i] = r; pix[i+1] = g; pix[i+2] = b

def chunk(tag, d):
    c = tag + d
    return struct.pack('>I', len(d)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)

png  = b'\x89PNG\r\n\x1a\n'
png += chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0))
raw = bytearray()
stride = W*3
for y in range(H):
    raw.append(0); raw += pix[y*stride:(y+1)*stride]
png += chunk(b'IDAT', zlib.compress(bytes(raw), 6))
png += chunk(b'IEND', b'')
open(OUT, 'wb').write(png)
```

**步长怎么算**：`ls -l fb.raw` 的字节数 ÷ 屏高 = 行跨度（本板 874496 ÷ 854 = 1024）；行跨度 ÷ 每像素字节（bits_per_pixel÷8）= 虚拟宽（1024 ÷ 2 = 512 ≥ 可见宽 480）。

## 五、调试方法论：三步破案（番外篇之二《系统监视器》全案沉淀）

> 程序在板上跑，看不见摸不着，但调试通道全是现成的。番外篇之二从开工到验收的每个问题（黑帧、内存读成 0、LED 显示 NONE、uptime 对不上）都是下面几步破的案——工具就是上方工具箱那几样，这里是**怎么组合着用**。

**第 1 步：先确认"程序活着"，再谈别的**

```bash
ssh root@192.168.0.8 'ps | grep 你的程序 | grep -v grep; cat /tmp/你的日志.log'
```

进程在、日志空，很可能只是**抓太早**：Qt 平台初始化要几秒，1 秒级定时器的程序还要等首个采样周期才出首帧——监视器实测 `sleep 4` 抓帧全黑、`sleep 8` 画面全对。**黑帧 ≠ 崩了，睡 6~8 秒再验。**进程没了才去看日志报错、对照第六节排查表。

**第 2 步：抓帧看屏 + 两帧对比**

fb0 抓帧三步见上方工具箱。补两条实战经验：

- **两次抓帧做 md5 对比**——相同 = 画面没在刷新（程序没画或冻帧），不同 = 程序活着在动：

```
md5sum /tmp/fb1.raw /tmp/fb2.raw
```

**第 3 步：UI ↔ 内核对账（"屏上说的不算，内核文件才算"）**

屏上每个数字，板上都有一个文件对应。UI 与预期不符时**先 cat 真值再怀疑代码**——监视器两次"以为有 bug"其实都是板上真值：屏显 `CPU LED: NONE`（板上 trigger 那轮开机真是 none，不是心跳），屏显 uptime 偏小（板子真的悄悄重启过，见下方环境陷阱）：

| 屏上内容 | 对账命令（板上） |
|----------|----------------|
| 开机时长 / 进程数 | `cat /proc/uptime`；`cat /proc/loadavg`（第 4 字段 `运行/总数`） |
| 内存 | `head -3 /proc/meminfo` |
| CPU 占用 | `grep ^cpu /proc/stat` 两次间隔几秒，看第 4/5 列（idle/iowait）差分 |
| 频率 / 温度 | `cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq`；`cat /sys/class/thermal/thermal_zone0/temp` |
| LED 卡片状态 | 下方巡检命令 |

**LED 状态一键巡检**（监视器五模式验收就是这么逐个核的；`$T` 取方括号里的当前 trigger）：

```bash
for L in cpu user1 user2; do
  T=$(cat /sys/class/leds/$L/trigger | tr ' ' '\n' | grep '^\[')
  echo "$L: trigger=$T brightness=$(cat /sys/class/leds/$L/brightness)"
done
```

**第 4 步：qDebug 取证迭代——5 分钟破案比 1 小时盯代码猜快**

在读写函数里把"路径 + 字节数 + 内容片段"打出来，重编部署跑一遍看日志（改码后 `make -j2` 秒级增量）：

```cpp
const QByteArray raw = f.readAll();
qDebug("sysRead %s -> %d bytes [%s]", qPrintable(path),
       raw.size(), raw.left(60).constData());
```

监视器靠这一招实锤"trigger 文件读到了 296 字节完整内容 `[none] ...`"——读取链路没问题、板上真值就是 none，一次排除半个嫌疑面。验完记得删调试行。

**编程坑两则（procfs / sysfs 特有，魔改前必读）**：

| 坑 | 真因 | 对策 |
|----|------|------|
| `while (!f.atEnd())` 读 `/proc`、`/sys` 文件读到的永远是空 | 这类文件 stat 出来的 size 是 0，**atEnd() 在首次读之前就为真**，循环一次都不执行（监视器 meminfo 读成 0 的真因） | 一律 `readAll()` 拿全量再自行 `split('\n')`；`readLine()` 实测可用，`readAll()` 最稳 |
| 解析 trigger 老拿到第一词 "none" | trigger 文件是一长串列表，**当前值包在方括号里**：`none rfkill-any ... [heartbeat] ...` | 取 `indexOf('[')` 到 `']'` 之间，别取首词 |

**环境陷阱：板子会"悄悄重启"**。板上无 RTC 电池，三个迹象说明重启过而你以为它一直开着：`/tmp` 下的文件消失、`/proc/uptime` 归零、`date` 回到 2000 年。所以**程序里别写死环境假设**——监视器的做法是启动时探测（LED trigger、屏幕分辨率、有无温度/频率文件各是什么就是什么）、退出按启动现场恢复，一轮开机 trigger 是 `[none]`、下一轮是 `[heartbeat]` 都兜得住。

**压测注入：制造负载看响应**。验证 CPU 采样是真是假、FOLLOW 类"负载联动"功能是否生效的最快办法：

```bash
ssh root@192.168.0.8 'yes > /dev/null &'   # 单核跑满，看屏上曲线抬头
ssh root@192.168.0.8 'killall yes'          # 收手
```

**⚠ 实测双核长时间满载后板子出现过自愈重启（根因未查，疑似看门狗相关）——演示十几秒即可，别拿它烤机。**

## 六、排查表（第 10 章三篇 + 番外篇的坑全在这）

| 症状 | 先查什么 |
|------|---------|
| `Could not find the Qt platform plugin "eglfs"` | `-platform linuxfb` 加了没（命令行和 Qt Creator Run 参数**两处都要**） |
| 板上跑报 `cannot open shared object file: libQt5Widgets...` | 编译用的 qmake 不是 buildroot `output/host/bin/` 的——换对重编 |
| 屏幕黑 | `uname -a` 看在哪个内核（自编内核无显示驱动是预期）；出厂内核还黑 = MIPI 排线（实验三十六白屏教训：`manufacturer ID==>0` 说明排线没通，重插即愈） |
| 触摸没反应 | 出厂内核确认过没；`cat /proc/bus/input/devices` 找 Goodix 的 eventX，加 `QT_QPA_GENERIC_PLUGINS=evdevtouch:/dev/input/eventX`；多点没生效 = 代码收的是鼠标事件，改 `QTouchEvent` |
| 画面叠影/花屏 | 上一个 Qt 程序没退——`killall` 掉再开（fb0 被两个进程同时写） |
| 中文显示成方块 | 根上没有 CJK 字体——换英文文案、拷字体上板、或自绘点阵字库（三选一） |
| `sudo cp` 报 Permission denied | sudo 加了没（NFS 根文件归 root） |
| Ubuntu 侧操作根目录后板子全部命令报 `Stale file handle` | 板子正挂着根时删了根目录——只能板子复位/断电；预防 = 覆盖式更新不 rm |
| 程序退出后屏幕留着最后一帧 | `dd if=/dev/zero of=/dev/fb0 bs=1024 count=854` 擦屏（ENOSPC 报错无害） |
| fb0 抓帧出来全黑 | 先 `ps` 确认进程在——在就是抓太早（Qt 初始化 + 首帧要等几秒），睡 6 秒重抓；两次抓帧 `md5sum` 对比看画面有没有在动（见第五节第 1/2 步） |
| 程序读 `/proc`、`/sys` 得到空值/0 | `atEnd()` 陷阱（见第五节编程坑两则）——改用 `readAll()` 后自行分行 |
| 最高分/存档想清零 | 存档文件在程序同目录（如 `/opt/hiscore.txt`），`rm` 掉即可 |

## 七、一分钟最小模板

从零新建一个工程的全部文件——`hello.pro`：

```qmake
QT       += widgets
CONFIG   += c++11
TEMPLATE  = app
TARGET    = hello
SOURCES  += main.cpp
```

`main.cpp`：

```cpp
#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QLabel label(QStringLiteral("Hello FS-MP1A\nQt 5.12 / linuxfb / 480x854"));
    label.setAlignment(Qt::AlignCenter);
    label.setStyleSheet("color: white; background-color: #1a3a6a; font-size: 24px;");
    label.showFullScreen();   // linuxfb 没有窗口管理器，直接全屏
    return app.exec();
}
```

编译部署运行（三连，判据见路线 A）：

```bash
cd <共享目录>/hello
<buildroot源码>/output/host/bin/qmake hello.pro && make -j2
echo 123 | sudo -S cp hello /home/cnu/nfsboot/rfs-buildroot/opt/
ssh root@192.168.0.8 '/opt/hello -platform linuxfb'
```

退出：程序所在终端 `Ctrl+C`；或 `ssh root@192.168.0.8 killall hello`。

## 八、环境出处对照表

| 环境能力 | 建在哪一章 |
|----------|-----------|
| buildroot 根文件系统（含 kmod/openssh/gdbserver） | 实验二十八/二十九（第 6 章） |
| 出厂内核 + 出厂 dtb + NFS 根混搭启动（`fcsys`） | 实验二十三（第 5 章）+ 实验三十六步骤 5 |
| QT5 模块进 buildroot（qt5base：Core/Gui/Widgets + linuxfb） | 实验三十六步骤 1~4 |
| calculator 验收（触摸可点 = evdev 通） | 实验三十六步骤 6 |
| ssh 通道（PermitRootLogin yes） | 实验三十六步骤 7 |
| Qt Creator Kit（交叉编译器/qmake/sysroot/mkspec）+ 一键部署 | 实验三十七步骤 1~5 |
| gdbserver 远程调试配置 | 实验三十六步骤 3 + 实验三十七步骤 6 |
| 完整实战样例（多点触控/自绘字库/动画循环/最高分持久化/暂停菜单） | 番外篇《板上像素马里奥》 |
| 系统级应用样例（/proc 与 sysfs 数据读取/内核 LED trigger/现场保存恢复/命令行参数） | 番外篇之二《板上系统监视器》 |
| 调试方法论三步破案（抓帧时序/UI↔内核对账/qDebug 取证/atEnd 坑/压测注入） | 番外篇之二（监视器全案实测沉淀，见第五节） |

——这套环境的全部边界都在上面了。它在显示、音频、模块上做减法，换来的是：**从改一行代码到板子屏幕见效，只隔三条命令**。
