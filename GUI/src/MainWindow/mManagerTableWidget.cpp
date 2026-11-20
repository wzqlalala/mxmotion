#include "mManagerTableWidget.h"

#include "mLoadBoundaryManager.h"
#include "mModelData.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QShowEvent>
#include <QPainter>
#include <QMouseEvent>
#include <QSet>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include <algorithm>
#include <unordered_set>
#include "mLoadBoundaryBase.h"
#include "mCaseManager.h"
#include "mMapRelationManager.h"
#include "mMechGlobalSignals.h"
#include "mInfoWindow.h"
#include "mBrowserWindow.h"
#include "mTreeView.h"
#include "mDockLayoutManager.h"
#include "mPredefinedFieldManager.h"
#include "mPredefinedFieldBase.h"
#include "mContactBase.h"
#include "mModelFixManager.h"
#include "mModelFixDataBase.h"
#include "mMenuWidget.h"
#include "mCaseBase.h"

using namespace MechCaseData;
using namespace MechModelData;
using namespace MechMapRelation;
using namespace MechCommon;
using namespace MechModelFixData;
using namespace MechMainWindow;

// ===================== CheckBox委托实现 =====================
CheckBoxDelegate::CheckBoxDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void CheckBoxDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 绘制单元格背景（对齐HTML原型：选中单元格淡蓝、悬浮更浅）
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, QColor("#d2e8fc"));
    else if (option.state & QStyle::State_MouseOver)
        painter->fillRect(option.rect, QColor("#f5faff"));

    // 从Item数据中读取状态
    bool isChecked = index.data(Qt::CheckStateRole).toBool();
    bool isEnabled = index.data(Qt::UserRole).toBool();

    // 计算Checkbox居中位置（对齐原型17px勾选框）
    const int checkSize = 15;
    int x = option.rect.x() + (option.rect.width() - checkSize) / 2;
    int y = option.rect.y() + (option.rect.height() - checkSize) / 2;
    QRect checkRect(x, y, checkSize, checkSize);

    // 绘制Checkbox背景与边框（对齐原型：未勾选白底灰边、勾选蓝底蓝边、禁用灰）
    QColor bgColor = isEnabled ? QColor("#ffffff") : QColor("#e2e5eb");
    QColor borderColor = isEnabled ? QColor("#d8dde5") : QColor("#d0d4da");
    if (option.state & QStyle::State_MouseOver && isEnabled)
        borderColor = QColor("#1689ee");
    if (isChecked)
    {
        bgColor = isEnabled ? QColor("#1689ee") : QColor("#b8cdd8");
        borderColor = isEnabled ? QColor("#1689ee") : QColor("#b8cdd8");
    }

    painter->setPen(QPen(borderColor, 1));
    painter->setBrush(bgColor);
    painter->drawRoundedRect(checkRect.adjusted(0, 0, -1, -1), 2, 2);

    // 绘制勾选标记（白色对勾）
    if (isChecked)
    {
        painter->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPoint p1(checkRect.x() + 3, checkRect.y() + 8);
        QPoint p2(checkRect.x() + 6, checkRect.y() + 11);
        QPoint p3(checkRect.x() + 12, checkRect.y() + 4);
        painter->drawLine(p1, p2);
        painter->drawLine(p2, p3);
    }

    painter->restore();
}

bool CheckBoxDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        const int checkSize = 14;
        int x = option.rect.x() + (option.rect.width() - checkSize) / 2;
        int y = option.rect.y() + (option.rect.height() - checkSize) / 2;
        QRect checkRect(x, y, checkSize, checkSize);

        // 仅点击Checkbox区域才触发状态切换
        if (checkRect.contains(me->pos()))
        {
            bool isEnabled = index.data(Qt::UserRole).toBool();
            if (!isEnabled) return true;

            bool newState = !index.data(Qt::CheckStateRole).toBool();
            emit checkStateChanged(index.row(), index.column(), newState);
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

QSize CheckBoxDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize(20, 20);
}

// ===================== 内部信息文本框 =====================
    class InfoTextEdit : public QTextEdit
    {
    public:
        explicit InfoTextEdit(QWidget* parent = nullptr) : QTextEdit(parent)
        {
            setViewportMargins(10, 4, 0, 0);
        }
    };

