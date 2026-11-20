#include "mCasePeakFilterPanel.h"

#include "mInfoWindow.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mGlobalParam.h"
#include "mDockLayoutManager.h"
#include "mCaseManager.h"
#include "mCaseImplicitFrequency.h"
#include "mCaseBase.h"

#include <QCompleter>
#include <QTimer>
#include <QPointer>
#include <QGridLayout>
#include <QToolButton>
#include <functional>
#include <algorithm>
#include <QStringListModel>

using namespace MechCommon;
using namespace MechCaseData;
namespace MechMainWindow
{
    mCasePeakFilterPanel::mCasePeakFilterPanel(QWidget* parent)
        : mPanelBase(parent), _caseId(-1), _listPeakFilter(nullptr), _btnCreatePeakFilter(nullptr)
    {
        _mainLayout = new QVBoxLayout(this);

        FoldSection* peakFilterFold = new FoldSection("峰值筛选", this);

        // ===================== 列表 + 按钮 =====================
        QWidget* container = new QWidget(peakFilterFold);
        QGridLayout* listLayout = new QGridLayout(container);
        _listPeakFilter = new QListWidget(this);
        _listPeakFilter->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _listPeakFilter->setMinimumHeight(600);
        _listPeakFilter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QToolButton* addBtn = new QToolButton();
        addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        addBtn->setToolTip("增加");
        QToolButton* reduceBtn = new QToolButton();
        reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        reduceBtn->setToolTip("减少");
        QToolButton* deleteBtn = new QToolButton();
        deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
        deleteBtn->setToolTip("清空");

        _btnCreatePeakFilter = new QToolButton(this);
        _btnCreatePeakFilter->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/peakfilter.png"));
        _btnCreatePeakFilter->setToolTip("创建峰值筛选");

        listLayout->addWidget(_listPeakFilter, 0, 0, 8, 7);
        listLayout->addWidget(addBtn, 0, 7);
        listLayout->addWidget(reduceBtn, 1, 7);
        listLayout->addWidget(deleteBtn, 2, 7);
        listLayout->addWidget(_btnCreatePeakFilter, 3, 7);

        // 按钮连接
        connect(addBtn, &QToolButton::clicked, this, &mCasePeakFilterPanel::slot_addPeakFilterBtn);
        connect(reduceBtn, &QToolButton::clicked, this, &mCasePeakFilterPanel::slot_reducePeakFilterBtn);
        connect(deleteBtn, &QToolButton::clicked, this, &mCasePeakFilterPanel::slot_clearPeakFilterBtn);
        connect(_btnCreatePeakFilter, &QToolButton::clicked, this, &mCasePeakFilterPanel::slot_createPeakFilterClicked);

        peakFilterFold->addRow(container);

        m_foldWidgets.append(peakFilterFold);
        _mainLayout->addWidget(peakFilterFold);
        _mainLayout->addStretch();
    }

    mCasePeakFilterPanel::~mCasePeakFilterPanel()
    {
    }

    void mCasePeakFilterPanel::createCasePeakFilter(int caseId)
    {
        _caseId = caseId;
        loadListFromMapRelation();
        emit mMechGlobalSignals::getInstance()->createUpdateCasePeakFilterSig(true, _caseId);
    }

    void mCasePeakFilterPanel::editCasePeakFilter(int caseId)
    {
        _caseId = caseId;
        loadListFromMapRelation();
    }

    void mCasePeakFilterPanel::refreshList()
    {
        if (_caseId > 0)
        {
            loadListFromMapRelation();
        }
    }

    void mCasePeakFilterPanel::slot_createPeakFilterClicked()
    {
        if (!getCVM || _caseId <= 0) return;

        // 在模型树下创建新的全局峰值筛选
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        int newId = globalParam->createPeakFilterID();
        QString name = QString("PeakOut_%1").arg(newId);
        mPeakFilterData* newData = new mPeakFilterData(newId, name);
        globalParam->addPeakFilterData(newData);

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建峰值筛选"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePeakFilterSig(true, newId);

        // 将新创建的峰值筛选自动关联到当前工况
        auto& mgr = getCVM->getModelData()->getMapRelationManager();
        mgr.appendTableMapRelation({ Physical_Case, _caseId }, { Physical_PeakFilter, newId }, _listPeakFilter->count());

        // 刷新列表
        loadListFromMapRelation();
    }

