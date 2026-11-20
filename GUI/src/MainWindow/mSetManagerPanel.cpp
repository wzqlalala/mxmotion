#include "mSetManagerPanel.h"
#include "mMechGlobalSignals.h"

//视口数据
#include "mViewportModelManager.h"
#include "mViewportModel.h"

//BasicData
#include "mModelData.h"
#include "mMapRelationManager.h"
#include "mSetManager.h"
#include "mSetBase.h"
#include "mNode.h"
#include "mElementSet.h"
#include "mElementSurface.h"
#include "mGeoSet.h"
#include "mPartSet.h"
#include "mFrequencyPoint.h"
#include "mDOFNode.h"
#include "mPanel.h"
#include "mNodeFace.h"

//QT
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHeaderView>

#include <algorithm>
#include <functional>

using namespace MxFunctions;
using namespace MechSetData;
using namespace MechMapRelation;
namespace MechMainWindow
{
    mSetManagerPanel::mSetManagerPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _stackedWidget = new QStackedWidget(this);
        _mainLayout->addWidget(_stackedWidget);

        //===== 列表页：集合管理器 =====
        _listPage = new QWidget(this);
        auto* listLayout = new QVBoxLayout(_listPage);
        //列表页只是承载折叠框的容器，边距清零与mLoadManagerPanel保持一致
        //（面板主布局已有一层默认边距，保留会造成双层边距）
        listLayout->setContentsMargins(0, 0, 0, 0);

        //折叠框：集合管理器
        _foldWidget = new FoldSection("集合管理器", _listPage);
        m_foldWidgets.append(_foldWidget);

        //筛选行：类型 + 状态 + 名称
        auto* filterWidget = new QWidget(_foldWidget);
        auto* filterLayout = new QHBoxLayout(filterWidget);
        filterLayout->setContentsMargins(0, 0, 0, 0);
        _comboType = new QComboBox(filterWidget);
        _comboType->addItem("全部类型");
        _comboType->addItems(QStringList()
            << "几何点集" << "几何线集" << "几何面集" << "几何实体集"
            << "节点集" << "单元边集"  << "单元表面集"
            << "单元集" << "部件集" << "频率点集" << "自由度节点集"
            << "面板集" << "节点面集" << "解析表面集");
        _comboState = new QComboBox(filterWidget);
        _comboState->addItems(QStringList() << "全部状态" << "未使用" << "空集合");
        _editNameFilter = new QLineEdit(filterWidget);
        _editNameFilter->setPlaceholderText("筛选集合名称");
        _editNameFilter->setClearButtonEnabled(true);
        filterLayout->addWidget(_comboType);
        filterLayout->addWidget(_comboState);
        filterLayout->addWidget(_editNameFilter, 1);
        _foldWidget->addRow(filterWidget);

        //表格：集合名称(复选框) / 类型 / 操作(编辑)
        _tableSets = new QTableWidget(_foldWidget);
        _tableSets->setColumnCount(3);
        _tableSets->setHorizontalHeaderLabels(QStringList() << "集合名称" << "类型" << "操作");
        _tableSets->verticalHeader()->setHidden(true);
        _tableSets->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        _tableSets->setSelectionBehavior(QAbstractItemView::SelectRows);
        _tableSets->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _tableSets->setEditTriggers(QAbstractItemView::NoEditTriggers);
        //表头占满整行会贡献较大的最小内容宽度，交由Info dock横向滚动条处理，不再撑宽面板
        _tableSets->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
        //按行高给表格一个最小高度（约16行+表头），用sizeHint计算以适配界面缩放
        _tableSets->setMinimumHeight(_tableSets->horizontalHeader()->sizeHint().height()
            + _tableSets->verticalHeader()->defaultSectionSize() * 16 + 4);
        _foldWidget->addRow(_tableSets);

