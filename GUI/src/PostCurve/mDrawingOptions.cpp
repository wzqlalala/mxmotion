#include "mDrawingOptions.h"

#include "CurveCore/CurveDefine.h"
#include "mDlgGeometry.h"

namespace PostCurve
{
    mDrawingOptions::mDrawingOptions(QWidget* parent)
        : MechMainWindow::mFramelessDialog(parent)
    {
        InitUI();
        InitConnct();
        //基类构造已重设窗口标志（自绘标题栏无"？"帮助钮），置顶需求在此恢复
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
    }

    mDrawingOptions::~mDrawingOptions()
    {
    }

    const DrawingOptionsStr& mDrawingOptions::getDrawOption()
    {
        //实时取当前界面状态：弹窗为非模态（mOpenCurveDlg::slot_drawOptions 用 show
        //打开），用户改完控件可能不点"保存"直接回主对话框建曲线；此时读到的选项
        //必须反映已改的值，否则绘图选项设置的字体/对齐等不生效
        syncDrawOptionFromUi();
        return drawoption;
    }

    int mDrawingOptions::headerAlignIndex() const
    {
        return headerAlignCombo ? headerAlignCombo->currentIndex() : 1;
    }

    int mDrawingOptions::footerAlignIndex() const
    {
        return footerAlignCombo ? footerAlignCombo->currentIndex() : 1;
    }

    void mDrawingOptions::setCoordinateAxisTitle(CoordinateAxisTitle t)
    {
        title = t;

        drawoption.xTitle = spliceTitle(xSelected);
        drawoption.yTitle = spliceTitle(ySelected);
    }

