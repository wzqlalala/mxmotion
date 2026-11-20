#include "mMeshNormalPanel.h"
#include "mMeshPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"

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
#include <QHBoxLayout>
#include <QMetaType>      // 元类型注册函数
#include <QRadioButton>
#include <QColorDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSlider>
#include <QLineEdit>
#include <QFrame>
#include <QRegularExpression>
#include <QTimer>

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mCaseImplicitLinearStatic.h"
#include "mCaseImplicitModal.h"
#include "mCaseImplicitNonLinearStatic.h"
#include "mCaseImplicitLinearBuckling.h"
#include "mCaseImplicitTransient.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mDataElement.h"
#include "mElementBase.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/mFontRender.h"
//PreData
#include "mGeoManager.h"
//Mesh
//#include "MXMeshAPI.h"
#include "MXMesh.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "PickCommonFunction.h"

#include "mInfoWindow.h"
#include "GlobalMeshMessage.h"
#include "mMeshTransform.h"
//#include <mGeoInfoCalculate.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
using namespace MechElementData;
using namespace mxr;
#include <set>
Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{
    mMeshNormalPanel::mMeshNormalPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        // 对象折叠栏
        m_ObjectFold = new FoldSection("对象", this);
        {
            QHBoxLayout* typeLay = new QHBoxLayout;
            typeLay->setMargin(0);
            typeLay->setSpacing(10);
            typeLabel = new PanelLabel("类型");
            shellType = new QRadioButton("壳");
            beamType = new QRadioButton("梁");
            shellType->setChecked(true);
            typeLay->addWidget(typeLabel);
            typeLay->addWidget(shellType);
            typeLay->addWidget(beamType);
            typeLay->addStretch();

            QWidget* typeWid = new QWidget(this);
            typeWid->setLayout(typeLay);
            m_ObjectFold->addRow(typeWid);

            posPick.initWidget("对象");
            posPick.setState(false);
            m_ObjectFold->addRow(posPick.propwid);
        }

        // 参数折叠栏
        m_ParamFold = new FoldSection("参数", this);
        {
            // 动态创建显示样式行，参数行由 updateParamFoldByStyle 添加
            QHBoxLayout* styleLay = new QHBoxLayout;
            styleLay->setMargin(0);
            styleLay->setSpacing(10);
            styleLabel = new PanelLabel("显示样式");
            arrowType = new QRadioButton("箭头");
            colorType = new QRadioButton("颜色");
            pyramidType = new QRadioButton("棱锥");
            arrowType->setChecked(true);
            styleLay->addWidget(styleLabel);
            styleLay->addWidget(arrowType);
            styleLay->addWidget(colorType);
            styleLay->addWidget(pyramidType);
            styleLay->addStretch();
            styleWid = new QWidget(this);
            styleWid->setLayout(styleLay);

            // 底部显示按钮
            showBtn = new QPushButton("显示");
        }

        // 法向调整折叠栏
        m_AdjustFold = new FoldSection("法向调整", this);
        {
            QHBoxLayout* modeLay = new QHBoxLayout;
            modeLay->setMargin(0);
            modeLay->setSpacing(10);
            adjustModeLabel = new PanelLabel("方式");
            autoNormalRadio = new QRadioButton("自动");
            refDirRadio = new QRadioButton("参考方向");
            autoNormalRadio->setChecked(true);
            modeLay->addWidget(adjustModeLabel);
            modeLay->addWidget(autoNormalRadio);
            modeLay->addWidget(refDirRadio);
            modeLay->addStretch();
            adjustModeWid = new QWidget(this);
            adjustModeWid->setLayout(modeLay);
            m_AdjustFold->addRow(adjustModeWid);

            refDirLabel = new PanelLabel("方向向量");
            refDirEdit = new QLineEdit();
            refDirEdit->setPlaceholderText("0,0,1");
            refDirEdit->setMinimumHeight(24);
            refDirEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            m_AdjustFold->addRow(refDirLabel, refDirEdit);
            refDirLabel->setVisible(false);
            refDirEdit->setVisible(false);

            QHBoxLayout* adjustBtnLay = new QHBoxLayout;
            adjustBtnLay->setMargin(0);
            adjustBtnLay->setSpacing(10);
            reversedBtn = new QPushButton("反向");
            unifyBtn = new QPushButton("统一设置");
            reversedBtn->setMinimumWidth(80);
            unifyBtn->setMinimumWidth(80);
            adjustBtnLay->addStretch();
            adjustBtnLay->addWidget(reversedBtn);
            adjustBtnLay->addWidget(unifyBtn);
            QWidget* adjustBtnWid = new QWidget(this);
            adjustBtnWid->setMinimumHeight(28);
            adjustBtnWid->setLayout(adjustBtnLay);
            m_AdjustFold->addRow(adjustBtnWid);
        }

        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_ParamFold);
        mainLay->addWidget(m_AdjustFold);
        mainLay->addStretch();

        // 初始按壳类型布局，默认箭头
        arrowType->setChecked(true);
        onTypeChanged();

        // 类型切换
        connect(shellType, &QRadioButton::toggled, this, [=](bool checked) {
            if (checked) {
                clearSelectionAndHighlight();
                clearNormalSymbol();
                arrowType->setChecked(true);
                onTypeChanged();
            }
            });
        connect(beamType, &QRadioButton::toggled, this, [=](bool checked) {
            if (checked) {
                clearSelectionAndHighlight();
                clearNormalSymbol();
                arrowType->setChecked(true);
                onTypeChanged();
            }
            });

        // 显示样式切换
        connect(arrowType, &QRadioButton::toggled, this, [=](bool checked) {
            if (checked) {
                updateParamFoldByStyle();
                if (m_hasRenderShown) // 仅已点击显示后才自动刷新渲染
                    renderNormal();
            }
            });
        connect(colorType, &QRadioButton::toggled, this, [=](bool checked) {
            if (checked) {
                updateParamFoldByStyle();
                if (m_hasRenderShown)
                    renderNormal();
            }
            });
        connect(pyramidType, &QRadioButton::toggled, this, [=](bool checked) {
            if (checked) {
                updateParamFoldByStyle();
                if (m_hasRenderShown)
                    renderNormal();
            }
            });

        // 参考方向编辑
        connect(refDirEdit, &QLineEdit::textChanged, this, [=]() {
            m_refDirValue = parseVectorImpl(refDirEdit->text());
            if (refDirRadio->isChecked())
                renderNormal();
            });
        connect(autoNormalRadio, &QRadioButton::toggled, this, [=](bool checked) {
            if (checked) {
                refDirLabel->setVisible(false);
                refDirEdit->setVisible(false);
                m_AdjustFold->refreshLayout();
                renderNormal();
            }
            });
        connect(refDirRadio, &QRadioButton::toggled, this, [=](bool checked) {
            if (checked) {
                refDirLabel->setVisible(true);
                refDirEdit->setVisible(true);
                m_refDirValue = parseVectorImpl(refDirEdit->text());
                m_AdjustFold->refreshLayout();
                renderNormal();
            }
            });

        // 对象拾取
        connect(posPick.btnok, &QPushButton::clicked, this, [=]() {
            MeshIDs = pickTargetElements();
            if (!MeshIDs.empty())
            {
                posPick.setState(true);
                posPick.labelpicked->setText(QString("已选择:%1个单元").arg(MeshIDs.size()));
                highlightSelected();
            }
            else
            {
                posPick.setState(false);
                clearSelectionAndHighlight();
            }
            if (_meshOptPanelData)
                _meshOptPanelData->setMeshIDs(MeshIDs);
            });
        connect(posPick.btnclear, &QPushButton::clicked, this, [=]() {
            clearSelectionAndHighlight();
            clearNormalSymbol();
            if (_meshOptPanelData)
                _meshOptPanelData->setMeshIDs(MeshIDs);
            });
        connect(posPick.btnadd, &QPushButton::clicked, this, [=]() {
            auto picked = pickTargetElements();
            for (auto& ele : picked)
            {
                pushBackUnique(MeshIDs, ele);
            }
            if (!MeshIDs.empty())
            {
                posPick.setState(true);
                posPick.labelpicked->setText(QString("已选择:%1个单元").arg(MeshIDs.size()));
                highlightSelected();
            }
            else
            {
                posPick.setState(false);
                clearSelectionAndHighlight();
            }
            if (_meshOptPanelData)
                _meshOptPanelData->setMeshIDs(MeshIDs);
            });

        // 功能按钮
        connect(unifyBtn, &QPushButton::clicked, this, [=]() {
            unifySetNormals();
            });
        connect(reversedBtn, &QPushButton::clicked, this, [=]() {
            reverseSelectedNormals();
            renderNormal();
            });
        connect(showBtn, &QPushButton::clicked, this, [=]() {
            clearHighlightOnly();
            m_hasRenderShown = true; // 标记为已显示状态
            renderNormal();
            });

        // 当选择参考方向时，方向向量仅对统一设置生效；
        // 反向按钮直接取反而不再使用输入的方向，因此无需额外处理反向逻辑。
        // 统一设置后，调用 renderNormal() 同步刷新渲染。
    }

    float mMeshNormalPanel::computeModelScale() const
    {
        auto meshData = getCVM->getModelData()->getMeshData();
        if (!meshData) return 1.0f;

        auto elements = getRenderElements();
        if (elements.empty()) return 1.0f;

        QVector3D minP, maxP;
        bool first = true;

        for (const auto& ele : elements)
        {
            auto nodes = meshData->getNodeIdsByEleIds({ ele });
            for (const auto& node : nodes)
            {
                QVector3D v = meshData->getVectorById(node);
                if (first) { minP = maxP = v; first = false; }
                else
                {
                    minP = QVector3D(std::min(minP.x(), v.x()), std::min(minP.y(), v.y()), std::min(minP.z(), v.z()));
                    maxP = QVector3D(std::max(maxP.x(), v.x()), std::max(maxP.y(), v.y()), std::max(maxP.z(), v.z()));
                }
            }
        }

        float d = (maxP - minP).length();
        if (d < 1e-6f) return 1.0f;
        // 让箭头大小约为包围盒对角线的 1/30
        return d / 30.0f;
    }

    void mMeshNormalPanel::clearNormalSymbol()
    {
        if (!getCVM) return;
        CCSMRendWidget* rend = getCVM->getPreRenderWidget();
        if (!rend) return;

        m_hasRenderShown = false; // 清除渲染后重置显示状态

        // 清除箭头/棱锥符号渲染
        auto symbolController = rend->GetSymbolController();
        if (symbolController)
        {
            QString symbolName = "TempNormal_Render";
            symbolController->deleteSymbol({ symbolName });
            symbolController->UpdateAllSymbol();
        }

        // 清除正反面颜色渲染：传入空vector取消显示
        auto preRender = rend->getPreRender();
        if (preRender)
        {
            std::vector<RenderPickResultData> emptyElements;
            QVector3D dummyFront, dummyBack; // 空列表时颜色参数无实际作用
            preRender->frontBackMeshFaceNorColor(mxr::pickRefsOf(emptyElements), dummyFront, dummyBack);
        }

        rend->updatePaint();
    }

    int mMeshNormalPanel::targetDim() const
    {
        return beamType->isChecked() ? 1 : 2;
    }

    bool mMeshNormalPanel::checkElementDim(const RenderPickResultData& data) const
    {
        auto meshData = getCVM->getModelData()->getMeshData();
        if (!meshData)
            return false;
        int dim = meshData->getDim(data);
        return dim == targetDim();
    }

    std::vector<RenderPickResultData> mMeshNormalPanel::pickTargetElements()
    {
        std::vector<RenderPickResultData> result;
        auto pickMeshIDs = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
        for (auto ele : pickMeshIDs)
        {
            RenderPickResultData rel = SelectData2ResultData(SelectionInfoType::MeshElement, ele);
            if (checkElementDim(rel))
                pushBackUnique(result, rel);
        }
        return result;
    }

    void mMeshNormalPanel::highlightSelected()
    {
        auto widget_ptr = getCVM->getPreRenderWidget();
        auto highlight_controller = widget_ptr->getHighlightController();

        // 仅高亮符合要求的单元，先清空所有拾取/高亮
        widget_ptr->getSelectionContainer()->getMeshSelectData()->ClearAll();
        highlight_controller->clearHighlightMesh();

        if (MeshIDs.empty())
            return;

        std::vector<MeshSelectInfoData> meshSelectInfos;
        meshSelectInfos.reserve(MeshIDs.size());
        for (const auto& rprd : MeshIDs)
        {
            MeshSelectInfoData info;
            info.dim = targetDim();
            info.part_id = rprd.part_id;
            info.data_id.element_id = rprd.result_id;
            info.data_index.face_index = -1;
            info.depth = 0.0f;
            meshSelectInfos.push_back(info);
        }

        MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
        meshData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, meshSelectInfos);
        highlight_controller->updateHighlightMeshData(meshData);
    }

    void mMeshNormalPanel::clearSelectionAndHighlight()
    {
        m_hasRenderShown = false; // 重置显示状态
        MeshIDs.clear();
        posPick.setState(false);
        posPick.labelpicked->setText("未选择");

        auto* preRender = getCVM->getPreRenderWidget();
        if (!preRender)
            return;
        preRender->getSelectionContainer()->getMeshSelectData()->ClearAll();
        preRender->getHighlightController()->clearHighlightMesh();
    }

    void mMeshNormalPanel::clearHighlightOnly()
    {
        auto* preRender = getCVM->getPreRenderWidget();
        if (!preRender)
            return;
        preRender->getSelectionContainer()->getMeshSelectData()->ClearAll();
        preRender->getHighlightController()->clearHighlightMesh();
    }

    std::vector<RenderPickResultData> mMeshNormalPanel::getRenderElements() const
    {
        if (!MeshIDs.empty())
            return MeshIDs;

        auto meshData = getCVM->getModelData()->getMeshData();
        if (!meshData)
            return {};

        if (beamType->isChecked())
            return meshData->getAllShow1DElements();
        else
            return meshData->getAllShow2DElements();
    }

    QVector3D mMeshNormalPanel::parseVectorImpl(const QString& text)
    {
        if (text.trimmed().isEmpty())
            return QVector3D(0, 0, 1);

        QStringList list = text.split(QRegularExpression("[,\\s]+"));
        if (list.size() >= 3)
        {
            return QVector3D(list[0].toDouble(), list[1].toDouble(), list[2].toDouble());
        }
        return QVector3D(0, 0, 1);
    }

    void mMeshNormalPanel::computeShellNormalData(const std::vector<RenderPickResultData>& elements,
        QVector<std::pair<QVector3D, QVector3D>>& posDir) const
    {
        auto meshData = getCVM->getModelData()->getMeshData();
        if (!meshData)
            return;

        for (const auto& ele : elements)
        {
            QVector3D outVec;
            QVector3D centerPos;
            meshData->GetOutVectorByMeshID(ele.result_id, outVec, centerPos);
            if (outVec.lengthSquared() < 1e-12)
                continue;
            posDir.append({ centerPos, outVec });
        }
    }

    static QVector3D parseBeamValueVector(const QString& beamValue)
    {
        if (beamValue.trimmed().isEmpty())
            return QVector3D(0, 0, 1);

        // 同时支持逗号和空格分隔
        QStringList list = beamValue.split(QRegularExpression("[,\\s]+"));
        if (list.size() >= 3)
        {
            return QVector3D(list[0].toDouble(), list[1].toDouble(), list[2].toDouble());
        }
        return QVector3D(0, 0, 1);
    }

    void mMeshNormalPanel::computeBeamNormalData(const std::vector<RenderPickResultData>& elements,
        QVector<std::pair<QVector3D, QVector3D>>& posDir) const
    {
        auto meshData = getCVM->getModelData()->getMeshData();
        auto elementData = getCVM->getModelData()->getElementData();
        if (!meshData || !elementData)
            return;

        for (const auto& it : elements)
        {
            mElementBase* ele = elementData->getElementDataByMeshID(it.result_id);
            mBeamElement* beam = dynamic_cast<mBeamElement*>(ele);
            if (!beam)
                continue;

            std::vector<RenderPickResultData> nodes = meshData->getNodeIdsByEleIds({ it });
            if (nodes.size() < 2)
                continue;

            QVector3D pos = meshData->getVectorById(nodes[0]);
            QVector3D pos1 = meshData->getVectorById(nodes[1]);

            QVector3D temp_pos = pos1 - pos;
            QVector3D beamVec = parseBeamValueVector(beam->_beamValue);
            if (beamVec.lengthSquared() < 1e-12f || temp_pos.lengthSquared() < 1e-12f)
                continue;

            QVector3D normal = QVector3D::crossProduct(temp_pos, beamVec);
            if (normal.lengthSquared() < 1e-12f)
                continue;

            QVector3D centerPos = (pos + pos1) / 2.0f;
            posDir.append({ centerPos, normal });
        }
    }

    void mMeshNormalPanel::reverseSelectedNormals()
    {
        auto elements = getRenderElements();
        if (elements.empty())
            return;

        auto meshData = getCVM->getModelData()->getMeshData();
        auto elementData = getCVM->getModelData()->getElementData();
        if (!meshData || !elementData)
            return;

        if (beamType->isChecked())
        {
            for (const auto& it : elements)
            {
                mElementBase* ele = elementData->getElementDataByMeshID(it.result_id);
                mBeamElement* beam = dynamic_cast<mBeamElement*>(ele);
                if (!beam)
                    continue;

                QVector3D beamVec = parseBeamValueVector(beam->_beamValue);
                if (beamVec.lengthSquared() < 1e-12f)
                    continue;

                beamVec = -beamVec;
                beam->_beamValue = QString("%1,%2,%3").arg(beamVec.x()).arg(beamVec.y()).arg(beamVec.z());
            }
        }
        else
        {
            meshData->adjustMeshNormal(elements);
        }

        renderNormal();
    }

    void mMeshNormalPanel::renderNormal()
    {
        auto elements = getRenderElements();
        if (elements.empty())
        {
            // 无有效单元时直接清除所有渲染
            clearNormalSymbol();
            return;
        }

        CCSMRendWidget* rend = getCVM->getPreRenderWidget();
        if (!rend)
            return;

        // 先清除上一次所有法向渲染（符号+正反面颜色）
        clearNormalSymbol();

        // ========== 颜色样式：仅壳(2D)单元支持 ==========
        if (shellType->isChecked() && colorType->isChecked())
        {
            auto preRender = rend->getPreRender();
            if (preRender)
            {
                preRender->frontBackMeshFaceNorColor(mxr::pickRefsOf(elements), curColor1, curColor2);
            }
            rend->updatePaint();
            return;
        }

        // ========== 箭头/棱锥样式：壳和梁均支持 ==========
        auto symbolController = rend->GetSymbolController();
        if (!symbolController)
            return;

        QVector<std::pair<QVector3D, QVector3D>> posDir;
        if (beamType->isChecked())
            computeBeamNormalData(elements, posDir);
        else
            computeShellNormalData(elements, posDir);

        if (posDir.empty())
            return;

        // 符号尺寸计算
        float symbolSize = m_arrowSizeFactor * m_arrowScale;
        if (symbolSize <= 0.0f) symbolSize = 1.0f;

        QString symbolName = "TempNormal_Render";
        if (arrowType->isChecked())
        {
            mxr::Arrow_Font_Load_Implicit* arrow = symbolController->getOrCreateSymbolData<mxr::Arrow_Font_Load_Implicit>(symbolName);
            arrow->_pos_dir = posDir;
            arrow->_color = curColor0;
            arrow->_txt = "";
            arrow->_font_pixel_size = 16.0f;
            arrow->_size = symbolSize;
            arrow->_isFixedPixel = true;
            arrow->_depthState = false;
            arrow->_showState = true;
            symbolController->createSymbol(arrow);
        }
        else if (pyramidType->isChecked())
        {
            mxr::TempNormal* tn = symbolController->getOrCreateSymbolData<mxr::TempNormal>(symbolName);
            tn->_pos_dir = std::vector<std::pair<QVector3D, QVector3D>>(posDir.begin(), posDir.end());
            tn->_color = curColor0;
            tn->_isFixedPixel = true;
            tn->_DepthState = false;
            tn->_showState = true;
            tn->_size = symbolSize;
            symbolController->createSymbol(tn);
        }

        symbolController->UpdateAllSymbol();
        rend->updatePaint();
    }

    void mMeshNormalPanel::updateParamFoldByStyle()
    {
        // 参考 mPropertyPanel::updateParamPanel / updateSectionPanel：
        // 每次切换类型/样式时彻底重建参数区，避免 removeAll() 后控件脱父导致丢失。
        // 持久化控件（styleWid/showBtn）需要在 removeRow 前脱离父窗口，否则会被 QFormLayout 删除。
        if (styleWid)
            styleWid->setParent(nullptr);
        if (showBtn)
            showBtn->setParent(nullptr);

        while (m_ParamFold->layout()->rowCount() > 0)
        {
            m_ParamFold->removeRow(0);
        }

        // 壳类型才显示“显示样式”行
        if (shellType->isChecked())
        {
            m_ParamFold->addRow(styleWid);
            styleWid->setVisible(true);
        }

        // 根据当前显示样式添加动态参数行
        if (shellType->isChecked() && colorType->isChecked())
        {
            buildColorParams();
        }
        else
        {
            // 壳 + 箭头/棱锥，或梁类型默认均使用箭头参数
            buildArrowParams();
        }

        // 底部显示按钮始终保留
        QHBoxLayout* showBtnLay = new QHBoxLayout;
        showBtnLay->setMargin(0);
        showBtnLay->setSpacing(10);
        showBtnLay->addStretch();
        showBtnLay->addWidget(showBtn);
        auto* showBtnWid = new QWidget(this);
        showBtnWid->setLayout(showBtnLay);
        m_ParamFold->addRow(showBtnWid);

        // 刷新布局，与属性面板一致
        QTimer::singleShot(0, m_ParamFold, [=]() {
            m_ParamFold->refreshLayout();
            });
    }

    void mMeshNormalPanel::buildArrowParams()
    {
        auto* arrowColorLabel = new PanelLabel("箭头颜色");
        auto* arrowColorBtn = new QToolButton();
        arrowColorBtn->setStyleSheet(QString("background:%1").arg(QColor(curColor0.x(), curColor0.y(), curColor0.z()).name()));
        connect(arrowColorBtn, &QToolButton::clicked, this, [=]() {
            QColor _color = QColorDialog::getColor(QColor(curColor0.x(), curColor0.y(), curColor0.z()), nullptr, "获取颜色");
            if (!_color.isValid()) return;
            curColor0 = QVector3D(_color.red(), _color.green(), _color.blue());
            arrowColorBtn->setStyleSheet(QString("background:%1").arg(_color.name()));
            renderNormal();
            });
        m_ParamFold->addRow(arrowColorLabel, arrowColorBtn);

        auto* arrowSizeLabel = new PanelLabel("箭头大小");
        auto* arrowSizeSlider = new QSlider(Qt::Horizontal);
        arrowSizeSlider->setRange(0, 100);
        // 由 m_arrowSizeFactor 0.5~3.0 映射回 slider 0~100
        int initValue = qRound((m_arrowSizeFactor - 0.5) / 2.5 * 100.0);
        initValue = qBound(0, initValue, 100);
        arrowSizeSlider->setValue(initValue);
        connect(arrowSizeSlider, &QSlider::valueChanged, this, [=](int value) {
            double percentage = static_cast<double>(value) / 100.0;
            m_arrowSizeFactor = static_cast<float>(0.5 + 2.5 * percentage);
            renderNormal();
            });
        m_ParamFold->addRow(arrowSizeLabel, arrowSizeSlider);
    }

    void mMeshNormalPanel::buildColorParams()
    {
        auto* frontColorLabel = new PanelLabel("正面颜色");
        auto* frontColorBtn = new QToolButton();
        frontColorBtn->setStyleSheet(QString("background:%1").arg(QColor(curColor1.x(), curColor1.y(), curColor1.z()).name()));
        connect(frontColorBtn, &QToolButton::clicked, this, [=]() {
            QColor _color = QColorDialog::getColor(QColor(curColor1.x(), curColor1.y(), curColor1.z()), nullptr, "获取颜色");
            if (!_color.isValid()) return;
            curColor1 = QVector3D(_color.red(), _color.green(), _color.blue());
            frontColorBtn->setStyleSheet(QString("background:%1").arg(_color.name()));
            renderNormal();
            });
        m_ParamFold->addRow(frontColorLabel, frontColorBtn);

        auto* backColorLabel = new PanelLabel("反面颜色");
        auto* backColorBtn = new QToolButton();
        backColorBtn->setStyleSheet(QString("background:%1").arg(QColor(curColor2.x(), curColor2.y(), curColor2.z()).name()));
        connect(backColorBtn, &QToolButton::clicked, this, [=]() {
            QColor _color = QColorDialog::getColor(QColor(curColor2.x(), curColor2.y(), curColor2.z()), nullptr, "获取颜色");
            if (!_color.isValid()) return;
            curColor2 = QVector3D(_color.red(), _color.green(), _color.blue());
            backColorBtn->setStyleSheet(QString("background:%1").arg(_color.name()));
            renderNormal();
            });
        m_ParamFold->addRow(backColorLabel, backColorBtn);
    }

    void mMeshNormalPanel::onTypeChanged()
    {
        m_hasRenderShown = false; // 切换类型，重置显示状态
        bool isShell = shellType->isChecked();

        // 切换类型时重新计算模型缩放，箭头大小因子保持用户设定
        if (isShell)
        {
            m_arrowScale = computeModelScale();
        }
        else
        {
            // 梁类型强制回到箭头样式
            arrowType->setChecked(true);
            m_arrowScale = 1.0f;
        }

        updateParamFoldByStyle();

        // 法向调整折叠框：壳类型显示方式选择，梁类型仅使用参考方向
        if (isShell)
        {
            adjustModeWid->setVisible(true);
            bool refDirChecked = refDirRadio->isChecked();
            refDirLabel->setVisible(refDirChecked);
            refDirEdit->setVisible(refDirChecked);
        }
        else
        {
            adjustModeWid->setVisible(false);
            refDirLabel->setVisible(true);
            refDirEdit->setVisible(true);
        }
        m_AdjustFold->refreshLayout();

        // 切换类型时清空已有的法向渲染和高亮
        clearNormalSymbol();
        clearSelectionAndHighlight();
    }

    void mMeshNormalPanel::unifySetNormals()
    {
        auto elements = getRenderElements();
        if (elements.empty())
            return;

        auto meshData = getCVM->getModelData()->getMeshData();
        auto elementData = getCVM->getModelData()->getElementData();
        if (!meshData || !elementData)
            return;

        if (beamType->isChecked())
        {
            // 梁类型：仅支持“参考方向”模式，以参考方向作为目标法向
            QVector3D refDir = m_refDirValue;
            if (refDir.lengthSquared() < 1e-12f)
                refDir = QVector3D(0, 0, 1);
            else
                refDir.normalize();

            for (const auto& it : elements)
            {
                mElementBase* ele = elementData->getElementDataByMeshID(it.result_id);
                mBeamElement* beam = dynamic_cast<mBeamElement*>(ele);
                if (!beam)
                    continue;

                // 当前梁矢量
                QVector3D curDir = parseBeamValueVector(beam->_beamValue);
                if (curDir.lengthSquared() < 1e-12f)
                    continue;

                // 梁轴方向
                std::vector<RenderPickResultData> nodes = meshData->getNodeIdsByEleIds({ it });
                if (nodes.size() < 2)
                    continue;
                QVector3D p0 = meshData->getVectorById(nodes[0]);
                QVector3D p1 = meshData->getVectorById(nodes[1]);
                QVector3D axis = p1 - p0;
                if (axis.lengthSquared() < 1e-12f)
                    continue;

                // 当前梁法向 = axis × beamVec
                QVector3D curNormal = QVector3D::crossProduct(axis, curDir);
                if (curNormal.lengthSquared() < 1e-12f)
                    continue;

                // 若输入方向向量与当前法向夹角大于 90° 且小于 180°（点积 < 0），则对 beamVec 取反
                if (QVector3D::dotProduct(refDir, curNormal) < 0.0f)
                {
                    curDir = -curDir;
                    beam->_beamValue = QString("%1,%2,%3").arg(curDir.x()).arg(curDir.y()).arg(curDir.z());
                }
            }
        }
        else
        {
            // 壳类型：根据当前方式选择自动或参考方向
            if (refDirRadio->isChecked())
            {
                QVector3D refDir = m_refDirValue;
                if (refDir.lengthSquared() < 1e-12f)
                    refDir = QVector3D(0, 0, 1);
                else
                    refDir.normalize();
                meshData->unify2DMeshNormal(refDir, elements);
            }
            else
            {
                meshData->unify2DMeshNormal(elements);
            }
        }

        renderNormal();
    }

    mMeshNormalPanel::~mMeshNormalPanel()
    {

    }
    void mMeshNormalPanel::newMeshPanel()
    {

    }
    void mMeshNormalPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        updatePanel();
    }
    void mMeshNormalPanel::getMeshInfoById(const std::vector<int> ids)
    {

    }
    void mMeshNormalPanel::updatePanel()
    {

    }

    void mMeshNormalPanel::hideEvent(QHideEvent* event)
    {
        // 面板切出时清除法向渲染（箭头/棱锥符号、正反面颜色）及拾取高亮，
        // 避免切到其它面板后法向颜色残留
        clearNormalSymbol();
        clearSelectionAndHighlight();
        mPanelBase::hideEvent(event);
    }

    void mMeshNormalPanel::setMeshSize()
    {

    }
    void mMeshNormalPanel::initSize()
    {

    }
}