// ===================== mManagerTableWidget 实现 =====================
mManagerTableWidget::mManagerTableWidget()
{
    // ===================== 0. 筛选工具栏 =====================
    //类型筛选 + 工况筛选 + 名称筛选（对齐管理器面板筛选行，仅过滤矩阵行列，不影响数据）
    m_filterBar = new QWidget(this);
    m_typeFilter = new QComboBox(m_filterBar);
    m_typeFilter->addItem("全部类型");
    m_typeFilter->setMinimumContentsLength(8);
    m_typeFilter->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    m_caseFilter = new QComboBox(m_filterBar);
    m_caseFilter->addItem("全部工况");
    m_caseFilter->setMinimumContentsLength(8);
    m_caseFilter->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    m_nameFilter = new QLineEdit(m_filterBar);
    m_nameFilter->setPlaceholderText("筛选名称");
    m_nameFilter->setClearButtonEnabled(true);

    QHBoxLayout* filterLayout = new QHBoxLayout(m_filterBar);
    filterLayout->addWidget(m_typeFilter);
    filterLayout->addWidget(m_caseFilter);
    filterLayout->addWidget(m_nameFilter, 1);
    filterLayout->setContentsMargins(0, 0, 0, 4);

    // ===================== 1. 表格初始化 =====================
    m_table = new QTableWidget(this);
    m_table->setAlternatingRowColors(false);
    m_table->setSelectionBehavior(QTableWidget::SelectItems);
    m_table->setSelectionMode(QTableWidget::ExtendedSelection);
    m_table->setEditTriggers(QTableWidget::NoEditTriggers); // 禁止默认编辑
    m_table->setSortingEnabled(false);                      // 关闭排序，减少表头计算开销
    m_table->setWordWrap(false);                            // 关闭自动换行，大幅提升绘制性能

    // 固定默认行高，避免逐行动态计算高度的开销
    m_table->verticalHeader()->setDefaultSectionSize(24);
    // 像素级滚动，大数据量下滚动更顺滑
    m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    // 禁用拖拽与Tab导航，减少事件分发的额外开销
    m_table->setDragEnabled(false);
    m_table->setAcceptDrops(false);
    m_table->setTabKeyNavigation(false);

    // 设置委托替代单元格Widget
    m_checkDelegate = new CheckBoxDelegate(m_table);
    m_table->setItemDelegate(m_checkDelegate);

    //对齐"配置工况载荷"原型：浅灰表头、细网格线、选中行淡蓝
    m_table->setStyleSheet(R"(
        QTableWidget {
            border: 1px solid #dce1e8;
            gridline-color: #edf0f3;
            background-color: #FFFFFF;
            selection-background-color: #d2e8fc;
            selection-color: #17191c;
        }
        QTableWidget::item { background: transparent; }
        QTableWidget::item:selected { background: #d2e8fc; }
        QTableWidget::item:hover { background: #f5faff; }
        QHeaderView::section:horizontal{
            background: #f5f7fa;
            color: #525d69;
            border: none;
            border-bottom: 1px solid #dce1e8;
            border-right: 1px solid #dce1e8;
            padding: 6px;
            font-weight: normal;
        }
        QHeaderView::section:vertical{
            background: #f5f7fa;
            color: #17191c;
            border: none;
            border-bottom: 1px solid #dce1e8;
            border-right: 1px solid #dce1e8;
            padding: 4px 8px;
            font-weight: normal;
        }
    )");
    m_table->horizontalHeader()->setMinimumHeight(32);

    // ===================== 1.5 弹窗顶部工具栏：类型/工况类型筛选 + 全部激活 =====================
    m_popupToolBar = new QWidget(this);
    QHBoxLayout* popupToolLayout = new QHBoxLayout(m_popupToolBar);
    popupToolLayout->setContentsMargins(0, 0, 0, 4);
    popupToolLayout->setSpacing(8);

    auto* popupTypeLabel = new QLabel("载荷类型", m_popupToolBar);
    m_popupTypeFilter = new QComboBox(m_popupToolBar);
    m_popupTypeFilter->addItem("全部类型");
    m_popupTypeFilter->setMinimumContentsLength(8);
    m_popupTypeFilter->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    auto* popupCaseTypeLabel = new QLabel("工况类型", m_popupToolBar);
    m_popupCaseTypeFilter = new QComboBox(m_popupToolBar);
    m_popupCaseTypeFilter->addItem("全部类型");
    m_popupCaseTypeFilter->setMinimumContentsLength(10);
    m_popupCaseTypeFilter->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    m_btnActivateAll = new QPushButton("全部激活", m_popupToolBar);
    m_btnActivateAll->setToolTip("勾选当前页面全部可勾选的选项；全部勾选后变为全部取消");

    popupToolLayout->addWidget(popupTypeLabel);
    popupToolLayout->addWidget(m_popupTypeFilter);
    popupToolLayout->addSpacing(12);
    popupToolLayout->addWidget(popupCaseTypeLabel);
    popupToolLayout->addWidget(m_popupCaseTypeFilter);
    popupToolLayout->addStretch();
    popupToolLayout->addWidget(m_btnActivateAll);
    m_popupToolBar->setVisible(false);

    // ===================== 2. 右侧信息文本框 =====================
    m_infoEdit = new InfoTextEdit(this);
    m_infoEdit->setReadOnly(true);
    m_infoEdit->setPlaceholderText("点击单元格查看信息");
    QFont font("Microsoft YaHei", 13, QFont::Normal);
    m_infoEdit->setFont(font);
    m_infoEdit->document()->setDocumentMargin(0);
    m_infoEdit->setStyleSheet(R"(
        QTextEdit {
            font-size: 13px;
            background: transparent;
            border: none;
        }
    )");
    m_infoEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget* infoContainer = new QWidget(this);
    QVBoxLayout* infoContainerLayout = new QVBoxLayout(infoContainer);
    infoContainerLayout->setContentsMargins(0, 0, 0, 0);
    infoContainerLayout->addWidget(m_infoEdit);
    infoContainer->setStyleSheet("QWidget { background: #FFFFFF; border: 1px solid #dcdcdc; }");

    // ===================== 3. 底部操作按钮（内嵌模式） =====================
    btnEdit = new QPushButton("编辑", this);
    btnCopy = new QPushButton("复制", this);
    btnDelete = new QPushButton("删除", this);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(btnEdit);
    btnLayout->addWidget(btnCopy);
    btnLayout->addWidget(btnDelete);
    btnLayout->addStretch();
    btnLayout->setContentsMargins(0, 5, 0, 0);

    // ===================== 3.5 底部选中信息区（弹窗模式，对齐原型） =====================
    m_selectionBar = new QWidget(this);
    QHBoxLayout* selectionLayout = new QHBoxLayout(m_selectionBar);
    selectionLayout->setContentsMargins(0, 6, 0, 0);
    selectionLayout->setSpacing(24);
    auto* selectedObjectTitle = new QLabel("选中载荷", m_selectionBar);
    selectedObjectTitle->setStyleSheet("color: #777e87;");
    m_selectedObjectLabel = new QLabel(m_selectionBar);
    auto* selectedCaseTitle = new QLabel("选中工况", m_selectionBar);
    selectedCaseTitle->setStyleSheet("color: #777e87;");
    m_selectedCaseLabel = new QLabel(m_selectionBar);
    selectionLayout->addWidget(selectedObjectTitle);
    selectionLayout->addWidget(m_selectedObjectLabel);
    selectionLayout->addWidget(selectedCaseTitle);
    selectionLayout->addWidget(m_selectedCaseLabel);
    selectionLayout->addStretch();
    m_selectionBar->setVisible(false);

    // ===================== 4. 左侧布局：弹窗工具栏 + 筛选 + 表格 + 按钮 + 选中信息 =====================
    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->addWidget(m_popupToolBar);
    leftLayout->addWidget(m_filterBar);
    leftLayout->addWidget(m_table, 1);
    leftLayout->addLayout(btnLayout);
    leftLayout->addWidget(m_selectionBar);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // ===================== 5. 主布局 =====================
    QHBoxLayout* mainLayout = new QHBoxLayout();
    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addWidget(infoContainer, 1);
    mainLayout->setAlignment(infoContainer, Qt::AlignTop);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(mainLayout);

    // 信号连接
    connect(m_table, &QTableWidget::cellClicked, this, &mManagerTableWidget::onCellClicked);
    connect(m_checkDelegate, &CheckBoxDelegate::checkStateChanged, this, &mManagerTableWidget::onCellCheckStateChanged);
    connect(btnEdit, &QPushButton::clicked, this, &mManagerTableWidget::onEditBtnClicked);
    connect(btnCopy, &QPushButton::clicked, this, &mManagerTableWidget::onCopyBtnClicked);
    connect(btnDelete, &QPushButton::clicked, this, &mManagerTableWidget::onDeleteBtnClicked);
    connect(m_typeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mManagerTableWidget::onFilterChanged);
    connect(m_caseFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mManagerTableWidget::onFilterChanged);
    connect(m_nameFilter, &QLineEdit::textChanged, this, &mManagerTableWidget::onFilterChanged);
    connect(m_popupTypeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mManagerTableWidget::onFilterChanged);
    connect(m_popupCaseTypeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mManagerTableWidget::onFilterChanged);
    connect(m_btnActivateAll, &QPushButton::clicked, this, &mManagerTableWidget::slot_activateAllBtn);
}

mManagerTableWidget::~mManagerTableWidget()
{
}

void mManagerTableWidget::setMenuWidget(mMenuWidget* menuWidget)
{
    m_menuWidget = menuWidget;
}

void mManagerTableWidget::setPopupMode(bool popup)
{
    m_popupMode = popup;
    //内嵌筛选栏仅内嵌模式显示；弹窗模式使用顶部工具栏与底部选中信息区
    if (m_filterBar)
        m_filterBar->setVisible(!popup);
    if (m_popupToolBar)
        m_popupToolBar->setVisible(popup);
    if (m_selectionBar)
        m_selectionBar->setVisible(popup);
    //弹窗模式下隐藏右侧信息框与编辑/复制/删除按钮（信息由底部选中区展示）
    if (m_infoEdit && m_infoEdit->parentWidget())
        m_infoEdit->parentWidget()->setVisible(!popup);
    if (btnEdit)
        btnEdit->setVisible(!popup);
    if (btnCopy)
        btnCopy->setVisible(!popup);
    if (btnDelete)
        btnDelete->setVisible(!popup);
}

// ===================== mManagerTablePopupDialog 实现 =====================
mManagerTablePopupDialog::mManagerTablePopupDialog(QWidget* parent)
    : QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint)
{
    // 不设置WA_DeleteOnClose：对话框由mManagerPanelTpl持有并复用，关闭仅隐藏，随面板销毁
    resize(960, 560);
    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    dialogLayout->setContentsMargins(10, 10, 10, 10);
    //独立的矩阵实例：与Table dock中常驻矩阵互不干扰，避免重父子导致的崩溃
    m_matrix = new mManagerTableWidget;
    //弹出模式下隐藏筛选工具栏（管理器面板自身提供筛选）
    m_matrix->setPopupMode(true);
    dialogLayout->addWidget(m_matrix);
}

void mManagerTablePopupDialog::positionBesideAnchor(QWidget* anchor)
{
    //对齐HTML原型：弹窗位于入口按钮所在面板的右侧，纵向与按钮顶部对齐
    QPoint anchorGlobal(100, 100);
    int anchorWidth = 380;
    if (anchor && anchor->isVisible())
    {
        //用按钮自身的全局矩形（frameGeometry在不可见/布局未就绪时不可靠）
        anchorGlobal = anchor->mapToGlobal(QPoint(0, 0));
        anchorWidth = anchor->width();
    }

    QScreen* screen = QGuiApplication::screenAt(anchorGlobal);
    if (screen == nullptr)
        screen = QGuiApplication::primaryScreen();
    QRect screenRect = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);

    int dialogWidth = qMin(900, screenRect.width() - 24);
    int dialogHeight = qMin(600, screenRect.height() - 24);
    resize(dialogWidth, dialogHeight);

    int x = anchorGlobal.x() + anchorWidth + 10;
    //右侧空间不足时回退到屏幕内
    if (x + dialogWidth > screenRect.right() - 12)
        x = qMax(screenRect.left() + 12, screenRect.right() - dialogWidth - 12);
    int y = qBound(screenRect.top() + 12, anchorGlobal.y(), screenRect.bottom() - dialogHeight - 12);
    move(x, y);
}