    void mDrawingOptions::InitUI()
    {
        setWindowTitle("绘图选项");
        resize(640, 400);
        DlgGeom::bind(this, "DrawingOptions");   //记住大小/位置：恢复上次值，隐藏时自动保存

        //白色背景（同"从文件创建曲线"弹窗：内容区在 QStackedWidget 里，
        //需显式刷白盖过全局 qss 的 QStackedWidget#F0F3F6 底色）
        setStyleSheet(R"(
        QDialog{
            background-color:#FFFFFF;
        }
        QStackedWidget{
            background-color:#FFFFFF;
        }
        QListWidget{
            background-color:#FFFFFF;
            border:none;
            border-right:1px solid #dbe5ee;
        }
        QPushButton#btnSave{
            background-color:#1385F6;
            color:#FFFFFF;
            border:none;
            border-radius:4px;
            padding:0px;
        }
        QPushButton#btnSave:hover{
            background-color:#3D9EF8;
        }
        QPushButton#btnSave:pressed{
            background-color:#0F6ACB;
        }
    )");

        QListWidget* listWidget = new QListWidget(this);
        listWidget->addItems({ "页眉", "页脚", "横轴", "纵轴", "曲线选项" });
        listWidget->setFixedWidth(120);

        QStackedWidget* stackedWidget = new QStackedWidget(this);

        // 1. 创建页眉页面
        QWidget* headerPage = new QWidget(this);
        QVBoxLayout* headerLayout = new QVBoxLayout(headerPage);
        headerCheck = new QCheckBox("显示页眉:", headerPage);
        headerCheck->setChecked(true);
        headerLayout->addWidget(headerCheck);
        headerFontLabel = new QLabel("字体设置");
        headerAlignLabel = new QLabel("对齐方式");
        QFormLayout* headerForm = new QFormLayout();
        headerForm->setVerticalSpacing(10);

        QHBoxLayout* headerFontRow = new QHBoxLayout();
        headerFontBtn = new QPushButton(" 默认");
        headerFontBtn->setStyleSheet("text-align: left;");
        headerFontRow->addWidget(headerFontBtn);

        headerAlignCombo = new QComboBox(headerPage); headerAlignCombo->addItems({ "左对齐", "居中", "右对齐" });
        headerAlignCombo->setCurrentIndex(1);
        headerForm->addRow(headerFontLabel, headerFontRow);
        headerForm->addRow(headerAlignLabel, headerAlignCombo);
        headerLayout->addLayout(headerForm);
        headerLayout->addStretch();
        stackedWidget->addWidget(headerPage);

        // 2. 创建页脚页面
        QWidget* footerPage = new QWidget(this);
        QVBoxLayout* footerLayout = new QVBoxLayout(footerPage);
        footerCheck = new QCheckBox("显示页脚:", footerPage);
        footerCheck->setChecked(false);
        footerLayout->addWidget(footerCheck);
        footerFontLabel = new QLabel("字体设置");
        footerAlignLabel = new QLabel("对齐方式");
        QFormLayout* footerForm = new QFormLayout();
        footerForm->setVerticalSpacing(10);
        QHBoxLayout* footerFontRow = new QHBoxLayout();
        footerFontBtn = new QPushButton(" 默认");
        footerFontBtn->setStyleSheet("text-align: left;");
        footerFontRow->addWidget(footerFontBtn);

        footerAlignCombo = new QComboBox(footerPage); footerAlignCombo->addItems({ "左对齐", "居中", "右对齐" });
        footerAlignCombo->setCurrentIndex(1);
        footerForm->addRow(footerFontLabel, footerFontRow);
        footerForm->addRow(footerAlignLabel, footerAlignCombo);
        footerLayout->addLayout(footerForm);
        footerLayout->addStretch();
        stackedWidget->addWidget(footerPage);

        //不勾选会禁用
        footerFontLabel->setEnabled(false);
        footerAlignLabel->setEnabled(false);
        footerFontBtn->setEnabled(false);
        footerAlignCombo->setEnabled(false);

        // 3. 创建横轴页面
        QWidget* xPage = new QWidget(this);
        QVBoxLayout* xLayout = new QVBoxLayout(xPage);
        QFormLayout* xForm = new QFormLayout();
        xForm->setVerticalSpacing(10);

        xTitleCombo = new MechMainWindow::mMultiSelectComboBox(xPage);
        xTitleCombo->addItems({ "X File Path","X File Name","X Subcase","X Type","X Request","X Component","Y File Path","Y File Name","Y Subcase","Y Type","Y Request","Y Component" });
        xTitleCombo->setBaseStr("X");
        xTitleCombo->setCheckedItems({ "X Type" });
        QHBoxLayout* xFontRow = new QHBoxLayout();
        xFontBtn = new QPushButton(" 默认");
        xFontBtn->setStyleSheet("text-align: left;");
        xFontRow->addWidget(xFontBtn);
        xScaleCombo = new QComboBox(xPage); xScaleCombo->addItems({ "Linear", "Logarithmic", "dB20", "dB10" });
        xWeightCombo = new QComboBox(xPage); xWeightCombo->addItems({ "无", "A", "B", "C", "U" });
        xForm->addRow("标题设置", xTitleCombo);
        xForm->addRow("字体设置", xFontRow);
        xForm->addRow("刻度类型", xScaleCombo);
        xForm->addRow("加    权", xWeightCombo);
        xLayout->addLayout(xForm); xLayout->addStretch();
        stackedWidget->addWidget(xPage);

        // 4. 创建纵轴页面
        QWidget* yPage = new QWidget(this);
        QVBoxLayout* yLayout = new QVBoxLayout(yPage);
        QFormLayout* yForm = new QFormLayout();
        yForm->setVerticalSpacing(10);
        yTitleCombo = new MechMainWindow::mMultiSelectComboBox(yPage);
        yTitleCombo->addItems({ "X File Path","X File Name","X Subcase","X Type","X Request","X Component","Y File Path","Y File Name","Y Subcase","Y Type","Y Request","Y Component" });
        yTitleCombo->setBaseStr("Y");
        yTitleCombo->setCheckedItems({ "Y Type" });
        QHBoxLayout* yFontRow = new QHBoxLayout();
        yFontBtn = new QPushButton(" 默认");
        yFontBtn->setStyleSheet("text-align: left;");
        yFontRow->addWidget(yFontBtn);
        yScaleCombo = new QComboBox(yPage); yScaleCombo->addItems({ "Linear", "Logarithmic", "dB20", "dB10" });
        yWeightCombo = new QComboBox(yPage); yWeightCombo->addItems({ "无", "A", "B", "C", "U" });
        yForm->addRow("标题设置", yTitleCombo);
        yForm->addRow("字体设置", yFontRow);
        yForm->addRow("刻度类型", yScaleCombo);
        yForm->addRow("加    权", yWeightCombo);
        yLayout->addLayout(yForm); yLayout->addStretch();
        stackedWidget->addWidget(yPage);

        // 5. 创建曲线选项页面
        QWidget* curvePage = new QWidget(this);
        QVBoxLayout* curveLayout = new QVBoxLayout(curvePage);
        QFormLayout* curveForm = new QFormLayout();
        curveForm->setVerticalSpacing(10);
        colorSchemeCombo = new QComboBox(curvePage); colorSchemeCombo->addItems({ "默认", "全部统一颜色" });
        lineColorBtn = new ColorPickButton(QColor(230, 170, 170), curvePage);
        lineStyleSchemeCombo = new QComboBox(curvePage); lineStyleSchemeCombo->addItems({ "默认", "全部统一线型" });
        lineStyleCombo = new QComboBox(curvePage);
        lineStyleCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/实线.svg"), "");
        lineStyleCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/虚线.svg"), "");
        lineStyleCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/点划线1.svg"), "");
        lineStyleCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/点划线2.svg"), "");

        lineWidthSchemeCombo = new QComboBox(curvePage);
        lineWidthSchemeCombo->addItems({ "默认", "全部统一线宽" });
        lineWidthCombo = new QComboBox(curvePage);
        lineWidthCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/1px.svg"), "");
        lineWidthCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/2px.svg"), "");
        lineWidthCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/3px.svg"), "");
        lineWidthCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/4px.svg"), "");

        symbolSchemeCombo = new QComboBox(curvePage);
        symbolSchemeCombo->addItems({ "默认", "全部统一符号" });
        symbolCombo = new QComboBox(curvePage);
        //索引即模型符号样式值（0无 1圆 2方 3菱 4三角 5十字 6倒三角 7星(五角星)
        //8交叉 9点），与属性编辑器/曲线属性页下拉一致；米字 Star1 仅库内保留
        symbolCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/标识无.svg"), "");
        symbolCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/添加.svg"), "");
        symbolCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/方.svg"), "");
        symbolCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/菱形.svg"), "");
        symbolCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/三角.svg"), "");
        symbolCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/十字.svg"), "");
        symbolCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/倒三角.svg"), "");
        symbolCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/星形.svg"), "");
        symbolCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/交叉.svg"), "");
        symbolCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/圆点.svg"), "");

        symbolColorBtn = new ColorPickButton(QColor(230, 170, 170), curvePage);
        lineColorLabel = new QLabel("线条颜色");
        lineStyleLabel = new QLabel("线型");
        lineWidthLabel = new QLabel("线宽");
        symbolLabel = new QLabel("符号");
        symbolColorLabel = new QLabel("符号颜色");

        curveForm->addRow("线条颜色方案", colorSchemeCombo);
        curveForm->addRow(lineColorLabel, lineColorBtn);
        curveForm->addRow("线型方案", lineStyleSchemeCombo);
        curveForm->addRow(lineStyleLabel, lineStyleCombo);
        curveForm->addRow("线宽方案", lineWidthSchemeCombo);
        curveForm->addRow(lineWidthLabel, lineWidthCombo);
        curveForm->addRow("符号方案", symbolSchemeCombo);
        curveForm->addRow(symbolLabel, symbolCombo);
        curveForm->addRow(symbolColorLabel, symbolColorBtn);
        curveLayout->addLayout(curveForm); curveLayout->addStretch();
        stackedWidget->addWidget(curvePage);
        //默认状态需要禁用
        lineColorLabel->setEnabled(false);
        lineStyleLabel->setEnabled(false);
        lineWidthLabel->setEnabled(false);
        symbolLabel->setEnabled(false);
        symbolColorLabel->setEnabled(false);
        lineColorBtn->setEnabled(false);
        lineStyleCombo->setEnabled(false);
        lineWidthCombo->setEnabled(false);
        symbolCombo->setEnabled(false);
        symbolColorBtn->setEnabled(false);

        // 总布局
        QHBoxLayout* mainLayout = new QHBoxLayout();
        mainLayout->addWidget(listWidget);
        mainLayout->addWidget(stackedWidget, 1);

        QPushButton* saveBtn = new QPushButton("保存", this);
        QPushButton* cancelBtn = new QPushButton("取消", this);
        saveBtn->setObjectName("btnSave");   //蓝色主按钮样式见本弹窗样式表（同主弹窗"绘图"钮）
        saveBtn->setFixedSize(44, 26);
        cancelBtn->setFixedSize(44, 26);
        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->addStretch();                // 弹簧，把按钮推到最右边
        btnLayout->addWidget(cancelBtn);
        btnLayout->addWidget(saveBtn);
        btnLayout->setSpacing(10);

        // 总布局装进内容容器再交给基类（基类 this 的布局已被标题栏占用）
        QWidget* content = new QWidget(this);
        QVBoxLayout* topLayout = new QVBoxLayout(content);
        topLayout->addLayout(mainLayout);
        topLayout->addLayout(btnLayout);
        setContentWidget(content);

        listWidget->setCurrentRow(0);

        //字体成员预设为各"默认"字体（否则为默认构造的 QFont，约 9pt 常规体，
        //经 syncDrawOptionFromUi 建图后会渲染成小字号）：既作 sync 的兜底值，
        //也作字体对话框的预选值；用户选过则被覆盖
        headerFont = headerDefaultFont();
        footerFont = footerDefaultFont();
        xFont = axisTitleDefaultFont();
        yFont = axisTitleDefaultFont();

        syncDrawOptionFromUi();

        QTimer::singleShot(0, this, [this]() {
            const auto combos = this->findChildren<QComboBox*>();
            for (QComboBox* combo : combos) {
                QListView* view = qobject_cast<QListView*>(combo->view());
                if (view) {
                    view->setUniformItemSizes(true);
                    view->setGridSize(QSize(24, 24));
                }
            }
            });

        // 5. 连接信号
        connect(saveBtn, &QPushButton::clicked, this, &mDrawingOptions::onSaveClicked);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        connect(listWidget, &QListWidget::currentRowChanged, stackedWidget, &QStackedWidget::setCurrentIndex);

        connect(headerFontBtn, &QPushButton::clicked, this, &mDrawingOptions::openFontDlg);
        connect(footerFontBtn, &QPushButton::clicked, this, &mDrawingOptions::openFontDlg);
        connect(xFontBtn, &QPushButton::clicked, this, &mDrawingOptions::openFontDlg);
        connect(yFontBtn, &QPushButton::clicked, this, &mDrawingOptions::openFontDlg);
    }

    void mDrawingOptions::InitConnct()
    {
        connect(headerCheck, &QAbstractButton::toggled, this, [this](bool check) {
            if (check)
            {
                headerFontLabel->setEnabled(true);
                headerAlignLabel->setEnabled(true);
                headerFontBtn->setEnabled(true);
                headerAlignCombo->setEnabled(true);
            }
            else {
                headerFontLabel->setEnabled(false);
                headerAlignLabel->setEnabled(false);
                headerFontBtn->setEnabled(false);
                headerAlignCombo->setEnabled(false);
            }});
        connect(footerCheck, &QAbstractButton::toggled, this, [this](bool check) {
            if (check)
            {
                footerFontLabel->setEnabled(true);
                footerAlignLabel->setEnabled(true);
                footerFontBtn->setEnabled(true);
                footerAlignCombo->setEnabled(true);
            }
            else {
                footerFontLabel->setEnabled(false);
                footerAlignLabel->setEnabled(false);
                footerFontBtn->setEnabled(false);
                footerAlignCombo->setEnabled(false);
            }});
        connect(colorSchemeCombo, &QComboBox::currentTextChanged, this, [this](QString text) {
            if (text == "默认")
            {
                lineColorLabel->setEnabled(false);
                lineColorBtn->setEnabled(false);
                lineColorBtn->setColor(QColor(230, 170, 170));
            }
            else {
                lineColorLabel->setEnabled(true);
                lineColorBtn->setEnabled(true);
                lineColorBtn->setColor(QColor(255, 100, 100));
            }});
        connect(lineStyleSchemeCombo, &QComboBox::currentTextChanged, this, [this](QString text) {
            if (text == "默认") {
                lineStyleLabel->setEnabled(false);
                lineStyleCombo->setEnabled(false);
            }
            else {
                lineStyleLabel->setEnabled(true);
                lineStyleCombo->setEnabled(true);
            }});
        connect(lineWidthSchemeCombo, &QComboBox::currentTextChanged, this, [this](QString text) {
            if (text == "默认") {
                lineWidthCombo->setEnabled(false);
                lineWidthLabel->setEnabled(false);
            }
            else {
                lineWidthCombo->setEnabled(true);
                lineWidthLabel->setEnabled(true);
            }});
        connect(symbolSchemeCombo, &QComboBox::currentTextChanged, this, [this](QString text) {
            if (text == "默认") {
                symbolLabel->setEnabled(false);
                symbolColorLabel->setEnabled(false);
                symbolCombo->setEnabled(false);
                symbolColorBtn->setEnabled(false);
                symbolColorBtn->setColor(QColor(230, 170, 170));
            }
            else {
                symbolLabel->setEnabled(true);
                symbolColorLabel->setEnabled(true);
                symbolCombo->setEnabled(true);
                symbolColorBtn->setEnabled(true);
                symbolColorBtn->setColor(QColor(255, 100, 100));
            }});
    }

    void mDrawingOptions::openFontDlg() {
        // 1. 获取当前点击的按钮
        QPushButton* btn = qobject_cast<QPushButton*>(sender());
        if (!btn) return;

        // 2. 判断是哪个页面的按钮，取出对应的 QFont 成员变量
        QFont* targetFont = nullptr;
        if (btn == headerFontBtn)      targetFont = &headerFont;
        else if (btn == footerFontBtn) targetFont = &footerFont;
        else if (btn == xFontBtn)      targetFont = &xFont;
        else if (btn == yFontBtn)      targetFont = &yFont;
        if (!targetFont) return;

        // 3. 弹出字体选择框
        bool ok;
        QFont font = QFontDialog::getFont(&ok, *targetFont, this, "选择字体");

        if (ok) {
            // 更新保存的字体对象
            *targetFont = font;

            // 处理字号
            int size = font.pointSize();
            if (size <= 0) size = font.pixelSize();

            // 更新按钮文字
            btn->setText(font.family());
            btn->setFont(*targetFont);
        }
    }

    QString mDrawingOptions::spliceTitle(QList<int>& list)
    {
        QStringList parts;
        for (auto i : list)
        {
            QString t;
            switch (i)
            {
            case 0:
                t = title.xFilePath; break;
            case 1:
                t = title.xFileName; break;
            case 2:
                t = title.xSubcase; break;
            case 3:
                t = title.xType; break;
            case 4:
                t = title.xRequest; break;
            case 5:
                t = title.xComponent; break;
            case 6:
                t = title.yFilePath; break;
            case 7:
                t = title.yFileName; break;
            case 8:
                t = title.ySubcase; break;
            case 9:
                t = title.yType; break;
            case 10:
                t = title.yRequest; break;
            case 11:
                t = title.yComponent; break;
            default:
                break;
            }
            if (!t.isEmpty())
                parts << t;
        }
        return parts.join("-");
    }

    void mDrawingOptions::onSaveClicked()
    {
        syncDrawOptionFromUi();
        accept();
    }

    void mDrawingOptions::syncDrawOptionFromUi()
    {
        drawoption.header = headerCheck->isChecked();
        //按钮为"默认"时用预写默认字体（CurveDefine 同源，属性页回显基准一致）；
        //用户选过则用所选字体。按钮初始文本含前导空格（" 默认"），须去空白后比较
        drawoption.headerFont = (headerFontBtn->text().trimmed() == QStringLiteral("默认"))
            ? headerDefaultFont() : headerFont;


        drawoption.headerAlign = headerAlignCombo->currentText();

        drawoption.footer = footerCheck->isChecked();
        drawoption.footerFont = (footerFontBtn->text().trimmed() == QStringLiteral("默认"))
            ? footerDefaultFont() : footerFont;


        drawoption.footerAlign = footerAlignCombo->currentText();
        xSelected = xTitleCombo->checkedIndexes();
        drawoption.xFont = (xFontBtn->text().trimmed() == QStringLiteral("默认"))
            ? axisTitleDefaultFont() : xFont;

        drawoption.xScale = xScaleCombo->currentText();
        drawoption.xWeight = xWeightCombo->currentText();

        ySelected = yTitleCombo->checkedIndexes();
        drawoption.yFont = (yFontBtn->text().trimmed() == QStringLiteral("默认"))
            ? axisTitleDefaultFont() : yFont;

        drawoption.yScale = yScaleCombo->currentText();
        drawoption.yWeight = yWeightCombo->currentText();

        if (colorSchemeCombo->currentText() == "默认") drawoption.selectLineColor = false;
        else drawoption.selectLineColor = true;
        drawoption.lineColor = lineColorBtn->color().name();

        if (lineStyleSchemeCombo->currentText() == "默认") drawoption.selectLineStyleScheme = false;
        else drawoption.selectLineStyleScheme = true;
        drawoption.lineStyleScheme = lineStyleCombo->currentIndex();

        if (lineWidthSchemeCombo->currentText() == "默认") drawoption.selectLineWidthScheme = false;
        else drawoption.selectLineWidthScheme = true;
        drawoption.lineWidthScheme = lineWidthCombo->currentIndex();

        if (symbolSchemeCombo->currentText() == "默认") drawoption.selectSymbol = false;
        else drawoption.selectSymbol = true;

        drawoption.symbol = symbolCombo->currentIndex();
        drawoption.symbolColor = symbolColorBtn->color().name();
    }
}