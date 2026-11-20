#include "mContactPairPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"
#include "mInfoWindow.h"
#include "mCalcLineEdit.h"
#include "mTableDataWindow.h"
#include "mDockLayoutManager.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/view/HighlightController.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/mPreGeoHighLightRender.h"
#include "mxr/view/CCSMMeshHighLightRender.h"

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
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QColorDialog>
#include <QtConcurrent/QtConcurrent>

//BasicData
#include "mModelData.h"
#include "mMeshData.h"
//#include "mGeoDataOfTie.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）
#include "mGeoDataBase.h"
#include "mContactManager.h"
#include "mContactBase.h"
#include "mMixSet.h"

#include "PickCommonFunction.h"

using namespace MechPreData;
using namespace mxr;
using namespace MxFunctions;
using namespace MGeoData;
using namespace MechContactData;
using namespace MechSetData;
//using namespace MGeoCommand; //【OCC摘除·S10】MXGeoCommand 冻结岛
namespace MechMainWindow
{
    // --- 工具函数：构造显示、tooltip、排序 key ---

// 显示用（单条时显示 "[部件:面]"）
// -------------------------------------------------------------
// 工具：收集一个 group 中所有 (partId, faceId) 并去重
// -------------------------------------------------------------
    static void collectUniqueFacePairs(
        const std::vector<mTiedFaces>& group,
        bool mainSide,
        std::set<std::pair<int, int>>& outPairs)
    {
        for (const auto& f : group)
        {
            int partId = mainSide ? f.mainPartId : f.secondPartId;
            const auto& ids = mainSide ? f.mainFaceIds : f.secondFaceIds;

            for (int id : ids)
                outPairs.insert({ partId, id });
        }
    }



    // -------------------------------------------------------------
    // 常显示文本：根据是否存在不同 partId 或 faceId 决定是否加 “等”
    // -------------------------------------------------------------
    static QString makeDisplayOne(const std::vector<mTiedFaces>& group, bool mainSide)
    {
        if (group.empty()) return "";

        // 1. 收集唯一 (partId, faceId)
        std::set<std::pair<int, int>> pairs;
        collectUniqueFacePairs(group, mainSide, pairs);

        if (pairs.empty()) return "";

        // 显示只用第一个
        auto first = *pairs.begin();
        int partId = first.first;
        int faceId = first.second;

        // 2. 判断是否都相同
        bool allSame = (pairs.size() == 1);

        if (allSame)
        {
            // 完全一样 → 直接显示
            return QString("[%1:%2]").arg(partId).arg(faceId);
        }
        else
        {
            // 存在不同 → 显示“等”
            return QString("[%1:%2]等").arg(partId).arg(faceId);
        }
    }



    // -------------------------------------------------------------
    // tooltip：列出所有唯一的 (partId, faceId)
    // -------------------------------------------------------------
    static QString makeTooltip(const std::vector<mTiedFaces>& group, bool mainSide)
    {
        std::set<std::pair<int, int>> pairs;
        collectUniqueFacePairs(group, mainSide, pairs);

        if (pairs.empty()) return "";

        QString tip;
        for (const auto& p : pairs)
            tip += QString("[%1:%2] ").arg(p.first).arg(p.second);

        return tip.trimmed();
    }


    // 排序/过滤依据（全部 face 展开）
    static QString makeFullKey(const std::vector<mTiedFaces>& group, bool mainSide)
    {
        return makeTooltip(group, mainSide);
    }

    ContactPairTable::ContactPairTable(QWidget* parent)
        : QTableWidget(parent)
    {
        setColumnCount(7);
        setHorizontalHeaderLabels({
            "接触名",
            "主面部件ID",
            "从面部件ID",
            "类型",
            "切换",
            "合并",
            "删除"
            });

        horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        verticalHeader()->setVisible(false);
        // 允许多行选择
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        // 行选择模式（选中整行）
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setFocusPolicy(Qt::NoFocus); // 避免 cellWidget 抢焦点导致表格重置 selection
        createWidgetUI();
    }

    ContactPairTable::~ContactPairTable() {}

    void ContactPairTable::setAllPartTransparent(bool transparent)
    {
        if (!getCVM)
        {
            return;
        }
        std::vector<int> partIds = getCVM->getModelData()->getMeshData()->getAllPartIds();
        std::set<int> sId;
        for (auto id : partIds)
        {
            sId.insert(id);
        }
        if (getCVM->getPreRenderWidget())
        {
            if (transparent)
            {
                getCVM->getPreRenderWidget()->getPreRender()->setTransparent(true, 0.02f);
            }
            else
            {
                getCVM->getPreRenderWidget()->getPreRender()->setTransparent(true, 1.0);
            }

            getCVM->updateAllModelOperate(ColorOnePart, sId);
            getCVM->updateRender();
        }
    }

