#include "mModelFixManaUnderCasePanel.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mMapRelationManager.h"

#include "mModelFixManager.h"
#include "mModelFixDataBase.h"

#include "mDataStatusManager.h"

//QT
#include <QCompleter>

using namespace MechModelFixData;
namespace MechMainWindow
{
    mModelFixManaUnderCasePanel::mModelFixManaUnderCasePanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _modelFixModel = new QStringListModel;

        //获取当前视口的数据
        _mainLayout = new QVBoxLayout(this);

        FoldSection* modelFixFold = new FoldSection("模型修改", this);

        _signLabel = new QLabel("选择模型修改");
        modelFixFold->addRow(_signLabel);

        QGridLayout* assignLayout = new QGridLayout();
        _listModelFix = new QListWidget();
        _listModelFix->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _allBtn = new QToolButton();
        _allBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png"));
        _allBtn->setToolTip("全部模型修改");

        _addBtn = new QToolButton();
        _addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        _addBtn->setToolTip("增加模型修改");
        _reduceBtn = new QToolButton();
        _reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        _reduceBtn->setToolTip("减少模型修改");
        _cleanBtn = new QToolButton();
        _cleanBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
        _cleanBtn->setToolTip("清空模型修改");

        assignLayout->addWidget(_listModelFix, 0, 0, 8, 7);
        assignLayout->addWidget(_allBtn, 0, 7);
        assignLayout->addWidget(_addBtn, 1, 7);
        assignLayout->addWidget(_reduceBtn, 2, 7);
        assignLayout->addWidget(_cleanBtn, 3, 7);
        modelFixFold->addRow(assignLayout);

        connect(_allBtn, &QToolButton::clicked, this, &mModelFixManaUnderCasePanel::slot_allModelFixBtn);
        connect(_addBtn, &QToolButton::clicked, this, &mModelFixManaUnderCasePanel::slot_addModelFixBtn);
        connect(_reduceBtn, &QToolButton::clicked, this, &mModelFixManaUnderCasePanel::slot_reduceBtn);
        connect(_cleanBtn, &QToolButton::clicked, this, &mModelFixManaUnderCasePanel::slot_cleanBtn);

        m_foldWidgets.append(modelFixFold);
        _mainLayout->addWidget(modelFixFold);

