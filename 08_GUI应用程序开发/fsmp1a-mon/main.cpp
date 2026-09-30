//
// main.cpp —— FS-MP1A 板上系统监视器（番外篇之二）
//
// 纯 QPainter 手绘的仪表盘：把 /proc 与 /sys 里的系统数据画成
// 实时仪表，并把指尖操作通到板载 LED（/sys/class/leds，内核
// LED 子系统）。零图片素材、零音频依赖、单点触摸即可（第一触点
// 会被 Qt 自动合成鼠标事件，所以连 QTouchEvent 都不用——要双指
// 同按的请看番外篇马里奥）。专为 FS-MP1A（STM32MP157A）+ 出厂
// 内核 + buildroot Qt 5.12 + linuxfb 编写。
//
//   编译：buildroot 的 qmake + make（见 fsmp1a-mon.pro 头部注释）
//   运行：/opt/fsmp1a-mon -platform linuxfb
//         可选 --mode=man|flow|blink|disk|follow 直接进入某模式
//   操作：MAN 模式点 USER1/USER2 卡片 = 单独开关对应板载灯；
//         右上角 [X] 退出，退出时自动把每颗灯恢复成启动前的样子
//
// 为什么有些灯"自己会闪"：
//   BLINK/DISK/FOLLOW 三个模式用的是内核 LED trigger（timer /
//   mmc1 / mmc2）——闪灯是内核在闪，本程序睡着了灯照样闪，
//   CPU 占用为零；FLOW 流水灯没有现成 trigger，由本程序定时
//   轮写 brightness。两条路各占一个模式，正好对照体会：
//   这就是 LED trigger 子系统与软件轮写的差别。
//
// 板上实测的数据源（出厂内核 5.4.31，2026-09-30）：
//   /proc/stat、/proc/meminfo、/proc/loadavg、/proc/uptime —— 内核标配
//   /sys/class/thermal/thermal_zone0/temp —— cpu-thermal，毫摄氏度
//   /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq —— ondemand 调频
//   /sys/class/leds/{cpu,user1,user2} —— 驱动 leds-gpio（非零即亮，无调光）
//   mmc host 映射（/sys/block 反查）：SD 卡 = mmc1、eMMC = mmc2、mmc0 空置
//

#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QVector>
#include <QDir>
#include <QFile>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>
#include <csignal>

#include "font5x7.h"

// ---------------- 配色（深底仪表风） ----------------
static const QRgb COL_BG    = 0xFF0E141B;  // 整屏底
static const QRgb COL_PANEL = 0xFF182430;  // 区块面板
static const QRgb COL_GRID  = 0xFF243646;  // 边框/网格/熄灭的灯
static const QRgb COL_AMBER = 0xFFFFB000;  // 琥珀：标题/选中按钮
static const QRgb COL_GREEN = 0xFF3ADF6E;  // 绿：CPU 曲线/正常数值
static const QRgb COL_CYAN  = 0xFF41C8F4;  // 青：次要数据
static const QRgb COL_RED   = 0xFFFF4D5E;  // 红：告警
static const QRgb COL_DIM   = 0xFF7C93A8;  // 暗灰蓝：说明文字
static const QRgb COL_WHITE = 0xFFE9F2FA;  // 亮白
static const QRgb COL_GOLD  = 0xFFFFD54A;  // 灯珠点亮色

// ---------------- 设计画布（板上实测 480x854；其他分辨率居中显示） ----------------
static const int CW = 480;
static const int CH = 854;

// ---------------- sysfs / procfs 读写小工具 ----------------
static QString sysRead(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromLatin1(f.readAll()).simplified();
}

static bool sysWrite(const QString& path, const QString& value)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const QByteArray data = value.toLatin1();
    return f.write(data) == data.size();
}

// trigger 文件内容形如 "none rfkill-any ... [heartbeat] ..."，方括号里是当前值
static QString parseTrigger(const QString& raw)
{
    const int b = raw.indexOf('[');
    const int e = raw.indexOf(']');
    if (b < 0 || e <= b)
        return raw.split(' ').value(0);
    return raw.mid(b + 1, e - b - 1);
}

// ---------------- CPU 占用：/proc/stat 两次采样差分 ----------------
struct CpuTick { long long idle = 0, total = 0; bool ok = false; };