    void ContactPairTable::computeAutoMergeForFaces2()
    {
        _faces2.clear();
        if (_faces1.empty()) return;

        struct PairKey { int part; int face; };
        auto keyHash = [](const PairKey& k)->size_t { return std::hash<long long>()(((long long)k.part << 32) ^ (unsigned long long)k.face); };
        auto keyEq = [](const PairKey& a, const PairKey& b)->bool { return a.part == b.part && a.face == b.face; };

        std::unordered_map<PairKey, std::vector<mTiedFaces>, decltype(keyHash), decltype(keyEq)>
            mergeMap(0, keyHash, keyEq);

        std::vector<PairKey> orderKeys;

        for (const auto& group : _faces1)
        {
            if (group.empty()) continue;
            const mTiedFaces& f = group[0];
            int faceId = f.mainFaceIds.empty() ? -1 : *f.mainFaceIds.begin();
            PairKey k{ f.mainPartId, faceId };

            auto it = mergeMap.find(k);
            if (it == mergeMap.end()) {
                mergeMap.emplace(k, std::vector<mTiedFaces>{ f });
                orderKeys.push_back(k);
            }
            else {
                it->second.push_back(f);
            }
        }

        _faces2.reserve(orderKeys.size());
        for (auto& k : orderKeys) {
            _faces2.push_back(std::move(mergeMap[k]));
        }
    }

    void ContactPairTable::setData(int method, std::vector<mTiedFaces> faces)
    {
        std::vector<std::vector<mTiedFaces>> newFaces;
        for (int i = 0; i < faces.size(); ++i)
        {
            faces[i].name = QString("Pair_%1").arg(i + 1);
            faces[i].type = QString("面面接触");
            newFaces.push_back({ faces.at(i) });
        }
        //==================================================
        // face1 是“原始数据”，内层固定 1 个
        //==================================================
        if (method == 1)
        {
            _faces1 = newFaces;

            //=========================
            // Step2：计算 _faces2
            //=========================
            computeAutoMergeForFaces2();

            // 刷新 UI
            refreshTable();
            return;
        }

        //==================================================
        // face3 原样存储
        //==================================================
        if (method == 3)
        {
            _faces3 = newFaces;
            refreshTable();
            return;
        }
    }

    std::vector<std::vector<mTiedFaces>>& ContactPairTable::currentFaces() {
        if (currentMethod == 1) return _faces1;
        if (currentMethod == 2) return _faces2;
        return _faces3;
    }

    // 根据单元格 widget 找到其所在行（线性查找，表行数一般不多）
    int ContactPairTable::findRowOfWidget(QWidget* w)
    {
        if (!w) return -1;
        for (int r = 0; r < rowCount(); ++r) {
            for (int c = 0; c < columnCount(); ++c) {
                QWidget* cw = cellWidget(r, c);
                if (cw == w) return r;
                // 某些情况下 nameEdit 是 QLineEdit 放在 cellWidget
            }
        }
        return -1;
    }

