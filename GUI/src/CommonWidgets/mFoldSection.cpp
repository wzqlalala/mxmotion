#include "mFoldSection.h"

#include <QEvent>
#include <QPalette>
#include <QStyle>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPointer>

namespace
{
    //存活折叠组登记（applyThemeToAll 的遍历对象）；QPointer 防悬挂，
    //析构时主动摘除
    QList<QPointer<MechMainWindow::FoldWidget>>& liveFoldWidgets()
    {
        static QList<QPointer<MechMainWindow::FoldWidget>> s_live;
        return s_live;
    }
}

namespace MechMainWindow
{
    FoldWidget::FoldWidget(const QString& title, QWidget* parent, const QColor& iconColor)
        : QWidget(parent)
        //无效色=未指定：箭头色取 UiPalette::muted
        , m_iconColor(iconColor.isValid() ? iconColor : MxFunctions::UiPalette::muted())
        , m_customIconColor(iconColor.isValid())
    {

        m_iconOpen = getFoldIcon(true);
        m_iconClose = getFoldIcon(false);
        m_btn = new QPushButton(title, this);
        m_btn->setCheckable(true);
        //标题钮样式完全自管（applyUiTokens 按 UiMetrics/UiPalette 令牌下发，切档
        //经 applyThemeToAll 全量重发）。挂 ThemeManager 按钮分支豁免约定
        //（mxBtnNoTheme）：弹窗（QDialog）Show 时 applyDialogTheme 会整树逐控件
        //setQss 整表替换，不豁免则本钮被通用按钮样式覆盖——checked 态 selected
        //蓝底+默认居中文字，折叠组标题条在弹窗里与 dock（灰底/左对齐）两副面孔
        m_btn->setProperty("mxBtnNoTheme", true);
        //长标题省略时提供完整提示
        m_btn->setToolTip(title);

        m_checkBox = new QCheckBox(this);
        //m_checkBox->setMaximumWidth(20);
        m_checkBox->setText("");
        m_checkBox->setChecked(true);
        m_checkBox->hide();
        m_checkBox->installEventFilter(this);

        connect(m_checkBox, &QCheckBox::toggled, this, &FoldWidget::onCheckBoxToggled);

        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

        //箭头固定在标题条最右的 iconSize 独立槽位，不随标题长度移动（规则 2.2.1）；
        //点击箭头与点击标题行共用一次展开/收起（eventFilter 转发为 m_btn 点击）
        m_arrowLabel = new QLabel(this);
        m_arrowLabel->setScaledContents(true);
        m_arrowLabel->setAlignment(Qt::AlignCenter);
        m_arrowLabel->installEventFilter(this);

        connect(m_btn, &QPushButton::toggled, this, [this](bool checked) {
            if (!m_collapsible)
            {
                //折叠已禁用：标题头点击不收缩，弹回展开态（isChecked 被
                //refreshLayout/showEvent 用来判定内容可见），不动画、不发信号
                m_btn->blockSignals(true);
                m_btn->setChecked(true);
                m_btn->blockSignals(false);
                updateFoldArrow(true);
                return;
            }
            updateFoldArrow(checked);
            onFoldClicked(checked);
            emit foldExpandChanged(checked);
            });

        //标题条：header 色通栏，高 foldHeaderHeight 由按钮 min-height 决定（规则 2.2.1）
        m_headerWidget = new QWidget(this);
        m_headerLayout = new QHBoxLayout(m_headerWidget);
        m_headerLayout->setSpacing(0);
        m_headerLayout->addWidget(m_checkBox);
        m_headerLayout->addWidget(m_btn, 1);
        m_headerLayout->addWidget(m_arrowLabel);
        updateFoldArrow(m_btn->isChecked());
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(0);
        m_mainLayout->addWidget(m_headerWidget);

        //组内控件禁用态需在本层显式补齐：属性编辑器弹窗等宿主的
        //QWidget{background:white} 祖先样式表会盖过应用级 QSS 的
        //:disabled 规则（Qt 中越近的祖先样式表优先级越高），导致
        //setEnabled(false) 后控件看起来仍是白底可用的样子——挂在本类上，
        //无论宿主是谁禁用态灰色都有保证。取值取自 UiPalette（panel 底/
        //disabled 文字），对前处理面板是视觉无差的兜底
        setStyleSheet(QString(
            "FoldWidget{background:transparent;border:none;}"
            "QComboBox:disabled, QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled { background-color:%1; color:%2; }")
            .arg(MxFunctions::UiPalette::panelHex())
            .arg(MxFunctions::UiPalette::disabledHex()));

        //构造只建结构，全部令牌取值统一经 applyUiTokens 下发（与切档/切主题
        //刷新同一条代码路径，构造与刷新不会出现两份取值）
        applyUiTokens();

        //登记存活实例：切档/切主题由 ThemeManager 经 applier 调
        //applyThemeToAll() 统一刷新
        liveFoldWidgets().append(this);
    }

