#include "mLaminatePanel.h"
#include "mCalcLineEdit.h"
#include "mInfoWindow.h"
#include "mMechGlobalSignals.h"
#include "mMaterialManager.h"
#include "mSetManager.h"
#include "mLaminateManager.h"
#include "mMapRelationManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetBase.h"
#include "mFieldManager.h"

#include "mSglLayerPlate.h"
#include "mGlobalSglLam.h"
#include "mLamPlate.h"

#include <QCompleter>
#include <QTimer>
#include <QPointer>
#include <functional>
#include <algorithm>
#include <QStringListModel>

using namespace MechPropertyData;
using namespace MechCommon;
using namespace MechSetData;

namespace MechMainWindow
{
    mLaminatePanel::mLaminatePanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _LaminateData = nullptr;

        // 只在构造函数创建一次两个折叠块，永远不销毁
        // 1. 定义折叠块（所有板类型共用）
        FoldSection* matFold = new FoldSection("定义", this);
        _comboMat = new QComboBox();
        QToolButton* matBtn = new QToolButton();
        matBtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/cailiao.png"));
        matBtn->setToolTip("新建材料");
        matFold->addRow(new PanelLabel("材料"), _comboMat, matBtn);
        m_foldWidgets.append(matFold);

        // 2. 参数折叠块（动态内容，每次更新时清空重填）
        FoldSection* paramFold = new FoldSection("参数", this);
        m_foldWidgets.append(paramFold);

        // 信号连接（只连一次，永远不重连）
        connect(matBtn, &QToolButton::pressed, this, &mLaminatePanel::slot_newMaterial);
        connect(_comboMat, QOverload<const QString&>::of(&QComboBox::currentIndexChanged),
            this, &mLaminatePanel::slot_matComboBoxChanged);