static CpuTick readCpuTick()
{
    CpuTick t;
    QFile f(QStringLiteral("/proc/stat"));
    if (!f.open(QIODevice::ReadOnly))
        return t;
    // 首行："cpu  user nice system idle iowait irq softirq steal ..."
    const QStringList v = QString::fromLatin1(f.readAll())
                              .split(' ', QString::SkipEmptyParts);
    if (v.size() < 5 || v.at(0) != QStringLiteral("cpu"))
        return t;
    long long sum = 0, idle = 0;
    for (int i = 1; i < v.size(); ++i) {
        const long long n = v.at(i).toLongLong();
        sum += n;
        if (i == 4 || i == 5)       // idle + iowait 都算空闲
            idle += n;
    }
    t.idle = idle;
    t.total = sum;
    t.ok = (sum > 0);
    return t;
}

// ---------------- 一次系统采样 ----------------
struct SysStats {
    int cpuPct = 0;
    long long memTotalKB = 0, memAvailKB = 0;
    int uptimeSec = 0;
    int procs = 0;
    double load1 = 0, load5 = 0, load15 = 0;
    int freqMHz = 0;      bool freqOk = false;
    double tempC = 0;     bool tempOk = false;
};

// /proc 文件 stat 出来的 size 是 0，atEnd() 在首次读之前就为真——
// 所以这里一律 readAll() 拿全量内容再自行分行，绝不用 atEnd 循环
static long long meminfoValue(const QString& key)      // 返回 kB
{
    QFile f(QStringLiteral("/proc/meminfo"));
    if (!f.open(QIODevice::ReadOnly))
        return 0;
    const QStringList lines = QString::fromLatin1(f.readAll()).split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        if (lines.at(i).startsWith(key)) {
            const QStringList v = lines.at(i).split(' ', QString::SkipEmptyParts);
            return v.size() > 1 ? v.at(1).toLongLong() : 0;
        }
    }
    return 0;
}

static void sampleStats(const CpuTick& prev, const CpuTick& now, SysStats& st)
{
    if (now.ok && prev.ok && now.total > prev.total) {
        const long long dTotal = now.total - prev.total;
        const long long dIdle  = now.idle  - prev.idle;
        st.cpuPct = qBound(0, int(100 - 100.0 * dIdle / dTotal + 0.5), 100);
    }

    st.memTotalKB = meminfoValue(QStringLiteral("MemTotal:"));
    st.memAvailKB = meminfoValue(QStringLiteral("MemAvailable:"));

    // "/proc/loadavg 一行拿三个指标：1.00 0.81 0.60 2/72 318"
    QFile f(QStringLiteral("/proc/loadavg"));
    if (f.open(QIODevice::ReadOnly)) {
        const QStringList v = QString::fromLatin1(f.readAll())
                                  .split(' ', QString::SkipEmptyParts);
        if (v.size() >= 3) {
            st.load1  = v.at(0).toDouble();
            st.load5  = v.at(1).toDouble();
            st.load15 = v.at(2).toDouble();
        }
        if (v.size() >= 4) {
            const QStringList rp = v.at(3).split('/');
            if (rp.size() == 2)
                st.procs = rp.at(1).toInt();   // "运行/总数" 取总数
        }
    }

    QFile up(QStringLiteral("/proc/uptime"));
    if (up.open(QIODevice::ReadOnly)) {
        const QStringList v = QString::fromLatin1(up.readAll())
                                  .split(' ', QString::SkipEmptyParts);
        if (!v.isEmpty())
            st.uptimeSec = int(v.at(0).toDouble());
    }

    const QString freq = sysRead(QStringLiteral(
        "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq"));
    st.freqOk = !freq.isEmpty();
    if (st.freqOk)
        st.freqMHz = freq.toInt() / 1000;      // kHz → MHz

    const QString temp = sysRead(QStringLiteral(
        "/sys/class/thermal/thermal_zone0/temp"));
    st.tempOk = !temp.isEmpty();
    if (st.tempOk)
        st.tempC = temp.toDouble() / 1000.0;   // 毫摄氏度 → 摄氏度
}

// ---------------- LED：一颗板载灯的抽象 ----------------
struct Led {
    QString name;             // cpu / user1 / user2
    QString dir;              // /sys/class/leds/<name>
    bool present = false;
    QString origTrigger;      // 启动现场（退出时恢复）
    int origBrightness = 0;
    bool lit = false;         // 此刻亮吗（brightness 镜像，仅 none 触发器下可靠）
    QString trigger = QStringLiteral("none");