    FoldWidget::~FoldWidget()
    {
        liveFoldWidgets().removeAll(this);
    }

    void FoldWidget::applyThemeToAll()
    {
        const auto live = liveFoldWidgets();
        for (const QPointer<FoldWidget>& w : live)
            if (w)
                w->applyUiTokens();
    }

    void FoldWidget::applyUiTokens()
    {
        if (!m_btn)
            return;

        //标题条：字号/内边距/高随档位
        m_btn->setStyleSheet(QString(
            "QPushButton{ font-size: %1px; background:transparent; text-align:left; padding:0 %2px; border:none; margin:0; min-height:%3px; outline:none; color: %4; size: %1px;}"
            "QPushButton:hover{ background:transparent; }"
            "QPushButton:checked{ background:transparent; }"
            "QPushButton:pressed{ background:transparent; }")
            .arg(MxFunctions::UiMetrics::bodyFontSize())
            .arg(MxFunctions::UiMetrics::fieldInset())
            .arg(MxFunctions::UiMetrics::foldHeaderHeight())
            .arg(MxFunctions::UiPalette::textHex()));

        if (m_checkBox)
        {
            m_checkBox->setMinimumHeight(MxFunctions::UiMetrics::choiceSize());
            m_checkBox->setStyleSheet(QString(
                "QCheckBox{ background:transparent; border:none; margin:0; padding:0; min-height:%1px; outline:none; spacing:%2; }"
                "QCheckBox::indicator{ width:%1px; height:%1px; margin:0; padding:0; }"
                "QCheckBox:focus{ border:none; outline:none; }")
                .arg(MxFunctions::UiMetrics::choiceSize())
                .arg(MxFunctions::UiMetrics::choiceGap()));
        }

        if (m_arrowLabel)
        {
            const int arrowSlot = MxFunctions::UiMetrics::iconSize();
            m_arrowLabel->setFixedSize(arrowSlot, arrowSlot);
        }
        //默认箭头色随主题重读并重画（自定义色保持）
        if (!m_customIconColor)
            m_iconColor = MxFunctions::UiPalette::muted();
        m_iconOpen = getFoldIcon(true);
        m_iconClose = getFoldIcon(false);
        updateFoldArrow(m_btn->isChecked());

        if (m_headerWidget)
            m_headerWidget->setStyleSheet(QString("QWidget{ background:%1; border:none; }")
                .arg(MxFunctions::UiPalette::headerHex()));
        if (m_headerLayout)
            m_headerLayout->setContentsMargins(0, 0, MxFunctions::UiMetrics::fieldInset(), 0);

        //内容区底色随主题重发
        applyContentBackground();

        //标题条高随档位变化，收起高度同步重算
        m_collapsedHeight = m_btn->sizeHint().height();
    }

    //内容区底色 surface：objectName 规则只命中内容区自身（不能用 QWidget 透底
    //行——类型选择器会命中 QFrame/QTableWidget 等全部子孙，接管绘制后原生
    //分隔线/表格边框会消失）；白级联在宿主源头收窄（见 mInfoWindow）；
    //普通 QWidget 画 QSS 背景需 WA_StyledBackground；禁用态兜底随近表保留
    void FoldWidget::applyContentBackground()
    {
        if (!m_contentWidget)
            return;
        if (m_contentWidget->objectName().isEmpty())
            m_contentWidget->setObjectName("foldContentHost");
        m_contentWidget->setStyleSheet(QStringLiteral(
            "#foldContentHost{ background-color:%1; border:none; }"
            "QComboBox:disabled, QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled"
            " { background-color:%2; color:%3; }")
            .arg(MxFunctions::UiPalette::surfaceHex())
            .arg(MxFunctions::UiPalette::panelHex())
            .arg(MxFunctions::UiPalette::disabledHex()));
        m_contentWidget->setAttribute(Qt::WA_StyledBackground, true);
        QPalette contentPal = m_contentWidget->palette();
        contentPal.setColor(QPalette::Window, MxFunctions::UiPalette::surface());
        m_contentWidget->setAutoFillBackground(true);
        m_contentWidget->setPalette(contentPal);
    }

