#ifndef MX_MOTION_OCC_COMPAT
#include <mGeoManager.h>
#include <mMeshData.h>
#else
#include "picking/LegacyPickContext.h"
#endif
#include "mPointPickerWidget.h"
#include "mGeneralPickUp.h"

#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QPointer>
#ifndef MX_MOTION_OCC_COMPAT
#include "mGeoManager.h"  //【集成回补】GeoManager 完整类型（旧栈测量调用仍存）
#endif
#include "mFoldSection.h" //PanelLabel：行内标签定宽 labelWidth，随档位
#include "mGroupBox.h"    //外层分组框(MxFunctions::mGroupBox 骑线标题)

//尺寸/颜色令牌与主题重发（素材库规则：禁止硬编码颜色/尺寸）
#include "UiMetrics.h"
#include "UiPalette.h"
#include "ThemeManager.h"

using namespace MxFunctions;

namespace MechMainWindow
{
    //辅助工具函数
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

    // 默认坐标计算器（与起点-终点模式的 defaultCoordinateCalculator 逻辑完全一致）
    static QVector3D defaultCoordinateCalculator(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData> >& picks)
    {
#ifdef MX_MOTION_OCC_COMPAT
        auto* context = MotionOcc::LegacyPickContext::current();
        return context ? context->averageCoordinate(picks) : QVector3D();
#else
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
                    if (!pos.isNull()) { sum += pos; ++count; }
                }
                continue;
            }
            else if (type == MxFunctions::PickDataType::Element)
            {
                for (auto id : data)
                {
                    QVector3D pos = meshData->getCentroid(id);
                    if (!pos.isNull()) { sum += pos; ++count; }
                }
                continue;
            }
            else if (type == MxFunctions::PickDataType::ElementEdge)
            {
                for (auto id : data)
                {
                    QVector3D pos1 = meshData->getVectorById(id.result_id);
                    QVector3D pos2 = meshData->getVectorById(id.result_id2);
                    if (!pos1.isNull()) { sum += pos1; ++count; }
                    if (!pos2.isNull()) { sum += pos2; ++count; }
                }
                continue;
            }
            else if (type == MxFunctions::PickDataType::ElementFace)
            {
                for (auto id : data)
                {
                    QVector3D outVec; QVector3D pos;
                    meshData->GetOutVectorByMeshFaceID(id, outVec, pos);
                    if (!pos.isNull()) { sum += pos; ++count; }
                }
                continue;
            }

            std::vector<int> ids;
            ids.reserve(data.size());
            for (const auto& d : data)
                ids.push_back(d.result_id);

            if (type == MxFunctions::PickDataType::GeoPoint)
            {
                auto vertexAttrs = geoManager->CalculateVertexAttr(ids);
                for (const auto& attr : vertexAttrs) { sum += attr.m_coor; ++count; }
                continue;
            }
            if (type == MxFunctions::PickDataType::GeoEdge)
            {
                auto edgeAttrs = geoManager->CalculateEdgeAttr(ids);
                for (const auto& attr : edgeAttrs) { sum += attr.m_coor; ++count; }
                continue;
            }

            if (type == MxFunctions::PickDataType::GeoFace)
            {
                auto faceAttrs = geoManager->CalculateFaceAttr(ids);
                for (const auto& attr : faceAttrs) { sum += attr.m_coor; ++count; }
                continue;
            }
            if (type == MxFunctions::PickDataType::GeoSolid)
            {
                auto solidAttrs = geoManager->CalculateSolidAttr(ids);
                for (const auto& attr : solidAttrs) { sum += attr.m_coor; ++count; }
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
                        if (!pos.isNull()) { sum += pos; ++count; }
                    }
                    else
                    {
                        auto pts = geoManager->CalculateVertexAttr({ id });
                        if (!pts.empty()) { sum += pts.front().m_coor; ++count; }
                    }
                }
                continue;
            }
        }
        if (count == 0) return QVector3D();
        return sum / count;