    bool setBrightness(int v)
    {
        const bool ok = sysWrite(dir + QStringLiteral("/brightness"),
                                 QString::number(v));
        if (ok) {
            lit = (v > 0);
            trigger = QStringLiteral("none");
        }
        return ok;
    }
    bool setTrigger(const QString& t)
    {
        const bool ok = sysWrite(dir + QStringLiteral("/trigger"), t);
        if (ok) {
            trigger = t;
            lit = false;      // 交给内核摆布后，镜像不再代表"亮"
        }
        return ok;
    }
    bool setPeriod(int onMs, int offMs)   // 仅 timer 触发器有效
    {
        bool ok = sysWrite(dir + QStringLiteral("/delay_on"),
                           QString::number(onMs));
        ok = sysWrite(dir + QStringLiteral("/delay_off"),
                      QString::number(offMs)) && ok;
        return ok;
    }
};

static QVector<Led> scanLeds()
{
    QVector<Led> out;
    QDir dir(QStringLiteral("/sys/class/leds"));
    if (!dir.exists())
        return out;
    const QStringList names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < names.size(); ++i) {
        Led led;
        led.name = names.at(i);
        led.dir = dir.absoluteFilePath(names.at(i));
        led.present = true;
        led.origTrigger = parseTrigger(
            sysRead(led.dir + QStringLiteral("/trigger")));
        led.origBrightness = sysRead(
            led.dir + QStringLiteral("/brightness")).toInt();
        led.trigger = led.origTrigger;
        led.lit = (led.origBrightness > 0);
        out.append(led);
    }
    return out;   // 字母序：cpu, user1, user2 正好是想要的顺序
}

// ---------------- 模式 ----------------
enum Mode { MODE_MAN, MODE_FLOW, MODE_BLINK, MODE_DISK, MODE_FOLLOW, MODE_COUNT };
static const char* const MODE_NAME[MODE_COUNT] = {
    "MAN", "FLOW", "BLINK", "DISK", "FOLLOW"
};
static const char* const MODE_HINT[MODE_COUNT] = {
    "TAP A CARD TO SWITCH ITS LED",     // MAN
    "3 LEDS ROTATE - CPU JOINS FLOW",   // FLOW
    "KERNEL FLASHES - CPU COST 0",      // BLINK
    "RUN DD ON BOARD - WATCH THEM",     // DISK
    "BUSY=FASTER  >70C=ALERT ON",       // FOLLOW
};
static const int SPEED_TICK_MS[3] = { 600, 300, 200 };   // FLOW 轮换间隔
static const int SPEED_ON_MS[3]   = { 1000, 450, 150 };  // BLINK 亮时长
static const int SPEED_OFF_MS[3]  = { 1000, 450, 150 };  // BLINK 灭时长
static const char  SPEED_CHAR[4]  = "SMF";
static const int    FOLLOW_MAX_MS = 850;   // 负载为零时的闪周期
static const int    FOLLOW_MIN_MS = 120;   // 满载时的闪周期
static const double TEMP_ALERT_C  = 70.0;  // 温度告警阈值（摄氏度）

// ---------------- 主窗口 ----------------
class MonWidget : public QWidget
{
public:
    explicit MonWidget(int initialMode, QWidget* parent = 0)
        : QWidget(parent)
    {
        setWindowTitle(QStringLiteral("FS-MP1A Monitor"));
        leds = scanLeds();
        prevTick = readCpuTick();
        mode = Mode(initialMode);
        applyMode(mode);                       // 进场就把灯接管起来
        connect(&timer, &QTimer::timeout, this, [this] { tick(); });
        timer.start(100);
    }
    ~MonWidget() { restoreAll(); }

    // 把每颗灯恢复成程序启动前的样子——"合格的设备管理员"
    void restoreAll()
    {
        for (int i = 0; i < leds.size(); ++i) {
            Led& led = leds[i];
            if (!led.present)
                continue;
            sysWrite(led.dir + QStringLiteral("/trigger"), led.origTrigger);
            sysWrite(led.dir + QStringLiteral("/brightness"),
                     QString::number(led.origBrightness));
            led.trigger = led.origTrigger;
            led.lit = (led.origBrightness > 0);
        }
        alertActive = false;
    }

protected:
    // 单点触摸已被 Qt 合成鼠标事件，收鼠标就够了
    void mousePressEvent(QMouseEvent* e) override
    {
        const int offX = (width() - CW) / 2;
        const int offY = (height() - CH) / 2;
        onPress(QPointF(e->localPos().x() - offX, e->localPos().y() - offY));
    }

