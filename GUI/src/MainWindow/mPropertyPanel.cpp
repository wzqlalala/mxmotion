#include "mPropertyPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"
#include "mInfoWindow.h"
#include "mCalcLineEdit.h"
#include "mGeoDataBase.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"
#include "PickCommonFunction.h"
#include "mDataStatusManager.h"
#include "mTableDataWindow.h"
#include "mDockLayoutManager.h"
#include "PlyViewportWidget.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/data/MeshElementExtract.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/basic/CameraController.h"
#include "mxr/gizmo/GizmoManager.h"
#include "mxr/view/mPlyRend.h"
#include "mxr/view/mCubeRender.h"


//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QCompleter>
#include <QTimer>
#include <QFormLayout>
#include <QDateTime>
#include <QDebug>
#include <QInputDialog>
#include <QStackedWidget>
#include <QFrame>
#include <QGridLayout>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QDesktopWidget>
#include <QRegularExpression>
#include <cmath>

//BasicData
#include "mPropertyBase.h"
#include "mPropertyManager.h"
#include "mTorsionBar.h"
#include "mTube.h"
#include "mSolidHomo.h"
#include "mShellHomo.h"
#include "mConstantBeam.h"
#include "mCrossBeam.h"
#include "mShearPlate.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mMaterialManager.h"
#include "mMaterialBase.h"
#include "mSetBase.h"
#include "mPartSet.h"
#include "mElementBase.h"
#include "mDataElement.h"
#include "mSectionShapeManager.h"
#include "mSectionShapeBase.h"
//截面形状可视化类(快速新建截面形状直接写入管理器)
#include "H_SectionVisual.h"
#include "L_SectionVisual.h"
#include "I_SectionVisual.h"
#include "I1_SectionVisual.h"
#include "T_SectionVisual.h"
#include "T1_SectionVisual.h"
#include "T2_SectionVisual.h"
#include "Z_SectionVisual.h"
#include "Box_SectionVisual.h"
#include "Box1_SectionVisual.h"
#include "Rectangle_SectionVisual.h"
#include "Circle_SectionVisual.h"
#include "Annular_SectionVisual.h"
#include "Channel_SectionVisual.h"
#include "Channel1_SectionVisual.h"
#include "Channel2_SectionVisual.h"
#include "Cross_SectionVisual.h"
#include "Hat_SectionVisual.h"
#include "mCustomSectionShape.h"
#include "mFieldManager.h"
#include "mDiscreteField.h"
#include "mSpring.h"
#include "mViscousdampsection.h"
#include "mCbush.h"
#include "mPlyLaminate.h"
#include "mGasket.h"
#include "mGapuni.h"
#include "mMembrane.h"
#include "mMacroEquivalent.h"
//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（拉 mOCCDocument→OCC 链；冻结岛保档）
#include "mOtherFunction.h"
#include "mPhysicalFieldEnum.h"

using namespace MechPreData;
using namespace MechSetData;
using namespace MechPropertyData;
using namespace MechElementData;
using namespace MechSectionShapeData;
using namespace MxFunctions;
using namespace mxr;
namespace MechMainWindow
{
    //========== 梁截面参数/梁单元参数面板辅助实现(参考交互原型) ==========

    //十八种几何形状+自定义 的元信息(顺序同原型形状选择器)
    struct BeamShapeInfo
    {
        MxFunctions::SecType type;
        QString label;
        QString icon;
    };
    static const QList<BeamShapeInfo>& beamShapeTable()
    {
        static const QList<BeamShapeInfo> table = {
            { MxFunctions::Sec_H,       QStringLiteral("H"),     QStringLiteral(":/mMainWidgets/Icon/sectionShape/H.png") },
            { MxFunctions::Sec_L,       QStringLiteral("L"),     QStringLiteral(":/mMainWidgets/Icon/sectionShape/L.png") },
            { MxFunctions::Sec_I,       QStringLiteral("I"),     QStringLiteral(":/mMainWidgets/Icon/sectionShape/I.png") },
            { MxFunctions::Sec_I1,      QStringLiteral("I1"),    QStringLiteral(":/mMainWidgets/Icon/sectionShape/I1.png") },
            { MxFunctions::Sec_T,       QStringLiteral("T"),     QStringLiteral(":/mMainWidgets/Icon/sectionShape/T.png") },
            { MxFunctions::Sec_T1,      QStringLiteral("T1"),    QStringLiteral(":/mMainWidgets/Icon/sectionShape/T1.png") },
            { MxFunctions::Sec_T2,      QStringLiteral("T2"),    QStringLiteral(":/mMainWidgets/Icon/sectionShape/T2.png") },
            { MxFunctions::Sec_Z,       QStringLiteral("Z"),     QStringLiteral(":/mMainWidgets/Icon/sectionShape/Z-3.png") },
            { MxFunctions::Sec_Box,     QStringLiteral("盒"),    QStringLiteral(":/mMainWidgets/Icon/sectionShape/he.png") },
            { MxFunctions::Sec_Box1,    QStringLiteral("盒1"),   QStringLiteral(":/mMainWidgets/Icon/sectionShape/he.png") },
            { MxFunctions::Sec_Rec,     QStringLiteral("矩形"),  QStringLiteral(":/mMainWidgets/Icon/sectionShape/juxing.png") },
            { MxFunctions::Sec_Cir,     QStringLiteral("圆"),    QStringLiteral(":/mMainWidgets/Icon/sectionShape/yuan.png") },
            { MxFunctions::Sec_Ann,     QStringLiteral("圆环"),  QStringLiteral(":/mMainWidgets/Icon/sectionShape/yuanhuan.png") },
            { MxFunctions::Sec_Channel, QStringLiteral("槽"),    QStringLiteral(":/mMainWidgets/Icon/sectionShape/cao.png") },
            { MxFunctions::Sec_Channel1,QStringLiteral("槽1"),   QStringLiteral(":/mMainWidgets/Icon/sectionShape/cao.png") },
            { MxFunctions::Sec_Channel2,QStringLiteral("槽2"),   QStringLiteral(":/mMainWidgets/Icon/sectionShape/cao1.png") },
            { MxFunctions::Sec_Cross,   QStringLiteral("十字"),  QStringLiteral(":/mMainWidgets/Icon/sectionShape/shizi.png") },
            { MxFunctions::Sec_Hat,     QStringLiteral("帽形"),  QStringLiteral(":/mMainWidgets/Icon/sectionShape/maoxing.png") },
            { MxFunctions::Sec_None,    QStringLiteral("自定义"),QStringLiteral(":/mMainWidgets/Icon/sectionShape/maoxing.png") },//仅一致截面快速新建显示(帽型图标)
        };
        return table;
    }
    static const BeamShapeInfo* beamShapeInfo(MxFunctions::SecType type)
    {
        for (const auto& info : beamShapeTable())
            if (info.type == type)
                return &info;
        return nullptr;
    }

    //截面参数区右侧图标路径(主图标资源)
    static QString sectionIconPath(MxFunctions::SecType type)
    {
        switch (type)
        {
        case MxFunctions::Sec_H: return QStringLiteral(":/mMainWidgets/Icon/H.png");
        case MxFunctions::Sec_I: return QStringLiteral(":/mMainWidgets/Icon/I.png");
        case MxFunctions::Sec_L: return QStringLiteral(":/mMainWidgets/Icon/L.png");
        case MxFunctions::Sec_T: return QStringLiteral(":/mMainWidgets/Icon/T.png");
        case MxFunctions::Sec_Box: return QStringLiteral(":/mMainWidgets/Icon/Box.png");
        case MxFunctions::Sec_Rec: return QStringLiteral(":/mMainWidgets/Icon/Rec.png");
        case MxFunctions::Sec_Cir: return QStringLiteral(":/mMainWidgets/Icon/Cir.png");
        case MxFunctions::Sec_Ann: return QStringLiteral(":/mMainWidgets/Icon/Ann.png");
        case MxFunctions::Sec_Polygon: return QStringLiteral(":/mMainWidgets/Icon/poly.png");
        case MxFunctions::Sec_Channel: return QStringLiteral(":/mMainWidgets/Icon/ch.png");
        case MxFunctions::Sec_Channel1: return QStringLiteral(":/mMainWidgets/Icon/ch1.png");
        case MxFunctions::Sec_Channel2: return QStringLiteral(":/mMainWidgets/Icon/ch2.png");
        case MxFunctions::Sec_I1: return QStringLiteral(":/mMainWidgets/Icon/I1.png");
        case MxFunctions::Sec_T1: return QStringLiteral(":/mMainWidgets/Icon/T1.png");
        case MxFunctions::Sec_T2: return QStringLiteral(":/mMainWidgets/Icon/T2.png");
        case MxFunctions::Sec_Box1: return QStringLiteral(":/mMainWidgets/Icon/Box1.png");
        case MxFunctions::Sec_Cross: return QStringLiteral(":/mMainWidgets/Icon/Cross.png");
        case MxFunctions::Sec_Hat: return QStringLiteral(":/mMainWidgets/Icon/Hat.png");
        case MxFunctions::Sec_Z: return QStringLiteral(":/mMainWidgets/Icon/Z.png");
        default: return QString();
        }
    }

    //标红/恢复输入框
    static void markLineEditInvalid(QLineEdit* edit, bool invalid)
    {
        //mCalcLineEdit编辑完成会重设自身样式表：非法时临时接管显示红框，
        //恢复时还原其原生样式，避免样式互相覆盖导致外观跳变(编辑后框变小/边距丢失)
        if (auto calcEdit = dynamic_cast<mCalcLineEdit*>(edit))
        {
            if (invalid)
                edit->setStyleSheet(QStringLiteral(
                    "QLineEdit{color:#d95151;border:1px solid #d95151;border-radius:5px;height:24px;padding:1px 6px;}"));
            else
                edit->setStyleSheet(QStringLiteral(
                    "QLineEdit{color:black;border:0.5px solid #DBE5EE;border-radius:5px;height:24px;}"));
            return;
        }
        edit->setProperty("invalid", invalid ? "true" : "false");
        edit->style()->unpolish(edit);
        edit->style()->polish(edit);
    }

    //形状选择网格(每行5个，图标+文字；可作为覆盖层或弹出菜单使用)
    class ShapeGridWidget : public QFrame
    {
    public:
        //includeCustom: 是否包含"自定义"截面(仅一致截面快速新建；变截面统一形状不包含)
        ShapeGridWidget(bool includeCustom, QWidget* parent = nullptr)
            : QFrame(parent)
        {
            setFrameShape(QFrame::NoFrame);
            setStyleSheet("QFrame{background:#ffffff;border:1px solid #dce2e9;border-radius:2px;}"
                "QToolButton{background:transparent;border:1px solid transparent;border-radius:2px;"
                "font-size:12px;color:#424b57;padding:2px;}"
                "QToolButton:hover{border-color:#91caff;background:#e8f3ff;color:#1890ff;}");
            auto grid = new QGridLayout(this);
            grid->setContentsMargins(6, 6, 6, 6);
            grid->setSpacing(4);
            int row = 0, col = 0;
            for (const auto& info : beamShapeTable())
            {
                if (!includeCustom && info.type == MxFunctions::Sec_None)
                    continue;
                auto btn = new QToolButton(this);
                btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
                btn->setIconSize(QSize(22, 22));
                btn->setIcon(QIcon(info.icon));
                btn->setText(info.label);
                btn->setFixedSize(54, 48);
                connect(btn, &QToolButton::clicked, this, [this, info]() {
                    hide();
                    if (onPick)
                        onPick(info.type);
                    });
                grid->addWidget(btn, row, col);
                if (++col == 5) { col = 0; ++row; }
            }
        }
        std::function<void(MxFunctions::SecType)> onPick;
    };

    //形状下拉弹出菜单(变截面统一形状选择)
    class ShapeGridPopup : public ShapeGridWidget
    {
    public:
        ShapeGridPopup(QWidget* parent = nullptr)
            : ShapeGridWidget(false, parent)//变截面统一形状：不含自定义
        {
            setWindowFlags(Qt::Popup);
        }
        void popupBelow(QWidget* anchor)
        {
            adjustSize();
            QPoint pos = anchor->mapToGlobal(QPoint(qMax(0, anchor->width() - width()), anchor->height() + 4));
            QRect screen = QApplication::desktop()->availableGeometry(anchor);
            if (pos.x() + width() > screen.right())
                pos.setX(screen.right() - width());
            if (pos.y() + height() > screen.bottom())
                pos.setY(qMax(screen.top(), anchor->mapToGlobal(QPoint(0, 0)).y() - height() - 4));
            move(pos);
            show();
        }
    };

    //吸附式弹窗：锚定在属性面板条目旁的浮层(标题栏+指向箭头+外部点击关闭)
    class AnchoredPopup : public QFrame
    {
    public:
        AnchoredPopup(const QString& title, QWidget* content, QWidget* owner)
            : QFrame(owner), _owner(owner), _content(content)
        {
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
            setAttribute(Qt::WA_ShowWithoutActivating, false);
            setObjectName(QStringLiteral("anchoredPopup"));
            setFrameShape(QFrame::NoFrame);
            setStyleSheet("QFrame#anchoredPopup{background:#ffffff;border:1px solid #91caff;border-radius:3px;}");
            auto root = new QVBoxLayout(this);
            root->setContentsMargins(0, 0, 0, 0);
            root->setSpacing(0);
            //标题栏(蓝色竖条+标题+关闭)
            auto header = new QWidget(this);
            header->setFixedHeight(36);
            _headerLay = new QHBoxLayout(header);
            _headerLay->setContentsMargins(10, 0, 4, 0);
            _headerLay->setSpacing(8);
            auto accent = new QFrame(header);
            accent->setFixedSize(3, 14);
            accent->setStyleSheet("background:#1890ff;border-radius:1px;");
            _titleLabel = new QLabel(title, header);
            QFont titleFont = _titleLabel->font();
            titleFont.setBold(true);
            _titleLabel->setFont(titleFont);
            auto closeBtn = new QToolButton(header);
            closeBtn->setText(QStringLiteral("×"));
            closeBtn->setFixedSize(24, 24);
            closeBtn->setToolTip(QStringLiteral("关闭"));
            closeBtn->setStyleSheet("QToolButton{border:0;background:transparent;color:#788391;font-size:16px;}"
                "QToolButton:hover{color:#424b57;background:#f5f7fa;border-radius:2px;}");
            _headerLay->addWidget(accent);
            _headerLay->addWidget(_titleLabel);
            _headerLay->addStretch();
            _headerLay->addWidget(closeBtn);
            //内容直接放入(无滚动区)：弹窗高度随内容自适应，宽度固定
            root->addWidget(header);
            root->addWidget(content, 1);
            connect(closeBtn, &QToolButton::clicked, this, [this]() { requestClose(); });
            qApp->installEventFilter(this);
        }
        ~AnchoredPopup() override
        {
            qApp->removeEventFilter(this);
        }

        //设置锚定条目(定位与箭头指向)
        void setAnchor(QWidget* anchor) { _anchor = anchor; }
        //期望宽度(内容布局按此宽度计算)
        void setPreferredWidth(int width) { _preferredWidth = width; }

        //动态修改标题
        void setTitle(const QString& title) { _titleLabel->setText(title); }

        //标题栏追加信息标签(如"N 行")，返回标签供外部更新
        QLabel* addHeaderInfo(const QString& text)
        {
            auto label = new QLabel(text, this);
            label->setStyleSheet("color:#788391;font-size:11px;border:0;background:transparent;");
            _headerLay->insertWidget(_headerLay->count() - 1, label);
            return label;
        }

        //在标题最左显示返回按钮(如梁单元参数弹窗)
        void enableBackButton(const std::function<void()>& onBack)
        {
            auto backBtn = new QToolButton(this);
            backBtn->setText(QStringLiteral("‹"));
            backBtn->setFixedSize(24, 24);
            backBtn->setToolTip(QStringLiteral("返回"));
            backBtn->setStyleSheet("QToolButton{border:0;background:transparent;color:#1890ff;font-size:18px;}"
                "QToolButton:hover{background:#f5f7fa;border-radius:2px;}");
            _headerLay->insertWidget(0, backBtn);
            connect(backBtn, &QToolButton::clicked, this, [this, onBack]() { if (onBack) onBack(); });
        }

        //按内容重新计算尺寸：宽度按布局最小尺寸(表格可见列宽之和)自适应，可增可减
        void fitToContent(bool keepPreferredWidth = true)
        {
            if (_anchor == nullptr)
                return;
            _content->layout()->invalidate();
            _content->layout()->activate();
            int width = (keepPreferredWidth && _preferredWidth > 0) ? _preferredWidth
                : _content->layout()->totalMinimumSize().width() + 2;
            int maxHeight = QApplication::desktop()->availableGeometry(_anchor).height() - 24;
            //高度随内容自适应(可增可减)，仅受屏幕高度封顶
            int height = qBound(120, _content->sizeHint().height() + 40, maxHeight);
            //宽高均硬性锁定(最小=最大)：布局重算/挂起删除不再把窗口顶回旧尺寸
            setMinimumWidth(width);
            setMaximumWidth(width);
            setMinimumHeight(height);
            setMaximumHeight(height);
            int left = geometry().left();
            int top = geometry().top();
            QRect screen = QApplication::desktop()->availableGeometry(_anchor);
            if (left + width > screen.right())
                left = screen.right() - width;
            if (top + height > screen.bottom())
                top = screen.bottom() - height;
            setGeometry(left, top, width, height);
        }
        //立即适配 + 挂起删除与排队布局事件落定后再复核，解决"第一次变化不生效"的问题
        void fitToContentDeferred(bool keepPreferredWidth = true)
        {
            fitToContent(keepPreferredWidth);
            QTimer::singleShot(0, this, [this, keepPreferredWidth]() {
                if (!isVisible())
                    return;
                //先强制处理挂起的deleteLater，保证测量时旧控件已销毁
                QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
                fitToContent(keepPreferredWidth);
                QTimer::singleShot(0, this, [this, keepPreferredWidth]() {
                    if (isVisible())
                        fitToContent(keepPreferredWidth);
                    });
                });
        }

        //打开(外部回调：allowClose 返回 false 阻止关闭并提示；keepOpen 返回 true 时外部点击不关闭，如拾取中)
        std::function<bool()> allowClose;
        std::function<bool()> keepOpen;
        std::function<void()> onClosed;

        void popup()
        {
            if (_anchor == nullptr)
                return;
            int maxHeight = QApplication::desktop()->availableGeometry(_anchor).height() - 24;
            //宽度：固定期望宽度；未设置时按布局最小尺寸(如表格可见列宽之和)自适应，可增可减
            _content->layout()->invalidate();
            _content->layout()->activate();
            int width = _preferredWidth > 0 ? _preferredWidth
                : _content->layout()->totalMinimumSize().width() + 2;
            int height = qBound(120, _content->sizeHint().height() + 40, maxHeight);
            //宽高均硬性锁定(最小=最大)，防止内容布局的最小宽度撑大窗口
            setMinimumWidth(width);
            setMaximumWidth(width);
            setMinimumHeight(height);
            setMaximumHeight(height);
            resize(width, height);
            //优先显示在条目右侧，空间不足则左侧
            QPoint anchorTopRight = _anchor->mapToGlobal(QPoint(_anchor->width(), 0));
            QPoint pos(anchorTopRight.x() + 10, anchorTopRight.y() - 5);
            QRect screen = QApplication::desktop()->availableGeometry(_anchor);
            if (pos.x() + width > screen.right())
            {
                pos.setX(_anchor->mapToGlobal(QPoint(0, 0)).x() - width - 10);
                _arrowOnLeft = false;
            }
            else
            {
                _arrowOnLeft = true;
            }
            if (pos.y() + height > screen.bottom())
                pos.setY(screen.bottom() - height);
            if (pos.y() < screen.top())
                pos.setY(screen.top());
            move(pos);
            show();
            raise();
            //不调用 activateWindow()：弹窗为 Qt::Tool 窗口，抢激活会触发 MDI 空激活，
            //导致当前视口模型被清空(getCVM 变空)、全局交互失效
            //显示后布局完全稳定再复核一次高度，消除边缘误差
            QTimer::singleShot(0, this, [this]() {
                if (isVisible())
                    fitToContent(true);
                });
        }