        //底部按钮：全选 + 删除
        auto* btnWidget = new QWidget(_foldWidget);
        auto* btnLayout = new QHBoxLayout(btnWidget);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        _btnSelectAll = new QPushButton("全选", btnWidget);
        _btnDelete = new QPushButton("删除", btnWidget);
        btnLayout->addWidget(_btnSelectAll);
        btnLayout->addStretch();
        btnLayout->addWidget(_btnDelete);
        _foldWidget->addRow(btnWidget);

        //折叠框置于列表页顶部，多余空白留在其后
        listLayout->addWidget(_foldWidget);
        listLayout->addStretch();

        _stackedWidget->addWidget(_listPage);

        //===== 集合参数页（重命名） =====
        _renamePage = new QWidget(this);
        auto* renameLayout = new QVBoxLayout(_renamePage);

        //标题行：返回按钮 + 集合参数
        auto* renameTitleLayout = new QHBoxLayout;
        _btnBack = new QPushButton("← 返回列表", _renamePage);
        _labelRenameTitle = new QLabel("集合参数", _renamePage);
        QFont titleFont = _labelRenameTitle->font();
        titleFont.setBold(true);
        _labelRenameTitle->setFont(titleFont);
        renameTitleLayout->addWidget(_labelRenameTitle);
        renameTitleLayout->addStretch();
        renameTitleLayout->addWidget(_btnBack);
        renameLayout->addLayout(renameTitleLayout);

        //参数表单：集合名称（可编辑），类型/状态/成员数量（只读，置灰显示）
        auto* formLayout = new QFormLayout;
        formLayout->setSpacing(10);
        _editRenameName = new QLineEdit(_renamePage);
        _editRenameName->setPlaceholderText("请输入集合名称");
        _editRenameType = new QLineEdit(_renamePage);
        _editRenameType->setReadOnly(true);
        _editRenameType->setStyleSheet("background-color: #F5F7FA; color: #808080;");
        _editRenameState = new QLineEdit(_renamePage);
        _editRenameState->setReadOnly(true);
        _editRenameState->setStyleSheet("background-color: #F5F7FA; color: #808080;");
        _editRenameCount = new QLineEdit(_renamePage);
        _editRenameCount->setReadOnly(true);
        _editRenameCount->setStyleSheet("background-color: #F5F7FA; color: #808080;");
        formLayout->addRow("集合名称", _editRenameName);
        formLayout->addRow("类型", _editRenameType);
        formLayout->addRow("状态", _editRenameState);
        formLayout->addRow("成员数量", _editRenameCount);
        renameLayout->addLayout(formLayout);

        //提示与保存按钮
        _labelRenameTip = new QLabel("可修改集合名称。", _renamePage);
        _labelRenameTip->setStyleSheet("color: gray;");
        renameLayout->addWidget(_labelRenameTip);

        _btnSaveRename = new QPushButton("保存重命名", _renamePage);
        renameLayout->addWidget(_btnSaveRename);
        renameLayout->addStretch();

        _stackedWidget->addWidget(_renamePage);

