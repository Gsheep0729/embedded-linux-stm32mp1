#ifndef PIXELART_H
#define PIXELART_H
//
// pixelart.h —— 本作的全部"美术"：像素字符画 + 5x7 点阵字库
//
// 规则：每个字符画是一个字符串数组，一个字符 = 一个像素；
//       '.' 或空格 = 透明，其余字符查 artColor() 上色。
// 想改外观：直接改下面的字符画再 make，不需要任何图片编辑器。
// 同一套云朵字符画换一张调色板就是树丛（任天堂当年也是这么省事的）。
//

#include <QImage>
#include <QPainter>
#include <QHash>
#include <QRgb>
#include <string.h>

// ---------------- 调色板 ----------------
inline QRgb artColor(char c)
{
    switch (c) {
    case 'R': return 0xFFE03028; // 红：帽子/上衣/旗子
    case 'U': return 0xFF2038EC; // 蓝：背带裤
    case 'S': return 0xFFF8B878; // 肤色
    case 'B': return 0xFF7C3800; // 深棕：头发/鞋/板栗仔脚
    case 'K': return 0xFF000000; // 黑：轮廓/眼睛/砖缝
    case 'W': return 0xFFFCFCFC; // 白
    case 'Y': return 0xFFFAC000; // 金：?块/金币
    case 'O': return 0xFFC84C0C; // 砖橙
    case 'E': return 0xFFE45C10; // 地面橙（比砖亮一档）
    case 'A': return 0xFF885818; // 深棕：顶过的?块
    case 'N': return 0xFFA04000; // 栗色：板栗仔身体
    case 'C': return 0xFFFCE0A8; // 米色：板栗仔脸
    case 'G': return 0xFF00A800; // 绿：树丛/山
    case 'P': return 0xFF00A800; // 绿：水管
    case 'L': return 0xFFB8F818; // 亮绿：水管高光/山头亮点
    case 'M': return 0xFF007000; // 暗绿：树丛阴影
    case 'c': return 0xFFB8D8F8; // 云底淡蓝
    default:  return 0x00000000; // 透明
    }
}

// ---------------- 字符画 → QImage ----------------
// remap：换色表（旧字符 → 新字符），树丛复用云朵字符画时用
template <int N>
inline QImage buildSprite(const char* const (&rows)[N], bool flip = false,
                          const QHash<char, char>& remap = QHash<char, char>())
{
    const int w = static_cast<int>(strlen(rows[0]));
    QImage img(w, static_cast<int>(N), QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < static_cast<int>(N); ++y) {
        const char* r = rows[y];
        for (int x = 0; x < w; ++x) {
            char c = r[x];
            if (remap.contains(c))
                c = remap.value(c);
            img.setPixel(x, y, (c == '.' || c == ' ') ? 0x00000000 : artColor(c));
        }
    }
    return flip ? img.mirrored(true, false) : img;
}

// ---------------- 马里奥（12x16，朝右） ----------------
static const char* const MARIO_IDLE[] = {
    "....RRRRR...",
    "..RRRRRRRRR.",
    "..BBBSSSB...",
    ".BSSSSSKSS..",
    ".BSSSSSSSSS.",
    "..SSSSKKKK..",
    "...SSSSSS...",
    "..RRRRRR....",
    ".RRRRRRRR...",
    ".RRUYUUYR...",
    ".SSUUUUUUS..",
    "..UUUUUU....",
    "..UUU.UUU...",
    "..UUU.UUU...",
    ".BBB...BBB..",
    ".BBB...BBB..",
};

static const char* const MARIO_RUN1[] = {
    "....RRRRR...",
    "..RRRRRRRRR.",
    "..BBBSSSB...",
    ".BSSSSSKSS..",
    ".BSSSSSSSSS.",
    "..SSSSKKKK..",
    "...SSSSSS...",
    "..RRRRRR....",
    ".RRRRRRRR...",
    ".RRUYUUYR...",
    ".SSUUUUUU...",
    "..UUUUUUU...",
    ".UUU...UUU..",
    "BBBB....BBB.",
    "BBB......BB.",
    "............",
};

static const char* const MARIO_RUN2[] = {
    "....RRRRR...",
    "..RRRRRRRRR.",
    "..BBBSSSB...",
    ".BSSSSSKSS..",
    ".BSSSSSSSSS.",
    "..SSSSKKKK..",
    "...SSSSSS...",
    "..RRRRRR....",
    ".RRRRRRRR...",
    ".RRUYUUYR...",
    ".SSUUUUUUS..",
    "...UUUUU....",
    "...UUUU.....",
    "..BBBB......",
    "..BBBBB.....",
    "............",
};

