#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 共享折叠分组控件（CommonWidgets 库，仅依赖 Qt Widgets）
//
// 背景：原 FoldWidget/FoldSection 定义在 GUIFoundation/mPanelBase（前处理参数栏
// 使用）；PostCurve 因依赖方向（MainWindow -> PostCurve 单向）无法引用，自带了
// 一份拷贝 mCurveFoldSection，两份实现靠注释约定同步样式，长期必然漂移。
// 现下沉到本轻量库，前处理（GUIFoundation）、后处理（PostWindow，经
// GUIFoundation 传递）、曲线（PostCurve）三方参数栏共用同一实现，样式单一来源。
//
// 命名空间说明：保留历史命名空间 MechMainWindow 与原类名/成员名，既有调用点
// （含 namespace MechMainWindow { class FoldSection; } 式前向声明）零改动。
//
// 布局基线（《通用结构软件界面UI设计规则》V1.2 §2.2.1 示意 2.2.1-C
// “固定形式与统一对齐线”，取值经 UiMetrics/UiPalette 派生，不写死像素值）：
//   - 标题条：header 色通栏，高 foldHeaderHeight；标题 bodyFontSize、左右
//     内边距 fieldInset；实心三角箭头固定在最右 iconSize 槽位；
//   - 表单：内容边距 fieldInset/rowGap，行距 rowGap、标签列间距 columnGap；
//     标签列左对齐，非定宽控件列拉伸到同一右对齐线。行内间距按语义：
//     标签→控件 columnGap，控件→控件 rowGap，选项→文字 choiceGap，共框 0；
//   - 行标签：QString 版 addRow 自动创建固定宽 labelWidth 的 PanelLabel；
//   - 折叠框距窗口主体 foldSideGap 由宿主面板布局提供，本控件不自带。
// =============================================================================

#include "CommonWidgets_global.h"

//尺寸/颜色令牌来源（RebuildFunctions：尺寸派生 + 语义色）
#include "UiMetrics.h"
#include "UiPalette.h"

//QT
#include <QWidget>
#include <QColor>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QTimer>
#include <QHash>
#include <QList>
#include <qpalette.h>
#include <qpainter.h>
#include <qstyle.h>
#include <QStylePainter>


//行标签（固定宽 labelWidth，参数栏标签列统一规格，规则 2.2.1-C）
class COMMONWIDGETS_EXPORT PanelLabel : public QLabel
{
    Q_OBJECT
public:
    explicit PanelLabel(QWidget* parent = nullptr) : QLabel(parent)
    {
        this->setFixedWidth(MxFunctions::UiMetrics::labelWidth());
    }
    explicit PanelLabel(QString text, QWidget* parent = nullptr) : QLabel(parent)
    {
        this->setText(text);
        this->setFixedWidth(MxFunctions::UiMetrics::labelWidth());
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing);
        QPalette pal = palette();
        pal.setColor(QPalette::WindowText, MxFunctions::UiPalette::text());   // 改的是局部副本，不是 this 的 palette
        QRect r = contentsRect().adjusted(margin(), margin(), -margin(), -margin());

        int ind = indent();
        if (ind > 0 && !text().isEmpty()) {
            Qt::Alignment a = alignment();
            if (a & Qt::AlignLeft)    r.adjust(ind, 0, 0, 0);
            else if (a & Qt::AlignRight)  r.adjust(0, 0, -ind, 0);
        }

        int flags = alignment();
        if (wordWrap()) flags |= Qt::TextWordWrap;

        style()->drawItemText(&p, r, flags, pal, isEnabled(), text(),
            QPalette::WindowText);
    }

};

namespace MechMainWindow
{
    //折叠箭头图标自绘（展开朝下、收起朝右；px 为逻辑尺寸，dpr 缩放抗锯齿）
    COMMONWIDGETS_EXPORT QIcon drawArrowIcon(bool open, const QColor& fill, const QColor& outline, int px, qreal dpr);

    class COMMONWIDGETS_EXPORT FoldWidget : public QWidget
    {
        Q_OBJECT