void mManagerTablePopupDialog::execMatrix(const QString& title, mMenuWidget* menuWidget, BrowserTreeType itemType, QWidget* anchor)
{
    setWindowTitle(title);
    if (menuWidget)
        m_matrix->setMenuWidget(menuWidget);
    //每次弹出都重新定位到锚点右侧（用户可拖动，但下次点击按钮仍回到原位）
    positionBesideAnchor(anchor);
    //模态打开前先填充矩阵（弹窗尚未进入事件循环，此时重建表格与按钮点击栈隔离）
    m_matrix->refreshTable(itemType);
    exec();
}

void mManagerTableWidget::refreshTable(BrowserTreeType itemType)
{
    //刷新前缓存筛选条件（切换类型时类型下拉重建、工况下拉重建，需尽量保留用户选择）
    QComboBox* typeFilterSrc = m_popupMode ? m_popupTypeFilter : m_typeFilter;
    QComboBox* caseFilterSrc = m_popupMode ? m_popupCaseTypeFilter : m_caseFilter;
    const QString previousTypeFilter = (typeFilterSrc && itemType == m_currentType) ? typeFilterSrc->currentText() : QString("全部类型");
    const QString previousCaseFilter = (caseFilterSrc && itemType == m_currentType) ? caseFilterSrc->currentText() : QString("全部类型");

    // 清空表格（轻量操作，仅销毁Item，无Widget销毁开销）
    m_table->clearContents();
    m_table->setRowCount(0);
    m_ids.clear();
    m_caseIds.clear();
    m_infoEdit->clear();
    m_currentType = itemType;

    // 提前缓存管理器指针，避免循环内重复调用
    auto modelData = getCVM->getModelData();
    auto& mapMgr = modelData->getMapRelationManager();
    auto& caseMgr = modelData->getManager<MechCaseData::mCaseManager>(Physical_Case);
    auto& lbMgr = modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary);
    auto& pfMgr = modelData->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField);
    auto& contactMgr = modelData->getManager<MechContactData::mContactManager>(Physical_Contact);
    auto& fixMgr = modelData->getManager<mModelFixManager>(Physical_ModelFix);

    m_caseIds = caseMgr.getAllCaseID();

    // 重建工况筛选下拉（保留原选中项；弹窗模式按工况类型去重，内嵌模式按工况名）
    if (caseFilterSrc)
    {
        caseFilterSrc->blockSignals(true);
        caseFilterSrc->clear();
        caseFilterSrc->addItem(m_popupMode ? "全部类型" : "全部工况");
        if (m_popupMode)
        {
            QSet<QString> addedCaseTypes;
            for (int caseId : m_caseIds)
            {
                QString caseTypeName = caseMgr.getCaseTypeNameByID(caseId);
                if (!caseTypeName.isEmpty() && !addedCaseTypes.contains(caseTypeName))
                {
                    addedCaseTypes.insert(caseTypeName);
                    caseFilterSrc->addItem(caseTypeName);
                }
            }
        }
        else
        {
            for (int caseId : m_caseIds)
                caseFilterSrc->addItem(caseMgr.getCaseNameByID(caseId));
        }
        int caseIndex = caseFilterSrc->findText(previousCaseFilter);
        caseFilterSrc->setCurrentIndex(caseIndex >= 0 ? caseIndex : 0);
        caseFilterSrc->blockSignals(false);
    }

    // 获取数据ID列表（全部对象，筛选在填充时进行）
    std::vector<int> allIds;
    if (itemType == LoadManagerParentItem)
        allIds = lbMgr.getAllLoadID();
    else if (itemType == BoundaryManagerParentItem)
        allIds = lbMgr.getAllBoundaryID();
    else if (itemType == PredefinedFieldManagerParentItem)
        allIds = pfMgr.getAllPredefinedFieldID();
    else if (itemType == ContactParentItem)
        allIds = contactMgr.getAllContactID();
    else if (itemType == ModelFixParentItem)
        allIds = fixMgr.getAllModelFixID();

    // 应用筛选：类型 / 工况(类型) / 名称
    QString typeFilter = typeFilterSrc ? typeFilterSrc->currentText() : QString("全部类型");
    QString nameFilter = (m_nameFilter && !m_popupMode) ? m_nameFilter->text() : QString();
    QString caseFilter = caseFilterSrc ? caseFilterSrc->currentText() : (m_popupMode ? "全部类型" : "全部工况");

    for (int id : allIds)
    {
        if (typeFilter != "全部类型" && objectTypeNameOf(id) != typeFilter)
            continue;
        if (!nameFilter.isEmpty() && !objectNameOf(id).contains(nameFilter, Qt::CaseInsensitive))
            continue;
        m_ids.push_back(id);
    }
    const QString allCasesText = m_popupMode ? "全部类型" : "全部工况";
    if (caseFilter != allCasesText)
    {
        std::vector<int> filteredCaseIds;
        for (int caseId : m_caseIds)
        {
            //弹窗模式按工况类型过滤，内嵌模式按工况名过滤
            QString key = m_popupMode ? caseMgr.getCaseTypeNameByID(caseId) : caseMgr.getCaseNameByID(caseId);
            if (key == caseFilter)
                filteredCaseIds.push_back(caseId);
        }
        m_caseIds = filteredCaseIds;
    }

    m_table->setRowCount((int)m_ids.size());
    m_table->setColumnCount((int)m_caseIds.size());

    // 设置垂直表头（行名称）
    for (int i = 0; i < (int)m_ids.size(); i++)
    {
        int id = m_ids[i];
        QString name;
        if (itemType == PredefinedFieldManagerParentItem)
            name = pfMgr.getPredefinedFieldNameByID(id);
        else if (itemType == ContactParentItem)
            name = contactMgr.getContactData(id)->getContactName();
        else if (itemType == ModelFixParentItem)
            name = fixMgr.getModelFixNameByID(id);
        else
            name = lbMgr.getLoadBoundaryNameByID(id);
        m_table->setVerticalHeaderItem(i, new QTableWidgetItem(name));
    }

    // 设置水平表头（工况名称）
    for (int i = 0; i < (int)m_caseIds.size(); i++)
    {
        int caseId = m_caseIds[i];
        QString name = caseMgr.getCaseNameByID(caseId);
        m_table->setHorizontalHeaderItem(i, new QTableWidgetItem(name));
    }

    // 缓存工况类型，避免循环内重复查询
    std::vector<MxFunctions::CaseType> caseTypes;
    caseTypes.reserve(m_caseIds.size());
    for (int caseId : m_caseIds)
    {
        auto caseData = caseMgr.getCaseData(caseId);
        caseTypes.push_back(caseData ? caseData->getCaseType() : MxFunctions::CaseType(-1));
    }

    // 逐行填充单元格
    for (int row = 0; row < (int)m_ids.size(); ++row)
    {
        int dataId = m_ids[row];
        bool forceChecked = false;
        bool checkBoxDisabled = false;

        MxFunctions::LoadBoundaryType lbType = MxFunctions::LoadBoundaryType(-1);
        MxFunctions::PredefinedFieldType pfType = MxFunctions::PredefinedFieldType(-1);

        // 计算行级禁用状态
        if (itemType == PredefinedFieldManagerParentItem)
        {
            auto pfData = pfMgr.getPredefinedFieldData(dataId);
            if (pfData)pfType = pfData->getPredefinedFieldType();
        }
        else if (itemType == LoadManagerParentItem || itemType == BoundaryManagerParentItem)
        {
            auto lbData = lbMgr.getLoadBoundaryData(dataId);
            if (lbData)lbType = lbData->getLoadBoundaryType();
        }

        if (itemType == ContactParentItem)
        {
            auto contactData = contactMgr.getContactData(dataId);
            auto contactType = contactData->getContactType();
            if (contactType != InterferenceContact)
            {
                forceChecked = true;
                checkBoxDisabled = true;
            }
            if (!checkBoxDisabled && contactData->isSuppressed())
            {
                checkBoxDisabled = true;
                for (int caseId : m_caseIds)
                {
                    mapMgr.deleteMapRelation({ Physical_Case, caseId },{ Physical_Contact, dataId });
                    emit mMechGlobalSignals::getInstance()->updateContactTreeView(caseId, dataId, 1);
                }
            }
        }
        else if (itemType == LoadManagerParentItem || itemType == BoundaryManagerParentItem)
        {
            auto loadBoundary = lbMgr.getLoadBoundaryData(dataId);
            if (loadBoundary && loadBoundary->isSuppressed())
            {
                checkBoxDisabled = true;
                for (int caseId : m_caseIds)
                {
                    mapMgr.deleteMapRelation({ Physical_Case, caseId },{ Physical_LoadBoundary, dataId });
                    emit mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseId, dataId, 1);
                }
            }
        }

        // 预取该行的映射关系
        std::unordered_map<int, double> caseMap;
        std::unordered_map<int, std::unordered_set<int>> modelFixCaseMap;
        bool isModelFixItem = (itemType == ModelFixParentItem);

        if (!checkBoxDisabled)
        {
            if (itemType == PredefinedFieldManagerParentItem)
                caseMap = mapMgr.getMapRelation<std::unordered_map<int, double>>({ Physical_PredefinedField, dataId }, Physical_Case);
            else if (itemType == ContactParentItem)
                caseMap = mapMgr.getMapRelation<std::unordered_map<int, double>>({ Physical_Contact, dataId }, Physical_Case);
            else if (itemType == ModelFixParentItem)
            {
                for (int cId : m_caseIds)
                {
                    QList<int> modelFixIds = mapMgr.getMapRelation<QList<int>>({ Physical_Case, cId }, Physical_ModelFix);
                    modelFixCaseMap[cId] = std::unordered_set<int>(modelFixIds.begin(), modelFixIds.end());
                }
            }
            else
                caseMap = mapMgr.getMapRelation<std::unordered_map<int, double>>({ Physical_LoadBoundary, dataId }, Physical_Case);
        }

        // 逐列创建Item
        for (int col = 0; col < (int)m_caseIds.size(); ++col)
        {
            int caseId = m_caseIds[col];
            bool isChecked;
            bool cellDisabled = checkBoxDisabled;

            if (checkBoxDisabled)
                isChecked = forceChecked;
            else if (isModelFixItem)
                isChecked = modelFixCaseMap[caseId].count(dataId) > 0;
            else
                isChecked = caseMap.count(caseId) > 0;

            // 根据菜单栏按钮校验可用性
            if (!cellDisabled && m_menuWidget)
            {
                bool hasBtn = false;
                MxFunctions::CaseType caseType = caseTypes[col];
                if (itemType == PredefinedFieldManagerParentItem)
                    hasBtn = m_menuWidget->hasCasePredefinedFieldBtn(caseType, pfType);
                else if (itemType == LoadManagerParentItem || itemType == BoundaryManagerParentItem)
                    hasBtn = m_menuWidget->hasCaseLoadBoundaryBtn(caseType, lbType);
                else if (itemType == ModelFixParentItem)
                    hasBtn = true;

                if (!hasBtn)
                {
                    cellDisabled = true;
                    isChecked = false;
                    // 清除无效映射
                    if (itemType == PredefinedFieldManagerParentItem)
                    {
                        mapMgr.deleteMapRelation({ Physical_Case, caseId }, { Physical_PredefinedField, dataId });
                        emit mMechGlobalSignals::getInstance()->updatePredefinedFieldTreeView(caseId, dataId, 1);
                    }
                    else if (itemType == ModelFixParentItem)
                    {
                        auto modelFixIds = mapMgr.getMapRelation<QList<int>>({ Physical_Case, caseId }, Physical_ModelFix);
                        int idx = modelFixIds.indexOf(dataId);
                        if (idx >= 0)
                            mapMgr.deleteTabelMapRelation({ Physical_Case, caseId }, { Physical_ModelFix, -1 }, idx);
                        emit mMechGlobalSignals::getInstance()->createUpdateModelFixSig(false, dataId);
                    }
                    else
                    {
                        mapMgr.deleteMapRelation({ Physical_Case, caseId }, { Physical_LoadBoundary, dataId });
                        emit mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseId, dataId, 1);
                    }
                }
            }

            // 创建轻量Item，存储状态
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setData(Qt::CheckStateRole, isChecked);
            item->setData(Qt::UserRole, !cellDisabled); // UserRole存储启用状态
            item->setTextAlignment(Qt::AlignCenter);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            m_table->setItem(row, col, item);
        }
    }

    resizeHeadersToContents();
    // 重建类型筛选下拉并恢复原选中项；若恢复的类型仍需要过滤，则以最终状态重排一次
    refreshTypeFilterCombo(previousTypeFilter);
    if (m_typeFilter && m_typeFilter->currentText() != typeFilter)
    {
        refreshTable(itemType);
        return;
    }
    // 弹窗模式：同步"全部激活"按钮文字与底部选中信息
    if (m_popupMode)
    {
        syncActivateAllBtnText();
        int row = m_table->currentRow();
        int col = m_table->currentColumn();
        syncSelectionInfo(row, col);
    }
}