    void FoldWidget::setContentWidget(QWidget* content)
    {
        if (m_contentWidget)
        {
            m_mainLayout->removeWidget(m_contentWidget);
            m_contentWidget->deleteLater();
        }

        m_contentWidget = content;
        m_contentWidget->setParent(this);
        m_mainLayout->addWidget(m_contentWidget);

        m_contentWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        if (m_btn->isChecked())
        {
            m_contentWidget->setVisible(true); // 默认展开
            m_contentWidget->setMaximumHeight(m_contentWidget->sizeHint().height()); // 初始化为展开状态
        }
        else
        {
            m_contentWidget->setVisible(false); // 默认折叠
            m_contentWidget->setMaximumHeight(0); // 初始化为折叠状态
        }

        m_contentWidget->setMinimumHeight(0);

        //内容区底色随主题（初始）
        applyContentBackground();

        // 根据复选框状态同步内容区域的启用/禁用
        if (m_checkBox && m_checkBox->isVisible()) {
            m_contentWidget->setEnabled(m_checkBox->isChecked());
        }

    }

    void FoldWidget::setTitle(const QString& title)
    {
        if(m_btn)
        {
            m_btn->setText(title);
            m_btn->setToolTip(title);   //长标题省略时提供完整提示（规则 2.2.1）
        }
    }

    void FoldWidget::setCheckable(bool checkable)
    {
        if (!m_checkBox) return;
        if (m_checkBox->isVisible())
        {
            m_btn->setText(m_checkBox->text());
            m_checkBox->setText("");
        }
        else
        {
            m_checkBox->setText(m_btn->text());
            m_btn->setText("");
        }
        m_checkBox->setVisible(checkable);
    }

    bool FoldWidget::isCheckable() const
    {
        return m_checkBox && m_checkBox->isVisible();
    }

    bool FoldWidget::isChecked() const
    {
        return m_checkBox && m_checkBox->isChecked();
    }

    void FoldWidget::setChecked(bool checked)
    {
        if (m_checkBox) {
            m_checkBox->setChecked(checked);
        }
    }

    void FoldWidget::onCheckBoxToggled(bool checked)
    {
        if (m_contentWidget) {
            m_contentWidget->setEnabled(checked);
        }
        //emit m_btn->toggled(checked);
        m_btn->setChecked(checked);
        emit enabledChanged(checked);
    }

    bool FoldWidget::eventFilter(QObject* watched, QEvent* event)
    {
        //箭头槽点击转成标题按钮点击：标题行与箭头共用一次展开/收起（规则 2.2.1）
        if (watched == m_arrowLabel && event->type() == QEvent::MouseButtonRelease)
        {
            m_btn->click();
            return true;
        }
        return QWidget::eventFilter(watched, event);
    }

    void FoldWidget::setExpanded(bool open)
    {
        //同值时 setChecked 不发 toggled，无副作用；变值时 toggled 已接管
        //图标切换与内容显隐
        m_btn->setChecked(open);
    }

    bool FoldWidget::isExpanded() const
    {
        return m_btn->isChecked();
    }

    void FoldWidget::setCollapsible(bool on)
    {
        //收起态直接关折叠会停在收起画面：先借原通道恢复展开（此刻仍可折叠，
        //toggled -> onFoldClicked 正常走展开动画），再关开关、摘箭头
        if (!on && m_collapsible && !isExpanded())
            setExpanded(true);
        m_collapsible = on;
        if (m_btn)
            updateFoldArrow(m_btn->isChecked());
    }

    void FoldWidget::onFoldClicked(bool open, bool changeIcon)
    {
        if (!m_contentWidget)
            return;
        m_btn->blockSignals(true);
        m_btn->setChecked(open);
        m_btn->blockSignals(false);
        //折叠禁用时箭头保持隐藏（refreshLayout 等内部路径也走这里重设图标）
        if(!changeIcon)
            updateFoldArrow(open);

        int targetHeight = open ? m_contentWidget->sizeHint().height() : 0;

        if (!m_anim) {
            m_anim = new QPropertyAnimation(m_contentWidget, "maximumHeight", this);

            connect(m_anim, &QPropertyAnimation::finished, this, [this]() {
                if (!m_btn->isChecked()) {
                    m_contentWidget->setVisible(false);
                    m_contentWidget->setMaximumHeight(0);
                }
                updateGeometry();
                });
        }

        if (m_anim->state() == QAbstractAnimation::Running)
            m_anim->stop();

        m_contentWidget->setVisible(true);
        m_anim->setDuration(0);
        //m_anim->setStartValue(m_contentWidget->maximumHeight());
        m_anim->setEndValue(targetHeight);


        m_anim->start();
    }