    void ContactPairTable::refreshTable()
    {
        clearContents();
        setRowCount(0);

        auto& groups = currentFaces();  // vector<vector<mTiedFaces>>

        for (int i = 0; i < groups.size(); ++i) {
            const auto& inner = groups[i];
            if (inner.empty()) continue;

            insertRow(i);

            // UI 展示 inner[0] 作为代表
            fillRowFromData(i, inner[0], i + 1);
        }
    }
    void ContactPairTable::fillRowFromData(int row, const mTiedFaces& f, int pairIndex)
    {
        auto& groups = currentFaces();
        const auto& group = groups[row];

        // 0 接触名
        QLineEdit* nameEdit = new QLineEdit(QString("%1").arg(group.at(0).name));
        setCellWidget(row, 0, nameEdit);
        // 连接信号，修改数据层
        connect(nameEdit, &QLineEdit::textChanged, this, [this, row](const QString& text) {
            auto& groups = currentFaces();
            if (row >= 0 && row < groups.size() && !groups[row].empty()) {
                groups[row][0].name = text;  // 更新代表名称
                emit sig_dataChanged();      // 通知外部数据变化
            }
            });

        // ==== 1 主面 ====
        QString showMain = makeDisplayOne(group, true);
        QString tipMain = makeTooltip(group, true);

        auto* item1 = new QTableWidgetItem(showMain);
        item1->setToolTip(tipMain);
        item1->setData(Qt::UserRole, makeFullKey(group, true));  // 排序 key
        item1->setFlags(item1->flags() & ~Qt::ItemIsEditable);
        setItem(row, 1, item1);

        // ==== 2 从面 ====
        QString showSecond = makeDisplayOne(group, false);
        QString tipSecond = makeTooltip(group, false);

        auto* item2 = new QTableWidgetItem(showSecond);
        item2->setToolTip(tipSecond);
        item2->setData(Qt::UserRole, makeFullKey(group, false));
        item2->setFlags(item1->flags() & ~Qt::ItemIsEditable);
        setItem(row, 2, item2);

        // ==== 3 类型 ====
        QComboBox* typeBox = new QComboBox;
        typeBox->addItems({ "面面接触", "点面接触" });
        typeBox->setFocusPolicy(Qt::NoFocus);
        typeBox->setCurrentText(group.at(0).type);
        setCellWidget(row, 3, typeBox);
        // 连接信号，修改数据层
        connect(typeBox, &QComboBox::currentTextChanged, this, &ContactPairTable::onTypeChanged);

        // ==== 4 切换 ====
        QPushButton* btnEx = new QPushButton("切换");
        btnEx->setFocusPolicy(Qt::NoFocus);
        setCellWidget(row, 4, btnEx);
        connect(btnEx, &QPushButton::clicked, this, &ContactPairTable::onExchangeClicked);

        // ==== 5 合并 ====
        QPushButton* btnMerge = new QPushButton("合并");
        btnMerge->setFocusPolicy(Qt::NoFocus);
        setCellWidget(row, 5, btnMerge);
        connect(btnMerge, &QPushButton::clicked, this, &ContactPairTable::onMergeClicked);

        // ==== 6 删除 ====
        QPushButton* btnDel = new QPushButton("删除");
        btnDel->setFocusPolicy(Qt::NoFocus);
        setCellWidget(row, 6, btnDel);
        connect(btnDel, &QPushButton::clicked, this, &ContactPairTable::onDeleteClicked);
    }


    void ContactPairTable::setMethod(int method)
    {
        if (method < 1 || method > 3) method = 1;
        currentMethod = method;
        _methodSelect->setCurrentIndex(currentMethod - 1);
        refreshTable();
    }
    void ContactPairTable::createWidgetUI()
    {
        _widget = new QWidget;
        _widget->hide();
        auto* layout = new QVBoxLayout(_widget);

        auto* toolLayout = new QHBoxLayout;

        _filterCol = new QComboBox; _filterCol->addItems({ "接触名","主面部件ID","从面部件ID","类型" });
        _filterText = new QLineEdit;
        // QPushButton* btnFilter = new QPushButton("过滤");

        _sortCol = new QComboBox; _sortCol->addItems({ "接触名","主面部件ID","从面部件ID","类型" });
        _sortOrder = new QComboBox; _sortOrder->addItems({ "升序","降序" });
        // QPushButton* btnSort = new QPushButton("排序");

        _methodSelect = new QComboBox;
        _methodSelect->addItems({ "默认","一对多","区域" });
        // QPushButton* btnRefresh = new QPushButton("刷新");

        toolLayout->addWidget(new QLabel("方式:"));
        toolLayout->addWidget(_methodSelect);
        // toolLayout->addWidget(btnRefresh);

        toolLayout->addSpacing(20);
        toolLayout->addWidget(new QLabel("过滤:"));
        toolLayout->addWidget(_filterCol);
        toolLayout->addWidget(_filterText);
        // toolLayout->addWidget(btnFilter);
        toolLayout->addSpacing(20);
        toolLayout->addWidget(new QLabel("排序:"));
        toolLayout->addWidget(_sortCol);
        toolLayout->addWidget(_sortOrder);
        // toolLayout->addWidget(btnSort);

        // ===== 新增颜色选择按钮 =====
        // 小颜色块按钮样式
        auto createColorButton = [&](const QString& text, const QColor& color, QPushButton*& outBtn) {
            QWidget* w = new QWidget;
            QHBoxLayout* h = new QHBoxLayout(w);
            h->setContentsMargins(0, 0, 0, 0);
            h->setSpacing(5);

            QLabel* lbl = new QLabel(text);
            QPushButton* btn = new QPushButton;
            btn->setFixedSize(20, 20);
            btn->setStyleSheet(QString("background: %1; border:1px solid #444;").arg(color.name()));

            outBtn = btn;

            h->addWidget(lbl);
            h->addWidget(btn);

            toolLayout->addWidget(w);
            };

        createColorButton("主面:", Qt::red, _btnMainColor);
        createColorButton("从面:", Qt::blue, _btnSecondColor);

        // 点击选择颜色
        connect(_btnMainColor, &QPushButton::clicked, this, [this]() {
            QColor c = QColorDialog::getColor(_mainColor, this, "选择主面颜色");
            if (c.isValid()) {
                _mainColor = c;
                _btnMainColor->setStyleSheet(QString("background:%1; border:1px solid #444;").arg(c.name()));
                onItemSelectionChanged();
            }
            });

        connect(_btnSecondColor, &QPushButton::clicked, this, [this]() {
            QColor c = QColorDialog::getColor(_secondColor, this, "选择从面颜色");
            if (c.isValid()) {
                _secondColor = c;
                _btnSecondColor->setStyleSheet(QString("background:%1; border:1px solid #444;").arg(c.name()));
                onItemSelectionChanged();
            }
            });

        // 保存默认颜色
        _mainColor = Qt::red;
        _secondColor = Qt::blue;

        auto space = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
        toolLayout->addSpacerItem(space);
        auto createButton = new QPushButton("创建");
        toolLayout->addWidget(createButton);
        connect(createButton, &QPushButton::clicked, this, &ContactPairTable::onCreateClicked);

        layout->addLayout(toolLayout);
        layout->addWidget(this);

        // ====== 实时过滤 ======
        connect(_filterText, &QLineEdit::textChanged, this, [=](const QString&) {
            filterByColumn(_filterCol->currentIndex(), _filterText->text());
            });
        connect(_filterCol, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int) {
            filterByColumn(_filterCol->currentIndex(), _filterText->text());
            });