// 筛选条件变化：按当前筛选重排矩阵（数据不丢失，仅行列显示变化）
void mManagerTableWidget::onFilterChanged()
{
    refreshTable(m_currentType);
}

QString mManagerTableWidget::objectNameOf(int dataId) const
{
    auto modelData = getCVM->getModelData();
    if (m_currentType == PredefinedFieldManagerParentItem)
        return modelData->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldNameByID(dataId);
    if (m_currentType == ContactParentItem)
    {
        auto contactData = modelData->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(dataId);
        return contactData ? contactData->getContactName() : QString();
    }
    if (m_currentType == ModelFixParentItem)
        return modelData->getManager<mModelFixManager>(Physical_ModelFix).getModelFixNameByID(dataId);
    return modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryNameByID(dataId);
}

QString mManagerTableWidget::objectTypeNameOf(int dataId) const
{
    auto modelData = getCVM->getModelData();
    if (m_currentType == PredefinedFieldManagerParentItem)
        return modelData->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldTypeNameByID(dataId);
    if (m_currentType == ContactParentItem)
    {
        auto contactData = modelData->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(dataId);
        return contactData ? contactData->getContactTypeStr() : QString();
    }
    if (m_currentType == ModelFixParentItem)
    {
        auto modelFixData = modelData->getManager<mModelFixManager>(Physical_ModelFix).getModelFixData(dataId);
        return modelFixData ? modelFixData->getModelFixType() : QString();
    }
    return modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryTypeNameByID(dataId);
}

