
#include "mGenericPartPanel.h"

//Qt
#include <QLabel>
#include <QDir>
#include <QFileDialog>
#include <QApplication>
#include <QCompleter>

//Function
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"
#include "mMeshData.h"
#include "PickCommonFunction.h"
#include "mxr/view/SelectionCaculator.h"
#include "mxr/view/CCSMRendWidget.h"

//MechModelData
#include "mGenericPartData.h"
#include "mGlobalParam.h"

using namespace MechPreData;
using namespace MxFunctions;
using namespace MechCommon;
using namespace MechGlobalParamData;
namespace MechMainWindow
{
    mGenericPartPanel::mGenericPartPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        _tempModel = new QStringListModel(this);
        // ========== 超单元部件设置折叠块 ==========
        auto defineFold = new FoldSection("超单元部件设置", this);

        QGridLayout* assignLayout = new QGridLayout(this);
        _listGenericPart = new QListWidget(this);
        _listGenericPart->setSelectionMode(QAbstractItemView::ExtendedSelection);
        QToolButton* allBtn = new QToolButton(); allBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png")); allBtn->setToolTip("全部部件");
        QToolButton* addBtn = new QToolButton(); addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); addBtn->setToolTip("增加部件");
        QToolButton* reduceBtn = new QToolButton(); reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); reduceBtn->setToolTip("减少部件");
        QToolButton* deleteBtn = new QToolButton(); deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); deleteBtn->setToolTip("清空部件");

        connect(allBtn, &QToolButton::clicked, this, &mGenericPartPanel::slot_allPartBtn);
        connect(addBtn, &QToolButton::clicked, this, &mGenericPartPanel::slot_addPartBtn);
        connect(reduceBtn, &QToolButton::clicked, this, &mGenericPartPanel::slot_reducePartBtn);
        connect(deleteBtn, &QToolButton::clicked, this, &mGenericPartPanel::slot_clearPartBtn);

        assignLayout->addWidget(_listGenericPart, 0, 0, 20, 7);
        assignLayout->addWidget(allBtn, 0, 7);
        assignLayout->addWidget(addBtn, 1, 7);
        assignLayout->addWidget(reduceBtn, 2, 7);
        assignLayout->addWidget(deleteBtn, 3, 7);

        defineFold->addRow(assignLayout);

        m_foldWidgets.append(defineFold);
        _mainLayout->addWidget(defineFold);

        // ========== 超单元部件创建折叠块 ==========
        auto casePartFold = new FoldSection("超单元部件创建", this);

        // 添加到折叠块
        QLineEdit* line = new QLineEdit(this);
        casePartFold->addRow(new PanelLabel("新部件名："), line);
        QPushButton* pb = new QPushButton("创建部件", this);
        pb->setToolTip("通过拾取单元，创建新部件");
        casePartFold->addRow(new PanelLabel(""), pb);
        connect(pb, &QPushButton::clicked, this, [this, line]() {
             if (line->text().isEmpty())
             {
                 mGlobalSignals::getInstance()->outputMessageSig(1, QString("请输入部件名！"));
                 return;
             }
             std::vector<mxr::MeshSelectInfoData> pickData = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
             if (pickData.empty())
             {
                 mGlobalSignals::getInstance()->outputMessageSig(1, QString("请拾取单元！"));
                 return;
             }
             vector<RenderPickResultData> elements;
             for (auto meshelem : pickData)
             {
                 elements.push_back(SelectData2ResultData(SelectionInfoType::MeshElement, meshelem));
             }
             int partID = getCVM->getModelData()->getMeshData()->createPartByElements(line->text(), elements);
             getCVM->clearPick();

             //添加映射
             QString partName = getCVM->getModelData()->getNameByPhyID(Physical_Part, partID);
             QListWidgetItem* newItem = new QListWidgetItem(partName);
             _listGenericPart->addItem(newItem);
             int row = _listGenericPart->row(newItem);
             getCVM->getModelData()->getMapRelationManager()
                 .appendTableMapRelation(
                     { Physical_GenericPart,  getCVM->getModelData()->getModelDataID() },
                     { Physical_Part, partID },
                     row
                 );

             mGlobalSignals::getInstance()->createUpdatePartSig(true, {partID});
             });

        m_foldWidgets.append(casePartFold);
        _mainLayout->addWidget(casePartFold);


        _mainLayout->addStretch();
    }

    mGenericPartPanel::~mGenericPartPanel()
    {
    }

    void mGenericPartPanel::createGenericPart()
    {
        if (!getCVM) return;
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        if (globalParam->getGenericPartData())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个超单元部件节点！"));
            return;
        }

        globalParam->createGenericPartData();
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建超单元部件"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateGenericPartSig(true);
    }

    void mGenericPartPanel::editGenericPartPanel()
    {
        if (_genericPartData == getCVM->getModelData()->getGlobalParam()->getGenericPartData())
            return;

        _genericPartData = getCVM->getModelData()->getGlobalParam()->getGenericPartData();

        int modelID = getCVM->getModelData()->getModelDataID();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        QList<int> partIDs = mapRelation.getMapRelation<QList<int>>({ Physical_GenericPart, modelID }, Physical_Part);

        QStringList partList;
        for (int row = 0; row < partIDs.size(); row++)
        {
            QString name = getCVM->getModelData()->getNameByPhyID(Physical_Part, partIDs[row]);
            partList.push_back(name);
        }

        _listGenericPart->clear();
        _listGenericPart->addItems(partList);
    }

    void mGenericPartPanel::slot_allPartBtn()
    {
        QStringList partList = getCVM->getModelData()->getAllNames(Physical_Part);
        _listGenericPart->clear();
        _listGenericPart->addItems(partList);

        int modelID = getCVM->getModelData()->getModelDataID();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();

        mapRelation.deleteMapRelation({ Physical_GenericPart, modelID });
        for (int row = 0; row < partList.size(); row++)
        {
            int paramId = getCVM->getModelData()->getPhyIDByName(Physical_Part, partList[row]);
            mapRelation.appendTableMapRelation(
                { Physical_GenericPart, modelID },
                { Physical_Part, paramId },
                row
            );
        }
    }
    void mGenericPartPanel::slot_addPartBtn()
    {
        QStringList partList = getCVM->getModelData()->getAllNames(Physical_Part);

        _tempModel = new QStringListModel(this);
        _tempModel->setStringList(partList);

        QListWidgetItem* newItem = new QListWidgetItem();
        _listGenericPart->addItem(newItem);

        QLineEdit* editor = new QLineEdit(_listGenericPart);
        _listGenericPart->setItemWidget(newItem, editor);
        editor->setFocus();

        // 自动补全
        QCompleter* completer = new QCompleter(_tempModel, editor);
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
                _tempModel->deleteLater();
                };

            if (!editorGuard) {
                int r = _listGenericPart->row(newItem);
                if (r >= 0) delete _listGenericPart->takeItem(r);
                cleanUp();
                return;
            }

            // 获取当前行号
            int row = _listGenericPart->row(newItem);
            if (row < 0) {
                cleanUp();
                return;
            }

            _listGenericPart->removeItemWidget(newItem);
            QString text = editorGuard->text().trimmed();

            // 空值删除
            if (text.isEmpty()) {
                delete _listGenericPart->takeItem(row);
                cleanUp();
                return;
            }

            // 不在候选列表删除
            if (!_tempModel->stringList().contains(text)) {
                delete _listGenericPart->takeItem(row);
                cleanUp();
                return;
            }

            // 去重
            bool exists = false;
            for (int i = 0; i < _listGenericPart->count(); ++i)
            {
                QListWidgetItem* it = _listGenericPart->item(i);
                if (it == newItem) continue;
                if (it && it->text() == text) {
                    exists = true;
                    break;
                }
            }
            if (exists)
            {
                delete _listGenericPart->takeItem(row);
                cleanUp();
                return;
            }

            newItem->setText(text);

            //映射
            int id = getCVM->getModelData()->getPhyIDByName(Physical_Part, text);
            getCVM->getModelData()->getMapRelationManager()
                .appendTableMapRelation(
                    { Physical_GenericPart,  getCVM->getModelData()->getModelDataID() },
                    { Physical_Part, id },
                    row
                );
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
    void mGenericPartPanel::slot_reducePartBtn()
    {
        // 获取当前选中的所有行
        auto selectedItems = _listGenericPart->selectedItems();
        if (selectedItems.isEmpty()) return;

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();

        // 遍历删除每一个选中行
        for (QListWidgetItem* item : selectedItems)
        {
            if (!item) continue;

            // 获取当前行号（必须从List获取，保证准确）
            int row = _listGenericPart->row(item);
            if (row < 0) continue;

            //删除表格映射
            mapRelation.deleteTabelMapRelation(
                { Physical_GenericPart,  getCVM->getModelData()->getModelDataID() },
                { Physical_Part, -1 },
                row
            );

            // 删除UI行
            delete _listGenericPart->takeItem(row);
        }
    }
    void mGenericPartPanel::slot_clearPartBtn()
    {
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int totalRow = _listGenericPart->count();

        // 倒序删除（从最后一行删到第0行，保证行号不乱）
        for (int row = totalRow - 1; row >= 0; --row)
        {
            QListWidgetItem* item = _listGenericPart->item(row);
            if (!item) continue;

            // ====================== 删除表格映射 ======================
            mapRelation.deleteTabelMapRelation(
                { Physical_GenericPart,  getCVM->getModelData()->getModelDataID() },
                { Physical_Part, -1 },
                row
            );
        }

        // UI全部清空
        _listGenericPart->clear();
    }

}