static const char* const MARIO_JUMP[] = {
    "....RRRRR.SS",
    "..RRRRRRRRSS",
    "..BBBSSSB...",
    ".BSSSSSKSS..",
    ".BSSSSSSSSS.",
    "..SSSSKKKK..",
    "...SSSSSS...",
    "..RRRRRR....",
    ".RRRRRRRR...",
    ".RRUYUUYR...",
    ".SSUUUUUU...",
    "..UUUUUU....",
    ".UUUU..UUU..",
    ".UUU....UUU.",
    "BBBB....BBBB",
    "............",
};

static const char* const MARIO_DIE[] = {
    "....RRRRR...",
    "..RRRRRRRRR.",
    "..BBBSSSB...",
    ".BSSKSKSSSS.",
    ".BSSSSSSSSS.",
    "..SKKKKKKKK.",
    "...SSSSSS...",
    "RRRRRRRRRRR.",
    "RRRRRRRRRRR.",
    ".RRUYUUYR...",
    ".SSUUUUUUS..",
    "..UUUUUU....",
    "..UUU.UUU...",
    "..UUU.UUU...",
    ".BBB...BBB..",
    ".BBB...BBB..",
};

// ---------------- 板栗仔（12x12，两种脚步 + 踩扁） ----------------
static const char* const GOOMBA1[] = {
    "....NNNN....",
    "..NNNNNNNN..",
    ".NNNNNNNNNN.",
    ".NNCCCCCCNN.",
    ".NCKWCCWKCN.",
    ".NCWWCCWWCN.",
    "NNCCCCCCCCNN",
    "NNNNNNNNNNNN",
    ".NNNNNNNNNN.",
    ".CCCC..CCCC.",
    ".BBBB..BBBB.",
    "BBBB....BBBB",
};

static const char* const GOOMBA2[] = {
    "....NNNN....",
    "..NNNNNNNN..",
    ".NNNNNNNNNN.",
    ".NNCCCCCCNN.",
    ".NCKWCCWKCN.",
    ".NCWWCCWWCN.",
    "NNCCCCCCCCNN",
    "NNNNNNNNNNNN",
    ".NNNNNNNNNN.",
    "..CCCCCCCC..",
    "..BBBBBB....",
    ".BBBB..BBB..",
};

static const char* const GOOMBA_FLAT[] = {
    "............",
    "..NNNNNNNN..",
    ".NNNNNNNNNN.",
    "NNKCCCCCCKNN",
    "NNNNNNNNNNNN",
    "KKKK....KKKK",
};

// ---------------- 金币（8x12，四帧旋转） ----------------
static const char* const COIN_F0[] = {
    "..YYYY..",
    ".YYYYYY.",
    ".YWYYYY.",
    "YYWYYYYY",
    "YYWYYYYY",
    "YYWYYYYY",
    "YYWYYYYY",
    "YYWYYYYY",
    "YYWYYYYY",
    ".YWYYYY.",
    ".YYYYYY.",
    "..YYYY..",
};

static const char* const COIN_F1[] = {
    "...YY...",
    "..YYYY..",
    "..YWWY..",
    "..YWWY..",
    "..YWWY..",
    "..YWWY..",
    "..YWWY..",
    "..YWWY..",
    "..YWWY..",
    "..YWWY..",
    "..YYYY..",
    "...YY...",
};

static const char* const COIN_F2[] = {
    "...WW...",
    "...WW...",
    "...WW...",
    "...WW...",
    "...WW...",
    "...WW...",
    "...WW...",
    "...WW...",
    "...WW...",
    "...WW...",
    "...WW...",
    "...WW...",
};

// ---------------- 方块（16x16） ----------------
static const char* const QBLOCK[] = {
    "KKKKKKKKKKKKKKKK",
    "KWWYYYYYYYYYYYYK",
    "KWKYYYYYYYYYYKYK",
    "KYYYYYYYYYYYYYYK",
    "KYYYYYKKKKYYYYYK",
    "KYYYYKKYYKKYYYYK",
    "KYYYYYYYYKKYYYYK",
    "KYYYYYYYKKYYYYYK",
    "KYYYYYYYYKKYYYYK",
    "KYYYYYYYYYYYYYYK",
    "KYYYYYYYYKKYYYYK",
    "KYYYYYYYYKKYYYYK",
    "KYYYYYYYYYYYYYYK",
    "KYKYYYYYYYYYYKYK",
    "KYYYYYYYYYYYYYYK",
    "KKKKKKKKKKKKKKKK",
};