void mManagerTableWidget::refreshTypeFilterCombo(const QString& restoreType)
{
    if (m_typeFilter == nullptr || getCVM == nullptr)
        return;

    // 保留原选中类型，重建后若仍存在则恢复选中
    QString currentType = restoreType.isEmpty() ? m_typeFilter->currentText() : restoreType;
    m_typeFilter->blockSignals(true);
    m_typeFilter->clear();
    m_typeFilter->addItem("全部类型");

    // 动态列举当前类型下实际存在的对象类型（按id顺序去重；std::unordered_set无QString哈希，用QSet）
    QSet<QString> addedTypes;
    std::vector<int> allIds;
    auto modelData = getCVM->getModelData();
    if (m_currentType == LoadManagerParentItem)
        allIds = modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadID();
    else if (m_currentType == BoundaryManagerParentItem)
        allIds = modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllBoundaryID();
    else if (m_currentType == PredefinedFieldManagerParentItem)
        allIds = modelData->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).getAllPredefinedFieldID();
    else if (m_currentType == ContactParentItem)
        allIds = modelData->getManager<MechContactData::mContactManager>(Physical_Contact).getAllContactID();
    else if (m_currentType == ModelFixParentItem)
        allIds = modelData->getManager<mModelFixManager>(Physical_ModelFix).getAllModelFixID();

    for (int id : allIds)
    {
        QString typeName = objectTypeNameOf(id);
        if (!typeName.isEmpty() && !addedTypes.contains(typeName))
        {
            addedTypes.insert(typeName);
            m_typeFilter->addItem(typeName);
        }
    }

    int typeIndex = m_typeFilter->findText(currentType);
    m_typeFilter->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
    m_typeFilter->blockSignals(false);

    // 弹窗模式的类型下拉与内嵌筛选共用同一套选项
    if (m_popupTypeFilter)
    {
        m_popupTypeFilter->blockSignals(true);
        m_popupTypeFilter->clear();
        for (int i = 0; i < m_typeFilter->count(); ++i)
            m_popupTypeFilter->addItem(m_typeFilter->itemText(i));
        int popupIndex = m_popupTypeFilter->findText(currentType);
        m_popupTypeFilter->setCurrentIndex(popupIndex >= 0 ? popupIndex : 0);
        m_popupTypeFilter->blockSignals(false);
    }
}