#endif
    }

    // ========== 类实现 ==========
    mPointPickerWidget::mPointPickerWidget(const QString& groupTitle, const QString& labelText, QWidget* parent)
        : QWidget(parent)
    {
        initUI(groupTitle, labelText);
        setCoordinateCalculator({});

        //档位/主题跟随：切档/切主题后按 UiMetrics 重发行高与标签列宽
        applyThemeStyles();
        QPointer<mPointPickerWidget> guard(this);
        ThemeManager::addDockThemeExtraApplier([guard]() { if (guard) guard->applyThemeStyles(); });
    }

    mPointPickerWidget::~mPointPickerWidget()
    {
        clearPreviewRender();
    }

    void mPointPickerWidget::initUI(const QString& groupTitle, const QString& labelText)
    {
        /*
        * 整体包进 MxFunctions::mGroupBox：骑线标题=groupTitle(业务名，两处可设置
        * 文字之一，运行时可经 setGroupTitle 修改)；内部保留原生两行结构
        * [标签 | 拾取控件 | 应用] + 坐标输入框(标签默认"模型拾取")
        */
        QVBoxLayout* outerLayout = new QVBoxLayout(this);
        outerLayout->setContentsMargins(0, 0, 0, 0);
        outerLayout->setSpacing(0);
        _groupBox = new mGroupBox(groupTitle, QPixmap(), QPixmap(), this);
        _groupBox->setCheckable(false);
        outerLayout->addWidget(_groupBox);

        _mainLayout = _groupBox->grid();
        _mainLayout->setHorizontalSpacing(UiMetrics::columnGap());
        _mainLayout->setVerticalSpacing(UiMetrics::rowGap());
        _mainLayout->setAlignment(Qt::AlignTop);

        // 第一行：标签 + 拾取控件 + 应用按钮（标签走 PanelLabel 定宽，随档位）
        _label = new PanelLabel(labelText, _groupBox->contentWidget());
        _label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        _pickController = mGeneralPickUp::createDirectionPickWidgetPickMore(CBNone, _groupBox->contentWidget());
        _pickController->setDefultTip(QStringLiteral("请选择"), groupTitle.isEmpty() ? labelText : groupTitle);
        _pickController->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        _applyBtn = new QToolButton(_groupBox->contentWidget());
        _applyBtn->setText(QStringLiteral("应用"));

        // 第二行：输入框（跨第1、2列，与上方拾取+应用按钮右边缘对齐，左侧第0列留白）
        _edit = new QLineEdit(_groupBox->contentWidget());
        _edit->setValidator(createVectorValidator(_edit));
        _edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        // 布局装配
        _mainLayout->addWidget(_label, 0, 0, Qt::AlignVCenter);
        _mainLayout->addWidget(_pickController->widget(), 0, 1, Qt::AlignVCenter);
        _mainLayout->addWidget(_applyBtn, 0, 2, Qt::AlignVCenter);
        _mainLayout->addWidget(_edit, 1, 1, 1, 2, Qt::AlignVCenter);

        // 信号槽连接
        connect(_applyBtn, &QToolButton::clicked, this, &mPointPickerWidget::onApplyClicked);
        connect(_pickController, &mPickWidgetController::dataChanged, this, &mPointPickerWidget::onPickDataChanged);
        connect(_pickController, &mPickWidgetController::pickActiveChanged, this, &mPointPickerWidget::onPickActiveChanged);
        connect(_edit, &QLineEdit::editingFinished, this, &mPointPickerWidget::onEditFinished);

        // 初始值
        _edit->setText(vectorToString(QVector3D(0, 0, 0)));
    }

    //档位/主题跟随：行高/标签列宽按 UiMetrics 重发。
    //颜色一律由全局 QSS/令牌下发，本函数不拼任何字面色值
    void mPointPickerWidget::applyThemeStyles()
    {
        //正文族/字号（标签/输入行兜底，随档位）
        QFont f(UiMetrics::uiFontFamily());
        f.setPixelSize(UiMetrics::bodyFontSize());
        setFont(f);

        //行高统一控件外高 H；应用按钮同高不锁宽（与方向控件应用按钮一致）
        if (_pickController)
            _pickController->widget()->setFixedHeight(UiMetrics::baseH());
        if (_applyBtn)
            _applyBtn->setFixedHeight(UiMetrics::baseH());
        if (_edit)
            _edit->setFixedHeight(UiMetrics::baseH());

        //行内标签列宽（PanelLabel 构造时定宽，切档后重发）
        if (_label)
            _label->setFixedWidth(UiMetrics::labelWidth());
    }

    void mPointPickerWidget::onApplyClicked()
    {
        applyPickCoordinate();
        //转发公有信号：外部(如坐标系快速创建)区分手动应用
        emit manuallyApplied();
    }

    void mPointPickerWidget::onPickDataChanged()
    {
        const auto& data = _pickController->getAllData();
        // 仅有效拾取数据才更新缓存，避免空数据覆盖有效缓存
        if (isPickDataValid(data.pickedData))
        {
            _cachedCoord = computeCoordinateFromController();
        }
        // 激活状态下同步更新预览渲染
        if (_isPickActive)
        {
            updatePreviewRender();
        }
        emit coordinateChanged();
    }

    void mPointPickerWidget::onPickActiveChanged(bool isActive)
    {
        _isPickActive = isActive;
        if (isActive)
        {
            emit pickControllerActivated(_pickController);
            updatePreviewRender();
        }
        else
        {
            clearPreviewRender();
        }
    }

    void mPointPickerWidget::onEditFinished()
    {
        // 输入框编辑完成后，同步更新预览渲染
        if (_isPickActive)
        {
            updatePreviewRender();
        }
        /*
        * editConfirmed 先发、coordinateChanged 后发：外部(如坐标系面板)可在
        * editConfirmed 中先置"手动输入"标志，随后到达的 coordinateChanged 据此
        * 跳过拾取类快速创建处理(自动应用+接力下一格)——手动输入只回车即写，不跳转
        */
        emit editConfirmed();
        emit coordinateChanged();
    }

    void mPointPickerWidget::applyPickCoordinate()
    {
        if (!_coordinateCalculator || !_pickController || !_edit)
        {
            qDebug() << "[PointPicker] apply coordinate ignored: calculator or widget null";
            return;
        }

        // 核心：使用预缓存的拾取坐标，避免点击按钮时拾取失活导致数据为空
        QVector3D coord = _cachedCoord;
        _edit->setText(vectorToString(coord));

        // 清空拾取框数据与高亮
        _pickController->displayData(PickWidgetData(), true);
        _pickController->clearAllHighlight();

        // 拾取清空后，回退到用输入框坐标保留渲染
        if (_isPickActive)
        {
            updatePreviewRender();
        }

        emit coordinateChanged();
    }

    QVector3D mPointPickerWidget::computeCoordinateFromController() const
    {
        if (!_pickController || !_coordinateCalculator)
            return QVector3D();
        const auto& data = _pickController->getAllData();
        return _coordinateCalculator(data.pickedData);
    }

    void mPointPickerWidget::updatePreviewRender()
    {
#ifdef MX_MOTION_OCC_COMPAT
        auto* context = MotionOcc::LegacyPickContext::forObject(this);
        if (!context || !_pickController) return;
        const auto& data = _pickController->getAllData().pickedData;
        const QVector3D point = isPickDataValid(data) ? computeCoordinateFromController() : coordinate();
        context->setPreviewPoints(QStringLiteral("point:%1").arg(quintptr(this)), {point}, UiPalette::primary());
#else
        auto cvm = getCVM;
        if (!cvm || !cvm->getPreRenderWidget())
            return;
        auto render = cvm->getPreRenderWidget()->getCommonShapeRender();
        if (!render)
            return;

        const QString renderKey = "point_picker_preview";
        QVector3D renderPos;

        const auto& pickData = _pickController->getAllData();
        // 有有效拾取数据用拾取计算坐标，无数据用输入框坐标
        if (isPickDataValid(pickData.pickedData))
        {
            renderPos = computeCoordinateFromController();
        }
        else
        {
            renderPos = parseVectorString(_edit->text());
        }

        // 预览点颜色随主题主色（与方向控件预览一致）
        const QColor previewColor = UiPalette::primary();
        QVector3D color(previewColor.redF(), previewColor.greenF(), previewColor.blueF());

        render->deletePoint(renderKey);
        render->appendPoint(renderKey, { renderPos }, color);

#endif
    }

    void mPointPickerWidget::clearPreviewRender()
    {
#ifdef MX_MOTION_OCC_COMPAT
        if (auto* context = MotionOcc::LegacyPickContext::forObject(this))
            context->clearPreviewPoints(QStringLiteral("point:%1").arg(quintptr(this)));
#else
        auto cvm = getCVM;
        if (!cvm || !cvm->getPreRenderWidget())
            return;
        auto render = cvm->getPreRenderWidget()->getCommonShapeRender();
        if (!render)
            return;

        render->deletePoint("point_picker_preview");

#endif
    }

    // ========== 对外接口 ==========
    QVector3D mPointPickerWidget::coordinate() const
    {
        return _edit ? parseVectorString(_edit->text()) : QVector3D();
    }

    void mPointPickerWidget::setCoordinate(const QVector3D& coord)
    {
        if (_edit)
            _edit->setText(vectorToString(coord));
    }

    void mPointPickerWidget::setCoordinateCalculator(CoordinateCalculator calculator)
    {
#ifdef MX_MOTION_OCC_COMPAT
        _coordinateCalculator = calculator ? std::move(calculator) : CoordinateCalculator([this](const auto& picks) {
            auto* context = _pickController ? _pickController->context() : MotionOcc::LegacyPickContext::forObject(this);
            return context ? context->averageCoordinate(picks) : QVector3D();
        });
#else
        _coordinateCalculator = calculator ? std::move(calculator) : &defaultCoordinateCalculator;
#endif
    }

    PickWidgetData mPointPickerWidget::pickData() const
    {
        return _pickController ? _pickController->getAllData() : PickWidgetData();
    }

    void mPointPickerWidget::setPickData(const PickWidgetData& data)
    {
        if (_pickController)
            _pickController->displayData(data);
    }

    void mPointPickerWidget::setWidgetEnabled(bool enabled)
    {
        if (_label) _label->setEnabled(enabled);
        if (_pickController) _pickController->widget()->setEnabled(enabled);
        if (_applyBtn) _applyBtn->setEnabled(enabled);
        if (_edit) _edit->setEnabled(enabled);
    }

    void mPointPickerWidget::setApplyButtonVisible(bool visible)
    {
        if (!_applyBtn || !_mainLayout)
        {
            return;
        }
        if (visible)
        {
            //恢复：按钮放回第0行第2列，编辑框跨第1、2列
            if (_mainLayout->indexOf(_applyBtn) < 0)
            {
                _applyBtn->setParent(this);
                _mainLayout->addWidget(_applyBtn, 0, 2, Qt::AlignVCenter);
            }
            _applyBtn->show();
        }
        else
        {
            //隐藏：按钮摘出布局使第2列塌缩，避免留白错位
            _mainLayout->removeWidget(_applyBtn);
            _applyBtn->setParent(this);
            _applyBtn->hide();
        }
        //编辑框跨列同步：按钮在时跨第1、2列，隐藏时只占第1列与拾取控件对齐
        if (_edit)
        {
            _mainLayout->removeWidget(_edit);
            _mainLayout->addWidget(_edit, 1, 1, 1, visible ? 2 : 1, Qt::AlignVCenter);
        }
    }

    void mPointPickerWidget::setLabelVisible(bool visible)
    {
        if (_label)
        {
            _label->setVisible(visible);
        }
    }

    void mPointPickerWidget::setLabelText(const QString& text)
    {
        if (_label)
        {
            _label->setText(text);
        }
    }

    void mPointPickerWidget::setGroupTitle(const QString& title)
    {
        if (_groupBox)
        {
            _groupBox->setTitle(title);
        }
    }

    void mPointPickerWidget::autoApply()
    {
        applyPickCoordinate();
    }

    void mPointPickerWidget::resetToDefault()
    {
        const bool wasBlocked = blockSignals(true);

        _cachedCoord = QVector3D(0, 0, 0);
        if (_edit) _edit->setText(vectorToString(QVector3D(0, 0, 0)));
        if (_pickController)
        {
            _pickController->displayData(PickWidgetData());
            _pickController->clearAllHighlight();
        }
        clearPreviewRender();
        _isPickActive = false;

        blockSignals(wasBlocked);
    }

    void mPointPickerWidget::clearAllHighlight()
    {
        if (_pickController && _pickController->isPickWidgetVisible())
        {
            _pickController->clearAllHighlight();
        }
    }
}
