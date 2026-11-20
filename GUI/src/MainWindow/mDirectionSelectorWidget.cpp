#include <mGeoManager.h>
#include <mMeshData.h>
#include "mDirectionSelectorWidget.h"
//#include "mGeometricFeatureAttribute.h" //【OCC迁移·链A续清配套】CalculateGeometricAttr 显式自含 //【OCC摘除·S10/P0-1】调用已切 engine().geoCalculate() 同语法视图
#include "mGeoManager.h" //【GUI适配】geoManager->getGeoModelBase() 需完整类型（本头已零 OCC）
#include "mGeneralPickUp.h"

#include <QDebug>
#include <QListView>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QTimer>
#include <QFormLayout>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QPainter>
#include <QPointer>
#include "mGetSetTypeTemplate.h"
#include "mGroupBox.h"    //外层分组框(MxFunctions::mGroupBox 骑线标题)

//尺寸/颜色令牌与主题重发（素材库规则：禁止硬编码颜色/尺寸）
#include "UiMetrics.h"
#include "UiPalette.h"
#include "ThemeManager.h"

//尺寸/颜色令牌与主题重发（素材库规则：禁止硬编码颜色/尺寸）
#include "UiMetrics.h"
#include "UiPalette.h"
#include "ThemeManager.h"

using namespace MechCommon;
using namespace MxFunctions;
namespace MechMainWindow
{
    //通用方向细线框（原型 load-general-direction-group）：
    //1px 直角边框 + 骑线题注（业务标题），无填充底色；
    //颜色/字号/间距绘制时取 UiPalette/UiMetrics，随主题与档位自适配
    class mDirectionFrame : public QWidget
    {
    public:
        mDirectionFrame(const QString& title, QWidget* parent)
            : QWidget(parent), _title(title)
        {
        }

        //档位/档题注带变化时重发内容边距并重绘（由宿主 applyThemeStyles 调用）
        void applyMetrics()
        {
            //题注带高 = 正文字号 + 按钮列间距（题注文字上下留白同源）
            const int band = UiMetrics::bodyFontSize() + UiMetrics::menuGap();
            if (layout())
                layout()->setContentsMargins(UiMetrics::baseG(), band, UiMetrics::baseG(), UiMetrics::rowGap());
            update();
        }

    protected:
        void paintEvent(QPaintEvent*) override
        {
            QPainter p(this);
            const int band = UiMetrics::bodyFontSize() + UiMetrics::menuGap();
            const int capX = UiMetrics::fieldInset();
            const int capGap = UiMetrics::choiceGap();
            QFont f(UiMetrics::uiFontFamily());
            f.setPixelSize(UiMetrics::bodyFontSize());
            p.setFont(f);
            const int textW = QFontMetrics(f).horizontalAdvance(_title);

            //1px 直角边框；上边线在题注两侧断开（骑线效果）
            p.setPen(QPen(UiPalette::border(), 1));
            const qreal l = 0.5;
            const qreal r = width() - 0.5;
            const qreal b = height() - 0.5;
            const qreal gapL = capX - capGap;
            const qreal gapR = capX + textW + capGap;
            p.drawLine(QPointF(l, b), QPointF(l, l));                              //左
            p.drawLine(QPointF(r, l), QPointF(r, b));                              //右
            p.drawLine(QPointF(l, b), QPointF(r, b));                              //下
            if (gapL > l)
                p.drawLine(QPointF(l, l), QPointF(gapL, l));                       //上左段
            if (gapR < r)
                p.drawLine(QPointF(gapR, l), QPointF(r, l));                       //上右段

            //骑线题注（次文字色）
            p.setPen(UiPalette::label());
            p.drawText(QRect(capX, 0, textW + capGap, band), Qt::AlignLeft | Qt::AlignVCenter, _title);
        }

    private:
        QString _title;
    };

    /// @brief 深度判断拾取数据是否真正为空（遍历所有分组，只要有一组有数据则非空）
    static bool isPickDataValid(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& pickData)
    {
        if (pickData.empty())
            return false;
        for (const auto& pair : pickData)
        {
            if (!pair.second.empty())
                return true;
        }
        return false;
    }

    static QValidator* createVectorValidator(QObject* parent)
    {
        // 严格限制为 x, y, z 三分量格式，中间必须使用英文逗号。
        // 允许整数、小数以及科学计数法；允许分量之间包含空格。
        // 在输入未完成时返回 Intermediate，保证用户能够正常编辑。
        QRegularExpression regExp(R"(^\s*[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?(\s*,\s*[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?){0,2}\s*$)");
        return new QRegularExpressionValidator(regExp, parent);
    }

    static QVector3D parseVectorString(const QString& text)
    {
        QStringList parts = text.split(',', QString::SkipEmptyParts);
        if (parts.size() != 3)
            return QVector3D();
        bool ok[3] = { false, false, false };
        float v[3];
        for (int i = 0; i < 3; ++i)
        {
            v[i] = parts[i].trimmed().toFloat(&ok[i]);
            if (!ok[i])
                return QVector3D();
        }
        return QVector3D(v[0], v[1], v[2]);
    }

    //分量显示：至多6位小数并去尾零(0显示"0"而非"0.000000")；用户输入不受此限
    static QString componentToString(double v)
    {
        QString s = QString::number(v, 'f', 6);
        if (s.contains('.'))
        {
            while (s.endsWith('0'))
                s.chop(1);
            if (s.endsWith('.'))
                s.chop(1);
        }
        return s == QStringLiteral("-0") ? QStringLiteral("0") : s;
    }

    static QString vectorToString(const QVector3D& vec)
    {
        return QString("%1, %2, %3")
            .arg(componentToString(vec.x()))
            .arg(componentToString(vec.y()))
            .arg(componentToString(vec.z()));
    }

