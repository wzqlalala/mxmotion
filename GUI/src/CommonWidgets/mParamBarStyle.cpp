#include "mParamBarStyle.h"

#include "UiMetrics.h"
#include "UiPalette.h"

#include <QWidget>

using namespace MxFunctions;

namespace MechMainWindow
{
    void applyParamBarStyle(QWidget* host)
    {
        if (!host)
            return;

        /*
        * 规格与全局 mApplicationStart.qss 同源：值在调用时从 UiPalette/UiMetrics
        * 取（与 ThemeManager::applyGlobalQss 的令牌表同一口径），切主题/切档后由
        * mPanelBase::applyTheme 重发，不再与全局 QSS 漂移。
        * QSS 的 width/height 是内容区尺寸：带 1px 边框的控件写 H−2（同全局 QSS 的
        * size.controlIn 令牌），加上下边框后控件外高恰为 H（规则 0.1"控件外高"语义）
        */
        const int bodyFont = UiMetrics::bodyFontSize();
        QString qss = QStringLiteral(R"(
QPushButton { font-size:%6px; font-family:'%11'; height:%7px; background-color:%8; border:1px solid %2; border-radius:3px; }
QPushButton:hover { background-color:%1; border-radius:4px; border:1px solid %2; }
QPushButton:pressed, QPushButton:checked { background-color:%3; border-radius:4px; border:1px solid %4; }
QPushButton:disabled { color:%5; background-color:%1; border-radius:4px; }
QToolButton { font-size:%6px; font-family:'%11'; width:%7px; height:%7px; background-color:%8; border-radius:3px; border:1px solid %2; }
QToolButton:hover { background-color:%1; border-radius:4px; border:1px solid %2; }
QToolButton:pressed, QToolButton:checked { background-color:%3; border-radius:4px; border:1px solid %4; }
QToolButton:disabled { color:%5; background-color:%1; border-radius:4px; }
QLineEdit { font-family:'%11'; font-size:%6px; height:%7px; color:%9; border:1px solid %2; border-radius:4px; background-color:%8; }
QLineEdit:hover { color:%9; border:1px solid %10; }
QLineEdit:focus { color:%9; border:1px solid %4; }
QLineEdit:disabled { background-color:%1; color:%5; }
QComboBox:disabled { background-color:%1; color:%5; }
QTextEdit { font-family:'%11'; font-size:%6px; }
QCheckBox { font-family:'%11'; font-size:%6px; color:%9; }
)")
            .arg(UiPalette::panelHex(),     // %1 悬停底/禁用底（全局 QSS 两态同用 panel）
                 UiPalette::borderHex(),    // %2 常态/悬停边框
                 UiPalette::selectedHex(),  // %3 按压/选中底
                 UiPalette::primaryHex(),   // %4 按压边框/输入焦点边框
                 UiPalette::disabledHex())  // %5 禁用文字
            .arg(bodyFont)                  // %6 正文字号
            .arg(UiMetrics::controlHeight())// %7 带 1px 边框控件的内容区高（H−2）
            .arg(UiPalette::surfaceHex(),   // %8 常态底
                 UiPalette::textHex(),      // %9 正文文字
                 UiPalette::focusHex())     // %10 输入悬停边框
            .arg(UiMetrics::uiFontFamily());// %11 平台 UI 字体族
        if (host->styleSheet() != qss)
            host->setStyleSheet(qss);
    }
}