    void keyPressEvent(QKeyEvent* e) override
    {
        if (e->key() == Qt::Key_Escape) {      // 板上没键盘，ssh 调试备用
            restoreAll();
            close();
        }
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.fillRect(rect(), QColor(0xFF000000));          // 屏比画布宽/窄时的边
        p.translate((width() - CW) / 2, (height() - CH) / 2);
        p.setRenderHint(QPainter::Antialiasing, true);   // 给 LED 圆珠用
        render(p);
    }

private:
    // ---------- 布局（设计坐标 480x854） ----------
    static QRect modeBtnRect(int i)  { return QRect(24 + i * 90, 518, 86, 40); }
    static QRect speedBtnRect(int i) { return QRect(24 + i * 68, 566, 60, 40); }
    static QRect userCardRect(int i) { return QRect(24 + i * 226, 614, 206, 108); }
    static QRect exitBtnRect()       { return QRect(CW - 58, 10, 46, 38); }

    // ---------- 交互 ----------
    void onPress(const QPointF& scene)
    {
        const QPoint pt = scene.toPoint();
        if (exitBtnRect().contains(pt)) {
            restoreAll();
            close();
            return;
        }
        for (int i = 0; i < MODE_COUNT; ++i)
            if (modeBtnRect(i).contains(pt)) { applyMode(Mode(i)); return; }
        for (int i = 0; i < 3; ++i)
            if (speedBtnRect(i).contains(pt)) { setSpeed(i); return; }
        if (mode == MODE_MAN)
            for (int i = 0; i < 2; ++i)
                if (userCardRect(i).contains(pt)) { toggleUser(i); return; }
    }

    void toggleUser(int i)
    {
        Led* led = ledByName(i == 0 ? "user1" : "user2");
        if (!led || !led->present)
            return;
        if (led->trigger != QStringLiteral("none"))
            led->setTrigger(QStringLiteral("none"));
        userOn[i] = !userOn[i];
        led->setBrightness(userOn[i] ? 255 : 0);
        update();
    }

    // ---------- 模式 ----------
    void applyMode(Mode m)
    {
        restoreAll();              // 先把三颗灯还原成启动现场，再接管
        mode = m;
        sub = 0;
        flowIdx = 0;
        userOn[0] = userOn[1] = false;

        Led* u1 = ledByName("user1");
        Led* u2 = ledByName("user2");
        switch (m) {
        case MODE_MAN:
            break;                 // 亮度全凭指尖
        case MODE_FLOW: {
            // 流水灯没有现成 trigger，软件轮写；请 cpu 灯临时离开心跳
            Led* c = ledByName("cpu");
            if (c && c->present)
                c->setTrigger(QStringLiteral("none"));
            flowStep();
            break;
        }
        case MODE_BLINK:
            // 内核 timer 触发器闪灯：本程序睡了灯照样闪
            if (u1) u1->setTrigger(QStringLiteral("timer"));
            if (u2) u2->setTrigger(QStringLiteral("timer"));
            applyBlinkPeriod();
            break;
        case MODE_DISK:
            // 磁盘活动灯：SD 卡 = mmc1，eMMC = mmc2（板上 /sys/block 反查实测）
            if (u1) u1->setTrigger(QStringLiteral("mmc1"));
            if (u2) u2->setTrigger(QStringLiteral("mmc2"));
            break;
        case MODE_FOLLOW:
            if (u1) u1->setTrigger(QStringLiteral("timer"));
            if (u2) u2->setTrigger(QStringLiteral("timer"));
            followStep();
            break;
        default:
            break;
        }
        update();
    }

    void setSpeed(int i)
    {
        speedIdx = i;
        if (mode == MODE_BLINK)
            applyBlinkPeriod();
        update();
    }

    void applyBlinkPeriod()
    {
        for (int i = 0; i < 2; ++i) {
            Led* led = ledByName(i == 0 ? "user1" : "user2");
            if (led && led->present)
                led->setPeriod(SPEED_ON_MS[speedIdx], SPEED_OFF_MS[speedIdx]);
        }
    }