        _mainLayout->addStretch();
        for (auto fold : m_foldWidgets)
        {
            _mainLayout->removeWidget(fold);
            fold->hide();
        }
    }

    mLaminatePanel::~mLaminatePanel()
    {

    }

    void mLaminatePanel::newLaminatePanel(LaminateType type)
    {
        int laminateID = getCVM->getModelData()
            ->getManager<mLaminateManager>(Physical_Laminate).createID();

        // 根据类型创建对应的数据类
        switch (type)
        {
        case SglLayerPlate:
            _LaminateData = new mSglLayerPlate(laminateID, type);
            break;
        case GlobalSglLam:
            _LaminateData = new mGlobalSglLam(laminateID, type);
            break;
        case LamPlate:
            _LaminateData = new mLamPlate(laminateID, type);
            break;
        default:
            return;
        }

        _LaminateData->setLaminateName(getCVM->getModelData()->createNameByType(Physical_Laminate, _LaminateData->laminateTypeToStringEn()));
        getCVM->getModelData()->addPhysicalField<mLaminateBase>(_LaminateData);

        emit mMechGlobalSignals::getInstance()->updateLaminateTree(true, _LaminateData->getLaminateId());
    }

    void mLaminatePanel::editLaminatePanel(int laminateID)
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        _LaminateData = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getLaminateData(laminateID);

        updateMatCombobox(_comboMat);
        updatePanel();
    }

    void mLaminatePanel::slot_newMaterial()
    {
        if (!getCVM || !_LaminateData) return;

        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMaterial();

        auto& matMgr = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material);
        auto matIDs = matMgr.getAllMatID();
        std::sort(matIDs.begin(), matIDs.end());

        if (matIDs.size() == 0)
        {
            return;
        }
        auto mat = matMgr.getMaterialData(matIDs.at(matIDs.size() - 1));
        if (!mat)
            return;
        _comboMat->setCurrentText(mat->getMaterialName());
        getCVM->getModelData()->getMapRelationManager().appendMapRelation(
            { Physical_Laminate, _LaminateData->getLaminateId() },
            { Physical_Material, mat->getMaterialId() }
        );

        emit mMechGlobalSignals::getInstance()
            ->updateLaminateTree(false, _LaminateData->getLaminateId());
    }

    void mLaminatePanel::slot_matComboBoxChanged(const QString& text)
    {
        if (!getCVM || !_LaminateData) return;

        auto mat = getCVM->getModelData()
            ->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(text);
        if (!mat) return;

        _comboMat->setCurrentText(mat->getMaterialName());
        getCVM->getModelData()->getMapRelationManager().appendMapRelation(
            { Physical_Laminate, _LaminateData->getLaminateId() },
            { Physical_Material, mat->getMaterialId() }
        );

        emit mMechGlobalSignals::getInstance()
            ->updateLaminateTree(false, _LaminateData->getLaminateId());
    }

    void mLaminatePanel::slot_disRotComboBoxChanged(int index)
    {
        if (!getCVM || !_LaminateData) return;

        auto sglLayer = toType<mSglLayerPlate>(_LaminateData);
        if (!sglLayer) return;

        QString text = _disRotCombo->itemText(index);
        if (text.isEmpty() || text == "无")
        {
            // 只删除当前离散场映射，不影响材料和集合
            int oldDfId = getCVM->getModelData()->getMapRelationManager()
                .getMapRelation<int>({ Physical_Laminate, sglLayer->getLaminateId() }, Physical_Field);
            if (oldDfId >= 0)
            {
                getCVM->getModelData()->getMapRelationManager().deleteMapRelation(
                    { Physical_Laminate, sglLayer->getLaminateId() },
                    { Physical_Field, oldDfId }
                );
            }
        }
        else
        {
            int dfId = getCVM->getModelData()->getPhyIDByName(Physical_Field, text);
            if (dfId >= 0)
            {
                getCVM->getModelData()->getMapRelationManager().appendMapRelation(
                    { Physical_Laminate, sglLayer->getLaminateId() },
                    { Physical_Field, dfId }
                );
            }
        }

        emit mMechGlobalSignals::getInstance()
            ->updateLaminateTree(false, sglLayer->getLaminateId());
    }

    void mLaminatePanel::slot_targetObjectChanged()
    {
        if (!_LaminateData || !_regionCombo) return;

        auto sglLayer = toType<mSglLayerPlate>(_LaminateData);
        if (!sglLayer) return;

        QString name = _regionCombo->currentText();
        auto set = getCVM->getModelData()
            ->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(name);
        if (!set) return;

        set->setInternal(false);
        getCVM->getModelData()->getMapRelationManager().appendMapRelation(
            { Physical_Laminate, sglLayer->getLaminateId() },
            { Physical_Set, set->getSetId() }
        );
        emit mMechGlobalSignals::getInstance()
            ->updateLaminateTree(false, sglLayer->getLaminateId());
    }

    void mLaminatePanel::updatePanel()
    {
        if (!_LaminateData) return;

        // 先隐藏所有折叠块
        for (auto fold : m_foldWidgets)
        {
            _mainLayout->removeWidget(fold);
            fold->hide();
        }

        //层合板不需要材料
        if (_LaminateData->getLaminateType() != LamPlate)
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[0]);
            m_foldWidgets[0]->show();
        }
        _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[1]);
        m_foldWidgets[1]->show();

        // 根据类型动态填充参数折叠块
        updateParamPanel();
        updateMatPanel();

        // 刷新布局
        _mainLayout->invalidate();
        _mainLayout->activate();
        this->updateGeometry();
    }

    void mLaminatePanel::updateParamPanel()
    {
        if (!_LaminateData) return;

        auto paramFold = m_foldWidgets[1];
        // 清空参数区，FoldSection自动删除所有旧控件
        while (paramFold->layout()->rowCount() > 0)
        {
            paramFold->removeRow(0);
        }

        // 重置所有动态控件指针
        _regionCombo = nullptr;
        _disRotCombo = nullptr;
        _lamTypeCombo = nullptr;

        // 根据类型创建对应的参数界面
        switch (_LaminateData->getLaminateType())
        {
        case SglLayerPlate:
            createSglLayerPanel(paramFold);
            break;
        case GlobalSglLam:
            createGlobalSglLamPanel(paramFold);
            break;
        case LamPlate:
            createLamPlatePanel(paramFold);
            break;
        default:
            break;
        }

        // 延迟刷新折叠块布局
        QTimer::singleShot(0, paramFold, [=]()
            {
            paramFold->refreshLayout();
            });
    }

    void mLaminatePanel::updateMatCombobox(QComboBox* combobox)
    {
        combobox->blockSignals(true);
        QString currentText = combobox->currentText();

        combobox->clear();
        combobox->addItems(getCVM->getModelData()
            ->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getAllMatNames());

        int idx = combobox->findText(currentText);
        combobox->setCurrentIndex(idx >= 0 ? idx : -1);
        combobox->blockSignals(false);
    }

    void mLaminatePanel::updateMatPanel()
    {
        if (!getCVM || !_LaminateData) return;

        int id = _LaminateData->getLaminateId();
        int matId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Laminate, id }, Physical_Material);

        if (matId == -1)
        {
            _comboMat->setCurrentIndex(-1);
            return;
        }

        auto mat = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(matId);
        if (mat)
        {
            _comboMat->setCurrentText(mat->getMaterialName());
        }
    }

    void mLaminatePanel::createSglLayerPanel(FoldSection* fold)
    {
        auto sglLayer = toType<mSglLayerPlate>(_LaminateData);
        if (!sglLayer) return;

        // 角度输入框
        auto thetaEdit = new mCalcLineEdit(sglLayer->_theta, this, true, false, false);
        connect(thetaEdit, &QLineEdit::editingFinished, this, [=]() {
            sglLayer->_theta = thetaEdit->text();
            });
        fold->addRow(new PanelLabel("角度"), thetaEdit);

        // 厚度输入框
        auto thickEdit = new mCalcLineEdit(sglLayer->_thick, this, false, true, true, true);
        connect(thickEdit, &QLineEdit::editingFinished, this, [=]() {
            sglLayer->_thick = thickEdit->text();
            emit mMechGlobalSignals::getInstance()
                ->updateLaminateTree(false, sglLayer->getLaminateId());
            });
        fold->addRow(new PanelLabel("厚度"), thickEdit);

        // 区域下拉框
        _regionCombo = new QComboBox(this);
        auto tarset = getCVM->getModelData()
            ->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(SetType::Element);
        _regionCombo->addItems(tarset);
        connect(_regionCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLaminatePanel::slot_targetObjectChanged);

        // 恢复选中值
        int setid = getCVM->getModelData()->getMapRelationManager()
            .getMapRelation<int>({ Physical_Laminate, sglLayer->getLaminateId() }, Physical_Set);
        if (setid != -1)
        {
            QString setname = getCVM->getModelData()
                ->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(setid);
            _regionCombo->setCurrentText(setname);
        }
        else if (!tarset.isEmpty())
        {
            _regionCombo->setCurrentIndex(0);
            slot_targetObjectChanged();
        }

        fold->addRow(new PanelLabel("区域"), _regionCombo);

        // 离散转角下拉框
        _disRotCombo = new QComboBox(this);
        _disRotCombo->addItem("无");
        auto& dfMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
        QStringList dfNames = dfMgr.getAllFieldNames(MxFunctions::FieldType::Discrete);
        _disRotCombo->addItems(dfNames);

        // 反写离散场映射
        int disFieldId = getCVM->getModelData()->getMapRelationManager()
            .getMapRelation<int>({ Physical_Laminate, sglLayer->getLaminateId() }, Physical_Field);
        if (disFieldId >= 0)
        {
            QString dfName = dfMgr.getFieldNameByID(disFieldId);
            int idx = _disRotCombo->findText(dfName);
            if (idx >= 0)
                _disRotCombo->setCurrentIndex(idx);
            else
                _disRotCombo->setCurrentIndex(0);
        }
        else
        {
            _disRotCombo->setCurrentIndex(0);
        }

        connect(_disRotCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mLaminatePanel::slot_disRotComboBoxChanged);
        fold->addRow(new PanelLabel("离散转角"), _disRotCombo);
    }

    void mLaminatePanel::createGlobalSglLamPanel(FoldSection* fold)
    {
        auto glsglLayer = toType<mGlobalSglLam>(_LaminateData);
        if (!glsglLayer) return;

        // 角度输入框
        auto thetaEdit = new mCalcLineEdit(glsglLayer->_theta, this, true, false, false);
        connect(thetaEdit, &QLineEdit::editingFinished, this, [=]() {
            glsglLayer->_theta = thetaEdit->text();
            });
        fold->addRow(new PanelLabel("角度"), thetaEdit);

        // 厚度输入框
        auto thickEdit = new mCalcLineEdit(glsglLayer->_thick, this, false, false, false);
        connect(thickEdit, &QLineEdit::editingFinished, this, [=]() {
            glsglLayer->_thick = thickEdit->text();
            emit mMechGlobalSignals::getInstance()
                ->updateLaminateTree(false, glsglLayer->getLaminateId());
            });
        fold->addRow(new PanelLabel("厚度"), thickEdit);
    }

    void mLaminatePanel::createLamPlatePanel(FoldSection* fold)
    {
        auto lamLayer = toType<mLamPlate>(_LaminateData);
        if (!lamLayer) return;

        // ===================== 1. 单层板类型下拉框 =====================
        _lamTypeCombo = new QComboBox(this);
        _lamTypeCombo->addItems({ "单层板", "全局单层板" });
        _lamTypeCombo->blockSignals(true);
        _lamTypeCombo->setCurrentIndex(lamLayer->_type);
        _lamTypeCombo->blockSignals(false);
        fold->addRow(new PanelLabel("单层板类型"), _lamTypeCombo);

        connect(_lamTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mLaminatePanel::onLaminateTypeChanged);

        // ===================== 2. 列表 + 按钮 =====================
        QGridLayout* assignLayout = new QGridLayout();
        _listAssignPart = new QListWidget(this);
        _listAssignPart->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _listAssignPart->setMinimumHeight(600);      // 高度调大
        _listAssignPart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QToolButton* addBtn = new QToolButton(); addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));    addBtn->setToolTip("增加");
        QToolButton* reduceBtn = new QToolButton(); reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); reduceBtn->setToolTip("减少");
        QToolButton* deleteBtn = new QToolButton(); deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); deleteBtn->setToolTip("清空");

        assignLayout->addWidget(_listAssignPart, 0, 0, 8, 7);
        assignLayout->addWidget(addBtn, 0, 7);
        assignLayout->addWidget(reduceBtn, 1, 7);
        assignLayout->addWidget(deleteBtn, 2, 7);
        fold->addRow(assignLayout);

        loadListFromMapRelation();

        // 按钮连接
        connect(addBtn, &QToolButton::clicked, this, &mLaminatePanel::slot_addPartBtn);
        connect(reduceBtn, &QToolButton::clicked, this, &mLaminatePanel::slot_reducePartBtn);
        connect(deleteBtn, &QToolButton::clicked, this, &mLaminatePanel::slot_clearPartBtn);
    }

    void mLaminatePanel::onLaminateTypeChanged(int index)
    {
        auto lamLayer = toType<mLamPlate>(_LaminateData);
        if (!lamLayer) return;

        int lamId = lamLayer->getLaminateId();
        auto& mapMgr = getCVM->getModelData()->getMapRelationManager();
        // 清空所有映射
        QList<int> subLamIds = mapMgr.getMapRelation<QList<int>>(
            { Physical_Laminate, lamId }, Physical_Laminate
        );
        for (int i = subLamIds.size() - 1; i >= 0; --i)
        {
            mapMgr.deleteTabelMapRelation(
                { Physical_Laminate, lamId },
                { Physical_Laminate, -1 },
                i
            );
        }
        lamLayer->_type = index;
        _listAssignPart->clear(); // 切换必清空

        emit mMechGlobalSignals::getInstance()
            ->updateLaminateTree(false, lamId);
    }

    void mLaminatePanel::slot_addPartBtn()
    {
        // 根据当前下拉类型获取候选列表
        QStringList candidateList;
        auto& lamManager = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate);

        if (_lamTypeCombo->currentIndex() == 0)
        {
            candidateList = lamManager.getAllLaminateByTypes(SglLayerPlate);
        }
        else
        {
            candidateList = lamManager.getAllLaminateByTypes(GlobalSglLam);
        }

        QStringListModel* tempModel = new QStringListModel(this);
        tempModel->setStringList(candidateList);

        QListWidgetItem* newItem = new QListWidgetItem();
        _listAssignPart->addItem(newItem);

        QLineEdit* editor = new QLineEdit(_listAssignPart);
        _listAssignPart->setItemWidget(newItem, editor);
        editor->setFocus();

        // 自动补全
        QCompleter* completer = new QCompleter(tempModel, editor);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        editor->setCompleter(completer);
        completer->complete();

        QPointer<QLineEdit> editorGuard(editor);
        auto finishFlag = std::make_shared<bool>(false);

        std::function<void()> finishEditing = [=]() mutable {
            if (*finishFlag) return;
            *finishFlag = true;

            // 统一清理
            auto cleanUp = [&]() {
                editorGuard->deleteLater();
                tempModel->deleteLater();
                };

            if (!editorGuard) {
                int r = _listAssignPart->row(newItem);
                if (r >= 0) delete _listAssignPart->takeItem(r);
                cleanUp();
                return;
            }

            // 获取当前行号
            int row = _listAssignPart->row(newItem);
            if (row < 0) {
                cleanUp();
                return;
            }

            _listAssignPart->removeItemWidget(newItem);
            QString text = editorGuard->text().trimmed();

            // 空值删除
            if (text.isEmpty()) {
                delete _listAssignPart->takeItem(row);
                cleanUp();
                return;
            }

            // 不在候选列表删除
            if (!tempModel->stringList().contains(text)) {
                delete _listAssignPart->takeItem(row);
                cleanUp();
                return;
            }

            // 去重
            bool exists = false;
            for (int i = 0; i < _listAssignPart->count(); ++i)
            {
                QListWidgetItem* it = _listAssignPart->item(i);
                if (it == newItem) continue;
                if (it && it->text() == text) {
                    exists = true;
                    break;
                }
            }
            if (exists)
            {
                delete _listAssignPart->takeItem(row);
                cleanUp();
                return;
            }

            auto lamLayer = toType<mLamPlate>(_LaminateData);
            if (!lamLayer) {
                delete _listAssignPart->takeItem(row);
                cleanUp();
                return;
            }

            newItem->setText(text);

            //映射
            int id = lamManager.getIDByLaminateName(text);
            getCVM->getModelData()->getMapRelationManager()
                .appendTableMapRelation(
                    { Physical_Laminate, lamLayer->getLaminateId() },
                    { Physical_Laminate, id },
                    row
                );
            cleanUp();
            emit mMechGlobalSignals::getInstance()
                ->updateLaminateTree(false, lamLayer->getLaminateId());
            };

        connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
            editor, [=](const QString& choice)
            {
                if (editorGuard)
                {
                    editorGuard->setText(choice);
                    finishEditing();
                }
            }, Qt::UniqueConnection);

        connect(editor, &QLineEdit::editingFinished, editor, finishEditing, Qt::UniqueConnection);

    }

    void mLaminatePanel::slot_reducePartBtn()
    {
        if (!_LaminateData) return;
        auto lamLayer = toType<mLamPlate>(_LaminateData);
        if (!lamLayer) return;

        // 获取当前选中的所有行
        auto selectedItems = _listAssignPart->selectedItems();
        if (selectedItems.isEmpty()) return;

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();

        // 遍历删除每一个选中行
        for (QListWidgetItem* item : selectedItems)
        {
            if (!item) continue;

            // 获取当前行号（必须从List获取，保证准确）
            int row = _listAssignPart->row(item);
            if (row < 0) continue;

            //删除表格映射
            mapRelation.deleteTabelMapRelation(
                { Physical_Laminate, lamLayer->getLaminateId() },
                { Physical_Laminate, -1 },
                row
            );

            // 删除UI行
            delete _listAssignPart->takeItem(row);
        }
        emit mMechGlobalSignals::getInstance()
            ->updateLaminateTree(false, _LaminateData->getLaminateId());
    }

    void mLaminatePanel::slot_clearPartBtn()
    {
        if (!_LaminateData) return;
        auto lamLayer = toType<mLamPlate>(_LaminateData);
        if (!lamLayer) return;

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int totalRow = _listAssignPart->count();

        // 倒序删除（从最后一行删到第0行，保证行号不乱）
        for (int row = totalRow - 1; row >= 0; --row)
        {
            QListWidgetItem* item = _listAssignPart->item(row);
            if (!item) continue;

            // ====================== 删除表格映射 ======================
            mapRelation.deleteTabelMapRelation(
                { Physical_Laminate, lamLayer->getLaminateId() },
                { Physical_Laminate, -1 },
                row
            );
        }

        // UI全部清空
        _listAssignPart->clear();

        emit mMechGlobalSignals::getInstance()
            ->updateLaminateTree(false, _LaminateData->getLaminateId());
    }

    void mLaminatePanel::loadListFromMapRelation()
    {
        _listAssignPart->clear();

        if (!_LaminateData) return;
        auto lamPlate = toType<mLamPlate>(_LaminateData);
        if (!lamPlate) return;

        int propId = lamPlate->getLaminateId();

        QList<int> lamIds = getCVM->getModelData()->getMapRelationManager()
            .getMapRelation<QList<int>>(
                { Physical_Laminate, propId },
                Physical_Laminate 
            );

        // 获取管理器
        auto& lamManager = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate);

        // 当前下拉选中的类型：0=普通单层板 1=全局单层板
        int currentType = _lamTypeCombo->currentIndex();

        // 遍历ID → 取名称 → 显示到列表
        for (int id : lamIds)
        {
            if (id <= 0) continue;

            // 获取单层板数据
            auto lamData = lamManager.getLaminateData(id);
            if (!lamData) continue;

            // 只显示当前下拉类型对应的单层板
            if (currentType == 0 && lamData->getLaminateType() != SglLayerPlate)
                continue;
            if (currentType == 1 && lamData->getLaminateType() != GlobalSglLam)
                continue;

            // 加入列表
            _listAssignPart->addItem(lamData->getLaminateName());
        }
    }
}