    // 默认坐标计算：根据点/线/面/体/节点/单元等计算一个平均坐标
    static QVector3D defaultCoordinateCalculator(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData> >& picks)
    {
        if (picks.empty())
            return QVector3D();

        auto* cvm = getCVM;
        if (!cvm || !cvm->getModelData())
            return QVector3D();

        auto* meshData = cvm->getModelData()->getMeshData();
        auto* geoManager = cvm->getModelData()->getGeoManager();
        auto* geoData = cvm->getModelData()->getGeoData();
        if (!meshData || !geoManager || !geoData)
            return QVector3D();

        QVector3D sum(0, 0, 0);
        int count = 0;

        for (const auto& pair : picks)
        {
            MxFunctions::PickDataType type = pair.first;
            const auto& data = pair.second;
            if (data.empty()) continue;

            if (type == MxFunctions::PickDataType::Node)
            {
                for (auto id : data)
                {
                    QVector3D pos = meshData->getVectorById(id.result_id);
                    if (!pos.isNull())
                    {
                        sum += pos;
                        ++count;
                    }
                }

                continue;
            }
            else if (type == MxFunctions::PickDataType::Element)
            {
                for (auto id : data)
                {
                    QVector3D pos = meshData->getCentroid(id);
                    if (!pos.isNull())
                    {
                        sum += pos;
                        ++count;
                    }
                }

                continue;
            }
            else if (type == MxFunctions::PickDataType::ElementEdge)
            {
                for (auto id : data)
                {
                    QVector3D pos1 = meshData->getVectorById(id.result_id);
                    QVector3D pos2 = meshData->getVectorById(id.result_id2);
                    if (!pos1.isNull())
                    {
                        sum += pos1;
                        ++count;
                    }
                    if (!pos2.isNull())
                    {
                        sum += pos2;
                        ++count;
                    }
                }

                continue;
            }
            else if (type == MxFunctions::PickDataType::ElementFace)
            {
                for (auto id : data)
                {
                    QVector3D outVec;
                    QVector3D pos;
                    meshData->GetOutVectorByMeshFaceID(id, outVec, pos);
                    if (!pos.isNull())
                    {
                        sum += pos;
                        ++count;
                    }
                }

                continue;
            }

            std::vector<int> ids;
            ids.reserve(data.size());
            for (const auto& d : data)
                ids.push_back(d.result_id);

            if (type == MxFunctions::PickDataType::GeoPoint)
            {
                auto vertexAttrs = getGEOServer.geoCalculate().CalculateVertexAttr(ids);//【GUI适配】成员法为未推送新版 API，改既有静态法等价
                for (const auto& attr : vertexAttrs)
                {
                    sum += attr.m_coor;
                    ++count;
                }
                continue;
            }

            if (type == MxFunctions::PickDataType::GeoEdge)
            {
                auto edgeAttrs = getGEOServer.geoCalculate().CalculateEdgeAttr(ids);//【GUI适配】同上
                for (const auto& attr : edgeAttrs)
                {
                    sum += attr.m_coor;
                    ++count;
                }
                continue;
            }

            QVector<int> qids;
            qids.reserve(static_cast<int>(ids.size()));
            for (int id : ids)
                qids.push_back(id);
            if (type == MxFunctions::PickDataType::GeoFace)
            {
                auto faceCoords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, qids);
                for (const auto& c : faceCoords)
                {
                    sum += c;
                    ++count;
                }
                continue;
            }

            if (type == MxFunctions::PickDataType::GeoSolid)
            {
                auto solidCoords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, qids);
                for (const auto& c : solidCoords)
                {
                    sum += c;
                    ++count;
                }
                continue;
            }

