#pragma once

#include <QSize>
#include <QString>

namespace MxFunctions {

// Logical pixels for the shared chrome. ThemeManager owns persistence and re-layout.
class UiMetrics final
{
public:
    static int level() { return s_level; }
    static void setLevel(int level) { s_level = qBound(1, level, 5); }

    static int baseH() { return 26 + 4 * (level() - 3); }
    static int baseG() { return 6 + (level() - 3); }
    static int controlHeight() { return baseH(); }
    static int controlIconSize() { return qMax(baseH() / 2 + 3, baseG() + 10); }
    static int choiceSize() { return baseH() / 2; }
    static constexpr int zoneRadius() { return 4; }

    // The original CommonWidgets/FoldSection uses these same logical tokens.
    static int labelWidth() { return 7 * bodyFontSize(); }
    static int fieldInset() { return baseG() + 2; }
    static int columnGap() { return baseG() + 2; }
    static int rowGap() { return baseG(); }
    static int choiceGap() { return qMax(2, baseG() - 2); }
    static int foldHeaderHeight() { return baseH(); }
    static int iconSize() { return controlIconSize(); }

    static QString uiFontFamily()
    {
#ifdef Q_OS_WIN
        return QStringLiteral("Microsoft YaHei");
#else
        return QStringLiteral("Sans Serif");
#endif
    }
    static int bodyFontSize() { return 12 + (level() - 3); }
    static int helperFontSize() { return bodyFontSize() - 1; }
    static int buttonFontSize() { return bodyFontSize() + 2; }
    static int tabFontSize() { return buttonFontSize(); }
    static int groupTitleFontSize() { return bodyFontSize(); }
    static int windowTitleFontSize() { return bodyFontSize(); }

    static int windowHeaderHeight() { return baseH() + 2; }
    static int windowHeaderControl() { return baseH() - 2; }
    static int windowInset() { return baseG() + 2; }
    static int quickButtonSize() { return windowHeaderControl(); }
    static QSize quickIconSize() { return QSize(controlIconSize(), controlIconSize()); }
    static int quickGap() { return baseG() - 2; }
    static int separatorLineHeight() { return controlIconSize(); }
    static int tabBarHeight() { return windowHeaderHeight(); }
    static int tabPadding() { return windowInset(); }
    static constexpr int statusBarHeight() { return 24; }
    static constexpr int statusButtonHeight() { return 19; }

    static int smallButtonHeight() { return baseH(); }
    static int bigButtonHeight() { return 2 * (baseH() + baseG()); }
    static constexpr int stackGap() { return 2; }
    static int halfButtonHeight() { return (bigButtonHeight() - stackGap()) / 2; }
    static int menuInset() { return baseG() - 2; }
    static int menuGap() { return menuInset(); }
    static int menuContentGap() { return qMax(1, baseG() - 4); }
    static int menuIconSize() { return qMax(baseH() / 2 + 7, baseG() + 10); }
    static int menuLargeIconSize() { return baseH() + 4; }
    static QSize comboIconSize() { return QSize(menuLargeIconSize(), menuLargeIconSize()); }
    static int menuFullMinWidth() { return menuLargeIconSize() + 2 * menuInset(); }
    static int menuLineHeight() { return buttonFontSize() + 4; }
    static int dropArrowTextGap() { return menuContentGap(); }
    static int arrowReserveWidth() { return dropdownSlot() + dropArrowTextGap(); }
    static qreal bigButtonTextBottomPaddingF()
    {
        // Center the text line in the space occupied by the lower half button.
        return (halfButtonHeight() - menuLineHeight()) / 2.0;
    }

    static int groupInset() { return baseG() + 1; }
    static int groupContentHeight() { return bigButtonHeight(); }
    static int groupTitleHeight() { return baseH() - baseG(); }
    static int groupSeparatorHeight() { return groupContentHeight() + groupTitleHeight(); }
    static int ribbonTopInset() { return menuInset(); }
    static int ribbonBottomInset() { return menuInset(); }
    static int menuBarTotalHeight()
    {
        return tabBarHeight() + ribbonTopInset() + groupSeparatorHeight() + ribbonBottomInset();
    }

    static int menuItemHeight() { return controlHeight(); }
    static int menuItemInset() { return baseG() + 1; }
    static int menuSeparatorHeight() { return 2 * menuInset() + 1; }
    static int menuIconTextGap() { return baseG(); }
    static int menuMinWidth() { return 12 * bodyFontSize(); }
    static int dropdownSlot() { return bodyFontSize(); }
    static qreal menuRightSlotPaddingF() { return 2 * baseG() + dropdownSlot() / 2.0; }

private:
    inline static int s_level = 3;
};

} // namespace MxFunctions