    void mCasePeakFilterPanel::loadListFromMapRelation()
    {
        if (!_listPeakFilter) return;
        _listPeakFilter->clear();

        if (_caseId <= 0 || !getCVM) return;

        QList<int> pfIds = getCVM->getModelData()->getMapRelationManager()
            .getMapRelation<QList<int>>({ Physical_Case, _caseId }, Physical_PeakFilter);

        auto globalParam = getCVM->getModelData()->getGlobalParam();

        for (int pfId : pfIds)
        {
            QString name = globalParam->getPeakFilterNameByID(pfId);
            if (!name.isEmpty())
            {
                _listPeakFilter->addItem(name);
            }
        }
    }


    void mCasePeakFilterPanel::slot_addPeakFilterBtn()
    {
        if (!getCVM || _caseId <= 0) return;

        // 获取所有已有峰值筛选名称作为候选
        QStringList candidateList = getCVM->getModelData()->getGlobalParam()->getAllPeakFilterNames();

        QStringListModel* tempModel = new QStringListModel(this);
        tempModel->setStringList(candidateList);

        QListWidgetItem* newItem = new QListWidgetItem();
        _listPeakFilter->addItem(newItem);

        QLineEdit* editor = new QLineEdit(_listPeakFilter);
        _listPeakFilter->setItemWidget(newItem, editor);
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
                int r = _listPeakFilter->row(newItem);
                if (r >= 0) delete _listPeakFilter->takeItem(r);
                cleanUp();
                return;
            }

            // 获取当前行号
            int row = _listPeakFilter->row(newItem);
            if (row < 0) {
                cleanUp();
                return;
            }

            _listPeakFilter->removeItemWidget(newItem);
            QString text = editorGuard->text().trimmed();

            // 空值删除
            if (text.isEmpty()) {
                delete _listPeakFilter->takeItem(row);
                cleanUp();
                return;
            }

            // 不在候选列表删除
            if (!tempModel->stringList().contains(text)) {
                delete _listPeakFilter->takeItem(row);
                cleanUp();
                return;
            }

            // 去重
            bool exists = false;
            for (int i = 0; i < _listPeakFilter->count(); ++i)
            {
                QListWidgetItem* it = _listPeakFilter->item(i);
                if (it == newItem) continue;
                if (it && it->text() == text) {
                    exists = true;
                    break;
                }
            }
            if (exists)
            {
                delete _listPeakFilter->takeItem(row);
                cleanUp();
                return;
            }

            newItem->setText(text);

            // 表格映射
            int pfId = getCVM->getModelData()->getGlobalParam()->getPeakFilterIDByName(text);
            if (pfId > 0)
            {
                getCVM->getModelData()->getMapRelationManager()
                    .appendTableMapRelation(
                        { Physical_Case, _caseId },
                        { Physical_PeakFilter, pfId },
                        row
                    );
            }

            cleanUp();
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

    void mCasePeakFilterPanel::slot_reducePeakFilterBtn()
    {
        if (!getCVM || _caseId <= 0) return;

        auto selectedItems = _listPeakFilter->selectedItems();
        if (selectedItems.isEmpty()) return;

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();

        for (QListWidgetItem* item : selectedItems)
        {
            if (!item) continue;
            int row = _listPeakFilter->row(item);
            if (row < 0) continue;

            // 删除表格映射
            mapRelation.deleteTabelMapRelation(
                { Physical_Case, _caseId },
                { Physical_PeakFilter, -1 },
                row
            );

            // 删除UI行
            delete _listPeakFilter->takeItem(row);
        }

    }

    void mCasePeakFilterPanel::slot_clearPeakFilterBtn()
    {
        if (!getCVM || _caseId <= 0) return;

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int totalRow = _listPeakFilter->count();

        // 倒序删除（从最后一行删到第0行，保证行号不乱）
        for (int row = totalRow - 1; row >= 0; --row)
        {
            QListWidgetItem* item = _listPeakFilter->item(row);
            if (!item) continue;

            mapRelation.deleteTabelMapRelation(
                { Physical_Case, _caseId },
                { Physical_PeakFilter, -1 },
                row
            );
        }

        // UI全部清空
        _listPeakFilter->clear();
    }
}