        //信号连接
        connect(_comboType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mSetManagerPanel::slot_typeFilterChanged);
        connect(_comboState, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mSetManagerPanel::slot_stateFilterChanged);
        connect(_editNameFilter, &QLineEdit::textChanged, this, &mSetManagerPanel::slot_nameFilterChanged);
        connect(_tableSets, &QTableWidget::itemChanged, this, &mSetManagerPanel::slot_tableItemChanged);
        connect(_btnSelectAll, &QPushButton::clicked, this, &mSetManagerPanel::slot_selectAllBtn);
        connect(_btnDelete, &QPushButton::clicked, this, &mSetManagerPanel::slot_deleteBtn);
        connect(_btnSaveRename, &QPushButton::clicked, this, &mSetManagerPanel::slot_saveRenameBtn);
        connect(_btnBack, &QPushButton::clicked, this, &mSetManagerPanel::slot_backToListBtn);
    }

    mSetManagerPanel::~mSetManagerPanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }
    }

    void mSetManagerPanel::showSetManager()
    {
        _stackedWidget->setCurrentWidget(_listPage);
        refreshSetList();
    }

    void mSetManagerPanel::slot_typeFilterChanged()
    {
        refreshSetList();
    }

    void mSetManagerPanel::slot_stateFilterChanged()
    {
        refreshSetList();
    }

    void mSetManagerPanel::slot_nameFilterChanged(const QString& text)
    {
        Q_UNUSED(text);
        refreshSetList();
    }

    void mSetManagerPanel::refreshSetList()
    {
        _syncingCheck = true;

        _tableSets->setRowCount(0);

        if (getCVM == nullptr)
        {
            _syncingCheck = false;
            syncSelectAllBtnText();
            return;
        }

        mSetManager& setManager = getCVM->getModelData()->getManager<mSetManager>(Physical_Set);
        //刷新前取消折叠块的最大高度限制，避免内容尺寸被旧上限裁剪后量取
        _foldWidget->contentWidget()->setMaximumHeight(QWIDGETSIZE_MAX);
        QString typeFilter = _comboType->currentText();
        int stateFilter = _comboState->currentIndex();//0全部 1未使用 2空集合
        QString nameFilter = _editNameFilter->text();

        for (int setID : setManager.getAllSetID())
        {
            mSetBase* set = setManager.getSetData(setID);
            if (set == nullptr)
            {
                continue;
            }

            //类型筛选
            if (typeFilter != "全部类型" && getSetTypeNameByType(set->getSetType()) != typeFilter)
            {
                continue;
            }
            //状态筛选
            if (stateFilter == 1 && !isSetUnused(setID))
            {
                continue;
            }
            if (stateFilter == 2 && !isSetEmpty(set))
            {
                continue;
            }
            //名称筛选
            if (!nameFilter.isEmpty() && !set->getSetName().contains(nameFilter, Qt::CaseInsensitive))
            {
                continue;
            }

            int row = _tableSets->rowCount();
            _tableSets->insertRow(row);

            QTableWidgetItem* nameItem = new QTableWidgetItem(set->getSetName());
            nameItem->setFlags((nameItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
            nameItem->setCheckState(Qt::Unchecked);
            nameItem->setData(Qt::UserRole, setID);
            _tableSets->setItem(row, 0, nameItem);

            QTableWidgetItem* typeItem = new QTableWidgetItem(getSetTypeNameByType(set->getSetType()));
            typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
            _tableSets->setItem(row, 1, typeItem);

            auto* editBtn = new QPushButton("编辑");
            connect(editBtn, &QPushButton::clicked, this, [this, setID]() {
                showRenamePage(setID);
                });
            _tableSets->setCellWidget(row, 2, editBtn);
        }

        _syncingCheck = false;
        syncSelectAllBtnText();
        //行数变化后刷新折叠块高度，多余高度由列表页末尾的stretch承担
        _foldWidget->refreshLayout();
    }

    QTableWidgetItem* mSetManagerPanel::nameItemOfRow(int row)
    {
        return _tableSets->item(row, 0);
    }

    void mSetManagerPanel::syncSelectAllBtnText()
    {
        int rowCount = _tableSets->rowCount();
        if (rowCount == 0)
        {
            _btnSelectAll->setText("全选");
            return;
        }
        bool allChecked = true;
        for (int row = 0; row < rowCount; ++row)
        {
            QTableWidgetItem* nameItem = nameItemOfRow(row);
            if (nameItem == nullptr || nameItem->checkState() != Qt::Checked)
            {
                allChecked = false;
                break;
            }
        }
        _btnSelectAll->setText(allChecked ? "取消全选" : "全选");
    }

    void mSetManagerPanel::slot_tableItemChanged(QTableWidgetItem* item)
    {
        if (item == nullptr || item->column() != 0)
        {
            return;
        }

        //Ctrl多选场景联动：勾选发生变化的行在当前选中集合内时，
        //把相同勾选状态应用到其他选中行
        if (!_syncingCheck && _tableSets->selectionModel() != nullptr
            && _tableSets->selectionModel()->isRowSelected(item->row(), QModelIndex()))
        {
            Qt::CheckState state = item->checkState();
            _syncingCheck = true;
            QModelIndexList selectedRows = _tableSets->selectionModel()->selectedRows();
            for (const QModelIndex& idx : selectedRows)
            {
                QTableWidgetItem* nameItem = nameItemOfRow(idx.row());
                if (nameItem != nullptr && nameItem != item && nameItem->checkState() != state)
                {
                    nameItem->setCheckState(state);
                }
            }
            _syncingCheck = false;
        }

        syncSelectAllBtnText();
    }

    void mSetManagerPanel::slot_selectAllBtn()
    {
        //存在未勾选行时全选，否则取消全选
        bool anyUnchecked = false;
        for (int row = 0; row < _tableSets->rowCount(); ++row)
        {
            QTableWidgetItem* nameItem = nameItemOfRow(row);
            if (nameItem == nullptr || nameItem->checkState() == Qt::Unchecked)
            {
                anyUnchecked = true;
                break;
            }
        }
        Qt::CheckState state = anyUnchecked ? Qt::Checked : Qt::Unchecked;

        //批量设置期间屏蔽联动勾选，由末尾统一刷新按钮文字
        _syncingCheck = true;
        for (int row = 0; row < _tableSets->rowCount(); ++row)
        {
            QTableWidgetItem* nameItem = nameItemOfRow(row);
            if (nameItem != nullptr)
            {
                nameItem->setCheckState(state);
            }
        }
        _syncingCheck = false;

        syncSelectAllBtnText();
    }

    void mSetManagerPanel::slot_deleteBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }

        QVector<int> checkedIDs;
        QList<int> checkedRows;
        for (int row = 0; row < _tableSets->rowCount(); ++row)
        {
            QTableWidgetItem* nameItem = nameItemOfRow(row);
            if (nameItem != nullptr && nameItem->checkState() == Qt::Checked)
            {
                checkedIDs.push_back(nameItem->data(Qt::UserRole).toInt());
                checkedRows.push_back(row);
            }
        }
        if (checkedIDs.isEmpty())
        {
            return;
        }

        //先从表格移除，删除数据与树同步由mTreeView接收deleteSetsSig完成
        _syncingCheck = true;
        std::sort(checkedRows.begin(), checkedRows.end(), std::greater<int>());
        for (int row : checkedRows)
        {
            _tableSets->removeRow(row);
        }
        _syncingCheck = false;

        emit MechCommon::mMechGlobalSignals::getInstance()->deleteSetsSig(checkedIDs);

        syncSelectAllBtnText();
    }

    void mSetManagerPanel::showRenamePage(int setID)
    {
        if (getCVM == nullptr)
        {
            return;
        }
        mSetBase* set = getCVM->getModelData()->getManager<mSetManager>(Physical_Set).getSetData(setID);
        if (set == nullptr)
        {
            return;
        }

        _renameSetID = setID;
        _editRenameName->setText(set->getSetName());
        _editRenameType->setText(getSetTypeNameByType(set->getSetType()));
        _editRenameState->setText(getSetStateText(set, setID));
        _editRenameCount->setText(QString::number(getSetMemberCount(set)));
        _labelRenameTip->setText("可修改集合名称。");
        _stackedWidget->setCurrentWidget(_renamePage);
    }

    void mSetManagerPanel::slot_saveRenameBtn()
    {
        if (getCVM == nullptr || _renameSetID < 0)
        {
            return;
        }
        mSetManager& setManager = getCVM->getModelData()->getManager<mSetManager>(Physical_Set);
        mSetBase* set = setManager.getSetData(_renameSetID);
        if (set == nullptr)
        {
            return;
        }

        QString newName = _editRenameName->text().trimmed();
        if (newName.isEmpty())
        {
            _labelRenameTip->setText("名称不能为空。");
            return;
        }
        if (newName == set->getSetName())
        {
            //名称未变化，直接返回列表
            showSetManager();
            return;
        }
        if (setManager.ExistSetName(newName))
        {
            _labelRenameTip->setText("该名称已存在，请更换名称。");
            return;
        }

        set->setSetName(newName);
        //树节点文字与状态由mTreeView接收createUpdateSetSig同步
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, set->getSetId());

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功重命名集合 %1").arg(newName), true);
        showSetManager();
    }

    void mSetManagerPanel::slot_backToListBtn()
    {
        showSetManager();
    }

    bool mSetManagerPanel::isSetEmpty(MechSetData::mSetBase* set)
    {
        if (set == nullptr)
        {
            return false;
        }
        switch (set->getSetType())
        {
        case MxFunctions::Node:
        {
            auto* s = dynamic_cast<mNode*>(set);
            return s != nullptr && s->getNodeData().empty();
        }
        case MxFunctions::DOFNode:
        {
            auto* s = dynamic_cast<mDOFNode*>(set);
            return s != nullptr && s->getNodeData().empty();
        }
        case MxFunctions::NodeSurface:
        {
            auto* s = dynamic_cast<mNodeFace*>(set);
            return s != nullptr && s->getNodeData().empty();
        }
        case MxFunctions::Element:
        {
            auto* s = dynamic_cast<mElementSet*>(set);
            return s != nullptr && s->getElementSetData().empty();
        }
        case MxFunctions::ElementSurface:
        case MxFunctions::ElementEdge:
        {
            auto* s = dynamic_cast<mElementSurface*>(set);
            return s != nullptr && s->getElementSurfaceData().empty();
        }
        case MxFunctions::GeoPoint:
        case MxFunctions::GeoEdge:
        case MxFunctions::GeoFace:
        case MxFunctions::GeoSolid:
        {
            auto* s = dynamic_cast<mGeoSet*>(set);
            return s != nullptr && s->getGeoSetData().empty();
        }
        case MxFunctions::Part:
        {
            auto* s = dynamic_cast<mPartSet*>(set);
            return s != nullptr && s->getPartSetData().empty();
        }
        case MxFunctions::FrequencyPoint:
        {
            auto* s = dynamic_cast<mFrequencyPoint*>(set);
            return s != nullptr && s->orderCount() == 0;
        }
        case MxFunctions::Panel:
        {
            auto* s = dynamic_cast<mPanel*>(set);
            return s != nullptr && s->elenames().isEmpty() && s->nodeNames().isEmpty();
        }
        default:
            //解析表面集等由参数定义的集合不按拾取数据判空
            return false;
        }
    }

    bool mSetManagerPanel::isSetUnused(int setID)
    {
        mMapRelationManager& mapManager = getCVM->getModelData()->getMapRelationManager();

        //引用集合的功能类型：载荷/边界、接触、工况、周期边界、优化设计域、各类参与输出
        const MxFunctions::PhysicalFieldType consumerTypes[] = {
            MxFunctions::Physical_LoadBoundary,
            MxFunctions::Physical_Contact,
            MxFunctions::Physical_Case,
            MxFunctions::Physical_PeriodicBoundary,
            MxFunctions::Physical_OptimationDesign,
            MxFunctions::Physical_ModalContri,
            MxFunctions::Physical_NodeContri,
            MxFunctions::Physical_PanelContri,
            MxFunctions::Physical_TPContri,
        };
        for (MxFunctions::PhysicalFieldType phyType : consumerTypes)
        {
            if (!mapManager.getMapRelation<std::set<int>>({ Physical_Set, setID }, phyType).empty())
            {
                return false;
            }
        }

        //面板集按名称引用单元/节点集：被任何面板集收录的集合视为已使用
        mSetManager& setManager = getCVM->getModelData()->getManager<mSetManager>(Physical_Set);
        mSetBase* self = setManager.getSetData(setID);
        if (self == nullptr)
        {
            return false;
        }
        QString selfName = self->getSetName();
        for (int id : setManager.getAllSetID())
        {
            auto* panel = dynamic_cast<mPanel*>(setManager.getSetData(id));
            if (panel == nullptr || id == setID)
            {
                continue;
            }
            if (panel->elenames().contains(selfName) || panel->nodeNames().contains(selfName))
            {
                return false;
            }
        }
        return true;
    }

    int mSetManagerPanel::getSetMemberCount(MechSetData::mSetBase* set)
    {
        if (set == nullptr)
        {
            return 0;
        }
        switch (set->getSetType())
        {
        case MxFunctions::Node:
        {
            auto* s = dynamic_cast<mNode*>(set);
            return s ? static_cast<int>(s->getNodeData().size()) : 0;
        }
        case MxFunctions::DOFNode:
        {
            auto* s = dynamic_cast<mDOFNode*>(set);
            return s ? static_cast<int>(s->getNodeData().size()) : 0;
        }
        case MxFunctions::NodeSurface:
        {
            auto* s = dynamic_cast<mNodeFace*>(set);
            return s ? static_cast<int>(s->getNodeData().size()) : 0;
        }
        case MxFunctions::Element:
        {
            auto* s = dynamic_cast<mElementSet*>(set);
            return s ? static_cast<int>(s->getElementSetData().size()) : 0;
        }
        case MxFunctions::ElementSurface:
        case MxFunctions::ElementEdge:
        {
            auto* s = dynamic_cast<mElementSurface*>(set);
            return s ? static_cast<int>(s->getElementSurfaceData().size()) : 0;
        }
        case MxFunctions::GeoPoint:
        case MxFunctions::GeoEdge:
        case MxFunctions::GeoFace:
        case MxFunctions::GeoSolid:
        {
            auto* s = dynamic_cast<mGeoSet*>(set);
            return s ? static_cast<int>(s->getGeoSetData().size()) : 0;
        }
        case MxFunctions::Part:
        {
            auto* s = dynamic_cast<mPartSet*>(set);
            return s ? static_cast<int>(s->getPartSetData().size()) : 0;
        }
        case MxFunctions::FrequencyPoint:
        {
            auto* s = dynamic_cast<mFrequencyPoint*>(set);
            return s ? s->orderCount() : 0;
        }
        case MxFunctions::Panel:
        {
            auto* s = dynamic_cast<mPanel*>(set);
            return s ? s->elenames().size() + s->nodeNames().size() : 0;
        }
        default:
            //解析表面集等由参数定义，无成员数量
            return 0;
        }
    }

    QString mSetManagerPanel::getSetStateText(MechSetData::mSetBase* set, int setID)
    {
        if (isSetEmpty(set))
        {
            return "空集合";
        }
        if (isSetUnused(setID))
        {
            return "未使用";
        }
        return "已使用";
    }

    QString mSetManagerPanel::getSetTypeNameByType(MxFunctions::SetType type)
    {
        switch (type)
        {
        case MxFunctions::GeoPoint:
            return "几何点集";
        case MxFunctions::GeoEdge:
            return "几何线集";
        case MxFunctions::GeoFace:
            return "几何面集";
        case MxFunctions::GeoSolid:
            return "几何实体集";
        case MxFunctions::Node:
            return "节点集";
        case MxFunctions::ElementEdge:
            return "单元边集";
        case MxFunctions::SurfaceAnalysis:
            return "解析表面集";
        case MxFunctions::ElementSurface:
            return "单元表面集";
        case MxFunctions::Element:
            return "单元集";
        case MxFunctions::Part:
            return "部件集";
        case MxFunctions::FrequencyPoint:
            return "频率点集";
        case MxFunctions::DOFNode:
            return "自由度节点集";
        case MxFunctions::Panel:
            return "面板集";
        case MxFunctions::NodeSurface:
            return "节点面集";
        default:
            return "集合";
        }
    }
}