        // ====== 实时排序 ======
        auto sortFunc = [=]() {
            Qt::SortOrder order = (_sortOrder->currentIndex() == 0) ? Qt::AscendingOrder : Qt::DescendingOrder;
            sortByColumnAndOrder(_sortCol->currentIndex(), order);
            };

        connect(_sortCol, QOverload<int>::of(&QComboBox::currentIndexChanged), this, sortFunc);
        connect(_sortOrder, QOverload<int>::of(&QComboBox::currentIndexChanged), this, sortFunc);
        // connect(btnFilter, &QPushButton::clicked, this, [=]() {
        //     filterByColumn(_filterCol->currentIndex(), _filterText->text());
        // });
        // connect(btnSort, &QPushButton::clicked, this, [=]() {
        //     Qt::SortOrder order = (_sortOrder->currentIndex() == 0) ? Qt::AscendingOrder : Qt::DescendingOrder;
        //     sortByColumnAndOrder(_sortCol->currentIndex(), order);
        // });
        // connect(btnRefresh, &QPushButton::clicked, this, [=]() {
        //     currentMethod = _methodSelect->currentIndex() + 1;
        //     setMethod(currentMethod);
        // });
        // ====== 方法切换实时刷新 ======
        connect(_methodSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int idx) {
            setMethod(idx + 1);  // 选择方式直接刷新表格
            });

        connect(this, &QTableWidget::itemSelectionChanged,
            this, &ContactPairTable::onItemSelectionChanged);
    }
    void ContactPairTable::onTypeChanged(const QString& text)
    {
        auto& groups = currentFaces();
        QList<QTableWidgetSelectionRange> ranges = selectedRanges();
        QVector<int> rows;
        QWidget* senderW = qobject_cast<QWidget*>(sender());
        int row = findRowOfWidget(senderW);
        if (!ranges.isEmpty()) {
            for (auto range : ranges)
            {
                for (int r = range.topRow(); r <= range.bottomRow(); ++r)
                    rows.push_back(r);
            }
        }
        else {
            QWidget* senderW = qobject_cast<QWidget*>(sender());
            int row = findRowOfWidget(senderW);
            if (row < 0) return;
            rows.push_back(row);
        }
        // 更新所有选中行类型
        for (int r : rows)
        {
            if (r >= 0 && r < groups.size() && !groups[r].empty())
                groups[r][0].type = text;

            // 同步 UI：其他行的 ComboBox 也要更新显示
            QComboBox* box = qobject_cast<QComboBox*>(cellWidget(r, 3));
            if (box && r != row)
            {
                QSignalBlocker blocker(box); // 避免递归触发
                box->setCurrentText(text);
            }
        }

        emit sig_dataChanged();
    }

    void ContactPairTable::onExchangeClicked()
    {
        auto& faces = currentFaces();
        QList<QTableWidgetSelectionRange> ranges = selectedRanges();
        QVector<int> rows;

        if (!ranges.isEmpty()) {
            for (auto range : ranges)
            {
                for (int r = range.topRow(); r <= range.bottomRow(); ++r)
                    rows.push_back(r);
            }
        }
        else {
            QWidget* senderW = qobject_cast<QWidget*>(sender());
            int row = findRowOfWidget(senderW);
            if (row < 0) return;
            rows.push_back(row);
        }

        for (int r : rows) {
            if (r >= faces.size()) continue;
            for (auto& innerfaces : faces[r])
            {
                std::swap(innerfaces.mainPartId, innerfaces.secondPartId);
                std::swap(innerfaces.mainFaceIds, innerfaces.secondFaceIds);
            }
        }

        refreshTable();

        // -------- 恢复选中 --------
        QItemSelectionModel* sm = selectionModel();

        for (int r : rows)
        {
            if (r < rowCount())
            {
                QModelIndex index = model()->index(r, 0);
                sm->select(index,
                    QItemSelectionModel::Select |
                    QItemSelectionModel::Rows);
            }
        }

        emit sig_dataChanged();
    }
    void ContactPairTable::onDeleteClicked()
    {
        auto& faces = currentFaces();

        QList<QTableWidgetSelectionRange> ranges = selectedRanges();
        QVector<int> delRows;

        if (!ranges.isEmpty()) {
            for (auto range : ranges)
            {
                for (int r = range.topRow(); r <= range.bottomRow(); ++r)
                    delRows.push_back(r);
            }
        }
        else {
            QWidget* senderW = qobject_cast<QWidget*>(sender());
            int row = findRowOfWidget(senderW);
            if (row < 0) return;
            delRows.push_back(row);
        }

        // 从后往前删除
        std::sort(delRows.begin(), delRows.end(), std::greater<int>());
        for (int r : delRows)
            if (r < faces.size()) faces.erase(faces.begin() + r);

        refreshTable();
        emit sig_dataChanged();
    }
    void ContactPairTable::onMergeClicked()
    {
        auto& groups = currentFaces();  // vector<vector<mTiedFaces>>

        // 取得用户选择的外层行
        QList<QTableWidgetSelectionRange> ranges = selectedRanges();
        QVector<int> rows;

        if (!ranges.isEmpty()) {
            for (auto range : ranges)
                for (int r = range.topRow(); r <= range.bottomRow(); ++r)
                    rows.push_back(r);
        }
        else {
            QWidget* senderW = qobject_cast<QWidget*>(sender());
            int row = findRowOfWidget(senderW);
            if (row < 0 || row + 1 >= rowCount()) return;

            rows << row << row + 1;
        }

        std::sort(rows.begin(), rows.end());
        int top = rows.first();
        int bottom = rows.last();

        if (top >= groups.size()) return;

        // --- 构建新内层 vector<mTiedFaces> ---
        std::vector<mTiedFaces> mergedInner;

        for (int r : rows) {
            if (r >= 0 && r < groups.size()) {
                // 把内层展开合并
                mergedInner.insert(
                    mergedInner.end(),
                    groups[r].begin(),
                    groups[r].end()
                );
            }
        }

        // --- 删除原有行 ---
        for (int i = bottom; i >= top; --i)
            groups.erase(groups.begin() + i);

        // --- 插入新合并后的行 ---
        groups.insert(groups.begin() + top, mergedInner);

        refreshTable();


        // -------- 恢复选中（合并后的行）--------
        QItemSelectionModel* sm = selectionModel();
        clearSelection();
        if (top < rowCount())
        {
            QModelIndex index = model()->index(top, 0);
            sm->select(index,
                QItemSelectionModel::Select |
                QItemSelectionModel::Rows);
        }



        emit sig_dataChanged();
    }
    void ContactPairTable::onCreateClicked()
    {
        auto& groups = currentFaces();
        std::vector<int> contactIds;
        for (auto& group : groups) {
            for (auto& innerfaces : group)
            {
                //std::swap(innerfaces.mainPartId, innerfaces.secondPartId);
                //std::swap(innerfaces.mainFaceIds, innerfaces.secondFaceIds);

                //获取映射
                auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();

                //创建数据
                int contactID = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).createID();
                ContactType contactType = SurfaceToSurfaceContact;
                if (innerfaces.type == "点面接触")
                {
                    contactType = NodeToSurfaceContact;
                }
                mContactBase * _contactData = new mContactBase(contactID, contactType);
                QString name = getCVM->getModelData()->createNameByType(Physical_Contact, innerfaces.type);
            
                _contactData->setContactName(name, false);
                _contactData->setContactName(getCVM->getModelData()->createNameByType(Physical_Contact, "Contact"), false);
                getCVM->getModelData()->addPhysicalField<mContactBase>(_contactData);

                //创建主面内部集合
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                mMixSet* set = new mMixSet(setID, SetType::Mix, "contain", true);
                set->replaceMixSetData(PickDataType::GeoFace, {}, { innerfaces.mainFaceIds.begin(), innerfaces.mainFaceIds.end() });
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
                mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Contact, contactID }, { Physical_Set, setID }, MapRelation_Master);

                //创建从面内部集合
                setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                set = new mMixSet(setID, SetType::Mix, "contain", true);
                set->replaceMixSetData(PickDataType::GeoFace, {}, { innerfaces.secondFaceIds.begin(), innerfaces.secondFaceIds.end() });
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set);
                mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Contact, contactID }, { Physical_Set, setID }, MapRelation_Slave);

                //emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateContactSig(true, _contactData->getContactId());
                contactIds.push_back(contactID);
            }
        }

        if (!contactIds.empty())
        {
            MechCommon::mMechGlobalSignals::getInstance()->batchCreateContactSig(contactIds);
        }


    }
    void ContactPairTable::onItemSelectionChanged()
    {
        QModelIndexList list = this->selectionModel()->selectedRows();

        // 主面数据
        std::vector<int> mainFaces;
        // 从面数据
        std::vector<int> secondFaces;
        if (list.isEmpty())
        {   //高亮主面
            auto widget_ptr = getCVM->getPreRenderWidget();
            auto highlight_controller = widget_ptr->getHighlightController();
            auto container = widget_ptr->getSelectionContainer();
            mxr::mGeoSelectData geodata;
            geodata.UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, mainFaces);
            highlight_controller->updateTempHighLight(&geodata);
            setAllPartTransparent(false);
        }
        else
        {
            setAllPartTransparent();
        }
        for (QModelIndex l : list)
        {
            int row = l.row();

            auto& groups = currentFaces();
            if (row < 0 || row >= groups.size())
                continue;

            const auto& group = groups[row];   // 内层 vector<mTiedFaces>
            if (group.empty()) return;


            for (auto& f : group)
            {
                for (int id : f.mainFaceIds)
                    mainFaces.push_back(id);
            }

            for (auto& f : group)
            {
                for (int id : f.secondFaceIds)
                    secondFaces.push_back(id);
            }
        }

        //高亮主面
        auto widget_ptr = getCVM->getPreRenderWidget();
        auto highlight_controller = widget_ptr->getHighlightController();
        {
            auto container = widget_ptr->getSelectionContainer();
            auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"));
            geoHighLightRender->setHighLightColor(_mainColor.red(), _mainColor.green(), _mainColor.blue());
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, mainFaces);
            geoHighLightRender->updateHighLightRender(geoData);
        }

        //高亮从面
        {
            auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"));
            geoHighLightRender->setHighLightColor(_secondColor.red(), _secondColor.green(), _secondColor.blue());
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, secondFaces);
            geoHighLightRender->updateHighLightRender(geoData);
        }
    }
    void ContactPairTable::filterByColumn(int col, const QString& text)
    {
        QString key = text.trimmed();

        if (key.isEmpty())
        {
            for (int r = 0; r < rowCount(); ++r)
                setRowHidden(r, false);
            return;
        }

        for (int r = 0; r < rowCount(); ++r)
        {
            bool match = false;

            if (col == 0)
            {
                QLineEdit* le = qobject_cast<QLineEdit*>(cellWidget(r, 0));
                if (le) match = le->text().contains(key);
            }
            else if (col == 1 || col == 2)
            {
                QTableWidgetItem* it = item(r, col);
                QString fullKey = it->data(Qt::UserRole).toString();
                match = fullKey.contains(key);
            }
            else if (col == 3)
            {
                QComboBox* cb = qobject_cast<QComboBox*>(cellWidget(r, 3));
                if (cb) match = cb->currentText().contains(key);
            }

            setRowHidden(r, !match);
        }
    }

    void ContactPairTable::sortByColumnAndOrder(int col, Qt::SortOrder order)
    {
        struct RowInfo {
            int row;
            QString key;
        };

        QVector<RowInfo> rowsInfo;

        for (int r = 0; r < rowCount(); ++r)
        {
            QString key;

            if (col == 0)
            {
                // 0 列是 QLineEdit
                QLineEdit* le = qobject_cast<QLineEdit*>(cellWidget(r, 0));
                key = le ? le->text() : "";
            }
            else if (col == 3)
            {
                // 3 列是 QComboBox
                QComboBox* cb = qobject_cast<QComboBox*>(cellWidget(r, 3));
                key = cb ? cb->currentText() : "";
            }
            else
            {
                // 1、2 列是 QTableWidgetItem，key 在 UserRole 中
                QTableWidgetItem* it = item(r, col);
                key = it ? it->data(Qt::UserRole).toString() : "";
            }

            rowsInfo.push_back({ r, key });
        }

        // 排序
        std::sort(rowsInfo.begin(), rowsInfo.end(), [=](RowInfo& a, RowInfo& b) {
            return order == Qt::AscendingOrder ? (a.key < b.key) : (a.key > b.key);
            });

        // ===== 重排数据层（_faces1/2/3） =====
        auto& data = currentFaces();
        std::vector<std::vector<mTiedFaces>> newData;
        newData.reserve(data.size());

        for (auto& info : rowsInfo)
            newData.push_back(data[info.row]);

        data = std::move(newData);

        // ===== 重建表格 =====
        refreshTable();
    }


    mContactPairPanel::mContactPairPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);

        _mainLayout->setContentsMargins(2, 0, 2, 0);

        _currentViewportPrivate = nullptr;
        _table  = new ContactPairTable();

        // ========== 部件折叠块(0) ==========
        //FoldSection* partFold = new FoldSection("搜索区域", this);
        //m_foldWidgets.append(partFold);
        //_mainLayout->addWidget(partFold);
        
        // ========== 控制参数折叠块(1) ==========
        FoldSection* paramFold = new FoldSection("控制参数", this);
        _toleranceEdit = new QDoubleSpinBox();
        _toleranceEdit->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons); _toleranceEdit->setRange(0.01, 100000); _toleranceEdit->setDecimals(2); _toleranceEdit->setValue(2);
        _angleEdit = new QDoubleSpinBox();
        _angleEdit->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons); _angleEdit->setRange(0, 359.9); _angleEdit->setDecimals(1); _angleEdit->setValue(135);
        _comboPartSearch = new QComboBox();
        // 插入提示项
        _comboPartSearch->addItems({ "开启", "关闭" });

        paramFold->addRow(new PanelLabel("最大距离容差"), _toleranceEdit);
        paramFold->addRow(new PanelLabel("最小偏移角度"), _angleEdit);
        paramFold->addRow(new PanelLabel("部件内部搜索"), _comboPartSearch);
        m_foldWidgets.append(paramFold);
        _mainLayout->addWidget(paramFold);

        auto layout = new QHBoxLayout();
        _startSearchButton = new QPushButton("开始搜索");
        layout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
        layout->addWidget(_startSearchButton);
        _mainLayout->addLayout(layout);

        connect(_startSearchButton, &QPushButton::clicked, this, &mContactPairPanel::slot_startSearch);

        _mainLayout->addStretch();
    }

    mContactPairPanel::~mContactPairPanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }

    }

    void mContactPairPanel::editContactPairPanel()
    {
        setEditing(true);
        connectSig();
        mTableDataWindow::getInstance()->setCustomWidget(_table->getWidget());
    }

    void mContactPairPanel::uneditContactPairPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        disconnectSig();
        //mTableDataWindow::getInstance()->resetInitialTable();
        _table->setAllPartTransparent(false);

        //高亮主面
        auto widget_ptr = getCVM->getPreRenderWidget();
        auto highlight_controller = widget_ptr->getHighlightController();
        {
            auto container = widget_ptr->getSelectionContainer();
            auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"));
            //geoHighLightRender->setHighLightColor(_mainColor.red(), _mainColor.green(), _mainColor.blue());
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, {});
            geoHighLightRender->updateHighLightRender(geoData);
        }

        //高亮从面
        {
            auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"));
            //geoHighLightRender->setHighLightColor(_secondColor.red(), _secondColor.green(), _secondColor.blue());
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, {});
            geoHighLightRender->updateHighLightRender(geoData);
        }
    }

    void mContactPairPanel::slot_startSearch()
    {
        _startSearchButton->setEnabled(false);
        
        mGlobalSignals::getInstance()->outputMessageSig(QString("开始搜索接触对"), true);
        //使用方法
        if (getCVM == nullptr)
        {
            return;
        }
        auto rend = getCVM->getPreRenderWidget();
        if (rend == nullptr)
        {
            return;
        }
        //获取拾取部件id
        auto partIds = getPreWidgetSelectPartID(rend);
        //【OCC摘除·S10 保档】自动搜索接触对依赖旧栈 mGeoDataOfTie（OCC 链），随冻结岛退役；引擎化移植点=geoCheck 接触对族
        /*auto tie = new mGeoDataOfTie(_toleranceEdit->value(), _angleEdit->value(), true, partIds, getCVM->getModelData()->getGeoData());
        QtConcurrent::run([=]() {
            tie->searchFaceTie();
            QMetaObject::invokeMethod(this, [=]() {
                mGlobalSignals::getInstance()->outputMessageSig(QString("接触对搜索完成"), true);
                auto faces = getCVM->getModelData()->getGeoData()->GetDiscreteFacePair();
                qDebug() << "faces" << faces.size();
                _table->setData(1, faces);
                faces = getCVM->getModelData()->getGeoData()->GetFuseFacePair();
                _table->setData(3, faces);
                _table->setMethod(1);
                _startSearchButton->setEnabled(true);
                }, Qt::QueuedConnection);
            });*/
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QStringLiteral("接触对自动搜索已随旧几何栈退役，待引擎化移植"));
        //清空拾取
        clearPreWidgetSelectPartID(rend);
        //ClearPreWidgetMeshPick(rend);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        //测试数据
        //     | mainPartId | mainFaceId | secondPartId | secondFaceId |
        //     | ---------- | ---------- | ------------ | ------------ |
        //     | 1          | 10         | 2            | 20           |
        //     | 1          | 10         | 3            | 30           |
        //     | 2          | 5          | 1            | 100          |
        //     | 1          | 11         | 4            | 40           |
        //     | 2          | 5          | 6            | 60           |
        /*
        std::vector<int> data{ 1,10,2,20,1,10,3,30,2,5,1,100,1,11,4,40,2,5,6,60 };
        std::vector<mTiedFaces> faces1;
        for (int i = 0; i < 5; ++i) {
            mTiedFaces face; face.mainPartId = data[i * 4 + 0]; face.mainFaceIds = { data[i * 4 + 1] }; face.secondPartId = data[i * 4 + 2]; face.secondFaceIds = { data[i * 4 + 3] };
            faces1.push_back(face);
        }

        _table->setData(1, faces1);

        std::vector<mTiedFaces> faces3;
        mTiedFaces face; face.mainPartId = 1; face.secondPartId = 2; face.mainFaceIds = { 1 }; face.secondFaceIds = { 3 };
        faces3.push_back(face);

        mTiedFaces face1; face1.mainPartId = 6; face1.secondPartId = 4; face1.mainFaceIds = { 9,19 }; face1.secondFaceIds = { 10,11 };
        faces3.push_back(face1);
        mTiedFaces face2; face2.mainPartId = 4; face2.secondPartId = 5; face2.mainFaceIds = { 7,8 }; face2.secondFaceIds = { 20,25 };
        faces3.push_back(face2);

        _table->setData(3, faces3);
        */
        _table->setMethod(1);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mContactPairPanel::slot_clearPartBtn()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        this->blockSignals(true);
        getCVM->clearPick();
        this->blockSignals(false);
    }

    void mContactPairPanel::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);
        if (_currentViewportPrivate == VMManager->getCurrentViewportPrivate())
        {
            return;
        }
        _currentViewportPrivate = VMManager->getCurrentViewportPrivate();
        connectSig();

    }
    void mContactPairPanel::slot_finishedPick()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        return;
        auto meshData = getCVM->getModelData()->getMeshData();
        //mDataElement* elementData = getCVM->getModelData()->getElementData();

        //if (_checkType == 0)//部件
        {
            auto partIds = MechPreData::getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
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
                    unHighlightPart(reducedIds);
                }
            }
            //_listAssignPart->clear();
            //_tableProperty->clearContents();
            //_tableProperty->setRowCount(0);
            //把所有属性都添加到部件中去
            for (int i = 0; i < partIds.size(); i++)
            {
                auto partName = meshData->getPartNameByPartId(partIds.at(i));
                //_listAssignPart->addItem(partName);
                //appendPartProperty(partName);
            }
        }
    }
    void mContactPairPanel::connectSig()
    {
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
        disconnectSig();
        if (!getCVM)
        {
            return;
        }
        connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mContactPairPanel::slot_subWindowActivated, Qt::UniqueConnection);
        conn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mContactPairPanel::slot_finishedPick);
    }
    void mContactPairPanel::disconnectSig()
    {
        disconnect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mContactPairPanel::slot_subWindowActivated);
        if (conn)
            disconnect(conn);
    }

    void mContactPairPanel::highlightPart(std::vector<int> ids)
    {
        if (conn)
            disconnect(conn);
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
        MechPreData::addPreWidgetSelectPartID(getCVM->getPreRenderWidget(), ids);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());


        getCVM->getPreRenderWidget()->updatePaint();

        conn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mContactPairPanel::slot_finishedPick);

        _lastPickPartData = MechPreData::getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
    }
    void mContactPairPanel::unHighlightPart(std::vector<int> ids)
    {
        if (conn)
            disconnect(conn);
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
        MechPreData::reducePreWidgetSelectPartID(getCVM->getPreRenderWidget(), ids);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

        getCVM->getPreRenderWidget()->updatePaint();

        conn = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mContactPairPanel::slot_finishedPick);

        _lastPickPartData = MechPreData::getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
    }
}