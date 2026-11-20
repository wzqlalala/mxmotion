#pragma once

//自绘标题栏对话框基类：标题栏/阴影/边框缩放/最大化动画由基类提供
#include "mFramelessDialog.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QColorDialog>
#include <QDebug>
#include <QTimer>
#include <QFontDialog>
#include <QFontDatabase>
#include "mMultiSelectComboBox.h"

// =============================================================================
// 绘图选项（pch 出图样式配置）
//
// DrawingOptionsStr  ：样式值结构——页眉页脚（开关/字体/对齐）、X/Y轴
//                      （标题/字体/刻度/加权）、线条（统一线色、线型/线宽方案）、符号；
// CoordinateAxisTitle：轴标题拼装的来源信息（文件/工况/结果类型/对象/分量），
//                      spliceTitle 按用户勾选拼出 xTitle/yTitle；
// ColorPickButton    ：颜色选择按钮（色块图标 + QColorDialog）；
// mDrawingOptions    ：绘图选项对话框（mOpenCurveDlg 的子弹窗）。
//
// 曲线模块框架落地后的映射关系（见 mOpenCurveDlg::createChartFromData）：
//   header           -> 图表 kHeaderVisible
//   xTitle / yTitle  -> XAxis/YAxis 节点 kTitle
//   selectLineColor+lineColor -> 全部曲线统一线色
// 其余字段（刻度/加权/字体方案等）暂由 PlotView 默认渲染，后续按需映射。
// =============================================================================

namespace PostCurve
{
struct DrawingOptionsStr
{
    bool header;
    QFont headerFont;
    QString headerAlign;

    bool footer;
    QFont footerFont;
    QString footerAlign;

    QString xTitle;
    QFont xFont;
    QString xScale;
    QString xWeight;

    QString yTitle;
    QFont yFont;
    QString yScale;
    QString yWeight;

    bool selectLineColor;
    QString lineColor;
    bool selectLineStyleScheme;
    int lineStyleScheme;
    bool selectLineWidthScheme;
    int lineWidthScheme;
    bool selectSymbol;
    int symbol;
    QString symbolColor;
};

struct CoordinateAxisTitle
{
    QString xFilePath;
    QString xFileName;
    QString xSubcase;
    QString xType;
    QString xRequest;
    QString xComponent;

    QString yFilePath;
    QString yFileName;
    QString ySubcase;
    QString yType;
    QString yRequest;
    QString yComponent;
};

//颜色选择按钮 
class ColorPickButton : public QPushButton {
public:
    ColorPickButton(QColor color, QWidget* parent = nullptr) : QPushButton(parent), m_color(color) {
        setObjectName("colorPickButton");
        setFixedSize(24, 24);
        setCursor(Qt::PointingHandCursor);
        updateStyle();
        connect(this, &QPushButton::clicked, this, [this]() {
            QColor color = QColorDialog::getColor(m_color, this, "选择颜色");
            if (color.isValid()) {
                m_color = color;
                updateStyle();
            }
            });
    }
    QColor color() const { return m_color; }

    void setColor(QColor color) {
        m_color = color;
        updateStyle();
    }
private:
    void updateStyle() {
        setStyleSheet(QString(
            "QPushButton#colorPickButton { background-color: %1; border: 1px solid #cccccc; border-radius: 2px; }"
        ).arg(m_color.name()));
    }

    QColor m_color;
};
}

namespace PostCurve
{
    class mDrawingOptions : public MechMainWindow::mFramelessDialog
    {
        Q_OBJECT

    public:
        mDrawingOptions(QWidget* parent = nullptr);
        ~mDrawingOptions();

        const DrawingOptionsStr& getDrawOption();

        void setCoordinateAxisTitle(CoordinateAxisTitle t);
        //页眉/页脚对齐下拉当前索引（0左对齐/1居中/2右对齐），
        //供曲线创建流程映射到模型 align 属性
        int headerAlignIndex() const;
        int footerAlignIndex() const;
    private slots:
        void onSaveClicked();

        //把当前界面状态（含各字体成员）同步进 drawoption：
        //onSaveClicked 与 getDrawOption 共用（弹窗非模态，读取时需反映未"保存"的改动）
        void syncDrawOptionFromUi();

        void InitUI();

        void InitConnct();

        void openFontDlg();

        QString spliceTitle(QList<int>& list);
    private:
        // 页眉控件
        QLabel* headerFontLabel;
        QLabel* headerAlignLabel;
        QCheckBox* headerCheck;
        QPushButton* headerFontBtn;
        QFont headerFont;
        QComboBox* headerAlignCombo;
        // 页脚控件
        QLabel* footerFontLabel;
        QLabel* footerAlignLabel;
        QCheckBox* footerCheck;
        QPushButton* footerFontBtn;
        QFont footerFont;
        QComboBox* footerAlignCombo;

        // 横轴控件
        MechMainWindow::mMultiSelectComboBox* xTitleCombo;
        QPushButton* xFontBtn;
        QFont xFont;
        QComboBox* xScaleCombo;
        QComboBox* xWeightCombo;

        // 纵轴控件
        MechMainWindow::mMultiSelectComboBox* yTitleCombo;
        QPushButton* yFontBtn;
        QFont yFont;
        QComboBox* yScaleCombo;
        QComboBox* yWeightCombo;
        // 曲线选项控件
        QLabel* lineColorLabel;
        QLabel* lineStyleLabel;
        QLabel* lineWidthLabel;
        QLabel* symbolLabel;
        QLabel* symbolColorLabel;

        QComboBox* colorSchemeCombo;
        ColorPickButton* lineColorBtn;
        QComboBox* lineStyleSchemeCombo;
        QComboBox* lineStyleCombo;
        QComboBox* lineWidthSchemeCombo;
        QComboBox* lineWidthCombo;
        QComboBox* symbolSchemeCombo;
        QComboBox* symbolCombo;
        ColorPickButton* symbolColorBtn;

        DrawingOptionsStr drawoption;
        CoordinateAxisTitle title;

        QList<int> xSelected;
        QList<int> ySelected;
    };
}