    public:
        //iconColor 缺省（无效色）时箭头取 UiPalette::muted
        explicit FoldWidget(const QString& title, QWidget* parent = nullptr, const QColor& iconColor = QColor());
        ~FoldWidget() override;

        //刷新全部存活折叠组的布局令牌（切档/切主题时由 ThemeManager 经
        //注册的 applier 调用，见 mDockLayoutManager；不依赖换肤信号）
        static void applyThemeToAll();

        void setContentWidget(QWidget* content);

        QString getTitle() { return m_btn->text(); };

        void    setTitle(const QString& title);

        // 复选框相关接口
        void setCheckable(bool checkable);
        bool isCheckable() const;
        bool isChecked() const;
        void setChecked(bool checked);

        QWidget* contentWidget() const {
            return m_contentWidget;
        };
        //
        QVBoxLayout* contentLayout() const {
            return m_mainLayout;
        };

        // 保留该接口以便现有调用点读取复选框状态；
        // 新代码应优先使用 enabledChanged 信号监听勾选变化。
        QCheckBox* checkBox() const { return m_checkBox; }

        //展开/收起状态（与标题头点击等价；同值调用无副作用）
        void setExpanded(bool open);
        bool isExpanded() const;

        //折叠开关（默认可折叠）：false 时标题头点击不再收起（内容恒展开，
        //正在收起态会先恢复展开），并隐藏右侧折叠箭头，标题文字不变
        void setCollapsible(bool on);

    signals:

        void foldExpandChanged(bool expand);
        void enabledChanged(bool enabled);

    public slots:
        void onFoldClicked(bool open, bool changeIcon = false);
        void onCheckBoxToggled(bool checked);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

        //布局令牌重读（仅折叠框自身：标题条/箭头槽/表单边距行距/标签列宽，
        //不涉及组内子控件样式）；切档时经 dock applier 链触发
        virtual void applyUiTokens();

        //内容区底色 surface 随主题（objectName QSS + 透底行压掉宿主白级联）
        void applyContentBackground();

        QPushButton* m_btn;
        QCheckBox* m_checkBox = nullptr;
        QLabel* m_arrowLabel = nullptr;     //最右 iconSize 箭头槽（不随标题长度移动）
        QWidget* m_contentWidget = nullptr;
        QVBoxLayout* m_mainLayout = nullptr;
        QPropertyAnimation* m_anim = nullptr;
        QColor m_iconColor;
        QIcon m_iconOpen;
        QIcon m_iconClose;
        QWidget* m_headerWidget = nullptr;      //标题条底板
        QHBoxLayout* m_headerLayout = nullptr;  //标题条布局（右边距随档位更新）
        bool m_customIconColor = false;         //构造显式指定箭头色时切主题保持不变

        bool m_collapsible = true;  //标题头是否可点击收起（setCollapsible 关闭后内容恒展开）

        QIcon getFoldIcon(bool open);

        //同步最右箭头槽显示（折叠禁用时不显示；箭头与标题行共用一次展开/收起）
        void updateFoldArrow(bool open);

        int m_collapsedHeight = 0;  // 可根据 button 的 height 自动设置
    };

    class COMMONWIDGETS_EXPORT FoldSection : public FoldWidget
    {
        Q_OBJECT
    public:
        explicit FoldSection(const QString& title, QWidget* parent = nullptr, const QColor& iconColor = QColor(), bool defaultOpen = true)
            : FoldWidget(title, parent, iconColor)
        {
            m_btn->setChecked(defaultOpen); // 默认展开
            updateFoldArrow(defaultOpen);
            QWidget* content = new QWidget;
            //内容区 = 纵向布局（表单区 + addBlock 追加块）：块挂 VBox 层，
            //隐藏时可完全收起（QFormLayout 行隐藏仍占位，见 setRowVisible 注释）
            m_contentLayout = new QVBoxLayout(content);
            m_contentLayout->setContentsMargins(0, 0, 0, 0);
            m_contentLayout->setSpacing(0);
            QWidget* formHost = new QWidget(content);
            m_formLayout = new QFormLayout(formHost);
            m_formLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
            //弹性控件列：非定宽控件拉伸到统一右对齐线
            m_formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
            m_contentLayout->addWidget(formHost);
            setContentWidget(content);
            //表单边距/行距/列间距等令牌统一经 applyUiTokens 下发（构造与刷新同源）
            applyUiTokens();
        }