            if (type == MxFunctions::PickDataType::Part)
            {
                for (int id : ids)
                {
                    QString partName = meshData->getPartNameByPartId(id);
                    if (meshData->returnPartType(partName) == 2)
                    {
                        auto pos = meshData->getMeshPartCenter(partName);
                        if (!pos.isNull())
                        {
                            sum += pos;
                            ++count;
                        }
                    }
                    else
                    {
                        auto pos = getGEOServer.geoCalculate().getCentroid(id);
                        if (!pos.isNull())
                        {
                            sum += pos;
                            ++count;
                        }
                    }
                }
                continue;
            }
        }

        if (count == 0)
            return QVector3D();
        return sum / count;
    }

    // 默认方向计算：将单个对象的类型转换为方向（仅用于选择对象模式）
    static QVector3D defaultDirectionCalculator(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData> >& picks)
    {
        if (picks.empty())
            return QVector3D();

        auto* cvm = getCVM;
        if (!cvm || !cvm->getModelData())
            return QVector3D(0, 0, 0);

        auto* meshData = cvm->getModelData()->getMeshData();
        auto* geoManager = cvm->getModelData()->getGeoManager();
        if (!meshData || !geoManager)
            return QVector3D(0, 0, 0);

        // 取第一个非空分组
        MxFunctions::PickDataType type = MxFunctions::PickDataType::None;
        RenderPickResultData id;
        for (const auto& pair : picks)
        {
            if (!pair.second.empty())
            {
                type = pair.first;
                id = pair.second.front();
                break;
            }
        }

        if (type == MxFunctions::PickDataType::ElementFace)
        {
            QVector3D outVec;
            QVector3D pos;
            meshData->GetOutVectorByMeshFaceID(id, outVec, pos);
            if (!outVec.isNull())
                return outVec.normalized();
        }
        else if (type == MxFunctions::PickDataType::GeoFace)
        {
            auto faceAttrs = getGEOServer.geoCalculate().CalculateFaceAttr({ id.result_id });//【GUI适配】同上
            if (!faceAttrs.empty())
            {
                QVector3D dir = faceAttrs.front().m_dir;
                if (dir.lengthSquared() > 1e-12)
                    return dir.normalized();
            }
        }
        else if (type == MxFunctions::PickDataType::ElementEdge)
        {
            QVector3D pos1 = meshData->getVectorById(id.result_id);
            QVector3D pos2 = meshData->getVectorById(id.result_id2);
            QVector3D pos = pos2 - pos1;
                return pos.normalized();
        }
        else if (type == MxFunctions::PickDataType::GeoEdge)
        {
            auto edgeAttrs = getGEOServer.geoCalculate().CalculateEdgeAttr({ id.result_id });//【GUI适配】同上
            if (!edgeAttrs.empty())
            {
                QVector3D dir = edgeAttrs.front().m_dir;
                if (dir.lengthSquared() > 1e-12)
                    return dir.normalized();
            }
        }

        // 兜底：返回 +X
        return QVector3D(0, 0, 0);
    }

    mDirectionSelectorWidget::mDirectionSelectorWidget(
        const QString& labelText,
        bool showMagnitude,
        const QString& magnitudeLabel,
        bool hasUnit,
        const QString& unitText,
        bool hasDefaultDirect,
        QWidget* parent,
        mColorBlock objectPickColorBlock)
        : QWidget(parent)
    {
        initUI(labelText, showMagnitude, magnitudeLabel, hasUnit, unitText, hasDefaultDirect, objectPickColorBlock);

        //档位/主题跟随：切档/切主题后按 UiMetrics/UiPalette 重发行高列宽与自绘细线框
        applyThemeStyles();
        QPointer<mDirectionSelectorWidget> guard(this);
        ThemeManager::addDockThemeExtraApplier([guard]() { if (guard) guard->applyThemeStyles(); });
    }

    mDirectionSelectorWidget::~mDirectionSelectorWidget() = default;

    void mDirectionSelectorWidget::initUI(
        const QString& labelText,
        bool showMagnitude,
        const QString& magnitudeLabel,
        bool hasUnit,
        const QString& unitText,
        bool hasDefaultDirect,
        mColorBlock objectPickColorBlock)
    {
        // 使用单一垂直布局，所有行都作为独立子控件/子布局加入。
        QVBoxLayout* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        // 方向容器与大小行之间通过 layout spacing 留出间距；
        // 当大小行隐藏时，该间距会自动消失，避免下部留白。
        rootLayout->setSpacing(UiMetrics::rowGap());
        rootLayout->setAlignment(Qt::AlignTop);
        _rootLayout = rootLayout;

        /*
        * 外边框容器：MxFunctions::mGroupBox（骑线标题=构造参数 labelText，与
        * mPointPickerWidget 外框同款）；内容挂其内容区网格(两行：首行+动态页)，
        * 边距由分组框按题注带自管理
        */
        _groupBox = new MxFunctions::mGroupBox(labelText, QPixmap(), QPixmap(), this);
        _groupBox->setCheckable(false);
        QGridLayout* containerGrid = _groupBox->grid();
        containerGrid->setHorizontalSpacing(UiMetrics::columnGap());
        containerGrid->setVerticalSpacing(UiMetrics::rowGap());
        containerGrid->setAlignment(Qt::AlignTop);
        rootLayout->addWidget(_groupBox);

        // 第一行：「定义方式」标签 + 定义方式下拉 + 反向按钮
        //（原型：细线框题注=业务标题，框内统一定义方式；6 项固定选项按素材库用下拉框）
        {
            QHBoxLayout* topLayout = new QHBoxLayout();
            topLayout->setContentsMargins(0, 0, 0, 0);
            topLayout->setSpacing(UiMetrics::columnGap());

            _label = new PanelLabel(QStringLiteral("定义方式"), this);
            _label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

            _modeCombo = new QComboBox(this);
            _modeCombo->setView(new QListView);
            _modeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            if (hasDefaultDirect)
            {
                _modeCombo->addItem(QStringLiteral("默认方向"), DefaultDirection);
            }
            _modeCombo->addItem(QStringLiteral("X轴"), XAxis);
            _modeCombo->addItem(QStringLiteral("Y轴"), YAxis);
            _modeCombo->addItem(QStringLiteral("Z轴"), ZAxis);
            _modeCombo->addItem(QStringLiteral("起点-终点"), CustomCoordinate);
            _modeCombo->addItem(QStringLiteral("选择对象"), SelectObject);
            _modeCombo->addItem(QStringLiteral("向量"), DirectInput);

            _reverseBtn = new QToolButton(this);
            _reverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/fanxiang1@3x.png"));
            _reverseBtn->setToolTip(QStringLiteral("反向"));

            topLayout->addWidget(_label);
            topLayout->addWidget(_modeCombo);
            topLayout->addWidget(_reverseBtn);
            containerGrid->addLayout(topLayout, 0, 0, 1, 2);
        }

        // 内容区域：QStackedWidget 按需延迟创建页面，避免默认方向下创建不必要的子控件。
        _stackedWidget = new QStackedWidget(this);
        _stackedWidget->setContentsMargins(0, 0, 0, 0);
        _stackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _stackedWidget->setVisible(false);
        _stackedWidget->setFixedHeight(0);
        containerGrid->addWidget(_stackedWidget, 1, 0, 1, 2, Qt::AlignTop);

        // 大小行：与 direction 容器平级，便于外部独立控制间距/边距
        _magnitudeInput.initWidget(magnitudeLabel, false, mCalcLineEdit::CalcMode::Standard);
        if (_magnitudeInput.lineeditvalue)
        {
            _magnitudeInput.lineeditvalue->setFixedHeight(UiMetrics::baseH());
            // 单位统一显示在输入框内部，不显示外部 labelunit
            _magnitudeInput.lineeditvalue->setUnit(unitText);
        }
        if (_magnitudeInput.labelunit)
        {
            _magnitudeInput.labelunit->setVisible(false);
        }
        _magnitudeWidget = _magnitudeInput.propwid;
        // initWidget 创建的 propwid 没有父窗口，若直接 setVisible(true) 会短暂作为独立顶层窗口显示。
        // 先挂到本控件下并隐藏，加入布局后再按需显示，避免“弹窗闪过”。
        _magnitudeWidget->setParent(this);
        _magnitudeWidget->hide();
        _magnitudeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        rootLayout->addWidget(_magnitudeWidget, 0, Qt::AlignTop);
        _magnitudeWidget->setVisible(showMagnitude);

        // 初始状态
        _modeCombo->setCurrentIndex(0);
        if (hasDefaultDirect)
        {
            _reverseBtn->setEnabled(false);
            _currentMode = DefaultDirection;
            _initialMode = DefaultDirection; // 新增：记录初始模式
        }
        else
        {
            _currentMode = XAxis;
            _initialMode = XAxis; // 新增：记录初始模式
        }

        // 默认计算回调
        _coordinateCalculator = &defaultCoordinateCalculator;
        _calculator = &defaultDirectionCalculator;

        _pickAllowedTypes.clear();
        _objectPickColorBlock = objectPickColorBlock;

        connect(_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &mDirectionSelectorWidget::onModeChanged);
        connect(_reverseBtn, &QToolButton::clicked,
                this, &mDirectionSelectorWidget::onReverseClicked);
        connect(_magnitudeInput.lineeditvalue, &QLineEdit::editingFinished,
                this, &mDirectionSelectorWidget::onMagnitudeChanged);
    }

    //档位/主题跟随：行高/列宽按 UiMetrics 重发，细线框随 UiPalette 自绘重绘。
    //颜色一律由全局 QSS/自绘令牌下发，本函数不拼任何字面色值
    void mDirectionSelectorWidget::applyThemeStyles()
    {
        //正文族/字号（标签/输入行兜底，随档位）
        QFont f(UiMetrics::uiFontFamily());
        f.setPixelSize(UiMetrics::bodyFontSize());
        setFont(f);

        //行高统一控件外高 H；反向按钮方形（原型 std-control 方形图标钮）
        if (_modeCombo)
            _modeCombo->setFixedHeight(UiMetrics::baseH());
        if (_reverseBtn)
            _reverseBtn->setFixedSize(UiMetrics::baseH(), UiMetrics::baseH());

        //大小行
        if (_magnitudeInput.lineeditvalue)
            _magnitudeInput.lineeditvalue->setFixedHeight(UiMetrics::baseH());
        if (_magnitudeInput.labelname)
            _magnitudeInput.labelname->setFixedWidth(UiMetrics::labelWidth());

        //行内标签列宽（PanelLabel 构造时定宽，切档后重发）
        if (_label) _label->setFixedWidth(UiMetrics::labelWidth());
        if (_startLabel) _startLabel->setFixedWidth(UiMetrics::labelWidth());
        if (_endLabel) _endLabel->setFixedWidth(UiMetrics::labelWidth());
        if (_objectLabel) _objectLabel->setFixedWidth(UiMetrics::labelWidth());
        if (_directLabel) _directLabel->setFixedWidth(UiMetrics::labelWidth());

        //动态页内控件行高（页面延迟创建，按存在性重发）
        if (_startPickController) _startPickController->widget()->setFixedHeight(UiMetrics::baseH());
        if (_endPickController) _endPickController->widget()->setFixedHeight(UiMetrics::baseH());
        if (_objectPickController) _objectPickController->widget()->setFixedHeight(UiMetrics::baseH());
        if (_startApplyBtn) _startApplyBtn->setFixedHeight(UiMetrics::baseH());
        if (_endApplyBtn) _endApplyBtn->setFixedHeight(UiMetrics::baseH());
        if (_startDirEdit) _startDirEdit->setFixedHeight(UiMetrics::baseH());
        if (_endDirEdit) _endDirEdit->setFixedHeight(UiMetrics::baseH());
        if (_directEdit) _directEdit->setFixedHeight(UiMetrics::baseH());

        //外层分组框由 mGroupBox 随主题/档位自管理，无需在此重发度量

        //动态页高度随子控件新度量重算
        if (_customPage)
            _customPage->setFixedHeight(_customPage->sizeHint().height());
        if (_objectPage)
            _objectPage->setFixedHeight(_objectPage->sizeHint().height());
        if (_directPage)
            _directPage->setFixedHeight(_directPage->sizeHint().height());

        refreshContainerLayout();
    }

    void mDirectionSelectorWidget::ensureCustomPage()
    {
        if (_customPage) return;

        _customPage = new QWidget(_stackedWidget);
        QVBoxLayout* customPageLayout = new QVBoxLayout(_customPage);
        customPageLayout->setContentsMargins(0, 0, 0, 0);
        customPageLayout->setSpacing(UiMetrics::rowGap());
        customPageLayout->setAlignment(Qt::AlignTop);

        auto buildCoordinateGroup = [this](const QString& labelText, mPickWidgetController** controller, QToolButton** applyBtn, QLineEdit** edit, QLabel** label)
            {
                QWidget* group = new QWidget(_customPage);
                QGridLayout* groupLayout = new QGridLayout(group);
                groupLayout->setContentsMargins(0, 0, 0, 0);
                //行距/列距分离（素材库：行距 G、标签列与控件列距 G+2）
                groupLayout->setHorizontalSpacing(UiMetrics::columnGap());
                groupLayout->setVerticalSpacing(UiMetrics::rowGap());
                groupLayout->setAlignment(Qt::AlignTop);

                *label = new PanelLabel(labelText, group);
                (*label)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

                *controller = mGeneralPickUp::createDirectionPickWidgetPickMore(CBNone, group);
                (*controller)->setDefultTip(QStringLiteral("请选择"), labelText);
                (*controller)->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                (*controller)->widget()->setFixedHeight(UiMetrics::baseH());

                *applyBtn = new QToolButton(group);
                (*applyBtn)->setText(QStringLiteral("应用"));
                (*applyBtn)->setFixedHeight(UiMetrics::baseH());

                // 第 0 行：标签 | 拾取控件 | 应用按钮
                groupLayout->addWidget(*label, 0, 0, Qt::AlignVCenter);
                groupLayout->addWidget((*controller)->widget(), 0, 1, Qt::AlignVCenter);
                groupLayout->addWidget(*applyBtn, 0, 2, Qt::AlignVCenter);

                *edit = new QLineEdit(group);
                (*edit)->setPlaceholderText(QStringLiteral("x, y, z"));
                (*edit)->setValidator(createVectorValidator(*edit));
                (*edit)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                (*edit)->setFixedHeight(UiMetrics::baseH());

                // 第 1 行：输入框跨列 1、2，使其右边缘与“应用”按钮右边缘对齐
                groupLayout->addWidget(*edit, 1, 1, 1, 2, Qt::AlignVCenter);

                return group;
            };

        QWidget* startGroup = buildCoordinateGroup(QStringLiteral("起点"), &_startPickController, &_startApplyBtn, &_startDirEdit, &_startLabel);
        customPageLayout->addWidget(startGroup);
        _startPickController->setAllowedTypes(&pickAllGeoAndMeshItem);

        QWidget* endGroup = buildCoordinateGroup(QStringLiteral("终点"), &_endPickController, &_endApplyBtn, &_endDirEdit, &_endLabel);
        customPageLayout->addWidget(endGroup);
        _endPickController->setAllowedTypes(&pickAllGeoAndMeshItem);

        _customPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _customPage->adjustSize();
        _customPage->setFixedHeight(_customPage->sizeHint().height());

        setInputLineEditText(_startDirEdit, QVector3D(0, 0, 0));
        setInputLineEditText(_endDirEdit, QVector3D(1, 0, 0));
        // 初始化方向缓存
        onCustomInputChanged();
        // 初始化缓存坐标与输入框默认值保持一致
        _cachedStartCoord = QVector3D(0, 0, 0);
        _cachedEndCoord = QVector3D(1, 0, 0);
        _cachedStartCoordValid = false;
        _cachedEndCoordValid = false;
        //出厂默认态标记：起点(0,0,0)/终点(1,0,0)尚未被编辑过
        _customUntouched = true;

        _customPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _customPage->adjustSize();
        _customPage->setFixedHeight(_customPage->sizeHint().height());
        _stackedWidget->addWidget(_customPage);

        // ========== 修改：输入框编辑完成信号，增加渲染更新 ==========
        connect(_startDirEdit, &QLineEdit::editingFinished, this, [this]()
            {
            _customUntouched = false;
            onCustomInputChanged();
            if (_activePickController == _startPickController)
                updatePreviewRender(_startPickController);
            emit directionChanged();
            });
        connect(_endDirEdit, &QLineEdit::editingFinished, this, [this]()
            {
            _customUntouched = false;
            onCustomInputChanged();
            if (_activePickController == _endPickController)
                updatePreviewRender(_endPickController);
            emit directionChanged();
            });
        connect(_startApplyBtn, &QToolButton::clicked,this, &mDirectionSelectorWidget::onApplyStartDirection);
        connect(_endApplyBtn, &QToolButton::clicked,this, &mDirectionSelectorWidget::onApplyEndDirection);
        // 起点拾取数据变化：更新缓存 + 更新渲染 + 通知外部方向变化
        connect(_startPickController, &mPickWidgetController::dataChanged, this, [this]()
            {
            const auto& data = _startPickController->getAllData();
            // 仅当存在有效拾取数据时更新缓存，避免空分组覆盖有效缓存
            if (isPickDataValid(data.pickedData)) {
                _cachedStartCoord = computeCoordinateFromController(_startPickController);
                _cachedStartCoordValid = true;
            }
            if (_activePickController == _startPickController)
                updatePreviewRender(_startPickController);
            if (_currentMode == CustomCoordinate)
                emit directionChanged();
            });

        // 终点拾取数据变化：更新缓存 + 更新渲染 + 通知外部方向变化
        connect(_endPickController, &mPickWidgetController::dataChanged, this, [this]()
            {
            const auto& data = _endPickController->getAllData();
            if (isPickDataValid(data.pickedData)) {
                _cachedEndCoord = computeCoordinateFromController(_endPickController);
                _cachedEndCoordValid = true;
            }
            if (_activePickController == _endPickController)
                updatePreviewRender(_endPickController);
            if (_currentMode == CustomCoordinate)
                emit directionChanged();
            });
        // 起点拾取：激活状态变化时同步渲染显隐
        connect(_startPickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive)
            {
            if (isActive)
            {
                _activePickController = _startPickController;
                updatePreviewRender(_startPickController);
            }
            else if (_activePickController == _startPickController)
            {
                clearPickPreview(_startPickController);
                _activePickController = nullptr;
            }
            });

        // 终点拾取：激活状态变化时同步渲染显隐
        connect(_endPickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive)
            {
            if (isActive) {
                _activePickController = _endPickController;
                updatePreviewRender(_endPickController);
            }
            else if (_activePickController == _endPickController) {
                clearPickPreview(_endPickController);
                _activePickController = nullptr;
            }
            });
    }

    void mDirectionSelectorWidget::ensureObjectPage()
    {
        if (_objectPage) return;

        _objectPage = new QWidget(_stackedWidget);
        QHBoxLayout* objectLayout = new QHBoxLayout(_objectPage);
        objectLayout->setContentsMargins(0, 0, 0, 0);
        objectLayout->setSpacing(UiMetrics::columnGap());
        _objectLabel = new PanelLabel(QStringLiteral("选择方向"), _objectPage);
        _objectPickController = mGeneralPickUp::createDirectionPickWidgetPickOne(_objectPickColorBlock, _objectPage);
        _objectPickController->setDefultTip(QStringLiteral("请选择"), QStringLiteral("选择对象"));
        _objectPickController->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _objectPickController->widget()->setFixedHeight(UiMetrics::baseH());
        _objectPickController->setAllowedTypes(&pickGeoEdgeFaceAndElementEdgeSurface);
        objectLayout->addWidget(_objectLabel);
        objectLayout->addWidget(_objectPickController->widget(), 1);
        _objectPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _objectPage->adjustSize();
        _objectPage->setFixedHeight(_objectPage->sizeHint().height());
        _stackedWidget->addWidget(_objectPage);

        connect(_objectPickController, &mPickWidgetController::dataChanged,
                this, &mDirectionSelectorWidget::onObjectPickedChanged);
        connect(_objectPickController, &mPickWidgetController::pickActiveChanged,
            this, [this](bool isActive) { if (isActive) { onPickControllerActivated(_objectPickController); }});
    }

    void mDirectionSelectorWidget::ensureDirectPage()
    {
        if (_directPage) return;

        _directPage = new QWidget(_stackedWidget);
        QHBoxLayout* directLayout = new QHBoxLayout(_directPage);
        directLayout->setContentsMargins(0, 0, 0, 0);
        directLayout->setSpacing(UiMetrics::columnGap());
        _directLabel = new PanelLabel(QStringLiteral("方向向量"), _directPage);
        _directEdit = new QLineEdit(_directPage);
        _directEdit->setPlaceholderText(QStringLiteral("x, y, z"));
        _directEdit->setValidator(createVectorValidator(_directEdit));
        _directEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _directEdit->setFixedHeight(UiMetrics::baseH());
        directLayout->addWidget(_directLabel);
        directLayout->addWidget(_directEdit, 1);
        _directPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _directPage->adjustSize();
        _directPage->setFixedHeight(_directPage->sizeHint().height());
        _stackedWidget->addWidget(_directPage);

        setInputLineEditText(_directEdit, QVector3D(1, 0, 0));

        connect(_directEdit, &QLineEdit::editingFinished,
                this, &mDirectionSelectorWidget::onDirectInputChanged);
    }

    void mDirectionSelectorWidget::setContentPage(QWidget* page)
    {
        if (_currentContentPage == page)
        {
            _stackedWidget->setVisible(page != nullptr);
            refreshContainerLayout();
            return;
        }
        if (_currentContentPage)
        {
            _currentContentPage->hide();
        }
        _currentContentPage = page;
        if (_currentContentPage)
        {
            _currentContentPage->setVisible(false);
            _stackedWidget->setCurrentWidget(_currentContentPage);
            _currentContentPage->setVisible(true);
            _stackedWidget->setVisible(true);
        }
        else
        {
            _stackedWidget->setVisible(false);
        }
        refreshContainerLayout();
    }

    void mDirectionSelectorWidget::refreshContainerLayout()
    {
        if (_stackedWidget && _currentContentPage)
        {
            int targetHeight = _currentContentPage->minimumSizeHint().height();
            if (targetHeight < _currentContentPage->sizeHint().height())
                targetHeight = _currentContentPage->sizeHint().height();
            if (targetHeight < UiMetrics::baseH())
                targetHeight = UiMetrics::baseH();
            _stackedWidget->setFixedHeight(targetHeight);
        }
        else if (_stackedWidget)
        {
            _stackedWidget->setFixedHeight(0);
        }

        if (_rootLayout)
        {
            _rootLayout->invalidate();
            _rootLayout->activate();
        }
        updateGeometry();
        adjustSize();

        // 避免同一事件循环内因外部刷新再次导致尺寸变化而重复发射信号。
        if (!_layoutRefreshPending)
        {
            _layoutRefreshPending = true;
            QTimer::singleShot(0, this, [this]() {
                _layoutRefreshPending = false;
                emit layoutRefreshRequested();
            });
        }
    }

    mDirectionSelectorWidget::DirectionMode mDirectionSelectorWidget::directionMode() const
    {
        return _currentMode;
    }

    void mDirectionSelectorWidget::setDirectionMode(DirectionMode mode)
    {
        int idx = _modeCombo->findData(mode);
        if (idx >= 0)
            _modeCombo->setCurrentIndex(idx);
        else
            _modeCombo->setCurrentIndex(0); // 回退到默认方向，避免越界
    }

    void mDirectionSelectorWidget::setGroupTitle(const QString& title)
    {
        if (_groupBox)
        {
            _groupBox->setTitle(title);
        }
    }

    void mDirectionSelectorWidget::setPickWidgetVisible(bool visible)
    {
        if (!visible)
        {
            if (_startPickController) _startPickController->setPickWidgetVisible(false);
            if (_endPickController)_endPickController->setPickWidgetVisible(false);
            if (_objectPickController) _objectPickController->setPickWidgetVisible(false);
        }
        else
        {
            switch (_currentMode)
            {
            case CustomCoordinate:
                if (_startPickController)
                {
                    _startPickController->setPickWidgetVisible(true);
                    _startPickController->refreshAll();
                }
                if (_endPickController)
                {
                    _endPickController->setPickWidgetVisible(true);
                    _endPickController->refreshAll();
                }
                if (_objectPickController) _objectPickController->setPickWidgetVisible(false);
                break;
            case SelectObject:
                if (_startPickController) _startPickController->setPickWidgetVisible(false);
                if (_endPickController)_endPickController->setPickWidgetVisible(false);
                if (_objectPickController)
                {
                    _objectPickController->setPickWidgetVisible(true);
                    _objectPickController->refreshAll();
                }
                break;
            }
        }
    }

    void mDirectionSelectorWidget::onModeChanged(int index)
    {
        DirectionMode mode = static_cast<DirectionMode>(_modeCombo->itemData(index).toInt());
        if (_currentMode == mode)
            return;
        _currentMode = mode;
        _reversed = false;
        _reverseBtn->setEnabled(mode != DefaultDirection);

        // 切换方向模式时，清除内部各拾取控件的临时高亮，避免旧模式残留高亮在视口上引起误解。
        if (_startPickController) _startPickController->clearAllHighlight();
        if (_endPickController) _endPickController->clearAllHighlight();
        if (_objectPickController) _objectPickController->clearAllHighlight();

        QWidget* page = nullptr;
        switch (_currentMode)
        {
        case DefaultDirection:
        case XAxis:
        case YAxis:
        case ZAxis:
            page = nullptr;
            setPickWidgetVisible(false);
            emit currentWayNoPickWidget();
            break;
        case CustomCoordinate:
            ensureCustomPage();
            page = _customPage;
            setPickWidgetVisible(true);
            _startPickController->setPickActive(true);
            // ========== 新增：默认激活起点拾取并显示渲染 ==========
            _activePickController = _startPickController;
            updatePreviewRender(_startPickController);
            break;
        case SelectObject:
            ensureObjectPage();
            page = _objectPage;
            setPickWidgetVisible(true);
            _objectPickController->setPickActive(true);
            break;
        case DirectInput:
            ensureDirectPage();
            page = _directPage;
            setPickWidgetVisible(false);
            emit currentWayNoPickWidget();
            break;
        }
        setContentPage(page);
        updateDirectionFromAxis();

        // ========== 新增：非自定义坐标模式下清除所有预览渲染 ==========
        if (_currentMode != CustomCoordinate)
        {
            clearPreviewRender();
        }

        emit directionChanged();
    }

    void mDirectionSelectorWidget::onReverseClicked()
    {
        if (_currentMode == DefaultDirection)
            return;
        _reversed = !_reversed;

        // CustomCoordinate 模式下，反向应体现为起点与终点物理交换，
        // 而不是仅靠 _reversed 在 getDirection 中取反。
        if (_currentMode == CustomCoordinate)
        {
            ensureCustomPage();
            if (_startDirEdit && _endDirEdit)
            {
                const QString startText = _startDirEdit->text();
                const QString endText = _endDirEdit->text();
                _startDirEdit->setText(endText);
                _endDirEdit->setText(startText);
            }
            std::swap(_cachedStartCoord, _cachedEndCoord);
            std::swap(_cachedStartCoordValid, _cachedEndCoordValid);
            _customUntouched = false;
            onCustomInputChanged();

            // 同步更新预览渲染点位置
            if (_startPickController && _startPickController->isPickActive())
                updatePreviewRender(_startPickController);
            else if (_endPickController && _endPickController->isPickActive())
                updatePreviewRender(_endPickController);
        }
        else
        {
            emit directionChanged();
        }

        updateReverseIcon();
    }

    void mDirectionSelectorWidget::setDirectionCalculator(DirectionCalculator calculator)
    {
        // 外部传入空函数时保留默认计算器，避免后续选择对象/自定义坐标模式无法计算方向。
        _calculator = calculator ? std::move(calculator) : &defaultDirectionCalculator;
    }

    void mDirectionSelectorWidget::setCoordinateCalculator(CoordinateCalculator calculator)
    {
        // 外部传入空函数时保留默认计算器，避免后续自定义坐标模式点击"应用"无法计算坐标。
        _coordinateCalculator = calculator ? std::move(calculator) : &defaultCoordinateCalculator;
    }

    void mDirectionSelectorWidget::updateDirectionFromAxis()
    {
        if (_currentMode == XAxis)
            _currentDir = QVector3D(1, 0, 0);
        else if (_currentMode == YAxis)
            _currentDir = QVector3D(0, 1, 0);
        else if (_currentMode == ZAxis)
            _currentDir = QVector3D(0, 0, 1);
        else if (_currentMode == CustomCoordinate)
        {
            QVector3D startDir = parseVectorString(_startDirEdit->text());
            QVector3D endDir = parseVectorString(_endDirEdit->text());
            QVector3D dir = endDir - startDir;
            if (dir.lengthSquared() > 1e-12)
                dir.normalize();
            else
                dir = QVector3D(1, 0, 0);
            _currentDir = dir;
        }
        else if (_currentMode == SelectObject)
        {
            QVector3D dir = computeDirectionFromController(_objectPickController);
            if (dir.lengthSquared() < 1e-12)
                dir = QVector3D(1, 0, 0);
            else
                dir.normalize();
            _currentDir = dir;
        }
    }

    QVector3D mDirectionSelectorWidget::computeDirectionFromController(const mPickWidgetController* controller) const
    {
        if (!controller || !_calculator)
            return QVector3D();
        const auto& data = controller->getAllData();
        return _calculator(data.pickedData);
    }

    QVector3D mDirectionSelectorWidget::computeCoordinateFromController(const mPickWidgetController* controller) const
    {
        if (!controller || !_coordinateCalculator)
            return QVector3D();
        const auto& data = controller->getAllData();
        return _coordinateCalculator(data.pickedData);
    }

    void mDirectionSelectorWidget::applyControllerCoordinate(mPickWidgetController* controller, QLineEdit* edit)
    {
        if (_currentMode != CustomCoordinate || !_coordinateCalculator || !controller || !edit)
        {
            qDebug() << "[DirectionSelector] apply coordinate ignored: mode=" << _currentMode
                << " calculator=" << (_coordinateCalculator ? "yes" : "no")
                << " controller=" << controller
                << " edit=" << edit;
            return;
        }

        // 核心修改：使用预缓存的拾取坐标，避免点击按钮时拾取失活导致数据为空
        QVector3D coord;
        bool hasValidPickCoord = false;
        if (controller == _startPickController) {
            coord = _cachedStartCoord;
            hasValidPickCoord = _cachedStartCoordValid;
        }
        else if (controller == _endPickController) {
            coord = _cachedEndCoord;
            hasValidPickCoord = _cachedEndCoordValid;
        }
        else {
            // 兜底：异常情况再尝试实时计算
            coord = computeCoordinateFromController(controller);
            hasValidPickCoord = isPickDataValid(controller->getAllData().pickedData);
        }

        if (!hasValidPickCoord)
        {
            // 无有效拾取数据：不修改坐标/方向，仅做清理，避免输入框被默认值覆盖
            qDebug() << "[DirectionSelector] apply ignored: no valid pick data";
            controller->displayData(PickWidgetData());
            controller->clearAllHighlight();
            if (_activePickController == controller)
            {
                updatePreviewRender(controller);
            }
            return;
        }

        qDebug() << "[DirectionSelector] apply coordinate:" << coord;
        setInputLineEditText(edit, coord);
        _customUntouched = false;

        // 清空拾取框数据与高亮，并标记缓存坐标已失效
        controller->displayData(PickWidgetData());
        controller->clearAllHighlight();
        if (controller == _startPickController)
            _cachedStartCoordValid = false;
        else if (controller == _endPickController)
            _cachedEndCoordValid = false;

        // 更新渲染点：拾取已清空，自动回退到输入框坐标保留渲染
        if (_activePickController == controller)
        {
            updatePreviewRender(controller);
        }

        onCustomInputChanged();
    }

    void mDirectionSelectorWidget::onApplyStartDirection()
    {
        applyControllerCoordinate(_startPickController, _startDirEdit);
    }

    void mDirectionSelectorWidget::onApplyEndDirection()
    {
        applyControllerCoordinate(_endPickController, _endDirEdit);
    }

    void mDirectionSelectorWidget::onObjectPickedChanged()
    {
        if (_currentMode != SelectObject)
            return;
        if (!_calculator)
        {
            return;
        }
        QVector3D dir = computeDirectionFromController(_objectPickController);
        if (dir.lengthSquared() < 1e-12)
            dir = QVector3D(0, 0, 0);
        else
            dir.normalize();
        _currentDir = dir;
        emit directionChanged();
    }

    // mPickWidgetController::setPickActive 内部已实现全局互斥，
    // 此处直接激活目标控件，全局互斥会自动失活之前激活的其它控件。
    void mDirectionSelectorWidget::onPickControllerActivated(mPickWidgetController* controller)
    {
        if (!controller)
            return;

        // 处理前一个激活控件：清除其预览渲染（仅自定义坐标模式下的拾取控件）
        if (_activePickController && _activePickController != controller)
        {
            if (_currentMode == CustomCoordinate &&
                (_activePickController == _startPickController || _activePickController == _endPickController))
            {
                clearPickPreview(_activePickController);
            }
        }

        // 原有逻辑：激活拾取控件（全局互斥由控件内部实现）
        controller->setPickActive(true);

        // 更新当前激活的控件记录
        _activePickController = controller;

        // 自定义坐标模式下，更新对应拾取控件的预览渲染
        if (_currentMode == CustomCoordinate &&
            (controller == _startPickController || controller == _endPickController))
        {
            updatePreviewRender(controller);
        }
    }

    void mDirectionSelectorWidget::setInputLineEditText(QLineEdit* edit, const QVector3D& vec)
    {
        if (edit)
            edit->setText(vectorToString(vec));
    }

    QVector3D mDirectionSelectorWidget::parseLineEditToVector(const QLineEdit* edit) const
    {
        if (!edit)
            return QVector3D();
        return parseVectorString(edit->text());
    }

    void mDirectionSelectorWidget::onDirectInputChanged()
    {
        if (_currentMode != DirectInput)
            return;
        QVector3D dir = parseLineEditToVector(_directEdit);
        if (dir.lengthSquared() > 1e-12)
            dir.normalize();
        else
            dir = QVector3D(0, 0, 0);
        _currentDir = dir;
        emit directionChanged();
    }

    void mDirectionSelectorWidget::onCustomInputChanged()
    {
        if (_currentMode != CustomCoordinate)
            return;
        QVector3D startDir = parseVectorString(_startDirEdit->text());
        QVector3D endDir = parseVectorString(_endDirEdit->text());
        QVector3D dir = endDir - startDir;
        if (dir.lengthSquared() > 1e-12)
            dir.normalize();
        else
            dir = QVector3D(0, 0, 0);
        _currentDir = dir;
        emit directionChanged();
    }

    QVariant mDirectionSelectorWidget::getDirection() const
    {
        if (_currentMode == DefaultDirection)
            return QVariant();

        QVector3D dir;
        if (_currentMode == XAxis)
            dir = QVector3D(1, 0, 0);
        else if (_currentMode == YAxis)
            dir = QVector3D(0, 1, 0);
        else if (_currentMode == ZAxis)
            dir = QVector3D(0, 0, 1);
        else if (_currentMode == DirectInput)
            dir = parseLineEditToVector(_directEdit);
        else if (_currentMode == CustomCoordinate)
            dir = _currentDir;
        else if (_currentMode == SelectObject)
            dir = _currentDir;

        // CustomCoordinate 模式下，反向已通过起点/终点物理交换体现，
        // 不再依赖 _reversed 在最终输出时取反。
        if (_reversed && _currentMode != CustomCoordinate)
            dir = -dir;
        return QVariant::fromValue(dir);
    }

    bool mDirectionSelectorWidget::isReversed() const
    {
        return _reversed;
    }

    void mDirectionSelectorWidget::setReversed(bool reversed)
    {
        _reversed = reversed;
        if (_reverseBtn)
        {
            _reverseBtn->setChecked(_reversed);
            _reverseBtn->setToolTip(_reversed ? QStringLiteral("已反向") : QStringLiteral("反向"));
            updateReverseIcon();
        }
    }

    void mDirectionSelectorWidget::updateReverseIcon()
    {
        if (!_reverseBtn)
            return;
       _reverseBtn->setIcon(QIcon(_reversed
            ? ":/mMainWidgets/Icon/fanxiang2@3x.png"
            : ":/mMainWidgets/Icon/fanxiang1@3x.png"));
    }

    QVector3D mDirectionSelectorWidget::startPoint() const
    {
        return _startDirEdit ? parseVectorString(_startDirEdit->text()) : QVector3D();
    }

    QVector3D mDirectionSelectorWidget::endPoint() const
    {
        return _endDirEdit ? parseVectorString(_endDirEdit->text()) : QVector3D();
    }

    void mDirectionSelectorWidget::setCustomCoordinatePoints(const QVector3D& start, const QVector3D& end)
    {
        ensureCustomPage();
        _customUntouched = false;
        setInputLineEditText(_startDirEdit, start);
        setInputLineEditText(_endDirEdit, end);
        QVector3D dir = end - start;
        if (dir.lengthSquared() > 1e-12)
            dir.normalize();
        else
            dir = QVector3D(0, 0, 0);
        _currentDir = dir;
    }

    void mDirectionSelectorWidget::setDirection(const QVector3D& dir)
    {
        if (_currentMode == XAxis || _currentMode == YAxis || _currentMode == ZAxis || _currentMode == DefaultDirection)
            return;

        if (_currentMode == DirectInput)
        {
            ensureDirectPage();
            setInputLineEditText(_directEdit, dir);
            _currentDir = dir;
        }
        else if (_currentMode == CustomCoordinate)
        {
            ensureCustomPage();
            // 反显时把方向回写到起点/终点编辑框：
            // 未反向时起点为原点、终点为 dir；已反向时（起点/终点已交换）则相反。
            if (_reversed)
            {
                setInputLineEditText(_startDirEdit, dir);
                setInputLineEditText(_endDirEdit, QVector3D(0, 0, 0));
            }
            else
            {
                setInputLineEditText(_startDirEdit, QVector3D(0, 0, 0));
                setInputLineEditText(_endDirEdit, dir);
            }
            // 起点/终点已按 _reversed 设置，实际方向需与之保持一致。
            _currentDir = _reversed ? -dir : dir;
        }
        else
        {
            _currentDir = dir;
        }
    }

    QString mDirectionSelectorWidget::magnitudeValue() const
    {
        return _magnitudeInput.lineeditvalue ? _magnitudeInput.lineeditvalue->text() : QString();
    }

    void mDirectionSelectorWidget::setMagnitudeValue(const QString& value)
    {
        if (_magnitudeInput.lineeditvalue)
            _magnitudeInput.lineeditvalue->setExpression(value);
    }

    void mDirectionSelectorWidget::setMagnitudeUnit(const QString& unitText)
    {
        if (_magnitudeInput.lineeditvalue)
            _magnitudeInput.lineeditvalue->setUnit(unitText);
    }

    void mDirectionSelectorWidget::setMagnitudeUnitVisible(bool visible)
    {
        // 单位统一显示在输入框内部，外部标签始终隐藏
        if (_magnitudeInput.labelunit)
            _magnitudeInput.labelunit->setVisible(false);
        if (_magnitudeInput.lineeditvalue)
        {
            QString currentText = _magnitudeInput.lineeditvalue->text();
            if (visible)
            {
                // 保持当前单位，无需清空
            }
            else
            {
                _magnitudeInput.lineeditvalue->setUnit("");
            }
        }
    }

    void mDirectionSelectorWidget::setMagnitudeVisible(bool visible)
    {
        if (_magnitudeWidget)
            _magnitudeWidget->setVisible(visible);
    }

    void mDirectionSelectorWidget::setMagnitudeLabel(const QString& label)
    {
        if (_magnitudeInput.labelname)
            _magnitudeInput.labelname->setText(label);
    }

    void mDirectionSelectorWidget::onMagnitudeChanged()
    {
        emit magnitudeChanged();
    }

    PickWidgetData mDirectionSelectorWidget::startPickData() const
    {
        return _startPickController ? _startPickController->getAllData() : PickWidgetData();
    }

    PickWidgetData mDirectionSelectorWidget::endPickData() const
    {
        return _endPickController ? _endPickController->getAllData() : PickWidgetData();
    }

    PickWidgetData mDirectionSelectorWidget::objectPickData() const
    {
        return _objectPickController ? _objectPickController->getAllData() : PickWidgetData();
    }

    void mDirectionSelectorWidget::setStartPickData(const PickWidgetData& data)
    {
        ensureCustomPage();
        if (_startPickController)
        {
            _startPickController->displayData(data);
            // 反显拾取数据时同步刷新缓存，保证“应用”按钮可用
            if (isPickDataValid(data.pickedData))
            {
                _cachedStartCoord = computeCoordinateFromController(_startPickController);
                _cachedStartCoordValid = true;
            }
        }
    }

    void mDirectionSelectorWidget::setEndPickData(const PickWidgetData& data)
    {
        ensureCustomPage();
        if (_endPickController)
        {
            _endPickController->displayData(data);
            if (isPickDataValid(data.pickedData))
            {
                _cachedEndCoord = computeCoordinateFromController(_endPickController);
                _cachedEndCoordValid = true;
            }
        }
    }

    void mDirectionSelectorWidget::setObjectPickData(const PickWidgetData& data)
    {
        ensureObjectPage();
        if (_objectPickController)
            _objectPickController->displayData(data);
    }

    void mDirectionSelectorWidget::setWidgetEnabled(bool enabled)
    {
        _modeCombo->setEnabled(enabled);
        _reverseBtn->setEnabled(enabled && _currentMode != DefaultDirection);
        if (_startPickController) _startPickController->widget()->setEnabled(enabled);
        if (_endPickController) _endPickController->widget()->setEnabled(enabled);
        if (_objectPickController) _objectPickController->widget()->setEnabled(enabled);
        if (_startDirEdit) _startDirEdit->setEnabled(enabled);
        if (_endDirEdit) _endDirEdit->setEnabled(enabled);
        if (_directEdit) _directEdit->setEnabled(enabled);
        if (_startApplyBtn) _startApplyBtn->setEnabled(enabled);
        if (_endApplyBtn) _endApplyBtn->setEnabled(enabled);
        _magnitudeInput.setDisableAll(!enabled);
    }

    void mDirectionSelectorWidget::updatePreviewRender(mPickWidgetController* controller)
    {
        if (!controller || _currentMode != CustomCoordinate)
            return;

        // 逐级判空，避免空指针崩溃
        auto cvm = getCVM;
        if (!cvm || !cvm->getPreRenderWidget())
            return;
        auto render = cvm->getPreRenderWidget()->getCommonShapeRender();
        if (!render)
            return;

        QString renderKey;
        QLineEdit* edit = nullptr;

        // 匹配拾取控件对应的渲染key和输入框
        if (controller == _startPickController)
        {
            renderKey = "dir_selector_start_preview";
            edit = _startDirEdit;
        }
        else if (controller == _endPickController)
        {
            renderKey = "dir_selector_end_preview";
            edit = _endDirEdit;
        }
        else
        {
            return;
        }

        if (!edit)
            return;

        QVector3D renderPos;
        const auto& pickData = controller->getAllData();

        // 核心逻辑：有有效拾取数据用拾取计算坐标，无数据用输入框坐标
        if (isPickDataValid(pickData.pickedData))
        {
            renderPos = computeCoordinateFromController(controller);
        }
        else
        {
            renderPos = parseLineEditToVector(edit);
        }

        // 颜色：主强调色（UiPalette 语义色，随主题）
        const QColor previewColor = UiPalette::primary();
        QVector3D color(previewColor.redF(), previewColor.greenF(), previewColor.blueF());

        // 先移除旧点避免叠加，再添加新点
        render->deletePoint(renderKey);
        render->appendPoint(renderKey, { renderPos }, color);
    }

    void mDirectionSelectorWidget::clearPickPreview(mPickWidgetController* controller)
    {
        if (!controller)
            return;

        auto cvm = getCVM;
        if (!cvm || !cvm->getPreRenderWidget())
            return;
        auto render = cvm->getPreRenderWidget()->getCommonShapeRender();
        if (!render)
            return;

        QString renderKey;
        if (controller == _startPickController)
            renderKey = "dir_selector_start_preview";
        else if (controller == _endPickController)
            renderKey = "dir_selector_end_preview";
        else
            return;

        render->deletePoint(renderKey);
    }

    void mDirectionSelectorWidget::clearPreviewRender()
    {
        auto cvm = getCVM;
        if (!cvm || !cvm->getPreRenderWidget())
            return;
        auto render = cvm->getPreRenderWidget()->getCommonShapeRender();
        if (!render)
            return;

        render->deletePoint("dir_selector_start_preview");
        render->deletePoint("dir_selector_end_preview");

        _activePickController = nullptr;
    }

    void mDirectionSelectorWidget::resetToDefault()
    {
        // 重置控件到默认状态时不应触发外部保存/刷新逻辑，
        // 否则在 setInputsValue 反显前调用会把新加载数据对象再次写脏。
        const bool wasBlocked = blockSignals(true);

        // 1. 重置内部缓存数据
        _cachedStartCoord = QVector3D(0, 0, 0);
        _cachedEndCoord = QVector3D(1, 0, 0);
        _cachedStartCoordValid = false;
        _cachedEndCoordValid = false;
        _currentDir = QVector3D(1, 0, 0);
        _customUntouched = true;

        // 2. 重置各页面输入内容（仅处理已创建的页面，不触发新建）
        // 自定义坐标页
        if (_customPage)
        {
            setInputLineEditText(_startDirEdit, QVector3D(0, 0, 0));
            setInputLineEditText(_endDirEdit, QVector3D(1, 0, 0));

            if (_startPickController)
                _startPickController->displayData(PickWidgetData());
            if (_endPickController)
                _endPickController->displayData(PickWidgetData());
        }

        // 选择对象页
        if (_objectPage && _objectPickController)
        {
            _objectPickController->displayData(PickWidgetData());
        }

        // 直接输入页
        if (_directPage && _directEdit)
        {
            _directEdit->clear();
        }

        // 3. 重置大小输入框
        if (_magnitudeInput.lineeditvalue)
        {
            _magnitudeInput.lineeditvalue->clear();
        }

        // 4. 清除所有预览渲染与拾取激活状态
        clearPreviewRender();

        // 5. 切回初始模式
        // 内部自动处理：反向状态复位、页面显隐切换、拾取高亮清除、布局刷新、发射 directionChanged 信号
        setDirectionMode(_initialMode);

        blockSignals(wasBlocked);

        // 6. 通知外部大小值已重置（输入框 clear 不会触发 editingFinished 信号，需手动发射）
        // 但 resetToDefault 是程序化重置，外部不应响应，故不再 emit magnitudeChanged。
    }
    void mDirectionSelectorWidget::clearAllHighlight()
    {
        if (_currentMode == mDirectionSelectorWidget::SelectObject && _objectPickController && _objectPickController->isPickWidgetVisible())
        {
            _objectPickController->clearAllHighlight();
        }
    }
}
