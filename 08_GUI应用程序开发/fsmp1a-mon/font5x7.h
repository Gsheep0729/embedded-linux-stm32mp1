#ifndef FONT5X7_H
#define FONT5X7_H
//
// font5x7.h —— 本作的全部"美术"：5x7 点阵字库
//
// 板子的根文件系统里没有可用的界面字体（只有西文 DejaVu），
// 所以和番外篇《板上像素马里奥》一样自带点阵字库：一个字形
// 是 7 个字符串、每个字符一笔，'X' = 亮点，'.' = 熄灭。
// 整数倍放大渲染，像素风且放大不糊，零 QFont 依赖。
// （字库本体搬自番外篇 pixelart.h，监视器另补了 % / ( ) > 五个字形。）
//

#include <QPainter>
#include <QString>
#include <QRgb>

static const char* const FONT_AZ[26][7] = {
    {".XXX.","X...X","X...X","XXXXX","X...X","X...X","X...X"}, // A
    {"XXXX.","X...X","X...X","XXXX.","X...X","X...X","XXXX."}, // B
    {".XXX.","X...X","X....","X....","X....","X...X",".XXX."}, // C
    {"XXXX.","X...X","X...X","X...X","X...X","X...X","XXXX."}, // D
    {"XXXXX","X....","X....","XXXX.","X....","X....","XXXXX"}, // E
    {"XXXXX","X....","X....","XXXX.","X....","X....","X...."}, // F
    {".XXX.","X...X","X....","X.XXX","X...X","X...X",".XXXX"}, // G
    {"X...X","X...X","X...X","XXXXX","X...X","X...X","X...X"}, // H
    {"XXXXX","..X..","..X..","..X..","..X..","..X..","XXXXX"}, // I
    {"....X","....X","....X","....X","X...X","X...X",".XXX."}, // J
    {"X...X","X..X.","X.X..","XX...","X.X..","X..X.","X...X"}, // K
    {"X....","X....","X....","X....","X....","X....","XXXXX"}, // L
    {"X...X","XX.XX","X.X.X","X.X.X","X...X","X...X","X...X"}, // M
    {"X...X","XX..X","X.X.X","X..XX","X...X","X...X","X...X"}, // N
    {".XXX.","X...X","X...X","X...X","X...X","X...X",".XXX."}, // O
    {"XXXX.","X...X","X...X","XXXX.","X....","X....","X...."}, // P
    {".XXX.","X...X","X...X","X...X","X.X.X","X..X.",".XX.X"}, // Q
    {"XXXX.","X...X","X...X","XXXX.","X.X..","X..X.","X...X"}, // R
    {".XXXX","X....","X....",".XXX.","....X","....X","XXXX."}, // S
    {"XXXXX","..X..","..X..","..X..","..X..","..X..","..X.."}, // T
    {"X...X","X...X","X...X","X...X","X...X","X...X",".XXX."}, // U
    {"X...X","X...X","X...X","X...X","X...X",".X.X.","..X.."}, // V
    {"X...X","X...X","X...X","X.X.X","X.X.X","XX.XX","X...X"}, // W
    {"X...X","X...X",".X.X.","..X..",".X.X.","X...X","X...X"}, // X
    {"X...X","X...X",".X.X.","..X..","..X..","..X..","..X.."}, // Y
    {"XXXXX","....X","...X.","..X..",".X...","X....","XXXXX"}, // Z
};

static const char* const FONT_09[10][7] = {
    {".XXX.","X...X","X..XX","X.X.X","XX..X","X...X",".XXX."}, // 0
    {"..X..",".XX..","..X..","..X..","..X..","..X..","XXXXX"}, // 1
    {".XXX.","X...X","....X","...X.","..X..",".X...","XXXXX"}, // 2
    {".XXX.","X...X","....X","..XX.","....X","X...X",".XXX."}, // 3
    {"...X.","..XX.",".X.X.","X..X.","XXXXX","...X.","...X."}, // 4
    {"XXXXX","X....","XXXX.","....X","....X","X...X",".XXX."}, // 5
    {".XXX.","X....","X....","XXXX.","X...X","X...X",".XXX."}, // 6
    {"XXXXX","....X","...X.","..X..",".X...",".X...",".X..."}, // 7
    {".XXX.","X...X","X...X",".XXX.","X...X","X...X",".XXX."}, // 8
    {".XXX.","X...X","X...X",".XXXX","....X","....X",".XXX."}, // 9
};

inline const char* const* glyphFor(char c)
{
    if (c >= 'A' && c <= 'Z') return FONT_AZ[c - 'A'];
    if (c >= '0' && c <= '9') return FONT_09[c - '0'];
    switch (c) {
    case '!': { static const char* const g[7] = {"..X..","..X..","..X..","..X..","..X..",".....","..X.."}; return g; }
    case '%': { static const char* const g[7] = {"XX..X","XX.X.","...X.","..X..",".X...","X.XX.","X..XX"}; return g; }
    case '/': { static const char* const g[7] = {"....X","...X.","...X.","..X..",".X...","X....","X...."}; return g; }
    case '(': { static const char* const g[7] = {"..XX.",".X...","X....","X....","X....",".X...","..XX."} ; return g; }
    case ')': { static const char* const g[7] = {".XX..","...X.","....X","....X","....X","...X.",".XX.."} ; return g; }
    case '>': { static const char* const g[7] = {"X....",".X...","..X..","...X.","..X..",".X...","X...."}; return g; }
    case '-': { static const char* const g[7] = {".....",".....",".....","XXXXX",".....",".....","....."}; return g; }
    case '.': { static const char* const g[7] = {".....",".....",".....",".....",".....",".XX..",".XX.."}; return g; }
    case ':': { static const char* const g[7] = {".....",".XX..",".XX..",".....",".XX..",".XX..","....."}; return g; }
    default: return 0;
    }
}

// 画一个字符（scale = 放大倍数，整数倍保持像素风）
inline void drawGlyph(QPainter& p, char c, int x, int y, int scale, QRgb color)
{
    const char* const* g = glyphFor(c);
    if (!g)
        return;
    for (int r = 0; r < 7; ++r)
        for (int col = 0; col < 5; ++col)
            if (g[r][col] == 'X')
                p.fillRect(x + col * scale, y + r * scale, scale, scale, QColor(color));
}

// 画一行文字；shadow 非 0 时先画一层偏移的影子，保证在花哨背景上也读得清
inline void drawTextPx(QPainter& p, int x, int y, const QString& s, int scale,
                       QRgb color, QRgb shadow = 0)
{
    int cx = x;
    for (int i = 0; i < s.length(); ++i) {
        const QChar qc = s.at(i);
        const char c = qc.toLatin1();
        if (shadow)
            drawGlyph(p, c, cx + scale, y + scale, scale, shadow);
        drawGlyph(p, c, cx, y, scale, color);
        cx += 6 * scale;
    }
}

inline int textW(const QString& s, int scale)
{
    return qMax(0, s.length() * 6 - 1) * scale;
}

#endif // FONT5X7_H