        //设置标题
        void setTitle(const QString& title)
        {
            m_btn->setText(title);
            m_btn->setToolTip(title);
        }

        //标题读取（与 getTitle 同义，供不同命名习惯的调用方）
        QString title() const { return m_btn->text(); }

        // 添加一行控件
        void addRow(QWidget* label, QWidget* editor)
        {
            if (editor)
            {
                registerRow(editor);
                m_formLayout->addRow(label, editor);
            }
            else
            {
                m_formLayout->addRow(label);
            }
        }

        // 添加一行控件（QString 标签自动创建固定宽 labelWidth 的 PanelLabel，
        // 保证三方参数栏标签列同规格）
        void addRow(const QString& labelText, QWidget* editor)
        {
            registerRow(editor);
            m_formLayout->addRow(new PanelLabel(labelText), editor);
        }

        // 添加一行控件
        void addRow(QWidget* label, QWidget* editor, int spacing)
        {
            QHBoxLayout* layout = new QHBoxLayout();
            layout->addWidget(editor);
            layout->addSpacing(spacing);
            registerRow(editor);
            m_formLayout->addRow(label, layout);

        }

        //行内并列控件间距 rowGap；columnGap 仅用于"标签→控件"关系
        void addRow(QWidget* label, QWidget* editor, QWidget *editor2)
        {
            QHBoxLayout* layout = new QHBoxLayout();
            layout->setSpacing(MxFunctions::UiMetrics::rowGap());
            layout->addWidget(editor);
            layout->addWidget(editor2);
            registerRow(editor);
            m_formLayout->addRow(label, layout);
        }

        void addRow(QWidget* label, QWidget* editor, QWidget* editor2, QWidget* editor3)
        {
            QHBoxLayout* layout = new QHBoxLayout();
            layout->setSpacing(MxFunctions::UiMetrics::rowGap());
            layout->addWidget(editor);
            layout->addWidget(editor2);
            layout->addWidget(editor3);
            registerRow(editor);
            m_formLayout->addRow(label, layout);
        }

        void addRow(QWidget* label, QWidget* editor, QWidget* editor2, QWidget* editor3, QWidget* editor4)
        {
            QHBoxLayout* layout = new QHBoxLayout();
            layout->setSpacing(MxFunctions::UiMetrics::rowGap());
            layout->addWidget(editor);
            layout->addWidget(editor2);
            layout->addWidget(editor3);
            layout->addWidget(editor4);
            registerRow(editor);
            m_formLayout->addRow(label, layout);
        }

        //三组"标签→控件"对同行：组内与组间统一 columnGap（组间再小会视觉粘连）
        void addRow(QWidget* label, QWidget* editor, QWidget* label2, QWidget* editor2, QWidget* label3, QWidget* editor3)
        {
            QHBoxLayout* layout = new QHBoxLayout();
            layout->setSpacing(MxFunctions::UiMetrics::columnGap());
            layout->addWidget(label);
            layout->addWidget(editor);
            layout->addWidget(label2);
            layout->addWidget(editor2);
            layout->addWidget(label3);
            layout->addWidget(editor3);
            m_formLayout->addRow(layout);
        }

        void addRow(QWidget* editor,QWidget* editor2, QString spacer)
        {
            QHBoxLayout* layout = new QHBoxLayout();
            layout->setSpacing(MxFunctions::UiMetrics::rowGap());

            if (spacer.compare("Left") == 0)
            {
                layout->addStretch();
            }

            layout->addWidget(editor);
            layout->addWidget(editor2);

            if (spacer.compare("Right") == 0)
            {
                layout->addStretch();
            }

            m_formLayout->addRow(layout);
        }

        void addRow(QWidget* editor, QWidget* editor2, QWidget* editor3, QString spacer)
        {
            QHBoxLayout* layout = new QHBoxLayout();
            layout->setSpacing(MxFunctions::UiMetrics::rowGap());

            if (spacer.compare("Left") == 0)
            {
                layout->addStretch();
            }

            layout->addWidget(editor);
            if (spacer.compare("mid") == 0)
            {
                layout->addStretch();
            }
            layout->addWidget(editor2);
            layout->addWidget(editor3);

            if (spacer.compare("Right") == 0)
            {
                layout->addStretch();
            }
            m_formLayout->addRow(layout);
        }