// 勾选状态变化：局部更新，不再全量刷新
void mManagerTableWidget::onCellCheckStateChanged(int row, int col, bool checked)
{
    if (row < 0 || col < 0 || row >= (int)m_ids.size() || col >= (int)m_caseIds.size())
        return;

    int dataId = m_ids[row];
    int caseId = m_caseIds[col];
    auto& mapMgr = getCVM->getModelData()->getMapRelationManager();

    // 弹窗模式：树子节点挂载/移除直接调用mTreeView接口（避开全局信号在勾选事件栈中触发的树节点操作），
    // 内嵌模式保持原全局信号通路（Table dock场景兼容）
    auto* tree = mBrowserWindow::getInstance()->getBrowserTree();

    // 更新数据映射（业务逻辑与原代码完全一致）
    if (m_currentType == PredefinedFieldManagerParentItem)
    {
        if (checked)
        {
            mapMgr.appendMapRelation({ Physical_PredefinedField, dataId }, { Physical_Case, caseId });
            if (m_popupMode) tree->addCasePredefinedFieldChildItem(caseId, dataId);
            else emit mMechGlobalSignals::getInstance()->updatePredefinedFieldTreeView(caseId, dataId, 0);
        }
        else
        {
            mapMgr.deleteMapRelation({ Physical_Case, caseId }, { Physical_PredefinedField, dataId });
            if (m_popupMode) tree->removeCasePredefinedFieldChildItem(caseId, dataId);
            else emit mMechGlobalSignals::getInstance()->updatePredefinedFieldTreeView(caseId, dataId, 1);
        }
    }
    else if (m_currentType == ContactParentItem)
    {
        if (checked)
        {
            mapMgr.appendMapRelation({ Physical_Contact, dataId }, { Physical_Case, caseId });
            emit mMechGlobalSignals::getInstance()->updateContactTreeView(caseId, dataId, 0);
        }
        else
        {
            mapMgr.deleteMapRelation({ Physical_Case, caseId }, { Physical_Contact, dataId });
            emit mMechGlobalSignals::getInstance()->updateContactTreeView(caseId, dataId, 1);
        }
    }
    else if (m_currentType == ModelFixParentItem)
    {
        if (checked)
        {
            QList<int> modelFixIds = mapMgr.getMapRelation<QList<int>>({ Physical_Case, caseId }, Physical_ModelFix);
            mapMgr.appendTableMapRelation({ Physical_Case, caseId }, { Physical_ModelFix, dataId }, modelFixIds.size());
            emit mMechGlobalSignals::getInstance()->updateModelFixTreeView(caseId, dataId, 0);
        }
        else
        {
            QList<int> modelFixIds = mapMgr.getMapRelation<QList<int>>({ Physical_Case, caseId }, Physical_ModelFix);
            int idx = modelFixIds.indexOf(dataId);
            if (idx >= 0)
                mapMgr.deleteTabelMapRelation({ Physical_Case, caseId }, { Physical_ModelFix, -1 }, idx);
            emit mMechGlobalSignals::getInstance()->updateModelFixTreeView(caseId, dataId, 1);
        }
    }
    else // 载荷/边界
    {
        auto lbData = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(dataId);
        bool isLoad = lbData && lbData->isLoadOrBoundary() == 0;
        if (checked)
        {
            mapMgr.appendMapRelation({ Physical_LoadBoundary, dataId }, { Physical_Case, caseId });
            if (m_popupMode)
            {
                if (isLoad) tree->addCaseLoadChildItem(caseId, dataId);
                else tree->addCaseBoundaryChildItem(caseId, dataId);
            }
            else emit mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseId, dataId, 0);
        }
        else
        {
            mapMgr.deleteMapRelation({ Physical_Case, caseId }, { Physical_LoadBoundary, dataId });
            if (m_popupMode)
            {
                if (isLoad) tree->removeCaseLoadChildItem(caseId, dataId);
                else tree->removeCaseBoundaryChildItem(caseId, dataId);
            }
            else emit mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseId, dataId, 1);
        }
    }

    // 仅更新当前单元格显示，不重建整张表
    QTableWidgetItem* item = m_table->item(row, col);
    if (item)
        item->setData(Qt::CheckStateRole, checked);

    // 弹窗模式：勾选变化后同步"全部激活/取消"按钮文字
    if (m_popupMode)
        syncActivateAllBtnText();

    // 同步触发信息刷新，与原行为一致
    onCellClicked(row, col);
}

void mManagerTableWidget::onCellClicked(int row, int col)
{
    if (row < 0 || col < 0) return;
    if (row >= (int)m_ids.size() || col >= (int)m_caseIds.size()) return;

    int dataId = m_ids[row];
    int caseId = m_caseIds[col];
    auto modelData = getCVM->getModelData();
    auto& caseMgr = modelData->getManager<MechCaseData::mCaseManager>(Physical_Case);

    QString caseTypeName = caseMgr.getCaseTypeNameByID(caseId);
    QString type = "";

    if (m_currentType == LoadManagerParentItem)
        type = "载荷类型";
    else if (m_currentType == BoundaryManagerParentItem)
        type = "边界类型";
    else if (m_currentType == PredefinedFieldManagerParentItem)
        type = "预定义场类型";
    else if (m_currentType == ContactParentItem)
        type = "接触类型";
    else if (m_currentType == ModelFixParentItem)
        type = "模型修改类型";

    QString loadBoundTypeName;
    if (m_currentType == PredefinedFieldManagerParentItem)
        loadBoundTypeName = modelData->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldTypeNameByID(dataId);
    else if (m_currentType == ContactParentItem)
        loadBoundTypeName = modelData->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(dataId)->getContactTypeStr();
    else if (m_currentType == ModelFixParentItem)
    {
        auto modelFixData = modelData->getManager<mModelFixManager>(Physical_ModelFix).getModelFixData(dataId);
        if (modelFixData)
            loadBoundTypeName = modelFixData->getModelFixType();
    }
    else
        loadBoundTypeName = modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryTypeNameByID(dataId);

    QString text = QString(R"(%1：%2<br/>分析类型：%3)")
        .arg(type).arg(loadBoundTypeName).arg(caseTypeName);

    m_infoEdit->setText(text);

    // 弹窗模式：底部选中信息区展示"选中载荷/选中工况"
    if (m_popupMode)
        syncSelectionInfo(row, col);
}

// 全部激活/取消：勾选或取消当前显示的全部可勾选单元格
void mManagerTableWidget::slot_activateAllBtn()
{
    auto modelData = getCVM->getModelData();
    if (modelData == nullptr)
        return;
    auto& mapMgr = modelData->getMapRelationManager();

    // 收集当前显示的可勾选单元格
    struct CellRef { int row; int col; int dataId; int caseId; bool checked; };
    std::vector<CellRef> cells;
    bool allChecked = true;
    for (int row = 0; row < (int)m_ids.size(); ++row)
    {
        for (int col = 0; col < (int)m_caseIds.size(); ++col)
        {
            QTableWidgetItem* item = m_table->item(row, col);
            if (item == nullptr || !item->data(Qt::UserRole).toBool())
                continue; // 禁用单元格跳过（强制挂载/被抑制/工况不支持）
            bool checked = item->data(Qt::CheckStateRole).toBool();
            cells.push_back({ row, col, m_ids[row], m_caseIds[col], checked });
            if (!checked)
                allChecked = false;
        }
    }
    if (cells.empty())
        return;

    // 全部勾选则取消全部，否则激活全部
    bool target = !allChecked;
    for (const CellRef& cell : cells)
    {
        if (cell.checked == target)
            continue;
        if (m_currentType == PredefinedFieldManagerParentItem)
        {
            if (target)
            {
                mapMgr.appendMapRelation({ Physical_PredefinedField, cell.dataId }, { Physical_Case, cell.caseId });
                emit mMechGlobalSignals::getInstance()->updatePredefinedFieldTreeView(cell.caseId, cell.dataId, 0);
            }
            else
            {
                mapMgr.deleteMapRelation({ Physical_Case, cell.caseId }, { Physical_PredefinedField, cell.dataId });
                emit mMechGlobalSignals::getInstance()->updatePredefinedFieldTreeView(cell.caseId, cell.dataId, 1);
            }
        }
        else if (m_currentType == ContactParentItem)
        {
            if (target)
            {
                mapMgr.appendMapRelation({ Physical_Contact, cell.dataId }, { Physical_Case, cell.caseId });
                emit mMechGlobalSignals::getInstance()->updateContactTreeView(cell.caseId, cell.dataId, 0);
            }
            else
            {
                mapMgr.deleteMapRelation({ Physical_Case, cell.caseId }, { Physical_Contact, cell.dataId });
                emit mMechGlobalSignals::getInstance()->updateContactTreeView(cell.caseId, cell.dataId, 1);
            }
        }
        else if (m_currentType == ModelFixParentItem)
        {
            if (target)
            {
                QList<int> modelFixIds = mapMgr.getMapRelation<QList<int>>({ Physical_Case, cell.caseId }, Physical_ModelFix);
                mapMgr.appendTableMapRelation({ Physical_Case, cell.caseId }, { Physical_ModelFix, cell.dataId }, modelFixIds.size());
                emit mMechGlobalSignals::getInstance()->updateModelFixTreeView(cell.caseId, cell.dataId, 0);
            }
            else
            {
                QList<int> modelFixIds = mapMgr.getMapRelation<QList<int>>({ Physical_Case, cell.caseId }, Physical_ModelFix);
                int idx = modelFixIds.indexOf(cell.dataId);
                if (idx >= 0)
                    mapMgr.deleteTabelMapRelation({ Physical_Case, cell.caseId }, { Physical_ModelFix, -1 }, idx);
                emit mMechGlobalSignals::getInstance()->updateModelFixTreeView(cell.caseId, cell.dataId, 1);
            }
        }
        else // 载荷/边界
        {
            if (target)
            {
                mapMgr.appendMapRelation({ Physical_LoadBoundary, cell.dataId }, { Physical_Case, cell.caseId });
                emit mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(cell.caseId, cell.dataId, 0);
            }
            else
            {
                mapMgr.deleteMapRelation({ Physical_Case, cell.caseId }, { Physical_LoadBoundary, cell.dataId });
                emit mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(cell.caseId, cell.dataId, 1);
            }
        }
        if (QTableWidgetItem* item = m_table->item(cell.row, cell.col))
            item->setData(Qt::CheckStateRole, target);
    }

    syncActivateAllBtnText();
}