        bool requestClose()
        {
            if (!isVisible())
                return true;
            if (allowClose && !allowClose())
                return false;
            hide();
            if (onClosed)
                onClosed();
            restoreFocus();
            return true;
        }
        //关闭后把焦点交还主窗口/入口条目：Qt::Tool窗口隐藏时若焦点滞留，
        //主窗口的下一次点击会被用于恢复焦点而"吃掉"，表现为按钮点不动
        void restoreFocus()
        {
            if (_owner != nullptr)
                _owner->activateWindow();
            if (_anchor != nullptr)
                _anchor->setFocus(Qt::OtherFocusReason);
        }

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override
        {
            if (!isVisible() || event->type() != QEvent::MouseButtonPress)
                return QFrame::eventFilter(watched, event);
            //注：应用级过滤器对一次点击会收到两次通知(QWindow 与 QWidget 各一次)，
            //因此按全局坐标判断，避免依赖事件目标类型
            auto mouse = static_cast<QMouseEvent*>(event);
            QPoint globalPos = mouse->globalPos();
            //弹窗自身矩形内不关闭
            if (geometry().contains(globalPos))
                return QFrame::eventFilter(watched, event);
            //组合框下拉/形状菜单等 Popup 弹层内不关闭
            for (QWidget* top : QApplication::topLevelWidgets())
            {
                if ((top->windowFlags() & Qt::Popup) && top->geometry().contains(globalPos))
                    return QFrame::eventFilter(watched, event);
            }
            //锚定条目(入口按钮)内不关闭
            if (_anchor != nullptr && _anchor->isVisible())
            {
                QRect anchorRect(_anchor->mapToGlobal(QPoint(0, 0)), _anchor->size());
                if (anchorRect.contains(globalPos))
                    return QFrame::eventFilter(watched, event);
            }
            //其余视为外部点击(拾取等场景 keepOpen 时保留)
            if (!(keepOpen && keepOpen()))
                requestClose();
            return QFrame::eventFilter(watched, event);
        }
        void paintEvent(QPaintEvent* event) override
        {
            QFrame::paintEvent(event);
            if (!_arrowOnLeft)
                return;
            //左侧指向锚定条目的小箭头(旋转45°方块)
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(QColor(0x91, 0xca, 0xff), 1));
            painter.setBrush(QColor(0xff, 0xff, 0xff));
            QPolygonF arrow;
            arrow << QPointF(0, 18) << QPointF(9, 13) << QPointF(9, 23);
            painter.drawPolygon(arrow);
        }
    private:
        QWidget* _owner = nullptr;
        QWidget* _content = nullptr;
        QWidget* _anchor = nullptr;
        QScrollArea* _scroll = nullptr;
        QLabel* _titleLabel = nullptr;
        QHBoxLayout* _headerLay = nullptr;
        int _preferredWidth = 500;
        bool _arrowOnLeft = true;
    };


    //自定义截面24项参数标签(与截面形状窗口保持一致)
    static const char* const kCustomSectionLabels[24] = {
        "截面面积A","惯性矩Ixy","惯性矩Ixz","扭转矩J","剪切系数K1","剪切系数K2","惯性积I12","非结构质量NSM",
        "应力恢复点C1","应力恢复点C2","应力恢复点D1","应力恢复点D2","应力恢复点E1","应力恢复点E2","应力恢复点F1","应力恢复点F2",
        "A端非结构质量质心y","A端非结构质量质心z","B端非结构质量质心y","B端非结构质量质心z",
        "A端中性轴y","A端中性轴z","B端中性轴y","B端中性轴z"
    };

    //梁截面参数编辑区：左侧参数列(几何参数/自定义三列) + 右侧截面图标，图标区可被快速新建网格覆盖
    class BeamSectionStage : public QWidget
    {
    public:
        explicit BeamSectionStage(QWidget* parent = nullptr) : QWidget(parent)
        {
            auto root = new QHBoxLayout(this);
            root->setContentsMargins(0, 0, 0, 0);
            root->setSpacing(10);
            _paramsHost = new QWidget(this);
            _paramsLayout = new QVBoxLayout(_paramsHost);
            _paramsLayout->setContentsMargins(0, 0, 0, 0);
            _paramsLayout->setSpacing(8);
            root->addWidget(_paramsHost, 2);
            _frame = new QFrame(this);
            _frame->setStyleSheet("QFrame{background:#ffffff;border:1px solid #e4e8ee;border-radius:2px;}");
            _frameLayout = new QGridLayout(_frame);
            _frameLayout->setContentsMargins(0, 0, 0, 0);
            _iconLabel = new QLabel(_frame);
            _iconLabel->setAlignment(Qt::AlignCenter);
            _iconLabel->setMinimumHeight(150);
            _frameLayout->addWidget(_iconLabel, 0, 0);
            root->addWidget(_frame, 3);
        }

        //展示某个截面形状的参数与图标(nullptr则清空)
        void showSection(MechSectionShapeData::mSectionShapeBase* shape)
        {
            _shape = shape;
            hideQuickCreate();
            clearParams();
            if (shape == nullptr)
            {
                _frame->hide();
                return;
            }
            updateIcon(shape);
            if (shape->getSectionShapeType() == MxFunctions::Sec_None)
            {
                _frame->hide();
                buildCustomParams(shape);
            }
            else
            {
                _frame->show();
                buildGeometryParams(shape);
            }
        }

        //在图纸区覆盖显示快速新建网格
        void showQuickCreate()
        {
            hideQuickCreate();
            _picker = new ShapeGridWidget(true, _frame);//一致截面快速新建：含自定义
            _picker->setStyleSheet("QFrame{background:#ffffff;}"
                "QToolButton{background:transparent;border:1px solid transparent;border-radius:2px;"
                "font-size:12px;color:#424b57;padding:2px;}"
                "QToolButton:hover{border-color:#91caff;background:#e8f3ff;color:#1890ff;}");
            _picker->onPick = [this](MxFunctions::SecType type) {
                hideQuickCreate();
                if (onCreateShape)
                    onCreateShape(type);
                };
            _frame->show();//自定义截面时也显示图纸框以承载新建网格
            _frameLayout->addWidget(_picker, 0, 0);
            _picker->raise();
            _picker->show();
            _picker->setFocus();
        }
        void hideQuickCreate()
        {
            if (_picker)
            {
                //同步从布局移除并隐藏：仅 deleteLater 时它仍占着布局高度，
                //随后的高度自适应(含延迟复核)测到的仍是撑高后的尺寸，留下空白
                _frameLayout->removeWidget(_picker);
                _picker->hide();
                _picker->deleteLater();
                _picker = nullptr;
            }
            //未选择截面或自定义截面不显示图标框：取消快速新建后恢复隐藏，
            //否则showQuickCreate强制显示的框会残留成空白矩形
            if (_shape == nullptr || _shape->getSectionShapeType() == MxFunctions::Sec_None)
                _frame->hide();
        }
        bool isQuickCreateVisible() const { return _picker != nullptr; }

        //校验参数输入；返回第一个错误描述(不含单元号)
        bool validateInputs(QString* firstError, QString* dimKey)
        {
            for (auto edit : _edits)
            {
                QString key = edit->property("dimKey").toString();
                QString text = edit->text().trimmed();
                double value = text.toDouble();
                bool invalid = false;
                QString msg;
                if (text.isEmpty() || !std::isfinite(value))
                {
                    invalid = true;
                    msg = QStringLiteral("请输入有效数值");
                }
                else if (edit->property("positive").toBool() && value <= 0)
                {
                    invalid = true;
                    msg = QStringLiteral("须大于 0");
                }
                else if (edit->property("nonNegative").toBool() && value < 0)
                {
                    invalid = true;
                    msg = QStringLiteral("不能小于 0");
                }
                markLineEditInvalid(edit, invalid);
                if (invalid && firstError && firstError->isEmpty())
                {
                    *firstError = QStringLiteral("%1：%2").arg(keyLabel(key)).arg(msg);
                    if (dimKey)
                        *dimKey = key;
                }
            }
            return firstError->isEmpty();
        }
        QLineEdit* firstInvalidEdit()
        {
            for (auto edit : _edits)
                if (edit->property("invalid").toString() == "true")
                    return edit;
            return nullptr;
        }

        std::function<void(MxFunctions::SecType)> onCreateShape;   //快速新建回调
        std::function<void()> onParamCommit;          //参数提交(刷新渲染等)

        MechSectionShapeData::mSectionShapeBase* section() const { return _shape; }

    private:
        QWidget* _paramsHost = nullptr;
        QVBoxLayout* _paramsLayout = nullptr;
        QFrame* _frame = nullptr;
        QGridLayout* _frameLayout = nullptr;
        QLabel* _iconLabel = nullptr;
        ShapeGridWidget* _picker = nullptr;
        MechSectionShapeData::mSectionShapeBase* _shape = nullptr;
        QList<QLineEdit*> _edits;

        //右侧显示当前截面形状的图标(使用主图标资源)
        void updateIcon(MechSectionShapeData::mSectionShapeBase* shape)
        {
            QPixmap pixmap(sectionIconPath(shape->getSectionShapeType()));
            _iconLabel->setPixmap(pixmap.scaled(QSize(200, 150), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        QString keyLabel(const QString& key) const
        {
            if (key == QStringLiteral("NSM"))
                return QStringLiteral("NSM");
            bool isDim = key.startsWith(QStringLiteral("DIM"));
            if (isDim)
                return key;
            if (key.startsWith(QStringLiteral("C")))
                return QStringLiteral("自定义参数 ") + key;
            return key;
        }
        void clearParams()
        {
            _edits.clear();
            while (QLayoutItem* item = _paramsLayout->takeAt(0))
            {
                if (QWidget* w = item->widget())
                    w->deleteLater();
                delete item;
            }
        }
        QLineEdit* makeParamEdit(const QString& text, bool positive, bool nonNegative)
        {
            //不覆盖mCalcLineEdit自身样式：它在编辑完成后会重设样式表，
            //外部设置的样式被整段替换会造成外观跳变(框变小/左边距丢失)
            auto edit = new mCalcLineEdit(text, this, false, false, nonNegative, positive);
            edit->setProperty("positive", positive);
            edit->setProperty("nonNegative", nonNegative);
            return edit;
        }
        void bindEdit(QLineEdit* edit, const QString& dimKey)
        {
            edit->setProperty("dimKey", dimKey);
            edit->setProperty("invalid", "false");
            _edits.append(edit);
            connect(edit, &QLineEdit::editingFinished, this, [this, edit, dimKey]() {
                commitEdit(edit, dimKey);
                });
            connect(edit, &QLineEdit::textChanged, this, [this, edit](const QString&) {
                if (edit->property("invalid").toString() == "true")
                    markLineEditInvalid(edit, false);
                });
        }
        void commitEdit(QLineEdit* edit, const QString& dimKey)
        {
            if (_shape == nullptr)
                return;
            double value = edit->text().trimmed().toDouble();
            if (dimKey == QStringLiteral("NSM"))
            {
                _shape->setNsm(edit->text().trimmed());
            }
            else if (dimKey.startsWith(QStringLiteral("DIM")))
            {
                int index = dimKey.mid(3).toInt() - 1;
                QList<double> para = _shape->getParaNoType();
                if (index >= 0 && index < para.size())
                {
                    para[index] = value;
                    _shape->setPara(para);
                }
            }
            else if (dimKey.startsWith(QStringLiteral("CP")))
            {
                auto custom = dynamic_cast<mCustomSectionShape*>(_shape);
                if (custom)
                {
                    QList<QString> para = custom->getCustomPara();
                    int index = dimKey.mid(2).toInt() - 1;
                    if (index >= 0 && index < para.size())
                    {
                        para[index] = edit->text().trimmed();
                        custom->setCustomPara(para);
                    }
                }
            }
            if (onParamCommit)
                onParamCommit();
        }
        void buildGeometryParams(MechSectionShapeData::mSectionShapeBase* shape)
        {
            QList<double> para = shape->getParaNoType();
            for (int i = 0; i < para.size(); ++i)
            {
                QString key = QStringLiteral("DIM%1").arg(i + 1);
                auto row = new QWidget(_paramsHost);
                auto lay = new QHBoxLayout(row);
                lay->setContentsMargins(0, 0, 0, 0);
                lay->setSpacing(6);
                auto label = new QLabel(key, row);
                label->setFixedWidth(42);
                auto edit = makeParamEdit(mNumberToString(para[i]), true, false);
                bindEdit(edit, key);
                lay->addWidget(label);
                lay->addWidget(edit);
                _paramsLayout->addWidget(row);
            }
            //NSM 与几何参数之间加分隔
            auto nsmRow = new QWidget(_paramsHost);
            auto nsmLay = new QVBoxLayout(nsmRow);
            nsmLay->setContentsMargins(0, 4, 0, 0);
            nsmLay->setSpacing(4);
            auto separator = new QFrame(nsmRow);
            separator->setFrameShape(QFrame::HLine);
            separator->setStyleSheet("color:#e4e8ee;");
            nsmLay->addWidget(separator);
            auto field = new QWidget(nsmRow);
            auto fieldLay = new QHBoxLayout(field);
            fieldLay->setContentsMargins(0, 0, 0, 0);
            fieldLay->setSpacing(6);
            auto nsmLabel = new QLabel(QStringLiteral("NSM"), field);
            nsmLabel->setFixedWidth(42);
            QString nsmText = shape->getNsm().isEmpty() ? QStringLiteral("0") : shape->getNsm();
            auto nsmEdit = makeParamEdit(nsmText, false, true);
            bindEdit(nsmEdit, QStringLiteral("NSM"));
            fieldLay->addWidget(nsmLabel);
            fieldLay->addWidget(nsmEdit);
            nsmLay->addWidget(field);
            _paramsLayout->addWidget(nsmRow);
            _paramsLayout->addStretch();
        }
        void buildCustomParams(MechSectionShapeData::mSectionShapeBase* shape)
        {
            auto custom = dynamic_cast<mCustomSectionShape*>(shape);
            if (!custom)
                return;
            QList<QString> para = custom->getCustomPara();
            while (para.size() < 24)
                para.append(QStringLiteral("0"));
            auto columns = new QWidget(_paramsHost);
            auto columnsLay = new QHBoxLayout(columns);
            columnsLay->setContentsMargins(0, 0, 0, 0);
            columnsLay->setSpacing(6);
            for (int col = 0; col < 3; ++col)
            {
                auto columnHost = new QWidget(columns);
                auto colLay = new QGridLayout(columnHost);
                colLay->setContentsMargins(0, 0, 0, 0);
                colLay->setHorizontalSpacing(4);
                colLay->setVerticalSpacing(6);
                for (int rowInCol = 0; rowInCol < 8; ++rowInCol)
                {
                    int index = col * 8 + rowInCol;
                    auto label = new QLabel(QString::fromUtf8(kCustomSectionLabels[index]), columnHost);
                    //标签允许收缩(宽度不足时截断)，避免三列参数的最小宽度把弹窗撑宽
                    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
                    label->setMinimumWidth(0);
                    label->setToolTip(QString::fromUtf8(kCustomSectionLabels[index]));
                    auto edit = makeParamEdit(para[index], index == 0, index >= 1 && index <= 7);
                    edit->setMinimumWidth(36);
                    bindEdit(edit, QStringLiteral("CP%1").arg(index + 1));
                    colLay->addWidget(label, rowInCol, 0);
                    colLay->addWidget(edit, rowInCol, 1);
                }
                columnsLay->addWidget(columnHost, 1);
            }
            _paramsLayout->addWidget(columns);
            _paramsLayout->addStretch();
        }
    };

    //方向草稿与解析(参考原型编辑区的方向定义)
    struct DirectionDraft
    {
        QString mode = QStringLiteral("vector");  //node/x/y/z/points/object/vector
        bool reversed = false;
        QString node;                                             //参考节点编号
        QString startId;
        QString startXYZ = QStringLiteral("0, 0, 0");
        QString endId;
        QString endXYZ = QStringLiteral("1, 0, 0");
        QString objectId;                                         //方向对象编号
        QVector3D objectDir;
        QString vector = QStringLiteral("0, 1, 0");
    };
    static bool parseBeamVector(const QString& raw, double* xyz)
    {
        QString text = raw.trimmed();
        if (!text.isEmpty() && (text.front() == QChar('(') || text.front() == QChar(u'（')))
            text.remove(0, 1);
        if (!text.isEmpty() && (text.back() == QChar(')') || text.back() == QChar(u'）')))
            text.chop(1);
        text.replace(QChar(u'，'), QChar(','));
        QStringList parts = text.contains(QChar(','))
            ? text.split(QChar(','), QString::SkipEmptyParts)
            : text.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        if (parts.size() != 3)
            return false;
        for (int i = 0; i < 3; ++i)
        {
            bool ok = false;
            double value = parts[i].trimmed().toDouble(&ok);
            if (!ok)
                return false;
            xyz[i] = value;
        }
        return true;
    }
    static QString formatBeamVector(const double* xyz)
    {
        return QString("%1, %2, %3")
            .arg(QString::number(xyz[0], 'g', 12))
            .arg(QString::number(xyz[1], 'g', 12))
            .arg(QString::number(xyz[2], 'g', 12));
    }
    static QString resolveBeamDirection(const DirectionDraft& draft, bool* ok)
    {
        *ok = true;
        double xyz[3] = { 0, 0, 0 };
        if (draft.mode == QStringLiteral("x"))
        {
            xyz[0] = 1;
        }
        else if (draft.mode == QStringLiteral("y"))
        {
            xyz[1] = 1;
        }
        else if (draft.mode == QStringLiteral("z"))
        {
            xyz[2] = 1;
        }
        else if (draft.mode == QStringLiteral("points"))
        {
            double a[3], b[3];
            if (!parseBeamVector(draft.startXYZ, a) || !parseBeamVector(draft.endXYZ, b))
            {
                *ok = false;
                return QString();
            }
            xyz[0] = b[0] - a[0];
            xyz[1] = b[1] - a[1];
            xyz[2] = b[2] - a[2];
        }
        else if (draft.mode == QStringLiteral("object"))
        {
            xyz[0] = draft.objectDir.x();
            xyz[1] = draft.objectDir.y();
            xyz[2] = draft.objectDir.z();
        }
        else
        {
            if (!parseBeamVector(draft.vector, xyz))
            {
                *ok = false;
                return QString();
            }
        }
        if (draft.reversed)
        {
            xyz[0] = -xyz[0];
            xyz[1] = -xyz[1];
            xyz[2] = -xyz[2];
        }
        return formatBeamVector(xyz);
    }

    //通用方向定义控件：方式下拉+反向按钮+(节点拾取/起点终点/选择对象/向量输入)
    class DirectionControl : public QWidget
    {
    public:
        DirectionControl(bool allowNode, QWidget* parent = nullptr) : QWidget(parent), _allowNode(allowNode)
        {
            auto root = new QVBoxLayout(this);
            root->setContentsMargins(0, 0, 0, 0);
            root->setSpacing(6);
            //首行：方式下拉框 + 反向图标按钮
            auto head = new QHBoxLayout();
            head->setSpacing(6);
            _modeCombo = new QComboBox(this);
            if (allowNode)
                _modeCombo->addItem(QStringLiteral("节点"), QStringLiteral("node"));
            _modeCombo->addItem(QStringLiteral("X轴"), QStringLiteral("x"));
            _modeCombo->addItem(QStringLiteral("Y轴"), QStringLiteral("y"));
            _modeCombo->addItem(QStringLiteral("Z轴"), QStringLiteral("z"));
            _modeCombo->addItem(QStringLiteral("起点-终点"), QStringLiteral("points"));
            _modeCombo->addItem(QStringLiteral("选择对象"), QStringLiteral("object"));
            _modeCombo->addItem(QStringLiteral("向量"), QStringLiteral("vector"));
            _modeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            _reverseBtn = new QToolButton(this);
            _reverseBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/fanxiang1@3x.png")));
            _reverseBtn->setIconSize(QSize(20, 20));
            _reverseBtn->setCheckable(true);
            _reverseBtn->setToolTip(QStringLiteral("反向"));
            _reverseBtn->setStyleSheet(QStringLiteral(
                "QToolButton{border:1px solid #dce2e9;border-radius:2px;background:#ffffff;}"
                "QToolButton:hover{border-color:#91caff;}"
                "QToolButton:checked{background:#e8f3ff;border-color:#1890ff;}"));
            head->addWidget(_modeCombo, 1);
            head->addWidget(_reverseBtn);
            root->addLayout(head);
            //次行："方向向量"标签 + 输入框占满剩余空间(仅"向量"方式显示，随输入框显隐)
            auto vecRow = new QHBoxLayout();
            vecRow->setSpacing(6);
            _vectorLabel = new QLabel(QStringLiteral("方向向量"), this);
            _vectorLabel->setStyleSheet("color:#788391;border:0;background:transparent;");
            vecRow->addWidget(_vectorLabel);
            _vectorEdit = new QLineEdit(this);
            _vectorEdit->setPlaceholderText(QStringLiteral("x, y, z"));
            _vectorEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            vecRow->addWidget(_vectorEdit, 1);
            root->addLayout(vecRow);
            //其余方式的附加控件：普通布局+显隐切换(QStackedWidget会按最大页预留高度，形成大片空白)
            _pagesLayout = new QVBoxLayout();
            _pagesLayout->setContentsMargins(0, 0, 0, 0);
            _pagesLayout->setSpacing(6);
            //节点页
            {
                auto page = new QWidget(this);
                auto lay = new QHBoxLayout(page);
                lay->setContentsMargins(0, 0, 0, 0);
                lay->setSpacing(6);
                _nodePickBtn = new QPushButton(QStringLiteral("请选择"), page);
                _nodePickBtn->setStyleSheet("color:#1890ff;");
                _nodePickBtn->setToolTip(QStringLiteral("拾取节点"));
                auto clearBtn = new QPushButton(QStringLiteral("×"), page);
                clearBtn->setFixedSize(24, 24);
                clearBtn->setToolTip(QStringLiteral("清除参考节点"));
                lay->addWidget(_nodePickBtn, 1);
                lay->addWidget(clearBtn);
                _pagesLayout->addWidget(page);
                _nodePage = page;
                connect(_nodePickBtn, &QPushButton::clicked, this, [this]() { startNodePick(); });
                connect(clearBtn, &QPushButton::clicked, this, [this]() {
                    _draft.node.clear();
                    syncNodeButton();
                    markChanged();
                    });
            }
            //起点-终点页
            {
                auto page = new QWidget(this);
                auto lay = new QVBoxLayout(page);
                lay->setContentsMargins(0, 0, 0, 0);
                lay->setSpacing(6);
                for (int i = 0; i < 2; ++i)
                {
                    auto row = new QWidget(page);
                    auto rowLay = new QHBoxLayout(row);
                    rowLay->setContentsMargins(0, 0, 0, 0);
                    rowLay->setSpacing(6);
                    auto label = new QLabel(i == 0 ? QStringLiteral("起点") : QStringLiteral("终点"), row);
                    label->setFixedWidth(32);
                    auto pickBtn = new QPushButton(QStringLiteral("请选择"), row);
                    pickBtn->setToolTip(i == 0 ? QStringLiteral("拾取起点节点") : QStringLiteral("拾取终点节点"));
                    auto applyBtn = new QPushButton(QStringLiteral("应用"), row);
                    auto edit = new QLineEdit(row);
                    edit->setPlaceholderText(QStringLiteral("x, y, z"));
                    rowLay->addWidget(label);
                    rowLay->addWidget(pickBtn);
                    rowLay->addWidget(applyBtn);
                    rowLay->addWidget(edit, 1);
                    lay->addWidget(row);
                    bool isStart = (i == 0);
                    connect(pickBtn, &QPushButton::clicked, this, [this, isStart, pickBtn]() {
                        startNodePick([this, isStart, pickBtn](int id, const QVector3D&) {
                            double xyz[3] = { 0, 0, 0 };
                            QString current = isStart ? _draft.startXYZ : _draft.endXYZ;
                            parseBeamVector(current, xyz);
                            if (isStart)
                            {
                                _draft.startId = QString::number(id);
                                _draft.startXYZ = formatBeamVector(xyz);
                            }
                            else
                            {
                                _draft.endId = QString::number(id);
                                _draft.endXYZ = formatBeamVector(xyz);
                            }
                            pickBtn->setText(QStringLiteral("节点 ") + QString::number(id));
                            markChanged();
                            });
                        });
                    connect(applyBtn, &QPushButton::clicked, this, [this, isStart]() {
                        //坐标已通过 textChanged 更新，此处仅触发变更通知
                        if (isStart)
                            _draft.startId.clear();
                        else
                            _draft.endId.clear();
                        markChanged();
                        });
                    connect(edit, &QLineEdit::textChanged, this, [this, isStart](const QString& text) {
                        if (_syncing)
                            return;
                        if (isStart)
                            _draft.startXYZ = text;
                        else
                            _draft.endXYZ = text;
                        });
                    if (isStart)
                    {
                        _startPickBtn = pickBtn;
                        _startEdit = edit;
                    }
                    else
                    {
                        _endPickBtn = pickBtn;
                        _endEdit = edit;
                    }
                }
                _pagesLayout->addWidget(page);
                _pointsPage = page;
            }
            //选择对象页
            {
                auto page = new QWidget(this);
                auto lay = new QHBoxLayout(page);
                lay->setContentsMargins(0, 0, 0, 0);
                _objectPickBtn = new QPushButton(QStringLiteral("请选择"), page);
                _objectPickBtn->setToolTip(QStringLiteral("拾取直线/边作为方向"));
                lay->addWidget(_objectPickBtn, 1);
                _pagesLayout->addWidget(page);
                _objectPage = page;
                connect(_objectPickBtn, &QPushButton::clicked, this, [this]() { startEdgePick(); });
            }
            root->addLayout(_pagesLayout);
            connect(_vectorEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
                if (_syncing)
                    return;
                _draft.vector = text;
                });
            connect(_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
                if (_syncing)
                    return;
                _draft.mode = _modeCombo->currentData().toString();
                syncPage();
                markChanged();
                });
            connect(_reverseBtn, &QToolButton::clicked, this, [this]() {
                _draft.reversed = _reverseBtn->isChecked();
                markChanged();
                });
        }
        ~DirectionControl() override
        {
            stopPicks();
        }
        void setDraft(const DirectionDraft& draft)
        {
            _syncing = true;
            _draft = draft;
            int index = _modeCombo->findData(draft.mode);
            if (index >= 0)
                _modeCombo->setCurrentIndex(index);
            _reverseBtn->setChecked(draft.reversed);
            _startEdit->setText(draft.startXYZ);
            _endEdit->setText(draft.endXYZ);
            _vectorEdit->setText(draft.vector);
            syncNodeButton();
            syncPointButtons();
            syncObjectButton();
            syncPage();
            _syncing = false;
        }
        const DirectionDraft& draft() const { return _draft; }
        bool touched() const { return _touched; }
        void clearTouched() { _touched = false; }
        //是否处于节点/边拾取中(外部点击不应关闭弹窗)
        bool isPickingActive() const { return _nodePickConn || _edgePickConn; }
        void stopPicks()
        {
            if (_nodePickConn)
            {
                disconnect(_nodePickConn);
                _nodePickConn = QMetaObject::Connection();
            }
            if (_edgePickConn)
            {
                disconnect(_edgePickConn);
                _edgePickConn = QMetaObject::Connection();
            }
        }
        std::function<void()> onChanged;
    private:
        bool _allowNode;
        DirectionDraft _draft;
        bool _touched = false;
        bool _syncing = false;
        QComboBox* _modeCombo = nullptr;
        QToolButton* _reverseBtn = nullptr;
        QVBoxLayout* _pagesLayout = nullptr;
        QWidget* _nodePage = nullptr;
        QWidget* _pointsPage = nullptr;
        QWidget* _objectPage = nullptr;
        QPushButton* _nodePickBtn = nullptr;
        QPushButton* _startPickBtn = nullptr;
        QPushButton* _endPickBtn = nullptr;
        QLineEdit* _startEdit = nullptr;
        QLineEdit* _endEdit = nullptr;
        QPushButton* _objectPickBtn = nullptr;
        QLabel* _vectorLabel = nullptr;
        QLineEdit* _vectorEdit = nullptr;
        QMetaObject::Connection _nodePickConn;
        QMetaObject::Connection _edgePickConn;

        void markChanged()
        {
            _touched = true;
            if (onChanged)
                onChanged();
        }
        void syncNodeButton()
        {
            if (_nodePickBtn)
                _nodePickBtn->setText(_draft.node.isEmpty() ? QStringLiteral("请选择")
                    : QStringLiteral("节点 ") + _draft.node);
        }
        void syncPointButtons()
        {
            if (_startPickBtn)
                _startPickBtn->setText(_draft.startId.isEmpty() ? QStringLiteral("请选择")
                    : QStringLiteral("节点 ") + _draft.startId);
            if (_endPickBtn)
                _endPickBtn->setText(_draft.endId.isEmpty() ? QStringLiteral("请选择")
                    : QStringLiteral("节点 ") + _draft.endId);
        }
        void syncObjectButton()
        {
            if (_objectPickBtn)
                _objectPickBtn->setText(_draft.objectId.isEmpty() ? QStringLiteral("请选择") : _draft.objectId);
        }
        void syncPage()
        {
            QString mode = _draft.mode;
            //各附加页按方式显隐(高度随当前方式变化，无预留空白)
            _nodePage->setVisible(mode == QStringLiteral("node"));
            _pointsPage->setVisible(mode == QStringLiteral("points"));
            _objectPage->setVisible(mode == QStringLiteral("object"));
            //向量输入框与"方向向量"标签仅"向量"方式显示(同步显隐)
            bool isVector = (mode == QStringLiteral("vector"));
            _vectorLabel->setVisible(isVector);
            _vectorEdit->setVisible(isVector);
            _reverseBtn->setVisible(mode != QStringLiteral("node"));
        }
        void startNodePick(const std::function<void(int, QVector3D)>& callback = {})
        {
            if (getCVM == nullptr || getCVM->getPreRenderWidget() == nullptr)
                return;
            stopPicks();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
            auto handler = [this, callback]() {
                const auto& selected = getCVM->getPreRenderWidget()->getSelectionContainer()
                    ->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
                if (selected.empty())
                    return;
                int nodeId = selected.back().data_id.node_id;
                QVector3D xyz;
                auto meshData = getCVM->getModelData()->getMeshData();
                if (meshData && meshData->_nodedata.nodeidmap.contains(nodeId))
                {
                    double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[nodeId] * 3;
                    xyz = QVector3D(vertex[0], vertex[1], vertex[2]);
                }
                if (callback)
                    callback(nodeId, xyz);
                };
            _nodePickConn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(),
                &mxr::SelectionDataContainer::ReadyToReadContainer, this, handler);
        }
        void startEdgePick()
        {
            if (getCVM == nullptr || getCVM->getPreRenderWidget() == nullptr)
                return;
            stopPicks();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            _edgePickConn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(),
                &mxr::SelectionDataContainer::ReadyToReadContainer, this, [this]() {
                auto geoManager = getCVM->getModelData()->getGeoManager();
                auto lines = getCVM->getPreRenderWidget()->getSelectionContainer()
                    ->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
                if (lines.empty() || geoManager == nullptr)
                    return;
                auto res = geoManager->CalculateEdgeAttr(lines);
                if (res.empty())
                    return;
                _draft.objectDir = res.at(0).m_dir;
                _draft.objectId = QStringLiteral("边 %1").arg(lines.back());
                syncObjectButton();
                markChanged();
                    });
        }
    };

    //梁单元参数面板：单元表格(行内编辑)+拾取/载入+多选批量编辑
    class BeamElementPanel : public QWidget
    {
    public:
        explicit BeamElementPanel(QWidget* parent = nullptr) : QWidget(parent)
        {
            auto root = new QVBoxLayout(this);
            root->setContentsMargins(0, 0, 0, 0);
            root->setSpacing(8);
            //单元表格(表头/交替行/选中色按原型)
            _table = new QTableWidget(this);
            _table->setColumnCount(5);
            _table->setHorizontalHeaderLabels({ QStringLiteral("单元编号"), QStringLiteral("参考方向"),
                QStringLiteral("偏置"), QStringLiteral("A端偏置"), QStringLiteral("B端偏置") });
            _table->verticalHeader()->setVisible(false);
            _table->verticalHeader()->setDefaultSectionSize(34);
            _table->setSelectionBehavior(QAbstractItemView::SelectRows);
            _table->setSelectionMode(QAbstractItemView::ExtendedSelection);
            _table->setEditTriggers(QAbstractItemView::NoEditTriggers);
            _table->setAlternatingRowColors(true);
            _table->setWordWrap(false);
            //表头占满整个表格(末列拉伸)，列宽可交互调整，表头文字居中
            _table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
            _table->horizontalHeader()->setStretchLastSection(true);
            _table->horizontalHeader()->setFixedHeight(28);
            _table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter | Qt::AlignVCenter);
            _table->setStyleSheet(QStringLiteral(
                "QTableWidget{background:#ffffff;alternate-background-color:#fafbfd;"
                "selection-background-color:#e8f3ff;selection-color:#424b57;"
                "gridline-color:#edf0f4;border:1px solid #e4e8ee;border-radius:2px;}"
                "QTableWidget::item{border:0;}"
                "QTableWidget::item:selected{background:#e8f3ff;color:#424b57;}"
                "QHeaderView::section{background:#e9edf3;color:#687383;font-size:11px;font-weight:normal;"
                "padding:0 6px;border:0;border-right:1px solid #ffffff;border-bottom:1px solid #e4e8ee;}"));
            _table->setColumnWidth(0, 56);
            _table->setColumnWidth(1, 150);
            _table->setColumnWidth(2, 60);
            _table->setColumnWidth(3, 104);
            _table->setColumnWidth(4, 104);
            _table->setMinimumHeight(160);
            _table->setMaximumHeight(300);
            root->addWidget(_table);
            //底部工具行：拾取单元 / 当前截面单元 / 偏置列 | 全部选中 / 编辑
            _toolBar = new QWidget(this);
            {
                auto toolsRow = new QHBoxLayout(_toolBar);
                toolsRow->setContentsMargins(0, 0, 0, 0);
                toolsRow->setSpacing(6);
                _btnPick = new QPushButton(QStringLiteral("拾取单元"), _toolBar);
                _btnScope = new QPushButton(QStringLiteral("当前截面单元"), _toolBar);
                _btnSelectAll = new QPushButton(QStringLiteral("全部选中"), _toolBar);
                _btnSelectAll->setEnabled(false);
                _btnEdit = new QPushButton(QStringLiteral("编辑"), _toolBar);
                _btnEdit->setEnabled(false);
                toolsRow->addWidget(_btnPick);
                toolsRow->addWidget(_btnScope);
                toolsRow->addStretch();
                toolsRow->addWidget(_btnSelectAll);
                toolsRow->addWidget(_btnEdit);
            }
            root->addWidget(_toolBar);
            //拾取中的底部完成栏(按钮居中)
            _pickFinishBar = new QWidget(this);
            {
                auto barLay = new QHBoxLayout(_pickFinishBar);
                barLay->setContentsMargins(0, 0, 0, 0);
                _btnFinishPick = new QPushButton(QStringLiteral("完成拾取"), _pickFinishBar);
                _btnFinishPick->setStyleSheet(QStringLiteral(
                    "QPushButton{background:#1890ff;color:#ffffff;border:1px solid #1890ff;border-radius:2px;"
                    "padding:2px 12px;}QPushButton:hover{background:#40a9ff;border-color:#40a9ff;}"));
                barLay->addStretch();
                barLay->addWidget(_btnFinishPick);
                barLay->addStretch();
            }
            _pickFinishBar->hide();
            root->addWidget(_pickFinishBar);
            //错误提示
            _errorLabel = new QLabel(this);
            _errorLabel->setStyleSheet("color:#d95151;font-size:11px;");
            _errorLabel->setWordWrap(true);
            _errorLabel->hide();
            root->addWidget(_errorLabel);
            //批量编辑面板
            buildEditPanel();
            root->addWidget(_editPanel);

            connect(_btnPick, &QPushButton::clicked, this, [this]() { _picking ? stopPick() : startPick(); });
            connect(_btnFinishPick, &QPushButton::clicked, this, [this]() { finishPick(); });
            connect(_btnScope, &QPushButton::clicked, this, [this]() {
                stopPick();
                appendRows(scopeBeamElements());
                clearTableError();
                });
            connect(_btnSelectAll, &QPushButton::clicked, this, [this]() {
                closeEditor();
                if (_table->selectionModel() && _table->selectionModel()->selectedRows().size() == _rows.size() && !_rows.isEmpty())
                    _table->clearSelection();
                else
                    _table->selectAll();
                });
            connect(_btnEdit, &QPushButton::clicked, this, [this]() { openEditor(); });
            connect(_table, &QTableWidget::cellDoubleClicked, this, [this](int, int column) {
                if (column == 0 && !_picking)
                    _table->selectAll();
                });
            connect(_table->horizontalHeader(), &QHeaderView::sectionDoubleClicked, this, [this](int section) {
                if (section == 0 && !_picking)
                    _table->selectAll();
                });
            connect(_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
                updateButtons();
                });
            refreshTable();
        }
        ~BeamElementPanel() override
        {
            stopPick();
        }

        //设置当前属性与作用域部件(属性变化时重置表格)
        void setScope(int propertyId, const std::vector<int>& partIds)
        {
            bool propertyChanged = (propertyId != _propertyId);
            _propertyId = propertyId;
            _scopeParts.clear();
            for (int partId : partIds)
                _scopeParts.insert(partId);
            if (propertyChanged)
            {
                _rows.clear();
                closeEditor();
                refreshTable();
            }
            else
            {
                removeRowsOutOfScope();
            }
        }
        void refreshTable()
        {
            _building = true;
            _table->clearSpans();
            _table->clearContents();
            _table->setRowCount(0);
            if (_rows.isEmpty())
            {
                _table->setRowCount(1);
                auto item = new QTableWidgetItem(QStringLiteral("请拾取梁单元"));
                item->setFlags(Qt::NoItemFlags);
                item->setTextAlignment(Qt::AlignCenter);
                item->setForeground(QColor(0x9a, 0xa3, 0xaf));
                _table->setItem(0, 0, item);
                _table->setSpan(0, 0, 1, 5);
            }
            else
            {
                _table->setRowCount(_rows.size());
                for (int r = 0; r < _rows.size(); ++r)
                    rebuildRow(r);
            }
            if (onRowCountChanged)
                onRowCountChanged(_rows.size());
            _building = false;
            updateOffsetColumns();
            updateButtons();
            ensureHeaderLabels();
            if (onContentResized)
                onContentResized();
        }
        std::function<void()> onElementDataChanged;
        //行数变化(弹窗标题栏"N 行"计数)
        std::function<void(int)> onRowCountChanged;
        //内容尺寸变化(行数/偏置列变化时弹窗高度自适应)
        std::function<void()> onContentResized;
        //拾取状态变化(标题切换"梁单元参数/单元数据"、弹窗收窄)
        std::function<void(bool)> onPickStateChanged;
        //是否存在偏置(任一行偏置非"无")
        bool hasAnyOffsets() const
        {
            for (const auto& row : _rows)
            {
                mBeamElement* element = elementOf(row.eleId);
                if (element && offsetModeOf(element) != QStringLiteral("无"))
                    return true;
            }
            return false;
        }
        //是否处于单元拾取中(外部点击不应关闭弹窗)
        bool isPicking() const { return _picking; }
        //完成拾取：保留本次实时加入的行(已在表中)，结束拾取状态
        void finishPick()
        {
            if (!_picking)
                return;
            stopPick();
            clearTableError();
        }
        //放弃拾取：返回按钮使用，回滚到拾取前的行集
        void cancelPick()
        {
            if (!_picking)
                return;
            _rows = _rowsBeforePick;
            _rowsBeforePick.clear();
            stopPick();
            refreshTable();
            clearTableError();
        }
        //外部结束拾取(返回按钮)，等价于放弃拾取
        void stopPickExternally()
        {
            cancelPick();
        }
        //当前表格行数(条目摘要用)
        int rowCount() const { return _rows.size(); }
        //打开弹窗时重置交互状态(停止拾取/关闭编辑/清空选择)
        void resetInteraction()
        {
            stopPick();
            closeEditor();
            _table->clearSelection();
        }
        //供外部(弹窗关闭校验)显示表格错误
        void showTableErrorExternal(const QString& message) { showTableError(message); }
        //参考方向/偏置控件是否处于节点或边拾取中
        bool isDirectionPickingActive() const
        {
            return (_refControl && _refControl->isPickingActive())
                || (_aControl && _aControl->isPickingActive())
                || (_bControl && _bControl->isPickingActive());
        }
        //整表校验(关闭弹窗前调用)；返回首个错误描述
        bool validateRows(QString* error)
        {
            for (int r = 0; r < _rows.size(); ++r)
            {
                int eleId = _rows[r].eleId;
                auto refCell = _table->cellWidget(r, 1);
                if (refCell)
                {
                    auto modeCombo = refCell->findChild<QComboBox*>();
                    auto valueEdit = refCell->findChild<QLineEdit*>();
                    if (modeCombo && valueEdit)
                    {
                        if (modeCombo->currentData().toString() == QStringLiteral("node"))
                        {
                            bool ok = false;
                            qint64 nodeId = valueEdit->text().simplified().toLongLong(&ok);
                            if (!ok || nodeId <= 0)
                            {
                                if (error)
                                    *error = QStringLiteral("单元 %1：节点编号须为正整数").arg(eleId);
                                return false;
                            }
                        }
                        else
                        {
                            double xyz[3];
                            if (!parseBeamVector(valueEdit->text(), xyz))
                            {
                                if (error)
                                    *error = QStringLiteral("单元 %1：参考方向请输入三个有效数值，例如 0, 1, 0").arg(eleId);
                                return false;
                            }
                            if (xyz[0] == 0 && xyz[1] == 0 && xyz[2] == 0)
                            {
                                if (error)
                                    *error = QStringLiteral("单元 %1：参考方向不能为零向量").arg(eleId);
                                return false;
                            }
                        }
                    }
                }
                mBeamElement* element = elementOf(eleId);
                if (!element || offsetModeOf(element) == QStringLiteral("无"))
                    continue;
                for (int side = 0; side < 2; ++side)
                {
                    auto cell = _table->cellWidget(r, 3 + side);
                    if (!cell)
                        continue;
                    auto edit = cell->findChild<QLineEdit*>();
                    if (!edit)
                        continue;
                    double xyz[3];
                    if (!parseBeamVector(edit->text(), xyz))
                    {
                        if (error)
                            *error = QStringLiteral("单元 %1：%2端偏置请输入三个有效数值，例如 0, 0, 0")
                            .arg(eleId).arg(side == 0 ? QStringLiteral("A") : QStringLiteral("B"));
                        return false;
                    }
                }
            }
            return true;
        }
    protected:
        void hideEvent(QHideEvent* event) override
        {
            stopPick();
            closeEditor();
            QWidget::hideEvent(event);
        }
    private:
        struct RowData
        {
            int eleId = 0;
            int partId = 0;
        };
        int _propertyId = -1;
        QSet<int> _scopeParts;
        QList<RowData> _rows;
        QList<RowData> _rowsBeforePick;   //拾取前行集快照(返回按钮回滚用)
        int _colWidths[5] = { 56, 150, 78, 104, 104 };//各列基础宽度(拉伸列不受此影响)
        QTableWidget* _table = nullptr;
        QWidget* _toolBar = nullptr;
        QWidget* _pickFinishBar = nullptr;
        QPushButton* _btnFinishPick = nullptr;
        QPushButton* _btnPick = nullptr;
        QPushButton* _btnScope = nullptr;
        QPushButton* _btnSelectAll = nullptr;
        QPushButton* _btnEdit = nullptr;
        QLabel* _errorLabel = nullptr;
        QWidget* _editPanel = nullptr;
        QLabel* _editCount = nullptr;
        QLabel* _editError = nullptr;
        DirectionControl* _refControl = nullptr;
        QComboBox* _offsetCombo = nullptr;
        QWidget* _aRow = nullptr;
        QWidget* _bRow = nullptr;
        DirectionControl* _aControl = nullptr;
        DirectionControl* _bControl = nullptr;
        QPushButton* _btnApply = nullptr;
        bool _picking = false;
        bool _building = false;
        bool _offsetTouched = false;
        QMetaObject::Connection _pickConn;

        //---------- 数据访问 ----------
        mBeamElement* elementOf(int eleId) const
        {
            if (getCVM == nullptr || _propertyId < 0)
                return nullptr;
            auto element = getCVM->getModelData()->getElementData()->getElementDataByMeshID(eleId);
            return dynamic_cast<mBeamElement*>(element);
        }
        QList<RowData> scopeBeamElements() const
        {
            QList<RowData> result;
            if (getCVM == nullptr)
                return result;
            auto meshData = getCVM->getModelData()->getMeshData();
            for (int partId : _scopeParts)
            {
                for (int eleId : meshData->getEleIdsByPartId(partId))
                {
                    if (elementOf(eleId))
                        result.append({ eleId, partId });
                }
            }
            return result;
        }
        int rowOf(int eleId) const
        {
            for (int r = 0; r < _rows.size(); ++r)
                if (_rows[r].eleId == eleId)
                    return r;
            return -1;
        }
        static QString offsetModeOf(mBeamElement* element)
        {
            QString mode = element->_biasType.simplified();
            if (mode == QStringLiteral("BGG") || mode == QStringLiteral("GGG"))
                return mode;
            return QStringLiteral("无");
        }
        static QString offsetText(mBeamElement* element, bool aEnd)
        {
            QString x = aEnd ? element->_X1 : element->_X2;
            QString y = aEnd ? element->_Y1 : element->_Y2;
            QString z = aEnd ? element->_Z1 : element->_Z2;
            if (x.simplified().isEmpty() && y.simplified().isEmpty() && z.simplified().isEmpty())
                return aEnd ? QStringLiteral("0, 0, 0") : QStringLiteral("0, 0, 0");
            auto fix = [](const QString& v) { return v.simplified().isEmpty() ? QStringLiteral("0") : v.simplified(); };
            return QStringLiteral("%1, %2, %3").arg(fix(x), fix(y), fix(z));
        }
        //---------- 表格 ----------
        void appendRows(const QList<RowData>& rows)
        {
            closeEditor();
            for (const auto& row : rows)
            {
                if (rowOf(row.eleId) >= 0)
                    continue;
                _rows.append(row);
            }
            refreshTable();
        }
        void removeRowsOutOfScope()
        {
            bool changed = false;
            for (int r = _rows.size() - 1; r >= 0; --r)
            {
                if (!_scopeParts.contains(_rows[r].partId))
                {
                    _rows.removeAt(r);
                    changed = true;
                }
            }
            if (changed)
                refreshTable();
        }
        //单元格容器：选中行时容器铺选中色(控件本身保持原样)，靠 rowSelected 属性联动
        QWidget* makeCellHost()
        {
            auto host = new QWidget(_table);
            host->setAttribute(Qt::WA_StyledBackground, true);//普通QWidget需此项QSS背景才生效
            host->setProperty("cellHost", true);
            host->setStyleSheet(QStringLiteral(
                "QWidget[cellHost=\"true\"]{background:transparent;}"
                "QWidget[cellHost=\"true\"][rowSelected=\"true\"]{background:#e8f3ff;}"));
            return host;
        }
        //按当前选中状态刷新各行单元格容器底色
        void syncRowSelectionStyles()
        {
            if (_table->selectionModel() == nullptr)
                return;
            for (int r = 0; r < _rows.size(); ++r)
            {
                bool selected = _table->selectionModel()->isRowSelected(r, QModelIndex());
                for (int col = 1; col < 5; ++col)
                {
                    QWidget* host = _table->cellWidget(r, col);
                    if (host == nullptr || !host->property("cellHost").toBool())
                        continue;
                    host->setProperty("rowSelected", selected);
                    host->style()->unpolish(host);
                    host->style()->polish(host);
                }
            }
        }
        QLineEdit* makeValueEdit()
        {
            auto edit = new QLineEdit();
            edit->setStyleSheet("QLineEdit{border:1px solid #dce2e9;border-radius:2px;padding:1px 4px;}"
                "QLineEdit:focus{border-color:#91caff;}"
                "QLineEdit[invalid=\"true\"]{border-color:#d95151;}"
                "QLineEdit:disabled{background:#f5f7fa;color:#a6afbb;border-color:transparent;}");
            edit->setFrame(false);
            return edit;
        }
        void rebuildRow(int r)
        {
            const auto& rowData = _rows[r];
            mBeamElement* element = elementOf(rowData.eleId);
            auto idItem = new QTableWidgetItem(QString::number(rowData.eleId));
            idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
            idItem->setTextAlignment(Qt::AlignCenter);
            _table->setItem(r, 0, idItem);
            //参考方向：方式+值
            {
                auto cell = makeCellHost();
                auto lay = new QHBoxLayout(cell);
                lay->setContentsMargins(2, 2, 2, 2);
                lay->setSpacing(4);
                auto modeCombo = new QComboBox(cell);
                modeCombo->setFixedHeight(24);
                modeCombo->setStyleSheet(QStringLiteral("QComboBox{font-size:11px;padding:0 4px;}"));
                modeCombo->addItem(QStringLiteral("节点"), QStringLiteral("node"));
                modeCombo->addItem(QStringLiteral("向量"), QStringLiteral("vector"));
                bool nodeMode = element && !element->_node.simplified().isEmpty();
                modeCombo->setCurrentIndex(nodeMode ? 0 : 1);
                auto valueEdit = makeValueEdit();
                if (element)
                {
                    if (nodeMode)
                        valueEdit->setText(element->_node.simplified());
                    else
                        valueEdit->setText(element->_beamValue.simplified().isEmpty()
                            ? QStringLiteral("0, 1, 0") : element->_beamValue.simplified());
                }
                lay->addWidget(modeCombo);
                lay->addWidget(valueEdit, 1);
                _table->setCellWidget(r, 1, cell);
                int eleId = rowData.eleId;
                connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, eleId, modeCombo, valueEdit](int) {
                    if (_building)
                        return;
                    closeEditor();
                    mBeamElement* e = elementOf(eleId);
                    if (!e)
                        return;
                    QString mode = modeCombo->currentData().toString();
                    if (mode == QStringLiteral("node"))
                        valueEdit->setText(e->_node.simplified().isEmpty() ? QString() : e->_node.simplified());
                    else
                        valueEdit->setText(e->_beamValue.simplified().isEmpty()
                            ? QStringLiteral("0, 1, 0") : e->_beamValue.simplified());
                    writeRef(eleId, mode, valueEdit->text(), valueEdit);
                    });
                connect(valueEdit, &QLineEdit::editingFinished, this, [this, eleId, modeCombo, valueEdit]() {
                    if (_building)
                        return;
                    closeEditor();
                    writeRef(eleId, modeCombo->currentData().toString(), valueEdit->text(), valueEdit);
                    });
                connect(valueEdit, &QLineEdit::textChanged, this, [valueEdit](const QString&) {
                    markLineEditInvalid(valueEdit, false);
                    });
            }
            //偏置类型
            {
                auto combo = new QComboBox();
                combo->addItem(QStringLiteral("无"), QString());
                combo->addItem(QStringLiteral("BGG"), QStringLiteral("BGG"));
                combo->addItem(QStringLiteral("GGG"), QStringLiteral("GGG"));
                combo->setFixedHeight(24);
                combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
                combo->setStyleSheet(QStringLiteral(
                    "QComboBox{font-size:11px;padding:0 4px;}"
                    "QComboBox::drop-down{width:16px;}"
                    "QComboBox:disabled{color:#788391;}"));
                QString current = element ? offsetModeOf(element) : QStringLiteral("无");
                combo->setCurrentIndex(qMax(0, combo->findData(current)));
                auto cell = makeCellHost();
                auto lay = new QHBoxLayout(cell);
                lay->setContentsMargins(2, 2, 2, 2);
                lay->addWidget(combo, 1);
                _table->setCellWidget(r, 2, cell);
                int eleId = rowData.eleId;
                connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, eleId, combo](int) {
                    if (_building)
                        return;
                    closeEditor();
                    mBeamElement* e = elementOf(eleId);
                    if (!e)
                        return;
                    e->_biasType = combo->currentData().toString();
                    int r = rowOf(eleId);
                    if (r >= 0)
                        refreshOffsetCells(r);
                    updateOffsetColumns();
                    notifyChanged();
                    });
            }
            //A/B端偏置
            for (int side = 0; side < 2; ++side)
            {
                bool aEnd = (side == 0);
                auto cell = makeCellHost();
                auto lay = new QHBoxLayout(cell);
                lay->setContentsMargins(2, 2, 2, 2);
                auto edit = makeValueEdit();
                lay->addWidget(edit);
                _table->setCellWidget(r, 3 + side, cell);
                refreshOffsetCell(r, aEnd, edit);
                int eleId = rowData.eleId;
                connect(edit, &QLineEdit::editingFinished, this, [this, eleId, aEnd, edit]() {
                    if (_building)
                        return;
                    closeEditor();
                    writeOffset(eleId, aEnd, edit->text(), edit);
                    });
                connect(edit, &QLineEdit::textChanged, this, [edit](const QString&) {
                    markLineEditInvalid(edit, false);
                    });
            }
        }
        void refreshOffsetCells(int r)
        {
            for (int side = 0; side < 2; ++side)
            {
                auto cell = _table->cellWidget(r, 3 + side);
                if (!cell)
                    continue;
                auto edit = cell->findChild<QLineEdit*>();
                if (edit)
                    refreshOffsetCell(r, side == 0, edit);
            }
        }
        void refreshOffsetCell(int r, bool aEnd, QLineEdit* edit)
        {
            mBeamElement* element = elementOf(_rows[r].eleId);
            bool enabled = element && offsetModeOf(element) != QStringLiteral("无");
            edit->setEnabled(enabled);
            if (enabled)
                edit->setText(element ? offsetText(element, aEnd) : QStringLiteral("0, 0, 0"));
            else
                edit->setText(QStringLiteral("—"));
        }
        //---------- 写入 ----------
        void notifyChanged()
        {
            if (onElementDataChanged)
                onElementDataChanged();
        }
        void showTableError(const QString& message)
        {
            _errorLabel->setText(message);
            _errorLabel->setVisible(!message.isEmpty());
        }
        void clearTableError() { showTableError(QString()); }
        void writeRef(int eleId, const QString& mode, const QString& value, QLineEdit* sourceEdit)
        {
            mBeamElement* element = elementOf(eleId);
            if (!element)
                return;
            QString error;
            if (mode == QStringLiteral("node"))
            {
                bool ok = false;
                qint64 nodeId = value.simplified().toLongLong(&ok);
                if (!ok || nodeId <= 0)
                    error = QStringLiteral("节点编号须为正整数");
            }
            else
            {
                double xyz[3];
                if (!parseBeamVector(value, xyz))
                    error = QStringLiteral("参考方向请输入三个有效数值，例如 0, 1, 0");
                else if (xyz[0] == 0 && xyz[1] == 0 && xyz[2] == 0)
                    error = QStringLiteral("参考方向不能为零向量");
            }
            if (!error.isEmpty())
            {
                if (sourceEdit)
                    markLineEditInvalid(sourceEdit, true);
                showTableError(QStringLiteral("单元 %1：%2").arg(eleId).arg(error));
                return;
            }
            if (mode == QStringLiteral("node"))
            {
                element->_node = value.simplified();
                element->_beamValue = QString();
            }
            else
            {
                double xyz[3];
                parseBeamVector(value, xyz);
                element->_beamValue = formatBeamVector(xyz);
                element->_node = QString();
            }
            if (sourceEdit)
                sourceEdit->setText(element->_node.isEmpty() ? element->_beamValue : element->_node);
            markLineEditInvalid(sourceEdit, false);
            clearTableError();
            notifyChanged();
        }
        void writeOffset(int eleId, bool aEnd, const QString& value, QLineEdit* sourceEdit)
        {
            mBeamElement* element = elementOf(eleId);
            if (!element)
                return;
            double xyz[3];
            if (!parseBeamVector(value, xyz))
            {
                if (sourceEdit)
                    markLineEditInvalid(sourceEdit, true);
                showTableError(QStringLiteral("单元 %1：%2端偏置请输入三个有效数值，例如 0, 0, 0")
                    .arg(eleId).arg(aEnd ? QStringLiteral("A") : QStringLiteral("B")));
                return;
            }
            element->_X1 = aEnd ? QString::number(xyz[0], 'g', 12) : element->_X1;
            element->_Y1 = aEnd ? QString::number(xyz[1], 'g', 12) : element->_Y1;
            element->_Z1 = aEnd ? QString::number(xyz[2], 'g', 12) : element->_Z1;
            element->_X2 = aEnd ? element->_X2 : QString::number(xyz[0], 'g', 12);
            element->_Y2 = aEnd ? element->_Y2 : QString::number(xyz[1], 'g', 12);
            element->_Z2 = aEnd ? element->_Z2 : QString::number(xyz[2], 'g', 12);
            if (sourceEdit)
            {
                markLineEditInvalid(sourceEdit, false);
                sourceEdit->setText(formatBeamVector(xyz));
            }
            clearTableError();
            notifyChanged();
        }
        //---------- 拾取 ----------
        void startPick()
        {
            if (getCVM == nullptr || getCVM->getPreRenderWidget() == nullptr)
                return;
            closeEditor();
            _picking = true;
            //快照当前行集：返回按钮回滚，完成拾取保留
            _rowsBeforePick = _rows;
            _btnPick->setText(QStringLiteral("完成拾取"));
            setPickingLayout(true);
            QVector<bool> enabled(10, false);
            enabled[8] = true;
            emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(enabled);
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
            _pickConn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(),
                &mxr::SelectionDataContainer::ReadyToReadContainer, this, [this]() {
                const auto& selected = getCVM->getPreRenderWidget()->getSelectionContainer()
                    ->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
                if (selected.empty())
                    return;
                bool rejected = false;
                QList<RowData> incoming;
                for (const auto& pick : selected)
                {
                    int eleId = pick.data_id.element_id;
                    int partId = pick.part_id;
                    if (rowOf(eleId) >= 0)
                        continue;
                    if (!_scopeParts.contains(partId) || elementOf(eleId) == nullptr)
                    {
                        rejected = true;
                        continue;
                    }
                    incoming.append({ eleId, partId });
                }
                if (!incoming.isEmpty())
                {
                    //单表模式：拾取结果实时加入表格(快照可回滚)
                    _rows.append(incoming);
                    refreshTable();
                }
                showTableError(rejected ? QStringLiteral("仅能拾取当前截面关联的梁单元。") : QString());
                    });
            clearTableError();
        }
        void stopPick()
        {
            if (_pickConn)
            {
                disconnect(_pickConn);
                _pickConn = QMetaObject::Connection();
            }
            if (_picking)
            {
                emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
                setPickingLayout(false);
            }
            _picking = false;
            _rowsBeforePick.clear();
            _btnPick->setText(QStringLiteral("拾取单元"));
        }
        //确保表头行与标签存在(清空/重建表格后防御性恢复)且居中显示
        void ensureHeaderLabels()
        {
            auto header = _table->horizontalHeader();
            header->setVisible(true);
            header->setFixedHeight(28);
            header->setDefaultAlignment(Qt::AlignCenter | Qt::AlignVCenter);
            static const char* const labels[5] = { "单元编号", "参考方向", "偏置", "A端偏置", "B端偏置" };
            for (int col = 0; col < 5; ++col)
            {
                auto item = _table->horizontalHeaderItem(col);
                if (item == nullptr || item->text().isEmpty())
                    _table->setHorizontalHeaderItem(col, new QTableWidgetItem(QString::fromUtf8(labels[col])));
            }
        }
        //拾取时隐藏右侧四列(仅剩单元编号)，完成拾取后按偏置状态恢复
        void setPickingLayout(bool picking)
        {
            for (int col = 1; col < 5; ++col)
                _table->setColumnHidden(col, picking);
            if (!picking)
            {
                _table->setMinimumHeight(160);
                updateOffsetColumns();
            }
            updateTableMinWidth();
            ensureHeaderLabels();
            _toolBar->setVisible(!picking);
            _pickFinishBar->setVisible(picking);
            if (onPickStateChanged)
                onPickStateChanged(picking);
        }
        //---------- 批量编辑 ----------
        void buildEditPanel()
        {
            _editPanel = new QFrame(this);
            _editPanel->setObjectName("editPanel");
            _editPanel->setStyleSheet(
                "#editPanel{background:#FFFFFF;border:1px solid #e4e8ee;border-radius:2px;}"
            );
            _editPanel->hide();
            auto panelLay = new QVBoxLayout(_editPanel);
            panelLay->setContentsMargins(8, 8, 8, 8);
            panelLay->setSpacing(8);
            auto header = new QHBoxLayout();
            auto title = new QLabel(QStringLiteral("参数设置"), _editPanel);
            QFont boldFont = title->font();
            boldFont.setBold(true);
            title->setFont(boldFont);
            _editCount = new QLabel(_editPanel);
            _editCount->setStyleSheet("color:#788391;font-size:11px;");
            auto collapseBtn = new QPushButton(QStringLiteral("×"), _editPanel);
            collapseBtn->setFixedSize(20, 20);
            collapseBtn->setToolTip(QStringLiteral("收起参数设置"));
            header->addWidget(title);
            header->addWidget(_editCount);
            header->addStretch();
            header->addWidget(collapseBtn);
            panelLay->addLayout(header);
            auto makeRow = [this, panelLay](const QString& label, QWidget* editor) {
                auto row = new QHBoxLayout();
                row->setSpacing(6);
                auto lab = new QLabel(label, _editPanel);
                lab->setFixedWidth(56);
                //标签与控件首行(下拉框)保持同一水平线
                row->addWidget(lab, 0, Qt::AlignTop);
                row->addWidget(editor, 1);
                panelLay->addLayout(row);
                return row;
                };
            _refControl = new DirectionControl(true, _editPanel);
            _refControl->onChanged = [this]() { updateApplyEnabled(); };
            makeRow(QStringLiteral("参考方向"), _refControl);
            _offsetCombo = new QComboBox(_editPanel);
            _offsetCombo->addItem(QStringLiteral("无"), QString());
            _offsetCombo->addItem(QStringLiteral("BGG"), QStringLiteral("BGG"));
            _offsetCombo->addItem(QStringLiteral("GGG"), QStringLiteral("GGG"));
            _offsetCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            makeRow(QStringLiteral("偏置类型"), _offsetCombo);
            _aControl = new DirectionControl(false, _editPanel);
            _aControl->onChanged = [this]() { updateApplyEnabled(); };
            _aRow = new QWidget(_editPanel);
            {
                auto row = new QHBoxLayout(_aRow);
                row->setContentsMargins(0, 0, 0, 0);
                row->setSpacing(6);
                auto lab = new QLabel(QStringLiteral("A端偏置"), _aRow);
                lab->setFixedWidth(56);
                row->addWidget(lab, 0, Qt::AlignTop);
                row->addWidget(_aControl, 1);
            }
            panelLay->addWidget(_aRow);
            _bControl = new DirectionControl(false, _editPanel);
            _bControl->onChanged = [this]() { updateApplyEnabled(); };
            _bRow = new QWidget(_editPanel);
            {
                auto row = new QHBoxLayout(_bRow);
                row->setContentsMargins(0, 0, 0, 0);
                row->setSpacing(6);
                auto lab = new QLabel(QStringLiteral("B端偏置"), _bRow);
                lab->setFixedWidth(56);
                row->addWidget(lab, 0, Qt::AlignTop);
                row->addWidget(_bControl, 1);
            }
            panelLay->addWidget(_bRow);
            _editError = new QLabel(_editPanel);
            _editError->setStyleSheet("color:#d95151;font-size:11px;");
            _editError->setWordWrap(true);
            _editError->hide();
            panelLay->addWidget(_editError);
            auto footer = new QHBoxLayout();
            auto hint = new QLabel(QStringLiteral("仅应用已修改项"), _editPanel);
            hint->setStyleSheet("color:#788391;font-size:11px;");
            auto cancelBtn = new QPushButton(QStringLiteral("取消"), _editPanel);
            _btnApply = new QPushButton(QStringLiteral("应用"), _editPanel);
            _btnApply->setStyleSheet(QStringLiteral(
                "QPushButton:enabled{background:#1890ff;color:#ffffff;border:1px solid #1890ff;border-radius:2px;}"
                "QPushButton:hover{background:#40a9ff;border-color:#40a9ff;}"));
            _btnApply->setEnabled(false);
            footer->addWidget(hint);
            footer->addStretch();
            footer->addWidget(cancelBtn);
            footer->addWidget(_btnApply);
            panelLay->addLayout(footer);

            connect(collapseBtn, &QPushButton::clicked, this, [this]() { closeEditor(); });
            connect(cancelBtn, &QPushButton::clicked, this, [this]() { closeEditor(); });
            connect(_btnApply, &QPushButton::clicked, this, [this]() { applyDraft(); });
            connect(_offsetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
                if (_building)
                    return;
                _offsetTouched = true;
                updateEditVisibility();
                updateApplyEnabled();
                });
        }
        QList<int> selectedElementIds() const
        {
            QList<int> ids;
            if (!_table->selectionModel())
                return ids;
            for (const QModelIndex& index : _table->selectionModel()->selectedRows())
            {
                int r = index.row();
                if (r >= 0 && r < _rows.size())
                    ids.append(_rows[r].eleId);
            }
            return ids;
        }
        void updateButtons()
        {
            int count = selectedElementIds().size();
            _btnEdit->setEnabled(count > 0);
            _btnEdit->setToolTip(count > 0 ? QStringLiteral("已选 %1 个单元").arg(count) : QString());
            bool all = !_rows.isEmpty() && count == _rows.size();
            _btnSelectAll->setText(all ? QStringLiteral("取消全选") : QStringLiteral("全部选中"));
            _btnSelectAll->setEnabled(!_rows.isEmpty());
            //选中状态变化时同步单元格容器底色(整行变色)
            syncRowSelectionStyles();
        }
        //偏置列纯数据驱动：有偏置显示A/B端列，全为"无"则隐藏
        void updateOffsetColumns()
        {
            bool expanded = hasAnyOffsets();
            _table->setColumnHidden(3, !expanded);
            _table->setColumnHidden(4, !expanded);
            if (expanded)
            {
                _colWidths[0] = 56; _colWidths[1] = 150; _colWidths[2] = 78; _colWidths[3] = 104; _colWidths[4] = 104;
            }
            else
            {
                //参考方向列约占表宽一半
                _colWidths[0] = 64; _colWidths[1] = 166; _colWidths[2] = 78; _colWidths[3] = 104; _colWidths[4] = 104;
            }
            for (int col = 0; col < 5; ++col)
                _table->setColumnWidth(col, _colWidths[col]);
            updateTableMinWidth();
            if (onContentResized)
                onContentResized();
        }
        //表格最小宽度=可见列的基础宽度之和(不受拉伸列当前宽度影响，弹窗宽度自适应依据)
        void updateTableMinWidth()
        {
            int width = 8;//边框/滚动条余量
            for (int col = 0; col < 5; ++col)
                if (!_table->isColumnHidden(col))
                    width += _colWidths[col];
            _table->setMinimumWidth(width);
        }
        void updateApplyEnabled()
        {
            _btnApply->setEnabled(_refControl->touched() || _offsetTouched
                || _aControl->touched() || _bControl->touched());
            _editError->hide();
        }
        void updateEditVisibility()
        {
            bool none = _offsetCombo->currentData().toString().isEmpty();
            _aRow->setVisible(!none);
            _bRow->setVisible(!none);
        }
        void openEditor()
        {
            QList<int> ids = selectedElementIds();
            if (ids.isEmpty())
                return;
            mBeamElement* first = elementOf(ids.first());
            if (!first)
                return;
            _building = true;
            //以首行数据初始化草稿
            DirectionDraft refDraft;
            bool nodeMode = !first->_node.simplified().isEmpty();
            refDraft.mode = nodeMode ? QStringLiteral("node") : QStringLiteral("vector");
            refDraft.node = first->_node.simplified();
            refDraft.vector = first->_beamValue.simplified().isEmpty()
                ? QStringLiteral("0, 1, 0") : first->_beamValue.simplified();
            _refControl->setDraft(refDraft);
            _refControl->clearTouched();
            _offsetCombo->setCurrentIndex(qMax(0, _offsetCombo->findData(offsetModeOf(first))));
            _offsetTouched = false;
            DirectionDraft aDraft, bDraft;
            aDraft.vector = offsetText(first, true);
            bDraft.vector = offsetText(first, false);
            _aControl->setDraft(aDraft);
            _aControl->clearTouched();
            _bControl->setDraft(bDraft);
            _bControl->clearTouched();
            _editCount->setText(QStringLiteral("%1 个单元").arg(ids.size()));
            _editError->hide();
            updateEditVisibility();
            _btnApply->setEnabled(false);
            _building = false;
            _editPanel->show();
            if (onContentResized)
                onContentResized();
        }
        void closeEditor()
        {
            _editPanel->hide();
            _editError->hide();
            if (onContentResized)
                onContentResized();
        }
        void showEditError(const QString& message)
        {
            _editError->setText(message);
            _editError->show();
        }
        void applyDraft()
        {
            QList<int> ids = selectedElementIds();
            if (ids.isEmpty())
                return;
            QString offsetMode = _offsetCombo->currentData().toString();
            bool useRef = _refControl->touched();
            bool useOffset = _offsetTouched || _aControl->touched() || _bControl->touched();
            bool useA = _aControl->touched() && !offsetMode.isEmpty();
            bool useB = _bControl->touched() && !offsetMode.isEmpty();
            if (!useRef && !useOffset)
                return;
            //解析草稿方向
            QString refNode, refVector;
            if (useRef)
            {
                if (_refControl->draft().mode == QStringLiteral("node"))
                {
                    refNode = _refControl->draft().node.simplified();
                    bool ok = false;
                    qint64 v = refNode.toLongLong(&ok);
                    if (!ok || v <= 0)
                    {
                        showEditError(QStringLiteral("单元 %1：节点编号须为正整数").arg(ids.first()));
                        return;
                    }
                }
                else
                {
                    bool ok = false;
                    refVector = resolveBeamDirection(_refControl->draft(), &ok);
                    double xyz[3];
                    if (!ok)
                    {
                        showEditError(QStringLiteral("单元 %1：参考方向请输入三个有效数值，例如 0, 1, 0").arg(ids.first()));
                        return;
                    }
                    parseBeamVector(refVector, xyz);
                    if (xyz[0] == 0 && xyz[1] == 0 && xyz[2] == 0)
                    {
                        showEditError(QStringLiteral("单元 %1：参考方向不能为零向量").arg(ids.first()));
                        return;
                    }
                }
            }
            QString aVector, bVector;
            bool aOk = true, bOk = true;
            if (useA)
                aVector = resolveBeamDirection(_aControl->draft(), &aOk);
            if (useB)
                bVector = resolveBeamDirection(_bControl->draft(), &bOk);
            if ((useA && !aOk) || (useB && !bOk))
            {
                showEditError(QStringLiteral("单元 %1：偏置请输入三个有效数值，例如 0, 0, 0").arg(ids.first()));
                return;
            }
            //写入
            for (int eleId : ids)
            {
                mBeamElement* element = elementOf(eleId);
                if (!element)
                    continue;
                if (useRef)
                {
                    if (!refNode.isEmpty())
                    {
                        element->_node = refNode;
                        element->_beamValue = QString();
                    }
                    else
                    {
                        element->_beamValue = refVector;
                        element->_node = QString();
                    }
                }
                if (useOffset)
                    element->_biasType = offsetMode;
                double xyz[3];
                if (useA && parseBeamVector(aVector, xyz))
                {
                    element->_X1 = QString::number(xyz[0], 'g', 12);
                    element->_Y1 = QString::number(xyz[1], 'g', 12);
                    element->_Z1 = QString::number(xyz[2], 'g', 12);
                }
                if (useB && parseBeamVector(bVector, xyz))
                {
                    element->_X2 = QString::number(xyz[0], 'g', 12);
                    element->_Y2 = QString::number(xyz[1], 'g', 12);
                    element->_Z2 = QString::number(xyz[2], 'g', 12);
                }
            }
            refreshTable();
            clearTableError();
            notifyChanged();
            openEditor(); //重新以首行初始化，保持面板打开
        }
    };

    mPropertyPanel::mPropertyPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);

        _propertyData = nullptr;

        // candidateParts: QStringList 候选项，提前准备好
        QStringList candidateParts = { "部件A", "部件B", "部件C", "部件D" };
        _partModel = new QStringListModel(candidateParts);

        _propShapeModel = new QStringListModel();

        //初始化布局
        // ========== 材料指派折叠块(0) ==========
        FoldSection* matFold = new FoldSection("材料指派", this);

        // 材料
        _comboMat = new QComboBox();
        //_comboMat->addItems({ "" });

        //跳转材料窗口按钮
        QToolButton* matBtn = new QToolButton(); matBtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/cailiao.png")); matBtn->setToolTip("新建材料");

        matFold->addRow(new PanelLabel("材料"), _comboMat, matBtn);
        m_foldWidgets.append(matFold);

        connect(matBtn, &QToolButton::pressed, this, &mPropertyPanel::slot_newMaterial);

        connect(_comboMat, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), this, &mPropertyPanel::slot_matComboBoxChanged);

        // ========== 作用域折叠块(1) ==========
        FoldSection* assignFold = new FoldSection("作用域", this);
        // 部件选择
        _comboPartSet = new QComboBox();
        // 插入提示项
        _comboPartSet->addItem("请选择部件集合");
        _comboPartSet->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        _comboPartSet->setCurrentIndex(0);                 // 默认不选中任何项

        connect(_comboPartSet, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            QString partSetName = _comboPartSet->currentText();
            _comboPartSet->blockSignals(true);
            _comboPartSet->setCurrentIndex(0);
            _comboPartSet->blockSignals(false);
            slot_partSetComboBoxChanged(partSetName);
            });

        //已经赋予的部件列表
        QGridLayout* assignLayout = new QGridLayout();
        _listAssignPart = new QListWidget();
        _listAssignPart->setSelectionMode(QAbstractItemView::ExtendedSelection);
        QToolButton* allBtn = new QToolButton(); allBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png")); allBtn->setToolTip("全部部件");
        QToolButton* addBtn = new QToolButton(); addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); addBtn->setToolTip("增加部件");
        QToolButton* reduceBtn = new QToolButton(); reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); reduceBtn->setToolTip("减少部件");
        QToolButton* deleteBtn = new QToolButton(); deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); deleteBtn->setToolTip("清空部件");

        assignLayout->addWidget(_listAssignPart, 0, 0, 8, 7);
        assignLayout->addWidget(allBtn, 0, 7);
        assignLayout->addWidget(addBtn, 1, 7);
        assignLayout->addWidget(reduceBtn, 2, 7);
        assignLayout->addWidget(deleteBtn, 3, 7);

        assignFold->addRow(new PanelLabel("部件集合"), _comboPartSet);
        assignFold->addRow(assignLayout);
        m_foldWidgets.append(assignFold);

        connect(allBtn, &QToolButton::clicked, this, &mPropertyPanel::slot_allPartBtn);
        connect(addBtn, &QToolButton::clicked, this, &mPropertyPanel::slot_addPartBtn);
        connect(reduceBtn, &QToolButton::clicked, this, &mPropertyPanel::slot_reducePartBtn);//槽函数待完善
        connect(deleteBtn, &QToolButton::clicked, this, &mPropertyPanel::slot_clearPartBtn);//槽函数待完善


        // ========== 梁截面参数折叠块(2) ==========
        FoldSection* propShapeFold = new FoldSection("梁截面参数", this);

        //轮廓类型
        _comboBeamType = new QComboBox();
        _comboBeamType->addItems({ "一致", "变化" });

        if (!_angleLineEdit)
        {
            _angleLineEdit = std::make_shared<mCalcLineEdit>("0", this, false, false); _angleLineEdit->setParent(nullptr);
            connect(_angleLineEdit.get(), &QLineEdit::editingFinished, this, [this]() {
                if (_propertyData == nullptr)
                {
                    return;
                }
                _propertyData->setAngle(_angleLineEdit->text());
                //updateRotateAngleRend();
                //
                });
        }

        //轮廓类型/截面内容均由 buildBeamSectionFold 构建的吸附弹窗承载
        buildBeamSectionFold(propShapeFold);
        m_foldWidgets.append(propShapeFold);

        //用户在下拉中点选时触发(程序设置文本不触发，避免误调槽函数)
        connect(_comboBeamType, QOverload<const QString&>::of(&QComboBox::activated), this, &mPropertyPanel::slot_beamTypeComboBoxChanged);
        connect(_comboPropShape, QOverload<const QString&>::of(&QComboBox::activated), this, &mPropertyPanel::slot_beamComboBoxChanged);

        // ========== 参数折叠块(3) ==========
        FoldSection* paramFold = new FoldSection("参数", this);
        m_foldWidgets.append(paramFold);

        _hor = { {"k"} , {"km"}, {"b"} , {"ge"} , {"ang"} , {"m"} };
        _ver = {
            {"K1", "K2", "K3","K4","K5", "K6"} ,
            {"KMAG1", "KMAG2", "KMAG3","KMAG4","KMAG5", "KMAG6"} ,
            {"B1", "B2", "B3","B4","B5", "B6"} ,
            {"GE1", "GE2", "GE3","GE4","GE5", "GE6"} ,
            {"ANGLE1", "ANGLE2", "ANGLE3","ANGLE4","ANGLE5", "ANGLE6"} ,
            {"M1", "M2", "M3","M4","M5", "M6"} ,
        };

        // ========== 梁单元参数折叠块(4) ==========
        FoldSection* beamElementFold = new FoldSection("梁单元参数", this);
        buildBeamElementFold(beamElementFold);
        m_foldWidgets.append(beamElementFold);

                // ========== 参考坐标折叠块(5) ==========
        FoldSection* coordinateFold = new FoldSection("参考坐标", this);

        // 坐标系
        _comboCoordinate = new QComboBox();
        //_comboMat->addItems({ "" });

        //跳转坐标系窗口按钮
        QToolButton* coordinateBtn = new QToolButton(); coordinateBtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/zuobiaoxi.png")); coordinateBtn->setToolTip("新建坐标系");

        coordinateFold->addRow(new PanelLabel("坐标系"), _comboCoordinate, coordinateBtn);
        m_foldWidgets.append(coordinateFold);

        connect(_comboCoordinate, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPropertyPanel::slot_coorStore);
        connect(coordinateBtn, &QPushButton::clicked, this, [this]() {
            int propertyID = _propertyData->getPropertyId();

            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createCoordinate();

            auto allCoor = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateID();

            int coorid = allCoor.at(allCoor.size() - 1);
            //获取映射
            auto& mapProp = getCVM->getModelData()->getMapRelationManager();
            mapProp.appendMapRelation({ Physical_Property, propertyID }, { Physical_Coordinate,coorid });
            });


        // ========== 弹簧单元参数折叠块(6) ==========
        FoldSection* springFold = new FoldSection("参数", this);
        m_foldWidgets.append(springFold);

        // ========== 铺层特殊的定义折叠块(7) ==========
        FoldSection* define = new FoldSection("铺层类型", this);
        // 铺层类型下拉框
        _comboPlyType = new QComboBox();
        _comboPlyType->addItems({ "壳","连续壳","实体" });
        define->addRow(new PanelLabel("单元类型"), _comboPlyType);
        connect(_comboPlyType, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPropertyPanel::slot_plyTypeChanged);
        m_foldWidgets.append(define);

        // ========== 铺层特殊的材料方向折叠块(8) ==========
        FoldSection* materialDir = new FoldSection("参考坐标", this);
        // 坐标系
        _comboPlyCoord = new QComboBox();

        //跳转坐标系窗口按钮
        QToolButton* plyCoordinateBtn = new QToolButton();
        plyCoordinateBtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/zuobiaoxi.png"));
        plyCoordinateBtn->setToolTip("新建坐标系");

        materialDir->addRow(new PanelLabel("坐标系"), _comboPlyCoord, plyCoordinateBtn);
        m_foldWidgets.append(materialDir);

        connect(_comboPlyCoord, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPropertyPanel::slot_materialDirCoor);
        connect(plyCoordinateBtn, &QPushButton::clicked, this, [this]() {
            int propertyID = _propertyData->getPropertyId();
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createCoordinate();
            auto allCoor = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateID();
            int coorid = allCoor.at(allCoor.size() - 1);
            //获取映射
            auto& mapProp = getCVM->getModelData()->getMapRelationManager();
            mapProp.appendMapRelation({ Physical_Property, propertyID }, { Physical_Coordinate,coorid });
            });

        // ========== 铺层特殊的铺层定义折叠块(9) ==========
        FoldSection* assign = new FoldSection("铺层定义", this);
        _radDefine = new QRadioButton("直接选择");
        _radLaminate = new QRadioButton("层合板");
        assign->addRow(_radDefine, _radLaminate);
        // 绑定信号槽
        connect(_radDefine, &QRadioButton::toggled,
            this, &mPropertyPanel::onRadioToggled);
        connect(_radLaminate, &QRadioButton::toggled,
            this, &mPropertyPanel::onRadioToggled);
        m_foldWidgets.append(assign);
        // 构造函数中添加
        m_plyTableWidget = new mPlyTableWidget();
        // 连接信号槽
        connect(m_plyTableWidget, &mPlyTableWidget::rowAdded, this, &mPropertyPanel::onPlyRowAdded);
        connect(m_plyTableWidget, &mPlyTableWidget::rowRemoved, this, &mPropertyPanel::onPlyRowRemoved);
        connect(m_plyTableWidget, &mPlyTableWidget::tableCleared, this, &mPropertyPanel::onPlyTableCleared);
        connect(m_plyTableWidget, &mPlyTableWidget::materialChanged, this, &mPropertyPanel::onPlyMaterialChanged);
        connect(m_plyTableWidget, &mPlyTableWidget::thicknessChanged, this, &mPropertyPanel::onPlyThicknessChanged);
        connect(m_plyTableWidget, &mPlyTableWidget::angleChanged, this, &mPropertyPanel::onPlyAngleChanged);
        connect(m_plyTableWidget, &mPlyTableWidget::intePointChanged, this, &mPropertyPanel::onPlyIntePointChanged);
        connect(m_plyTableWidget, &mPlyTableWidget::plyStackPlotClicked, this, &mPropertyPanel::slot_showPlyWindow);

        // ========== 截面定义折叠块(10) ==========
        FoldSection* defineFold = new FoldSection("截面定义", this);
        _lblVariety = new QLabel("");
        _lblType = new QLabel("");
        defineFold->addRow(new PanelLabel("截面类别："), _lblVariety);
        defineFold->addRow(new PanelLabel("截面类型："), _lblType);

        m_foldWidgets.append(defineFold);

        // ========== 弹簧类型折叠块(11) ==========
        FoldSection* springCategoryFold = new FoldSection("弹簧类型", this);
        _springCategoryCombo = new QComboBox(this);
        _springCategoryCombo->addItems({ "单自由度弹簧", "三自由度弹簧", "六自由度接地弹簧", "六自由度弹簧" });
        springCategoryFold->addRow(new PanelLabel("弹簧类型"), _springCategoryCombo);
        connect(_springCategoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (_propertyData == nullptr) return;
            mSpring* spring = toType<mSpring>(_propertyData);
            if (!spring) return;
            QString text = _springCategoryCombo->currentText();
            spring->_springCategory = text;
            if (text == "单自由度弹簧")
                _propertyData->setPropertyType(Elas1Property);
            else if (text == "三自由度弹簧")
                _propertyData->setPropertyType(SpringAProperty);
            else if (text == "六自由度接地弹簧")
                _propertyData->setPropertyType(Spring1Property);
            else if (text == "六自由度弹簧")
                _propertyData->setPropertyType(Spring2Property);
            updatePanel();
            updateSpringParamPanel();
            updateDynamicInfoData();
            updatePropertyDefine();
            });
        m_foldWidgets.append(springCategoryFold);

        _mainLayout->addStretch();

        for (int i = 0; i < m_foldWidgets.size(); i++)
        {
            _mainLayout->removeWidget(m_foldWidgets[i]); m_foldWidgets[i]->hide();
        }

        // --- 保持之前选项 ---
        //connect(mGlobalSignals::getInstance(), &mGlobalSignals::updateModelData, this, [this](PhysicalFieldType physicalFieldType, int modelId)
        //    {
        //        if (physicalFieldType == Physical_Set)
        //        {
        //            updatePartSetCombobox();
        //        }
        //        else if (physicalFieldType == Physical_Material)
        //        {
        //            updateMatCombobox(_comboMat);
        //        }
        //        else if (physicalFieldType == Physical_PropertyShape)
        //        {
        //            updatePropShapeCombobox(_comboPropShape);
        //        }
        //    });

        // ========== 定义折叠块(12) ==========
        {
            FoldSection* defineFold = new FoldSection("定义", this);
            _comboMacro = new QComboBox();
            connect(_comboMacro, &QComboBox::currentTextChanged, this, [this](QString name) {
                auto pMacro = toType<mMacroEquivalent>(_propertyData);
                if (pMacro)
                    pMacro->_frbName = name;
                });
            defineFold->addRow(new PanelLabel("宏观等效截面"), _comboMacro);
            m_foldWidgets.append(defineFold);
        }

    }

    void mPropertyPanel::updatePanel()
    {
        if (_propertyData == nullptr)
        {
            return;
        }
        PropertyType type = _propertyData->getPropertyType();
        for (int i = 0; i < m_foldWidgets.size(); i++)
        {
            _mainLayout->removeWidget(m_foldWidgets[i]); m_foldWidgets[i]->hide();
        }
        auto partIds = updatePartAssignPanel();
        highlightPart(partIds);
        //int c = _mainLayout->count();
        if (type != MacroEquivalentSection)
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_SectionDefine]); m_foldWidgets[Fold_SectionDefine]->show();
            m_foldWidgets[Fold_Scope]->setTitle("作用域");
        }
        if (type == ResultantProperty)
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Material]); m_foldWidgets[Fold_Material]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Scope]); m_foldWidgets[Fold_Scope]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_BeamSection]); m_foldWidgets[Fold_BeamSection]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_BeamElement]); m_foldWidgets[Fold_BeamElement]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Coordinate]); m_foldWidgets[Fold_Coordinate]->show();

            QSignalBlocker blocker(_comboBeamType);
            _comboBeamType->setCurrentText("一致");

            //更新截面页面与梁单元参数面板
            this->updateBeamSectionPanel();
            this->syncBeamElementPanel();
        }
        else if (type == CrossProperty)
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Material]); m_foldWidgets[Fold_Material]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Scope]); m_foldWidgets[Fold_Scope]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_BeamSection]); m_foldWidgets[Fold_BeamSection]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_BeamElement]); m_foldWidgets[Fold_BeamElement]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Coordinate]); m_foldWidgets[Fold_Coordinate]->show();

            QSignalBlocker blocker(_comboBeamType);
            _comboBeamType->setCurrentText("变化");

            //更新变截面页面与梁单元参数面板
            this->updateBeamSectionPanel();
            this->syncBeamElementPanel();
        }
        else if (type == RodProperty || type == TubeProperty || type == PlaneStrainProperty || type == PlaneStressProperty || type == ShellProperty || type == ShearPlateProperty || type == GasketProperty)
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Material]); m_foldWidgets[Fold_Material]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Scope]); m_foldWidgets[Fold_Scope]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Param]); m_foldWidgets[Fold_Param]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Coordinate]); m_foldWidgets[Fold_Coordinate]->show();

            //更新参数页面
            this->updateParamPanel();
        }
        else if (type == ViscousDampProperty || type == CbushProperty || type == GapuniProperty)
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Scope]); m_foldWidgets[Fold_Scope]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Param]); m_foldWidgets[Fold_Param]->show();

            //更新参数页面
            this->updateParamPanel();
        }
        else if (type == Elas1Property || type == Spring1Property || type == SpringAProperty || type == Spring2Property)
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Scope]); m_foldWidgets[Fold_Scope]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_SpringCategory]); m_foldWidgets[Fold_SpringCategory]->show();
            if (type == Spring2Property)
            {
                _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Coordinate]); m_foldWidgets[Fold_Coordinate]->show();
            }
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Spring]); m_foldWidgets[Fold_Spring]->show();

            //更新弹簧类型下拉框
            mSpring* spring = toType<mSpring>(_propertyData);
            if (spring)
            {
                QSignalBlocker blocker(_springCategoryCombo);
                _springCategoryCombo->setCurrentText(spring->_springCategory);
            }

            //更新参数页面
            this->updateSpringParamPanel();
        }
        else if (type == SolidProperty || type == SolidShellProperty || type == MembraneProperty)
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Material]); m_foldWidgets[Fold_Material]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Scope]); m_foldWidgets[Fold_Scope]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Coordinate]); m_foldWidgets[Fold_Coordinate]->show();
            //_mainLayout->addStretch();

            //设置现在的材料
            //_comboMat->setCurrentText("");
        }
        else if (type == PlyLaminateProperty)
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Scope]); m_foldWidgets[Fold_Scope]->show(); //作用域
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_PlyType]); m_foldWidgets[Fold_PlyType]->show(); //铺层类型
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_MaterialDir]); m_foldWidgets[Fold_MaterialDir]->show(); //材料方向 ->参考坐标系
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Param]); m_foldWidgets[Fold_Param]->show(); //参数
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_PlyAssignment]); m_foldWidgets[Fold_PlyAssignment]->show(); //铺层定义

            this->updatematerialDirCoor();
            this->updateParamPanel();

            int coorid = -1;
            QString coorname = _comboPlyCoord->currentText();
            if (!coorname.isEmpty() && getCVM != nullptr)
            {
                coorid = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getIDByCoordinateName(coorname);
            }
            this->updatematerialDirFold(coorid);
        }
        else if (type == MacroEquivalentSection)
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_MacroEquivalentSection]); m_foldWidgets[Fold_MacroEquivalentSection]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[Fold_Scope]); m_foldWidgets[Fold_Scope]->show(); //作用域
            m_foldWidgets[Fold_Scope]->setTitle("赋予");

            QStringList names = getCVM->getModelData()->getManager<MechFrpData::mFrpManager>(Physical_FRCMaterial).getAllRootsName();
            _comboMacro->blockSignals(true);
            _comboMacro->clear();
            _comboMacro->addItems(names);
            auto pMacro = toType<mMacroEquivalent>(_propertyData);
            if (names.contains(pMacro->_frbName))
            {
                std::string aa = pMacro->_frbName.toStdString();
                _comboMacro->setCurrentText(pMacro->_frbName);
            }
            else
            {
                _comboMacro->setCurrentIndex(0);
                pMacro->_frbName = _comboMacro->currentText();
            }
            _comboMacro->blockSignals(false);
        }

        if (type != LayeredShellProperty)
        {
            //更新材料页面
            this->updateMatPanel();
            this->updateCoorPanel();
        }

        _mainLayout->invalidate();
        _mainLayout->activate();
        this->updateGeometry();

    }

    void mPropertyPanel::slot_finishedPick()
    {
        QTime time;
        time.start();
        if (getCVM == nullptr)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        auto partIds = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        // 当前 UI 中已有的元素 ID
        std::vector<mxr::MeshSelectInfoData> res;
        if (partIds.size() > _lastPickData.size())
        {
            // 拾取数据比当前多 → 新增的部分
            std::vector<int> addedIds;
            QSet<int> currentSet;
            for (auto id : _lastPickData)
            {
                currentSet.insert(id);
            }
            for (auto& e : partIds)
            {
                if (!currentSet.contains(e))
                {
                    addedIds.push_back(e);
                    mapRelation.appendMapRelation({ Physical_Part, e }, { Physical_Property, id });
                    getCVM->updatePartShowColor(PartColorType::MatColor, { e });
                }
            }

            if (!addedIds.empty()) {
                //std::vector<mxr::MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getPreRender()->getMeshSelectInfoData()[SelectionInfoType::MeshElement];
                qDebug() << "开始赋予" << time.elapsed();
                assignElement(addedIds);

                highlightPart(addedIds);
                updatePartAssignPanel();
                qDebug() << "结束填充列表" << time.elapsed();
            }
            getCVM->getDataStatusManager()->updateDynamicInfoData({ {Physical_Part, addedIds} });
        }
        else
        {
            // 拾取数据比当前少 → 减少的部分
            std::vector<int> reducedIds;
            QSet<int> selectedSet;
            for (auto& e : partIds)
                selectedSet.insert(e);

            for (int id : _lastPickData)
            {
                if (!selectedSet.contains(id))
                {
                    reducedIds.push_back(id);
                    mapRelation.deleteMapRelation({ Physical_Part, id });                    //删除映射关系
                    getCVM->updatePartShowColor(PartColorType::MatColor, { id });
                }
            }

            if (!reducedIds.empty()) {
                //std::vector<mxr::MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getPreRender()->getMeshSelectInfoData()[SelectionInfoType::MeshElement];
                qDebug() << "开始取消高亮" << time.elapsed();
                unHighlightPart(reducedIds);
                qDebug() << "开始填充列表" << time.elapsed();
                updatePartAssignPanel();
                qDebug() << "结束填充列表" << time.elapsed();
            }
            getCVM->getDataStatusManager()->updateDynamicInfoData({ {Physical_Part, reducedIds} });
        }

        //梁属性：作用域变化后同步梁单元参数面板
        this->syncBeamElementPanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, _propertyData->getPropertyId());
    }

    void mPropertyPanel::slot_coorStore()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (_propertyData == nullptr)
        {
            return;
        }
        int coorid = getCoor();
        //获取映射
        auto& mapProp = getCVM->getModelData()->getMapRelationManager();
        //坐标系映射
        int id = _propertyData->getPropertyId();
        mapProp.appendMapRelation({ Physical_Property, id }, { Physical_Coordinate,coorid });
    }

    void mPropertyPanel::slot_tableData(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (_propertyData == nullptr) return;

        PropertyType type = _propertyData->getPropertyType();
        switch (type)
        {
        case MxFunctions::Spring1Property:
        case MxFunctions::Spring2Property:
        case MxFunctions::SpringAProperty:
        case MxFunctions::Elas1Property:
        {
            mSpring* spring = toType<mSpring>(_propertyData);
            spring->_luCurve.clear();
            for (int i = 0; i < x[0].size(); i++)
            {
                spring->_luCurve.push_back({ x[0][i], y[0][i] });
            }
            break;
        }
        case MxFunctions::CbushProperty:
        {
            mCbush* cbush = toType<mCbush>(_propertyData);
            int index = -1;
            for (int i = 0; i < _hor.size(); ++i)
            {
                if (_hor[i][0] == title)
                {
                    index = i;
                    break;
                }
            }
            if (index == -1 || x.empty())
                break;

            for (int i = 0; i < x[0].size(); i++)
            {
                cbush->_value[index][i] = x[0][i];
            }

            break;
        }
        default:
            break;
        }



    }

    void mPropertyPanel::slot_plyTypeChanged(int idx)
    {
        //数据
        auto plyLam = toType<mPlyLaminate>(_propertyData);
        plyLam->_type = idx;
        this->updateParamPanel();
        this->updatePlyLamDefineFold();
    }

    void mPropertyPanel::onRadioToggled()
    {
        auto plyLam = toType<mPlyLaminate>(_propertyData);
        if (_radDefine->isChecked())
        {
            plyLam->_plyDefType = 0;

        }
        else if (_radLaminate->isChecked())
        {
            plyLam->_plyDefType = 1;
        }
        updatePlyLamDefineFold();
    }

    void mPropertyPanel::updatematerialDirCoor()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (_propertyData == nullptr)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int coorId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_Coordinate);
        QString coorname = getCVM->getModelData()->getNameByPhyID(Physical_Coordinate, coorId);
        _comboPlyCoord->setCurrentText(coorname);
    }

    void mPropertyPanel::updatematerialDirFold(int coorid)
    {
        // 最优先空指针检查
        if (!_propertyData)
            return;

        auto data = toType<mPlyLaminate>(_propertyData);
        if (!data) return;

        auto fold = m_foldWidgets[Fold_MaterialDir];
        if (!fold || !fold->layout()) return;

        // 保存坐标系选择
        data->_materialDir = (coorid == -1) ? 0 : 1;

        QFormLayout* formLayout = fold->layout();
        while (formLayout && formLayout->rowCount() > 1)
        {
            formLayout->removeRow(1);
        }

        if (coorid != -1)
        {
            // ================== 行1：法向 ==================
            QComboBox* comboDir = new QComboBox(this);
            comboDir->addItems({ "X", "Y", "Z" });
            comboDir->setCurrentIndex((data->_normaldirection - 1 >= 0) ? (data->_normaldirection - 1) : 2); //大于等于0则说明读取文件没问题，按读取的反写即可，小于等于0说明文件有问题，默认选Z
            connect(comboDir, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int idx)
                {
                    data->_normaldirection = idx + 1;
                });
            fold->addRow(new PanelLabel("法向"), comboDir);

            // ================== 行2：额外转角 ==================
            QComboBox* comboAngleType = new QComboBox(this);
            comboAngleType->addItems({ "无","固定角度","离散角度" });
            comboAngleType->setCurrentIndex(data->_rotAngleType);

            connect(comboAngleType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int idx)
                {
                    data->_rotAngleType = idx;
                    QTimer::singleShot(0, this, [=]() {
                        updatematerialDirFold(coorid);
                        });
                }, Qt::QueuedConnection);

            fold->addRow(new PanelLabel("额外转角"), comboAngleType);

            // ================== 行3：动态内容 ==================
            if (data->_rotAngleType == 1)
            {
                QLabel* labelAngle = new PanelLabel("转角");
                mCalcLineEdit* lineAngle = new mCalcLineEdit(QString(data->_rotAngle), this, true, false, true);
                connect(lineAngle, &QLineEdit::editingFinished, this, [=]()
                    {
                        data->_rotAngle = lineAngle->text();
                    });
                fold->addRow(labelAngle, lineAngle);
            }
            else if (data->_rotAngleType == 2)
            {
                QLabel* labelField = new PanelLabel("角度离散场");
                QComboBox* comboField = new QComboBox(this);
                auto& dfManager = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
                QStringList allNames = dfManager.getAllFieldNames(MxFunctions::FieldType::Discrete);
                QStringList angleNames;
                for (const QString& name : allNames)
                {
                    int fid = dfManager.getIDByFieldName(name);
                    auto* fdata = dfManager.getDiscreteFieldData(fid);
                    if (fdata && fdata->getSpaceType() == MxFunctions::AngelSpace)
                        angleNames.append(name);
                }
                comboField->addItems(angleNames);
                // 读取当前角度离散场映射并设置选中项
                int propID = data->getPropertyId();
                auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
                int curFieldID = mapRelation.getMapRelation<int>(
                    { Physical_Property, propID }, Physical_Field, MapRelation_RotationField);
                if (curFieldID >= 0)
                {
                    QString curName = dfManager.getFieldNameByID(curFieldID);
                    if (angleNames.contains(curName))
                    {
                        comboField->setCurrentText(curName);
                    }
                    else
                    {
                        // 映射的离散场已不存在于当前类型列表中（被修改为其他类型）
                        mapRelation.appendMapRelation({ Physical_Property, propID }, { Physical_Field, -1 }, MapRelation_RotationField);
                        if (!angleNames.isEmpty())
                        {
                            comboField->setCurrentIndex(0);
                            int fid = dfManager.getIDByFieldName(angleNames[0]);
                            mapRelation.appendMapRelation({ Physical_Property, propID }, { Physical_Field, fid }, MapRelation_RotationField);
                        }
                    }
                }
                else
                {
                    //无离散场映射，但可能创建了离散场，防止默认选择第一个但没有添加上映射
                    if (!angleNames.isEmpty())
                    {
                        int fid = dfManager.getIDByFieldName(comboField->currentText());
                        mapRelation.appendMapRelation({ Physical_Property, propID }, { Physical_Field, fid }, MapRelation_RotationField);
                    }
                }

                // 绑定切换信号，更新映射
                connect(comboField, &QComboBox::currentTextChanged, this, [=](const QString& text)
                    {
                        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
                        auto& mgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
                        int id = data->getPropertyId();
                        int oldFieldID = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_Field, MapRelation_RotationField);
                        if (oldFieldID != -1)
                            mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_Field, -1 }, MapRelation_RotationField);
                        if (!text.isEmpty())
                        {
                            int fid = mgr.getIDByFieldName(text);
                            mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_Field, fid }, MapRelation_RotationField);
                        }
                    });
                fold->addRow(labelField, comboField);
            }
        }

        // 最后刷新布局
        QTimer::singleShot(0, fold, [=]() {
            fold->refreshLayout();
            fold->updateGeometry();
            });
    }

    void mPropertyPanel::updatePlyLamDefineFold()
    {
        auto plyLam = toType<mPlyLaminate>(_propertyData);
        if (!plyLam) return;
        auto fold = m_foldWidgets[Fold_PlyAssignment];

        // 彻底清空单选按钮之后的所有内容
        QFormLayout* formLayout = fold->layout();
        while (formLayout && formLayout->rowCount() > 1)
        {
            formLayout->removeRow(1);
        }

        // 切换逻辑：根据数据状态创建控件，避免依赖UI单选按钮状态
        if (plyLam->_plyDefType == 0)
        {
            plyTableShow();
        }
        else if (plyLam->_plyDefType == 1)
        {
            //创建控件
            QLabel* labelLam = new PanelLabel("层合板");
            QComboBox* comboLam = new QComboBox(this);
            fold->addRow(labelLam, comboLam);

            //绑定信号
            connect(comboLam, &QComboBox::currentTextChanged, this, [=](const QString& text)
                {
                    int lamId = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getIDByLaminateName(text);
                    getCVM->getModelData()->getMapRelationManager().appendMapRelation(
                        { Physical_Property, plyLam->getPropertyId() },
                        { Physical_Laminate, lamId });
                });

            mTableDataWindow::getInstance()->resetInitialTable();
            // 刷新下拉数据
            comboLam->blockSignals(true);
            comboLam->clear();
            QStringList names = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getAllLaminateByTypes(LamPlate);
            comboLam->addItems(names);

            // 恢复映射
            int id = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>(
                { Physical_Property, plyLam->getPropertyId() },
                Physical_Laminate
            );
            if (id != -1)
            {
                QString name = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getLaminateNameByID(id);
                comboLam->setCurrentText(name);
            }
            else
            {
                comboLam->setCurrentIndex(0);
                int lamId = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getIDByLaminateName(comboLam->currentText());
                getCVM->getModelData()->getMapRelationManager().appendMapRelation(
                    { Physical_Property, plyLam->getPropertyId() },
                    { Physical_Laminate, lamId });
            }
            comboLam->blockSignals(false);
        }

        // 刷新布局
        QTimer::singleShot(0, fold, [=]()
            {
                fold->refreshLayout();
            });
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Property, _propertyData->getPropertyId());
    }

    void mPropertyPanel::slot_materialDirCoor()
    {
        // 安全判断
        if (getCVM == nullptr || _propertyData == nullptr)
        {
            return;
        }

        int coorid = -1;
        QString coorname = _comboPlyCoord->currentText();
        coorid = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getIDByCoordinateName(coorname);

        // 映射
        auto& mapProp = getCVM->getModelData()->getMapRelationManager();
        int id = _propertyData->getPropertyId();
        mapProp.appendMapRelation({ Physical_Property, id }, { Physical_Coordinate,coorid });

        //更新材料方向折叠块
        this->updatematerialDirFold(coorid);
    }

    void mPropertyPanel::updateMatCombobox(QComboBox* combobox)
    {
        combobox->blockSignals(true);
        QString currentText = combobox->currentText();

        combobox->clear();
        combobox->addItems(getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getAllMatNames());

        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        }
        else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(-1); // 默认选择第一个
        }
        combobox->blockSignals(false);
    }

    void mPropertyPanel::updateCoordiCombobox(QComboBox* combobox)
    {
        combobox->blockSignals(true);
        QString currentText = combobox->currentText();

        combobox->clear();
        combobox->addItems(getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames());

        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        }
        else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(0); // 默认选择第一个
        }
        combobox->blockSignals(false);
    }

    void mPropertyPanel::updatePartSetCombobox()
    {
        _comboPartSet->blockSignals(true);
        _comboPartSet->clear();

        _comboPartSet->addItem("请选择部件集合");
        _comboPartSet->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        _comboPartSet->setCurrentIndex(0);                 // 默认不选中任何项

        _comboPartSet->addItems(getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Part));
        _comboPartSet->blockSignals(false);
    }

    void mPropertyPanel::updatePropShapeCombobox(QComboBox* combobox)
    {
        combobox->blockSignals(true);
        //必须在刷新模型前捕获当前文本：setStringList 会清空模型，之后读到的已不是用户所选
        QString currentText = combobox->currentText();
        QStringList names = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape).getAllSectionShapeNames();
        _propShapeModel->setStringList(names);//截面形状数据

        combobox->setModel(_propShapeModel);

        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        }
        else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(-1); // 默认不选择
        }
        combobox->blockSignals(false);
    }

    mPropertyPanel::~mPropertyPanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }

        if (_plyViewport != nullptr)
        {
            delete _plyViewport;
            _plyViewport = nullptr;
        }

    }

    void mPropertyPanel::newPropertyPanel(PropertyType type)
    {
        setEditing(true);
        int propertyID;
        switch (type)
        {
        case MxFunctions::RodProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mTorsionBar(propertyID, RodProperty);
            break;
        case MxFunctions::ResultantProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mConstantBeam(propertyID, ResultantProperty);
            break;
        case MxFunctions::CrossProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mCrossBeam(propertyID, CrossProperty);
            break;
        case MxFunctions::TubeProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mTube(propertyID, TubeProperty);
            break;
        case MxFunctions::PlaneStrainProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mShellHomo(propertyID, PlaneStrainProperty);
            break;
        case MxFunctions::PlaneStressProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mShellHomo(propertyID, PlaneStressProperty);
            break;
        case MxFunctions::ShellProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mShellHomo(propertyID, ShellProperty);
            break;
        case MxFunctions::ShearPlateProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mShearPlate(propertyID, ShearPlateProperty);
            break;
        case MxFunctions::SolidProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mSolidHomo(propertyID, SolidProperty);
            break;
        case MxFunctions::SolidShellProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mSolidHomo(propertyID, SolidShellProperty);
            break;
        case MxFunctions::LayeredShellProperty:
            break;
        case MxFunctions::Spring1Property:
        case MxFunctions::Spring2Property:
        case MxFunctions::SpringAProperty:
        case MxFunctions::Elas1Property:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mSpring(propertyID, type);
            break;
        case MxFunctions::ViscousDampProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mViscousdampsection(propertyID, type);
            break;
        case MxFunctions::CbushProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mCbush(propertyID, type);
            break;
        case MxFunctions::PlyLaminateProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mPlyLaminate(propertyID, type);
            break;
        case MxFunctions::MembraneProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mMembrane(propertyID, MembraneProperty);
            break;
        case MxFunctions::GasketProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mGasket(propertyID, GasketProperty);
            break;
        case MxFunctions::GapuniProperty:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mGapuni(propertyID, GapuniProperty);
            break;
        case MxFunctions::MacroEquivalentSection:
            propertyID = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).createID();
            _propertyData = new mMacroEquivalent(propertyID, MacroEquivalentSection);
            break;
        default:
            return;
        }

        _propertyData->setPropertyName(getCVM->getModelData()->createNameByType(Physical_Property, _propertyData->propertyTypeToStringEn()));
        getCVM->getModelData()->addPhysicalField<mPropertyBase>(_propertyData);

        //添加默认映射关系
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        mapRelation.appendTableMapRelation({ Physical_Property, _propertyData->getPropertyId() }, { Physical_PropertyShape, -1 }, 0);
        mapRelation.appendTableMapRelation({ Physical_Property, _propertyData->getPropertyId() }, { Physical_PropertyShape, -1 }, 1);

        //updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(true, _propertyData->getPropertyId());
    }

    void mPropertyPanel::editPropertyPanel(int propertyID)
    {
        if (!getCVM)
            return;
        if (!getCVM->getPreRenderWidget())
            return;
        //切换编辑对象时关闭吸附式弹窗
        closeBeamPopups();
        _propertyData = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property).getPropertyData(propertyID);
        setEditing(true);
        connectSig();
        updatePropertyDefine();
        updatePartSetCombobox();
        updateMatCombobox(_comboMat);
        updateCoordiCombobox(_comboCoordinate);
        if (_propertyData->getPropertyType() == PlyLaminateProperty)
        {
            updateCoordiCombobox(_comboPlyCoord);
            updatePlyLamDefineFold();
            updatematerialDirCoor();
        }
        updatePropShapeCombobox(_comboPropShape);
        updatePanel();
    }

    void mPropertyPanel::uneditPropertyPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        if (!getCVM)
            return;
        if (!getCVM->getPreRenderWidget())
            return;
        //取消编辑时关闭吸附式弹窗
        closeBeamPopups();
        _propertyData = nullptr;
        disconnectSig();
        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        MechPreData::mPreViewportPrivate* preViewport = nullptr;
        if (viewportPrivate != nullptr)
        {
            preViewport = dynamic_cast<MechPreData::mPreViewportPrivate*>(viewportPrivate);
            if (preViewport == nullptr)
            {
                return;
            }
        }
        //渲染接入（取消编辑截面形状角度）
        _lastPickData;//这是部件id
        for (const auto& part_id : _lastPickData)
        {
            if (preViewport->_entityRodBeam.find(part_id) == preViewport->_entityRodBeam.end())
            {
                getCVM->getPreRenderWidget()->GetBeamSystem()->deleteBeam(QString("rod_%1").arg(part_id));
            }
        }
        for (const auto& part_id : _lastPickData)
        {
            if (preViewport->_entityShell.find(part_id) == preViewport->_entityShell.end())
            {
                getCVM->getPreRenderWidget()->GetBeamSystem()->deleteShell(QString("shell_%1").arg(part_id));
            }
        }
        getCVM->getPreRenderWidget()->getGizmoManager()->setActiveGizmo(mxr::GizmoType::None); // 设置控件模式
        //getCVM->getPreRenderWidget()->getStateMachineManager()->enableGizmo(false); // 开启控件编辑
    }

    int mPropertyPanel::getCoor()
    {
        int coorid = -1;
        QString coorname = _comboCoordinate->currentText();
        coorid = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getIDByCoordinateName(coorname);
        return coorid;
    }

    void mPropertyPanel::slot_matComboBoxChanged(const QString& text)
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (_propertyData == nullptr)
        {
            return;
        }
        auto& materialManager = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material);
        auto mat = materialManager.getMaterialData(text);
        if (mat == nullptr)
        {
            return;
        }
        _comboMat->setCurrentText(mat->getMaterialName());
        //设置id

        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int matId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_Material);
        mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_Material, mat->getMaterialId() });

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, _propertyData->getPropertyId());

        auto partList = mapRelation.getMapRelation<QList<int>>({ Physical_Property, id }, Physical_Part);
        std::vector<int> partIds{ partList.begin(), partList.end() };
        getCVM->updatePartShowColor(PartColorType::MatColor, partIds);
    }

    void mPropertyPanel::slot_partSetComboBoxChanged(const QString& text)
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (_propertyData == nullptr)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        auto setData = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(text);
        if (setData == nullptr)
        {
            return;
        }
        auto partSet = dynamic_cast<mPartSet*>(setData);
        auto partIds = partSet->getPartSetData();
        if (partIds.empty())
        {
            return;
        }
        for (auto partId : partIds)
        {
            mapRelation.appendMapRelation({ Physical_Part, partId }, { Physical_Property, id });
        }
        getCVM->updatePartShowColor(PartColorType::MatColor, partIds);
        highlightPart(partIds);
        assignElement(partIds);
        updatePartAssignPanel();
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, _propertyData->getPropertyId());
    }

    void mPropertyPanel::slot_beamTypeComboBoxChanged(const QString& text)
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (_propertyData == nullptr)
        {
            return;
        }
        //切换前先校验当前参数编辑区
        if (!canLeaveSectionEditor())
        {
            QSignalBlocker blocker(_comboBeamType);
            _comboBeamType->setCurrentText(_propertyData->getPropertyType() == CrossProperty ? "变化" : "一致");
            return;
        }
        int id = _propertyData->getPropertyId();
        auto fold = m_foldWidgets[Fold_BeamSection];
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        //deleteByPhysicalType 会统一清空该属性的全部映射，切换前先快照、切换后回填，避免数据丢失
        int matId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_Material);
        int coorId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_Coordinate);
        QList<int> partList = mapRelation.getMapRelation<QList<int>>({ Physical_Property, id }, Physical_Part);
        int uniformShapeId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_PropertyShape);
        QList<int> tableShapeIds = mapRelation.getMapRelation<QList<int>>({ Physical_Property, id }, Physical_PropertyShape);
        //一致/变化各自维护一套数据，互不干涉：离开时各自暂存，切回时各自恢复
        if (_propertyData->getPropertyType() == CrossProperty)
        {
            //离开变化模式：暂存位置列表与各行截面，供切回时恢复
            auto crossBeam = toType<mCrossBeam>(_propertyData);
            if (crossBeam != nullptr && crossBeam->_paraList.size() >= 2)
            {
                _stashCrossPositions = crossBeam->_paraList;
                _stashCrossShapeRows = tableShapeIds;
                while (_stashCrossShapeRows.size() < _stashCrossPositions.size())
                    _stashCrossShapeRows.append(-1);
            }
        }
        else
        {
            //离开一致模式：暂存一致截面，供切回时恢复
            _stashUniformShapeId = uniformShapeId;
        }
        if (text == "一致")//常截面
        {
            //数据类转换(变截面转常截面)

            auto propertyData = new mConstantBeam(id, ResultantProperty);
            getCVM->getModelData()->deleteByPhysicalType(PhysicalFieldType::Physical_Property, id);
            getCVM->getModelData()->addPhysicalField<mPropertyBase>(propertyData);
            _propertyData = propertyData;
            //回填材料/坐标系/部件映射
            if (matId != -1)
                mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_Material, matId });
            if (coorId != -1)
                mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_Coordinate, coorId });
            for (int partId : partList)
                mapRelation.appendMapRelation({ Physical_Part, partId }, { Physical_Property, id });
            //仅恢复一致模式自己的截面暂存，不取变化数据
            if (_stashUniformShapeId > 0)
                mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_PropertyShape, _stashUniformShapeId });

            //更新截面页面
            this->updateBeamSectionPanel();
            this->syncBeamElementPanel();
        }
        else//变截面
        {
            auto propertyData = new mCrossBeam(id, CrossProperty);
            getCVM->getModelData()->deleteByPhysicalType(PhysicalFieldType::Physical_Property, id);
            getCVM->getModelData()->addPhysicalField<mPropertyBase>(propertyData);
            _propertyData = propertyData;
            //回填材料/坐标系/部件映射
            if (matId != -1)
                mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_Material, matId });
            if (coorId != -1)
                mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_Coordinate, coorId });
            for (int partId : partList)
                mapRelation.appendMapRelation({ Physical_Part, partId }, { Physical_Property, id });
            //仅恢复变化模式自己的数据暂存；无暂存(首次进入)则为空数据，不取一致截面
            if (_stashCrossPositions.size() >= 2 && _stashCrossPositions.size() == _stashCrossShapeRows.size())
            {
                propertyData->_paraList = _stashCrossPositions;
                for (int row = 0; row < _stashCrossShapeRows.size(); ++row)
                    mapRelation.appendTableMapRelation({ Physical_Property, id }, { Physical_PropertyShape, _stashCrossShapeRows[row] }, row);
            }
            else
            {
                propertyData->_paraList = QStringList{ "0", "1" };
                mapRelation.appendTableMapRelation({ Physical_Property, id }, { Physical_PropertyShape, -1 }, 0);
                mapRelation.appendTableMapRelation({ Physical_Property, id }, { Physical_PropertyShape, -1 }, 1);
            }

            //更新变截面页面
            this->updateBeamSectionPanel();
            this->syncBeamElementPanel();
            //取消显示梁形状
            auto viewportPrivate = VMManager->getCurrentViewportPrivate();
            MechPreData::mPreViewportPrivate* preViewport = nullptr;
            if (viewportPrivate != nullptr)
            {
                preViewport = dynamic_cast<MechPreData::mPreViewportPrivate*>(viewportPrivate);
                if (preViewport == nullptr)
                {
                    return;
                }
            }
            for (const auto& part_id : _lastPickData)
            {
                if (preViewport->_entityRodBeam.find(part_id) == preViewport->_entityRodBeam.end())
                {
                    getCVM->getPreRenderWidget()->GetBeamSystem()->deleteBeam(QString("rod_%1").arg(part_id));
                }
            }
            getCVM->getPreRenderWidget()->updatePaint();
        }
        QTimer::singleShot(0, fold, [=]() {
            fold->refreshLayout();
            });
    }

    void mPropertyPanel::slot_beamComboBoxChanged(const QString& text)
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (_propertyData == nullptr)
        {
            return;
        }

        auto sectionShape = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(text);
        if (sectionShape == nullptr)
        {
            _beamStage->showSection(nullptr);
            return;
        }
        //存储到映射关系中
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        mapRelation.appendMapRelation({ Physical_Property, _propertyData->getPropertyId() }, { Physical_PropertyShape, sectionShape->getSectionShapeId() });
        _beamStage->hideQuickCreate();
        updateSectionPanel();
        updateRotateAngleRend();
        updateBeamEntrySummaries();
        //切换截面后参数数量变化，弹窗高度自适应
        if (_beamSectionPopup != nullptr && _beamSectionPopup->isVisible())
            _beamSectionPopup->fitToContentDeferred(true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, _propertyData->getPropertyId());
    }

    void mPropertyPanel::slot_allPartBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        QStringList partList = getCVM->getModelData()->getMeshData()->getAllPartNames();
        _listAssignPart->clear();
        _listAssignPart->addItems(partList);

        //把所有属性都添加到部件中去
        if (getCVM == nullptr)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        std::vector<int> partIds;
        for (auto partName : partList)
        {
            int partId = meshData->getPartIdByPartName(partName);
            if (partId == -1)
            {
                continue;
            }
            partIds.push_back(partId);
            mapRelation.appendMapRelation({ Physical_Part, partId }, { Physical_Property, id });
        }

        highlightPart(partIds);
        assignElement(partIds);
        updatePartAssignPanel();
        getCVM->updatePartShowColor(PartColorType::MatColor, partIds);
        getCVM->getDataStatusManager()->updateDynamicInfoData({ {Physical_Part, partIds} });

        //打印赋予信息
        if (!partIds.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("截面属性【%1】已赋予全部%2个部件！").arg(_propertyData->getPropertyName()).arg(static_cast<int>(partIds.size())));
        }
    }

    void mPropertyPanel::slot_addPartBtn()
    {
        QStringList partList = getCVM->getModelData()->getMeshData()->getAllPartNames();
        _partModel->setStringList(partList);
        // 1) 插入 item
        QListWidgetItem* newItem = new QListWidgetItem();
        _listAssignPart->insertItem(0, newItem);

        // 2) 创建编辑器并设置为 item 的 widget
        QLineEdit* editor = new QLineEdit(_listAssignPart);
        _listAssignPart->setItemWidget(newItem, editor);
        editor->setFocus();

        // 3) 给每个 editor 单独创建 model + completer（parent 为 editor）
        QCompleter* completer = new QCompleter(_partModel, editor);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        editor->setCompleter(completer);

        completer->complete();
        //QTimer::singleShot(0, editor, [=]() {
        //    completer->complete();
        //    });

        QPointer<QLineEdit> editorGuard(editor);
        // finishFlag 防止 finishEditing 被重复执行
        auto finishFlag = std::make_shared<bool>(false);

        // helper：安全地结束编辑（幂等）
        std::function<void()> finishEditing = [=]() mutable {
            if (*finishFlag)
                return;// 已经完成过一次，直接返回
            *finishFlag = true;// 标志为已完成

            // 如果 editor 已经被删除，没必要继续
            if (!editorGuard) {
                // 尝试把 item 从列表里移除（如果仍存在）
                int r = _listAssignPart->row(newItem);
                if (r >= 0) {
                    delete _listAssignPart->takeItem(r);
                }
                return;
            }

            // 先查 item 是否还在列表中
            int row = _listAssignPart->row(newItem);
            if (row < 0) {
                // item 不在列表
                editorGuard->deleteLater();
                return;
            }

            _listAssignPart->removeItemWidget(newItem);

            QString text = editorGuard->text().trimmed();

            // 1) 空文本 -> 删除该行
            if (text.isEmpty()) {
                delete _listAssignPart->takeItem(row);
                editorGuard->deleteLater();
                return;
            }

            // 2) 非候选 -> 删除
            QStringList candidates = _partModel->stringList();
            if (!candidates.contains(text)) {
                delete _listAssignPart->takeItem(row);
                editorGuard->deleteLater();
                return;
            }

            // 3) 去重：检查列表中是否存在相同文本
            bool exists = false;
            for (int i = 0; i < _listAssignPart->count(); ++i) {
                QListWidgetItem* it = _listAssignPart->item(i);
                if (!it)
                    continue;
                if (it == newItem)
                    continue;
                if (it->text() == text)
                {
                    exists = true; break;
                }
            }
            if (exists) {
                delete _listAssignPart->takeItem(row);
                editorGuard->deleteLater();
                return;
            }
            // 通过校验 -> 写入 item 文本，保留 item
            newItem->setText(text);
            int partId = this->updatePartProperty(text);
            if (partId != -1)
            {
                highlightPart(vector<int>{partId});
                assignElement(vector<int>{partId});
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Part, partId);

                //打印赋予信息
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("截面属性【%1】已赋予部件【%2】！").arg(_propertyData->getPropertyName()).arg(text));
            }
            editorGuard->deleteLater();
            };
        // completer 激活时（鼠标点击/回车选项）
        connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
            editor,  // receiver
            [=](const QString& choice) {
                if (!editorGuard) return;
                editorGuard->setText(choice);
                finishEditing();
            }, Qt::UniqueConnection);

        connect(editor, &QLineEdit::editingFinished, editor, [=]() {
            finishEditing();
            }, Qt::UniqueConnection);

        editor->installEventFilter(new QObject(editor));

    }

    void mPropertyPanel::slot_reducePartBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        vector<int> partIds;
        QStringList removedPartNames;
        auto items = _listAssignPart->selectedItems();
        for (QListWidgetItem* item : items) {
            if (!item) continue;

            QString text = item->text().trimmed();
            if (!text.isEmpty()) {
            }

            // 从 UI 中删除
            delete _listAssignPart->takeItem(_listAssignPart->row(item));

            int partId = meshData->getPartIdByPartName(text);
            if (partId == -1)
            {
                continue;
            }
            partIds.push_back(partId);
            removedPartNames << text;
            mapRelation.deleteMapRelation({ Physical_Part, partId });   //删除映射关系
        }

        unHighlightPart(partIds);

        //打印取消赋予信息
        if (!partIds.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("截面属性【%1】已取消赋予部件【%2】！").arg(_propertyData->getPropertyName()).arg(removedPartNames.join(",")));
        }
        getCVM->updatePartShowColor(PartColorType::MatColor, partIds);
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Part, partIds } });
    }

    void mPropertyPanel::slot_clearPartBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        vector<int> partIds;
        QStringList removedItems;

        // 收集所有已有的字符串
        for (int i = 0; i < _listAssignPart->count(); ++i) {
            QListWidgetItem* item = _listAssignPart->item(i);
            if (item && !item->text().trimmed().isEmpty()) {
                removedItems << item->text().trimmed();
            }
        }

        // 从属性列表里移除这些字符串
        for (const QString& s : removedItems) {
            int partId = meshData->getPartIdByPartName(s);
            if (partId == -1)
            {
                continue;
            }
            partIds.push_back(partId);
            mapRelation.deleteMapRelation({ Physical_Part, partId });//删除映射关系
        }
        unHighlightPart(partIds);

        // 清空 UI
        _listAssignPart->clear();

        getCVM->updatePartShowColor(PartColorType::MatColor, partIds);
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Part, partIds } });

        //打印清空信息
        if (!partIds.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("截面属性【%1】已取消赋予全部部件：【%2】！").arg(_propertyData->getPropertyName()).arg(removedItems.join(",")));
        }
    }

    void mPropertyPanel::slot_newMaterial()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        auto propertyData = _propertyData;
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMaterial();

        auto& materialManager = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material);

        auto matIDs = materialManager.getAllMatID();
        std::sort(matIDs.begin(), matIDs.end());
        if (matIDs.size() == 0)
        {
            return;
        }
        auto mat = materialManager.getMaterialData(matIDs.at(matIDs.size() - 1));
        if (mat == nullptr)
        {
            return;
        }
        _comboMat->setCurrentText(mat->getMaterialName());
        //设置id

        int id = propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        //int matId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_Material);
        mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_Material, mat->getMaterialId() });
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Property, id);

        auto partList = mapRelation.getMapRelation<QList<int>>({ Physical_Property, id }, Physical_Part);
        std::vector<int> partIds{ partList.begin(), partList.end() };
        getCVM->updatePartShowColor(PartColorType::MatColor, partIds);
    }

    void mPropertyPanel::slot_newPropertyShape()
    {
        auto propertyData = _propertyData;
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createSectionShape(MxFunctions::Sec_H);

        auto& sectionShapeManager = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape);

        auto shapeIDs = sectionShapeManager.getAllSectionShapeID();
        std::sort(shapeIDs.begin(), shapeIDs.end());
        if (shapeIDs.size() == 0)
        {
            return;
        }
        auto sectionShape = sectionShapeManager.getSectionShapeData(shapeIDs.at(shapeIDs.size() - 1));
        if (sectionShape == nullptr)
        {
            return;
        }
        _comboPropShape->setCurrentText(sectionShape->getSectionShapeName());
        //设置id

        int id = propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        //int sectionShapeId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_PropertyShape);
        mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_PropertyShape, sectionShape->getSectionShapeId() });
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Property, id);
    }

    void mPropertyPanel::slot_subWindowActivated(int viewportID)
    {
        //信号签名由(QMdiSubWindow*)改为(int viewportID)，此槽为空操作占位
        Q_UNUSED(viewportID);
        //if (_currentViewportPrivate == VMManager->getCurrentViewportPrivate())
        //{
        //    return;
        //}
        //connectSig();

    }
    void mPropertyPanel::connectSig()
    {
        disconnectSig();
        if (!getCVM)
        {
            return;
        }
        conn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mPropertyPanel::slot_finishedPick);
        connectAngleRotateUpdate();
        connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mPropertyPanel::slot_tableData);
    }
    void mPropertyPanel::disconnectSig()
    {
        disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mPropertyPanel::slot_tableData);
        if (conn)
            disconnect(conn);
    }
    int mPropertyPanel::updatePartProperty(QString partName)
    {
        if (getCVM == nullptr)
        {
            return -1;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        int partId = meshData->getPartIdByPartName(partName);
        if (partId == -1)
        {
            return partId;
        }
        mapRelation.appendMapRelation({ Physical_Part, partId }, { Physical_Property, id });
        getCVM->updatePartShowColor(PartColorType::MatColor, { partId });
        updateDynamicInfoData();
        return partId;
    }
    void mPropertyPanel::updateSectionPanel()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (_propertyData == nullptr)
        {
            return;
        }
        if (_propertyData->getPropertyType() != ResultantProperty)
        {
            return;
        }
        //刷新下拉框(全部截面形状，含自定义)
        updatePropShapeCombobox(_comboPropShape);
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int propShapeId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_PropertyShape);
        auto& sectionShapeManager = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape);
        MechSectionShapeData::mSectionShapeBase* sectionShape = nullptr;
        if (propShapeId != -1)
        {
            sectionShape = sectionShapeManager.getSectionShapeData(propShapeId);
        }
        _comboPropShape->blockSignals(true);//避免重复进updateSectionPanel
        _comboPropShape->setCurrentIndex(-1);
        if (sectionShape)
        {
            _comboPropShape->setCurrentText(sectionShape->getSectionShapeName());
        }
        _comboPropShape->blockSignals(false);

        _beamStage->hideQuickCreate();
        _btnQuickCreate->setChecked(false);
        _beamStage->showSection(sectionShape);

        //渲染接入(连接角度变化的信号和槽函数)
        connectAngleRotateUpdate();

        QTimer::singleShot(0, m_foldWidgets[Fold_BeamSection], [this]() {
            m_foldWidgets[Fold_BeamSection]->refreshLayout();
            });

        updateRotateAngleRend();
    }

    void mPropertyPanel::buildBeamSectionFold(FoldSection* fold)
    {
        //========== 属性面板条目入口(点击弹出吸附式弹窗) ==========
        _btnSectionEntry = new QPushButton(fold);
        _btnSectionEntry->setCheckable(true);
        _btnSectionEntry->setFixedHeight(32);
        _btnSectionEntry->setStyleSheet(QStringLiteral(
            "QPushButton{background:#ffffff;border:1px solid #dce2e9;border-radius:2px;text-align:left;}"
            "QPushButton:hover,QPushButton:checked{border-color:#1890ff;background:#e8f3ff;}"));
        _btnSectionEntry->setToolTip(QStringLiteral("打开梁截面定义"));
        {
            auto entryLay = new QHBoxLayout(_btnSectionEntry);
            entryLay->setContentsMargins(8, 0, 8, 0);
            entryLay->setSpacing(6);
            auto title = new QLabel(QStringLiteral("梁截面定义"), _btnSectionEntry);
            title->setStyleSheet("border:0;background:transparent;");
            _entrySectionSummary = new QLabel(QStringLiteral("—"), _btnSectionEntry);
            _entrySectionSummary->setStyleSheet("color:#5c6674;border:0;background:transparent;");
            auto arrow = new QLabel(QStringLiteral("›"), _btnSectionEntry);
            arrow->setStyleSheet("color:#1890ff;font-size:16px;border:0;background:transparent;");
            for (QWidget* w : { (QWidget*)title, (QWidget*)_entrySectionSummary, (QWidget*)arrow })
            {
                w->setAttribute(Qt::WA_TransparentForMouseEvents);
                w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                w->setFixedHeight(20);
            }
            entryLay->addWidget(title);
            entryLay->addSpacing(4);
            entryLay->addWidget(_entrySectionSummary, 1);
            entryLay->addWidget(arrow);
        }
        fold->addRow(new PanelLabel(QStringLiteral("截面定义")), _btnSectionEntry);

        //========== 弹窗内容：选择区+位置区+参数区 ==========
        auto content = new QWidget();
        content->setContentsMargins(12, 10, 12, 12);
        auto contentLay = new QVBoxLayout(content);
        contentLay->setSpacing(10);

        //一致：梁截面行
        _comboPropShape = new QComboBox();
        _comboPropShape->setModel(_propShapeModel);
        _btnQuickCreate = new QPushButton(QStringLiteral("快速新建"));
        _btnQuickCreate->setCheckable(true);
        _btnQuickCreate->setToolTip(QStringLiteral("选择形状快速新建截面"));
        _uniformSelectionRow = new QWidget(content);
        {
            auto lay = new QHBoxLayout(_uniformSelectionRow);
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(6);
            lay->addWidget(new QLabel(QStringLiteral("梁截面"), _uniformSelectionRow));
            lay->addWidget(_comboPropShape, 1);
            lay->addWidget(_btnQuickCreate);
        }

        //变化：统一形状行 + 位置编辑工具
        _variableSelectionRow = new QWidget(content);
        {
            auto lay = new QHBoxLayout(_variableSelectionRow);
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(6);
            lay->addWidget(new QLabel(QStringLiteral("截面形状"), _variableSelectionRow));
            _btnVariableShape = new QToolButton(_variableSelectionRow);
            _btnVariableShape->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            _btnVariableShape->setIconSize(QSize(20, 20));
            _btnVariableShape->setToolTip(QStringLiteral("统一截面形状(各位置仅可关联同形状截面)"));
            _btnVariableShape->setMinimumSize(70, 26);
            _btnInsertStation = new QPushButton(QStringLiteral("插入"), _variableSelectionRow);
            _btnInsertStation->setToolTip(QStringLiteral("在当前截面后插入位置"));
            _btnDeleteStation = new QPushButton(QStringLiteral("删除"), _variableSelectionRow);
            _btnDeleteStation->setToolTip(QStringLiteral("删除当前选中的中间位置"));
            _btnClearStation = new QPushButton(QStringLiteral("清空"), _variableSelectionRow);
            _btnClearStation->setToolTip(QStringLiteral("清空中间截面，保留 A、B 端"));
            lay->addWidget(_btnVariableShape);
            lay->addWidget(_btnInsertStation);
            lay->addWidget(_btnDeleteStation);
            lay->addWidget(_btnClearStation);
            lay->addStretch();
        }

        //选择行：轮廓类型 + (一致/变化行)，上下加浅灰分隔线
        auto makeSeparator = [content]() {
            auto separator = new QFrame(content);
            separator->setFixedHeight(1);
            separator->setStyleSheet("background:#e9edf3;border:0;");
            return separator;
            };
        auto selectorRow = new QWidget(content);
        {
            auto lay = new QHBoxLayout(selectorRow);
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(8);
            lay->addWidget(new QLabel(QStringLiteral("轮廓类型"), selectorRow));
            lay->addWidget(_comboBeamType);
            lay->addSpacing(6);
            lay->addWidget(_uniformSelectionRow, 1);
            lay->addWidget(_variableSelectionRow, 1);
        }
        contentLay->addWidget(makeSeparator());
        contentLay->addWidget(selectorRow);
        contentLay->addWidget(makeSeparator());

        //变化：位置行区域
        _stationWidget = new QWidget(content);
        {
            auto stationLay = new QVBoxLayout(_stationWidget);
            stationLay->setContentsMargins(0, 0, 0, 0);
            stationLay->setSpacing(4);
            //表头(位置 x/L | 梁截面)
            auto head = new QWidget(_stationWidget);
            head->setStyleSheet("background:#e9edf3;color:#687383;font-size:11px;border-radius:2px;");
            auto headLay = new QHBoxLayout(head);
            headLay->setContentsMargins(6, 3, 6, 3);
            headLay->setSpacing(6);
            headLay->addWidget(new QLabel(QStringLiteral("位置 x/L"), head), 1);
            headLay->addWidget(new QLabel(QStringLiteral("梁截面"), head), 2);
            stationLay->addWidget(head);
            _stationRowsLayout = new QGridLayout();
            _stationRowsLayout->setContentsMargins(0, 0, 0, 0);
            _stationRowsLayout->setVerticalSpacing(4);
            stationLay->addLayout(_stationRowsLayout);
            _stationErrorLabel = new QLabel(_stationWidget);
            _stationErrorLabel->setStyleSheet("color:#d95151;font-size:11px;");
            _stationErrorLabel->setWordWrap(true);
            _stationErrorLabel->hide();
            stationLay->addWidget(_stationErrorLabel);
        }
        contentLay->addWidget(_stationWidget);

        //参数编辑区(参数+图纸+快速新建)
        _beamStage = new BeamSectionStage(content);
        _beamStage->onCreateShape = [this](MxFunctions::SecType type) {
            auto shape = quickCreateSectionShape(type);
            if (shape == nullptr)
                return;
            //刷新下拉模型并选中新截面(程序设置不触发槽)，随后直接调槽完成映射与参数区同步，停留在本弹窗
            updatePropShapeCombobox(_comboPropShape);
            {
                QSignalBlocker blocker(_comboPropShape);
                _comboPropShape->setCurrentText(shape->getSectionShapeName());
            }
            slot_beamComboBoxChanged(shape->getSectionShapeName());
            updateBeamEntrySummaries();
            if (_beamSectionPopup != nullptr && _beamSectionPopup->isVisible())
                _beamSectionPopup->fitToContentDeferred(true);
            };
        _beamStage->onParamCommit = [this]() {
            updateRotateAngleRend();
            updateDynamicInfoData();
            };
        _beamErrorLabel = new QLabel(content);
        _beamErrorLabel->setStyleSheet("color:#d95151;font-size:11px;");
        _beamErrorLabel->setWordWrap(true);
        _beamErrorLabel->hide();
        {
            auto lay = new QWidget(content);
            auto vLay = new QVBoxLayout(lay);
            vLay->setContentsMargins(0, 0, 0, 0);
            vLay->setSpacing(4);
            vLay->addWidget(_beamStage);
            vLay->addWidget(_beamErrorLabel);
            contentLay->addWidget(lay);
        }
        contentLay->addStretch();

        //吸附弹窗
        _beamSectionPopup = new AnchoredPopup(QStringLiteral("梁截面定义"), content, this);
        _beamSectionPopup->setAnchor(_btnSectionEntry);
        _beamSectionPopup->setPreferredWidth(520);
        _beamSectionPopup->allowClose = [this]() {
            //参数/位置非法时阻止关闭并提示
            return canLeaveSectionEditor();
            };
        _beamSectionPopup->keepOpen = [this]() { return beamPopupPickActive(); };
        _beamSectionPopup->onClosed = [this]() { _btnSectionEntry->setChecked(false); };

        connect(_btnSectionEntry, &QPushButton::clicked, this, [this]() { toggleBeamSectionPopup(); });

        //快速新建开关
        connect(_btnQuickCreate, &QPushButton::clicked, this, [this]() {
            if (_beamStage->isQuickCreateVisible())
            {
                _beamStage->hideQuickCreate();
                _btnQuickCreate->setChecked(false);
            }
            else
            {
                _beamStage->showQuickCreate();
                _btnQuickCreate->setChecked(true);
            }
            //网格展开/收起改变内容高度，弹窗自适应
            if (_beamSectionPopup != nullptr && _beamSectionPopup->isVisible())
                _beamSectionPopup->fitToContentDeferred(true);
            });

        //变截面统一形状下拉
        connect(_btnVariableShape, &QToolButton::clicked, this, [this]() {
            auto popup = new ShapeGridPopup(this);
            popup->setAttribute(Qt::WA_DeleteOnClose);
            popup->onPick = [this](MxFunctions::SecType type) { changeVariableShape(type); };
            popup->popupBelow(_btnVariableShape);
            });

        connect(_btnInsertStation, &QPushButton::clicked, this, [this]() { insertStation(); });
        connect(_btnDeleteStation, &QPushButton::clicked, this, [this]() { removeStation(); });
        connect(_btnClearStation, &QPushButton::clicked, this, [this]() { clearStations(); });
        //截面形状按钮初始显示(图标+文字)
        syncVariableShapeButton();

        //折叠块收起时关闭吸附式弹窗
        connect(fold, &FoldWidget::foldExpandChanged, this, [this](bool expand) {
            if (!expand)
                closeBeamPopups();
            });
    }

    void mPropertyPanel::buildBeamElementFold(FoldSection* fold)
    {
        //========== 属性面板条目入口 ==========
        _btnElementEntry = new QPushButton(fold);
        _btnElementEntry->setCheckable(true);
        _btnElementEntry->setFixedHeight(32);
        _btnElementEntry->setStyleSheet(QStringLiteral(
            "QPushButton{background:#ffffff;border:1px solid #dce2e9;border-radius:2px;text-align:left;}"
            "QPushButton:hover,QPushButton:checked{border-color:#1890ff;background:#e8f3ff;}"));
        _btnElementEntry->setToolTip(QStringLiteral("打开梁单元参数"));
        {
            auto entryLay = new QHBoxLayout(_btnElementEntry);
            entryLay->setContentsMargins(8, 0, 8, 0);
            entryLay->setSpacing(6);
            auto title = new QLabel(QStringLiteral("梁单元参数"), _btnElementEntry);
            title->setStyleSheet("border:0;background:transparent;");
            _entryElementSummary = new QLabel(QStringLiteral("—"), _btnElementEntry);
            _entryElementSummary->setStyleSheet("color:#5c6674;border:0;background:transparent;");
            auto arrow = new QLabel(QStringLiteral("›"), _btnElementEntry);
            arrow->setStyleSheet("color:#1890ff;font-size:16px;border:0;background:transparent;");
            for (QWidget* w : { (QWidget*)title, (QWidget*)_entryElementSummary, (QWidget*)arrow })
            {
                w->setAttribute(Qt::WA_TransparentForMouseEvents);
                w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                w->setFixedHeight(20);
            }
            entryLay->addWidget(title);
            entryLay->addSpacing(4);
            entryLay->addWidget(_entryElementSummary, 1);
            entryLay->addWidget(arrow);
        }
        fold->addRow(new PanelLabel(QStringLiteral("参数设置")), _btnElementEntry);

        //========== 弹窗内容：梁单元参数面板 ==========
        _beamElementPanel = new BeamElementPanel();
        _beamElementPanel->onElementDataChanged = [this]() {
            if (_propertyData == nullptr)
                return;
            updateDynamicInfoData();
            updateRotateRender();
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, _propertyData->getPropertyId());
            };
        auto content = new QWidget();
        content->setContentsMargins(10, 10, 10, 10);
        auto contentLay = new QVBoxLayout(content);
        contentLay->setContentsMargins(0, 0, 0, 0);
        contentLay->addWidget(_beamElementPanel);

        //吸附弹窗(宽度随表格列宽自适应)
        _beamElementPopup = new AnchoredPopup(QStringLiteral("梁单元参数"), content, this);
        _beamElementPopup->setAnchor(_btnElementEntry);
        _beamElementPopup->setPreferredWidth(0);
        _beamElementPopup->allowClose = [this]() {
            //单元数据非法时阻止关闭并提示
            QString error;
            if (_beamElementPanel && !_beamElementPanel->validateRows(&error))
            {
                _beamElementPanel->showTableErrorExternal(error);
                return false;
            }
            return true;
            };
        _beamElementPopup->keepOpen = [this]() { return beamPopupPickActive(); };
        _beamElementPopup->onClosed = [this]() { _btnElementEntry->setChecked(false); };

        //标题栏行数计数
        QLabel* headerCount = _beamElementPopup->addHeaderInfo(QStringLiteral("0 行"));
        _beamElementPanel->onRowCountChanged = [headerCount](int count) {
            headerCount->setText(QStringLiteral("%1 行").arg(count));
            };
        //行数/偏置状态变化时弹窗随表格宽度自适应
        _beamElementPanel->onContentResized = [this]() {
            if (_beamElementPopup != nullptr && _beamElementPopup->isVisible())
            {
                _beamElementPopup->fitToContent(true);
                //子控件最小宽度变化触发的布局失效是排队事件，落定后再复核一次宽度
                QTimer::singleShot(0, this, [this]() {
                    if (_beamElementPopup != nullptr && _beamElementPopup->isVisible())
                        _beamElementPopup->fitToContent(true);
                    });
            }
            };
        //返回按钮：拾取中放弃本次拾取(回滚到拾取前)，否则关闭弹窗
        _beamElementPopup->enableBackButton([this]() {
            if (_beamElementPanel != nullptr && _beamElementPanel->isPicking())
                _beamElementPanel->cancelPick();
            else
                _beamElementPopup->requestClose();
            });
        //拾取中标题切换为"单元数据"并隐藏行计数，弹窗随表格(仅单元编号列)自适应
        _beamElementPanel->onPickStateChanged = [this, headerCount](bool picking) {
            if (_beamElementPopup == nullptr)
                return;
            _beamElementPopup->setTitle(picking ? QStringLiteral("单元数据") : QStringLiteral("梁单元参数"));
            headerCount->setVisible(!picking);
            _beamElementPopup->fitToContent(true);
            };

        connect(_btnElementEntry, &QPushButton::clicked, this, [this]() { toggleBeamElementPopup(); });
    }

    void mPropertyPanel::toggleBeamSectionPopup()
    {
        if (_beamSectionPopup == nullptr || _propertyData == nullptr)
            return;
        if (_beamSectionPopup->isVisible())
        {
            _beamSectionPopup->requestClose();
            return;
        }
        //打开前先关闭梁单元参数弹窗(其数据非法则阻止)
        if (_beamElementPopup != nullptr && _beamElementPopup->isVisible())
        {
            if (!_beamElementPopup->requestClose())
                return;
        }
        updateBeamSectionPanel();
        _btnSectionEntry->setChecked(true);
        _beamSectionPopup->popup();
    }

    void mPropertyPanel::toggleBeamElementPopup()
    {
        if (_beamElementPopup == nullptr || _propertyData == nullptr)
            return;
        if (_beamElementPopup->isVisible())
        {
            _beamElementPopup->requestClose();
            return;
        }
        //打开前先关闭梁截面定义弹窗(其参数/位置非法则阻止)
        if (_beamSectionPopup != nullptr && _beamSectionPopup->isVisible())
        {
            if (!_beamSectionPopup->requestClose())
                return;
        }
        syncBeamElementPanel();
        if (_beamElementPanel)
            _beamElementPanel->resetInteraction();
        _btnElementEntry->setChecked(true);
        _beamElementPopup->popup();
    }

    void mPropertyPanel::closeBeamPopups()
    {
        if (_beamSectionPopup != nullptr && _beamSectionPopup->isVisible())
        {
            _beamSectionPopup->allowClose = nullptr;
            _beamSectionPopup->hide();
            _beamSectionPopup->restoreFocus();
            if (_btnSectionEntry)
                _btnSectionEntry->setChecked(false);
        }
        if (_beamElementPopup != nullptr && _beamElementPopup->isVisible())
        {
            _beamElementPopup->allowClose = nullptr;
            _beamElementPopup->hide();
            _beamElementPopup->restoreFocus();
            if (_beamElementPanel)
                _beamElementPanel->resetInteraction();
            if (_btnElementEntry)
                _btnElementEntry->setChecked(false);
        }
    }

    void mPropertyPanel::updateBeamEntrySummaries()
    {
        if (_entrySectionSummary == nullptr || _entryElementSummary == nullptr)
            return;
        QString sectionText = QStringLiteral("—");
        if (_propertyData != nullptr && getCVM != nullptr)
        {
            if (_propertyData->getPropertyType() == CrossProperty)
            {
                auto crossBeam = toType<mCrossBeam>(_propertyData);
                if (crossBeam)
                    sectionText = QStringLiteral("变化 · %1 个位置点").arg(crossBeam->_paraList.size());
            }
            else
            {
                auto shape = currentEditSection();
                if (shape)
                    sectionText = shape->getSectionShapeName();
            }
        }
        _entrySectionSummary->setText(sectionText);
        QString elementText = QStringLiteral("—");
        if (_beamElementPanel != nullptr && _beamElementPanel->rowCount() > 0)
            elementText = QStringLiteral("%1 个单元").arg(_beamElementPanel->rowCount());
        _entryElementSummary->setText(elementText);
    }

    bool mPropertyPanel::beamPopupPickActive()
    {
        return _beamElementPopup != nullptr && _beamElementPopup->isVisible()
            && _beamElementPanel != nullptr
            && (_beamElementPanel->isPicking() || _beamElementPanel->isDirectionPickingActive());
    }

    void mPropertyPanel::syncBeamElementPanel()
    {
        if (_beamElementPanel == nullptr)
        {
            return;
        }
        if (getCVM == nullptr || _propertyData == nullptr)
        {
            _beamElementPanel->setScope(-1, {});
            return;
        }
        PropertyType type = _propertyData->getPropertyType();
        if (type != ResultantProperty && type != CrossProperty)
        {
            _beamElementPanel->setScope(-1, {});
            return;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        QList<int> partList = mapRelation.getMapRelation<QList<int>>({ Physical_Property, id }, Physical_Part);
        std::vector<int> partIds(partList.begin(), partList.end());
        _beamElementPanel->setScope(id, partIds);
        updateBeamEntrySummaries();
    }

    void mPropertyPanel::updateBeamSectionPanel()
    {
        if (getCVM == nullptr || _propertyData == nullptr)
        {
            return;
        }
        bool variable = (_propertyData->getPropertyType() == CrossProperty);
        {
            QSignalBlocker blocker(_comboBeamType);
            _comboBeamType->setCurrentText(variable ? QStringLiteral("变化") : QStringLiteral("一致"));
        }
        _beamStage->hideQuickCreate();
        _btnQuickCreate->setChecked(false);
        _uniformSelectionRow->setVisible(!variable);
        _variableSelectionRow->setVisible(variable);
        _stationWidget->setVisible(variable);
        _stationErrorLabel->hide();
        _beamErrorLabel->hide();
        if (variable)
        {
            syncVariableSectionUI();
        }
        else
        {
            syncUniformSectionUI();
        }
        updateBeamEntrySummaries();
        //弹窗打开时尺寸随内容(一致/变化模式内容不同)自适应，内部已含落定后复核
        if (_beamSectionPopup != nullptr && _beamSectionPopup->isVisible())
            _beamSectionPopup->fitToContentDeferred(true);
        QTimer::singleShot(0, m_foldWidgets[Fold_BeamSection], [this]() {
            m_foldWidgets[Fold_BeamSection]->refreshLayout();
            });
    }

    void mPropertyPanel::syncUniformSectionUI()
    {
        updateSectionPanel();
    }

    void mPropertyPanel::syncVariableSectionUI()
    {
        auto crossBeam = toType<mCrossBeam>(_propertyData);
        if (crossBeam == nullptr)
        {
            return;
        }
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto& sectionShapeManager = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape);
        int pid = _propertyData->getPropertyId();
        QList<int> beamIds = mapRelation.getMapRelation<QList<int>>({ Physical_Property, pid }, Physical_PropertyShape);
        //映射行数与位置数保持一致
        while (beamIds.size() < crossBeam->_paraList.size())
        {
            mapRelation.appendTableMapRelation({ Physical_Property, pid }, { Physical_PropertyShape, -1 }, beamIds.size());
            beamIds.append(-1);
        }
        while (beamIds.size() > crossBeam->_paraList.size())
        {
            mapRelation.deleteTabelMapRelation({ Physical_Property, pid }, { Physical_PropertyShape, -1 }, beamIds.size() - 1);
            beamIds.removeLast();
        }
        //统一形状取第一个有效截面行的类型
        MxFunctions::SecType shape = MxFunctions::Sec_H;
        for (int shapeId : beamIds)
        {
            if (shapeId == -1)
                continue;
            auto section = sectionShapeManager.getSectionShapeData(shapeId);
            if (section && section->getSectionShapeType() != MxFunctions::Sec_None)
            {
                shape = section->getSectionShapeType();
                break;
            }
        }
        _variableShape = shape;
        _stationShapeCache.clear();
        syncVariableShapeButton();
        renderStationRows();
        int active = qBound(0, _activeStation, crossBeam->_paraList.size() - 1);
        _activeStation = -1;
        selectStation(active);
    }

    void mPropertyPanel::syncVariableShapeButton()
    {
        const BeamShapeInfo* info = beamShapeInfo(_variableShape);
        if (info == nullptr || _btnVariableShape == nullptr)
            return;
        //左侧图标 + 右侧文字
        _btnVariableShape->setIcon(QIcon(info->icon));
        _btnVariableShape->setText(info->label);
    }

    QLineEdit* mPropertyPanel::stationPosEdit(int row)
    {
        if (_stationRowsLayout == nullptr)
            return nullptr;
        QLayoutItem* item = _stationRowsLayout->itemAtPosition(row, 0);
        if (item == nullptr || item->widget() == nullptr)
            return nullptr;
        return item->widget()->findChild<QLineEdit*>();
    }

    int mPropertyPanel::stationShapeId(int row)
    {
        if (getCVM == nullptr || _propertyData == nullptr)
            return -1;
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        QList<int> beamIds = mapRelation.getMapRelation<QList<int>>({ Physical_Property, _propertyData->getPropertyId() }, Physical_PropertyShape);
        if (row < 0 || row >= beamIds.size())
            return -1;
        return beamIds[row];
    }

    void mPropertyPanel::renderStationRows()
    {
        if (getCVM == nullptr || _propertyData == nullptr)
            return;
        auto crossBeam = toType<mCrossBeam>(_propertyData);
        if (crossBeam == nullptr)
            return;
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto& sectionShapeManager = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape);
        int pid = _propertyData->getPropertyId();
        QList<int> beamIds = mapRelation.getMapRelation<QList<int>>({ Physical_Property, pid }, Physical_PropertyShape);
        QStringList shapeNames = sectionShapeManager.getAllSectionShapeNamesByTypes(_variableShape);
        //清空旧行
        while (QLayoutItem* item = _stationRowsLayout->takeAt(0))
        {
            if (QWidget* widget = item->widget())
                widget->deleteLater();
            delete item;
        }
        int count = crossBeam->_paraList.size();
        for (int i = 0; i < count; ++i)
        {
            bool isEnd = (i == 0 || i == count - 1);
            QString stationName = (i == 0) ? QStringLiteral("A端") : (isEnd ? QStringLiteral("B端") : QString::number(i));
            auto frame = new QFrame(_stationWidget);
            frame->setObjectName(QStringLiteral("stationRow"));
            frame->setStyleSheet("QFrame#stationRow{background:#ffffff;border:1px solid #dce2e9;border-radius:2px;}"
                "QFrame#stationRow[active=\"true\"]{background:#e8f3ff;border-color:#91caff;}");
            frame->setProperty("active", i == _activeStation);
            frame->setProperty("stationRow", i);
            auto lay = new QHBoxLayout(frame);
            lay->setContentsMargins(4, 2, 4, 2);
            lay->setSpacing(4);
            auto nameBtn = new QToolButton(frame);
            nameBtn->setText(stationName);
            nameBtn->setToolTip(QStringLiteral("选择%1截面").arg(stationName));
            nameBtn->setStyleSheet("QToolButton{border:0;background:transparent;color:#5c6674;font-size:11px;}"
                "QToolButton:hover{color:#1890ff;}");
            auto posEdit = new QLineEdit(crossBeam->_paraList[i], frame);
            posEdit->setPlaceholderText(QStringLiteral("0~1"));
            posEdit->setStyleSheet("QLineEdit{border:1px solid #dce2e9;border-radius:2px;padding:1px 4px;}"
                "QLineEdit:focus{border-color:#91caff;}"
                "QLineEdit[invalid=\"true\"]{border-color:#d95151;}");
            if (isEnd)
            {
                posEdit->setReadOnly(true);
            }
            else
            {
                posEdit->setValidator(new QDoubleValidator(0.0, 1.0, 12, posEdit));
            }
            auto combo = new QComboBox(frame);
            combo->addItems(shapeNames);
            int shapeId = (i < beamIds.size()) ? beamIds[i] : -1;
            combo->setCurrentIndex(-1);
            if (shapeId != -1)
            {
                auto section = sectionShapeManager.getSectionShapeData(shapeId);
                if (section && combo->findText(section->getSectionShapeName()) >= 0)
                    combo->setCurrentText(section->getSectionShapeName());
            }
            auto createBtn = new QPushButton(QStringLiteral("新建"), frame);
            createBtn->setToolTip(QStringLiteral("新建%1梁截面").arg(stationName));
            lay->addWidget(nameBtn);
            lay->addWidget(posEdit, 1);
            lay->addWidget(combo, 2);
            lay->addWidget(createBtn);
            _stationRowsLayout->addWidget(frame, i, 0);
            //行内任意位置点击/聚焦时选中该行
            frame->installEventFilter(this);
            nameBtn->setProperty("stationRow", i);
            nameBtn->installEventFilter(this);
            posEdit->setProperty("stationRow", i);
            posEdit->installEventFilter(this);
            combo->setProperty("stationRow", i);
            combo->installEventFilter(this);
            createBtn->setProperty("stationRow", i);
            createBtn->installEventFilter(this);
            int row = i;
            connect(nameBtn, &QToolButton::clicked, this, [this, row]() { selectStation(row); });
            connect(posEdit, &QLineEdit::textChanged, this, [this, row](const QString& text) {
                if (_propertyData == nullptr)
                    return;
                auto crossBeamData = toType<mCrossBeam>(_propertyData);
                if (crossBeamData && row < crossBeamData->_paraList.size() && crossBeamData->_paraList[row] != text)
                {
                    crossBeamData->_paraList[row] = text;
                    validateStations(false);
                }
                });
            connect(combo, &QComboBox::currentTextChanged, this, [this, row, combo](const QString& name) {
                if (getCVM == nullptr || _propertyData == nullptr || name.isEmpty())
                    return;
                auto& mapRel = getCVM->getModelData()->getMapRelationManager();
                auto& mgr = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape);
                auto section = mgr.getSectionShapeData(name);
                int previousId = stationShapeId(row);
                QString previousName;
                if (previousId != -1)
                {
                    auto prev = mgr.getSectionShapeData(previousId);
                    if (prev)
                        previousName = prev->getSectionShapeName();
                }
                if (section == nullptr || section->getSectionShapeType() != _variableShape || !canLeaveSectionEditor())
                {
                    QSignalBlocker blocker(combo);
                    if (previousName.isEmpty())
                        combo->setCurrentIndex(-1);
                    else
                        combo->setCurrentText(previousName);
                    return;
                }
                mapRel.updateTabelMapRelation({ Physical_Property, _propertyData->getPropertyId() },
                    { Physical_PropertyShape, section->getSectionShapeId() }, row);
                _stationShapeCache[row][(int)_variableShape] = section->getSectionShapeId();
                selectStation(row);
                updateDynamicInfoData();
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, _propertyData->getPropertyId());
                });
            connect(createBtn, &QPushButton::clicked, this, [this, row]() {
                if (!canLeaveSectionEditor())
                    return;
                auto shape = quickCreateSectionShape(_variableShape);
                if (shape == nullptr || getCVM == nullptr || _propertyData == nullptr)
                    return;
                auto& mapRel = getCVM->getModelData()->getMapRelationManager();
                mapRel.updateTabelMapRelation({ Physical_Property, _propertyData->getPropertyId() },
                    { Physical_PropertyShape, shape->getSectionShapeId() }, row);
                _stationShapeCache[row][(int)_variableShape] = shape->getSectionShapeId();
                renderStationRows();
                _activeStation = -1;
                selectStation(row);
                QLineEdit* edit = stationPosEdit(row);
                if (edit == nullptr)
                {
                    QLayoutItem* item = _stationRowsLayout->itemAtPosition(row, 0);
                    if (item && item->widget())
                    {
                        auto comboOfRow = item->widget()->findChild<QComboBox*>();
                        if (comboOfRow)
                            comboOfRow->setFocus();
                    }
                }
                updateDynamicInfoData();
                });
        }
        updateStationButtons();
        //位置行数量变化，弹窗高度自适应
        if (_beamSectionPopup != nullptr && _beamSectionPopup->isVisible())
            _beamSectionPopup->fitToContentDeferred(true);
        QTimer::singleShot(0, m_foldWidgets[Fold_BeamSection], [this]() {
            m_foldWidgets[Fold_BeamSection]->refreshLayout();
            });
    }

    void mPropertyPanel::updateStationButtons()
    {
        if (_propertyData == nullptr || _btnDeleteStation == nullptr)
            return;
        auto crossBeam = toType<mCrossBeam>(_propertyData);
        if (crossBeam == nullptr)
            return;
        int count = crossBeam->_paraList.size();
        bool middleActive = _activeStation > 0 && _activeStation < count - 1;
        _btnDeleteStation->setEnabled(middleActive);
        _btnClearStation->setEnabled(count > 2);
    }

    bool mPropertyPanel::validateStations(bool focusFirst)
    {
        if (_propertyData == nullptr)
            return true;
        auto crossBeam = toType<mCrossBeam>(_propertyData);
        if (crossBeam == nullptr)
            return true;
        QString error;
        QLineEdit* firstBad = nullptr;
        double previous = -1.0;
        int count = crossBeam->_paraList.size();
        for (int i = 0; i < count; ++i)
        {
            QLineEdit* edit = stationPosEdit(i);
            QString raw = edit ? edit->text() : crossBeam->_paraList[i];
            if (i == 0)
            {
                previous = 0.0;
                continue;
            }
            if (i == count - 1)
            {
                //B端固定为1
                previous = 1.0;
                continue;
            }
            bool ok = false;
            double value = raw.trimmed().toDouble(&ok);
            bool bad = raw.trimmed().isEmpty() || !ok || value < 0.0 || value > 1.0 || value <= previous;
            if (edit)
                markLineEditInvalid(edit, bad);
            if (bad && firstBad == nullptr)
                firstBad = edit;
            previous = value;
        }
        if (firstBad)
            error = QStringLiteral("位置须为 0～1 内的数值，按从小到大排列且不可重复。");
        _stationErrorLabel->setText(error);
        _stationErrorLabel->setVisible(!error.isEmpty());
        if (firstBad && focusFirst)
            firstBad->setFocus();
        return error.isEmpty();
    }

    void mPropertyPanel::selectStation(int row)
    {
        if (getCVM == nullptr || _propertyData == nullptr)
            return;
        auto crossBeam = toType<mCrossBeam>(_propertyData);
        if (crossBeam == nullptr)
            return;
        int count = crossBeam->_paraList.size();
        if (row < 0 || row >= count)
            return;
        if (row == _activeStation)
            return;
        if (!canLeaveSectionEditor())
            return;
        _activeStation = row;
        //刷新行高亮
        for (int i = 0; i < count; ++i)
        {
            QLayoutItem* item = _stationRowsLayout->itemAtPosition(i, 0);
            if (item == nullptr || item->widget() == nullptr)
                continue;
            QWidget* frame = item->widget();
            frame->setProperty("active", i == row);
            frame->style()->unpolish(frame);
            frame->style()->polish(frame);
        }
        updateStationButtons();
        //显示该行截面参数；行未关联截面时回退显示统一形状类型的第一个已有截面(仅展示)，保证参数编辑区常驻
        auto& sectionShapeManager = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape);
        int shapeId = stationShapeId(row);
        MechSectionShapeData::mSectionShapeBase* section = (shapeId == -1) ? nullptr : sectionShapeManager.getSectionShapeData(shapeId);
        if (section == nullptr)
        {
            QStringList names = sectionShapeManager.getAllSectionShapeNamesByTypes(_variableShape);
            if (!names.isEmpty())
                section = sectionShapeManager.getSectionShapeData(names.first());
        }
        _beamStage->showSection(section);
        //各位置截面参数数量可能不同，弹窗高度自适应
        if (_beamSectionPopup != nullptr && _beamSectionPopup->isVisible())
            _beamSectionPopup->fitToContentDeferred(true);
        QTimer::singleShot(0, m_foldWidgets[Fold_BeamSection], [this]() {
            m_foldWidgets[Fold_BeamSection]->refreshLayout();
            });
    }

    void mPropertyPanel::insertStation()
    {
        if (getCVM == nullptr || _propertyData == nullptr)
            return;
        auto crossBeam = toType<mCrossBeam>(_propertyData);
        if (crossBeam == nullptr)
            return;
        if (!canLeaveSectionEditor() || !validateStations(true))
            return;
        int count = crossBeam->_paraList.size();
        if (count < 2)
            return;
        int row = qMin(_activeStation >= 0 ? _activeStation : 0, count - 2);
        double left = crossBeam->_paraList[row].toDouble();
        double right = crossBeam->_paraList[row + 1].toDouble();
        double mid = (left + right) / 2.0;
        if (mid <= left || mid >= right)
        {
            _stationErrorLabel->setText(QStringLiteral("相邻位置过近，请先调整位置再添加。"));
            _stationErrorLabel->show();
            return;
        }
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        crossBeam->_paraList.insert(row + 1, mNumberToString(mid));
        mapRelation.appendTableMapRelation({ Physical_Property, _propertyData->getPropertyId() },
            { Physical_PropertyShape, stationShapeId(row) }, row + 1);
        renderStationRows();
        _activeStation = -1;
        selectStation(row + 1);
        QLineEdit* edit = stationPosEdit(row + 1);
        if (edit)
        {
            edit->setFocus();
            edit->selectAll();
        }
        updateDynamicInfoData();
        updateBeamEntrySummaries();
    }

    void mPropertyPanel::removeStation()
    {
        if (getCVM == nullptr || _propertyData == nullptr)
            return;
        auto crossBeam = toType<mCrossBeam>(_propertyData);
        if (crossBeam == nullptr)
            return;
        int count = crossBeam->_paraList.size();
        int row = _activeStation;
        if (row <= 0 || row >= count - 1 || count <= 2)
            return;
        if (!canLeaveSectionEditor())
            return;
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        crossBeam->_paraList.removeAt(row);
        mapRelation.deleteTabelMapRelation({ Physical_Property, _propertyData->getPropertyId() }, { Physical_PropertyShape, -1 }, row);
        renderStationRows();
        _activeStation = -1;
        selectStation(qMax(0, row - 1));
        _stationErrorLabel->hide();
        updateDynamicInfoData();
        updateBeamEntrySummaries();
    }

    void mPropertyPanel::clearStations()
    {
        if (getCVM == nullptr || _propertyData == nullptr)
            return;
        auto crossBeam = toType<mCrossBeam>(_propertyData);
        if (crossBeam == nullptr)
            return;
        if (crossBeam->_paraList.size() <= 2)
            return;
        if (!canLeaveSectionEditor())
            return;
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        while (crossBeam->_paraList.size() > 2)
        {
            crossBeam->_paraList.removeAt(1);
            mapRelation.deleteTabelMapRelation({ Physical_Property, _propertyData->getPropertyId() }, { Physical_PropertyShape, -1 }, 1);
        }
        _stationShapeCache.clear();
        renderStationRows();
        _activeStation = -1;
        selectStation(0);
        _stationErrorLabel->hide();
        updateDynamicInfoData();
        updateBeamEntrySummaries();
    }

    void mPropertyPanel::changeVariableShape(MxFunctions::SecType type)
    {
        if (type == _variableShape || type == MxFunctions::Sec_None)
            return;
        if (getCVM == nullptr || _propertyData == nullptr)
            return;
        auto crossBeam = toType<mCrossBeam>(_propertyData);
        if (crossBeam == nullptr)
            return;
        if (!canLeaveSectionEditor())
            return;
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto& sectionShapeManager = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape);
        int pid = _propertyData->getPropertyId();
        QList<int> beamIds = mapRelation.getMapRelation<QList<int>>({ Physical_Property, pid }, Physical_PropertyShape);
        QHash<int, int> converted;  //同一旧截面共享转换结果
        QSet<int> usedNew;
        int count = crossBeam->_paraList.size();
        for (int i = 0; i < count && i < beamIds.size(); ++i)
        {
            int oldId = beamIds[i];
            if (oldId == -1)
                continue;//未设置截面的行不强行分配
            _stationShapeCache[i][(int)_variableShape] = oldId;
            int newId = -1;
            //优先使用该行此类型的历史截面
            auto cached = _stationShapeCache[i].find((int)type);
            if (cached != _stationShapeCache[i].end() && cached.value() != -1)
            {
                auto section = sectionShapeManager.getSectionShapeData(cached.value());
                if (section && section->getSectionShapeType() == type)
                    newId = cached.value();
            }
            //同一旧截面在其他行已转换则复用
            if (newId == -1 && oldId != -1 && converted.contains(oldId))
                newId = converted[oldId];
            if (newId == -1)
            {
                //取一个未被占用的同类型已有截面，否则快速新建
                QStringList names = sectionShapeManager.getAllSectionShapeNamesByTypes(type);
                for (const QString& name : names)
                {
                    int candidateId = sectionShapeManager.getIDByPropertyShapeName(name);
                    if (candidateId != -1 && !usedNew.contains(candidateId))
                    {
                        newId = candidateId;
                        break;
                    }
                }
                if (newId == -1)
                {
                    auto created = quickCreateSectionShape(type);
                    if (created)
                        newId = created->getSectionShapeId();
                }
            }
            if (newId == -1)
                continue;
            usedNew.insert(newId);
            if (oldId != -1)
                converted[oldId] = newId;
            mapRelation.updateTabelMapRelation({ Physical_Property, pid }, { Physical_PropertyShape, newId }, i);
            _stationShapeCache[i][(int)type] = newId;
        }
        _variableShape = type;
        syncVariableShapeButton();
        int active = qBound(0, _activeStation, count - 1);
        renderStationRows();
        _activeStation = -1;
        selectStation(active);
        updateDynamicInfoData();
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, pid);
    }

    MechSectionShapeData::mSectionShapeBase* mPropertyPanel::currentEditSection()
    {
        if (getCVM == nullptr || _propertyData == nullptr)
            return nullptr;
        auto& sectionShapeManager = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape);
        if (_propertyData->getPropertyType() == CrossProperty)
        {
            int shapeId = stationShapeId(_activeStation);
            return shapeId == -1 ? nullptr : sectionShapeManager.getSectionShapeData(shapeId);
        }
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int shapeId = mapRelation.getMapRelation<int>({ Physical_Property, _propertyData->getPropertyId() }, Physical_PropertyShape);
        return shapeId == -1 ? nullptr : sectionShapeManager.getSectionShapeData(shapeId);
    }

    MechSectionShapeData::mSectionShapeBase* mPropertyPanel::quickCreateSectionShape(MxFunctions::SecType type)
    {
        if (getCVM == nullptr)
            return nullptr;
        auto& sectionShapeManager = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape);
        int id = sectionShapeManager.createID();
        sectionShapeManager.setCurrentID(id);
        QString prefix;
        switch (type)
        {
        case MxFunctions::Sec_H: prefix = "H"; break;
        case MxFunctions::Sec_L: prefix = "L"; break;
        case MxFunctions::Sec_I: prefix = "I"; break;
        case MxFunctions::Sec_I1: prefix = "I1"; break;
        case MxFunctions::Sec_T: prefix = "T"; break;
        case MxFunctions::Sec_T1: prefix = "T1"; break;
        case MxFunctions::Sec_T2: prefix = "T2"; break;
        case MxFunctions::Sec_Z: prefix = "Z"; break;
        case MxFunctions::Sec_Box: prefix = "Box"; break;
        case MxFunctions::Sec_Box1: prefix = "Box1"; break;
        case MxFunctions::Sec_Rec: prefix = "Rec"; break;
        case MxFunctions::Sec_Cir: prefix = "Cir"; break;
        case MxFunctions::Sec_Ann: prefix = "Ann"; break;
        case MxFunctions::Sec_Channel: prefix = "Channel"; break;
        case MxFunctions::Sec_Channel1: prefix = "Channel1"; break;
        case MxFunctions::Sec_Channel2: prefix = "Channel2"; break;
        case MxFunctions::Sec_Cross: prefix = "Cross"; break;
        case MxFunctions::Sec_Hat: prefix = "Hat"; break;
        case MxFunctions::Sec_None: prefix = "Custom"; break;
        default: return nullptr;
        }
        QString name = getCVM->getModelData()->createNameByType(Physical_PropertyShape, prefix);
        MechSectionShapeData::mSectionShapeBase* data = nullptr;
        switch (type)
        {
        case MxFunctions::Sec_H: data = new H_SectionVisual(name, id); break;
        case MxFunctions::Sec_L: data = new L_SectionVisual(name, id); break;
        case MxFunctions::Sec_I: data = new I_SectionVisual(name, id); break;
        case MxFunctions::Sec_I1: data = new I1_SectionVisual(name, id); break;
        case MxFunctions::Sec_T: data = new T_SectionVisual(name, id); break;
        case MxFunctions::Sec_T1: data = new T1_SectionVisual(name, id); break;
        case MxFunctions::Sec_T2: data = new T2_SectionVisual(name, id); break;
        case MxFunctions::Sec_Z: data = new Z_SectionVisual(name, id); break;
        case MxFunctions::Sec_Box: data = new Box_SectionVisual(name, id); break;
        case MxFunctions::Sec_Box1: data = new Box1_SectionVisual(name, id); break;
        case MxFunctions::Sec_Rec: data = new Rectangle_SectionVisual(name, id); break;
        case MxFunctions::Sec_Cir: data = new Circle_SectionVisual(name, id); break;
        case MxFunctions::Sec_Ann: data = new Annular_SectionVisual(name, id); break;
        case MxFunctions::Sec_Channel: data = new Channel_SectionVisual(name, id); break;
        case MxFunctions::Sec_Channel1: data = new Channel1_SectionVisual(name, id); break;
        case MxFunctions::Sec_Channel2: data = new Channel2_SectionVisual(name, id); break;
        case MxFunctions::Sec_Cross: data = new Cross_SectionVisual(name, id); break;
        case MxFunctions::Sec_Hat: data = new Hat_SectionVisual(name, id); break;
        case MxFunctions::Sec_None:
            data = new mCustomSectionShape(name, id);
            //初始化24项空参数：否则_customPara为空，截面形状面板打开时para[i]越界崩溃
            data->CreateSec();
            break;
        default: return nullptr;
        }
        getCVM->getModelData()->addPhysicalField<MechSectionShapeData::mSectionShapeBase>(data);
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建截面形状 %1").arg(name), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSectionShapeSig(true, id);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PropertyShape, id);
        return data;
    }

    bool mPropertyPanel::canLeaveSectionEditor()
    {
        if (_beamStage == nullptr)
            return true;
        //参数校验
        QString error;
        QString dimKey;
        if (!_beamStage->validateInputs(&error, &dimKey))
        {
            _beamErrorLabel->setText(error);
            _beamErrorLabel->show();
            QLineEdit* edit = _beamStage->firstInvalidEdit();
            if (edit)
                edit->setFocus();
            return false;
        }
        _beamErrorLabel->hide();
        //变截面位置校验
        if (_propertyData != nullptr && _propertyData->getPropertyType() == CrossProperty)
        {
            return validateStations(true);
        }
        return true;
    }

    bool mPropertyPanel::eventFilter(QObject* watched, QEvent* event)
    {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::FocusIn)
        {
            QVariant row = watched->property("stationRow");
            if (row.isValid())
                selectStation(row.toInt());
        }
        return mPanelBase::eventFilter(watched, event);
    }

    void mPropertyPanel::updateMatPanel()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (_propertyData == nullptr)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int matId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_Material);
        if (matId == -1)
        {
            _comboMat->setCurrentIndex(-1);
        }
        else
        {
            auto mat = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(matId);
            if (mat)
            {
                _comboMat->setCurrentText(mat->getMaterialName());
            }
        }
    }
    void mPropertyPanel::updateCoorPanel()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (_propertyData == nullptr)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int coorId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_Coordinate);
        QString coorname = getCVM->getModelData()->getNameByPhyID(Physical_Coordinate, coorId);
        _comboCoordinate->setCurrentText(coorname);
    }
    void mPropertyPanel::updateParamPanel()
    {
        if (_propertyData == nullptr)
        {
            return;
        }
        PropertyType type = _propertyData->getPropertyType();

        auto fold = m_foldWidgets[Fold_Param];
        while (fold->layout()->rowCount() > 0)
        {
            fold->removeRow(0);
        }
        if (type == RodProperty)
        {
            fold->setTitle("杆截面参数");

            auto bar = toType<mTorsionBar>(_propertyData);
            auto lineEdit = new mCalcLineEdit(QString(bar->_J), mCalcLineEdit::CalcMode::NoEmpty, this);//必选，浮点数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, bar, lineEdit]() {bar->_J = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("扭转矩"), lineEdit);

            lineEdit = new mCalcLineEdit(QString(bar->_A), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);//必选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, bar, lineEdit]() {bar->_A = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("横截面积"), lineEdit);

            lineEdit = new mCalcLineEdit(QString(bar->_nsm), mCalcLineEdit::CalcMode::Positive, this);//可选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, bar, lineEdit]() {bar->_nsm = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("非结构质量"), lineEdit);
        }
        else if (type == TubeProperty)
        {
            fold->setTitle("管截面参数");

            auto tube = toType<mTube>(_propertyData);
            auto lineEdit = new mCalcLineEdit(QString(tube->_od), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);//必选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, tube, lineEdit]() {tube->_od = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("管外径"), lineEdit);

            lineEdit = new mCalcLineEdit(QString(tube->_t), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);//必选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, tube, lineEdit]() {tube->_t = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("管壁厚"), lineEdit);

            lineEdit = new mCalcLineEdit(QString(tube->_nsm), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);//必选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, tube, lineEdit]() {tube->_nsm = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("非结构质量"), lineEdit);

            lineEdit = new mCalcLineEdit(QString(tube->_od2), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);//必选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, tube, lineEdit]() {tube->_od2 = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("管内径"), lineEdit);
        }
        else if (type == PlaneStrainProperty || type == PlaneStressProperty)
        {
            if (type == PlaneStressProperty)
            {
                fold->setTitle("平面应力截面参数");
            }
            else if (type == PlaneStrainProperty)
            {
                fold->setTitle("平面应变截面参数");
            }
            auto lineEdit = new mCalcLineEdit(QString(_propertyData->getThickness()), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);//必选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, lineEdit]() {
                _propertyData->setThickness(lineEdit->text());
                updateThickRender();
                updateDynamicInfoData();
                });
            fold->addRow(new PanelLabel("厚度"), lineEdit);
            updateThickRender();
        }
        else if (type == ShellProperty)
        {
            fold->setTitle("壳截面参数");
            auto shell = toType<mShellHomo>(_propertyData);
            auto lineEdit = new mCalcLineEdit(QString(shell->getThickness()), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);//必选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, lineEdit]() {
                _propertyData->setThickness(lineEdit->text());
                updateThickRender();
                updateDynamicInfoData();
                });
            fold->addRow(new PanelLabel("厚度"), lineEdit);
            updateThickRender();

            lineEdit = new mCalcLineEdit(QString(shell->_nsm), mCalcLineEdit::CalcMode::NonNegative, this);//可选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, shell, lineEdit]() {shell->_nsm = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("非结构质量"), lineEdit);

            auto comboBox = new QComboBox(this);
            auto label = new QLabel("参考面偏置距离");
            lineEdit = new mCalcLineEdit(QString(shell->_z0), mCalcLineEdit::CalcMode::NoEmpty, this);//必选，浮点数
            comboBox->addItems({ "无","偏置距离", "底部", "顶部" });
            connect(comboBox, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                if (text == "偏置距离")
                {
                    label->show(); lineEdit->show();
                }
                else
                {
                    label->hide(); lineEdit->hide();
                }
                shell->_referenceplane = text;
                updateDynamicInfoData();
                QTimer::singleShot(0, fold, [=]() {
                    fold->refreshLayout();
                    });
                });
            label->hide(); lineEdit->hide();
            comboBox->setCurrentText(shell->_referenceplane);

            fold->addRow(new PanelLabel("参考面位置"), comboBox);

            connect(lineEdit, &QLineEdit::editingFinished, this, [this, shell, lineEdit]() {shell->_z0 = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(label, lineEdit);


        }
        else if (type == ShearPlateProperty)
        {
            fold->setTitle("剪切板截面参数");
            auto shearPlate = toType<mShearPlate>(_propertyData);
            auto lineEdit = new mCalcLineEdit(QString(_propertyData->getThickness()), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);//必选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, lineEdit]() {
                _propertyData->setThickness(lineEdit->text());
                updateThickRender();
                updateDynamicInfoData();
                });
            fold->addRow(new PanelLabel("厚度"), lineEdit);
            updateThickRender();

            lineEdit = new mCalcLineEdit(QString(shearPlate->_valF1), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);//必选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, shearPlate, lineEdit]() {shearPlate->_valF1 = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("拉伸刚度F1"), lineEdit);

            lineEdit = new mCalcLineEdit(QString(shearPlate->_valF2), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);//必选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, shearPlate, lineEdit]() {shearPlate->_valF2 = lineEdit->text();  updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("拉伸刚度F2"), lineEdit);
        }
        else if (type == ViscousDampProperty)
        {
            fold->setTitle("阻尼参数");

            auto viscous = toType<mViscousdampsection>(_propertyData);

            auto lineEdit = new mCalcLineEdit(QString(viscous->_GE), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);//必选，浮点数，正数
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, viscous, lineEdit]() {viscous->_GE = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("阻尼系数"), lineEdit);
        }
        else if (type == CbushProperty)
        {
            fold->setTitle("六向弹簧阻尼参数");

            auto cbush = toType<mCbush>(_propertyData);

            auto addPushButton = []() {
                auto pb = new QPushButton();
                pb->setFixedSize(30, 30);
                QIcon icon(":/mMainWidgets/Icon/table.png");
                pb->setIcon(icon);
                pb->setIconSize(pb->size());
                pb->setToolTip("输入参数");
                return pb;
                };

            vector<QCheckBox*> check;
            vector<QPushButton*> pb;

            auto gridLayout = new QGridLayout();
            gridLayout->setVerticalSpacing(4);
            gridLayout->setHorizontalSpacing(4);

            auto check1 = new QCheckBox("k");
            check.push_back(check1);
            check1->setFixedHeight(30);
            auto pb1 = addPushButton();
            pb.push_back(pb1);

            auto check2 = new QCheckBox("km");
            check.push_back(check2);
            check2->setFixedHeight(30);
            auto pb2 = addPushButton();
            pb.push_back(pb2);

            gridLayout->addWidget(check1, 0, 0, Qt::AlignVCenter);
            gridLayout->addWidget(pb1, 0, 1, Qt::AlignVCenter);
            gridLayout->addWidget(check2, 0, 3, Qt::AlignVCenter);
            gridLayout->addWidget(pb2, 0, 4, Qt::AlignVCenter);

            check1 = new QCheckBox("b");
            check.push_back(check1);
            check1->setFixedHeight(30);
            pb1 = addPushButton();
            pb.push_back(pb1);

            check2 = new QCheckBox("ge");
            check.push_back(check2);
            check2->setFixedHeight(30);
            pb2 = addPushButton();
            pb.push_back(pb2);

            gridLayout->addWidget(check1, 1, 0, Qt::AlignVCenter);
            gridLayout->addWidget(pb1, 1, 1, Qt::AlignVCenter);
            gridLayout->addWidget(check2, 1, 3, Qt::AlignVCenter);
            gridLayout->addWidget(pb2, 1, 4, Qt::AlignVCenter);

            check1 = new QCheckBox("ang");
            check.push_back(check1);
            check1->setFixedHeight(30);
            pb1 = addPushButton();
            pb.push_back(pb1);

            check2 = new QCheckBox("m");
            check.push_back(check2);
            check2->setFixedHeight(30);
            pb2 = addPushButton();
            pb.push_back(pb2);

            gridLayout->addWidget(check1, 2, 0, Qt::AlignVCenter);
            gridLayout->addWidget(pb1, 2, 1, Qt::AlignVCenter);
            gridLayout->addWidget(check2, 2, 3, Qt::AlignVCenter);
            gridLayout->addWidget(pb2, 2, 4, Qt::AlignVCenter);

            gridLayout->setColumnStretch(2, 1);
            fold->addRow(gridLayout);

            for (int i = 0; i < 6; i++)
            {
                check[i]->setChecked(cbush->_isCheck[i]);
                pb[i]->setEnabled(cbush->_isCheck[i]);
                connect(check[i], &QCheckBox::clicked, this, [cbush, pb, this, i](bool isChecked) {
                    mTableDataWindow::getInstance()->resetInitialTable();
                    mTableDataWindow::getInstance()->clearAndReset(0, 0);
                    cbush->_isCheck[i] = isChecked;
                    pb[i]->setEnabled(isChecked);
                    if (isChecked)
                    {
                        auto table = mTableDataWindow::getInstance()->getTableWidget();
                        table->setFixedRow(6);
                        table->setSizeMode(mTableWidget::SizeMode::FixedSize);
                        QVector<QVector<double>> datas;
                        datas.resize(1);
                        datas[0] = cbush->_value[i];

                        mTableDataWindow::getInstance()->setTableData(datas);
                        table->setHorizontalHeaders(_hor[i]);
                        table->setVerticalHeaders(_ver[i]);
                        mTableDataWindow::getInstance()->setTableTitle(_hor[i][0]);
                        mTableDataWindow::getInstance()->setUseCurve(false);
                    }

                    mDockLayoutManager::getInstance()->activateDock("Table");
                    });

                connect(pb[i], &QPushButton::clicked, this, [cbush, this, i]() {
                    mTableDataWindow::getInstance()->resetInitialTable();
                    auto table = mTableDataWindow::getInstance()->getTableWidget();
                    table->setFixedRow(6);
                    table->setSizeMode(mTableWidget::SizeMode::FixedSize);
                    QVector<QVector<double>> datas;
                    datas.resize(1);
                    datas[0] = cbush->_value[i];

                    mTableDataWindow::getInstance()->setTableData(datas);
                    table->setHorizontalHeaders(_hor[i]);
                    table->setVerticalHeaders(_ver[i]);
                    mTableDataWindow::getInstance()->setTableTitle(_hor[i][0]);
                    mTableDataWindow::getInstance()->setUseCurve(false);
                    mDockLayoutManager::getInstance()->activateDock("Table");
                    });
            }
        }
        else if (type == GasketProperty)
        {
            fold->setTitle("垫片截面参数");

            auto gasket = toType<mGasket>(_propertyData);

            auto comboBox = new QComboBox(this);
            auto label = new QLabel("初始厚度"); label->setParent(nullptr);
            auto lineEdit = new mCalcLineEdit(QString(gasket->_initialThickness), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this); lineEdit->setParent(nullptr);
            comboBox->addItems({ "自动计算", "指定厚度" });
            connect(comboBox, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                if (text == "指定厚度")
                {
                    fold->insertRow(1, label, lineEdit);
                    //label->show(); lineEdit->show();
                }
                else
                {
                    label->setParent(nullptr); lineEdit->setParent(nullptr);
                    //label->hide(); lineEdit->hide();
                }
                gasket->_thicknessDef = text;
                updateDynamicInfoData();
                QTimer::singleShot(0, fold, [=]() {
                    fold->refreshLayout();
                    });
                });
            comboBox->setCurrentText(gasket->_thicknessDef);

            fold->addRow(new PanelLabel("厚度定义"), comboBox);

            connect(lineEdit, &QLineEdit::editingFinished, this, [this, gasket, lineEdit]() {gasket->_initialThickness = lineEdit->text(); updateDynamicInfoData(); });
            if (gasket->_thicknessDef == "指定厚度")
            {
                fold->addRow(label, lineEdit);
            }

            auto lineEdit1 = new mCalcLineEdit(QString(gasket->_initialGap), mCalcLineEdit::CalcMode::Positive, this);
            connect(lineEdit1, &QLineEdit::editingFinished, this, [this, gasket, lineEdit1]() {gasket->_initialGap = lineEdit1->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("初始间隙"), lineEdit1);
        }
        else if (type == GapuniProperty)
        {
            fold->setTitle("间隙参数");

            auto gapuni = toType<mGapuni>(_propertyData);

            auto lineEdit = new mCalcLineEdit(QString(gapuni->_gapDistance), mCalcLineEdit::CalcMode::Positive, this);
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, gapuni, lineEdit]() {gapuni->_gapDistance = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("间隙距离"), lineEdit);

            lineEdit = new mCalcLineEdit(QString(gapuni->_area), mCalcLineEdit::CalcMode::Positive, this);
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, gapuni, lineEdit]() {gapuni->_area = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("截面积"), lineEdit);

            auto dirEdit = new QLineEdit(gapuni->_direction, this);
            connect(dirEdit, &QLineEdit::editingFinished, this, [this, gapuni, dirEdit]() {gapuni->_direction = dirEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("方向"), dirEdit);
        }
        else if (type == PlyLaminateProperty)
        {
            fold->setTitle("铺层复材截面参数");
            updatePlyLaminatePanel();
        }

        // 最后刷新布局
        QTimer::singleShot(0, fold, [=]() {
            fold->refreshLayout();
            fold->updateGeometry();
            });
    }
    void mPropertyPanel::updateSpringParamPanel()
    {
        if (_propertyData == nullptr)
        {
            return;
        }
        PropertyType type = _propertyData->getPropertyType();
        auto fold = m_foldWidgets[Fold_Spring];

        // 清空所有行（同时销毁旧控件，避免泄漏）
        while (fold->layout()->rowCount() > 0)
        {
            fold->removeRow(0);
        }

        auto spring = toType<mSpring>(_propertyData);
        if (!spring) return;

        if (type == Elas1Property)
        {
            fold->setTitle("单自由度参数");

            auto lineEdit = new mCalcLineEdit(QString(spring->_K), mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);
            connect(lineEdit, &QLineEdit::editingFinished, this, [this, spring, lineEdit]() {spring->_K = lineEdit->text(); updateDynamicInfoData(); });
            fold->addRow(new PanelLabel("弹性属性"), lineEdit);

            lineEdit = new mCalcLineEdit(QString(spring->_GE), mCalcLineEdit::CalcMode::Positive, this);
            connect(lineEdit, &QLineEdit::editingFinished, this, [spring, lineEdit]() {spring->_GE = lineEdit->text(); });
            fold->addRow(new PanelLabel("阻尼系数"), lineEdit);

            lineEdit = new mCalcLineEdit(QString(spring->_S), mCalcLineEdit::CalcMode::Positive, this);
            connect(lineEdit, &QLineEdit::editingFinished, this, [spring, lineEdit]() {spring->_S = lineEdit->text(); });
            fold->addRow(new PanelLabel("应力系数"), lineEdit);
        }
        else
        {
            if (type == Spring1Property)
                fold->setTitle("六自由度接地弹簧参数");
            else if (type == Spring2Property)
                fold->setTitle("六自由度弹簧参数");
            else if (type == SpringAProperty)
                fold->setTitle("三自由度弹簧参数");

            if (type == Spring1Property || type == Spring2Property)
            {
                auto dofCombo = new QComboBox(this);
                dofCombo->addItems({ "x","y", "z", "rx", "ry" , "rz" });
                dofCombo->setCurrentText(spring->_k);
                connect(dofCombo, &QComboBox::currentTextChanged, this, [this, spring](const QString& text) {
                    spring->_k = text;
                    });
                fold->addRow(new PanelLabel("刚度分量"), dofCombo);
            }

            auto typeCombo = new QComboBox(this);
            typeCombo->addItems({ "线性","非线性" });
            typeCombo->setCurrentText(spring->_nonlinear);
            connect(typeCombo, &QComboBox::currentTextChanged, this, [this, spring](const QString& text) {
                spring->_nonlinear = text;
                if (text == "线性")
                {
                    mTableDataWindow::getInstance()->clearAndReset(0, 0);
                }
                QTimer::singleShot(0, this, [this]() {
                    updateSpringParamPanel();
                    updateDynamicInfoData();
                    });
                });
            fold->addRow(new PanelLabel("属性"), typeCombo);

            if (spring->_nonlinear == "线性")
            {
                auto lineEdit = new mCalcLineEdit(spring->_value, this, true, false, true);
                connect(lineEdit, &QLineEdit::editingFinished, this, [spring, lineEdit]() {spring->_value = lineEdit->text(); });
                fold->addRow(new QLabel("常刚度"), lineEdit);
            }
            else
            {
                auto label = new QLabel("力-位移曲线");
                auto pb = new QPushButton(this);
                pb->setMaximumWidth(30);
                pb->setIcon(QIcon(":/mMainWidgets/Icon/table.png"));
                connect(pb, &QPushButton::clicked, this, [this, spring]() {
                    mTableDataWindow::getInstance()->resetInitialTable();
                    mTableDataWindow::getInstance()->setUseCurve(false);
                    mTableDataWindow::getInstance()->setTableTitle("力-位移曲线");
                    QStringList list;
                    list << "弹簧力" << "伸长量";
                    QVector<QVector<double>> datas;
                    datas.resize(2);
                    for (auto& it : spring->_luCurve)
                    {
                        datas[0].push_back(it.first);
                        datas[1].push_back(it.second);
                    }
                    mTableDataWindow::getInstance()->setTableData(datas);
                    mTableDataWindow::getInstance()->setHorizontalHeaders(list);
                    mDockLayoutManager::getInstance()->activateDock("Table");
                    });
                fold->addRow(label, pb);
            }
        }

        QTimer::singleShot(0, fold, [=]() {
            fold->refreshLayout();
            });
    }
    std::vector<int> mPropertyPanel::updatePartAssignPanel()
    {
        //设置已经赋予的部件
        _listAssignPart->clear();
        //_listAssignPart->addItems({});

        if (getCVM == nullptr)
        {
            return std::vector<int>();
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        auto partList = mapRelation.getMapRelation<QList<int>>({ Physical_Property, id }, Physical_Part);
        std::vector<int> partIds{ partList.begin(), partList.end() };
        QStringList partNames;
        for (auto partId : partList)
        {
            auto partName = meshData->getPartNameByPartId(partId);
            if (partName.isEmpty())
            {
                continue;
            }
            partNames.append(partName);
        }
        _listAssignPart->addItems(partNames);
        return partIds;

    }
    void mPropertyPanel::assignElement(std::vector<int> partIds)
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (_propertyData == nullptr)
        {
            return;
        }
        PropertyType propertyType = _propertyData->getPropertyType();
        QString propertyTypeStr = _propertyData->getPropertyTypeStr();
        mDataElement* elementData = getCVM->getModelData()->getElementData();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        std::set<int> changedPartIds;
        for (auto partId : partIds)
        {
            auto meshIDs = meshData->getEleIdsByPartId(partId);
            bool partChanged = false;

            for (auto meshID : meshIDs)
            {
                MeshType meshType = meshData->getMeshTypeByEleId(meshID);
                RenderPickResultData data;
                data.result_id = meshID;
                data.part_id = partId;
                int order = meshData->getOrder(data);

                auto elementType = mElementFunction::getDefaultElementType(propertyType, meshType, order);
                if (elementType == NoneElement)
                {
                    continue;
                }
                if (elementData->hasElementData(meshID))
                {
                    QString mainType = mElementFunction::getElementMainType(elementData->getElementDataByMeshID(meshID)->getElementType());
                    if (mainType.isEmpty())
                    {
                        continue;
                    }
                    if (mainType == "低阶高精度" || mainType == "轴对称" || mainType == "绘图显示")
                    {
                        continue;
                    }
                    else if (propertyType == ShellProperty && (mainType == "壳" || mainType == "膜"))
                    {
                        continue; // 壳单元/膜单元都匹配壳
                    }
                    else if (mainType == "实体壳" && propertyType == SolidShellProperty)
                    {
                        continue;
                    }
                    else if (propertyType == PlyLaminateProperty && mainType == "壳")
                    {
                        continue; // 壳单元已匹配铺层，无需重新赋予
                    }
                    else if (mainType.contains("弹簧") && propertyTypeStr.contains("弹簧") && mainType != propertyTypeStr)
                    {
                        //弹簧大类不相符
                    }
                    else if (mainType.contains("阻尼") && propertyTypeStr.contains("阻尼") && mainType != propertyTypeStr)
                    {
                        //阻尼大类不相符
                    }
                    else if (propertyTypeStr.contains(mainType)) // 大类已相符，无需重新赋予
                    {
                        continue;
                    }
                }
                if (propertyType == RodProperty || propertyType == TubeProperty || propertyType == SolidProperty || propertyType == SolidShellProperty || propertyType == PlaneStrainProperty
                    || propertyType == PlaneStressProperty || propertyType == ShearPlateProperty || propertyType == Spring1Property || propertyType == Spring2Property || propertyType == SpringAProperty || propertyType == MembraneProperty || propertyType == GasketProperty || propertyType == GapuniProperty)
                {
                    elementData->assignElementTypeToMesh(meshID, elementType);
                    partChanged = true;
                }
                else if (propertyType == ResultantProperty || propertyType == CrossProperty)
                {
                    auto element = new mBeamElement(elementType);
                    elementData->assignElementToMesh(meshID, element);
                    partChanged = true;
                }
                else if (propertyType == ShellProperty)
                {
                    auto element = new mShellElement(elementType);
                    elementData->assignElementToMesh(meshID, element);
                    partChanged = true;
                }
                else if (propertyType == Elas1Property || propertyType == ViscousDampProperty)
                {
                    auto element = new mElas1Element(elementType);
                    elementData->assignElementToMesh(meshID, element);
                    partChanged = true;
                }
                else if (propertyType == CbushProperty)
                {
                    auto element = new mCbushElement(elementType);
                    elementData->assignElementToMesh(meshID, element);
                    partChanged = true;
                }
            }

            if (partChanged)
            {
                changedPartIds.insert(partId);
            }
        }

        if (!changedPartIds.empty())
        {
            QStringList changedPartNames;
            for (int id : changedPartIds)
            {
                changedPartNames.append(meshData->getPartNameByPartId(id));
            }
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("部件%1的单元类型已改变！").arg(changedPartNames.join(",")));
        }
    }
    void mPropertyPanel::highlightPart(std::vector<int> ids)
    {
        if (conn)
            disconnect(conn);
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
        addPreWidgetSelectPartID(getCVM->getPreRenderWidget(), ids);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

        getCVM->getPreRenderWidget()->updatePaint();

        conn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mPropertyPanel::slot_finishedPick);

        _lastPickData = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        updateRotateAngleRend();
        updateThickRender();
    }
    void mPropertyPanel::unHighlightPart(std::vector<int> ids)
    {
        if (conn)
            disconnect(conn);
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
        reducePreWidgetSelectPartID(getCVM->getPreRenderWidget(), ids);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

        getCVM->getPreRenderWidget()->updatePaint();

        conn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mPropertyPanel::slot_finishedPick);

        _lastPickData = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        for (const auto& part_id : ids)
        {
            getCVM->getPreRenderWidget()->GetBeamSystem()->deleteBeam(QString("rod_%1").arg(part_id));
        }
        for (const auto& part_id : ids)
        {
            getCVM->getPreRenderWidget()->GetBeamSystem()->deleteShell(QString("shell_%1").arg(part_id));
        }
    }
    void mPropertyPanel::updateRotateAngleRend()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoDatabase = getCVM->getModelData()->getGeoData();
        auto meshData = getCVM->getModelData()->getMeshData();
        if (_propertyData == nullptr)
        {
            return;
        }
        if (_propertyData->getPropertyType() != ResultantProperty)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int propShapeId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_PropertyShape);
        if (propShapeId == -1)
        {
            return;
        }

        auto sectionShape = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(propShapeId);
        if (!sectionShape)
        {
            return;
        }

        updateRotateRender();

        return;//暂时不用角度编辑
        // 设置控件需要的信息
        std::vector<SelectionItem> selections;
        {
            _beam_struct = std::make_shared<mxr::BeamEditStruct>();
            _beam_struct->ids = _lastPickData;
            _beam_struct->angle = _propertyData->getAngle().toDouble();
            _beam_struct->up_dirs = this->getDefaultUpVector(_lastPickData);//这是部件初始up向量
            SelectionItem item;
            item.handle = AnyHandle::from(_beam_struct.get()); // 封装之后的void*
            selections.push_back(item);
        }
        {
            SelectionItem item;
            item.handle = AnyHandle::from(getCVM->getPreRenderWidget()); // 封装之后的void*
            selections.push_back(item);
        }

        // 开启控件
        {
            getCVM->getPreRenderWidget()->getGizmoManager()->setActiveGizmo(mxr::GizmoType::BeamEdit); // 设置控件模式
            getCVM->getPreRenderWidget()->getGizmoManager()->setSelection(selections); // 设置选中的编辑对象
            if (getCVM->getPreRenderWidget()->getGizmoManager()->activeGizmo())
                getCVM->getPreRenderWidget()->getGizmoManager()->activeGizmo()->active(true);
            //getCVM->getPreRenderWidget()->getStateMachineManager()->enableGizmo(true); // 开启控件编辑
        }
        getCVM->getPreRenderWidget()->updatePaint();
    }
    std::vector<QVector3D> mPropertyPanel::getDefaultUpVector(std::vector<int> parIds)
    {
        std::vector<QVector3D> vecs;//这是部件的初始角度矢量方向(up)
        vecs.resize(parIds.size(), QVector3D(1, 0, 0));//默认
        if (getCVM == nullptr)
        {
            return vecs;
        }
        auto areVectorsParallel = [](const QVector3D& v1, const QVector3D& v2) { // 计算两个向量的叉乘 
            QVector3D crossProd = QVector3D::crossProduct(v1, v2);
            // 如果叉乘结果是零向量，则两个向量平行 
            return crossProd.isNull();
            };
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoDatabase = getCVM->getModelData()->getGeoData();
        auto meshData = getCVM->getModelData()->getMeshData();

        for (int i = 0; i < parIds.size(); ++i)
        {
            int partId = parIds.at(i);
            auto partData = meshData->getPartData(partId);
            if (partData == nullptr)
            {
                continue;
            }
            int partType = partData->getPartType();
            if (partType == 2)//网格部件
            {
                auto meshIds = meshData->getMeshIdsByPartId(partId, 1);
                if (meshIds.size() > 0)
                {
                    RenderPickResultData resultData;
                    resultData.result_id = meshIds.at(0);
                    resultData.part_id = partId;
                    std::vector<QVector3D> nodeVertexs = meshData->getCoorsOfEle(resultData);
                    if (nodeVertexs.size() >= 2)
                    {
                        if (areVectorsParallel(nodeVertexs.at(1) - nodeVertexs.at(0), vecs[i]))
                        {
                            vecs[i] = QVector3D(0, 1, 0);
                            continue;

                        }
                    }
                }
            }
            else
            {
                auto lines = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 1);
                auto res = getGEOServer.geoCalculate().CalculateEdgeAttr(lines);
                for (auto vec : res)
                {
                    if (areVectorsParallel(vec.m_dir, vecs.at(i)))
                    {
                        vecs[i] = QVector3D(0, 1, 0);
                        continue;
                    }
                }
            }
        }
        return vecs;
    }
    void mPropertyPanel::updateRotateRender()
    {
        if (_propertyData == nullptr)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        mDataElement* elementData = getCVM->getModelData()->getElementData();
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        int propShapeId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_PropertyShape);
        if (propShapeId == -1)
        {
            return;
        }
        PropertyType type = _propertyData->getPropertyType();
        if (type != ResultantProperty)
        {
            return;
        }
        auto sectionShape = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(propShapeId);
        if (!sectionShape)
        {
            return;
        }

        //渲染接入
        _lastPickData;//这是部件id
        _propertyData->getAngle().toDouble();//这是角度
        sectionShape->getSectionShapeType();//这是截面形状类型
        sectionShape->getParaNoType();//这是截面形状参数
        std::set<QString> names;
        std::vector<float> params;
        const auto& up_vectors = this->getDefaultUpVector(_lastPickData);//这是部件初始up向量
        for (const auto& param : sectionShape->getParaNoType())
        {
            params.push_back(param);
        }
        for (const auto& part_id : _lastPickData)
        {
            QString partName = meshData->getPartNameByPartId(part_id);
            auto meshIds = meshData->getAllShow1DMeshElementID({ partName });
            QVector<QVector3D> norms;
            for (auto meshId : meshIds)
            {
                mElementBase* elementBase = elementData->getElementDataByMeshID(meshId.result_id);
                mBeamElement* element = dynamic_cast<mBeamElement*>(elementBase);
                if (element == nullptr)
                {
                    norms.push_back(QVector3D(0, 0, 0));
                }
                else
                {
                    if (!element->_node.isEmpty())
                    {
                        std::vector<QVector3D> eleCoor = meshData->getCoorsOfEle(meshId);
                        QVector3D nodeCoor = meshData->getVectorById(element->_node.toInt());
                        norms.push_back(QVector3D(nodeCoor.x() - eleCoor[0].x(), nodeCoor.y() - eleCoor[0].y(), nodeCoor.z() - eleCoor[0].z()));
                    }
                    else if (!element->_beamValue.isEmpty())
                    {
                        QStringList list = element->_beamValue.split(",");
                        QVector3D coor;
                        if (list.size() >= 3)
                        {
                            coor.setX(list[0].toDouble());
                            coor.setY(list[1].toDouble());
                            coor.setZ(list[2].toDouble());
                        }
                        norms.push_back(coor);
                    }
                    else
                    {
                        //新建梁单元的参考方向字段为空：必须补默认向量，
                        //否则 norms 数量与单元数不一致，渲染端 rot[index] 越界崩溃
                        norms.push_back(QVector3D(0, 1, 0));
                    }
                }
            }
            getCVM->getPreRenderWidget()->GetBeamSystem()->createOrUpdateBeam(QString("rod_%1").arg(part_id), part_id, static_cast<mxr::BeamSectionType>(sectionShape->getSectionShapeType()), params, mxr::pickRefsOf(meshIds), norms);
        }
        getCVM->getPreRenderWidget()->updatePaint();
    }
    void mPropertyPanel::updateThickRender()
    {
        if (_propertyData == nullptr)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        PropertyType type = _propertyData->getPropertyType();
        //if (type != PlaneStrainProperty && type != PlaneStressProperty && type != ShellProperty && type != ShearPlateProperty)
        //{
        //    return;
        //}

        if (type != ShellProperty)//暂时只考虑壳厚度显示
        {
            return;
        }

        //渲染接入
        _lastPickData;//这是部件id
        double thickness = _propertyData->getThickness().toDouble();//这是厚度
        for (const auto& part_id : _lastPickData)
        {
            getCVM->getPreRenderWidget()->GetBeamSystem()->createOrUpdateShell(QString("shell_%1").arg(part_id), part_id, thickness);
        }
        getCVM->getPreRenderWidget()->updatePaint();
    }
    void mPropertyPanel::connectAngleRotateUpdate()
    {
        if (angleConn)
            disconnect(angleConn);
        if (_propertyData == nullptr)
        {
            return;
        }
        _angleLineEdit->setExpression(_propertyData->getAngle());
        angleConn = connect(getCVM->getPreRenderWidget(), &mxr::CCSMRendWidget::updateBeamAngle, this, [this](float angle)
            {
                float new_angle = _propertyData->getAngle().toFloat() + angle;
                new_angle = std::fmod(new_angle, 360.0f);  // 取模，保留小数
                if (new_angle < 0)
                    new_angle += 360.0f;          // 映射到 0-360
                _propertyData->setAngle(MxFunctions::mNumberToString(new_angle));
                _angleLineEdit->setExpression(_propertyData->getAngle());

                //updateRotateRender();
            });
    }


    void mPropertyPanel::plyTableShow()
    {
        if (!_propertyData) return;
        // 静默清空，不触发外部数据清空
        m_plyTableWidget->clearTableData(true);
        QCoreApplication::processEvents();

        QStringList matNames = getCVM->getModelData()
            ->getManager<MechMaterialData::mMaterialManager>(Physical_Material)
            .getAllMatNames();
        m_plyTableWidget->setMaterialItems(matNames);

        int propId = _propertyData->getPropertyId();
        auto plyLam = toType<mPlyLaminate>(_propertyData);
        QList<int> matIDs = getCVM->getModelData()->getMapRelationManager()
            .getMapRelation<QList<int>>({ Physical_Property, propId }, Physical_Material);

        // ====================== 正确的数据同步 ======================
        int dataSize = qMax(plyLam->thicknessList.size(), matIDs.size());

        // 统一大小（天然空字符串，无默认值；空值保持用户清空的状态，不自动回填默认值）
        plyLam->thicknessList.resize(dataSize);
        plyLam->angleList.resize(dataSize);
        plyLam->intePointList.resize(dataSize);

        // 材料ID列表补齐
        while (matIDs.size() < dataSize)
            matIDs.append(-1);
        // ============================================================

        // 静默添加行 → 不触发信号
        m_plyTableWidget->addRows(dataSize, true);

        // 反写数据（顺序：材料、角度、厚度、积分点）
        for (int i = 0; i < dataSize; ++i)
        {
            QString matName;
            if (matIDs[i] >= 0) {
                matName = getCVM->getModelData()
                    ->getManager<MechMaterialData::mMaterialManager>(Physical_Material)
                    .getMatNameByID(matIDs[i]);
            }

            m_plyTableWidget->setRowData(i, matName,
                plyLam->angleList[i],
                plyLam->thicknessList[i],
                plyLam->intePointList[i]);
        }

        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(m_plyTableWidget);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mPropertyPanel::onPlyRowAdded(int row)
    {
        int propId = _propertyData->getPropertyId();
        auto plyLam = toType<mPlyLaminate>(_propertyData);
        if (!plyLam) return;

        if (row < 0 || row > plyLam->thicknessList.size())
            return;

        // 直接插入空字符串，积分点默认 "1"
        plyLam->thicknessList.insert(row, "");
        plyLam->angleList.insert(row, "");
        plyLam->intePointList.insert(row, "1");

        // 材料默认空，不选任何材料
        QString defaultMat;
        int defaultMatId = -1;

        // 写入映射（空材料对应 -1）
        getCVM->getModelData()->getMapRelationManager()
            .appendTableMapRelation(
                { Physical_Property, propId },
                { Physical_Material, defaultMatId },
                row
            );

        // 同步表格
        if (m_plyTableWidget)
        {
            m_plyTableWidget->setRowData(row,
                defaultMat,
                plyLam->angleList[row],
                plyLam->thicknessList[row],
                plyLam->intePointList[row]);
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, propId);
    }

    void mPropertyPanel::onPlyRowRemoved(int row)
    {
        auto plyLam = toType<mPlyLaminate>(_propertyData);
        if (!plyLam) return;

        if (row < 0 || row >= plyLam->thicknessList.size())
            return;

        int propId = _propertyData->getPropertyId();

        // 删除映射
        getCVM->getModelData()->getMapRelationManager()
            .deleteTabelMapRelation(
                { Physical_Property, propId },
                { Physical_Material, -1 },
                row
            );

        // 删除数据
        plyLam->thicknessList.remove(row);
        plyLam->angleList.remove(row);
        plyLam->intePointList.remove(row);

        // 同步动态状态（刷新警告）
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, propId);
    }

    void mPropertyPanel::onPlyTableCleared()
    {
        auto plyLam = toType<mPlyLaminate>(_propertyData);
        if (!plyLam) return;

        int propId = _propertyData->getPropertyId();
        int rowCount = plyLam->thicknessList.size();

        // 清空映射
        for (int i = rowCount - 1; i >= 0; --i) {
            getCVM->getModelData()->getMapRelationManager()
                .deleteTabelMapRelation(
                    { Physical_Property, propId },
                    { Physical_Material, -1 },
                    i
                );
        }

        // 清空数据
        plyLam->thicknessList.clear();
        plyLam->angleList.clear();
        plyLam->intePointList.clear();

        // 同步动态状态（刷新警告）
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, propId);
    }

    void mPropertyPanel::onPlyMaterialChanged(int row, const QString& material)
    {
        auto plyLam = toType<mPlyLaminate>(_propertyData);
        if (!plyLam) return;
        if (row < 0 || row >= plyLam->thicknessList.size()) return;

        int propId = _propertyData->getPropertyId();
        int matId = -1;

        if (!material.isEmpty()) {
            matId = getCVM->getModelData()
                ->getManager<MechMaterialData::mMaterialManager>(Physical_Material)
                .getIDByMatName(material);
            if (matId < 0) return;
        }

        // 空材料时matId为-1，把该行映射同样置为-1，避免清空后旧材料复原
        getCVM->getModelData()->getMapRelationManager()
            .updateTabelMapRelation(
                { Physical_Property, propId },
                { Physical_Material, matId },
                row
            );

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, _propertyData->getPropertyId());
    }

    void mPropertyPanel::onPlyThicknessChanged(int row, const QString& value)
    {
        auto plyLam = toType<mPlyLaminate>(_propertyData);
        if (row >= 0 && row < plyLam->thicknessList.size())
            plyLam->thicknessList[row] = value;
    }

    void mPropertyPanel::onPlyAngleChanged(int row, const QString& value)
    {
        auto plyLam = toType<mPlyLaminate>(_propertyData);
        if (row >= 0 && row < plyLam->angleList.size())
            plyLam->angleList[row] = value;
    }

    void mPropertyPanel::onPlyIntePointChanged(int row, const QString& value)
    {
        auto plyLam = toType<mPlyLaminate>(_propertyData);
        if (row >= 0 && row < plyLam->intePointList.size())
            plyLam->intePointList[row] = value;
    }

    void mPropertyPanel::slot_showPlyWindow()
    {
        if (m_plyTableWidget == nullptr)
            return;

        //获取表格数据(材料、角度、厚度、积分点)
        const QList<QPair<QString, QList<QString>>> plyData = m_plyTableWidget->getAllPlyData();
        const int nrow = plyData.size();

        //没有铺层不弹窗
        if (nrow < 1)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前没有铺层！"));
            return;
        }

        std::vector<std::string> plyNames;
        std::vector<float> Thicknesses;
        std::vector<float> rotationAngle;

        for (int i = 0; i < nrow; ++i)
        {
            //材料名
            QString matname = plyData.at(i).first;
            if (matname.isEmpty())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, QString("第%1层没有选择材料！").arg(i + 1));
                return;
            }

            //厚度
            double thick = 0.0;
            QString thickstr = plyData.at(i).second.at(1).simplified();
            if (!thickstr.isEmpty())
            {
                thick = thickstr.toDouble();
            }
            //角度
            double angel = 0.0;
            QString anglestr = plyData.at(i).second.at(0).simplified();
            if (!anglestr.isEmpty())
            {
                angel = anglestr.toDouble();
            }

            plyNames.push_back(matname.toStdString());
            Thicknesses.push_back(thick);
            rotationAngle.push_back(angel);
        }

        if (_plyViewport != nullptr)
        {
            _plyViewport->close();
            delete _plyViewport;
            _plyViewport = nullptr;
        }

        _plyViewport = new MechPreData::PlyViewportWidget();
        _plyViewport->resize(800, 600);
        _plyViewport->setWindowTitle("Ply Stack Plot");
        mxr::mPlyRend* plyRend = _plyViewport->plyRend();
        std::shared_ptr<mxr::mCubeRender> cube = plyRend->getCubeRender();
        cube->addToScene(plyNames, Thicknesses, nrow, rotationAngle, true);

        // 3.0相机默认位于(0,0,100)，不会像2.0那样按场景包围球自动home
        // show之前按立方体几何适配视角(宽0.5 单层高0.2 长1.2，中心位于原点)，保证首帧即为最终视角
        {
            auto cameraController = plyRend->getCameraController();
            float dy = 0.2f * nrow;
            float radius = 0.5f * std::sqrt(0.5f * 0.5f + dy * dy + 1.2f * 1.2f);
            //与OSG相机home公式一致：距离 = 半径 / sin(fov/2)，再留10%边距
            //fitToView透视模式取相机到中心的距离等于当前包围球半径，故先存入放大后的半径
            double fovRad = cameraController->getCameraState().fov / 180.0 * 3.14159265358979323846;
            float fitRadius = static_cast<float>(radius * 1.1 / std::sin(fovRad * 0.5));
            glm::vec3 center(0.0f, 0.0f, 0.0f);
            cameraController->updateSceneBoundingSphere(mxr::BoundingSphere(center, fitRadius));
            cameraController->fitToView(center.x, center.y, center.z, fitRadius);
            //直接跳转到目标视角，不做由远及近的动画
            cameraController->immediateToTargetState();
        }

        _plyViewport->show();
    }

    void mPropertyPanel::updatePlyLaminatePanel()
    {
        // 最优先空指针检查
        if (!_propertyData)
            return;

        auto plyLam = toType<mPlyLaminate>(_propertyData);
        if (!plyLam) return;

        auto fold = m_foldWidgets[Fold_Param];
        if (!fold || !fold->layout()) return;

        while (fold->layout()->count() > 0)
        {
            QLayoutItem* item = fold->layout()->takeAt(0);
            if (item->widget())
            {
                item->widget()->setParent(nullptr);
                item->widget()->deleteLater();
            }
            delete item;
        }

        _comboPlyType->blockSignals(true);
        _comboPlyType->setCurrentIndex(plyLam->_type);
        _comboPlyType->blockSignals(false);

        if (_radDefine && _radLaminate)
        {
            // 先阻塞两个单选按钮的信号
            const bool oldBlock1 = _radDefine->blockSignals(true);
            const bool oldBlock2 = _radLaminate->blockSignals(true);

            if (plyLam->_plyDefType == 0)
            {
                _radDefine->setChecked(true);
                plyTableShow();
            }
            else if (plyLam->_plyDefType == 1)
            {
                _radLaminate->setChecked(true);
            }

            // 恢复原来的信号状态
            _radDefine->blockSignals(oldBlock1);
            _radLaminate->blockSignals(oldBlock2);
        }

        if (plyLam->_type == 0)
        {
            // ================== 行0：属性 ==================
            QComboBox* shellPropertyCombo = new QComboBox(this);
            shellPropertyCombo->addItems({ "无","对称铺层","弯曲行为","膜行为","等效单层","对称弯曲","对称膜","对称等效单层" });
            shellPropertyCombo->setCurrentText(plyLam->_shellProperty);
            connect(shellPropertyCombo, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                plyLam->_shellProperty = text;
                });
            fold->addRow(new PanelLabel("属性"), shellPropertyCombo);

            // ================== 行1：参考面位置 ==================
            QComboBox* referenceSurfaceCombo = new QComboBox(this);
            referenceSurfaceCombo->addItems({ "无","偏置距离", "偏置率", "底部", "顶部" });
            referenceSurfaceCombo->setCurrentText(plyLam->_referenceSurfacePos);
            connect(referenceSurfaceCombo, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                plyLam->_referenceSurfacePos = text;
                QTimer::singleShot(0, this, [=]() {
                    updatePlyLaminatePanel();
                    QTimer::singleShot(0, fold, [=]() {
                        fold->refreshLayout();
                        fold->updateGeometry();
                        });
                    });
                }, Qt::QueuedConnection);
            fold->addRow(new PanelLabel("参考面位置"), referenceSurfaceCombo);

            // ================== 行2：偏置距离/偏置率 ==================
            if (plyLam->_referenceSurfacePos == "偏置距离" || plyLam->_referenceSurfacePos == "偏置率")
            {
                QString labelText = plyLam->_referenceSurfacePos == "偏置率" ? "参考面偏置率" : "参考面偏置距离";
                QLabel* refLabel = new QLabel(labelText, this);
                mCalcLineEdit* refEdit = new mCalcLineEdit(QString(plyLam->_referenceSurfaceDis), this, true, false, true);
                connect(refEdit, &QLineEdit::editingFinished, this, [=]() {
                    plyLam->_referenceSurfaceDis = refEdit->text();
                    });
                fold->addRow(refLabel, refEdit);
            }

            // ================== 行3：失效 ==================
            QComboBox* failureCombo = new QComboBox(this);
            failureCombo->addItems({ "无","最大应力","最大应变","Tsai-Hill","Tsai-Wu","Azzi-Tsai-Hill","Hoff","Puck","Hashin" });
            failureCombo->setCurrentText(plyLam->_failure);
            connect(failureCombo, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                plyLam->_failure = text;
                });
            fold->addRow(new PanelLabel("失效"), failureCombo);

            // ================== 行4：厚度 ==================
            QComboBox* thicknessCombo = new QComboBox(this);
            thicknessCombo->addItems({ "截面厚度","单元厚度离散场","节点厚度离散场" });
            thicknessCombo->setCurrentText(plyLam->_thickness);
            connect(thicknessCombo, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                plyLam->_thickness = text;
                QTimer::singleShot(0, this, [=]() {
                    updatePlyLaminatePanel();
                    QTimer::singleShot(0, fold, [=]() {
                        fold->refreshLayout();
                        fold->updateGeometry();
                        });
                    });
                }, Qt::QueuedConnection);
            fold->addRow(new PanelLabel("厚度"), thicknessCombo);

            // ================== 行5：离散场 ==================
            if (plyLam->_thickness != "截面厚度")
            {
                QLabel* fieldLabel = new QLabel("离散场", this);
                QComboBox* fieldCombo = new QComboBox(this);
                auto& dfManager = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
                QStringList allNames = dfManager.getAllFieldNames(MxFunctions::FieldType::Discrete);
                QStringList thickNames;
                MxFunctions::PickDataType requiredLocate = (plyLam->_thickness == "单元厚度离散场") ? PickDataType::Element : PickDataType::Node;
                for (const QString& name : allNames)
                {
                    int fid = dfManager.getIDByFieldName(name);
                    auto* fdata = dfManager.getDiscreteFieldData(fid);
                    if (fdata && fdata->getSpaceType() == MxFunctions::ThicknessSpace && fdata->getLocate() == requiredLocate)
                        thickNames.append(name);
                }
                // 添加空白行，因为此项非必选
                fieldCombo->addItem("");
                fieldCombo->addItems(thickNames);

                // 读取当前厚度离散场映射并设置选中项
                int propID = plyLam->getPropertyId();
                auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
                int curFieldID = mapRelation.getMapRelation<int>(
                    { Physical_Property, propID }, Physical_Field, MapRelation_ThickField);
                if (curFieldID >= 0)
                {
                    QString curName = dfManager.getFieldNameByID(curFieldID);
                    if (thickNames.contains(curName))
                    {
                        fieldCombo->setCurrentText(curName);
                    }
                    else
                    {
                        // 映射的离散场已不存在于当前类型列表中（被修改为其他类型）
                        mapRelation.appendMapRelation({ Physical_Property, propID }, { Physical_Field, -1 }, MapRelation_ThickField);
                        fieldCombo->setCurrentIndex(0);
                    }
                }
                else
                {
                    fieldCombo->setCurrentIndex(0);
                }

                // 绑定切换信号，更新映射
                connect(fieldCombo, &QComboBox::currentTextChanged, this, [=](const QString& text)
                    {
                        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
                        auto& mgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
                        int id = plyLam->getPropertyId();
                        int oldFieldID = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_Field, MapRelation_ThickField);
                        if (oldFieldID != -1)
                            mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_Field, -1 }, MapRelation_ThickField);
                        if (!text.isEmpty())
                        {
                            int fid = mgr.getIDByFieldName(text);
                            mapRelation.appendMapRelation({ Physical_Property, id }, { Physical_Field, fid }, MapRelation_ThickField);
                        }
                    });

                fold->addRow(fieldLabel, fieldCombo);
            }
        }
        else if (plyLam->_type == 1)
        {
            QComboBox* solidShellPropertyCombo = new QComboBox(this);
            solidShellPropertyCombo->addItems({ "无","对称" });
            solidShellPropertyCombo->setCurrentText(plyLam->_solidShellProperty);
            connect(solidShellPropertyCombo, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                plyLam->_solidShellProperty = text;
                });
            fold->addRow(new PanelLabel("属性"), solidShellPropertyCombo);
        }
        else if (plyLam->_type == 2)
        {
            QComboBox* solidPropertyCombo = new QComboBox(this);
            solidPropertyCombo->addItems({ "无","对称" });
            solidPropertyCombo->setCurrentText(plyLam->_solidProperty);
            connect(solidPropertyCombo, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                plyLam->_solidProperty = text;
                });
            fold->addRow(new PanelLabel("属性"), solidPropertyCombo);

            QComboBox* stackDirCombo = new QComboBox(this);
            stackDirCombo->addItems({ "单元方向1","单元方向2","单元方向3" });
            stackDirCombo->setCurrentIndex(plyLam->eleMentDir - 1);
            connect(stackDirCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int idx) {
                plyLam->eleMentDir = idx + 1;
                });
            fold->addRow(new PanelLabel("堆叠方向"), stackDirCombo);
        }
    }

    void mPropertyPanel::updateDynamicInfoData()
    {
        if (_propertyData == nullptr)
        {
            return;
        }
        if (!getCVM)
        {
            return;
        }
        int id = _propertyData->getPropertyId();
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Property, id);
    }

    void mPropertyPanel::updatePropertyDefine()
    {
        if (_propertyData == nullptr)
        {
            return;
        }

        QString variety = "";
        switch (_propertyData->getPropertyType())
        {
        case MxFunctions::RodProperty:
        case MxFunctions::ResultantProperty:
        case MxFunctions::CrossProperty:
        case MxFunctions::TubeProperty:
        case MxFunctions::Elas1Property:
        case MxFunctions::Spring1Property:
        case MxFunctions::Spring2Property:
        case MxFunctions::SpringAProperty:
        case MxFunctions::ViscousDampProperty:
        case MxFunctions::CbushProperty:
        case MxFunctions::GapuniProperty:
            variety = "一维截面"; break;
        case MxFunctions::PlaneStrainProperty:
        case MxFunctions::PlaneStressProperty:
        case MxFunctions::ShellProperty:
        case MxFunctions::ShearPlateProperty:
        case MxFunctions::MembraneProperty:
            variety = "二维截面"; break;
        case MxFunctions::SolidProperty:
        case MxFunctions::SolidShellProperty:
        case MxFunctions::GasketProperty:
            variety = "三维截面"; break;
        case MxFunctions::LayeredShellProperty:
        case MxFunctions::PlyLaminateProperty:
            variety = "复材截面"; break;
        default:
            break;
        }
        _lblVariety->setText(variety);
        QString type = _propertyData->getPropertyTypeStr();
        if (type.contains("梁"))
        {
            _lblType->setText("梁");
        }
        else
        {
            _lblType->setText(type);
        }
    }
}