static const char* const USEDBLOCK[] = {
    "KKKKKKKKKKKKKKKK",
    "KAAAAAAAAAAAAAAK",
    "KAKAAAAAAAAAAKAK",
    "KAAAAAAAAAAAAAAK",
    "KAAAAAAAAAAAAAAK",
    "KAAAAAAAAAAAAAAK",
    "KAAAAAAAAAAAAAAK",
    "KAAAAAAAAAAAAAAK",
    "KAAAAAAAAAAAAAAK",
    "KAAAAAAAAAAAAAAK",
    "KAAAAAAAAAAAAAAK",
    "KAAAAAAAAAAAAAAK",
    "KAAAAAAAAAAAAAAK",
    "KAKAAAAAAAAAAKAK",
    "KAAAAAAAAAAAAAAK",
    "KKKKKKKKKKKKKKKK",
};

static const char* const BRICK[] = {
    "OOOOOOOKOOOOOOOO",
    "OOOOOOOKOOOOOOOO",
    "OOOOOOOKOOOOOOOO",
    "KKKKKKKKKKKKKKKK",
    "OOOKOOOOOOOKOOOO",
    "OOOKOOOOOOOKOOOO",
    "OOOKOOOOOOOKOOOO",
    "KKKKKKKKKKKKKKKK",
    "OOOOOOOKOOOOOOOO",
    "OOOOOOOKOOOOOOOO",
    "OOOOOOOKOOOOOOOO",
    "KKKKKKKKKKKKKKKK",
    "OOOKOOOOOOOKOOOO",
    "OOOKOOOOOOOKOOOO",
    "OOOKOOOOOOOKOOOO",
    "KKKKKKKKKKKKKKKK",
};

static const char* const GROUND[] = {
    "WWWWWWWWWWWWWWWW",
    "EEEEEEEKEEEEEEEE",
    "EEEEEEEKEEEEEEEE",
    "KKKKKKKKKKKKKKKK",
    "EEEKEEEEEEEEKEEE",
    "EEEKEEEEEEEEKEEE",
    "EEEKEEEEEEEEKEEE",
    "KKKKKKKKKKKKKKKK",
    "EEEEEEEKEEEEEEEE",
    "EEEEEEEKEEEEEEEE",
    "KKKKKKKKKKKKKKKK",
    "EEEKEEEEEEEEKEEE",
    "EEEKEEEEEEEEKEEE",
    "EEEKEEEEEEEEKEEE",
    "EEEEEEEKEEEEEEEE",
    "KKKKKKKKKKKKKKKK",
};

// ---------------- 水管（左右两半，各 16x16） ----------------
static const char* const PIPE_TL[] = {
    "KKKKKKKKKKKKKKKK",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KLPPPPPPPPPPPPPP",
    "KKKKKKKKKKKKKKKK",
};

static const char* const PIPE_TR[] = {
    "KKKKKKKKKKKKKKKK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "PPPPPPPPPPPPPPPK",
    "KKKKKKKKKKKKKKKK",
};

static const char* const PIPE_BL[] = {
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
    "..KLPPPPPPPPPPPP",
};

static const char* const PIPE_BR[] = {
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
    "PPPPPPPPPPPPK...",
};

// ---------------- 云朵（24x10，换色即树丛） ----------------
static const char* const CLOUD[] = {
    ".......WWWWW............",
    ".....WWWWWWWWW..........",
    "....WWWWWWWWWWW.........",
    "...WWWWWWWWWWWWWW.......",
    "..WWWWWWWWWWWWWWWWW.....",
    ".WWWWWWWWWWWWWWWWWWWWW..",
    "WWWWWWWWWWWWWWWWWWWWWWWW",
    "WWWWWWWWWWWWWWWWWWWWWWWW",
    ".WWccccccccccccccccccWW.",
    "..cccccccccccccccccccc..",
};

// ---------------- 山（32x14） ----------------
static const char* const HILL[] = {
    "..............PP..............",
    ".............PPPP.............",
    "............PPPPPP............",
    "...........PPPPPPPP...........",
    "..........PPLPPPPPPP..........",
    ".........PPPPPPPPPPPP.........",
    "........PPPPPPPPPPPPPP........",
    ".......PPPLPPPPPPPPPPPP.......",
    "......PPPPPPPPPPPPPPPPPP......",
    ".....PPPPPPPPPPPLPPPPPPPPP....",
    "....PPPPPPPPPPPPPPPPPPPPPP....",
    "...PPPPPPPPPPPPPPPPPPPPPPPP...",
    "..PPPPPPPPPPPPPPPPPPPPPPPPPP..",
    ".PPPPPPPPPPPPPPPPPPPPPPPPPPPP.",
};

// ---------------- 旗（10x8，右缘贴旗杆） ----------------
static const char* const FLAG[] = {
    "RRRRRRRR..",
    ".RRRRRRR..",
    "..RRRRRR..",
    "...RRRRR..",
    "....RRRR..",
    ".....RRR..",
    "......RR..",
    ".......R..",
};

// ---------------- 5x7 点阵字库（够用的字符集） ----------------
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

#endif // PIXELART_H
