#include "mNsmManagerUnderCasePanel.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mMapRelationManager.h"

//QT
#include <QCompleter>

namespace MechMainWindow
{
    mNsmManagerUnderCasePanel::mNsmManagerUnderCasePanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _nsmManagerData = new MechCaseData::mNsmUnderCaseData;
        _nsmModel = new QStringListModel;

        //获取当前视口的数据
        _mainLayout = new QVBoxLayout(this);

        FoldSection* nsmFold = new FoldSection("非结构质量", this);

        _signLabel = new QLabel("选择非结构质量");
        nsmFold->addRow(_signLabel);

        QGridLayout* assignLayout = new QGridLayout();
        _listNsm = new QListWidget();
        _listNsm->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _allBtn = new QToolButton();
        _allBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png"));
        _allBtn->setToolTip("全部非结构质量");

        _addBtn = new QToolButton();
        _addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        _addBtn->setToolTip("增加非结构质量");
        _reduceBtn = new QToolButton();
        _reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        _reduceBtn->setToolTip("减少非结构质量");
        _cleanBtn = new QToolButton();
        _cleanBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
        _cleanBtn->setToolTip("清空非结构质量");

        assignLayout->addWidget(_listNsm, 0, 0, 8, 7);
        assignLayout->addWidget(_allBtn, 0, 7);
        assignLayout->addWidget(_addBtn, 1, 7);
        assignLayout->addWidget(_reduceBtn, 2, 7);
        assignLayout->addWidget(_cleanBtn, 3, 7);
        nsmFold->addRow(assignLayout);

        connect(_allBtn, &QToolButton::clicked, this, &mNsmManagerUnderCasePanel::slot_allNsmBtn);
        connect(_addBtn, &QToolButton::clicked, this, &mNsmManagerUnderCasePanel::slot_addNsmBtn);
        connect(_reduceBtn, &QToolButton::clicked, this, &mNsmManagerUnderCasePanel::slot_reduceBtn);
        connect(_cleanBtn, &QToolButton::clicked, this, &mNsmManagerUnderCasePanel::slot_cleanBtn);

        m_foldWidgets.append(nsmFold);
        _mainLayout->addWidget(nsmFold);

