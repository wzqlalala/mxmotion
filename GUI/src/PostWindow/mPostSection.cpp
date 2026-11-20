#include "mPostSection.h"

#include <QFormLayout>
#include <algorithm>
#include <QGraphicsDropShadowEffect>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "mCommonFunctions.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mViewTreeFunction.h"

//MxFunctions
#include "mGlobalSignals.h"

//BasicData
#include "mPhysicalFieldEnum.h"

//Post
#include "mPostDataBase.h"
#include "mIMxdbFile1.h"
#include "mMxdbThread.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "PickCommonFunction.h"
//窗口
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mOtherFunction.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;
using namespace MIOFile;

namespace PostWindow
{
    enum AXIS
    {
        X,Y,Z,END
    };
    enum class PLANE_TYPE : uint8_t
    {
        XY,
        YZ,
        ZX,
        NORMAL
    };
    enum class POS_TYPE : uint8_t
    {
        NODE,
        DISTANCE,
        OFFSET
    };
    mPostSectionPanel::mPostSectionPanel(QWidget* parent) : mPanelBase(parent)
    {

        _mainLayout = new QVBoxLayout(this);                                    // 创建垂直布局管理器
        _mainLayout->setContentsMargins(2, 0, 2, 0);                            // 设置布局的左右边距

        /********************切面列表表格********************/
        FoldSection* sectionlist = new FoldSection("切面列表", this);           // 创建可折叠的区块控件

        _tableSection = new QTableWidget(this);                                 // 创建一个表格控件
        _tableSection->setColumnCount(6);                                       // 设置表格列数
        QStringList horizontalHeader = { "序号", "状态", "反向", "滚动条", "值", "删除" };
        _tableSection->setHorizontalHeaderLabels(horizontalHeader);             // 设置表格的水平表头
        _tableSection->verticalHeader()->setHidden(true);                       // 隐藏表格的垂直表头（行号列）
        _tableSection->horizontalHeader()->setMinimumSectionSize(50);           // 设置水平表头最小列宽为50
        _tableSection->resizeColumnsToContents();                               // 自动调整列宽
        _tableSection->horizontalHeader()->setStretchLastSection(true);         // 让表格的最后一列自动拉伸填充剩余空间
        _tableSection->horizontalHeader()->setHighlightSections(false);         // 禁用表头的高亮显示效果
        _tableSection->setSelectionBehavior(QAbstractItemView::SelectRows);     // 设置为整行选中
        _tableSection->setSelectionMode(QAbstractItemView::SingleSelection);    // 设置为单选模式
        _tableSection->setFixedHeight(140);

        sectionlist->addRow(_tableSection);

        _btnNew = new QPushButton("新建");
        _btnBanAll = new QPushButton("禁用全部");
        _btnDelAll = new QPushButton("删除全部");

        sectionlist->addRow(_btnNew, _btnBanAll, _btnDelAll);

        /********************平面********************/
        FoldSection* plane = new FoldSection("平面", this);

        QLabel* _planeShapeLabel = new QLabel("类型");
        _planeShapeBox = new QComboBox();
        _planeShapeBox->addItems({ "XY平面" , "YZ平面" , "ZX平面","任意平面" });//
        plane->addRow(_planeShapeLabel, _planeShapeBox);

        QLabel* planePoint = new QLabel("点1");
        _planePointLineEdits.resize(3);
        _planePointLineEdits[AXIS::X] = new QLineEdit();
        _planePointLineEdits[AXIS::Y] = new QLineEdit();
        _planePointLineEdits[AXIS::Z] = new QLineEdit();
        _planePointBtn = new QToolButton();
        /*
        * 不需要点1， 三种不同类型的切面，其面板都包含了点拾取的功能/不需要点拾取功能。
            plane->addRow(planePoint,
                _planePointLineEdits[AXIS::X],
                _planePointLineEdits[AXIS::Y],
                _planePointLineEdits[AXIS::Z],
                _planePointBtn);
        */

        /********************数量和位置********************/
        _numandpos = new FoldSection("数量和位置", this);
        QLabel* num = new QLabel("数量");
        _numEdit = new QLineEdit("1");
        _numEdit->setValidator(new QIntValidator(0, 8, this));
        _numandpos->addRow(num, _numEdit);

        QLabel* position = new QLabel("位置");
        _radioBasePoint = new QRadioButton("基于点");
        _radioEqualSpacing = new QRadioButton("等间距");
        _radioOffset = new QRadioButton("偏移");
        _radioBasePoint->setChecked(true);
        _radioEqualSpacing->setChecked(false);
        _radioOffset->setChecked(false);
        _numandpos->addRow(position, _radioBasePoint, _radioEqualSpacing, _radioOffset);

        _pointContainer = new QWidget();
        QHBoxLayout* layout_point = new QHBoxLayout(_pointContainer);
        _planePointSelectPanel.initWidget("点");
        _planePointSelectPanel.setState(false);
        layout_point->addWidget(_planePointSelectPanel.propwid);
        _pointContainer->setContentsMargins(0, 0, 0, 0);
        _numandpos->addRow(_pointContainer);

        //向量
        _vectorContainer = new QWidget();
        QHBoxLayout* layout_vector = new QHBoxLayout(_vectorContainer);
        QLabel* vectorLabel = new QLabel("向量");
        QLabel* xLabel = new QLabel("X");
        _xEdit = new QLineEdit("1");
        QLabel* yLabel = new QLabel("Y");
        _yEdit = new QLineEdit("0");
        QLabel* zLabel = new QLabel("Z");
        _zEdit = new QLineEdit("0");
        layout_vector->addWidget(vectorLabel);
        layout_vector->addWidget(xLabel);
        layout_vector->addWidget(_xEdit);
        layout_vector->addWidget(yLabel);
        layout_vector->addWidget(_yEdit);
        layout_vector->addWidget(zLabel);
        layout_vector->addWidget(_zEdit);
        _numandpos->addRow(_vectorContainer);

        _vectorContainer->hide();
        //切换到任意平面时，界面改变
        connect(_planeShapeBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index) {
            if (index == 3)//任意平面
            {
                _vectorContainer->show();
                num->hide();
                _numEdit->hide();
                _radioEqualSpacing->hide();
                _radioOffset->hide();
                // 任意平面仅支持"基于点"，重置选中状态并隐藏旧平面参数（基准/距离等）
                _radioBasePoint->setChecked(true);
            }
            else
            {
                _vectorContainer->hide();
                num->show();
                _numEdit->show();
                _radioEqualSpacing->show();
                _radioOffset->show();
            }

            });

        _baseContainer = new QWidget();
        _baseLineEdits.resize(4);
        QHBoxLayout* layout_base = new QHBoxLayout(_baseContainer);
        QLabel* baseLabel = new QLabel("基准");
        _baseLineEdits[AXIS::X] = new QLineEdit();
        _baseLineEdits[AXIS::X]->setValidator(new QDoubleValidator(this));
        _baseLineEdits[AXIS::Y] = new QLineEdit();
        _baseLineEdits[AXIS::Y]->setValidator(new QDoubleValidator(this));
        _baseLineEdits[AXIS::Z] = new QLineEdit();
        _baseLineEdits[AXIS::Z]->setValidator(new QDoubleValidator(this));
        _basePointBtn = new QToolButton();
        _basePointBtn->setIcon(QIcon(":/mMainWidgets/Icon/post/tihuan.png"));
        _basePointBtn->setToolTip("拾取基准点");
        layout_base->addWidget(baseLabel);
        layout_base->addWidget(_baseLineEdits[AXIS::X]);
        layout_base->addWidget(_baseLineEdits[AXIS::Y]);
        layout_base->addWidget(_baseLineEdits[AXIS::Z]);
        layout_base->addWidget(_basePointBtn);
        _baseContainer->setContentsMargins(0, 0, 0, 0);
        _numandpos->addRow(_baseContainer);
        _baseContainer->hide();

        // 信号槽统一在构造时连接一次，避免 newSectionPanel 多次调用导致重复连接
        connect(_btnNew, &QToolButton::clicked, this, &mPostSectionPanel::slot_newSection);
        connect(_btnBanAll, &QToolButton::clicked, this, &mPostSectionPanel::slot_banAll);
        connect(_btnDelAll, &QToolButton::clicked, this, &mPostSectionPanel::slot_delAll);
        connect(_basePointBtn, &QToolButton::clicked, this, [&]() {innerSlot_planePointBtnClicked(_basePointBtn); });
        connect(_planePointSelectPanel.btnok, &QToolButton::clicked, this, [&]() {innerSlot_countPlaneBtnClicked(); });
        connect(_planePointSelectPanel.btnadd, &QPushButton::clicked, this, &mPostSectionPanel::innerSlot_btnAddClicked);
        connect(_planePointSelectPanel.btnclear, &QToolButton::clicked, this, [&]() {innerSlot_clearAllPickNode(); });
        connect(_tableSection, &QTableWidget::itemClicked, this, [&](QTableWidgetItem* item) {innerSlot_itemPressed(item); });
        connect(_numEdit, &QLineEdit::textChanged, this, [&](const QString& text) {innerSlot_planeCountChanged(text); });
        connect(_numEdit, &QLineEdit::editingFinished, this, [&]() {innerSlot_planeCountFinished(); });
        connect(_planeShapeBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &mPostSectionPanel::slot_planeShapeChanged);
        connect(_radioBasePoint, &QRadioButton::toggled, this, &mPostSectionPanel::slot_positionChanged);
        connect(_radioEqualSpacing, &QRadioButton::toggled, this, &mPostSectionPanel::slot_positionChanged);
        connect(_radioOffset, &QRadioButton::toggled, this, &mPostSectionPanel::slot_positionChanged);