    QIcon drawArrowIcon(bool open, const QColor& fill, const QColor& outline, int px, qreal dpr)
    {
        QPixmap pm(px * dpr, px * dpr);
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);

        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal w = px, h = px;

        QPainterPath path;
        QLine line1,line2;
        if (open) {
            // ↓ 三角
            path.moveTo(0.20 * w, 0.35 * h);
            path.lineTo(0.80 * w, 0.35 * h);
            path.lineTo(0.50 * w, 0.75 * h);

            line1.setPoints(QPoint(0.20 * w, 0.35 * h),QPoint(0.50 * w, 0.75 * h));
            line2.setPoints(QPoint(0.50 * w, 0.75 * h),QPoint(0.80 * w, 0.35 * h));
        }
        else {
            // → 三角
            path.moveTo(0.35 * w, 0.20 * h);
            path.lineTo(0.75 * w, 0.50 * h);
            path.lineTo(0.35 * w, 0.80 * h);
            line1.setPoints(QPoint(0.35 * w, 0.20 * h), QPoint(0.75 * w, 0.50 * h));
            line2.setPoints(QPoint(0.75 * w, 0.50 * h),QPoint(0.35 * w, 0.80 * h));
        }
        path.closeSubpath();

        // 填充
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(path);
       /* p.drawLine(line1);
        p.drawLine(line2);*/
        // 描边（让它不是“纯色块”，边缘更立体）
        if (outline.alpha() > 0) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(outline, 1.2 * dpr));
            p.drawPath(path);
            /*p.drawLine(line1);
            p.drawLine(line2);*/
        }

        return QIcon(pm);
    }

    QIcon FoldWidget::getFoldIcon(bool open)
    {
        qreal dpr = devicePixelRatioF();
        //规则 2.2.1：实心三角（无描边）；高分辨率位图供箭头槽平滑缩放
        return drawArrowIcon(open, m_iconColor, QColor(Qt::transparent), 64, dpr);
    }

    void FoldWidget::updateFoldArrow(bool open)
    {
        if (!m_arrowLabel)
            return;
        //折叠禁用恒展开：不显示箭头（沿用原 setIcon(空) 行为）
        if (!m_collapsible)
            m_arrowLabel->setPixmap(QPixmap());
        else
            m_arrowLabel->setPixmap(getFoldIcon(open).pixmap(QSize(64, 64)));
    }

    //--- FoldSection 行级显隐/置灰/追加块（原 PostCurve mCurveFoldSection 能力合入） ---

    void FoldSection::setRowVisible(QWidget* editor, bool on)
    {
        setRowVisibleImpl(editor, on);
        //行插回/摘除改变内容区行数，maximumHeight 需同步到新 sizeHint：
        //该高度只在 showEvent/折叠切换时重算，若不同步刷新，新行会被压进
        //旧高度里（QFormLayout 挤压可收缩控件至 0 高，仅定尺寸控件如色块
        //按钮可见），直到换页/折叠才恢复。先 invalidate 清布局缓存再取
        //sizeHint（takeRow 后缓存可能仍为旧值，收不回目标高度）
        if (m_contentWidget && m_btn->isChecked())
        {
            //activate 强制同步重算（takeRow 不失效布局缓存，sizeHint 链路
            //上的旧值会让收缩方向的高度收不回来）
            m_formLayout->invalidate();
            m_formLayout->activate();
            m_contentWidget->setMaximumHeight(m_contentWidget->sizeHint().height());
        }
    }

    void FoldSection::setRowVisibleImpl(QWidget* editor, bool on)
    {
        //QFormLayout 行隐藏只收控件高度、不收行距（每隐藏行仍摊一份
        //verticalSpacing，条件行多的勾选框上下留大空隙）；平铺的 removeWidget
        //还会残留空行并打乱 label/field 配对（实测）——隐藏必须用行级
        //takeRow 整行摘除（行高行距一并收起），显示时取出控件按逻辑行序
        //insertRow 插回原位
        const bool wasTaken = m_takenRows.contains(editor);
        if (on)
        {
            if (wasTaken)
            {
                //插回原位：目标行号 = 逻辑序中位于本行之前、且当前在布局里的行数
                const HiddenRow row = m_takenRows.take(editor);
                int pos = 0;
                for (QWidget* e : m_rowOrder)
                {
                    if (e == editor)
                        break;
                    if (!m_takenRows.contains(e))
                        ++pos;
                }
                if (row.label)
                    m_formLayout->insertRow(pos, row.label, row.editor);
                else
                    m_formLayout->insertRow(pos, row.editor);   //单控件跨列行
                if (row.label)
                    row.label->setVisible(true);
                editor->setVisible(true);
                return;
            }
            //行在布局中：行 label 与编辑器一并显示
            if (QWidget* label = m_formLayout->labelForField(editor))
                label->setVisible(true);
            editor->setVisible(true);
        }
        else
        {
            if (!wasTaken)
            {
                int row = -1;
                QFormLayout::ItemRole role = QFormLayout::LabelRole;
                m_formLayout->getWidgetPosition(editor, &row, &role);
                if (row >= 0)
                {
                    //行级摘除：takeRow 同步收缩行结构（TakeRowResult 的项取完
                    //控件即删，控件转登记、仍为本类子级随父级销毁；跨列行的
                    //控件在 fieldItem 槽，labelItem 为空）
                    QFormLayout::TakeRowResult taken = m_formLayout->takeRow(row);
                    HiddenRow hidden;
                    hidden.label = taken.labelItem ? taken.labelItem->widget() : nullptr;
                    QWidget* field = taken.fieldItem ? taken.fieldItem->widget() : nullptr;
                    delete taken.labelItem;
                    delete taken.fieldItem;
                    hidden.editor = field ? field : hidden.label;
                    hidden.label = field ? hidden.label : nullptr;
                    if (hidden.label)
                        hidden.label->setVisible(false);
                    if (hidden.editor)
                        hidden.editor->setVisible(false);
                    m_takenRows.insert(editor, hidden);
                    return;
                }
            }
            //行已摘除或不在本布局（getWidgetPosition=-1）：仅保证控件隐藏
            if (QWidget* label = m_formLayout->labelForField(editor))
                label->setVisible(false);
            editor->setVisible(false);
        }
    }

    void FoldSection::setRowEnabled(QWidget* editor, bool on)
    {
        //label 与编辑器同时启停（QLabel 不随编辑器禁用，行标签会保持黑字；
        //禁用态灰色由全局 QSS 的 :disabled 规则保证）
        if (QWidget* label = m_formLayout->labelForField(editor))
            label->setEnabled(on);
        editor->setEnabled(on);
    }

    void FoldSection::addBlock(QWidget* block)
    {
        if (!block)
            return;
        block->setParent(contentWidget());
        m_contentLayout->addWidget(block);
    }

    void FoldSection::setHeaderHeight(int heightPx)
    {
        //标题按钮样式内联（min-height），覆盖高度需整体重设；字号/左右内边距
        //仍取令牌，只覆盖高度一项
        if (!m_btn)
            return;
        m_btn->setStyleSheet(QString(
            "QPushButton{ font-size:%1px; background:transparent; text-align:left; padding:0 %2px; border:none; margin:0; min-height:%3px; outline:none; }"
            "QPushButton:hover{ background:transparent; }"
            "QPushButton:checked{ background:transparent; }"
            "QPushButton:pressed{ background:transparent; }")
            .arg(MxFunctions::UiMetrics::bodyFontSize())
            .arg(MxFunctions::UiMetrics::fieldInset())
            .arg(heightPx));
        m_collapsedHeight = m_btn->sizeHint().height();
    }

    void FoldSection::applyUiTokens()
    {
        //标题条/箭头/复选框等基类部分先刷新
        FoldWidget::applyUiTokens();
        if (!m_formLayout)
            return;

        const int a = MxFunctions::UiMetrics::fieldInset();   //左右内容边距
        const int b = MxFunctions::UiMetrics::rowGap();        //行距/上边距
        const int c = MxFunctions::UiMetrics::columnGap();     //标签列间距

        //表单边距/行距/列距随档位（2.2.1-C）；下边距取 0——最后一行贴折叠框
        //底缘，与下一个折叠框的间距正好等于面板行间距 baseG（否则视觉上
        // 变成 baseG + rowGap 两段叠加）
        m_formLayout->setContentsMargins(a, b, a, 0);
        m_formLayout->setVerticalSpacing(b);
        m_formLayout->setHorizontalSpacing(c);

        //标签列宽随档位（labelWidth = 7×body）
        const int labelW = MxFunctions::UiMetrics::labelWidth();
        const auto labels = findChildren<PanelLabel*>();
        for (PanelLabel* lbl : labels)
            lbl->setFixedWidth(labelW);

        //展开态内容区高度按新档位重算，旧 maximumHeight 会压扁/裁掉行
        if (m_contentWidget && m_btn && m_btn->isChecked())
        {
            m_formLayout->invalidate();
            m_formLayout->activate();
            m_contentWidget->setMaximumHeight(m_contentWidget->sizeHint().height());
        }
    }
}
