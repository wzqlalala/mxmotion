#include "mPropertyCheckPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"
#include "mInfoWindow.h"
#include "mCalcLineEdit.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"
#include "PickCommonFunction.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/CCSMPreRender.h"

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

//BasicData
#include "mPropertyBase.h"
#include "mPropertyManager.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mElementBase.h"
#include "mDataElement.h"
#include "mSetBase.h"
#include "mPartSet.h"
#include "mElementSet.h"
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace mxr;
using namespace MechPropertyData;
using namespace MechElementData;
using namespace MxFunctions;
using namespace MechSetData;
namespace MechMainWindow
{
    mPropertyCheckPanel::mPropertyCheckPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);


        _currentViewportPrivate = nullptr;

        // candidateParts: QStringList 候选项，提前准备好
        _partModel = new QStringListModel();

        // --- 保持之前选项 ---
        //connect(mGlobalSignals::getInstance(), &mGlobalSignals::updateModelData, this, [this](PhysicalFieldType physicalFieldType, int modelId)
        //    {
        //        if (physicalFieldType == Physical_Set)
        //        {
        //            _comboPartSet->clear();

        //            _comboPartSet->addItem("请选择部件集合");
        //            _comboPartSet->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        //            _comboPartSet->setCurrentIndex(0);                 // 默认不选中任何项

        //            _comboPartSet->addItems(getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Part));

        //            _comboEleSet->clear();

        //            _comboEleSet->addItem("请选择单元集合");
        //            _comboEleSet->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        //            _comboEleSet->setCurrentIndex(0);                 // 默认不选中任何项

        //            _comboEleSet->addItems(getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Element));
        //        }
        //    });

        // ========== 部件折叠块(0) ==========
        FoldSection* partFold = new FoldSection("部件", this);
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

        //要检查的部件列表
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

        partFold->addRow(new PanelLabel("部件集合"), _comboPartSet);
        partFold->addRow(assignLayout);
        m_foldWidgets.append(partFold);

        connect(allBtn, &QToolButton::clicked, this, &mPropertyCheckPanel::slot_allPartBtn);
        connect(addBtn, &QToolButton::clicked, this, &mPropertyCheckPanel::slot_addPartBtn);
        connect(reduceBtn, &QToolButton::clicked, this, &mPropertyCheckPanel::slot_reducePartBtn);
        connect(deleteBtn, &QToolButton::clicked, this, &mPropertyCheckPanel::slot_clearPartBtn);

        // ========== 属性信息折叠块(1) ==========
        FoldSection* propertyFold = new FoldSection("属性信息", this);

        _tableProperty = new QTableWidget();
        _tableProperty->setColumnCount(3);
        //_tableProperty->verticalHeader()->setHidden(true);
        _tableProperty->resizeColumnsToContents();
        _tableProperty->setSelectionBehavior(QAbstractItemView::SelectRows);  // 整行选中
        QStringList headerText;
        headerText << QString("部件") << QString("截面属性") << QString("单元类型");
        _tableProperty->setHorizontalHeaderLabels(headerText);
        _tableProperty->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        _tableProperty->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _tableProperty->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _tableProperty->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _tableProperty->setSelectionBehavior(QAbstractItemView::SelectItems);

        propertyFold->addRow(_tableProperty);
        m_foldWidgets.append(propertyFold);
        
        // ========== 部件折叠块(2) ==========
        FoldSection* eleFold = new FoldSection("单元", this);
        // 部件选择
        _comboEleSet = new QComboBox();
        // 插入提示项
        _comboEleSet->addItem("请选择单元集合");
        _comboEleSet->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        _comboEleSet->setCurrentIndex(0);                 // 默认不选中任何项

        connect(_comboEleSet, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            QString eleSetName = _comboEleSet->currentText();
            _comboEleSet->blockSignals(true);
            _comboEleSet->setCurrentIndex(0);
            _comboEleSet->blockSignals(false);
            slot_eleSetComboBoxChanged(eleSetName);
            });

        //要检查的部件列表
        assignLayout = new QGridLayout();
        _listAssignEle = new QListWidget();
        _listAssignEle->setSelectionMode(QAbstractItemView::ExtendedSelection);
        allBtn = new QToolButton(); allBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png")); allBtn->setToolTip("全部");
        addBtn = new QToolButton(); addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); addBtn->setToolTip("增加");
        reduceBtn = new QToolButton(); reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); reduceBtn->setToolTip("减少");
        deleteBtn = new QToolButton(); deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); deleteBtn->setToolTip("清空");

        assignLayout->addWidget(_listAssignEle, 0, 0, 8, 7);
        assignLayout->addWidget(allBtn, 0, 7);
        assignLayout->addWidget(addBtn, 1, 7);
        assignLayout->addWidget(reduceBtn, 2, 7);
        assignLayout->addWidget(deleteBtn, 3, 7);

        eleFold->addRow(new PanelLabel("单元集合"), _comboEleSet);
        eleFold->addRow(assignLayout);
        m_foldWidgets.append(eleFold);

        connect(allBtn, &QToolButton::clicked, this, &mPropertyCheckPanel::slot_allIdBtn);
        connect(addBtn, &QToolButton::clicked, this, &mPropertyCheckPanel::slot_addIdBtn);
        connect(reduceBtn, &QToolButton::clicked, this, &mPropertyCheckPanel::slot_reduceIdBtn);
        connect(deleteBtn, &QToolButton::clicked, this, &mPropertyCheckPanel::slot_clearIdBtn);

        // ========== 单元信息折叠块(3) ==========
        FoldSection* elePropertyFold = new FoldSection("单元信息", this);

        _tableEle = new QTableWidget();
        _tableEle->setColumnCount(2);
        //_tableEle->verticalHeader()->setHidden(true);
        _tableEle->resizeColumnsToContents();
        _tableEle->setSelectionBehavior(QAbstractItemView::SelectRows);  // 整行选中
        headerText.clear();
        headerText << QString("网格ID") << QString("单元类型");
        _tableEle->setHorizontalHeaderLabels(headerText);
        _tableEle->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        _tableEle->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _tableEle->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _tableEle->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _tableEle->setSelectionBehavior(QAbstractItemView::SelectItems);

        elePropertyFold->addRow(_tableEle);
        m_foldWidgets.append(elePropertyFold);

        _mainLayout->addStretch();

        for (int i = 0; i < m_foldWidgets.size(); i++)
        {
            _mainLayout->removeWidget(m_foldWidgets[i]); m_foldWidgets[i]->hide();
        }

    }

    mPropertyCheckPanel::~mPropertyCheckPanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }

    }

    void mPropertyCheckPanel::checkPropertyPanel(int type)
    {
        _checkType = type;
        setEditing(true);
        connectSig();
        for (int i = 0; i < m_foldWidgets.size(); i++)
        {
            _mainLayout->removeWidget(m_foldWidgets[i]); m_foldWidgets[i]->hide();
        }
        if (type == 0)
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[0]); m_foldWidgets[0]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[1]); m_foldWidgets[1]->show();

            slot_allPartBtn();
        }
        else
        {
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[2]); m_foldWidgets[2]->show();
            _mainLayout->insertWidget(_mainLayout->count() - 1, m_foldWidgets[3]); m_foldWidgets[3]->show();

            slot_allIdBtn();
        }
        _comboPartSet->clear();

        _comboPartSet->addItem("请选择部件集合");
        _comboPartSet->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        _comboPartSet->setCurrentIndex(0);                 // 默认不选中任何项

        _comboPartSet->addItems(getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Part));

        _comboEleSet->clear();

        _comboEleSet->addItem("请选择单元集合");
        _comboEleSet->setItemData(0, 0, Qt::UserRole - 1); // 禁止选中
        _comboEleSet->setCurrentIndex(0);                 // 默认不选中任何项

        _comboEleSet->addItems(getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(Element));
    }

    void mPropertyCheckPanel::uneditPropertyCheckPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        disconnectSig();
    }


    void mPropertyCheckPanel::slot_partSetComboBoxChanged(const QString& text)
    {
        if (getCVM == nullptr)
        {
            return;
        }
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

        //获取增加的部件id
        std::vector<int> addedIds;
        QSet<int> currentSet;
        for (auto id : _lastPickPartData)
        {
            currentSet.insert(id);
        }
        for (auto& e : partIds)
        {
            if (!currentSet.contains(e))
            {
                addedIds.push_back(e);
            }
        }

        highlightPart(addedIds);

        //把所有属性都添加到部件中去
        for (int i = 0; i < addedIds.size(); i++)
        {
            auto partName = meshData->getPartNameByPartId(addedIds.at(i));
            _listAssignPart->addItem(partName);
            appendPartProperty(partName);
        }
    }

    void mPropertyCheckPanel::slot_eleSetComboBoxChanged(const QString& text)
    {
        if (getCVM == nullptr)
        {
            return;
        }
        auto setData = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(text);
        if (setData == nullptr)
        {
            return;
        }
        mDataElement* elementData = getCVM->getModelData()->getElementData();
        auto eleSet = dynamic_cast<MechSetData::mElementSet*>(setData);
        auto eleData = eleSet->getElementSetData();
        if (eleData.empty())
        {
            return;
        }
        std::vector<mxr::MeshSelectInfoData> eles;
        for (auto ele : eleData)
        {
            MeshSelectInfoData n;
            n.data_id.element_id = ele.result_id;
            n.part_id = ele.part_id;
            eles.push_back(n);
        }

        highlightEle(eles);

        QList<int> res;
        for (auto ele : _lastPickElementData)
        {
            res.append(ele.data_id.element_id);
        }

        fillListWidget(res);
        fillEleTableWidget();
    }

    void mPropertyCheckPanel::slot_allPartBtn()
    {
        QStringList partList = getCVM->getModelData()->getMeshData()->getAllPartNames();
        _listAssignPart->clear();
        _listAssignPart->addItems(partList);

        //先清空之前的所有属性

        _tableProperty->clearContents();
        _tableProperty->setRowCount(0);
        //把所有属性都添加到部件中去
        for (int i = 0; i < partList.size(); i++)
        {
            appendPartProperty(partList[i]);
        }

        highlightPart(getCVM->getModelData()->getMeshData()->getAllPartIds());
    }

    void mPropertyCheckPanel::slot_addPartBtn()
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

        // 4) 安全引用
        // 用 QPointer 管理 editor 的有效性
        QPointer<QLineEdit> editorGuard(editor);
        // finishFlag 防止 finishEditing 被重复执行
        auto finishFlag = std::make_shared<bool>(false);

        // helper：安全地结束编辑（幂等）
        std::function<void()> finishEditing = [=]() mutable {
            if (*finishFlag) return;      // 已经完成过一次，直接返回
            *finishFlag = true;           // 标志为已完成

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
                // item 不在列表（可能已被外部删除），直接清理 editor
                editorGuard->deleteLater();
                return;
            }

            // 先把 editor widget 从 item 中移除（但不要 delete item 还没决定）
            _listAssignPart->removeItemWidget(newItem);

            QString text = editorGuard->text().trimmed();

            // 1) 空文本 -> 删除该行
            if (text.isEmpty()) {
                delete _listAssignPart->takeItem(row);
                editorGuard->deleteLater();
                return;
            }

            // 2) 非候选 -> 删除
            // 如果你使用的是 model，请用 candidateModel->stringList().contains(...)
            QStringList candidates = _partModel->stringList();
            if (!candidates.contains(text)) {
                delete _listAssignPart->takeItem(row);
                editorGuard->deleteLater();
                return;
            }

            // 3) 去重：检查列表中是否存在相同文本（忽略 newItem 本身）
            bool exists = false;
            for (int i = 0; i < _listAssignPart->count(); ++i) {
                QListWidgetItem* it = _listAssignPart->item(i);
                if (!it) continue;
                if (it == newItem) continue;
                if (it->text() == text) { exists = true; break; }
            }
            if (exists) {
                delete _listAssignPart->takeItem(row);
                editorGuard->deleteLater();
                return;
            }
            // 通过校验 -> 写入 item 文本，保留 item
            newItem->setText(text);
            int partId = getCVM->getModelData()->getMeshData()->getPartIdByPartName(text);
            highlightPart(vector<int>{partId});
            this->appendPartProperty(text);
            editorGuard->deleteLater();
            };
        // completer 激活时（鼠标点击/回车选项） — 使用 editor 作为 receiver（自动断开在 editor 销毁时）
        connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
            editor,  // receiver
            [=](const QString& choice) {
                if (!editorGuard) return;
                editorGuard->setText(choice);
                finishEditing();
            }, Qt::UniqueConnection);

        // editingFinished（回车或失去焦点）也触发结束（两者可能同时发生，所以有 finishFlag 防护）
        connect(editor, &QLineEdit::editingFinished, editor, [=]() {
            finishEditing();
            }, Qt::UniqueConnection);

        // 可选：Esc 键取消（按 Esc 清除并删除行）
        editor->installEventFilter(new QObject(editor)); // placeholder if you want custom handling
        // 若需要自定义 Esc 行为，可用子类或事件过滤器捕获 Qt::Key_Escape

    }

    void mPropertyCheckPanel::slot_reducePartBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        auto items = _listAssignPart->selectedItems();
        for (QListWidgetItem* item : items) {
            if (!item) continue;

            QString text = item->text().trimmed();
            if (!text.isEmpty()) {
                //从属性里面移除部件
                int partId = getCVM->getModelData()->getMeshData()->getPartIdByPartName(text);
                unHighlightPart(vector<int>{partId});
                _tableProperty->removeRow(_listAssignPart->row(item));
            }

            // 从 UI 中删除
            delete _listAssignPart->takeItem(_listAssignPart->row(item));
        }
    }

    void mPropertyCheckPanel::slot_clearPartBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        this->blockSignals(true);
        getCVM->clearPick();
        this->blockSignals(false);

        // 清空 UI
        _listAssignPart->clear();

        _tableProperty->clearContents();
        _tableProperty->setRowCount(0);

    }

    void mPropertyCheckPanel::slot_allIdBtn()
    {
        QTime time;
        time.start();
        if (getCVM == nullptr)
        {
            return;
        }
        auto meshData = getCVM->getModelData()->getMeshData();
        mDataElement* elementData = getCVM->getModelData()->getElementData();
        auto eleData = meshData->getAllElements();
        qDebug() << "获取所有的网格Id" << time.elapsed();
        std::vector<mxr::MeshSelectInfoData> eles;
        QList<int> ids;
        for (auto& ele : eleData)
        {
            MeshSelectInfoData n;
            n.data_id.element_id = ele.result_id;
            n.part_id = ele.part_id;
            eles.push_back(n);
            ids.push_back(ele.result_id);
        }
        highlightEle(eles);
        qDebug() << "结束高亮" << time.elapsed();
        fillListWidget(ids);
        qDebug() << "结束填充列表" << time.elapsed();
        fillEleTableWidget();
    }

    void mPropertyCheckPanel::slot_addIdBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        auto meshData = getCVM->getModelData()->getMeshData();
        mDataElement* elementData = getCVM->getModelData()->getElementData();
        // === Step 1: 在添加新行之前，保存当前 ID 列表 ===
        QList<int> oldIds = collectIdsFromListWidget();

        // === Step 2: 插入新 item ===
        QListWidgetItem* newItem = new QListWidgetItem();
        _listAssignEle->insertItem(0, newItem);

        QLineEdit* editor = new QLineEdit(_listAssignEle);
        _listAssignEle->setItemWidget(newItem, editor);
        editor->setFocus();

        QPointer<QLineEdit> editorGuard(editor);
        auto finishFlag = std::make_shared<bool>(false);

        std::function<void()> finishEditing = [=]() mutable {
            if (*finishFlag) return;
            *finishFlag = true;
            QTime time;
            time.start();
            if (!editorGuard) {
                int r = _listAssignEle->row(newItem);
                if (r >= 0) delete _listAssignEle->takeItem(r);
                return;
            }

            int row = _listAssignEle->row(newItem);
            if (row < 0) {
                editorGuard->deleteLater();
                return;
            }

            _listAssignEle->removeItemWidget(newItem);
            QString text = editorGuard->text().trimmed();

            if (text.isEmpty()) {
                delete _listAssignEle->takeItem(row);
                editorGuard->deleteLater();
                return;
            }

            // 检查重复文本
            bool exists = false;
            for (int i = 0; i < _listAssignEle->count(); ++i) {
                QListWidgetItem* it = _listAssignEle->item(i);
                if (it == newItem) continue;
                if (it->text() == text) { exists = true; break; }
            }
            if (exists) {
                delete _listAssignEle->takeItem(row);
                editorGuard->deleteLater();
                return;
            }

            // 写入 item 文本
            newItem->setText(text);
            editorGuard->deleteLater();

            // === Step 3: 比较新旧 IDs，找出新增的 ===
            qDebug() << "开始解析原有的数据" << time.elapsed();
            QList<int> newIds = collectIdsFromListWidget();
            qDebug() << "结束解析原有的数据" << time.elapsed();
            QSet<int> oldSet;
            for (auto id : oldIds)
            {
                oldSet.insert(id);
            }

            QList<int> addedIds;
            for (int id : newIds)
                if (!oldSet.contains(id))
                    addedIds.append(id);
            qDebug() << "获取新增的单元数据" << time.elapsed();
            if (!addedIds.isEmpty()) {
                std::vector<mxr::MeshSelectInfoData> eles;
                for (auto& ele : addedIds)
                {
                    MeshSelectInfoData n;
                    n.data_id.element_id = ele;
                    int partId = meshData->getPartIdByEleId(ele);
                    if (partId <= 0)
                    {
                        continue;
                    }
                    n.part_id = partId;
                    eles.push_back(n);
                }
                qDebug() << "结束赋予" << time.elapsed();
                highlightEle(eles);
                qDebug() << "结束高亮" << time.elapsed();
                fillListWidget(newIds);
                qDebug() << "结束填充列表" << time.elapsed();
                fillEleTableWidget();
            }
            else
            {
                int r = _listAssignEle->row(newItem);
                if (r >= 0) delete _listAssignEle->takeItem(r);
            }
            };

        connect(editor, &QLineEdit::editingFinished, editor, [=]() {
            finishEditing();
            }, Qt::UniqueConnection);
    }

    void mPropertyCheckPanel::slot_reduceIdBtn()
    {
        QTime time;
        time.start();
        if (getCVM == nullptr)
        {
            return;
        }
        auto meshData = getCVM->getModelData()->getMeshData();
        mDataElement* elementData = getCVM->getModelData()->getElementData();
        QList<int> removedIds;  // 保存所有被删除的ID

        auto items = _listAssignEle->selectedItems();
        for (QListWidgetItem* item : items)
        {
            if (!item) continue;

            QString text = item->text().trimmed();
            if (!text.isEmpty()) {
                QList<int> ids = parseIdsFromText(text);
                removedIds.append(ids);
            }

            // 从 UI 中删除
            delete _listAssignEle->takeItem(_listAssignEle->row(item));
        }

        // 去重并排序
        std::sort(removedIds.begin(), removedIds.end());
        removedIds.erase(std::unique(removedIds.begin(), removedIds.end()), removedIds.end());
        if (!removedIds.isEmpty()) {

            std::vector<mxr::MeshSelectInfoData> eles;
            for (auto& ele : removedIds)
            {
                MeshSelectInfoData n;
                n.data_id.element_id = ele;
                int partId = meshData->getPartIdByEleId(ele);
                if (partId <= 0)
                {
                    continue;
                }
                n.part_id = partId;
                eles.push_back(n);
            }
            unHighlightEle(eles);
            qDebug() << "结束取消高亮" << time.elapsed();
            fillListWidget(collectIdsFromListWidget());
            fillEleTableWidget();
        }
    }

    void mPropertyCheckPanel::slot_clearIdBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        disconnectSig();
        getCVM->clearPick();
        connectSig();
        // 清空 UI
        _listAssignEle->clear();

        _tableEle->clearContents();
        _tableEle->setRowCount(0);

    }

    void mPropertyCheckPanel::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);
        if (_currentViewportPrivate == VMManager->getCurrentViewportPrivate())
        {
            return;
        }
        _currentViewportPrivate = VMManager->getCurrentViewportPrivate();
        connectSig();

    }
    void mPropertyCheckPanel::slot_finishedPick()
    {
        QTime time;
        time.start();
        if (getCVM == nullptr)
        {
            return;
        }
        auto meshData = getCVM->getModelData()->getMeshData();
        mDataElement* elementData = getCVM->getModelData()->getElementData();

        if (_checkType == 0)//部件
        {
            auto partIds = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
            if (partIds.size() > _lastPickPartData.size())
            {
                // 拾取数据比当前多 → 新增的部分
                std::vector<int> addedIds;
                QSet<int> currentSet;
                for (auto id : _lastPickPartData)
                {
                    currentSet.insert(id);
                }
                for (auto& e : partIds)
                {
                    if (!currentSet.contains(e))
                    {
                        addedIds.push_back(e);
                    }
                }
                highlightPart(addedIds);
            }
            else
            {
                // 拾取数据比当前少 → 减少的部分
                std::vector<int> reducedIds;
                QSet<int> selectedSet;
                for (auto& e : partIds)
                    selectedSet.insert(e);

                for (int id : _lastPickPartData)
                {
                    if (!selectedSet.contains(id))
                    {
                        reducedIds.push_back(id);
                    }
                }

                if (!reducedIds.empty()) {
                    //std::vector<mxr::MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getPreRender()->getMeshSelectInfoData()[SelectionInfoType::MeshElement];
                    qDebug() << "开始取消高亮" << time.elapsed();
                    unHighlightPart(reducedIds);
                }
            }
            _listAssignPart->clear();
            _tableProperty->clearContents();
            _tableProperty->setRowCount(0);
            //把所有属性都添加到部件中去
            for (int i = 0; i < partIds.size(); i++)
            {
                auto partName = meshData->getPartNameByPartId(partIds.at(i));
                _listAssignPart->addItem(partName);
                appendPartProperty(partName);
            }
        }
        else
        {
            auto eleIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
            QList<int> currentIds = collectIdsFromListWidget();
            std::vector<mxr::MeshSelectInfoData> res;
            if (eleIds.size() > _lastPickElementData.size())
            {
                // 拾取数据比当前多 → 新增的部分
                QList<int> allIds;
                QSet<int> currentSet;
                for (auto id : currentIds)
                {
                    currentSet.insert(id);
                }
                for (auto& e : eleIds)
                {
                    if (!currentSet.contains(e.data_id.element_id))
                    {
                        MeshSelectInfoData n;
                        n.data_id.element_id = e.data_id.element_id;
                        int partId = meshData->getPartIdByEleId(e.data_id.element_id);
                        if (partId <= 0)
                            continue;
                        n.part_id = partId;
                        res.push_back(n);
                    }
                    allIds.push_back(e.data_id.element_id);
                }
                highlightEle(res);

                fillListWidget(allIds);
            }
            else
            {
                // 拾取数据比当前少 → 减少的部分
                //QList<int> reducedIds;
                QSet<int> selectedSet;
                for (auto& e : eleIds)
                    selectedSet.insert(e.data_id.element_id);

                for (int id : currentIds)
                {
                    if (!selectedSet.contains(id))
                    {
                        MeshSelectInfoData n;
                        n.data_id.element_id = id;
                        int partId = meshData->getPartIdByEleId(id);
                        if (partId <= 0)
                            continue;
                        n.part_id = partId;
                        res.push_back(n);
                        //reducedIds.push_back(id);
                    }
                }

                unHighlightEle(res);

                fillListWidget(selectedSet.toList());
            }
            
            fillEleTableWidget();

        }
    }
    void mPropertyCheckPanel::connectSig()
    {
        disconnectSig();
        if (!getCVM)
        {
            return;
        }
        connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mPropertyCheckPanel::slot_subWindowActivated, Qt::UniqueConnection);
        conn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mPropertyCheckPanel::slot_finishedPick);
    }
    void mPropertyCheckPanel::disconnectSig()
    {
        disconnect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mPropertyCheckPanel::slot_subWindowActivated);
        if (conn)
            disconnect(conn);
    }
    void mPropertyCheckPanel::appendPartProperty(QString partName)
    {
        if (getCVM == nullptr)
        {
            return;
        }
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        mDataElement* elementData = getCVM->getModelData()->getElementData();
        int partId = meshData->getPartIdByPartName(partName);
        if (partId == -1)
        {
            return;
        }
        int propertyId = mapRelation.getMapRelation<int>({ Physical_Part, partId }, Physical_Property);
        QString propertyName = "";
        if (propertyId != -1)
        {
            auto propertyData = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property).getPropertyData(propertyId);
            if (propertyData != nullptr)
            {
                propertyName = propertyData->getPropertyName();
            }
        }

        _tableProperty->insertRow(0);

        _tableProperty->setItem(0, 0, new QTableWidgetItem(partName));
        if (propertyName.isEmpty())
        {
            auto item = new QTableWidgetItem("无");
            item->setBackground(QBrush(Qt::yellow));
            _tableProperty->setItem(0, 1, item);

        }
        else
        {
            _tableProperty->setItem(0, 1, new QTableWidgetItem(propertyName));
        }

        //获取一个部件所有的单元Id
        std::unordered_set<ElementType> eleTypes;
        auto eleIds = meshData->getEleIdsByPartId(partId);
        for (auto eleId : eleIds)
        {
            auto eleType = elementData->getElementTypeByMeshID(eleId);
            if (eleType == NoneElement)
            {
                continue;
            }
            eleTypes.insert(eleType);
        }
        // 把所有的单元类型转成字符串然后用逗号连接
        QStringList typeStrings;
        for (const auto& type : eleTypes)
        {
            typeStrings << mElementFunction::getElementString(type);
        }

        QString eleTypeStrs = typeStrings.join(", ");

        //把所有的单元类型转成字符串然后用逗号连接填入单元格中
        _tableProperty->setItem(0, 2, new QTableWidgetItem(eleTypeStrs));
    }

    void mPropertyCheckPanel::fillEleTableWidget()
    {
        auto eleIds = collectIdsFromListWidget();

        //清空表格
        _tableEle->clearContents();
        _tableEle->setRowCount(0);

        if (eleIds.empty())
            return;

        //根据eleIds填充QTableWidgetItem

        //网格Id连续或者间断：由Id和单元类型共同决定
        auto meshData = getCVM->getModelData()->getMeshData();
        auto elementData = getCVM->getModelData()->getElementData();

        // 按单元类型分组
        std::unordered_map<ElementType, std::vector<int>> typeToIds;
        for (int id : eleIds)
        {
            ElementType type = elementData->getElementTypeByMeshID(id);
            if (type == NoneElement)
                continue;
            typeToIds[type].push_back(id);
        }

        int row = 0;
        for (auto it = typeToIds.begin(); it != typeToIds.end(); ++it)
        {
            ElementType type = it->first;
            auto& ids = it->second;

            if (ids.empty())
                continue;

            // 排序以识别连续段
            std::sort(ids.begin(), ids.end());

            // 拆分为连续段
            int start = ids[0], prev = ids[0];
            for (size_t i = 1; i <= ids.size(); ++i)
            {
                bool isBreak = (i == ids.size()) || (ids[i] != prev + 1);
                if (isBreak)
                {
                    QString idText;
                    if (start == prev)
                        idText = MxFunctions::mNumberToString(start);
                    else
                        idText = QString("%1-%2").arg(start).arg(prev);

                    QString typeText = mElementFunction::getElementString(type);

                    // 插入表格行
                    _tableEle->insertRow(row);
                    _tableEle->setItem(row, 0, new QTableWidgetItem(idText));
                    _tableEle->setItem(row, 1, new QTableWidgetItem(typeText));
                    ++row;

                    if (i < ids.size())
                        start = ids[i];
                }
                if (i < ids.size())
                    prev = ids[i];
            }
        }
    }

    void mPropertyCheckPanel::fillListWidget(const QList<int>& ids)
    {
        _listAssignEle->clear();
        if (ids.isEmpty())
            return;

        // 1 拷贝并排序
        QList<int> sortedIds = ids;
        std::sort(sortedIds.begin(), sortedIds.end());

        // 2 遍历并合并连续段
        QList<QPair<int, int>> ranges;
        int start = sortedIds.first();
        int prev = start;

        for (int i = 1; i < sortedIds.size(); ++i) {
            int curr = sortedIds[i];
            if (curr == prev + 1) {
                // 连续
                prev = curr;
            }
            else {
                // 断开，保存区间
                ranges.append({ start, prev });
                start = prev = curr;
            }
        }
        // 最后一段
        ranges.append({ start, prev });

        // 3 添加到 QListWidget
        for (auto& range : ranges) {
            QString text;
            if (range.first == range.second)
                text = MxFunctions::mNumberToString(range.first);
            else
                text = QString("%1-%2").arg(range.first).arg(range.second);

            _listAssignEle->addItem(new QListWidgetItem(text));
        }
    }

    QList<int> mPropertyCheckPanel::collectIdsFromListWidget()
    {
        QSet<int> uniqueIds;  // 使用 QSet 去重

        for (int i = 0; i < _listAssignEle->count(); ++i)
        {
            QString text = _listAssignEle->item(i)->text().trimmed();
            if (text.isEmpty())
                continue;

            // 支持 "19-20" 或 "8"
            if (text.contains('-'))
            {
                QStringList parts = text.split('-', QString::SkipEmptyParts);
                if (parts.size() == 2)
                {
                    bool ok1, ok2;
                    int start = parts[0].trimmed().toInt(&ok1);
                    int end = parts[1].trimmed().toInt(&ok2);
                    if (ok1 && ok2)
                    {
                        if (start > end) std::swap(start, end); // 自动纠正顺序
                        for (int id = start; id <= end; ++id)
                            uniqueIds.insert(id);
                    }
                }
            }
            else
            {
                bool ok;
                int id = text.toInt(&ok);
                if (ok)
                    uniqueIds.insert(id);
            }
        }

        // 转换为排序后的 QList
        QList<int> result = uniqueIds.values();
        std::sort(result.begin(), result.end());
        return result;

    }

    QList<int> mPropertyCheckPanel::parseIdsFromText(const QString& text)
    {
        QList<int> ids;
        QString trimmed = text.trimmed();
        if (trimmed.isEmpty())
            return ids;

        if (trimmed.contains('-')) {
            QStringList parts = trimmed.split('-', QString::SkipEmptyParts);
            if (parts.size() == 2) {
                bool ok1, ok2;
                int start = parts[0].trimmed().toInt(&ok1);
                int end = parts[1].trimmed().toInt(&ok2);
                if (ok1 && ok2) {
                    if (start > end) std::swap(start, end);
                    for (int i = start; i <= end; ++i)
                        ids.append(i);
                }
            }
        }
        else {
            bool ok;
            int id = trimmed.toInt(&ok);
            if (ok)
                ids.append(id);
        }
        return ids;
    }

    void mPropertyCheckPanel::highlightPart(std::vector<int> ids)
    {
        if (conn)
            disconnect(conn);
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
        addPreWidgetSelectPartID(getCVM->getPreRenderWidget(), ids);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

        getCVM->getPreRenderWidget()->updatePaint();

        conn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mPropertyCheckPanel::slot_finishedPick);

        _lastPickPartData = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
    }
    void mPropertyCheckPanel::unHighlightPart(std::vector<int> ids)
    {
        if (conn)
            disconnect(conn);
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
        reducePreWidgetSelectPartID(getCVM->getPreRenderWidget(), ids);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

        getCVM->getPreRenderWidget()->updatePaint();

        conn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mPropertyCheckPanel::slot_finishedPick);

        _lastPickPartData = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
    }

    void mPropertyCheckPanel::highlightEle(std::vector<mxr::MeshSelectInfoData> ids)
    {
        if (conn)
            disconnect(conn);
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, ids);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

        getCVM->getPreRenderWidget()->updatePaint();

        conn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mPropertyCheckPanel::slot_finishedPick);

        _lastPickElementData = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
    }
    void mPropertyCheckPanel::unHighlightEle(std::vector<mxr::MeshSelectInfoData> ids)
    {
        if (conn)
            disconnect(conn);
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::REDUCE, SelectionInfoType::MeshElement, ids);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

        getCVM->getPreRenderWidget()->updatePaint();
        conn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mPropertyCheckPanel::slot_finishedPick);

        _lastPickElementData = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
    }

}