        _distanceContainer = new QWidget();
        QHBoxLayout* layout_distance = new QHBoxLayout(_distanceContainer);
        QLabel* distanceLabel = new QLabel("距离");
        _baseLineEdits[AXIS::END] = new QLineEdit();
        _baseLineEdits[AXIS::END]->setValidator(new QDoubleValidator(this));
        layout_distance->addWidget(distanceLabel);
        layout_distance->addWidget(_baseLineEdits[AXIS::END]);
        _distanceContainer->setContentsMargins(0, 0, 0, 0);
        _numandpos->addRow(_distanceContainer);
        _distanceContainer->hide();

        sectionlist->refreshLayout();
        plane->refreshLayout();
        _numandpos->refreshLayout();
        _numandpos->layout()->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));       // 防止多行FormLayout的数据视图在显影后变得扁扁的。

        m_foldWidgets.append(sectionlist);
        m_foldWidgets.append(plane);
        m_foldWidgets.append(_numandpos);

        _mainLayout->addWidget(sectionlist);
        _mainLayout->addWidget(plane);
        _mainLayout->addWidget(_numandpos);

        _mainLayout->addStretch();


    }

    void mPostSectionPanel::newSectionPanel(int caseid)
    {
        QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
        if (postDB == nullptr) return;
        _db = postDB;
		//创建切面节点
		MechCommon::mMechGlobalSignals::getInstance()->createSectionSig();

        // 新建切面图前，先把渲染器切换到当前激活的后处理视口，
        // 避免 clearPanel() 误删旧模型渲染器上的切面。
        _rWidget = getCVM->getPostRenderWidget();

        // 新建切面图时，清空旧数据，确保面板为空
        clearPanel();

        QPointer<PostData::mPostGuiData> guiData = new PostData::mPostGuiData();
        getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addSectionData(_curViewName, guiData);

        slot_positionChanged();
	}

    void mPostSectionPanel::clearPanel()
    {
        // 1. 清理渲染器中的切面（如果渲染器已就绪）
        if (_rWidget != nullptr)
        {
            auto postRender = _rWidget->getPostRender();
            if (postRender != nullptr)
            {
                // S5 G5：deleteCuttingPlane 逐槽位全删→台账家族摘除（removeObject 按条目
                // 承接渲染器槽位删除，摘空即全清）
                PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::CuttingPlane);
            }
        }

        // 2. 清空面板数据
        _clipsData.clear();
        _currentClip = nullptr;
        _planeCount = 0;
        _countPoints.clear();
        _planePointSelectPanel.setState(false);

        // 3. 清空表格
        _tableSection->blockSignals(true);
        _tableSection->clear();
        _tableSection->setRowCount(0);
        _tableSection->setHorizontalHeaderLabels({ "序号", "状态", "反向", "滚动条", "值", "删除" });
        _tableSection->blockSignals(false);

        // 4. 重置输入控件到默认状态
        _planeShapeBox->setCurrentIndex(0);
        _numEdit->setText("1");
        _numEdit->setStyleSheet("");
        _xEdit->setText("1");
        _yEdit->setText("0");
        _zEdit->setText("0");
        for (int i = 0; i <= AXIS::Z; ++i)
        {
            if (i < (int)_baseLineEdits.size())
                _baseLineEdits[i]->clear();
        }
        if (_baseLineEdits.size() > AXIS::END)
            _baseLineEdits[AXIS::END]->clear();
        _radioBasePoint->setChecked(true);
        _posType = 0;

        // 5. 重置拾取状态
        _basePointIsSelecting = false;
        _planePointIsSelecting = false;
        _countPointIsSelecting = false;
        disconnect(_planePointFinishPick);
        disconnect(_basePointFinishPick);
        disconnect(_countPointFinishPick);
        setupButtonSelectStyle(_planePointBtn, false, nullptr);
        setupButtonSelectStyle(_planePointSelectPanel.btnok, false, nullptr);
        setupButtonSelectStyle(_basePointBtn, false, nullptr);

        _isSectionVisible = true;
    }

    void mPostSectionPanel::syncClipsToGuiData()
    {
        if (_isLoadingView)
            return;
        QPointer<PostData::mPostGuiData> guiData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getSectionData(_curViewName);
        if (guiData == nullptr)
            return;
        guiData->_hasSectionView = !_clipsData.empty();
        guiData->_sectionClipPanels = _clipsData;
    }

    void mPostSectionPanel::syncPickedNodesToGuiData()
    {
        if (_isLoadingView)
            return;
        QPointer<PostData::mPostGuiData> guiData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getSectionData(_curViewName);
        if (guiData == nullptr)
            return;
        guiData->_sectionPickedNodeIDs.clear();
        for (const auto& p : _countPoints)
            guiData->_sectionPickedNodeIDs.push_back(p.data_id.node_id);
        guiData->_sectionHasPickedNodes = _planePointSelectPanel._picked;
    }

    void mPostSectionPanel::restorePickedNodesFromGuiData()
    {
        QPointer<PostData::mPostGuiData> guiData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getSectionData(_curViewName);
        if (guiData == nullptr)
            return;
        if (!guiData->_sectionHasPickedNodes || guiData->_sectionPickedNodeIDs.empty())
            return;

        _countPoints.clear();
        for (int nodeId : guiData->_sectionPickedNodeIDs)
        {
            mxr::MeshSelectInfoData info;
            info.dim = 0;
            info.data_id.node_id = nodeId;
            info.part_id = 0;
            info.data_index.face_index = -1;
            info.depth = 0.0f;
            _countPoints.push_back(info);
        }

        _planePointSelectPanel.setState(true);

        // 恢复3D视图高亮
        if (_rWidget != nullptr)
        {
            MechPreData::ClearPostWidgetMeshPick(_rWidget);
            _rWidget->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(
                mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshNode, _countPoints);
            MechPreData::UpdatePostWidgetHighlight(_rWidget);
        }
    }

    void mPostSectionPanel::updateContextByViewName(const QString& viewName)
    {
        // 根据 viewName 重新定位当前应操作的渲染窗口和后处理数据库，
        // 避免在多后处理模型之间切换时仍持有旧模型的 _rWidget/_db。
        mxr::REBUILD_PostRend* rend = getCVM->getPostRenderByViewName(viewName);
        if (rend == nullptr)
            rend = getCVM->getPostRenderWidget();
        _rWidget = rend;

        if (_rWidget != nullptr)
        {
            QString postName = _rWidget->getPostDataName();
            if (!postName.isEmpty())
                _db = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        }
    }

    void mPostSectionPanel::setCurViewName(const QString& viewName)
    {
        _curViewName = viewName;

        // 先根据当前 viewName 重新定位渲染器和数据库，再进行显隐/重建操作
        updateContextByViewName(viewName);

        // 如果面板当前不可见，需要确保切面也是隐藏状态
        if (!isVisible())
        {
            setAllClipingPlaneVisible(false);
            return;
        }

        if (_rWidget == nullptr)
            return;
        auto postRender = _rWidget->getPostRender();
        if (postRender == nullptr)
            return;

        // 加载该视口对应的切面数据
        QPointer<PostData::mPostGuiData> guiData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getSectionData(_curViewName);
        if (guiData == nullptr)
        {
            // 无持久化数据：清空面板即可
            clearPanel();
            return;
        }

        _isLoadingView = true;
        clearPanel();

        // 重建表格与切片数据
        _clipsData = guiData->_sectionClipPanels;
        _planeCount = 0;
        for (const auto& clip : _clipsData)
            _planeCount += clip._panelCount;

        _tableSection->blockSignals(true);
        int rowCount = (int)_clipsData.size();
        _tableSection->setRowCount(rowCount);
        for (int row = 0; row < rowCount; ++row)
        {
            _tableSection->setItem(row, 0, createNumItem(row));
            _tableSection->setCellWidget(row, 1, createStateWidget(_clipsData[row]._isActive, row));
            _tableSection->setCellWidget(row, 2, createReverseWidget(row));
            _tableSection->setCellWidget(row, 3, createSliderWidget(row));
            _tableSection->setCellWidget(row, 4, createValueWidget(row));
            _tableSection->setCellWidget(row, 5, createDeleteWidget(row));
            _tableSection->setRowHeight(row, 40);

            // 恢复滚动条位置和值显示（默认居中=0偏移）
            QWidget* sliderContainer = _tableSection->cellWidget(row, 3);
            if (sliderContainer != nullptr)
            {
                QSlider* slider = sliderContainer->findChild<QSlider*>();
                if (slider != nullptr)
                    slider->setValue(offsetToSliderValue(_clipsData[row], _clipsData[row]._sliderOffset));
            }
            QWidget* valueContainer = _tableSection->cellWidget(row, 4);
            if (valueContainer != nullptr)
            {
                QLabel* valueLabel = valueContainer->findChild<QLabel*>();
                if (valueLabel != nullptr)
                    valueLabel->setText(MxFunctions::mNumberToString(_clipsData[row]._sliderOffset, 'f', 3));
            }
        }
        _tableSection->blockSignals(false);

        // 重建渲染
        int planeIndex = 0;
        for (const auto& clip : _clipsData)
        {
            mxr::AABB aabb = postRender->getCurrentModelData();   // S5：下钻链消亡，getCurrentModelData 同源承载（绑定实例模型 AABB）
            float maxx = aabb.maxEdge[0];
            float maxy = aabb.maxEdge[1];
            float maxz = aabb.maxEdge[2];
            float minx = aabb.minEdge[0];
            float miny = aabb.minEdge[1];
            float minz = aabb.minEdge[2];

            QVector3D pointvec;
            pointvec.setX((maxx - minx) / (clip._panelCount + 1));
            pointvec.setY((maxy - miny) / (clip._panelCount + 1));
            pointvec.setZ((maxz - minz) / (clip._panelCount + 1));

            for (int nId = 0; nId < clip._panelCount; nId++)
            {
                QVector3D center;
                float radius;
                PostCommon::mCommonFunctions::caculateModelCenterAndRaduis(
                    minx, miny, minz, maxx, maxy, maxz, center, radius);

                switch ((POS_TYPE)clip._positionType)
                {
                case POS_TYPE::NODE:
                {
                    const auto& v = _db->getModelData(1)->getVectorById(clip._countNodeIds[nId]);
                    if (clip._panelType >= (int)PLANE_TYPE::NORMAL)
                        center = v;
                    else
                        center[(clip._panelType + 2) % 3] = v[(clip._panelType + 2) % 3];
                    break;
                }
                case POS_TYPE::DISTANCE:
                {
                    QVector3D min(minx, miny, minz);
                    center[(clip._panelType + 2) % 3] = min[(clip._panelType + 2) % 3] + pointvec[(clip._panelType + 2) % 3] * (nId + 1);
                    break;
                }
                case POS_TYPE::OFFSET:
                {
                    QVector3D min = clip._offset;
                    center[(clip._panelType + 2) % 3] = min[(clip._panelType + 2) % 3] + clip._distance * (nId + 1);
                    break;
                }
                default:
                    break;
                }

                // 叠加滚动条偏移量（统一沿当前法向）
                {
                    QVector3D n = clip._normal;
                    if (n.lengthSquared() > 1e-12f)
                        center += n.normalized() * clip._sliderOffset;
                }

                // S5 G5（D9 载体）：addObject(CuttingPlaneParams) 建面返回 id（externalIndex=
                // 建面时槽位），updateObject 掩码门控落位 normal/vertex/maxR（旧 create+
                // setPlaneData 语义承接）
                mxr::CuttingPlaneParams planeCreate;
                planeCreate.normal = clip._normal;
                planeCreate.vertex = center;
                const int planeId = postRender->addObject(planeCreate);
                if (planeId < 0)   // 建面失败（无就绪实例等）——槽位未占用，不推进 planeIndex
                    continue;
                mxr::CuttingPlaneParams planeData;
                planeData.normal = clip._normal;
                planeData.vertex = center;
                planeData.maxR = radius;
                planeData.updateMask = mxr::CuttingPlaneParams::Mask_Normal | mxr::CuttingPlaneParams::Mask_Vertex
                                     | mxr::CuttingPlaneParams::Mask_MaxR;
                postRender->updateObject(planeId, planeData);
                // 恢复该平面灰色切片（透明面）的显隐状态
                mxr::CuttingPlaneParams planeShow;
                planeShow.isShow = clip._isActive;
                planeShow.updateMask = mxr::CuttingPlaneParams::Mask_IsShow;
                postRender->updateObject(planeId, planeShow);   // S5 G5：setIsShowCuttingPlane→updateObject(IsShow 掩码)
                planeIndex++;
            }
        }

        // 恢复之前保存的拾取点高亮（基于点模式下）
        restorePickedNodesFromGuiData();

        // 默认选中最后一行，并把该切片的参数同步到面板控件
        if (_tableSection->rowCount() > 0)
        {
            _tableSection->selectRow(_tableSection->rowCount() - 1);
            _currentClip = &_clipsData[_clipsData.size() - 1];
            updatePanelControlsFromClip(*_currentClip);
        }

        // 恢复用户为该视口最后选择的平面类型（视角偏好），不强制重置为最后一个切片的类型
        if (guiData != nullptr)
        {
            _planeShapeBox->blockSignals(true);
            _planeShapeBox->setCurrentIndex(guiData->_sectionPlaneType);
            _planeShapeBox->blockSignals(false);
        }

        _isLoadingView = false;
        _rWidget->updatePaint();
    }

    QString mPostSectionPanel::getCloudViewName(QString view)
    {
        //源云图定位：优先管理器关系（SectionInfo.viewName → resultId → CloudItem.viewName）
        //——显示名接管后视口名不再携带 "_Section<N>" 后缀，名字解析只作兜底
        //（派生视口创建信号先于 addItem 登记的窗口期，彼时视口名仍带后缀）
        const QString byInfo = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post)
            .getCloudViewNameByView(PostData::ItemType::SectionItem, view);
        if (!byInfo.isEmpty())
            return byInfo;
        //剥 "_Section%1" 后缀反查源云图：正则须与命名协议一致（带下划线）——
        //批次A修正视口命名漏下划线时此处未同步，贪婪匹配剥出尾缀下划线
        //导致反查源云图落空（切面继承链断裂，2026-10-13 审查发现补修）
        QRegularExpression re("^(.*)(_Section\\d+)$");
        QRegularExpressionMatch match = re.match(view);
        if (match.hasMatch()) {
            return match.captured(1);
        }
        return view;
    }

    bool mPostSectionPanel::relocateRendWidget()
    {
        // 面板常驻(单例)而渲染视口会随云图等节点删除一并销毁，_rWidget 可能悬空。
        // 先按 _curViewName 重定位到现存视口；定位失败时，若 _rWidget 已不在
        // 视口注册表中(原视口已被删除)则置空，避免后续解引用已释放的渲染器。
        if (!_curViewName.isEmpty())
        {
            mxr::REBUILD_PostRend* rend = getCVM->getPostRenderByViewName(_curViewName);
            if (rend != nullptr)
            {
                _rWidget = rend;
                return true;
            }
        }
        if (_rWidget != nullptr && getCVM->getSubWindowFromRend(_rWidget) == nullptr)
            _rWidget = nullptr;
        return _rWidget != nullptr;
    }

    void mPostSectionPanel::setAllClipingPlaneVisible(bool visible)
    {
        // 渲染器可能已随原视口删除(如删除云图节点后重建)，先校验再解引用
        if (_rWidget != nullptr && getCVM->getSubWindowFromRend(_rWidget) == nullptr)
            _rWidget = nullptr;
        if (_rWidget == nullptr) return;
        auto postRender = _rWidget->getPostRender();
        if (postRender == nullptr) return;

        // 隐藏/显示切面：采用"删除再重建"策略彻底移除/恢复GL_CLIP_DISTANCE的裁剪效果
        // 1. 先删除所有现有切面（无论显示还是隐藏都需要先清空）
        int totalPlanes = 0;
        for (int i = 0; i < _clipsData.size(); i++)
        {
            totalPlanes += _clipsData[i]._panelCount;
        }
        // S5 G5：从后往前逐槽位删除→台账家族摘除（removeObject 按条目承接槽位删除）
        PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::CuttingPlane);

        // 2. 如果是显示切面，重新创建所有切面
        if (visible)
        {
            int planeIndex = 0;
            for (int i = 0; i < _clipsData.size(); i++)
            {
                auto& clip = _clipsData[i];
                // 计算模型包围盒
                mxr::AABB aabb = postRender->getCurrentModelData();   // S5：下钻链消亡，getCurrentModelData 同源承载（绑定实例模型 AABB）
                float maxx = aabb.maxEdge[0];
                float maxy = aabb.maxEdge[1];
                float maxz = aabb.maxEdge[2];
                float minx = aabb.minEdge[0];
                float miny = aabb.minEdge[1];
                float minz = aabb.minEdge[2];

                QVector3D pointvec;
                pointvec.setX((maxx - minx) / (clip._panelCount + 1));
                pointvec.setY((maxy - miny) / (clip._panelCount + 1));
                pointvec.setZ((maxz - minz) / (clip._panelCount + 1));

                for (int nId = 0; nId < clip._panelCount; nId++)
                {
                    QVector3D center;
                    float radius;
                    PostCommon::mCommonFunctions::caculateModelCenterAndRaduis(
                        minx, miny, minz, maxx, maxy, maxz, center, radius);

                    switch ((POS_TYPE)clip._positionType)
                    {
                    case POS_TYPE::NODE:
                    {
                        const auto& v = _db->getModelData(1)->getVectorById(clip._countNodeIds[nId]);
                        if (clip._panelType >= (int)PLANE_TYPE::NORMAL)
                        {
                            center = v;
                        }
                        else
                        {
                            center[(clip._panelType + 2) % 3] = v[(clip._panelType + 2) % 3];
                        }
                        break;
                    }
                    case POS_TYPE::DISTANCE:
                    {
                        QVector3D min(minx, miny, minz);
                        center[(clip._panelType + 2) % 3] = min[(clip._panelType + 2) % 3] + pointvec[(clip._panelType + 2) % 3] * (nId + 1);
                        break;
                    }
                    case POS_TYPE::OFFSET:
                    {
                        QVector3D min = clip._offset;
                        center[(clip._panelType + 2) % 3] = min[(clip._panelType + 2) % 3] + clip._distance * (nId + 1);
                        break;
                    }
                    default:
                        break;
                    }

                    // 叠加滚动条偏移量（统一沿当前法向）
                    {
                        QVector3D n = clip._normal;
                        if (n.lengthSquared() > 1e-12f)
                            center += n.normalized() * clip._sliderOffset;
                    }

                    // S5 G5（D9 载体）：addObject 建面返回 id + updateObject 掩码落位
                    //（旧 create+setPlaneData 语义承接）
                    mxr::CuttingPlaneParams planeCreate;
                    planeCreate.normal = clip._normal;
                    planeCreate.vertex = center;
                    const int planeId = postRender->addObject(planeCreate);
                    if (planeId < 0)   // 建面失败（无就绪实例等）——槽位未占用，不推进 planeIndex
                        continue;
                    mxr::CuttingPlaneParams planeData;
                    planeData.normal = clip._normal;
                    planeData.vertex = center;
                    planeData.maxR = radius;
                    planeData.updateMask = mxr::CuttingPlaneParams::Mask_Normal | mxr::CuttingPlaneParams::Mask_Vertex
                                         | mxr::CuttingPlaneParams::Mask_MaxR;
                    postRender->updateObject(planeId, planeData);
                    planeIndex++;
                }
            }
        }

        // 通过setIsShowCuttingPlane控制透明平面的显隐
        int planeIndex = 0;
        for (int i = 0; i < _clipsData.size(); i++)
        {
            for (int nId = 0; nId < _clipsData[i]._panelCount; nId++)
            {
                mxr::CuttingPlaneParams planeShow;
                planeShow.isShow = _clipsData[i]._isActive;
                planeShow.updateMask = mxr::CuttingPlaneParams::Mask_IsShow;
                postRender->updateObject(PostCommon::cuttingPlaneIdByIndex(postRender.get(), planeIndex), planeShow);   // S5 G5：setIsShowCuttingPlane→updateObject(IsShow 掩码，按槽位寻台账 id)
                planeIndex++;
            }
        }

        _isSectionVisible = visible;
        _rWidget->updatePaint();
    }

    void mPostSectionPanel::slot_checkBox(bool istrue)
    {
        //获取信号发射源
        QCheckBox* cb = qobject_cast<QCheckBox*>(sender());
        if (cb == nullptr)
        {
            return;
        }
        // 遍历每行表格
        for (int row = 0; row < _tableSection->rowCount(); ++row)
        {
            QWidget* container = qobject_cast<QWidget*>(_tableSection->cellWidget(row, 1));

            if (container != nullptr)
            {
                if (cb == container->findChild<QCheckBox*>())
                {
                    if (row >= 0 && row < (int)_clipsData.size())
                    {
                        _clipsData[row]._isActive = istrue;
                        syncClipsToGuiData();

                        // 根据激活状态控制该组灰色切片（透明面）的显隐：
                        // 一行对应渲染器中连续的一组平面（等间距/偏移时含多个），
                        // 需先按前序行的平面总数定位该组起始索引，再逐面设置，
                        // 不能只对第 row 个平面设置
                        if (_rWidget != nullptr)
                        {
                            auto postRender = _rWidget->getPostRender();
                            if (postRender != nullptr)
                            {
                                int planeStart = 0;
                                for (int i = 0; i < row; i++)
                                    planeStart += _clipsData[i]._panelCount;
                                for (int n = 0; n < _clipsData[row]._panelCount; n++)
                                {
                                    mxr::CuttingPlaneParams planeShow;
                                    planeShow.isShow = istrue;
                                    planeShow.updateMask = mxr::CuttingPlaneParams::Mask_IsShow;
                                    postRender->updateObject(PostCommon::cuttingPlaneIdByIndex(postRender.get(), planeStart + n), planeShow);   // S5 G5：setIsShowCuttingPlane→updateObject(IsShow 掩码，按槽位寻台账 id)
                                }
                            }
                            _rWidget->updatePaint();
                        }
                    }
                    break;
                }
            }
        }
    }

    void mPostSectionPanel::slot_reverseButton()
    {
        // 获取信号发射源
        QToolButton* tb = qobject_cast<QToolButton*>(sender());
        if (tb == nullptr)
        {
            return;
        }

        // 遍历每行表格
        for (int row = 0; row < _tableSection->rowCount(); ++row)
        {
            QWidget* container = qobject_cast<QWidget*>(_tableSection->cellWidget(row, 2));

            if (container != nullptr)
            {
                if (tb == container->findChild<QToolButton*>())
                {
                    if (row >= 0 && row < (int)_clipsData.size())
                    {
                        _clipsData[row]._isReserve = !_clipsData[row]._isReserve;
                        _clipsData[row]._normal = -_clipsData[row]._normal;

                        // 反向时偏移量同步取反，保证滚动条位置与视觉方向一致
                        _clipsData[row]._sliderOffset = -_clipsData[row]._sliderOffset;

                        // 更新滚动条和值显示
                        QWidget* sliderContainer = _tableSection->cellWidget(row, 3);
                        if (sliderContainer != nullptr)
                        {
                            QSlider* slider = sliderContainer->findChild<QSlider*>();
                            if (slider != nullptr)
                                slider->setValue(offsetToSliderValue(_clipsData[row], _clipsData[row]._sliderOffset));
                        }
                        QWidget* valueContainer = _tableSection->cellWidget(row, 4);
                        if (valueContainer != nullptr)
                        {
                            QLabel* valueLabel = valueContainer->findChild<QLabel*>();
                            if (valueLabel != nullptr)
                                valueLabel->setText(MxFunctions::mNumberToString(_clipsData[row]._sliderOffset, 'f', 3));
                        }

                        // 更新该组切面的法向
                        if (_rWidget != nullptr)
                        {
                            auto postRender = _rWidget->getPostRender();
                            if (postRender != nullptr)
                            {
                                UpdateClipingPlane(row, _clipsData[row]);
                            }
                        }
                        syncClipsToGuiData();
                    }
                    break;
                }
            }
        }
    }

    void mPostSectionPanel::slot_deleteButton()
    {
        // 获取信号发射源
        QToolButton* tb = qobject_cast<QToolButton*>(sender());
        if (tb == nullptr)
        {
            return;
        }

        // 遍历每行表格
        for (int row = 0; row < _tableSection->rowCount(); ++row)
        {
            QWidget* container = qobject_cast<QWidget*>(_tableSection->cellWidget(row, 5));

            if (container != nullptr)
            {
                if (tb == container->findChild<QToolButton*>())
                {
                    DeleteClipingPlane(row);
                    _tableSection->removeRow(row);

                    // 更新剩余行的序号
                    for (int i = row; i < _tableSection->rowCount(); ++i)
                    {
                        QTableWidgetItem* numItem = _tableSection->item(i, 0);
                        if (numItem != nullptr)
                            numItem->setText(MxFunctions::mNumberToString(i + 1));
                    }

                    syncClipsToGuiData();
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("已删除第 %1 个切面。").arg(row + 1));
                    break;
                }
            }
        }
    }

    QTableWidgetItem* mPostSectionPanel::createNumItem(int row) const
    {
        QTableWidgetItem* num = new QTableWidgetItem(MxFunctions::mNumberToString(row + 1));
        num->setTextAlignment(Qt::AlignCenter);
        num->setFlags(num->flags() & ~Qt::ItemIsEditable);
        return num;
    }

    QWidget* mPostSectionPanel::createStateWidget(bool checked, int /*row*/) const
    {
        QCheckBox* state = new QCheckBox();
        state->setChecked(checked);
        connect(state, SIGNAL(toggled(bool)), this, SLOT(slot_checkBox(bool)));

        QWidget* container_state = new QWidget();
        QHBoxLayout* layout_state = new QHBoxLayout(container_state);
        layout_state->addWidget(state);
        layout_state->setAlignment(Qt::AlignCenter);
        layout_state->setContentsMargins(0, 0, 0, 0);
        return container_state;
    }

    QWidget* mPostSectionPanel::createReverseWidget(int /*row*/) const
    {
        QToolButton* reverseBtn = new QToolButton();
        reverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
        reverseBtn->setIconSize(QSize(20, 20));
        connect(reverseBtn, SIGNAL(clicked()), this, SLOT(slot_reverseButton()));

        QWidget* container_reverse = new QWidget();
        QHBoxLayout* layout_reverse = new QHBoxLayout(container_reverse);
        layout_reverse->addWidget(reverseBtn);
        layout_reverse->setAlignment(Qt::AlignCenter);
        layout_reverse->setContentsMargins(0, 0, 0, 0);
        return container_reverse;
    }

    QWidget* mPostSectionPanel::createSliderWidget(int row) const
    {
        QSlider* slider = new QSlider(Qt::Horizontal);
        slider->setObjectName(QString("slider_%1").arg(row));
        // 范围 [-1, 1]，默认 0 表示不偏移；初始加载时会根据 _sliderOffset 重设
        slider->setMinimum(-1000);
        slider->setMaximum(1000);
        slider->setSingleStep(1);
        slider->setPageStep(10);
        slider->setValue(0);
        connect(slider, &QSlider::valueChanged, this, &mPostSectionPanel::slot_sliderValueChanged);

        QWidget* container_slider = new QWidget();
        QHBoxLayout* layout_slider = new QHBoxLayout(container_slider);
        layout_slider->addWidget(slider);
        layout_slider->setAlignment(Qt::AlignCenter);
        layout_slider->setContentsMargins(0, 0, 0, 0);
        return container_slider;
    }

    QWidget* mPostSectionPanel::createValueWidget(int /*row*/) const
    {
        QLabel* valueLabel = new QLabel("0.000");
        valueLabel->setAlignment(Qt::AlignCenter);

        QWidget* container_value = new QWidget();
        QHBoxLayout* layout_value = new QHBoxLayout(container_value);
        layout_value->addWidget(valueLabel);
        layout_value->setAlignment(Qt::AlignCenter);
        layout_value->setContentsMargins(0, 0, 0, 0);
        return container_value;
    }

    QWidget* mPostSectionPanel::createDeleteWidget(int /*row*/) const
    {
        QToolButton* deleteBtn = new QToolButton();
        deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"));
        deleteBtn->setIconSize(QSize(20, 20));
        connect(deleteBtn, SIGNAL(clicked()), this, SLOT(slot_deleteButton()));

        QWidget* container_delete = new QWidget();
        QHBoxLayout* layout_delete = new QHBoxLayout(container_delete);
        layout_delete->addWidget(deleteBtn);
        layout_delete->setAlignment(Qt::AlignCenter);
        layout_delete->setContentsMargins(0, 0, 0, 0);
        return container_delete;
    }
    /** 获取切面组初始位置（未加 slider 偏移前）在法向上的投影值 */
    float mPostSectionPanel::calcInitialPlanePosition(const PostClipingPanel& clip) const
{
        if (_rWidget == nullptr)
            return 0.0f;
        auto postRender = _rWidget->getPostRender();
        if (postRender == nullptr)
            return 0.0f;

        mxr::AABB aabb = postRender->getCurrentModelData();   // S5：下钻链消亡，getCurrentModelData 同源承载（绑定实例模型 AABB）
        QVector3D minCorner(aabb.minEdge[0], aabb.minEdge[1], aabb.minEdge[2]);
        QVector3D maxCorner(aabb.maxEdge[0], aabb.maxEdge[1], aabb.maxEdge[2]);

        // 基于点：取第一个拾取节点在法向上的投影
        if (clip._positionType == (int)POS_TYPE::NODE && !clip._countNodeIds.empty())
        {
            QPointer<PostData::mPostModelData> modeldata;
            if (_db != nullptr)
                modeldata = _db->getModelData(1);
            if (modeldata != nullptr)
            {
                QVector3D v = modeldata->getVectorById(clip._countNodeIds[0]);
                if (clip._panelType >= (int)PLANE_TYPE::NORMAL)
                {
                    QVector3D n = clip._normal;
                    if (n.lengthSquared() > 1e-12f)
                    {
                        n.normalize();
                        return QVector3D::dotProduct(v, n);
                    }
                    return v.z();
                }
                return v[(clip._panelType + 2) % 3];
            }
        }

        // 等间距：取第一个切片所在位置
        if (clip._positionType == (int)POS_TYPE::DISTANCE)
        {
            int axis = (clip._panelType + 2) % 3;
            float pointvec = (maxCorner[axis] - minCorner[axis]) / (clip._panelCount + 1);
            return minCorner[axis] + pointvec;
        }

        // 偏移：取第一个偏移位置
        if (clip._positionType == (int)POS_TYPE::OFFSET)
        {
            int axis = (clip._panelType + 2) % 3;
            return clip._offset[axis] + clip._distance;
        }

        // 默认回退到 AABB 中点
        if (clip._panelType >= (int)PLANE_TYPE::NORMAL)
        {
            QVector3D n = clip._normal;
            if (n.lengthSquared() < 1e-12f)
                n = QVector3D(0, 0, 1);
            else
                n.normalize();
            return QVector3D::dotProduct((minCorner + maxCorner) * 0.5f, n);
        }
        return (minCorner[(clip._panelType + 2) % 3] + maxCorner[(clip._panelType + 2) % 3]) * 0.5f;
    }

    QPair<float, float> mPostSectionPanel::calcSliderRange(const PostClipingPanel& clip) const
    {
        if (_rWidget == nullptr)
            return qMakePair(0.0f, 0.0f);
        auto postRender = _rWidget->getPostRender();
        if (postRender == nullptr)
            return qMakePair(0.0f, 0.0f);

        mxr::AABB aabb = postRender->getCurrentModelData();   // S5：下钻链消亡，getCurrentModelData 同源承载（绑定实例模型 AABB）
        QVector3D minCorner(aabb.minEdge[0], aabb.minEdge[1], aabb.minEdge[2]);
        QVector3D maxCorner(aabb.maxEdge[0], aabb.maxEdge[1], aabb.maxEdge[2]);

        float minProj = 0.0f, maxProj = 0.0f;
        if (clip._panelType >= (int)PLANE_TYPE::NORMAL)
        {
            QVector3D n = clip._normal;
            if (n.lengthSquared() < 1e-12f)
                n = QVector3D(0, 0, 1);
            else
                n.normalize();

            minProj = std::numeric_limits<float>::max();
            maxProj = std::numeric_limits<float>::lowest();
            for (int ix = 0; ix <= 1; ++ix)
                for (int iy = 0; iy <= 1; ++iy)
                    for (int iz = 0; iz <= 1; ++iz)
                    {
                        QVector3D corner(
                            ix ? maxCorner.x() : minCorner.x(),
                            iy ? maxCorner.y() : minCorner.y(),
                            iz ? maxCorner.z() : minCorner.z());
                        float proj = QVector3D::dotProduct(corner, n);
                        minProj = std::min(minProj, proj);
                        maxProj = std::max(maxProj, proj);
                    }
        }
        else
        {
            int axis = (clip._panelType + 2) % 3;
            minProj = minCorner[axis];
            maxProj = maxCorner[axis];
        }

        // 以初始位置为原点，范围取到 AABB 两端的最大可调距离，
        // 使滑块能够从模型一端移动到另一端，而不是只对称覆盖初始位置两侧的最小距离。
        float initPos = calcInitialPlanePosition(clip);
        float rangeMin = minProj - initPos;
        float rangeMax = maxProj - initPos;
        // 保证最小也有一点调节空间
        if (std::abs(rangeMax - rangeMin) < 1e-6f)
        {
            rangeMin = -1e-3f;
            rangeMax = 1e-3f;
        }
        return qMakePair(rangeMin, rangeMax);
    }

    float mPostSectionPanel::sliderValueToOffset(const PostClipingPanel& clip, int value) const
    {
        QPair<float, float> range = calcSliderRange(clip);
        float t = value / 1000.0f;
        // slider 范围 [-1000, 1000]，线性映射到 [range.first, range.second]
        float center = (range.first + range.second) * 0.5f;
        float half = (range.second - range.first) * 0.5f;
        return center + t * half;
    }

    int mPostSectionPanel::offsetToSliderValue(const PostClipingPanel& clip, float offset) const
    {
        QPair<float, float> range = calcSliderRange(clip);
        if (std::abs(range.second - range.first) < 1e-9f)
            return 0;
        float center = (range.first + range.second) * 0.5f;
        float half = (range.second - range.first) * 0.5f;
        if (std::abs(half) < 1e-9f)
            return 0;
        float t = (offset - center) / half;
        t = std::max(-1.0f, std::min(1.0f, t));
        return qRound(t * 1000.0f);
    }

    void mPostSectionPanel::slot_newSection()
    {
        int rowCount = _tableSection->rowCount();
        _tableSection->insertRow(rowCount);
        _tableSection->setRowHeight(rowCount, 40);

        // 序号
        _tableSection->setItem(rowCount, 0, createNumItem(rowCount));

        // 状态
        _tableSection->setCellWidget(rowCount, 1, createStateWidget(true, rowCount));

        // 反向
        _tableSection->setCellWidget(rowCount, 2, createReverseWidget(rowCount));

        // 滚动条
        _tableSection->setCellWidget(rowCount, 3, createSliderWidget(rowCount));

        // 值
        _tableSection->setCellWidget(rowCount, 4, createValueWidget(rowCount));

        // 删除
        _tableSection->setCellWidget(rowCount, 5, createDeleteWidget(rowCount));

        // 新建切片数据并保存
        const auto& clip = CreateClipingPlane();
        // 更新切片数据
        // UpdateAllClipingPlane();
        UpdateClipingPlane(rowCount, clip);

        // 同步到 per-view 数据
        syncClipsToGuiData();
        syncPickedNodesToGuiData();

        // 如果当前面板处于隐藏状态，需要同步隐藏新切面
        if (!_isSectionVisible)
        {
            setAllClipingPlaneVisible(false);
        }

        // 输出切面信息
        static const QStringList planeNames = { "XY平面", "YZ平面", "ZX平面", "任意平面" };
        static const QStringList posNames = { "基于点", "等间距", "偏移" };
        const QString& planeName = planeNames.value(clip._panelType, "未知");
        const QString& posName = posNames.value(clip._positionType, "未知");
        QString detail = QString("平面类型：%1，位置方式：%2，数量：%3，法向：(%4, %5, %6)")
            .arg(planeName).arg(posName).arg(clip._panelCount)
            .arg(clip._normal.x()).arg(clip._normal.y()).arg(clip._normal.z());
        mGlobalSignals::getInstance()->outputMessageSig(0, QString("切面新建成功！%1").arg(detail));
    }

    void mPostSectionPanel::slot_banAll()
    {
        // 遍历所有行
        for (int row = 0; row < _tableSection->rowCount(); ++row)
        {
            QWidget* container = qobject_cast<QWidget*>(_tableSection->cellWidget(row, 1));

            if (container != nullptr)
            {
                QCheckBox* checkbox = container->findChild<QCheckBox*>();
                checkbox->setChecked(false);
            }
        }
    }

    void PostWindow::mPostSectionPanel::slot_delAll()
    {
        for (int row = 0; row < _tableSection->rowCount(); row++)
        {
            DeleteClipingPlane(0);      // 这个函数内部对vector进行了去除操作，每次只需要删除第0个即可。
        }
        _currentClip = nullptr;
        _tableSection->clear();
        _tableSection->setRowCount(0);
        _tableSection->setHorizontalHeaderLabels({ "序号", "状态", "反向", "滚动条", "值", "删除" });

        syncClipsToGuiData();

        mGlobalSignals::getInstance()->outputMessageSig(0, QString("已删除全部切面。"));
    }

    void mPostSectionPanel::slot_planeShapeChanged()
    {
        // 平面类型只作为视角调整和“下一次新建切面”的偏好，
        // 不修改任何已生成的切面。
        if (_rWidget == nullptr)
            return;

        // 同步当前选择到 per-view 数据，切回该视口时可恢复
        if (!_isLoadingView)
        {
            QPointer<PostData::mPostGuiData> guiData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getSectionData(_curViewName);
            if (guiData != nullptr)
            {
                guiData->_sectionPlaneType = _planeShapeBox->currentIndex();
            }
        }

        // 切换视角到对应方向
        PLANE_TYPE planeType = (PLANE_TYPE)_planeShapeBox->currentIndex();
        switch (planeType)
        {
        case PLANE_TYPE::XY:
            _rWidget->getCameraController()->setView(mxr::Perspective::Right);
            break;
        case PLANE_TYPE::YZ:
            _rWidget->getCameraController()->setView(mxr::Perspective::Top);
            break;
        case PLANE_TYPE::ZX:
            _rWidget->getCameraController()->setView(mxr::Perspective::Front);
            break;
        default:
            break;
        }
        _rWidget->updatePaint();
    }

    void mPostSectionPanel::slot_sliderValueChanged(int value)
    {
        QSlider* slider = qobject_cast<QSlider*>(sender());
        if (slider == nullptr)
            return;

        // 通过 objectName 定位行号
        int row = -1;
        bool ok = false;
        QString name = slider->objectName();
        if (name.startsWith("slider_"))
            row = name.mid(7).toInt(&ok);
        if (!ok || row < 0 || row >= _tableSection->rowCount())
            return;

        // 更新数据
        if (row >= (int)_clipsData.size())
            return;
        float offset = sliderValueToOffset(_clipsData[row], value);
        _clipsData[row]._sliderOffset = offset;
        // 更新值显示
        QWidget* valueContainer = _tableSection->cellWidget(row, 4);
        if (valueContainer != nullptr)
        {
            QLabel* valueLabel = valueContainer->findChild<QLabel*>();
            if (valueLabel != nullptr)
                valueLabel->setText(MxFunctions::mNumberToString(offset, 'f', 3));
        }

        // 实时刷新该组切面
        if (_rWidget != nullptr)
        {
            auto postRender = _rWidget->getPostRender();
            if (postRender != nullptr)
            {
                UpdateClipingPlane(row, _clipsData[row]);
            }
        }

        syncClipsToGuiData();
    }

    void mPostSectionPanel::slot_positionChanged()
    {
        bool basePoint = false;
        if (_radioBasePoint->isChecked())
        {
            _pointContainer->show();
            _baseContainer->hide();
            _distanceContainer->hide();
            basePoint = true;
            _posType = 0;
        }
        else if (_radioEqualSpacing->isChecked())
        {
            _pointContainer->hide();
            _baseContainer->hide();
            _distanceContainer->hide();
            _posType = 1;
        }
        else if (_radioOffset->isChecked())
        {
            _pointContainer->hide();
            _baseContainer->show();
            _distanceContainer->show();
            _posType = 2;
        }
        for (int i = 0; i <= AXIS::Z; i++)
        {
            if (basePoint)
            {
                _baseLineEdits[i]->disconnect();
                _basePointIsSelecting = false;
                _countPointIsSelecting = false;
                disconnect(_basePointFinishPick);
                disconnect(_countPointFinishPick);
                setupButtonSelectStyle(_basePointBtn, false, nullptr);
                setupButtonSelectStyle(_planePointSelectPanel.btnok, false, nullptr);

            }
        }

    }
    void mPostSectionPanel::paintEvent(QPaintEvent* e)
    {
        if (_rWidget)
        {
            int count = _numEdit->text().toInt();
            // 根据当前数据设置所有控件状态
            switch ((POS_TYPE)_posType)
            {
            case POS_TYPE::NODE:
            {
                // 只有在未确认选择或在拾取过程中，才用视口实时选择覆盖已保存的选择。
                // 否则用户点击“添加”确认后，一旦焦点切换导致视口高亮被清空，
                // paintEvent 实时读取到的选择数会变为 0，使后续逻辑错误。
                if (!_planePointSelectPanel._picked || _countPointIsSelecting)
                {
                    this->_countPoints = _rWidget->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
                }
                int selCount = this->_countPoints.size();
                // 仅当尚未确认选择时，才允许实时空选择重置“已选择”状态。
                if (selCount == 0 && !_planePointSelectPanel._picked)
                    _planePointSelectPanel.setState(false);

                // “新建”必须等用户点击“添加（+）”确认选择后才可用，
                // 不能仅因实时拾取数量匹配就启用。
                bool hasConfirmedSelection = _planePointSelectPanel._picked && (int)_countPoints.size() == count;
                // 加号按钮由点击槽内部判断数量并提示，这里只做视觉提示：
                // 拾取中且数量匹配时保持可用，否则也保持可用但点击会提示。
                _btnNew->setDisabled(!hasConfirmedSelection || count == 0 || (count + _planeCount > 8));
                break;
            }
            case POS_TYPE::DISTANCE:
                _btnNew->setDisabled(count == 0 || (count + _planeCount > 8));
                break;
            case POS_TYPE::OFFSET:
            {
                bool ok = true;
                for (int i = 0; i < _baseLineEdits.size(); i++)
                {
                    bool innerOk = false;
                    _baseLineEdits[i]->text().toFloat(&innerOk);
                    ok &= innerOk;
                }
                _btnNew->setDisabled(!ok || count == 0 || (count + _planeCount > 8));
                break;
            }
            default:
                break;
            }
            innerSlot_planeCountChanged(_numEdit->text());
        }
        QWidget::paintEvent(e);
    }
    void mPostSectionPanel::hideEvent(QHideEvent* event)
    {
        // 隐藏切面前，先确认当前 _curViewName 是否仍属于当前活动模型。
        // 若用户已切换到另一个后处理模型，当前模型找不到对应渲染器，
        // 此时不应再删除旧模型上的切面。
        if (relocateRendWidget())
            setAllClipingPlaneVisible(false);

        setupButtonSelectStyle(_planePointBtn, false, {});
        setupButtonSelectStyle(_planePointSelectPanel.btnok, false, {});
        setupButtonSelectStyle(_basePointBtn, false, {});
        _basePointIsSelecting = false;
        _planePointIsSelecting = false;
        _countPointIsSelecting = false;
        disconnect(_planePointFinishPick);
        disconnect(_basePointFinishPick);
        disconnect(_countPointFinishPick);
        mPanelBase::hideEvent(event);
    }

    void mPostSectionPanel::showEvent(QShowEvent* event)
    {
        // 恢复显示所有切面前，先按 _curViewName 重新定位渲染器，
        // 避免在多模型切换后错误地操作旧模型渲染器。
        // createSection 等流程先 show 后 newSectionPanel，此时 _rWidget 仍是
        // 上一轮绑定；若原视口已随节点删除，relocateRendWidget 会将其置空。
        if (relocateRendWidget())
            setAllClipingPlaneVisible(true);

        mPanelBase::showEvent(event);
    }
    void mPostSectionPanel::setupButtonSelectStyle(QWidget* btn, bool selecting, std::function<void()> selectingFunc)
    {
        // 改变外观
        const auto& glow = new QGraphicsDropShadowEffect();
        if (selecting)
        {
            glow->setColor(QRgb(0x5677fc));     // 淡蓝色
            glow->setBlurRadius(5);             // 羽化半径
            glow->setOffset(0, 0);
        }
        else
        {
            glow->setColor(QRgb(0xdcdcdc));     // 亮灰色，默认
            glow->setBlurRadius(0);
            glow->setOffset(0, 0);
        }
        btn->setGraphicsEffect(glow);

        if (selecting)
        {
            if (selectingFunc)
            {
                selectingFunc();
            }
        }

    }
    QVector3D mPostSectionPanel::GetPickSinglePoint()
    {
        const auto& nodes_itr = _rWidget->getSelectionCaculator()->getMeshSelectComputeData();
        if (nodes_itr.empty()) return { NAN,NAN,NAN };
        const auto& nodes = nodes_itr.at(mxr::SelectionInfoType::MeshNode);
        if (nodes.empty()) return { NAN,NAN,NAN };
        int nodeID = nodes[0].data_id.node_id;
        // 清空旧高亮，并添加当前数据
        MechPreData::ClearPostWidgetMeshPick(_rWidget);
        _rWidget->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshNode, { nodes[0] });
        MechPreData::UpdatePostWidgetHighlight(_rWidget);
        return _db->getModelData(_rWidget->getPostRender()->currentFrame())->getVectorById(nodeID);   // S5：getCurrentAnimationFrame→currentFrame
    }

    PostClipingPanel mPostSectionPanel::CreateClipingPlane()
    {
        PostClipingPanel clip;
        // 获取平面类型，切片数量，选中点

        clip._id = QUuid().createUuid().data1;
        clip._panelType = _planeShapeBox->currentIndex();
        if (clip._panelType <= (int)PLANE_TYPE::ZX)
            clip._normal[(clip._panelType + 2) % 3] = 1;
        else//矢量
        {
            clip._normal.setX(_xEdit->text().toFloat());
            clip._normal.setY(_yEdit->text().toFloat());
            clip._normal.setZ(_zEdit->text().toFloat());
        }
        clip._positionType = _posType;
        clip._panelCount = _numEdit->text().toInt();
        switch (POS_TYPE(_posType))
        {
        case PostWindow::POS_TYPE::NODE:
            for (int i = 0; i < _countPoints.size(); i++)
                clip._countNodeIds.push_back(_countPoints[i].data_id.node_id);
            break;
        case PostWindow::POS_TYPE::DISTANCE:
            break;
        case PostWindow::POS_TYPE::OFFSET:
            clip._offset = {
                _baseLineEdits[AXIS::X]->text().toFloat(),
                _baseLineEdits[AXIS::Y]->text().toFloat(),
                _baseLineEdits[AXIS::Z]->text().toFloat()
            };
            clip._distance = _baseLineEdits[AXIS::END]->text().toFloat();
            break;
        default:
            break;
        }

        _planeCount += clip._panelCount;
        _clipsData.push_back(clip);
        return clip;
    }

    void mPostSectionPanel::DeleteClipingPlane(int index)
    {
        auto  postRender = _rWidget->getPostRender();
        int i = 0;
        for (auto itr = _clipsData.begin(); itr != _clipsData.end(); itr++)
        {
            if (index != i)
            {
                i++;
                continue;
            }
            // 该组平面从"之前所有行的平面总数"处开始，连续占用 _panelCount 个槽位；
            // 删除planeStart后下一个平面会补位，重复删除同一索引即可清空整组
            int planeStart = 0;
            for (int j = 0; j < i; j++)
            {
                planeStart += _clipsData[j]._panelCount;
            }
            for (int nId = 0; nId < _clipsData[i]._panelCount; nId++)
            {
                // S5 G5：deleteCuttingPlane(planeStart) 同槽重复删→寻台账"外部索引≥槽位"
                // 首个条目摘除（渲染器删除后槽位前移，逐面清空整组语义承接）
                int planeId = -1;
                const auto records = postRender->listObjects();
                for (const auto& rec : records)
                {
                    if (rec.type == mxr::SceneObjectType::CuttingPlane && rec.externalIndex >= planeStart)
                    {
                        planeId = rec.id;
                        break;
                    }
                }
                if (planeId >= 0)
                    postRender->removeObject(planeId);
            }
            _planeCount -= _clipsData[i]._panelCount;
            // 删除当前
            _clipsData.erase(itr);
            break;
        }
        _rWidget->updatePaint();
    }

    void mPostSectionPanel::UpdateAllClipingPlane()
    {
        const auto& postRender = _rWidget->getPostRender();
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_rWidget);
        // 等距离时的包围盒步进长度
        QVector3D pointvec(0, 0, 0);
        // S5 门面直接替换：getOneFrameRender 下钻通道消亡——绑定就绪经
        // frameProxy(currentFrame()) 判定（F8：现取，不缓存）
        if (postRender->frameProxy(postRender->currentFrame()) == nullptr)
            return;
        QHash<int, QVector3D> dispdata = _db->getDisplacementDataByFrameID(guidata->_cloudFrame);

        float maxx, minx, maxy, miny, maxz, minz;
        mPostModelData* modeldata = _db->getModelData(guidata->_cloudFrame);
        if (modeldata == nullptr)
            return;
        mxr::AABB aabb = postRender->getCurrentModelData();   // S5：下钻链消亡，getCurrentModelData 同源承载（绑定实例模型 AABB）
        maxx = aabb.maxEdge[0];
        maxy = aabb.maxEdge[1];
        maxz = aabb.maxEdge[2];
        minx = aabb.minEdge[0];
        miny = aabb.minEdge[1];
        minz = aabb.minEdge[2];
        int planeCount = 0;     // 需要统计所有切面组里的切面数量，总量不能超过8个
        for (int i = 0; i < _clipsData.size(); i++)
        {
            auto& clip = _clipsData[i];
            pointvec.setX((maxx - minx) / (clip._panelCount + 1));
            pointvec.setY((maxy - miny) / (clip._panelCount + 1));
            pointvec.setZ((maxz - minz) / (clip._panelCount + 1));
            clip.maxx = aabb.maxEdge[0];
            clip.maxy = aabb.maxEdge[1];
            clip.maxz = aabb.maxEdge[2];
            clip.minx = aabb.minEdge[0];
            clip.miny = aabb.minEdge[1];
            clip.minz = aabb.minEdge[2];
            for (int nId = 0; nId < clip._panelCount; nId++)
            {

                QVector3D center; float radius;
                PostCommon::mCommonFunctions::caculateModelCenterAndRaduis(
                    clip.minx, clip.miny, clip.minz,
                    clip.maxx, clip.maxy, clip.maxz,
                    center, radius);

                switch ((POS_TYPE)clip._positionType)
                {
                        case POS_TYPE::NODE:
                        {
                            const auto& v = modeldata->getVectorById(clip._countNodeIds[nId]);
                            if (clip._panelType >= (int)PLANE_TYPE::NORMAL)
                            {
                                // 任意平面：平面方程 n·(p - p0)=0，center 必须取拾取点完整坐标，否则平面不过该点
                                center = v;
                            }
                            else
                            {
                                center[(clip._panelType + 2) % 3] = v[(clip._panelType + 2) % 3];       // XY平面，就是Z方向和点齐平，YZ 就是X方向....
                            }
                            break;
                        }
                        case POS_TYPE::DISTANCE:
                        {
                            QVector3D min(minx, miny, minz);
                            center[(clip._panelType + 2) % 3] = min[(clip._panelType + 2) % 3] + pointvec[(clip._panelType + 2) % 3] * (nId + 1);       // XY平面，就是Z方向和点齐平，YZ 就是X方向....
                            break;
                        }
                        case POS_TYPE::OFFSET:
                        {
                            QVector3D min = clip._offset;
                            center[(clip._panelType + 2) % 3] = min[(clip._panelType + 2) % 3] + clip._distance * (nId + 1);       // XY平面，就是Z方向和点齐平，YZ 就是X方向....
                            break;
                        }
                default:
                    break;
                }

                // 叠加滚动条偏移量（统一沿当前法向）
                {
                    QVector3D n = clip._normal;
                    if (n.lengthSquared() > 1e-12f)
                        center += n.normalized() * clip._sliderOffset;
                }

                // S5 G5（D9 载体）：按槽位寻台账 id 落位（旧 setPlaneData(planeCount,...) 语义承接）
                mxr::CuttingPlaneParams planeData;
                planeData.normal = clip._normal;
                planeData.vertex = center;
                planeData.maxR = radius;
                planeData.updateMask = mxr::CuttingPlaneParams::Mask_Normal | mxr::CuttingPlaneParams::Mask_Vertex
                                     | mxr::CuttingPlaneParams::Mask_MaxR;
                postRender->updateObject(PostCommon::cuttingPlaneIdByIndex(postRender.get(), planeCount), planeData);
                planeCount++;
            }
        }
        _rWidget->updatePaint();
    }

    void mPostSectionPanel::UpdateClipingPlane(int row, PostClipingPanel clip)
    {
        const auto& postRender = _rWidget->getPostRender();
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_rWidget);
        // 等距离时的包围盒步进长度
        QVector3D pointvec(0, 0, 0);
        // S5 门面直接替换：getOneFrameRender 下钻通道消亡——绑定就绪经
        // frameProxy(currentFrame()) 判定（F8：现取，不缓存）
        if (postRender->frameProxy(postRender->currentFrame()) == nullptr)
            return;
        QHash<int, QVector3D> dispdata = _db->getDisplacementDataByFrameID(guidata->_cloudFrame);
        float maxx, minx, maxy, miny, maxz, minz;
        mPostModelData* modeldata = _db->getModelData(guidata->_cloudFrame);
        if (modeldata == nullptr)
            return;
        mxr::AABB aabb = postRender->getCurrentModelData();   // S5：下钻链消亡，getCurrentModelData 同源承载（绑定实例模型 AABB）
        maxx = aabb.maxEdge[0];
        maxy = aabb.maxEdge[1];
        maxz = aabb.maxEdge[2];
        minx = aabb.minEdge[0];
        miny = aabb.minEdge[1];
        minz = aabb.minEdge[2];

        pointvec.setX((maxx - minx) / (clip._panelCount + 1));
        pointvec.setY((maxy - miny) / (clip._panelCount + 1));
        pointvec.setZ((maxz - minz) / (clip._panelCount + 1));
        clip.maxx = aabb.maxEdge[0];
        clip.maxy = aabb.maxEdge[1];
        clip.maxz = aabb.maxEdge[2];
        clip.minx = aabb.minEdge[0];
        clip.miny = aabb.minEdge[1];
        clip.minz = aabb.minEdge[2];

        // 计算已有的面片数量
        int planeCount = 0;
        for (int i = 0; i < row; i++)
        {
            planeCount += _clipsData[i]._panelCount;
        }

        for (int nId = 0; nId < clip._panelCount; nId++)
        {
            QVector3D center; float radius;
            PostCommon::mCommonFunctions::caculateModelCenterAndRaduis(
                clip.minx, clip.miny, clip.minz,
                clip.maxx, clip.maxy, clip.maxz,
                center, radius);

            switch ((POS_TYPE)clip._positionType)
            {
            case POS_TYPE::NODE:
            {
                const auto& v = modeldata->getVectorById(clip._countNodeIds[nId]);
                if (clip._panelType >= (int)PLANE_TYPE::NORMAL)
                {
                    // 任意平面：平面方程 n·(p - p0)=0，center 必须取拾取点完整坐标，否则平面不过该点
                    center = v;
                }
                else
                {
                    center[(clip._panelType + 2) % 3] = v[(clip._panelType + 2) % 3];       // XY平面，就是Z方向和点齐平，YZ 就是X方向....
                }
                break;
            }
            case POS_TYPE::DISTANCE:
            {
                QVector3D min(minx, miny, minz);
                center[(clip._panelType + 2) % 3] = min[(clip._panelType + 2) % 3] + pointvec[(clip._panelType + 2) % 3] * (nId + 1);       // XY平面，就是Z方向和点齐平，YZ 就是X方向....
                break;
            }
            case POS_TYPE::OFFSET:
            {
                QVector3D min = clip._offset;
                center[(clip._panelType + 2) % 3] = min[(clip._panelType + 2) % 3] + clip._distance * (nId + 1);       // XY平面，就是Z方向和点齐平，YZ 就是X方向....
                break;
            }
            default:
                break;
            }

            // 叠加滚动条偏移量（统一沿当前法向）
            {
                QVector3D n = clip._normal;
                if (n.lengthSquared() > 1e-12f)
                    center += n.normalized() * clip._sliderOffset;
            }

            // S5 G5（D9 载体）：addObject(CuttingPlaneParams) 建面返回 id，updateObject 掩码
            // 落位（旧 create+setPlaneData 语义承接）
            mxr::CuttingPlaneParams planeCreate;
            planeCreate.normal = clip._normal;
            planeCreate.vertex = center;
            const int planeId = postRender->addObject(planeCreate);
            if (planeId < 0)   // 建面失败（无就绪实例等）——槽位未占用，不推进 planeCount
                continue;
            mxr::CuttingPlaneParams planeData;
            planeData.normal = clip._normal;
            planeData.vertex = center;
            planeData.maxR = radius;
            planeData.updateMask = mxr::CuttingPlaneParams::Mask_Normal | mxr::CuttingPlaneParams::Mask_Vertex
                                 | mxr::CuttingPlaneParams::Mask_MaxR;
            postRender->updateObject(planeId, planeData);
            planeCount++;
        }
        _rWidget->updatePaint();
    }

    void mPostSectionPanel::innerSlot_planePointBtnClicked(QWidget* btn)
    {
        _basePointIsSelecting = !_basePointIsSelecting;
        setupButtonSelectStyle(btn, _basePointIsSelecting, [=]() {
            // 设置拾取模式为点拾取,并连接槽函数，实时修改输入框内容z
            _rWidget->setPickFilter(mxr::PickFilter::PickMeshNode);
            if (!_basePointFinishPick)
            {
                _basePointFinishPick = connect(_rWidget->getSelectionCaculator(),
                    &mxr::PostSelectionCalculator::signals_FinishPick,
                    this, [=](mxr::FinishPickInfo info) {
                        // 获取当前拾取结果
                        QVector3D nodePos = GetPickSinglePoint();
                        for (int i = 0; i <= AXIS::Z; i++)
                        {
                            _baseLineEdits[i]->setText(QString().number(nodePos[i]));
                        }
                    });
            }
            });

        if (!_basePointIsSelecting)
        {
            disconnect(_basePointFinishPick);
        }
    }
    void mPostSectionPanel::innerSlot_countPlaneBtnClicked()
    {
        _countPointIsSelecting = !_countPointIsSelecting;
        setupButtonSelectStyle(_planePointSelectPanel.btnok, _countPointIsSelecting, [=]() {
            // 设置拾取模式为点拾取,并连接槽函数
            _rWidget->setPickFilter(mxr::PickFilter::PickMeshNode);
            if (!_countPointFinishPick)
            {
                _countPointFinishPick = connect(_rWidget->getSelectionContainer(),
                    &mxr::SelectionDataContainer::ReadyToReadContainer,
                    this, [=]() {
                        // 仅做视觉提示：实时数量不匹配时变灰，但不阻断点击事件（统一在 innerSlot_btnAddClicked 处理）
                        const auto& nodes = _rWidget->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
                        bool matched = !nodes.empty() && (int)nodes.size() == _numEdit->text().toInt();
                        _planePointSelectPanel.btnadd->setDisabled(!matched);
                    });
                {
                    // 初始检测一次
                    const auto& nodes = _rWidget->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
                    bool matched = !nodes.empty() && (int)nodes.size() == _numEdit->text().toInt();
                    _planePointSelectPanel.btnadd->setDisabled(!matched);
                }
            }
            });
        if (!_countPointIsSelecting)
        {
            disconnect(_countPointFinishPick);
        }
    }
    void mPostSectionPanel::innerSlot_btnAddClicked()
    {
        const auto& nodes = _rWidget->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
        int count = _numEdit->text().toInt();
        if ((int)nodes.size() != count)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2,
                QString("当前选中了 %1 个点，与设置数量 %2 不一致，请先选择正确数量的点后再确认。")
                .arg(nodes.size()).arg(count));
            return;
        }

        // 保存当前拾取数据
        _planePointSelectPanel.setState(true);
        this->_countPoints = nodes;
        syncPickedNodesToGuiData();
        // 确认后自动退出拾取模式，避免 paintEvent 继续读取实时选择并清空状态
        this->_countPointIsSelecting = false;
        setupButtonSelectStyle(_planePointSelectPanel.btnok, false, nullptr);
        disconnect(_countPointFinishPick);
    }
    void mPostSectionPanel::innerSlot_clearAllPickNode()
    {
        // 清空时如果还处于点拾取模式，先退出，避免清空后继续响应拾取信号
        if (_countPointIsSelecting)
        {
            _countPointIsSelecting = false;
            setupButtonSelectStyle(_planePointSelectPanel.btnok, false, nullptr);
            disconnect(_countPointFinishPick);
        }

        _planePointSelectPanel.setState(false);
        _countPoints.clear();
        syncPickedNodesToGuiData();
        MechPreData::ClearPostWidgetMeshPick(_rWidget);
        // 显式清除后处理网格高亮，避免残留
        if (_rWidget != nullptr && _rWidget->getPostRender() != nullptr)
        {
            auto highlight_controller = _rWidget->getHighlightController();
            auto postRender = _rWidget->getPostRender();
            if (highlight_controller != nullptr)
            {
                highlight_controller->clearHighlightPostMesh(
                    postRender->getHighLightRender(),
                    postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy(),   // S5：getCurFrameDataProxy→frameProxy(currentFrame())（F8 现取）
                    postRender->frameProxy(postRender->currentFrame())->getHighlightDataProxy());   // S5：getPostFrameDataProxy 同源=帧数据代理，frameProxy 承载
            }
            _rWidget->updatePaint();
        }
        // 再刷新一次选中数据状态
        MechPreData::UpdatePostWidgetHighlight(_rWidget);
    }
    void mPostSectionPanel::innerSlot_itemPressed(QTableWidgetItem* item)
    {
        _currentClip = &_clipsData[item->row()];
        if (_currentClip == nullptr)
        {
            // 清空面板
            return;
        }
        updatePanelControlsFromClip(*_currentClip);
    }

    void mPostSectionPanel::updatePanelControlsFromClip(const PostClipingPanel& clip)
    {
        // 平面类型属于视角/新建偏好，不再随选中行改变，避免误改已生成的切面。

        // 位置类型
        switch ((POS_TYPE)clip._positionType)
        {
        case POS_TYPE::NODE:
            _radioBasePoint->setChecked(true);
            break;
        case POS_TYPE::DISTANCE:
            _radioEqualSpacing->setChecked(true);
            break;
        case POS_TYPE::OFFSET:
            _radioOffset->setChecked(true);
            break;
        default:
            break;
        }
        _posType = clip._positionType;

        // 数量
        _numEdit->setText(MxFunctions::mNumberToString(clip._panelCount));

        // 向量（任意平面）
        _xEdit->setText(MxFunctions::mNumberToString(clip._normal.x()));
        _yEdit->setText(MxFunctions::mNumberToString(clip._normal.y()));
        _zEdit->setText(MxFunctions::mNumberToString(clip._normal.z()));

        // 偏移基准与距离
        if (_baseLineEdits.size() > AXIS::X)
            _baseLineEdits[AXIS::X]->setText(MxFunctions::mNumberToString(clip._offset.x()));
        if (_baseLineEdits.size() > AXIS::Y)
            _baseLineEdits[AXIS::Y]->setText(MxFunctions::mNumberToString(clip._offset.y()));
        if (_baseLineEdits.size() > AXIS::Z)
            _baseLineEdits[AXIS::Z]->setText(MxFunctions::mNumberToString(clip._offset.z()));
        if (_baseLineEdits.size() > AXIS::END)
            _baseLineEdits[AXIS::END]->setText(MxFunctions::mNumberToString(clip._distance));

        // 基于点时恢复拾取点数量匹配提示
        if ((POS_TYPE)clip._positionType == POS_TYPE::NODE)
        {
            _planePointSelectPanel.setState(!clip._countNodeIds.empty());
        }
    }
    void mPostSectionPanel::innerSlot_planeCountChanged(const QString& text)
    {
        int v = text.toInt();
        if (v > 8 - _planeCount)
        {
            // 提示变红
            _numEdit->setStyleSheet("QLineEdit {border:2px solid red;}");
        }
        else
            _numEdit->setStyleSheet("");
    }
    void mPostSectionPanel::innerSlot_planeCountFinished()
    {
        int v = _numEdit->text().toInt();
        if (v > 8 - _planeCount)
        {
            // 强制更改
            _numEdit->setText(QString().number(8 - _planeCount));
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("切面数量超出上限，已自动调整为 %1（最多 8 个切面）。").arg(8 - _planeCount));
        }
    }
}