        _mainLayout->addStretch();
    }

    mNsmManagerUnderCasePanel::~mNsmManagerUnderCasePanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }
    }

    void mNsmManagerUnderCasePanel::editNsmManagerUnderCasePanel(int parentCaseId)
    {
        _parentCaseId = parentCaseId;
        if (getCVM == nullptr)
        {
            return;
        }

        QStringList nsmList = getCVM->getModelData()
            ->getManager<MechCaseData::mNonStructQualityManager>(Physical_NonStructuralQuality)
            .getAllNsmNames();

        //根据所有的维护数据里的

        //从映射中获取
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        QList<int> nsmIds = mapRelation.getMapRelation<QList<int>>({ Physical_Case, _parentCaseId }, Physical_NonStructuralQuality);


        _listNsm->clear();
        QStringList target;
        for (auto id : nsmIds)
        {
            QString nsmName = getCVM->getModelData()->getNameByPhyID(Physical_NonStructuralQuality, id);
            if (!nsmName.isEmpty())
            {
                _listNsm->addItem(nsmName);
                target.append(nsmName);
            }

        }
        _nsmManagerData->setNsmStrs(target);
 

    }

    void mNsmManagerUnderCasePanel::slot_allNsmBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }

        //获取所有非结构质量名称
        QStringList nsmList = getCVM->getModelData()
            ->getManager<MechCaseData::mNonStructQualityManager>(Physical_NonStructuralQuality).getAllNsmNames();

        _listNsm->clear();
        _listNsm->addItems(nsmList);

        //更新数据类
        _nsmManagerData->setNsmStrs(nsmList);

        //更新映射
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        QList<int> nsmIds = mapRelation.getMapRelation<QList<int>>({ Physical_Case, _parentCaseId }, Physical_NonStructuralQuality);

        while (nsmIds.size() > 0)
        {
            mapRelation.deleteTabelMapRelation({ Physical_Case, _parentCaseId }, { Physical_NonStructuralQuality, -1 }, 0);
            nsmIds = mapRelation.getMapRelation<QList<int>>({ Physical_Case, _parentCaseId }, Physical_NonStructuralQuality);
        }
        for (int i = 0; i < nsmList.size(); i++)
        {
            QString nsmName = nsmList.at(i);
            int nsmId = getCVM->getModelData()->getPhyIDByName(Physical_NonStructuralQuality, nsmName);
            mapRelation.appendTableMapRelation({ Physical_Case, _parentCaseId }, { Physical_NonStructuralQuality, nsmId }, i);
        }

    }

    void mNsmManagerUnderCasePanel::slot_addNsmBtn()
    {
        if (getCVM == nullptr) return;

        QStringList nsmList = getCVM->getModelData()
            ->getManager<MechCaseData::mNonStructQualityManager>(Physical_NonStructuralQuality).getAllNsmNames();
        _nsmModel->setStringList(nsmList);

        QListWidgetItem* newItem = new QListWidgetItem();
        _listNsm->insertItem(0, newItem);

        QLineEdit* editor = new QLineEdit(_listNsm);
        _listNsm->setItemWidget(newItem, editor);
        editor->setFocus();

        QCompleter* completer = new QCompleter(_nsmModel, editor);
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
                    int r = _listNsm->row(newItem);
                    if (r >= 0)
                    {
                        delete _listNsm->takeItem(r);
                    }
                    return;
                }

                int row = _listNsm->row(newItem);
                if (row < 0)
                {
                    editorGuard->deleteLater();
                    return;
                }

                _listNsm->removeItemWidget(newItem);

                QString text = editorGuard->text().trimmed();

                if (text.isEmpty())
                {
                    delete _listNsm->takeItem(row);
                    editorGuard->deleteLater();
                    return;
                }

                QStringList candidates = _nsmModel->stringList();
                if (!candidates.contains(text))
                {
                    delete _listNsm->takeItem(row);
                    editorGuard->deleteLater();
                    return;
                }

                bool exists = false;
                for (int i = 0; i < _listNsm->count(); ++i)
                {
                    QListWidgetItem* it = _listNsm->item(i);
                    if (!it) continue;
                    if (it == newItem) continue;
                    if (it->text() == text) { exists = true; break; }
                }
                if (exists)
                {
                    delete _listNsm->takeItem(row);
                    editorGuard->deleteLater();
                    return;
                }

                newItem->setText(text);
                _nsmManagerData->appendNsm(text);

                //更新映射
                auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
                int nsmId = getCVM->getModelData()->getPhyIDByName(Physical_NonStructuralQuality, text);
                mapRelation.appendTableMapRelation({ Physical_Case, _parentCaseId }, { Physical_NonStructuralQuality, nsmId }, row);

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

    void mNsmManagerUnderCasePanel::slot_reduceBtn()
    {
        if (getCVM == nullptr) return;

        auto items = _listNsm->selectedItems();
        for (QListWidgetItem* item : items)
        {
            if (!item) continue;
            int row = _listNsm->row(item);
            QString text = item->text().trimmed();
            if (!text.isEmpty())
            {
                _nsmManagerData->removeNsm(text);

                //更新映射
                auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
                mapRelation.deleteTabelMapRelation({ Physical_Case, _parentCaseId }, { Physical_NonStructuralQuality, -1 }, row);
            }
            //从列表中删除
            delete _listNsm->takeItem(_listNsm->row(item));
        }
    }
    void mNsmManagerUnderCasePanel::slot_cleanBtn()
    {
        _listNsm->clear();
        QStringList emptyStrs;
        _nsmManagerData->setNsmStrs(emptyStrs);

        //更新映射
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        QList<int> nsmIds = mapRelation.getMapRelation<QList<int>>({ Physical_Case, _parentCaseId}, Physical_NonStructuralQuality);
        
        while (nsmIds.size() > 0)
        {
            mapRelation.deleteTabelMapRelation({ Physical_Case, _parentCaseId }, {Physical_NonStructuralQuality, -1}, 0);
            nsmIds = mapRelation.getMapRelation<QList<int>>({ Physical_Case, _parentCaseId }, Physical_NonStructuralQuality);
        }
    }


}