// 当前显示的全部可勾选单元格均已勾选时显示"全部取消"，否则显示"全部激活"
void mManagerTableWidget::syncActivateAllBtnText()
{
    if (m_btnActivateAll == nullptr)
        return;
    bool anyEnabled = false;
    bool allChecked = true;
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        for (int col = 0; col < m_table->columnCount(); ++col)
        {
            QTableWidgetItem* item = m_table->item(row, col);
            if (item == nullptr || !item->data(Qt::UserRole).toBool())
                continue;
            anyEnabled = true;
            if (!item->data(Qt::CheckStateRole).toBool())
            {
                allChecked = false;
                break;
            }
        }
        if (!allChecked)
            break;
    }
    m_btnActivateAll->setEnabled(anyEnabled);
    m_btnActivateAll->setText(anyEnabled && allChecked ? "全部取消" : "全部激活");
}

// 底部选中信息区：选中载荷（名称+类型）/ 选中工况（名称+分析类型）
void mManagerTableWidget::syncSelectionInfo(int row, int col)
{
    if (m_selectedObjectLabel == nullptr || m_selectedCaseLabel == nullptr)
        return;
    QString objectText;
    QString caseText;
    if (row >= 0 && row < (int)m_ids.size())
    {
        int dataId = m_ids[row];
        objectText = QString("%1  %2").arg(objectNameOf(dataId), objectTypeNameOf(dataId));
    }
    if (col >= 0 && col < (int)m_caseIds.size())
    {
        int caseId = m_caseIds[col];
        auto& caseMgr = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case);
        caseText = QString("%1  %2").arg(caseMgr.getCaseNameByID(caseId), caseMgr.getCaseTypeNameByID(caseId));
    }
    m_selectedObjectLabel->setText(objectText);
    m_selectedCaseLabel->setText(caseText);
}

void mManagerTableWidget::onEditBtnClicked()
{
    QModelIndex currentIndex = m_table->currentIndex();
    if (!currentIndex.isValid())
        return;

    int row = currentIndex.row();
    int col = currentIndex.column();
    int dataId = m_ids[row];
    int caseId = m_caseIds[col];

    // 从Item数据中获取勾选状态
    QTableWidgetItem* item = m_table->item(row, col);
    bool checked = item ? item->data(Qt::CheckStateRole).toBool() : false;

    if (m_currentType == PredefinedFieldManagerParentItem)
    {
        mInfoWindow::getInstance()->unEditPredefinedField();
        checked ? mInfoWindow::getInstance()->editPredefinedField(dataId, caseId)
            : mInfoWindow::getInstance()->editPredefinedField(dataId, -1);
        return;
    }

    if (m_currentType == ContactParentItem)
    {
        mInfoWindow::getInstance()->unEditContact();
        mInfoWindow::getInstance()->editContact(dataId);
        return;
    }

    if (m_currentType == ModelFixParentItem)
    {
        mInfoWindow::getInstance()->unEditContact();
        mInfoWindow::getInstance()->editModelFix(dataId);
        return;
    }

    auto data = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(dataId);
    if (data->isLoadOrBoundary() == 0)
    {
        mInfoWindow::getInstance()->unEditLoad();
        checked ? mInfoWindow::getInstance()->editLoad(dataId, caseId)
            : mInfoWindow::getInstance()->editLoad(dataId, -1);
    }
    else if (data->isLoadOrBoundary() == 1)
    {
        mInfoWindow::getInstance()->unEditBoundary();
        checked ? mInfoWindow::getInstance()->editBoundary(dataId, caseId)
            : mInfoWindow::getInstance()->editBoundary(dataId, -1);
    }
}

QList<int> mManagerTableWidget::getSelectedRows() const
{
    QList<int> rows;
    const auto& selected = m_table->selectionModel()->selectedIndexes();
    for (const QModelIndex& index : selected)
    {
        int row = index.row();
        if (row >= 0 && row < (int)m_ids.size() && !rows.contains(row))
            rows.append(row);
    }
    if (rows.isEmpty())
    {
        int row = m_table->currentIndex().row();
        if (row >= 0 && row < (int)m_ids.size())
            rows.append(row);
    }
    std::sort(rows.begin(), rows.end());
    return rows;
}

