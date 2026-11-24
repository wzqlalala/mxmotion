#pragma once
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include <QRect>
#include <QSize>
#include <QVector>

namespace MechMainWindow {

    struct mViewLayoutDef
    {
        const char* icon;   //图标文件名（:/mMainWidgets/Icon/CenterMainWindow/<icon>）
        const char* name;   //显示名/tooltip
        bool verticalBands;
        int bands[4];       //各条带格子数
        int bandCount;
    };

    inline const QVector<mViewLayoutDef>& viewLayoutTable()
    {
        //verticalBands: true=条带纵向堆叠(行)；false=条带横向排列(列)
        static const QVector<mViewLayoutDef> table = {
            { "1.svg",           "单视口",          true,  {1}, 1 },
            { "2@横分.svg",      "2视口-横分",      true,  {1,1}, 2 },
            { "2@纵分.svg",      "2视口-纵分",      false, {1,1}, 2 },
            { "3@上1下2.svg",    "3视口-上1下2",    true,  {1,2}, 2 },
            { "3@上2下1.svg",    "3视口-上2下1",    true,  {2,1}, 2 },
            { "3@左1右2.svg",    "3视口-左1右2",    false, {1,2}, 2 },
            { "3@左2右1.svg",    "3视口-左2右1",    false, {2,1}, 2 },
            { "3@横分.svg",      "3视口-横分",      true,  {1,1,1}, 3 },
            { "3@纵分.svg",      "3视口-纵分",      false, {1,1,1}, 3 },
            { "4@均分.svg",      "4视口-均分",      true,  {2,2}, 2 },
            { "4@横分.svg",      "4视口-横分",      true,  {1,1,1,1}, 4 },
            { "4@纵分.svg",      "4视口-纵分",      false, {1,1,1,1}, 4 },
            { "6@上3下3.svg",    "6视口-上3下3",    true,  {3,3}, 2 },
            { "6@左3右3.svg",    "6视口-左3右3",    false, {3,3}, 2 },
            { "8@上4下4.svg",    "8视口-上4下4",    true,  {4,4}, 2 },
            { "8@左4右4.svg",    "8视口-左4右4",    false, {4,4}, 2 },
            { "9@均分.svg",      "9视口-均分",      true,  {3,3,3}, 3 },
            { "12@上4中4下4.svg","12视口-上4中4下4",true,  {4,4,4}, 3 },
            { "12@左4中4右4.svg","12视口-左4中4右4",false, {4,4,4}, 3 },
            { "16@均分.svg",     "16视口-均分",     true,  {4,4,4,4}, 4 },
        };
        return table;
    }

    inline int viewLayoutSlotCount(const mViewLayoutDef& def)
    {
        int n = 0;
        for (int b = 0; b < def.bandCount; ++b) n += def.bands[b];
        return n;
    }

    inline QRect viewLayoutCellRect(const mViewLayoutDef& def, int cellIndex, const QSize& area)
    {
        //定位条带与带内序号
        int before = 0, band = 0, inBand = 0;
        for (int b = 0; b < def.bandCount; ++b)
        {
            if (cellIndex < before + def.bands[b])
            {
                band = b;
                inBand = cellIndex - before;
                break;
            }
            before += def.bands[b];
        }
        if (def.verticalBands)
        {
            //行条带：等分高度；带内格子等分宽度横向排列
            const int bh = area.height() / def.bandCount;
            const int cw = area.width() / def.bands[band];
            return QRect(inBand * cw, band * bh, cw, bh);
        }
        //列条带：等分宽度；带内格子等分高度纵向堆叠
        const int bw = area.width() / def.bandCount;
        const int ch = area.height() / def.bands[band];
        return QRect(band * bw, inBand * ch, bw, ch);
    }

} // namespace MechMainWindow