        _mainLayout->addStretch();
    }

    mModelFixManaUnderCasePanel::~mModelFixManaUnderCasePanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }
    }

    void mModelFixManaUnderCasePanel::editModelFixManagerUnderCasePanel(int parentCaseId)
    {
        _parentCaseId = parentCaseId;
        if (getCVM == nullptr)
        {
            return;
        }

        QStringList modelFixList = getCVM->getModelData()
            ->getManager<mModelFixManager>(Physical_ModelFix)
            .getAllModelFixNames();

        //从映射中获取
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        QList<int> modelFixIds = mapRelation.getMapRelation<QList<int>>({ Physical_Case, _parentCaseId }, Physical_ModelFix);

        _listModelFix->clear();
        for (auto id : modelFixIds)
        {
            QString modelFixName = getCVM->getModelData()->getNameByPhyID(Physical_ModelFix, id);
            if (!modelFixName.isEmpty())
            {
                _listModelFix->addItem(modelFixName);
            }
        }
    }

    void mModelFixManaUnderCasePanel::slot_allModelFixBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }

        //获取所有模型修改名称
        QStringList modelFixList = getCVM->getModelData()
            ->getManager<mModelFixManager>(Physical_ModelFix).getAllModelFixNames();

        _listModelFix->clear();
        _listModelFix->addItems(modelFixList);

        //更新映射
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        QList<int> modelFixIds = mapRelation.getMapRelation<QList<int>>({ Physical_Case, _parentCaseId }, Physical_ModelFix);

        while (modelFixIds.size() > 0)
        {
            mapRelation.deleteTabelMapRelation({ Physical_Case, _parentCaseId }, { Physical_ModelFix, -1 }, 0);
            modelFixIds = mapRelation.getMapRelation<QList<int>>({ Physical_Case, _parentCaseId }, Physical_ModelFix);
        }
        for (int i = 0; i < modelFixList.size(); i++)
        {
            QString modelFixName = modelFixList.at(i);
            int modelFixId = getCVM->getModelData()->getPhyIDByName(Physical_ModelFix, modelFixName);
            mapRelation.appendTableMapRelation({ Physical_Case, _parentCaseId }, { Physical_ModelFix, modelFixId }, i);
        }

        // 发送状态同步信号
        for (const QString& modelFixName : modelFixList)
        {
            int id = getCVM->getModelData()->getPhyIDByName(Physical_ModelFix, modelFixName);
            if (id >= 0)
            {
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ModelFix, id);
            }
        }
    }

    void mModelFixManaUnderCasePanel::slot_addModelFixBtn()
    {
        if (getCVM == nullptr) return;

        QStringList modelFixList = getCVM->getModelData()
            ->getManager<mModelFixManager>(Physical_ModelFix).getAllModelFixNames();
        _modelFixModel->setStringList(modelFixList);

        QListWidgetItem* newItem = new QListWidgetItem();
        _listModelFix->insertItem(0, newItem);

        QLineEdit* editor = new QLineEdit(_listModelFix);
        _listModelFix->setItemWidget(newItem, editor);
        editor->setFocus();

        QCompleter* completer = new QCompleter(_modelFixModel, editor);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        editor->setCompleter(completer);

        completer->complete();

        QPointer<QLineEdit> editorGuard(editor);
        // finishFlag 防止 finishEditing 被重复执行
        auto finishFlag = std::make_shared<bool>(false);

        std::function<void()> finishEditing = [=]() mutable
            {
                if (*finishFlag) return;
                *finishFlag = true;

                if (!editorGuard)
                {
                    int r = _listModelFix->row(newItem);
                    if (r >= 0)
                    {
                        delete _listModelFix->takeItem(r);
                    }
                    return;
                }

                int row = _listModelFix->row(newItem);
                if (row < 0)
                {
                    editorGuard->deleteLater();
                    return;
                }

                _listModelFix->removeItemWidget(newItem);

                QString text = editorGuard->text().trimmed();

                if (text.isEmpty())
                {
                    delete _listModelFix->takeItem(row);
                    editorGuard->deleteLater();
                    return;
                }

                QStringList candidates = _modelFixModel->stringList();
                if (!candidates.contains(text))
                {
                    delete _listModelFix->takeItem(row);
                    editorGuard->deleteLater();
                    return;
                }

                bool exists = false;
                for (int i = 0; i < _listModelFix->count(); ++i)
                {
                    QListWidgetItem* it = _listModelFix->item(i);
                    if (!it) continue;
                    if (it == newItem) continue;
                    if (it->text() == text) { exists = true; break; }
                }
                if (exists)
                {
                    delete _listModelFix->takeItem(row);
                    editorGuard->deleteLater();
                    return;
                }

                newItem->setText(text);

                //更新映射
                auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
                int modelFixId = getCVM->getModelData()->getPhyIDByName(Physical_ModelFix, text);
                mapRelation.appendTableMapRelation({ Physical_Case, _parentCaseId }, { Physical_ModelFix, modelFixId }, row);

                // 发送状态同步信号
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ModelFix, modelFixId);

                editorGuard->deleteLater();
            };
        // completer激活时（鼠标点击/回车选项） - 使用editor作为receiver（自动断开在editor销毁时）
        connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
            editor,  // receiver
            [=](const QString& choice)
            {
                if (!editorGuard) return;
                editorGuard->setText(choice);
                finishEditing();
            }, Qt::UniqueConnection);
        connect(editor, &QLineEdit::editingFinished, editor, [=]() {
            finishEditing();
            }, Qt::UniqueConnection);
    }

    void mModelFixManaUnderCasePanel::slot_reduceBtn()
    {
        if (getCVM == nullptr) return;

        auto items = _listModelFix->selectedItems();
        for (QListWidgetItem* item : items)
        {
            if (!item) continue;
            int row = _listModelFix->row(item);
            QString text = item->text().trimmed();
            if (!text.isEmpty())
            {
                // 发送状态同步信号
                int id = getCVM->getModelData()->getPhyIDByName(Physical_ModelFix, text);
                if (id >= 0)
                {
                    getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ModelFix, id);
                }

                //更新映射
                auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
                mapRelation.deleteTabelMapRelation({ Physical_Case, _parentCaseId }, { Physical_ModelFix, -1 }, row);
            }
            //从列表中删除
            delete _listModelFix->takeItem(_listModelFix->row(item));
        }
    }
    void mModelFixManaUnderCasePanel::slot_cleanBtn()
    {
        // 发送状态同步信号（在清空列表前记录所有关联的模型修改ID）
        if (getCVM)
        {
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            QList<int> modelFixIds = mapRelation.getMapRelation<QList<int>>({ Physical_Case, _parentCaseId }, Physical_ModelFix);
            for (int id : modelFixIds)
            {
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ModelFix, id);
            }
        }

        _listModelFix->clear();

        //更新映射
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        QList<int> modelFixIds = mapRelation.getMapRelation<QList<int>>({ Physical_Case, _parentCaseId }, Physical_ModelFix);

        while (modelFixIds.size() > 0)
        {
            mapRelation.deleteTabelMapRelation({ Physical_Case, _parentCaseId }, { Physical_ModelFix, -1 }, 0);
            modelFixIds = mapRelation.getMapRelation<QList<int>>({ Physical_Case, _parentCaseId }, Physical_ModelFix);
        }
    }

}