        // 添加跨列控件
        void addRow(QWidget* widget)
        {
            registerRow(widget);
            m_formLayout->addRow(widget);
        }

        void addRow(QWidget* label, QLayout* field)
        {
            m_formLayout->addRow(label, field);
        };
        void addRow(const QString& labelText, QLayout* field)
        {
            m_formLayout->addRow(new PanelLabel(labelText), field);
        };
        void addRow(QLayout* layout) { m_formLayout->addRow(layout); };

        void insertRow(int row, QWidget* label, QWidget* field) { m_formLayout->insertRow(row, label, field); };
        void insertRow(int row, QWidget* label, QLayout* field) { m_formLayout->insertRow(row, label, field); };
        void insertRow(int row, const QString& labelText, QWidget* field) { m_formLayout->insertRow(row, labelText, field); };
        void insertRow(int row, const QString& labelText, QLayout* field) { m_formLayout->insertRow(row, labelText, field); };
        void insertRow(int row, QWidget* widget) { m_formLayout->insertRow(row, widget); };
        void insertRow(int row, QLayout* layout) { m_formLayout->insertRow(row, layout); };

        void removeRow(int row) { m_formLayout->removeRow(row); };
        void removeRow(QWidget* widget) { m_formLayout->removeRow(widget); };
        void removeRow(QLayout* layout) { m_formLayout->removeRow(layout); };

        void hideRow(QWidget* label, QWidget* editor) { label->hide(); editor->hide(); };
        void hideRow(QWidget* label, QWidget* editor, QWidget* editor2) { label->hide(); editor->hide(); editor2->hide(); };
        void hideRow(QWidget* label, QWidget* editor, QWidget* editor2, QWidget* editor3) { label->hide(); editor->hide(); editor2->hide(); editor3->hide(); };
        void showRow(QWidget* label, QWidget* editor) { label->show(); editor->show(); };
        void showRow(QWidget* label, QWidget* editor, QWidget* editor2) { label->show(); editor->show(); editor2->show(); };
        void showRow(QWidget* label, QWidget* editor, QWidget* editor2, QWidget* editor3) { label->show(); editor->show(); editor2->show(); editor3->show(); };

        //行显隐（条件行联动：勾选"最小化"后才显示半径/颜色行等场景）。
        //QFormLayout 行隐藏后仍摊行距，隐藏用行级 takeRow 整行摘除，
        //显示时按 addRow 逻辑行序 insertRow 插回原位，并同步内容区
        //maximumHeight（行数变了，高度不刷新会把新行压进旧高度）
        void setRowVisible(QWidget* editor, bool on);

        //行启停（置灰联动：取消"显示标签"后标签设置行整行置灰等场景）。
        //行 label 与编辑器一并禁用——label 是 addRow 单建的控件，不随编辑器
        //禁用，只禁编辑器时行标签保持黑字，整行看起来仍可用
        void setRowEnabled(QWidget* editor, bool on);

        //整块追加区域：挂在全部表单行之后、同属内容区（随折叠显隐）。
        //用途：需要整体动态显隐的若干行——QFormLayout 的行隐藏后仍占位
        //不收起，块挂在 QVBoxLayout 层，隐藏时才完全收起（无空白残留）。
        //追加后需 refreshLayout() 刷新折叠高度
        void addBlock(QWidget* block);

        //标题头高度覆盖（默认 foldHeaderHeight；大尺寸宿主可调大）
        void setHeaderHeight(int heightPx);

        //设置图标
        void setBtnIcon(QString type)
        {
            if (type.compare("yes") == 0)
            {
                m_btn->setIcon(QIcon(":/mMainWidgets/Icon/yes.png"));
            }
            else if (type.compare("no") == 0)
            {
                m_btn->setIcon(QIcon(":/mMainWidgets/Icon/no.png"));
            }
            else if (type.compare("sel") == 0)
            {
                m_btn->setIcon(QIcon(":/mMainWidgets/Icon/sel.png"));
            }
        };