    // FLOW：按 user1 → user2 → cpu 的顺序轮播
    void flowStep()
    {
        static const char* const order[3] = { "user1", "user2", "cpu" };
        for (int k = 0; k < 3; ++k) {
            Led* led = ledByName(order[k]);
            if (led && led->present)
                led->setBrightness(0);
        }
        Led* cur = ledByName(order[flowIdx % 3]);
        if (cur && cur->present)
            cur->setBrightness(255);
        ++flowIdx;
    }

    // FOLLOW：负载越高闪得越急；温度超阈值转常亮告警
    void followStep()
    {
        // 即时占用率主导（1 秒差分，响应快），1 分钟负载兜底（双核 load 2.0 = 满载）
        const double busy = qMax(st.cpuPct / 100.0,
                                 qMin(1.0, st.load1 / 2.0));
        const int period = FOLLOW_MAX_MS
                         - int((FOLLOW_MAX_MS - FOLLOW_MIN_MS) * busy);
        alertActive = false;
        for (int i = 0; i < 2; ++i) {
            Led* led = ledByName(i == 0 ? "user1" : "user2");
            if (!led || !led->present)
                continue;
            if (st.tempOk && st.tempC >= TEMP_ALERT_C) {
                if (led->trigger != QStringLiteral("none"))
                    led->setTrigger(QStringLiteral("none"));
                led->setBrightness(255);
                alertActive = true;
            } else {
                if (led->trigger != QStringLiteral("timer"))
                    led->setTrigger(QStringLiteral("timer"));
                led->setPeriod(period, period);
            }
        }
    }

    // ---------- 主循环：100ms 一拍，数据每秒采一次 ----------
    void tick()
    {
        ++sub;
        if (sub % 10 == 0) {
            const CpuTick now = readCpuTick();
            sampleStats(prevTick, now, st);
            prevTick = now;
            hist.append(st.cpuPct);
            if (hist.size() > CURVE_N)
                hist.removeFirst();
            if (st.cpuPct > peakCpu)
                peakCpu = st.cpuPct;
            if (mode == MODE_FOLLOW)
                followStep();
            update();
        }
        if (mode == MODE_FLOW
                && sub % (SPEED_TICK_MS[speedIdx] / 100) == 0) {
            flowStep();
            update();
        }
    }

    // ---------- 绘制 ----------
    void render(QPainter& p)
    {
        // 标题栏
        p.fillRect(QRect(0, 0, CW, 56), QColor(COL_PANEL));
        drawTextPx(p, 16, 20, QStringLiteral("FS-MP1A MONITOR"), 3, COL_AMBER);
        p.fillRect(exitBtnRect(), QColor(COL_BG));
        p.setPen(QColor(COL_RED));
        p.drawRect(exitBtnRect().adjusted(0, 0, -1, -1));
        drawGlyph(p, 'X', exitBtnRect().center().x() - 10,
                  exitBtnRect().center().y() - 14, 4, COL_RED);

        // 信息行
        const int up = st.uptimeSec;
        const QString upStr = QStringLiteral("%1:%2:%3")
                .arg(up / 3600, 2, 10, QChar('0'))
                .arg((up / 60) % 60, 2, 10, QChar('0'))
                .arg(up % 60, 2, 10, QChar('0'));
        drawTextPx(p, 16, 64, QStringLiteral("UP ") + upStr, 3, COL_WHITE);
        const QString procStr = QStringLiteral("PROCS %1").arg(st.procs);
        drawTextPx(p, 456 - textW(procStr, 3), 64, procStr, 3, COL_CYAN);

        // CPU 区
        drawPanel(p, QRect(10, 92, 460, 224),
                  alertActive ? COL_RED : COL_GRID);
        drawTextPx(p, 24, 100, QStringLiteral("CPU"), 2, COL_DIM);
        drawTextPx(p, 24, 122, QStringLiteral("%1%").arg(st.cpuPct), 7, COL_GREEN);
        const QString freqStr = st.freqOk
                ? QStringLiteral("%1 MHZ").arg(st.freqMHz)
                : QStringLiteral("-- MHZ");
        drawTextPx(p, 24, 186, freqStr, 3, COL_CYAN);
        const bool hot = st.tempOk && st.tempC >= TEMP_ALERT_C;
        const QString tempStr = st.tempOk
                ? QStringLiteral("%1 C").arg(st.tempC, 0, 'f', 1)
                : QStringLiteral("--.- C");
        drawTextPx(p, 24, 226, tempStr, 3, hot ? COL_RED : COL_CYAN);
        drawTextPx(p, 24, 264, QStringLiteral("PEAK %1%").arg(peakCpu), 2, COL_DIM);

        const QRect curve(222, 100, 248, 200);
        p.fillRect(curve, QColor(COL_BG));
        p.setPen(QColor(COL_GRID));
        p.setBrush(Qt::NoBrush);
        p.drawRect(curve.adjusted(0, 0, -1, -1));
        for (int g = 1; g <= 3; ++g) {           // 25/50/75% 三条网格线
            const int y = 296 - 184 * g / 4;
            p.drawLine(224, y, 468, y);
        }
        for (int i = 0; i < hist.size(); ++i) {  // 每样本一根 1px 柱，2px 步距
            const int x = 226 + i * 2;
            const int h = qMax(1, hist.at(i) * 184 / 100);
            p.fillRect(x, 296 - h, 1, h, QColor(COL_GREEN));
        }

        // MEMORY 区
        drawPanel(p, QRect(10, 316, 460, 118));
        drawTextPx(p, 24, 324, QStringLiteral("MEMORY"), 3, COL_AMBER);
        const long long usedMB = (st.memTotalKB - st.memAvailKB) / 1024;
        const long long freeMB = st.memAvailKB / 1024;
        const int memPct = st.memTotalKB > 0
                ? int((st.memTotalKB - st.memAvailKB) * 100 / st.memTotalKB) : 0;
        const QString memStr = QStringLiteral("%1/%2 MB")
                .arg(usedMB).arg(st.memTotalKB / 1024);
        drawTextPx(p, 456 - textW(memStr, 3), 324, memStr, 3, COL_WHITE);
        p.fillRect(QRect(24, 360, 432, 30), QColor(COL_GRID));
        p.fillRect(QRect(24, 360, 432 * memPct / 100, 30),
                   QColor(memPct > 85 ? COL_RED : COL_GREEN));
        p.setPen(QColor(COL_WHITE));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRect(24, 360, 432, 30));
        drawTextPx(p, 24, 402,
                   QStringLiteral("USED %1 MB   FREE %2 MB").arg(usedMB).arg(freeMB),
                   2, COL_DIM);