void mManagerTableWidget::copySingleRow(int row)
{
    if (row < 0 || row >= (int)m_ids.size())
        return;

    int srcDataId = m_ids[row];
    auto modelData = getCVM->getModelData();
    auto& mapMgr = modelData->getMapRelationManager();

    // 从Item中读取勾选状态，无需遍历Widget
    QList<bool> checkStates;
    for (int col = 0; col < (int)m_caseIds.size(); col++)
    {
        QTableWidgetItem* item = m_table->item(row, col);
        checkStates.append(item ? item->data(Qt::CheckStateRole).toBool() : false);
    }

    if (m_currentType == PredefinedFieldManagerParentItem)
    {
        auto& pfMgr = modelData->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField);
        int copyId = pfMgr.copyPredefinedFieldData(srcDataId);
        auto newField = pfMgr.getPredefinedFieldData(copyId);
        if (!newField) return;

        for (int col = 0; col < (int)m_caseIds.size(); col++)
        {
            int caseId = m_caseIds[col];
            if (checkStates[col])
            {
                mapMgr.appendMapRelation({ Physical_PredefinedField, copyId }, { Physical_Case, caseId });
                emit mMechGlobalSignals::getInstance()->updatePredefinedFieldTreeView(caseId, copyId, 0);
            }
        }
        emit mMechGlobalSignals::getInstance()->updatePredefinedFieldTreeView(-1, copyId, 0);
        return;
    }

    if (m_currentType == ContactParentItem)
    {
        int copyId = modelData->copyPhysicalField(Physical_Contact, srcDataId, -1);
        if (copyId < 0) return;

        auto& contactMgr = modelData->getManager<MechContactData::mContactManager>(Physical_Contact);
        auto contactType = contactMgr.getContactData(copyId)->getContactType();

        if (contactType == InterferenceContact)
        {
            for (int col = 0; col < (int)m_caseIds.size(); col++)
            {
                int caseId = m_caseIds[col];
                if (checkStates[col])
                {
                    mapMgr.appendMapRelation({ Physical_Contact, copyId }, { Physical_Case, caseId });
                    emit mMechGlobalSignals::getInstance()->updateContactTreeView(caseId, copyId, 0);
                }
            }
        }
        emit mMechGlobalSignals::getInstance()->updateContactTreeView(-1, copyId, 0);
        return;
    }

    if (m_currentType == ModelFixParentItem)
    {
        int copyId = modelData->copyPhysicalField(Physical_ModelFix, srcDataId, -1);
        if (copyId < 0) return;

        for (int col = 0; col < (int)m_caseIds.size(); col++)
        {
            int caseId = m_caseIds[col];
            if (checkStates[col])
            {
                QList<int> modelFixIds = mapMgr.getMapRelation<QList<int>>({ Physical_Case, caseId }, Physical_ModelFix);
                mapMgr.appendTableMapRelation({ Physical_Case, caseId }, { Physical_ModelFix, copyId }, modelFixIds.size());
            }
        }
        emit mMechGlobalSignals::getInstance()->createUpdateModelFixSig(true, copyId);
        return;
    }

    int copyId = modelData->copyPhysicalField(Physical_LoadBoundary, srcDataId, -1);
    auto newLoadBoundary = modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(copyId);
    if (!newLoadBoundary) return;

    for (int col = 0; col < (int)m_caseIds.size(); col++)
    {
        int caseId = m_caseIds[col];
        if (checkStates[col])
        {
            mapMgr.appendMapRelation({ Physical_LoadBoundary, copyId }, { Physical_Case, caseId });
            emit mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseId, copyId, 0);
        }
    }
    emit mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(-1, copyId, 0);
}

void mManagerTableWidget::onCopyBtnClicked()
{
    QList<int> selectedRows = getSelectedRows();
    if (selectedRows.isEmpty())
        return;

    for (int row : selectedRows)
        copySingleRow(row);

    refreshTable(m_currentType);
}

void mManagerTableWidget::deleteSingleRow(int row)
{
    if (row < 0 || row >= (int)m_ids.size())
        return;

    int delDataId = m_ids[row];
    auto modelData = getCVM->getModelData();
    auto& mapMgr = modelData->getMapRelationManager();

    if (m_currentType == PredefinedFieldManagerParentItem)
    {
        for (int col = 0; col < (int)m_caseIds.size(); col++)
        {
            int caseId = m_caseIds[col];
            auto caseMap = mapMgr.getMapRelation<std::unordered_map<int, double>>({ Physical_PredefinedField, delDataId }, Physical_Case);
            if (caseMap.count(caseId))
            {
                mapMgr.deleteMapRelation({ Physical_PredefinedField, delDataId }, { Physical_Case, caseId });
                emit mMechGlobalSignals::getInstance()->updatePredefinedFieldTreeView(caseId, delDataId, 1);
            }
        }
        emit mMechGlobalSignals::getInstance()->updatePredefinedFieldTreeView(-1, delDataId, 1);
        modelData->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).deletePredefinedField(delDataId);
        return;
    }

    if (m_currentType == ContactParentItem)
    {
        auto& contactMgr = modelData->getManager<MechContactData::mContactManager>(Physical_Contact);
        auto contactType = contactMgr.getContactData(delDataId)->getContactType();

        if (contactType == InterferenceContact)
        {
            for (int col = 0; col < (int)m_caseIds.size(); col++)
            {
                int caseId = m_caseIds[col];
                auto caseMap = mapMgr.getMapRelation<std::unordered_map<int, double>>({ Physical_Contact, delDataId }, Physical_Case);
                if (caseMap.count(caseId))
                {
                    mapMgr.deleteMapRelation({ Physical_Contact, delDataId }, { Physical_Case, caseId });
                    emit mMechGlobalSignals::getInstance()->updateContactTreeView(caseId, delDataId, 1);
                }
            }
        }
        emit mMechGlobalSignals::getInstance()->updateContactTreeView(-1, delDataId, 1);
        //先通知面板丢弃悬垂指针（此时接触对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyContactDataAboutToBeDestroyed(delDataId);
        modelData->deleteByPhysicalType(Physical_Contact, delDataId);
        return;
    }

    if (m_currentType == ModelFixParentItem)
    {
        for (int col = 0; col < (int)m_caseIds.size(); col++)
        {
            int caseId = m_caseIds[col];
            auto modelFixIds = mapMgr.getMapRelation<QList<int>>({ Physical_Case, caseId }, Physical_ModelFix);
            int idx = modelFixIds.indexOf(delDataId);
            if (idx >= 0)
                mapMgr.deleteTabelMapRelation({ Physical_Case, caseId }, { Physical_ModelFix, -1 }, idx);
        }
        modelData->deleteByPhysicalType(Physical_ModelFix, delDataId);
        emit mMechGlobalSignals::getInstance()->createUpdateModelFixSig(false, delDataId);
        return;
    }

    for (int col = 0; col < (int)m_caseIds.size(); col++)
    {
        int caseId = m_caseIds[col];
        auto caseMap = mapMgr.getMapRelation<std::unordered_map<int, double>>({ Physical_LoadBoundary, delDataId }, Physical_Case);
        if (caseMap.count(caseId))
        {
            mapMgr.deleteMapRelation({ Physical_LoadBoundary, delDataId }, { Physical_Case, caseId });
            emit mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseId, delDataId, 1);
        }
    }
    emit mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(-1, delDataId, 1);
    modelData->deleteByPhysicalType(Physical_LoadBoundary, delDataId);
}

void mManagerTableWidget::onDeleteBtnClicked()
{
    QList<int> selectedRows = getSelectedRows();
    if (selectedRows.isEmpty())
        return;

    // 倒序删除，避免索引偏移
    for (int i = selectedRows.size() - 1; i >= 0; --i)
        deleteSingleRow(selectedRows[i]);

    refreshTable(m_currentType);
}

void mManagerTableWidget::resizeHeadersToContents()
{
    // 行头按内容收缩；列头使用固定宽度（默认96px），避免工况列多时逐列计算尺寸的开销
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_table->horizontalHeader()->setDefaultSectionSize(96);
    m_table->horizontalHeader()->setStretchLastSection(false);
    // 行数/列数变化后强制表头与视口几何重排（Fixed模式下部分刷新路径不触发布局更新）
    m_table->horizontalHeader()->updateGeometry();
    m_table->verticalHeader()->updateGeometry();
    m_table->updateGeometry();
}

void mManagerTableWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_infoEdit->setFixedHeight(m_table->viewport()->height() + m_table->horizontalHeader()->height());
}

void mManagerTableWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_infoEdit->setFixedHeight(m_table->viewport()->height() + m_table->horizontalHeader()->height());
}