        //更改m_btn槽连接，
        void changeBtnConnect()
        {
            m_btn->disconnect();
            connect(m_btn, &QPushButton::toggled, this, [this](bool checked) {
                onFoldClicked(checked, true);
                });
        };


        //移除但不释放
        void removeAll() {
                if (!m_formLayout) return;
                while (QLayoutItem* item = m_formLayout->takeAt(0)) {
                    // 如果是 widget，先把它从父窗口脱离并隐藏（或 delete）
                    if (QWidget* w = item->widget()) {
                        w->hide();
                        w->setParent(nullptr);
                    }
                    // 如果是 spacer / layout，直接 delete
                    delete item;
                }
                m_takenRows.clear();
                m_rowOrder.clear();
        };

        //刷新该折叠块布局
        void refreshLayout() {
            QTimer::singleShot(5, this, [=]() {
                if (contentWidget()) {
                    // 先解除最大高度限制，确保按最新内容计算理想高度，避免旧值裁剪/挤压内容
                    contentWidget()->setMaximumHeight(QWIDGETSIZE_MAX);
                    m_formLayout->invalidate();
                    m_formLayout->activate();
                    contentWidget()->updateGeometry();
                    /* 只重算高度：adjustSize 会把宽度一并设为 sizeHint（组合框取
                       最长条目宽），折叠组迁入较窄宿主（后处理共享折叠组跨面板
                       接管）时行控件会超出折叠块右缘被裁切且无滚动条补偿——
                       宽度始终归父级布局管理，此处保持当前值 */
                    const int keepWidth = contentWidget()->width();
                    contentWidget()->adjustSize();
                    contentWidget()->resize(keepWidth, contentWidget()->height());
                    // 再次按最新内容重算高度，避免内容减少时仍保持旧高度导致布局空白
                    contentWidget()->setMaximumHeight(contentWidget()->sizeHint().height());
                    onFoldClicked(m_btn->isChecked());
                }
                });
        };

        // 获取内容布局（用于更复杂布局）
        QFormLayout* layout() const { return m_formLayout; }

    protected:
        //追加刷新：表单边距/行距/列距、PanelLabel 列宽、展开态内容高度
        void applyUiTokens() override;

        void showEvent(QShowEvent* e) override
        {
            // 确保高度是最新的
            if (contentWidget())
                contentWidget()->setMaximumHeight(contentWidget()->sizeHint().height());
            FoldWidget::showEvent(e);
        }

        void resizeEvent(QResizeEvent* e) override
        {
            FoldWidget::resizeEvent(e);
            /* 宽度变化可能改变行换行状态（WrapLongRows 下行数随宽度增减），
               内容区最大高度需按最新内容重算，否则窄栏换行后新增行被旧
               最大高度裁掉；同值 setMaximumHeight 无事件，不会递归 */
            if (m_btn->isChecked() && contentWidget())
                contentWidget()->setMaximumHeight(contentWidget()->sizeHint().height());
        }

    private:
        //行登记（setRowVisible 的行序依据）：编辑器 widget 逻辑行序
        void registerRow(QWidget* editor)
        {
            if (editor)
                m_rowOrder.append(editor);
        }

        //setRowVisible 的行摘除/插回实现（不含内容区高度同步）
        void setRowVisibleImpl(QWidget* editor, bool on);

        //整行移除登记（setRowVisible 隐藏用）：编辑器 widget -> 行的 label/
        //编辑器控件（控件仍为本类子级，随父级销毁）；m_rowOrder 记 addRow
        //逻辑行序，插回时据此定位原行号
        struct HiddenRow
        {
            QWidget* label = nullptr;   //单控件跨列行无 label，为 nullptr
            QWidget* editor = nullptr;
        };
        QHash<QWidget*, HiddenRow> m_takenRows;
        QList<QWidget*> m_rowOrder;

        QFormLayout* m_formLayout = nullptr;
        QVBoxLayout* m_contentLayout = nullptr;   //内容区纵向布局（表单区 + addBlock 块）
    };
}