        // LOAD AVG 行
        drawTextPx(p, 24, 452, QStringLiteral("LOAD AVG"), 2, COL_DIM);
        drawTextPx(p, 140, 448,
                   QStringLiteral("%1 %2 %3").arg(st.load1, 0, 'f', 2)
                           .arg(st.load5, 0, 'f', 2).arg(st.load15, 0, 'f', 2),
                   3, COL_WHITE);

        // LED 区
        drawPanel(p, QRect(10, 478, 460, 344));
        drawTextPx(p, 24, 486, QStringLiteral("LED CONTROL"), 3, COL_AMBER);
        for (int i = 0; i < MODE_COUNT; ++i)
            drawBtn(p, modeBtnRect(i), QLatin1String(MODE_NAME[i]), i == mode, 2);
        for (int i = 0; i < 3; ++i)
            drawBtn(p, speedBtnRect(i), QString(QChar(SPEED_CHAR[i])),
                    i == speedIdx, 2);
        drawTextPx(p, 240, 578, QStringLiteral("SPEED"), 2, COL_DIM);

        for (int i = 0; i < 2; ++i) {            // 两张灯卡片
            Led* led = ledByName(i == 0 ? "user1" : "user2");
            const QRect r = userCardRect(i);
            p.fillRect(r, QColor(COL_BG));
            p.setPen(QColor(mode == MODE_MAN ? COL_AMBER : COL_GRID));
            p.setBrush(Qt::NoBrush);
            p.drawRect(r.adjusted(0, 0, -1, -1));

            const bool on = led && led->present
                    ? (led->trigger == QStringLiteral("none") ? led->lit : true)
                    : false;
            const QPointF c(r.x() + 68, r.y() + 54);
            if (on) {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(255, 213, 74, 70));      // 光晕
                p.drawEllipse(c, 30, 30);
                p.setBrush(QColor(COL_GOLD));
                p.drawEllipse(c, 20, 20);
            } else {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(COL_GRID));
                p.drawEllipse(c, 20, 20);
            }
            p.setBrush(Qt::NoBrush);
            p.setPen(QColor(COL_GRID));
            p.drawEllipse(c, 20, 20);

            drawTextPx(p, r.x() + 104, r.y() + 26,
                       i == 0 ? QStringLiteral("USER1") : QStringLiteral("USER2"),
                       3, COL_WHITE);
            QString sstat;
            if (!led || !led->present)
                sstat = QStringLiteral("N/A");
            else if (led->trigger == QStringLiteral("none"))
                sstat = led->lit ? QStringLiteral("ON") : QStringLiteral("OFF");
            else
                sstat = led->trigger.toUpper();   // TIMER / MMC1 / MMC2
            drawTextPx(p, r.x() + 104, r.y() + 64, sstat, 3,
                       on ? COL_GOLD : COL_DIM);
        }

        // cpu 灯状态行（心跳灯只展示，不开关）
        Led* cpu = ledByName("cpu");
        const QPointF mc(34, 740);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(cpu ? COL_GOLD : COL_GRID));
        p.drawEllipse(mc, 8, 8);
        const QString cpuTrig = cpu ? cpu->trigger.toUpper()
                                    : QStringLiteral("N/A");
        drawTextPx(p, 52, 733, QStringLiteral("CPU LED: %1 (SYS)").arg(cpuTrig),
                   2, COL_DIM);

        // 当前模式说明行
        QString hint = QStringLiteral("> ");
        hint += alertActive ? QStringLiteral("TEMP ALERT - USER LEDS ON")
                            : QLatin1String(MODE_HINT[mode]);
        drawTextPx(p, 24, 764, hint, 2, alertActive ? COL_RED : COL_DIM);

        // 页脚
        drawTextPx(p, 24, 830,
                   QStringLiteral("FSMP1A-MON V1.0 - QT 5.12 - LINUXFB"),
                   2, COL_DIM);
    }

    // ---------- 小工具 ----------
    static void drawPanel(QPainter& p, const QRect& r, QRgb border = COL_GRID)
    {
        p.fillRect(r, QColor(COL_PANEL));
        p.setPen(QColor(border));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(0, 0, -1, -1));
    }

    static void drawBtn(QPainter& p, const QRect& r, const QString& label,
                        bool active, int scale)
    {
        if (active) {
            p.fillRect(r, QColor(COL_AMBER));
            drawTextPx(p, r.x() + (r.width() - textW(label, scale)) / 2,
                       r.y() + (r.height() - 7 * scale) / 2, label, scale, COL_BG);
        } else {
            p.fillRect(r, QColor(COL_BG));
            p.setPen(QColor(COL_GRID));
            p.setBrush(Qt::NoBrush);
            p.drawRect(r.adjusted(0, 0, -1, -1));
            drawTextPx(p, r.x() + (r.width() - textW(label, scale)) / 2,
                       r.y() + (r.height() - 7 * scale) / 2, label, scale, COL_WHITE);
        }
    }

    Led* ledByName(const char* n)
    {
        for (int i = 0; i < leds.size(); ++i)
            if (leds.at(i).name == QLatin1String(n))
                return &leds[i];
        return 0;
    }

private:
    static const int CURVE_N = 120;      // 120 点 x 1 秒 = 2 分钟 CPU 历史
    QVector<Led> leds;
    SysStats st;
    CpuTick prevTick;
    QVector<int> hist;
    int peakCpu = 0;
    Mode mode;
    int speedIdx = 1;
    bool userOn[2];
    bool alertActive = false;
    QTimer timer;
    int sub = 0;
    int flowIdx = 0;
};

// ---------------- main ----------------
static void onTermSig(int)
{
    // killall 发 SIGTERM；转回事件循环正常退出，"恢复现场"才有路可走
    qApp->quit();
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    signal(SIGTERM, onTermSig);
    signal(SIGINT, onTermSig);

    int initialMode = MODE_MAN;
    for (int i = 1; i < argc; ++i) {
        const QString a = QString::fromLocal8Bit(argv[i]);
        if (a.startsWith(QStringLiteral("--mode="))) {
            const QString m = a.mid(7).toUpper();
            for (int k = 0; k < MODE_COUNT; ++k)
                if (m == QLatin1String(MODE_NAME[k]))
                    initialMode = k;
        }
    }

    MonWidget w(initialMode);
    w.showFullScreen();

    const QRect scr = QGuiApplication::primaryScreen()->availableGeometry();
    qDebug("screen %dx%d canvas %dx%d mode %d",
           scr.width(), scr.height(), CW, CH, initialMode);

    const int rc = app.exec();
    w.restoreAll();     // 与析构双保险
    return rc;
}
