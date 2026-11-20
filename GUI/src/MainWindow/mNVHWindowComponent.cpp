#include "mNVHWindowComponent.h"
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QTableWidget>
#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QPair>
#include <QHeaderView>
#include <QLineEdit>
#include <QCheckBox>
#include <QPainter>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPixmap>
#include <QResizeEvent>
#include <QTimer>
#include <algorithm>
#include "mOtherFunction.h"
#include <QComboBox>
#include "mPlotEnum.h"
#include "mPlotView.h"
#include <QButtonGroup>
#include "mPlotViewStyleWidget.h"

/*****************************************NavIconDelegate******************************************************/
NavIconDelegate::NavIconDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
    , m_iconAnalysisActive(":/mMainWidgets/Icon/NVH/jihuo.svg")
    , m_iconAnalysisInactive(":/mMainWidgets/Icon/NVH/weijihuo.svg")
    , m_iconOutputActive(":/mMainWidgets/Icon/NVH/shuchu_jihuo.svg")
    , m_iconOutputInactive(":/mMainWidgets/Icon/NVH/shuchu_weijihuo.svg")
    , m_iconOutputClickable(":/mMainWidgets/Icon/NVH/shuchu_kedianji.svg")
    , m_iconCompleted(":/mMainWidgets/Icon/NVH/dagou.svg")
{
}

void NavIconDelegate::setPageCompleted(int pageIndex, bool completed)
{
    m_pageCompleted[pageIndex] = completed;
}

bool NavIconDelegate::isPageCompleted(int pageIndex) const
{
    return m_pageCompleted.value(pageIndex, false);
}

void NavIconDelegate::setCurrentPage(int pageIndex)
{
    m_currentPage = pageIndex;
}

int NavIconDelegate::currentPage() const
{
    return m_currentPage;
}

void NavIconDelegate::setPageEnabled(int pageIndex, bool enabled)
{
    m_pageEnabled[pageIndex] = enabled;
}

bool NavIconDelegate::isPageEnabled(int pageIndex) const
{
    return m_pageEnabled.value(pageIndex, false);
}

void NavIconDelegate::setOutputPageIndex(int pageIndex)
{
    m_outputPageIndex = pageIndex;
}

int NavIconDelegate::outputPageIndex() const
{
    return m_outputPageIndex;
}

void NavIconDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const bool isSelected = (opt.state & QStyle::State_Selected) || (opt.state & QStyle::State_MouseOver);
    const int pageIndex = index.data(Qt::UserRole).toInt();
    const bool completed = isPageCompleted(pageIndex);
    const bool enabled = isPageEnabled(pageIndex);
    const bool isOutput = (m_outputPageIndex >= 0 && pageIndex == m_outputPageIndex);

    const QIcon& icon = completed
        ? m_iconCompleted
        : (isOutput
            ? (isSelected ? m_iconOutputActive : (enabled ? m_iconOutputClickable : m_iconOutputInactive))
            : (isSelected ? m_iconAnalysisActive : m_iconAnalysisInactive));

    const int iconSize = 16;
    const int leftMargin = 16;
    const int lineWidth = 2;
    const int lineX = leftMargin + iconSize / 2 - lineWidth / 2;
    const int iconHalf = iconSize / 2;

    // 绘制项背景（透明）
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->fillRect(opt.rect, Qt::transparent);

    const int centerY = opt.rect.top() + opt.rect.height() / 2;

    // 绘制上方连接线（除第一项）：从上一项边缘连到本项图标上边缘
    if (index.row() > 0)
    {
        painter->setPen(QPen(QColor("#DCDFE6"), lineWidth));
        painter->drawLine(QPoint(lineX, opt.rect.top()), QPoint(lineX, centerY - iconHalf - 5));
    }

    // 绘制下方连接线（除最后一项）：从本项图标下边缘连到下一项边缘
    if (index.row() < index.model()->rowCount() - 1)
    {
        painter->setPen(QPen(QColor("#DCDFE6"), lineWidth));
        painter->drawLine(QPoint(lineX, centerY + iconHalf + 5), QPoint(lineX, opt.rect.bottom()));
    }

    // 绘制图标
    QRect iconRect(opt.rect.left() + leftMargin, centerY - iconHalf, iconSize, iconSize);
    icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, isSelected ? QIcon::On : QIcon::Off);

    // 绘制文字
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen(isSelected ? QColor("#1385F6") : (enabled ? ("#606266") : QColor("#A8ABB2")));
    QFont font;
    font.setBold(isSelected);  
    painter->setFont(font);
    int textX = opt.rect.left() + leftMargin + iconSize + 10;
    QRect textRect(textX, opt.rect.top(), opt.rect.right() - textX, opt.rect.height());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

    painter->restore();
}

QSize NavIconDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(180, 48);
}

/*****************************************ResponseSettingFoldablePanel******************************************************/
ResponseSettingFoldablePanel::ResponseSettingFoldablePanel(const QString& panelName, QWidget* parent)
    : QWidget(parent), m_isOpen(true), m_isElementForce(false)
{
    // ----- 主布局 -----
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // ----- 标题栏 -----
    m_titleWidget = new QWidget(this);
    QHBoxLayout* titleLayout = new QHBoxLayout(m_titleWidget);
    titleLayout->setContentsMargins(0, 5, 0, 5);

    m_toggleButton = new QPushButton(panelName, this);
    m_toggleButton->setIcon(getIcon(true));
    m_toggleButton->setLayoutDirection(Qt::LeftToRight);
    m_toggleButton->setContentsMargins(0, 0, 0, 0);
    m_toggleButton->setStyleSheet("QPushButton{"
        "  text-align:left; border:none; margin:0; background:#ffffff;color:#000000;font-weight:bold;}"
        "QPushButton:hover{background:#ffffff;}"
        "QPushButton:checked{background:#ffffff;}");
    connect(m_toggleButton, &QPushButton::clicked, this, &ResponseSettingFoldablePanel::onFoldClicked);
    titleLayout->addWidget(m_toggleButton);
    mainLayout->addWidget(m_titleWidget);

    // ---- - 可折叠的内容区域---- -
    m_contentWidget = new QWidget(this);
    m_contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout* contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(5, 0, 5, 0);
    contentLayout->setSpacing(0);

    // 第一行：节点个数 + 按钮
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setSpacing(10);
    topLayout->setContentsMargins(0, 0, 5, 5);

    QLabel* countLabel = new QLabel("节点个数：", this);
    m_PointCount = new QLineEdit(this);
    m_PointCount->setMinimumWidth(250);
    m_PointCount->setText("1");
    m_AddRow = new QPushButton(this);
    m_AddRow->setMaximumWidth(30);
    m_RemoveRow = new QPushButton(this);
    m_RemoveRow->setMaximumWidth(30);
    m_RemoveRow->setEnabled(false);
    m_AddRow->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
    m_AddRow->setIconSize(QSize(14, 14));
    m_RemoveRow->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
    m_RemoveRow->setIconSize(QSize(14, 14));
    m_ImportFile = new QPushButton("导入文件", this);
    m_ExportFile = new QPushButton("导出文件", this);
    m_AddRow->setAutoDefault(false);
    m_RemoveRow->setAutoDefault(false);
    m_ImportFile->setAutoDefault(false);
    m_ExportFile->setAutoDefault(false);

    topLayout->addWidget(countLabel);
    topLayout->addWidget(m_PointCount);
    topLayout->addWidget(m_AddRow);
    topLayout->addWidget(m_RemoveRow);
    topLayout->addStretch();
    topLayout->addWidget(m_ImportFile);
    topLayout->addWidget(m_ExportFile);
    contentLayout->addLayout(topLayout);

    if (panelName == "单元力设置")
    {
        m_isElementForce = true;
    }

    // ---- 表格区域 ----
    m_TableWidget = new QTableWidget(this);
    m_TableWidget->setColumnCount(2);
    QString headerText = m_isElementForce ? "单元ID" : "节点ID";
    m_TableWidget->setHorizontalHeaderLabels(QStringList() << headerText << "标签");
    // 列宽由 ResponseSettingFoldablePanel::resizeEvent 统一按 3:7 设置
    m_TableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_TableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_TableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_TableWidget->verticalHeader()->setVisible(false);
    m_TableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_TableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_TableWidget->setAlternatingRowColors(true);
    m_TableWidget->setFixedHeight(200);
    m_TableWidget->setShowGrid(false);
    m_TableWidget->setStyleSheet(
        "QTableWidget {"
        "   alternate-background-color: #F6F8FA;"   /* 奇数行背景 */
        "   background-color: #FFFFFF;"             /* 偶数行背景 */
        "   selection-background-color: #D0E7FD;"
        "    border-radius: 2px;"
        "   gridline-color: #DCDFE6;"
        "   border: 1px solid #ccc;"
        "}"
        "QTableWidget:disabled {"
        "   background-color: #F5F7FA;"
        "   alternate-background-color: #F5F7FA;"
        "   color: #808080;"
        "}"
        "QTableWidget::item {"
        "  border: none;"
        "  border-bottom: 1px solid #ccc;"
        "}"
        "QTableWidget:disabled QHeaderView::section { background-color: #E8EBF2; color: #A8ABB2; }"
    );
    m_TableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section:horizontal {"
        "   background-color: #E8EBF2;"
        "   color: #2C3E50;"
        "   border: 1px solid #DCDFE6;"
        "}"
    );
    contentLayout->addWidget(m_TableWidget, 1); // 让表格占据剩余高度
    mainLayout->addWidget(m_contentWidget);

    // 表格选择变化时同步删除按钮状态
    connect(m_TableWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ResponseSettingFoldablePanel::updateDeleteButtonState);
    connect(m_TableWidget, &QTableWidget::itemChanged,
            this, &ResponseSettingFoldablePanel::onTableItemChanged);
    connect(m_AddRow, &QPushButton::clicked, this, &ResponseSettingFoldablePanel::onAddRow);
    connect(m_RemoveRow, &QPushButton::clicked, this, &ResponseSettingFoldablePanel::onDeleteRow);
    connect(m_ImportFile, &QPushButton::clicked, this, &ResponseSettingFoldablePanel::onImportFromFile);
    connect(m_ExportFile, &QPushButton::clicked, this, &ResponseSettingFoldablePanel::onExportToFile);
    connect(m_PointCount, &QLineEdit::editingFinished, this, &ResponseSettingFoldablePanel::slot_onPointCountChanged);
    m_AddRow->click();
    m_defaultRows = m_TableWidget ? m_TableWidget->rowCount() : 0;
}

ResponseSettingFoldablePanel::~ResponseSettingFoldablePanel() {}

void ResponseSettingFoldablePanel::onFoldClicked()
{
    m_isOpen = !m_isOpen;
    m_toggleButton->setIcon(getIcon(m_isOpen));
    m_contentWidget->setVisible(m_isOpen);
}

int ResponseSettingFoldablePanel::rowCount() const
{
    if (!m_TableWidget)
    {
        return 0;
    }
    return m_TableWidget->rowCount();
}

bool ResponseSettingFoldablePanel::hasValidRow() const
{
    if (!m_TableWidget)
    {
        return false;
    }
    for (int row = 0; row < m_TableWidget->rowCount(); ++row)
    {
        QTableWidgetItem* idItem = m_TableWidget->item(row, 0);
        if (!idItem)
        {
            continue;
        }
        QString idText = idItem->text().trimmed();
        if (idText.isEmpty() || idText == "请输入")
        {
            continue;
        }
        bool ok = false;
        int id = idText.toInt(&ok);
        if (ok && id > 0)
        {
            // 只判断ID列有效即可，标签列不参与有效性判断
            return true;
        }
    }
    return false;
}

QList<QPair<int, QString>> ResponseSettingFoldablePanel::getTableIds() const
{
    QList<QPair<int, QString>> result;
    if (!m_TableWidget)
    {
        return result;
    }
    for (int row = 0; row < m_TableWidget->rowCount(); ++row)
    {
        QTableWidgetItem* idItem = m_TableWidget->item(row, 0);
        if (!idItem)
        {
            continue;
        }
        QString idText = idItem->text().trimmed();
        if (idText.isEmpty() || idText == "请输入")
        {
            continue;
        }
        bool ok = false;
        int id = idText.toInt(&ok);
        if (!ok || id <= 0)
        {
            continue;
        }

        QTableWidgetItem* labelItem = m_TableWidget->item(row, 1);
        QString labelText = labelItem ? labelItem->text().trimmed() : QString();
        result.append(qMakePair(id, labelText));
    }
    return result;
}

void ResponseSettingFoldablePanel::collapsedOnInitialization()
{
    m_toggleButton->click();
}

void ResponseSettingFoldablePanel::SetContentEnable(bool ok)
{
    m_contentWidget->setEnabled(ok);
}

QIcon ResponseSettingFoldablePanel::getIcon(bool open)
{
    qreal dpr = devicePixelRatioF();
    QIcon sys = style()->standardIcon(open ? QStyle::SP_ArrowDown : QStyle::SP_ArrowRight);
    return drawArrowIcon(open, QColor("#909399"), QColor(0, 0, 0, 70), 64, dpr);
}

QIcon ResponseSettingFoldablePanel::drawArrowIcon(bool open, const QColor& fill, const QColor& outline, int px, qreal dpr)
{
    QPixmap pm(px * dpr, px * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal w = px, h = px;

    QPainterPath path;
    if (open) {
        // ↓ 三角
        path.moveTo(0.20 * w, 0.35 * h);
        path.lineTo(0.80 * w, 0.35 * h);
        path.lineTo(0.50 * w, 0.75 * h);
    }
    else {
        // → 三角
        path.moveTo(0.35 * w, 0.20 * h);
        path.lineTo(0.75 * w, 0.50 * h);
        path.lineTo(0.35 * w, 0.80 * h);
    }
    path.closeSubpath();

    // 填充
    p.setPen(Qt::NoPen);
    p.setBrush(fill);
    p.drawPath(path);

    // 描边（让它不是“纯色块”，边缘更立体）
    if (outline.alpha() > 0) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(outline, 1.2 * dpr));
        p.drawPath(path);
    }

    return QIcon(pm);
}

// ---------- 槽函数 ----------
void ResponseSettingFoldablePanel::resetToDefault()
{
    if (!m_TableWidget)
    {
        return;
    }
    // 清空后按默认行数走添加行逻辑重建，保证与“添加”按钮产生的行一致
    m_TableWidget->clearContents();
    m_TableWidget->setRowCount(0);
    for (int i = 0; i < m_defaultRows; ++i)
    {
        onAddRow();
    }
}

void ResponseSettingFoldablePanel::onAddRow()
{
    if (!m_TableWidget)
    {
        return;
    }

    int row = m_TableWidget->rowCount();
    m_TableWidget->insertRow(row);

    QTableWidgetItem* idItem = new QTableWidgetItem();
    idItem->setFlags(idItem->flags() | Qt::ItemIsEditable);
    idItem->setTextAlignment(Qt::AlignCenter);
    idItem->setText("请输入");
    idItem->setForeground(QBrush(QColor("#909399")));
    m_TableWidget->setItem(row, 0, idItem);

    QTableWidgetItem* labelItem = new QTableWidgetItem();
    labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
    labelItem->setTextAlignment(Qt::AlignCenter);
    labelItem->setText("请输入");
    labelItem->setForeground(QBrush(QColor("#909399")));
    m_TableWidget->setItem(row, 1, labelItem);

    m_PointCount->setText(QString::number(rowCount()));
    emit dataChanged();
}

void ResponseSettingFoldablePanel::onTableItemChanged(QTableWidgetItem* item)
{
    if (!item)
    {
        return;
    }

    int col = item->column();
    if (col != 0 && col != 1)
    {
        return;
    }

    QString text = item->text().trimmed();
    if (text.isEmpty())
    {
        QSignalBlocker blocker(m_TableWidget);
        item->setText("请输入");
        item->setForeground(QBrush(QColor("#909399")));
    }
    else if (text != "请输入")
    {
        // 标签查重：标签列不允许与同表其他行重复，重复则回退占位符并提示
        if (col == 1)
        {
            for (int r = 0; r < m_TableWidget->rowCount(); ++r)
            {
                if (r == item->row())
                    continue;
                QTableWidgetItem* other = m_TableWidget->item(r, 1);
                if (other && other->text().trimmed() == text)
                {
                    QSignalBlocker blocker(m_TableWidget);
                    item->setText("请输入");
                    item->setForeground(QBrush(QColor("#909399")));
                    QMessageBox::warning(this, QString("提示"),
                        QString("标签“%1”已存在。").arg(text));
                    emit dataChanged();
                    return;
                }
            }
        }
        item->setForeground(QBrush(QColor(0, 0, 0)));
    }
    emit dataChanged();
}

void ResponseSettingFoldablePanel::onDeleteRow()
{
    if (!m_TableWidget)
    {
        return;
    }

    QModelIndexList selected = m_TableWidget->selectionModel()->selectedRows();
    if (selected.isEmpty())
    {
        updateDeleteButtonState();
        return;
    }

    QList<int> rows;
    for (const QModelIndex& index : selected) {
        rows.append(index.row());
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());

    for (int row : rows) {
        m_TableWidget->removeRow(row);
    }

    // 更新计数
    m_PointCount->setText(MxFunctions::mNumberToString(rowCount()));
    updateDeleteButtonState();
    emit dataChanged();
}

void ResponseSettingFoldablePanel::onImportFromFile()
{
    if (!m_TableWidget)
    {
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(
        this,
        "导入文件",
        QString(),
        "文本文件 (*.txt *.csv)");
    if (fileName.isEmpty())
    {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "警告", QString("无法打开文件：%1").arg(fileName));
        return;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");

    QString idHeaderText = m_isElementForce ? "elementID" : "nodeID";
    QList<QPair<int, QString>> importedData;
    int lineIndex = 0;

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        ++lineIndex;
        if (line.isEmpty())
        {
            continue;
        }

        QStringList parts = line.split(',');
        if (parts.size() != 2)
        {
            QMessageBox::warning(this, "警告", QString("第 %1 行列数不等于 2。").arg(lineIndex));
            file.close();
            return;
        }

        QString firstCol = parts[0].trimmed();
        if (firstCol.compare(idHeaderText, Qt::CaseInsensitive) == 0 ||
            firstCol.compare("nodeid", Qt::CaseInsensitive) == 0 ||
            firstCol.compare("elementid", Qt::CaseInsensitive) == 0 ||
            firstCol.compare("id", Qt::CaseInsensitive) == 0)
        {
            continue;
        }

        bool idOk = false;
        int id = firstCol.toInt(&idOk);
        if (!idOk || id <= 0)
        {
            if (lineIndex == 1)
            {
                continue;
            }
            QMessageBox::warning(this, "警告", QString("第 %1 行 ID 格式无效。").arg(lineIndex));
            file.close();
            return;
        }

        importedData.append(qMakePair(id, parts[1].trimmed()));
    }

    file.close();

    if (importedData.isEmpty())
    {
        QMessageBox::warning(this, "警告", "文件中没有有效的数据行。");
        return;
    }

    m_TableWidget->clearContents();
    m_TableWidget->setRowCount(0);

    for (const QPair<int, QString>& item : importedData)
    {
        int row = m_TableWidget->rowCount();
        m_TableWidget->insertRow(row);

        QTableWidgetItem* idItem = new QTableWidgetItem(MxFunctions::mNumberToString(item.first));
        idItem->setFlags(idItem->flags() | Qt::ItemIsEditable);
        idItem->setTextAlignment(Qt::AlignCenter);
        m_TableWidget->setItem(row, 0, idItem);

        QTableWidgetItem* labelItem = new QTableWidgetItem(item.second);
        labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
        labelItem->setTextAlignment(Qt::AlignCenter);
        m_TableWidget->setItem(row, 1, labelItem);
    }

    m_PointCount->setText(QString::number(m_TableWidget->rowCount()));
    emit dataChanged();
}

void ResponseSettingFoldablePanel::onExportToFile()
{
    if (!m_TableWidget)
    {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出文件",
        QString(),
        "CSV 文件 (*.csv);;文本文件 (*.txt)");
    if (fileName.isEmpty())
    {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "警告", QString("无法打开文件：%1").arg(fileName));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    QString idHeaderText = m_isElementForce ? "elementID" : "nodeID";
    out << idHeaderText << ", label\n";

    int rowCount = m_TableWidget->rowCount();
    for (int row = 0; row < rowCount; ++row)
    {
        QTableWidgetItem* idItem = m_TableWidget->item(row, 0);
        QTableWidgetItem* labelItem = m_TableWidget->item(row, 1);
        if (!idItem)
        {
            continue;
        }

        QString idText = idItem->text().trimmed();
        if (idText.isEmpty())
        {
            continue;
        }

        bool idOk = false;
        int id = idText.toInt(&idOk);
        if (!idOk || id <= 0)
        {
            continue;
        }

        QString labelText = labelItem ? labelItem->text().trimmed() : QString();
        out << MxFunctions::mNumberToString(id) << ", " << labelText << "\n";
    }

    file.close();
}

void ResponseSettingFoldablePanel::updateDeleteButtonState()
{
    bool hasSelected = m_TableWidget && !m_TableWidget->selectionModel()->selectedRows().isEmpty();
    m_RemoveRow->setEnabled(hasSelected);
}

void ResponseSettingFoldablePanel::slot_onPointCountChanged()
{
    if (!m_PointCount || !m_TableWidget)
    {
        return;
    }

    bool ok = false;
    int targetCount = m_PointCount->text().trimmed().toInt(&ok);
    if (!ok || targetCount < 0)
    {
        m_PointCount->setText(MxFunctions::mNumberToString(rowCount()));
        return;
    }

    int currentCount = rowCount();
    if (targetCount == currentCount)
    {
        return;
    }

    // 批量调整行数时阻塞 lineEdit 信号，防止重入或多算一次。
    QSignalBlocker blocker(m_PointCount);
    if (targetCount > currentCount)
    {
        for (int i = currentCount; i < targetCount; ++i)
        {
            onAddRow();
        }
    }
    else
    {
        for (int i = currentCount - 1; i >= targetCount; --i)
        {
            m_TableWidget->removeRow(i);
        }
    }
    m_PointCount->setText(MxFunctions::mNumberToString(rowCount()));
    updateDeleteButtonState();
    emit dataChanged();
}

void ResponseSettingFoldablePanel::adjustColumnWidths()
{
    if (!m_TableWidget)
    {
        return;
    }
    // 用 panel 自身的宽度来算，避免 table 在布局完成前 viewport 只有默认小尺寸。
    int panelWidth = this->width();
    if (panelWidth <= 0)
    {
        return;
    }
    // 扣除 contentLayout 左右边距 5+5、table 自身 frame/scrollbar 占用，
    // 留出安全余量，保证不会出现横向滚动条。
    int availableWidth = panelWidth - 10;
    if (availableWidth <= 0)
    {
        availableWidth = panelWidth;
    }
    int col0Width = static_cast<int>(availableWidth * 0.3);
    int col1Width = availableWidth - col0Width;
    m_TableWidget->setColumnWidth(0, col0Width);
    m_TableWidget->setColumnWidth(1, col1Width);
}

void ResponseSettingFoldablePanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!m_firstResizeDone)
    {
        // 第一次 resize 时布局可能还没稳定，延迟一帧再算
        QTimer::singleShot(0, this, &ResponseSettingFoldablePanel::adjustColumnWidths);
        m_firstResizeDone = true;
    }
    else
    {
        adjustColumnWidths();
    }
}
/*****************************************mCheckBoxHeader******************************************************/
mCheckBoxHeader::mCheckBoxHeader(Qt::Orientation orientation, QWidget* parent)
    : QHeaderView(orientation, parent)
{
    setSectionsClickable(true);
}

mCheckBoxHeader::~mCheckBoxHeader() {}

void mCheckBoxHeader::setColumnCheckBox(int col, bool show, const QString& text)
{
    if (show)
    {
        if (!columnCheckBoxMap.contains(col))
        {
            QCheckBox* cb = new QCheckBox(this);
            cb->setText(text);
            cb->setChecked(false);
            cb->setTristate(false); // 永久2态，禁用三态
            cb->setStyleSheet(
                "QCheckBox { background-color: transparent; spacing: 0px; padding: 0px; margin: 0px; border: none; }"
                "QCheckBox::indicator { width: 14px; height: 14px; background-color: #FFFFFF;  }"
                "QCheckBox::indicator:checked { background-color: #FFFFFF;  }"
                "QCheckBox::indicator:unchecked { background-color: #FFFFFF; }"
            );
            connect(cb, &QCheckBox::stateChanged, this, [this, col](int state) {
                emit columnCheckStateChanged(col, state);
            });
            columnCheckBoxMap[col] = cb;
            updateCheckBoxPositions();
        }
    }
    else
    {
        if (columnCheckBoxMap.contains(col))
        {
            delete columnCheckBoxMap[col];
            columnCheckBoxMap.remove(col);
        }
    }
}

void mCheckBoxHeader::updateCheckBoxPositions()
{
    for (auto it = columnCheckBoxMap.begin(); it != columnCheckBoxMap.end(); ++it)
    {
        int col = it.key();
        QCheckBox* cb = it.value();
        int cbWidth = cb->sizeHint().width();
        cb->setFixedWidth(cbWidth);
        int x = sectionViewportPosition(col) + 8;
        int y = (height() - cb->height()) / 2;
        cb->move(x, y);
        cb->setVisible(true);
    }
}

void mCheckBoxHeader::setColumnCheckState(int col, int state)
{
    if (columnCheckBoxMap.contains(col))
    {
        columnCheckBoxMap[col]->blockSignals(true);
        columnCheckBoxMap[col]->setChecked(state == Qt::Checked);
        columnCheckBoxMap[col]->blockSignals(false);
    }
}

int mCheckBoxHeader::getColumnCheckState(int col) const
{
    if (columnCheckBoxMap.contains(col))
        return columnCheckBoxMap[col]->isChecked() ? Qt::Checked : Qt::Unchecked;
    return Qt::Unchecked;
}

void mCheckBoxHeader::clearAllCheckBoxes()
{
    for (auto it = columnCheckBoxMap.begin(); it != columnCheckBoxMap.end(); ++it)
    {
        delete it.value();
    }
    columnCheckBoxMap.clear();
}

void mCheckBoxHeader::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
    QHeaderView::paintSection(painter, rect, logicalIndex);
    if (columnCheckBoxMap.contains(logicalIndex))
    {
        // 复选框已经在updateCheckBoxPositions中移动到正确位置
        // 这里不做额外绘制，让复选框浮在header上方
    }
}

void mCheckBoxHeader::resizeEvent(QResizeEvent* event)
{
    QHeaderView::resizeEvent(event);
    updateCheckBoxPositions();
}

void mCheckBoxHeader::sectionResized(int logicalIndex, int oldSize, int newSize)
{
    QHeaderView::sectionResized(logicalIndex, oldSize, newSize);
    updateCheckBoxPositions();
}

/*****************************************IncentiveNodeTableWidget******************************************************/
/*****************************************输入框输入限制******************************************************/

// 数值输入允许的临时形态：符号、小数点、科学计数法输入过程中也合法，如 "1."、"1e-"
static const QRegularExpression g_doubleInputPattern("^[+-]?[0-9]*\\.?[0-9]*([eE][+-]?[0-9]*)?$");
// 整数输入仅允许数字
static const QRegularExpression g_intInputPattern("^[0-9]*$");
// 表格节点ID列仅允许非空数字
static const QRegularExpression g_tableIdPattern("^[0-9]+$");

/*
* 数值输入框限制：输入不合法时直接置空（重复安装自动忽略）
*/
void installDoubleInputFilter(QLineEdit* edit)
{
    if (!edit || edit->property("doubleInputFilterInstalled").toBool())
        return;
    edit->setProperty("doubleInputFilterInstalled", true);
    QObject::connect(edit, &QLineEdit::textEdited, edit, [edit](const QString& text) {
        if (!g_doubleInputPattern.match(text).hasMatch())
            edit->clear();
    });
}

/*
* 整数输入框限制：输入含非数字字符时直接置空（重复安装自动忽略）
*/
void installIntInputFilter(QLineEdit* edit)
{
    if (!edit || edit->property("intInputFilterInstalled").toBool())
        return;
    edit->setProperty("intInputFilterInstalled", true);
    QObject::connect(edit, &QLineEdit::textEdited, edit, [edit](const QString& text) {
        if (!g_intInputPattern.match(text).hasMatch())
            edit->clear();
    });
}

IncentiveNodeTableWidget::IncentiveNodeTableWidget(QWidget* parent)
    : QTableWidget(parent)
{
    setupAppearance();
    refreshHeader();

    // 使用自定义表头
    mCheckBoxHeader* header = qobject_cast<mCheckBoxHeader*>(horizontalHeader());

    // 连接表头复选框信号：同步到对应列所有单元格复选框
    if (header)
    {
        connect(header, &mCheckBoxHeader::columnCheckStateChanged, this, [this](int column, int state) {
            if (column < 1 || column > 6)
                return;
            bool checked = (state == Qt::Checked);
            for (int row = 0; row < rowCount(); ++row) {
                if (QWidget* w = cellWidget(row, column)) {
                    if (QCheckBox* cb = w->findChild<QCheckBox*>()) {
                        cb->setChecked(checked);
                    }
                }
            }
        });
    }

    // 节点ID与标签列的占位符颜色处理
    connect(this, &IncentiveNodeTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (!item)
            return;
        int col = item->column();
        if (col != 0 && col != 7)
            return;
        QString text = item->text().trimmed();
        // 节点ID列仅允许数字，非法输入直接置回占位符
        if (col == 0 && text != "请输入" && !g_tableIdPattern.match(text).hasMatch())
            text.clear();
        if (text.isEmpty())
        {
            QSignalBlocker blocker(this);
            item->setText("请输入");
            item->setForeground(QBrush(QColor("#909399")));
        }
        else if (text != "请输入")
        {
            // 标签查重：标签列不允许与同表其他行重复，重复则回退占位符并提示
            if (col == 7 && m_showLabelColumn)
            {
                for (int r = 0; r < rowCount(); ++r)
                {
                    if (r == item->row())
                        continue;
                    QTableWidgetItem* other = this->item(r, 7);
                    if (other && other->text().trimmed() == text)
                    {
                        QSignalBlocker blocker(this);
                        item->setText("请输入");
                        item->setForeground(QBrush(QColor("#909399")));
                        QMessageBox::warning(this, QString("提示"),
                            QString("标签“%1”已存在，同一表格中标签不能重复。").arg(text));
                        return;
                    }
                }
            }
            item->setForeground(QBrush(QColor(0, 0, 0)));
        }
    });

    // 默认保留一行
    addRow();
    m_defaultRows = rowCount();
}

IncentiveNodeTableWidget::~IncentiveNodeTableWidget() {}

void IncentiveNodeTableWidget::setShowLabelColumn(bool show)
{
    if (m_showLabelColumn == show)
        return;
    m_showLabelColumn = show;
    refreshHeader();

    // 如果刚显示标签列，为已有行补上空白的标签占位单元格
    if (m_showLabelColumn)
    {
        int rowCount = this->rowCount();
        for (int row = 0; row < rowCount; ++row)
        {
            if (!item(row, 7))
            {
                QTableWidgetItem* labelItem = new QTableWidgetItem();
                labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
                labelItem->setTextAlignment(Qt::AlignCenter);
                labelItem->setText("请输入");
                labelItem->setForeground(QBrush(QColor("#909399")));
                setItem(row, 7, labelItem);
            }
        }
    }
}

void IncentiveNodeTableWidget::refreshHeader()
{
    int colCount = m_showLabelColumn ? 8 : 7;
    setColumnCount(colCount);
    if (m_showLabelColumn)
    {
        setHorizontalHeaderLabels(QStringList()
            << "节点ID" << "X" << "Y" << "Z" << "RX" << "RY" << "RZ" << "标签");
    }
    else
    {
        setHorizontalHeaderLabels(QStringList()
            << "节点ID" << "X" << "Y" << "Z" << "RX" << "RY" << "RZ");
    }

    mCheckBoxHeader* header = qobject_cast<mCheckBoxHeader*>(horizontalHeader());
    if (header)
    {
        header->clearAllCheckBoxes();
        for (int col = 1; col < 7; ++col)
        {
            header->setColumnCheckBox(col, true);
        }
    }

    // 第2~7列（X/Y/Z/RX/RY/RZ）设置左对齐
    for (int col = 1; col < 7; ++col)
    {
        if (horizontalHeaderItem(col))
        {
            horizontalHeaderItem(col)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }
    if (horizontalHeaderItem(0))
    {
        horizontalHeaderItem(0)->setTextAlignment(Qt::AlignCenter);
    }
    if (m_showLabelColumn && horizontalHeaderItem(7))
    {
        horizontalHeaderItem(7)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
}

void IncentiveNodeTableWidget::setupAppearance()
{
    // 使用自定义表头
    mCheckBoxHeader* header = new mCheckBoxHeader(Qt::Horizontal, this);
    setHorizontalHeader(header);

    // 隐藏垂直表头（序号列）
    verticalHeader()->setVisible(false);
    horizontalHeader()->setHighlightSections(false);
    // 水平表头列宽自动拉伸，平均分配
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 滚动条按需显示，像素级平滑滚动
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 整行选择，支持多选
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAlternatingRowColors(true);
    setShowGrid(false);
    // 表头样式：背景 #DCDFE6，深色文字。
    // 给表头设置唯一 objectName，使用 ID 选择器提高优先级，覆盖全局 QSS 中的 QHeaderView::section:horizontal
    ///horizontalHeader()->setObjectName("incentiveNodeHeader");
    horizontalHeader()->setStyleSheet(
        "QHeaderView::section:horizontal {"
        "   background-color: #E8EBF2;"
        "   color: #2C3E50;"
        "   border: 1px solid #DCDFE6;"
        "   padding-left: 28px;"
        "}"
        "QHeaderView::section:horizontal:first {"
        "   padding-left: 0px;"
        "}"
    );

    // 表格样式：选中行背景 #D0E7FD，文字灰色；数据行单元格只保留上下横线，去掉左右竖线
    setStyleSheet(
        "QTableWidget {"
        "   alternate-background-color: #F6F8FA;"   /* 奇数行背景 */
        "   background-color: #FFFFFF;"             /* 偶数行背景 */
        "   selection-background-color: #D0E7FD;"
        "   gridline-color: #DCDFE6;"
        "    border: 1px solid #DCDFE6;"
        "    border-radius: 2px;"          // 圆角半径，数值越大越圆滑
        "}"
        "QTableWidget::item {"
        "  border: none;"
        "  border-bottom: 1px solid #ccc;"
        "}"
    );
}

void IncentiveNodeTableWidget::initRow(int row)
{
    QTableWidgetItem* idItem = new QTableWidgetItem();
    idItem->setFlags(idItem->flags() | Qt::ItemIsEditable);
    idItem->setTextAlignment(Qt::AlignCenter);
    idItem->setText("请输入");
    idItem->setForeground(QBrush(QColor("#909399")));
    setItem(row, 0, idItem);

    // 第2~7列：X/Y/Z/RX/RY/RZ —— 左对齐真实复选框，无文字
    for (int col = 1; col < 7; ++col) {
        QWidget* widget = new QWidget(this);
        widget->setStyleSheet("background-color: transparent;");

        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(8, 0, 0, 0);
        layout->setSpacing(0);

        QCheckBox* checkBox = new QCheckBox(widget);
        checkBox->setText("");
        checkBox->setStyleSheet(
            "QCheckBox { background-color: transparent; spacing: 0px; padding: 0px; margin: 0px; border: none; }"
            "QCheckBox::indicator { width: 14px; height: 14px; background-color: #FFFFFF;  }"
            "QCheckBox::indicator:checked { background-color: #FFFFFF;  }"
            "QCheckBox::indicator:unchecked { background-color: #FFFFFF; }"
        );

        layout->addWidget(checkBox, 0, Qt::AlignLeft | Qt::AlignVCenter);
        layout->addStretch();
        layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        setCellWidget(row, col, widget);
    }

    if (m_showLabelColumn)
    {
        QTableWidgetItem* labelItem = new QTableWidgetItem();
        labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
        labelItem->setTextAlignment(Qt::AlignCenter);
        labelItem->setText("请输入");
        labelItem->setForeground(QBrush(QColor("#909399")));
        setItem(row, 7, labelItem);
    }
}

void IncentiveNodeTableWidget::addRow()
{
    int row = rowCount();
    insertRow(row);
    initRow(row);
}

void IncentiveNodeTableWidget::resetToDefault()
{
    setRowCount(0);
    for (int i = 0; i < m_defaultRows; ++i)
    {
        addRow();
    }
}

void IncentiveNodeTableWidget::setEmptyDefault()
{
    m_defaultRows = 0;
    setRowCount(0);
}

void IncentiveNodeTableWidget::removeSelectedRows()
{
    QModelIndexList selected = selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QList<int> rows;
    for (const QModelIndex& index : selected) {
        rows.append(index.row());
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());

    for (int row : rows) {
        removeRow(row);
    }
}

void IncentiveNodeTableWidget::setRowCount(int count)
{
    if (count < 0) count = 0;

    int current = rowCount();
    if (count == current) return;

    if (count > current) {
        for (int i = current; i < count; ++i) {
            insertRow(i);
            initRow(i);
        }
    }
    else {
        for (int i = current - 1; i >= count; --i) {
            removeRow(i);
        }
    }
}

void IncentiveNodeTableWidget::setTableData(const QList<IncentiveNodeRowData>& data)
{
    blockSignals(true);
    // 清空现有行
    while (rowCount() > 0)
        removeRow(0);

    // 填充数据
    for (int i = 0; i < data.size(); ++i) {
        insertRow(i);
        initRow(i);

        const IncentiveNodeRowData& row = data.at(i);
        // 第1列：节点ID
        item(i, 0)->setText(row.nodeId);
        // 第2~7列：复选框状态
        bool states[6] = { row.x, row.y, row.z, row.rx, row.ry, row.rz };
        for (int col = 1; col < 7; ++col) {
            if (QWidget* w = cellWidget(i, col)) {
                if (QCheckBox* cb = w->findChild<QCheckBox*>()) {
                    cb->setChecked(states[col - 1]);
                }
            }
        }
        // 第8列：标签
        if (m_showLabelColumn && item(i, 7))
        {
            QString labelText = row.label.trimmed();
            if (labelText.isEmpty() || labelText == "请输入")
            {
                item(i, 7)->setText("请输入");
                item(i, 7)->setForeground(QBrush(QColor("#909399")));
            }
            else
            {
                item(i, 7)->setText(labelText);
                item(i, 7)->setForeground(QBrush(QColor(0, 0, 0)));
            }
        }
    }
    // 数据为空时自动保留一行
    if (rowCount() == 0)
        addRow();
    blockSignals(false);
    // 刷新一次背景色（清除旧选中状态残留）
    emit selectionModel()->selectionChanged(QItemSelection(), QItemSelection());
}

QList<IncentiveNodeTableWidget::IncentiveNodeRowData> IncentiveNodeTableWidget::getTableData() const
{
    QList<IncentiveNodeRowData> result;
    for (int i = 0; i < rowCount(); ++i) {
        result.append(getRowData(i));
    }
    return result;
}

IncentiveNodeTableWidget::IncentiveNodeRowData IncentiveNodeTableWidget::getRowData(int row) const
{
    IncentiveNodeRowData data;
    if (row < 0 || row >= rowCount())
        return data;

    // 第1列：节点ID
    if (QTableWidgetItem* idItem = item(row, 0))
        data.nodeId = idItem->text();

    // 第2~7列：复选框选中状态
    bool* states[6] = { &data.x, &data.y, &data.z, &data.rx, &data.ry, &data.rz };
    for (int col = 1; col < 7; ++col) {
        if (QWidget* w = cellWidget(row, col)) {
            if (QCheckBox* cb = w->findChild<QCheckBox*>()) {
                *(states[col - 1]) = cb->isChecked();
            }
        }
    }

    // 第8列：标签
    if (m_showLabelColumn)
    {
        if (QTableWidgetItem* labelItem = item(row, 7))
            data.label = labelItem->text();
    }

    return data;
}

/*
* 从 CSV/txt 文件导入曲线数据：
* - 分隔符自动探测（逗号/制表符/分号/连续空格）
* - 空行、# $ % 注释行、非数字表头行自动跳过
* - 数值列不足时缺省列补 0；数值非法的行跳过并计入 skippedRows
* 返回导入的有效行数，文件打开失败返回 -1
*/
static int importCurveFromCsv(const QString& filePath, int expectCols,
    QVector<double>& outX, QVector<double>& outY, QVector<double>& outZ, int& skippedRows)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;
    skippedRows = 0;
    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith('$') || line.startsWith('%'))
            continue;
        // 分隔符探测：取本行出现次数最多的分隔符
        QChar delim = ' ';
        const int comma = line.count(',');
        const int tab = line.count('	');
        const int semi = line.count(';');
        if (comma > 0 && comma >= tab && comma >= semi)
            delim = ',';
        else if (tab > 0)
            delim = '	';
        else if (semi > 0)
            delim = ';';
        const QStringList tokens = (delim == ' ')
            ? line.split(' ', QString::SkipEmptyParts)
            : line.split(delim, QString::SkipEmptyParts);
        if (tokens.isEmpty())
            continue;
        bool headOk = false;
        const double first = tokens.first().toDouble(&headOk);
        if (!headOk)
            continue;   // 表头行
        double vals[3] = { first, 0.0, 0.0 };
        bool rowOk = true;
        for (int c = 1; c < expectCols && c < tokens.size(); ++c)
        {
            bool ok = false;
            const double v = tokens.at(c).toDouble(&ok);
            if (!ok)
            {
                rowOk = false;
                break;
            }
            vals[c] = v;
        }
        if (!rowOk)
        {
            ++skippedRows;
            continue;
        }
        outX.push_back(vals[0]);
        outY.push_back(vals[1]);
        outZ.push_back(vals[2]);
    }
    return outX.size();
}

/*****************************************CreateCurveDialog******************************************************/
CreateCurveDialog::CreateCurveDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUI();
}

CreateCurveDialog::~CreateCurveDialog() { }

CreateCurveDialog* CreateCurveDialog::_instance = nullptr;

CreateCurveDialog* CreateCurveDialog::getInstance()
{
    if (_instance == nullptr)
    {
        _instance = new CreateCurveDialog;
    }
    return _instance;
}

void CreateCurveDialog::initDialog()
{
    m_CurveNameEdit->clear();
    m_LineEdit_DELAY->setText("0");
    m_LineEdit_DPHASE->setText("0");
    m_ComboBox_Freq->setCurrentIndex(0);
    m_ComboBox_Load->setCurrentIndex(0);
    m_tabWidget->setCurrentIndex(0);
    m_TableBtn->setChecked(true);
    m_TableWidget->clear();
    m_TableWidget->setHorizontalHeaderLabels({ "频率", "实部/幅值", "虚部/相位" });
    m_TableWidget->setRowCount(72);
    m_Plot1->removeCurve("curveRealPart");
    m_Plot2->removeCurve("curveImaginaryPart");
}

void CreateCurveDialog::setDialogData(IncentiveCurve data)
{
    m_TableWidget->clear();
    m_TableWidget->setHorizontalHeaderLabels({ "频率", "实部/幅值", "虚部/相位" });
    m_TableWidget->setRowCount(72);
    m_CurveNameEdit->setText(data.name);
    m_LineEdit_DELAY->setText(data.delay);
    m_LineEdit_DPHASE->setText(data.dphase);
    m_ComboBox_Freq->setCurrentIndex(data.freqCombobox);
    m_ComboBox_Load->setCurrentIndex(data.loadCombobox);
    for (int i = 0; i < data.colX.size(); ++i)
    {
        m_TableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(data.colX[i])));
        m_TableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(data.colY[i])));
        m_TableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(data.colZ[i])));
    }
}

void CreateCurveDialog::setupUI()
{
    setWindowTitle("创建曲线");
    resize(480, 530);
    this->setStyleSheet(
        "QDialog { background-color: #ffffff; }"
        "QWidget { background-color: #ffffff; }"
        "QComboBox:enabled { background-color: white; }"
        "QLineEdit { background-color: white; }"
        "QLineEdit:enabled { background-color: white; }"
        "QCheckBox { background-color: transparent; }"
        "QLabel { background-color: transparent; }"
        "QPushButton { background-color: white; }"
    );
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // ---- 曲线名称行 ----
    QHBoxLayout* nameLayout = new QHBoxLayout(this);
    nameLayout->addWidget(new QLabel("曲线名称"));
    m_CurveNameEdit = new QLineEdit(this);
    nameLayout->addWidget(m_CurveNameEdit);
    m_ImportCurve = new QPushButton("导入曲线",this);
    nameLayout->addWidget(m_ImportCurve);
    mainLayout->addLayout(nameLayout);

    QGridLayout* gridLayout = new QGridLayout(this);
    m_LineEdit_DELAY = new QLineEdit(this);
    m_LineEdit_DELAY->setFixedWidth(150);
    m_LineEdit_DELAY->setText("0");
    m_LineEdit_DPHASE = new QLineEdit(this);
    m_LineEdit_DPHASE->setFixedWidth(150);
    m_LineEdit_DPHASE->setText("0");
    m_ComboBox_Freq = new QComboBox(this);
    m_ComboBox_Freq->setFixedWidth(150);
    m_ComboBox_Freq->addItems({ "线性分布","对数分布" });
    m_ComboBox_Freq->setStyleSheet(
        "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
        "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
    );
    m_ComboBox_Load = new QComboBox(this);
    m_ComboBox_Load->setFixedWidth(150);
    m_ComboBox_Load->addItems({ "线性分布","对数分布" });
    m_ComboBox_Load->setStyleSheet(
        "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
        "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
    );
    QLabel* label1 = new QLabel("DELAY");
    label1->setFixedWidth(48);
    QLabel* label2 = new QLabel("DPHASE");
    label2->setFixedWidth(48);
    QLabel* label3 = new QLabel("频率");
    label3->setFixedWidth(48);
    QLabel* label4 = new QLabel("载荷");
    label4->setFixedWidth(48);
    gridLayout->addWidget(label1, 0, 0);
    gridLayout->addWidget(m_LineEdit_DELAY, 0, 1);
    gridLayout->addWidget(label2, 0, 2);
    gridLayout->addWidget(m_LineEdit_DPHASE, 0, 3);
    gridLayout->addWidget(label3, 1, 0);
    gridLayout->addWidget(m_ComboBox_Freq, 1, 1);
    gridLayout->addWidget(label4, 1, 2);
    gridLayout->addWidget(m_ComboBox_Load, 1, 3);
    mainLayout->addLayout(gridLayout);

    // ---- 表格 + 图表（水平并排） ----
    QHBoxLayout* pushButtonLayout = new QHBoxLayout;
    pushButtonLayout->setSpacing(0);
    m_TableBtn = new QPushButton("表格", this);
    m_TableBtn->setFixedSize(48, 24);
    m_TableBtn->setCheckable(true);
    m_ChartBtn = new QPushButton("绘图", this);
    m_ChartBtn->setFixedSize(48,24);
    m_ChartBtn->setCheckable(true);
    QButtonGroup* group = new QButtonGroup(this);
    group->addButton(m_TableBtn);
    group->addButton(m_ChartBtn);
    m_TableBtn->setChecked(true);

    QString buttonStyle =
        "QPushButton {"
        "    background-color: #FFFFFF;"   // 未选中时白色背景
        "    color: #333333;"              // 未选中时深灰文字
        "    border: 1px solid #D0D0D0;"   // 未选中时浅灰边框
        "    padding: 5px 10px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:checked {"
        "    background-color: #E7F3FE;"   // 选中时你的背景色
        "    color: #1385F6;"              // 选中时你的文字色
        "    border: 1px solid #1385F6;"   // 选中时你的边缘色
        "}";
    m_TableBtn->setStyleSheet(buttonStyle);
    m_ChartBtn->setStyleSheet(buttonStyle);
    pushButtonLayout->addWidget(m_TableBtn);
    pushButtonLayout->addWidget(m_ChartBtn);
    pushButtonLayout->addStretch(1);
    mainLayout->addLayout(pushButtonLayout);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setMinimumHeight(220);
    m_tabWidget->tabBar()->setVisible(false);
    m_tabWidget->setStyleSheet(
        "QTabWidget {"
        "    border: 1px solid #E8EBF2;"
        "}"
        "QTabWidget::pane {"
        "    border: none;"               
        "}"
    );

    // 表格
    m_TableWidget = new QTableWidget(72, 3);
    m_TableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section::horizontal{"
        "   background-color: #E8EBF2;"
        "   color: #2C3E50;"
        "}"
    );
    m_TableWidget->verticalHeader()->setStyleSheet(
        "QHeaderView::section::vertical{"
        "   background-color: #E8EBF2;"
        "   color: #2C3E50;"
        "}"
    );
    m_TableWidget->setStyleSheet(
        "QTableWidget {"
        "    border: 1px solid #DCDFE6;"
        "    border-radius: 2px;"          // 圆角半径，数值越大越圆滑
        "    gridline-color: #DCDFE6;"
        "}"
    );
    QHeaderView* verticalHeader = m_TableWidget->verticalHeader();
    verticalHeader->setDefaultAlignment(Qt::AlignCenter);
    m_TableWidget->setHorizontalHeaderLabels({ "频率", "实部/幅值", "虚部/相位" });
    m_TableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 表格数据变化时更新图表
    m_tabWidget->addTab(m_TableWidget, "Page 1");

    m_PlotView = new QWidget(this);
    m_PlotView->setObjectName("parent");
    m_PlotView->setStyleSheet(
        "    #parent {"
        "    border: 1px solid #DCDFE6;"
        "    border-radius: 2px;"          // 圆角半径，数值越大越圆滑
        "}"
    );
    QVBoxLayout* plotLayout = new QVBoxLayout(m_PlotView);
    m_Plot1 = new mPlotView(m_PlotView);
    m_bottomAxis.title = "频率";
    AxisSettings topAxis;
    AxisSettings rightAxis;
    topAxis.isVisible = false;
    rightAxis.isVisible = false;
    m_leftAxis1.title = "实部/幅值";
    m_Plot1->updateAxisAppearance(X_Bottom, m_bottomAxis);
    m_Plot1->updateAxisAppearance(Y_Left, m_leftAxis1);
    m_Plot1->updateAxisScale(X_Top, topAxis);
    m_Plot1->updateAxisScale(Y_Right, rightAxis);
    m_Plot1->setPlotLegendItemVisible(false);

    m_Plot2 = new mPlotView(m_PlotView);
    m_leftAxis2.title = "虚部/相位";
    m_Plot2->updateAxisAppearance(X_Bottom, m_bottomAxis);
    m_Plot2->updateAxisAppearance(Y_Left, m_leftAxis2);
    m_Plot2->updateAxisScale(X_Top, topAxis);
    m_Plot2->updateAxisScale(Y_Right, rightAxis);
    m_Plot2->setPlotLegendItemVisible(false);

    plotLayout->addWidget(m_Plot1);
    plotLayout->addWidget(m_Plot2);
    m_tabWidget->addTab(m_PlotView, "Page 2");

    mainLayout->addWidget(m_tabWidget, 1);
    // ---- 按钮行 ----
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    m_SaveBtn = new QPushButton("保存");
    m_SaveBtn->setStyleSheet(
        "QPushButton { background-color: #1385F6; color: white;}");
    m_SaveBtn->setFixedSize(44, 26);
    m_CloseBtn = new QPushButton("关闭");
    m_CloseBtn->setFixedSize(44, 26);
    buttonLayout->addWidget(m_CloseBtn);
    buttonLayout->addWidget(m_SaveBtn);
    mainLayout->addLayout(buttonLayout);

    connect(m_TableBtn, &QPushButton::clicked, this, [this]()
        {
            m_tabWidget->setCurrentIndex(0);
        });
    connect(m_ChartBtn, &QPushButton::clicked, this, &CreateCurveDialog::onPlotCurve);
    connect(m_ImportCurve, &QPushButton::clicked, this, &CreateCurveDialog::onImportCurve);
    connect(m_SaveBtn, &QPushButton::clicked, this, &CreateCurveDialog::onSave);
    connect(m_CloseBtn, &QPushButton::clicked, this, &CreateCurveDialog::onClose);
    connect(m_ComboBox_Freq, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int index) {
            m_bottomAxis.isLog = (index == 1);
            m_Plot1->updateAxisLog(X_Bottom, m_bottomAxis);
            m_Plot2->updateAxisLog(X_Bottom, m_bottomAxis);
        });
    connect(m_ComboBox_Load, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int index) {
            m_leftAxis1.isLog = (index == 1);
            m_leftAxis2.isLog = (index == 1);
            m_Plot1->updateAxisLog(Y_Left, m_leftAxis1);
            m_Plot2->updateAxisLog(Y_Left, m_leftAxis2);
        });

}

void CreateCurveDialog::onImportCurve()
{
    const QString file = QFileDialog::getOpenFileName(this, QString("导入曲线"), QString(), "曲线文件 (*.csv *.txt)");
    if (file.isEmpty())
        return;
    QVector<double> xs, ys, zs;
    int skipped = 0;
    const int n = importCurveFromCsv(file, 3, xs, ys, zs, skipped);
    if (n < 0)
    {
        QMessageBox::warning(this, QString("提示"), QString("无法打开文件：%1").arg(file));
        return;
    }
    if (n == 0)
    {
        QMessageBox::warning(this, QString("提示"), QString("文件中未找到有效数据行。"));
        return;
    }
    m_TableWidget->clear();
    m_TableWidget->setHorizontalHeaderLabels({ "频率", "实部/幅值", "虚部/相位" });
    m_TableWidget->setRowCount(qMax(72, n));
    for (int i = 0; i < n; ++i)
    {
        m_TableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(xs[i], 'g', 15)));
        m_TableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(ys[i], 'g', 15)));
        m_TableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(zs[i], 'g', 15)));
    }
    // 名称未填时用文件名，顺带满足名称必填校验
    if (m_CurveNameEdit->text().trimmed().isEmpty())
        m_CurveNameEdit->setText(QFileInfo(file).baseName());
    m_tabWidget->setCurrentIndex(0);
    if (skipped > 0)
        QMessageBox::information(this, QString("提示"), QString("已导入 %1 行数据，跳过 %2 行无法解析的数据。").arg(n).arg(skipped));
}

void CreateCurveDialog::onSave()
{
    // 先校验再写入：数据无效时不改动按钮曲线，也不关闭对话框
    if (!isDataValid())
    {
        QMessageBox::warning(this, QString("提示"), QString("曲线名不能为空，且表格至少需要两行完整数据。"));
        return;
    }
    IncentiveCurve data;
    data.name = m_CurveNameEdit->text();
    data.delay = m_LineEdit_DELAY->text();
    data.dphase = m_LineEdit_DPHASE->text();
    data.freqCombobox = m_ComboBox_Freq->currentIndex();
    data.loadCombobox = m_ComboBox_Load->currentIndex();
    for (int i = 0; i < m_TableWidget->rowCount(); ++i)
    {
        QTableWidgetItem* item1 = m_TableWidget->item(i, 0);
        QTableWidgetItem* item2 = m_TableWidget->item(i, 1);
        QTableWidgetItem* item3 = m_TableWidget->item(i, 2);
        // 频率与实部必填，虚部/相位为空时按0保存
        if (item1 && !item1->text().isEmpty() && item2 && !item2->text().isEmpty())
        {
            data.colX.push_back(item1->text().toDouble());
            data.colY.push_back(item2->text().toDouble());
            data.colZ.push_back(item3 && !item3->text().isEmpty() ? item3->text().toDouble() : 0.0);
        }
        else
            break;
    }
    m_currentButton->setCurveData(data);
    m_currentButton->setButtonIcon(":/mMainWidgets/Icon/NVH/quxian_wancheng.png");
    m_currentButton->m_isCompleted = true;
    accept();
}

void CreateCurveDialog::onClose()
{
    reject();
}

void CreateCurveDialog::onPlotCurve()
{
    QVector<double> columnX;
    QVector<double> columnY;
    QVector<double> columnZ;
        for (int i = 0; i < m_TableWidget->rowCount(); ++i)
        {
            QTableWidgetItem* item1 = m_TableWidget->item(i, 0);
            QTableWidgetItem* item2 = m_TableWidget->item(i, 1);
            QTableWidgetItem* item3 = m_TableWidget->item(i, 2);
            // 频率与实部必填，虚部/相位为空时按0绘图
            if (item1 && !item1->text().isEmpty() && item2 && !item2->text().isEmpty())
            {
                columnX.push_back(item1->text().toDouble());
                columnY.push_back(item2->text().toDouble());
                columnZ.push_back(item3 && !item3->text().isEmpty() ? item3->text().toDouble() : 0.0);
            }
            else
                break;
    }
            m_Plot1->updateCurveData("curveRealPart", columnX, columnY);
            m_Plot2->updateCurveData("curveImaginaryPart", columnX, columnZ);
    m_tabWidget->setCurrentIndex(1);
}

bool CreateCurveDialog::isDataValid()
{
        if (m_CurveNameEdit->text().isEmpty() || m_LineEdit_DELAY->text().isEmpty() || m_LineEdit_DPHASE->text().isEmpty())
            return false;
    int count = 0;
        for (int i = 0; i < m_TableWidget->rowCount(); ++i)
        {
            QTableWidgetItem* item1 = m_TableWidget->item(i, 0);
            QTableWidgetItem* item2 = m_TableWidget->item(i, 1);
            QTableWidgetItem* item3 = m_TableWidget->item(i, 2);
            // 频率与实部必填，虚部/相位允许为空（按0处理）
            if (item1 && !item1->text().isEmpty() && item2 && !item2->text().isEmpty())
            {
                ++count;
            }
            else
                break;
        }
    return count >= 2;
}

/*****************************************CreateSingleCurveDialog******************************************************/
CreateSingleCurveDialog::CreateSingleCurveDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUI();
}

CreateSingleCurveDialog::~CreateSingleCurveDialog() { }

CreateSingleCurveDialog* CreateSingleCurveDialog::_instance = nullptr;

CreateSingleCurveDialog* CreateSingleCurveDialog::getInstance()
{
    if (_instance == nullptr)
    {
        _instance = new CreateSingleCurveDialog;
    }
    return _instance;
}

void CreateSingleCurveDialog::initDialog()
{
    m_CurveNameEdit->clear();
    m_LineEdit_DELAY->setText("0");
    m_tabWidget->setCurrentIndex(0);
    m_TableBtn->setChecked(true);
    m_TableWidget->clear();
    m_TableWidget->setHorizontalHeaderLabels({ "X", "Y" });
    m_TableWidget->setRowCount(72);
    m_Plot1->removeCurve("curveRealPart");
}

void CreateSingleCurveDialog::setDialogData(IncentiveCurve data)
{
    m_TableWidget->clear();
    m_TableWidget->setHorizontalHeaderLabels({ "X", "Y" });
    m_TableWidget->setRowCount(72);
    m_CurveNameEdit->setText(data.name);
    m_LineEdit_DELAY->setText(data.delay);
    for (int i = 0; i < data.colX.size(); ++i)
    {
        m_TableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(data.colX[i])));
        m_TableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(data.colY[i])));
    }
}

void CreateSingleCurveDialog::setupUI()
{
    setWindowTitle("创建曲线");
    resize(480, 480);
    this->setStyleSheet(
        "QDialog { background-color: #ffffff; }"
        "QWidget { background-color: #ffffff; }"
        "QComboBox:enabled { background-color: white; }"
        "QLineEdit { background-color: white; }"
        "QLineEdit:enabled { background-color: white; }"
        "QCheckBox { background-color: transparent; }"
        "QLabel { background-color: transparent; }"
        "QPushButton { background-color: white; }"
    );
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // ---- 曲线名称行 ----
    QHBoxLayout* nameLayout = new QHBoxLayout(this);
    nameLayout->addWidget(new QLabel("曲线名称"));
    m_CurveNameEdit = new QLineEdit(this);
    nameLayout->addWidget(m_CurveNameEdit);
    m_ImportCurve = new QPushButton("导入曲线", this);
    nameLayout->addWidget(m_ImportCurve);
    mainLayout->addLayout(nameLayout);

    QHBoxLayout* HLayout = new QHBoxLayout(this);
    QLabel* label1 = new QLabel("DELAY");
    label1->setFixedWidth(48);
    m_LineEdit_DELAY = new QLineEdit(this);
    m_LineEdit_DELAY->setFixedWidth(150);
    m_LineEdit_DELAY->setText("0");
    HLayout->addWidget(label1);
    HLayout->addWidget(m_LineEdit_DELAY);
    HLayout->addStretch(1);

    mainLayout->addLayout(HLayout);

    // ---- 表格 + 图表（水平并排） ----
    QHBoxLayout* pushButtonLayout = new QHBoxLayout;
    pushButtonLayout->setSpacing(0);
    m_TableBtn = new QPushButton("表格", this);
    m_TableBtn->setFixedSize(48, 24);
    m_TableBtn->setCheckable(true);
    m_ChartBtn = new QPushButton("绘图", this);
    m_ChartBtn->setFixedSize(48, 24);
    m_ChartBtn->setCheckable(true);
    QButtonGroup* group = new QButtonGroup(this);
    group->addButton(m_TableBtn);
    group->addButton(m_ChartBtn);
    m_TableBtn->setChecked(true);

    QString buttonStyle =
        "QPushButton {"
        "    background-color: #FFFFFF;"   // 未选中时白色背景
        "    color: #333333;"              // 未选中时深灰文字
        "    border: 1px solid #D0D0D0;"   // 未选中时浅灰边框
        "    padding: 5px 10px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:checked {"
        "    background-color: #E7F3FE;"   // 选中时你的背景色
        "    color: #1385F6;"              // 选中时你的文字色
        "    border: 1px solid #1385F6;"   // 选中时你的边缘色
        "}";
    m_TableBtn->setStyleSheet(buttonStyle);
    m_ChartBtn->setStyleSheet(buttonStyle);
    pushButtonLayout->addWidget(m_TableBtn);
    pushButtonLayout->addWidget(m_ChartBtn);
    pushButtonLayout->addStretch(1);
    mainLayout->addLayout(pushButtonLayout);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setMinimumHeight(220);
    m_tabWidget->tabBar()->setVisible(false);
    m_tabWidget->setStyleSheet(
        "QTabWidget {"
        "    border: 1px solid #E8EBF2;"
        "}"
        "QTabWidget::pane {"
        "    border: none;"
        "}"
    );

    // 表格
    m_TableWidget = new QTableWidget(72, 2);
    m_TableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section::horizontal{"
        "   background-color: #E8EBF2;"
        "   color: #2C3E50;"
        "}"
    );
    m_TableWidget->verticalHeader()->setStyleSheet(
        "QHeaderView::section::vertical{"
        "   background-color: #E8EBF2;"
        "   color: #2C3E50;"
        "}"
    );
    m_TableWidget->setStyleSheet(
        "QTableWidget {"
        "    border: 1px solid #DCDFE6;"
        "    border-radius: 2px;"          // 圆角半径，数值越大越圆滑
        "    gridline-color: #DCDFE6;"
        "}"
    );
    QHeaderView* verticalHeader = m_TableWidget->verticalHeader();
    verticalHeader->setDefaultAlignment(Qt::AlignCenter);
    m_TableWidget->setHorizontalHeaderLabels({ "X", "Y"});
    m_TableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 表格数据变化时更新图表
    m_tabWidget->addTab(m_TableWidget, "Page 1");

    m_PlotView = new QWidget(this);
    m_PlotView->setObjectName("parent");
    m_PlotView->setStyleSheet(
        "    #parent {"
        "    border: 1px solid #DCDFE6;"
        "    border-radius: 2px;"          // 圆角半径，数值越大越圆滑
        "}"
    );
    QVBoxLayout* plotLayout = new QVBoxLayout(m_PlotView);
    m_Plot1 = new mPlotView(this);
    AxisSettings bottomAxis;
    bottomAxis.title = "X";
    AxisSettings leftAxis;
    leftAxis.title = "Y";
    AxisSettings topAxis;
    AxisSettings rightAxis;
    topAxis.isVisible = false;
    rightAxis.isVisible = false;
    m_Plot1->updateAxisAppearance(X_Bottom, bottomAxis);
    m_Plot1->updateAxisAppearance(Y_Left, leftAxis);
    m_Plot1->updateAxisScale(X_Top, topAxis);
    m_Plot1->updateAxisScale(Y_Right, rightAxis);
    m_Plot1->setPlotLegendItemVisible(false);
    plotLayout->addWidget(m_Plot1);
    m_tabWidget->addTab(m_PlotView, "Page 2");

    mainLayout->addWidget(m_tabWidget, 1);
    // ---- 按钮行 ----
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    m_SaveBtn = new QPushButton("保存");
    m_SaveBtn->setStyleSheet(
        "QPushButton { background-color: #1385F6; color: white;}");
    m_SaveBtn->setFixedSize(44, 26);
    m_CloseBtn = new QPushButton("关闭");
    m_CloseBtn->setFixedSize(44, 26);
    buttonLayout->addWidget(m_CloseBtn);
    buttonLayout->addWidget(m_SaveBtn);
    mainLayout->addLayout(buttonLayout);

    connect(m_TableBtn, &QPushButton::clicked, this, [this]()
        {
            m_tabWidget->setCurrentIndex(0);
        });
    connect(m_ChartBtn, &QPushButton::clicked, this, &CreateSingleCurveDialog::onPlotCurve);
    connect(m_ImportCurve, &QPushButton::clicked, this, &CreateSingleCurveDialog::onImportCurve);
    connect(m_SaveBtn, &QPushButton::clicked, this, &CreateSingleCurveDialog::onSave);
    connect(m_CloseBtn, &QPushButton::clicked, this, &CreateSingleCurveDialog::onClose);
}

void CreateSingleCurveDialog::onImportCurve()
{
    const QString file = QFileDialog::getOpenFileName(this, QString("导入曲线"), QString(), "曲线文件 (*.csv *.txt)");
    if (file.isEmpty())
        return;
    QVector<double> xs, ys, zs;
    int skipped = 0;
    const int n = importCurveFromCsv(file, 2, xs, ys, zs, skipped);
    if (n < 0)
    {
        QMessageBox::warning(this, QString("提示"), QString("无法打开文件：%1").arg(file));
        return;
    }
    if (n == 0)
    {
        QMessageBox::warning(this, QString("提示"), QString("文件中未找到有效数据行。"));
        return;
    }
    m_TableWidget->clear();
    m_TableWidget->setHorizontalHeaderLabels({ "X", "Y" });
    m_TableWidget->setRowCount(qMax(72, n));
    for (int i = 0; i < n; ++i)
    {
        m_TableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(xs[i], 'g', 15)));
        m_TableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(ys[i], 'g', 15)));
    }
    // 名称未填时用文件名，顺带满足名称必填校验
    if (m_CurveNameEdit->text().trimmed().isEmpty())
        m_CurveNameEdit->setText(QFileInfo(file).baseName());
    m_tabWidget->setCurrentIndex(0);
    if (skipped > 0)
        QMessageBox::information(this, QString("提示"), QString("已导入 %1 行数据，跳过 %2 行无法解析的数据。").arg(n).arg(skipped));
}

void CreateSingleCurveDialog::onSave()
{
    // 先校验再写入：数据无效时不改动按钮曲线，也不关闭对话框
    if (!isDataValid())
    {
        QMessageBox::warning(this, QString("提示"), QString("曲线名不能为空，且表格至少需要两行完整数据。"));
        return;
    }
    IncentiveCurve data;
    data.name = m_CurveNameEdit->text();
    data.delay = m_LineEdit_DELAY->text();
    for (int i = 0; i < m_TableWidget->rowCount(); ++i)
    {
        QTableWidgetItem* item1 = m_TableWidget->item(i, 0);
        QTableWidgetItem* item2 = m_TableWidget->item(i, 1);
        if (item1 && !item1->text().isEmpty() && item2 && !item2->text().isEmpty())
        {
            data.colX.push_back(item1->text().toDouble());
            data.colY.push_back(item2->text().toDouble());
        }
        else
            break;
    }
    m_currentButton->setCurveData(data);
    m_currentButton->setButtonIcon(":/mMainWidgets/Icon/NVH/quxian_wancheng.png");
    m_currentButton->m_isCompleted = true;
    accept();
}

void CreateSingleCurveDialog::onClose()
{
    reject();
}

void CreateSingleCurveDialog::onPlotCurve()
{
    QVector<double> columnX;
    QVector<double> columnY;
    for (int i = 0; i < m_TableWidget->rowCount(); ++i)
    {
        QTableWidgetItem* item1 = m_TableWidget->item(i, 0);
        QTableWidgetItem* item2 = m_TableWidget->item(i, 1);
        if (item1 && !item1->text().isEmpty() && item2 && !item2->text().isEmpty())
        {
            columnX.push_back(item1->text().toDouble());
            columnY.push_back(item2->text().toDouble());
        }
        else
            break;
    }
    m_Plot1->updateCurveData("curveRealPart", columnX, columnY);
    m_tabWidget->setCurrentIndex(1);
}

bool CreateSingleCurveDialog::isDataValid()
{
    if (m_CurveNameEdit->text().isEmpty())
        return false;
    int count = 0;
    for (int i = 0; i < m_TableWidget->rowCount(); ++i)
    {
        QTableWidgetItem* item1 = m_TableWidget->item(i, 0);
        QTableWidgetItem* item2 = m_TableWidget->item(i, 1);
        if (item1 && !item1->text().isEmpty() && item2 && !item2->text().isEmpty())
        {
            ++count;
        }
        else
            break;
    }
    return count >= 2;
}

/*****************************************CreatePSDCurveDialog******************************************************/
CreatePSDCurveDialog::CreatePSDCurveDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUI();
}

CreatePSDCurveDialog::~CreatePSDCurveDialog() { }

CreatePSDCurveDialog* CreatePSDCurveDialog::_instance = nullptr;

CreatePSDCurveDialog* CreatePSDCurveDialog::getInstance()
{
    if (_instance == nullptr)
    {
        _instance = new CreatePSDCurveDialog;
    }
    return _instance;
}

void CreatePSDCurveDialog::initDialog()
{
    m_CurveNameEdit->clear();
    m_ComboBox_Freq->setCurrentIndex(0);
    m_ComboBox_Load->setCurrentIndex(0);
    m_tabWidget->setCurrentIndex(0);
    m_TableBtn->setChecked(true);
    m_TableWidget->clear();
    m_TableWidget->setHorizontalHeaderLabels({ "X", "Y" });
    m_TableWidget->setRowCount(72);
    m_Plot1->removeCurve("curveRealPart");
}

void CreatePSDCurveDialog::setDialogData(IncentiveCurve data)
{
    m_TableWidget->clear();
    m_TableWidget->setHorizontalHeaderLabels({ "X", "Y" });
    m_TableWidget->setRowCount(72);
    m_CurveNameEdit->setText(data.name);
    m_ComboBox_Freq->setCurrentIndex(data.freqCombobox);
    m_ComboBox_Load->setCurrentIndex(data.loadCombobox);
    for (int i = 0; i < data.colX.size(); ++i)
    {
        m_TableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(data.colX[i])));
        m_TableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(data.colY[i])));
    }
}

void CreatePSDCurveDialog::setupUI()
{
    setWindowTitle("创建曲线");
    resize(480, 450);
    this->setStyleSheet(
        "QDialog { background-color: #ffffff; }"
        "QWidget { background-color: #ffffff; }"
        "QComboBox:enabled { background-color: white; }"
        "QLineEdit { background-color: white; }"
        "QLineEdit:enabled { background-color: white; }"
        "QCheckBox { background-color: transparent; }"
        "QLabel { background-color: transparent; }"
        "QPushButton { background-color: white; }"
    );
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // ---- 曲线名称行 ----
    QHBoxLayout* nameLayout = new QHBoxLayout(this);
    nameLayout->addWidget(new QLabel("曲线名称"));
    m_CurveNameEdit = new QLineEdit(this);
    nameLayout->addWidget(m_CurveNameEdit);
    m_ImportCurve = new QPushButton("导入曲线", this);
    nameLayout->addWidget(m_ImportCurve);
    mainLayout->addLayout(nameLayout);

    QGridLayout* gridLayout = new QGridLayout(this);
    m_ComboBox_Freq = new QComboBox(this);
    m_ComboBox_Freq->setFixedWidth(150);
    m_ComboBox_Freq->addItems({ "线性分布","对数分布" });
    m_ComboBox_Freq->setStyleSheet(
        "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
        "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
    );
    m_ComboBox_Load = new QComboBox(this);
    m_ComboBox_Load->setFixedWidth(150);
    m_ComboBox_Load->addItems({ "线性分布","对数分布" });
    m_ComboBox_Load->setStyleSheet(
        "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
        "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
    );
    QLabel* label3 = new QLabel("频率");
    label3->setFixedWidth(48);
    QLabel* label4 = new QLabel("载荷");
    label4->setFixedWidth(48);
    gridLayout->addWidget(label3, 1, 0);
    gridLayout->addWidget(m_ComboBox_Freq, 1, 1);
    gridLayout->addWidget(label4, 1, 2);
    gridLayout->addWidget(m_ComboBox_Load, 1, 3);
    mainLayout->addLayout(gridLayout);

    // ---- 表格 + 图表（水平并排） ----
    QHBoxLayout* pushButtonLayout = new QHBoxLayout;
    pushButtonLayout->setSpacing(0);
    m_TableBtn = new QPushButton("表格", this);
    m_TableBtn->setFixedSize(48, 24);
    m_TableBtn->setCheckable(true);
    m_ChartBtn = new QPushButton("绘图", this);
    m_ChartBtn->setFixedSize(48, 24);
    m_ChartBtn->setCheckable(true);
    QButtonGroup* group = new QButtonGroup(this);
    group->addButton(m_TableBtn);
    group->addButton(m_ChartBtn);
    m_TableBtn->setChecked(true);

    QString buttonStyle =
        "QPushButton {"
        "    background-color: #FFFFFF;"   // 未选中时白色背景
        "    color: #333333;"              // 未选中时深灰文字
        "    border: 1px solid #D0D0D0;"   // 未选中时浅灰边框
        "    padding: 5px 10px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:checked {"
        "    background-color: #E7F3FE;"   // 选中时你的背景色
        "    color: #1385F6;"              // 选中时你的文字色
        "    border: 1px solid #1385F6;"   // 选中时你的边缘色
        "}";
    m_TableBtn->setStyleSheet(buttonStyle);
    m_ChartBtn->setStyleSheet(buttonStyle);
    pushButtonLayout->addWidget(m_TableBtn);
    pushButtonLayout->addWidget(m_ChartBtn);
    pushButtonLayout->addStretch(1);
    mainLayout->addLayout(pushButtonLayout);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setMinimumHeight(220);
    m_tabWidget->tabBar()->setVisible(false);
    m_tabWidget->setStyleSheet(
        "QTabWidget {"
        "    border: 1px solid #E8EBF2;"
        "}"
        "QTabWidget::pane {"
        "    border: none;"
        "}"
    );

    // 表格
    m_TableWidget = new QTableWidget(72, 2);
    m_TableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section::horizontal{"
        "   background-color: #E8EBF2;"
        "   color: #2C3E50;"
        "}"
    );
    m_TableWidget->verticalHeader()->setStyleSheet(
        "QHeaderView::section::vertical{"
        "   background-color: #E8EBF2;"
        "   color: #2C3E50;"
        "}"
    );
    m_TableWidget->setStyleSheet(
        "QTableWidget {"
        "    border: 1px solid #DCDFE6;"
        "    border-radius: 2px;"          // 圆角半径，数值越大越圆滑
        "    gridline-color: #DCDFE6;"
        "}"
    );
    QHeaderView* verticalHeader = m_TableWidget->verticalHeader();
    verticalHeader->setDefaultAlignment(Qt::AlignCenter);
    m_TableWidget->setHorizontalHeaderLabels({ "X", "Y" });
    m_TableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 表格数据变化时更新图表
    m_tabWidget->addTab(m_TableWidget, "Page 1");

    m_PlotView = new QWidget(this);
    m_PlotView->setObjectName("parent");
    m_PlotView->setStyleSheet(
        "    #parent {"
        "    border: 1px solid #DCDFE6;"
        "    border-radius: 2px;"          // 圆角半径，数值越大越圆滑
        "}"
    );
    QVBoxLayout* plotLayout = new QVBoxLayout(m_PlotView);
    m_Plot1 = new mPlotView(this);
    m_bottomAxis.title = "X";
    m_leftAxis.title = "Y";
    AxisSettings topAxis;
    AxisSettings rightAxis;
    topAxis.isVisible = false;
    rightAxis.isVisible = false;
    m_Plot1->updateAxisAppearance(X_Bottom, m_bottomAxis);
    m_Plot1->updateAxisAppearance(Y_Left, m_leftAxis);
    m_Plot1->updateAxisScale(X_Top, topAxis);
    m_Plot1->updateAxisScale(Y_Right, rightAxis);
    m_Plot1->setPlotLegendItemVisible(false);
    plotLayout->addWidget(m_Plot1);
    m_tabWidget->addTab(m_PlotView, "Page 2");
    mainLayout->addWidget(m_tabWidget, 1);

    // ---- 按钮行 ----
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    m_SaveBtn = new QPushButton("保存");
    m_SaveBtn->setStyleSheet(
        "QPushButton { background-color: #1385F6; color: white;}");
    m_SaveBtn->setFixedSize(44, 26);
    m_CloseBtn = new QPushButton("关闭");
    m_CloseBtn->setFixedSize(44, 26);
    buttonLayout->addWidget(m_CloseBtn);
    buttonLayout->addWidget(m_SaveBtn);
    mainLayout->addLayout(buttonLayout);

    connect(m_TableBtn, &QPushButton::clicked, this, [this]()
        {
            m_tabWidget->setCurrentIndex(0);
        });
    connect(m_ChartBtn, &QPushButton::clicked, this, &CreatePSDCurveDialog::onPlotCurve);
    connect(m_ImportCurve, &QPushButton::clicked, this, &CreatePSDCurveDialog::onImportCurve);
    connect(m_SaveBtn, &QPushButton::clicked, this, &CreatePSDCurveDialog::onSave);
    connect(m_CloseBtn, &QPushButton::clicked, this, &CreatePSDCurveDialog::onClose);
    connect(m_ComboBox_Freq, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int index) {
            m_bottomAxis.isLog = (index == 1);
            m_Plot1->updateAxisLog(X_Bottom, m_bottomAxis);
        });
    connect(m_ComboBox_Load, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int index) {
            m_leftAxis.isLog = (index == 1);
            m_Plot1->updateAxisLog(Y_Left, m_leftAxis);
        });

}

void CreatePSDCurveDialog::onImportCurve()
{
    const QString file = QFileDialog::getOpenFileName(this, QString("导入曲线"), QString(), "曲线文件 (*.csv *.txt)");
    if (file.isEmpty())
        return;
    QVector<double> xs, ys, zs;
    int skipped = 0;
    const int n = importCurveFromCsv(file, 2, xs, ys, zs, skipped);
    if (n < 0)
    {
        QMessageBox::warning(this, QString("提示"), QString("无法打开文件：%1").arg(file));
        return;
    }
    if (n == 0)
    {
        QMessageBox::warning(this, QString("提示"), QString("文件中未找到有效数据行。"));
        return;
    }
    m_TableWidget->clear();
    m_TableWidget->setHorizontalHeaderLabels({ "X", "Y" });
    m_TableWidget->setRowCount(qMax(72, n));
    for (int i = 0; i < n; ++i)
    {
        m_TableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(xs[i], 'g', 15)));
        m_TableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(ys[i], 'g', 15)));
    }
    // 名称未填时用文件名，顺带满足名称必填校验
    if (m_CurveNameEdit->text().trimmed().isEmpty())
        m_CurveNameEdit->setText(QFileInfo(file).baseName());
    m_tabWidget->setCurrentIndex(0);
    if (skipped > 0)
        QMessageBox::information(this, QString("提示"), QString("已导入 %1 行数据，跳过 %2 行无法解析的数据。").arg(n).arg(skipped));
}

void CreatePSDCurveDialog::onSave()
{
    accept();
}

IncentiveCurve CreatePSDCurveDialog::getDialogData() const
{
    IncentiveCurve data;
    data.name = m_CurveNameEdit->text();
    data.freqCombobox = m_ComboBox_Freq->currentIndex();
    data.loadCombobox = m_ComboBox_Load->currentIndex();
    for (int i = 0; i < m_TableWidget->rowCount(); ++i)
    {
        QTableWidgetItem* item1 = m_TableWidget->item(i, 0);
        QTableWidgetItem* item2 = m_TableWidget->item(i, 1);
        if (item1 && !item1->text().isEmpty() && item2 && !item2->text().isEmpty())
        {
            data.colX.push_back(item1->text().toDouble());
            data.colY.push_back(item2->text().toDouble());
        }
        else
        {
            break;
        }
    }
    return data;
}

void CreatePSDCurveDialog::onClose()
{
    reject();
}

void CreatePSDCurveDialog::onPlotCurve()
{
    QVector<double> columnX;
    QVector<double> columnY;
    for (int i = 0; i < m_TableWidget->rowCount(); ++i)
    {
        QTableWidgetItem* item1 = m_TableWidget->item(i, 0);
        QTableWidgetItem* item2 = m_TableWidget->item(i, 1);
        if (item1 && !item1->text().isEmpty() && item2 && !item2->text().isEmpty())
        {
            columnX.push_back(item1->text().toDouble());
            columnY.push_back(item2->text().toDouble());
        }
        else
            break;
    }
    m_Plot1->updateCurveData("curveRealPart", columnX, columnY);
    m_tabWidget->setCurrentIndex(1);
}

bool CreatePSDCurveDialog::isDataValid()
{
    if (m_CurveNameEdit->text().isEmpty())
        return false;
    int count = 0;
    for (int i = 0; i < m_TableWidget->rowCount(); ++i)
    {
        QTableWidgetItem* item1 = m_TableWidget->item(i, 0);
        QTableWidgetItem* item2 = m_TableWidget->item(i, 1);
        if (item1 && !item1->text().isEmpty() && item2 && !item2->text().isEmpty())
        {
            ++count;
        }
        else
            break;
    }
    return count >= 2;
}

/*****************************************ComboDeleteItemDelegate******************************************************/
ComboDeleteItemDelegate::ComboDeleteItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void ComboDeleteItemDelegate::setDeleteIcon(const QIcon& icon, bool skipFirstRow)
{
    m_deleteIcon = icon;
    m_skipFirstRow = skipFirstRow;
}

QRect ComboDeleteItemDelegate::deleteRect(const QStyleOptionViewItem& option) const
{
    const int size = 14;
    return QRect(option.rect.right() - size - 8,
        option.rect.top() + (option.rect.height() - size) / 2,
        size, size);
}

void ComboDeleteItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);
    if (m_deleteIcon.isNull() || (m_skipFirstRow && index.row() == 0))
    {
        return;
    }
    m_deleteIcon.paint(painter, deleteRect(option));
}

bool ComboDeleteItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    Q_UNUSED(model);
    if (!m_deleteIcon.isNull()
        && !(m_skipFirstRow && index.row() == 0)
        && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease))
    {
        const QMouseEvent* mouseEvent = static_cast<const QMouseEvent*>(event);
        if (deleteRect(option).contains(mouseEvent->pos()))
        {
            if (event->type() == QEvent::MouseButtonRelease)
            {
                emit deleteRequested(index);
            }
            // 按下与释放都吞掉，避免触发选中/激活导致下拉框收起
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

/*****************************************SplitButtonEdit******************************************************/
SplitButtonEdit::SplitButtonEdit(bool singleCurve, QWidget* parent)
    : m_singleCurve(singleCurve), QWidget(parent)
{
    this->setStyleSheet(R"(
        SplitButtonEdit:disabled {
            background-color: #F5F7FA;
        }
    )");
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);   // 右边留2px，避免按钮贴边
    m_layout->setSpacing(0);

    // 输入框 —— 宽高都撑满
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setFrame(false);
    m_lineEdit->setFixedSize(28, 24);
    m_lineEdit->setStyleSheet(
        "QLineEdit {"
        "   background: transparent;"
        "   border: none;"
        "   padding: 0px 4px;"          // 左右留一点文字空隙，上下0px确保撑满
        "}"
        "QLineEdit:disabled{"
        "  background-color: #F5F7FA;"
        "}"
    );

    // 竖线
    m_separator = new QFrame(this);
    m_separator->setFrameShape(QFrame::VLine);
    m_separator->setFrameShadow(QFrame::Plain);
    m_separator->setFixedWidth(1);
    m_separator->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);  // 竖线也撑满高度
    m_separator->setStyleSheet(QString("color: #DCDFE6;"));

    // 按钮 
    m_button = new QPushButton(this);
    m_button->setFlat(true);
    m_button->setFixedSize(24, 24);
    m_button->setCursor(Qt::PointingHandCursor);
    m_button->setStyleSheet(
        "QPushButton {"
        "   background: transparent;"
        "   border: none;"
        "   padding: 0px;"             
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(0,0,0,0.05);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(0,0,0,0.1);"
        "}"
        "QPushButton:disabled{"
         "  background-color: #F5F7FA;"
        "}"
    );

    // 输入框为力/力矩幅值，仅允许数值，非法输入直接置空
    installDoubleInputFilter(m_lineEdit);

    // 组装
    m_layout->addWidget(m_lineEdit);
    m_layout->addWidget(m_separator);
    m_layout->addWidget(m_button);
    setFixedSize(52,24);
    // 连接按钮点击信号
    connect(this, &SplitButtonEdit::CurveStatusChanged,this,[this](bool isCompleted) {
        m_isCompleted = isCompleted;
        });
    connect(m_button, &QPushButton::clicked, this, [this]()
        {
            if (m_singleCurve) 
            {
                if (m_curveData.name.isEmpty())
                    CreateSingleCurveDialog::getInstance()->initDialog();
                else
                    CreateSingleCurveDialog::getInstance()->setDialogData(m_curveData);
                CreateSingleCurveDialog::getInstance()->m_currentButton = this;
                CreateSingleCurveDialog::getInstance()->exec();
            }
            else
            {
                if (m_curveData.name.isEmpty())
                    CreateCurveDialog::getInstance()->initDialog();
                else
                    CreateCurveDialog::getInstance()->setDialogData(m_curveData);
                CreateCurveDialog::getInstance()->m_currentButton = this;
                CreateCurveDialog::getInstance()->exec();
            }
        });
}

void SplitButtonEdit::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 画背景
    QRect rect = this->rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(rect, m_borderRadius, m_borderRadius);

    painter.fillPath(path, Qt::white);

    // 画边框
    QPen pen(m_borderColor);
    pen.setWidth(m_borderWidth);
    painter.setPen(pen);
    painter.drawPath(path);
}

void SplitButtonEdit::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // 竖线高度随控件高度自适应
    m_separator->setFixedHeight(height());
}

void SplitButtonEdit::setText(const QString& text)
{
    m_lineEdit->setText(text);
}

QString SplitButtonEdit::text() const
{
    return m_lineEdit->text();
}

void SplitButtonEdit::setPlaceholderText(const QString& text)
{
    m_lineEdit->setPlaceholderText(text);
}

void SplitButtonEdit::setButtonIcon(const QString& imagePath)
{
    m_button->setIcon(QIcon(imagePath));
}

void SplitButtonEdit::setCurveData(const IncentiveCurve& data)
{
    m_curveData = data;
}

IncentiveCurve SplitButtonEdit::getCurveData() { return m_curveData; }
/*****************************************IncentiveSettingTableWidget******************************************************/
IncentiveSettingTableWidget::IncentiveSettingTableWidget(bool singCurve,QWidget* parent)
    : m_SingleCurve(singCurve), QTableWidget(parent)
{
    setupAppearance();
    refreshHeader();

    // 使用自定义表头
    mCheckBoxHeader* header = qobject_cast<mCheckBoxHeader*>(horizontalHeader());

    // 连接表头复选框信号：同步到对应列所有单元格复选框
    if (header)
    {
        connect(header, &mCheckBoxHeader::columnCheckStateChanged, this, [this](int column, int state) {
            if (column < 1 || column > 6)
                return;
            bool checked = (state == Qt::Checked);
            for (int row = 0; row < rowCount(); ++row) {
                if (QWidget* w = cellWidget(row, column)) {
                    if (QCheckBox* cb = w->findChild<QCheckBox*>()) {
                        cb->setChecked(checked);
                    }
                }
            }
            });
    }

    // 节点ID与标签列的占位符颜色处理
    connect(this, &IncentiveSettingTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (!item)
            return;
        int col = item->column();
        if (col != 0 )
            return;
        QString text = item->text().trimmed();
        // ID列仅允许数字，非法输入直接置回占位符
        if (text != "请输入" && !g_tableIdPattern.match(text).hasMatch())
            text.clear();
        if (text.isEmpty())
        {
            QSignalBlocker blocker(this);
            item->setText("请输入");
            item->setForeground(QBrush(QColor("#909399")));
        }
        else if (text != "请输入")
        {
            item->setForeground(QBrush(QColor(0, 0, 0)));
        }
        });

    // 默认保留一行
    addRow();
}

IncentiveSettingTableWidget::~IncentiveSettingTableWidget() {}

void IncentiveSettingTableWidget::refreshHeader()
{
    setColumnCount(7);
    setHorizontalHeaderLabels(QStringList()
            << "节点ID" << "X" << "Y" << "Z" << "RX" << "RY" << "RZ");

    mCheckBoxHeader* header = qobject_cast<mCheckBoxHeader*>(horizontalHeader());
    if (header)
    {
        header->clearAllCheckBoxes();
        for (int col = 1; col < 7; ++col)
        {
            header->setColumnCheckBox(col, true);
        }
    }

    // 第2~7列（X/Y/Z/RX/RY/RZ）设置左对齐
    for (int col = 1; col < 7; ++col)
    {
        if (horizontalHeaderItem(col))
        {
            horizontalHeaderItem(col)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }
    if (horizontalHeaderItem(0))
    {
        horizontalHeaderItem(0)->setTextAlignment(Qt::AlignCenter);
    }
}

void IncentiveSettingTableWidget::setupAppearance()
{
    // 使用自定义表头
    mCheckBoxHeader* header = new mCheckBoxHeader(Qt::Horizontal, this);
    setHorizontalHeader(header);

    // 隐藏垂直表头（序号列）
    verticalHeader()->setVisible(false);
    horizontalHeader()->setHighlightSections(false);
    // 水平表头列宽自动拉伸，平均分配
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 滚动条按需显示，像素级平滑滚动
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 整行选择，支持多选
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAlternatingRowColors(true);
    setShowGrid(false);
    // 表头样式：背景 #DCDFE6，深色文字。
    // 给表头设置唯一 objectName，使用 ID 选择器提高优先级，覆盖全局 QSS 中的 QHeaderView::section:horizontal
    ///horizontalHeader()->setObjectName("incentiveNodeHeader");
    horizontalHeader()->setStyleSheet(
        "QHeaderView::section:horizontal {"
        "   background-color: #E8EBF2;"
        "   color: #2C3E50;"
        "   border: 1px solid #DCDFE6;"
        "   padding-left: 28px;"
        "}"
        "QHeaderView::section:horizontal:first {"
        "   padding-left: 0px;"
        "}"
    );

    // 表格样式：选中行背景 #D0E7FD，文字灰色；数据行单元格只保留上下横线，去掉左右竖线
    setStyleSheet(
        "QTableWidget {"
        "    border: 1px solid #DCDFE6;"
        "    border-radius: 2px;"          // 圆角半径，数值越大越圆滑
        "    gridline-color: #DCDFE6;"
        "   alternate-background-color: #F6F8FA;"   /* 奇数行背景 */
        "   background-color: #FFFFFF;"             /* 偶数行背景 */
        "   selection-background-color: #D0E7FD;"
        "}"
        "QTableWidget::item {"
        "  border: none;"
        "  border-bottom: 1px solid #DCDFE6;"
        "}"
    );
}

void IncentiveSettingTableWidget::initRow(int row)
{
    QTableWidgetItem* idItem = new QTableWidgetItem();
    idItem->setFlags(idItem->flags() | Qt::ItemIsEditable);
    idItem->setTextAlignment(Qt::AlignCenter);
    idItem->setText("请输入");
    idItem->setForeground(QBrush(QColor("#909399")));
    setItem(row, 0, idItem);

    // 第2~7列：X/Y/Z/RX/RY/RZ —— 左对齐真实复选框，无文字
    for (int col = 1; col < 7; ++col) {
        QWidget* widget = new QWidget(this);
        widget->setStyleSheet("background-color: transparent;");

        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(8, 0, 0, 0);
        layout->setSpacing(0);

        QCheckBox* checkBox = new QCheckBox(widget);
        checkBox->setText("");
        checkBox->setStyleSheet(
            "QCheckBox { background-color: transparent; spacing: 0px; padding: 0px; margin: 0px; border: none; }"
            "QCheckBox::indicator { width: 14px; height: 14px; background-color: #FFFFFF;  }"
            "QCheckBox::indicator:checked { background-color: #FFFFFF;  }"
            "QCheckBox::indicator:unchecked { background-color: #FFFFFF; }"
        );
        SplitButtonEdit* editButton = new SplitButtonEdit(m_SingleCurve,widget);
        editButton->setText("1.0");
        editButton->setButtonIcon(":/mMainWidgets/Icon/NVH/quxian_zhiwei.png");
        editButton->setEnabled(false);
        connect(checkBox, &QCheckBox::toggled, this, [=](bool checked) {
            if (editButton)
            {
                editButton->setEnabled(checked);
                if (checked && editButton->m_isCompleted)
                {
                    editButton->setButtonIcon(":/mMainWidgets/Icon/NVH/quxian_wancheng.png");
                }
                else if(checked && !editButton->m_isCompleted)
                {
                    editButton->setButtonIcon(":/mMainWidgets/Icon/NVH/quxian_weishezhi.png");
                }
            }
            });
        layout->addWidget(checkBox);
        layout->addSpacing(8);
        layout->addWidget(editButton);
        layout->addStretch();
        layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        setCellWidget(row, col, widget);
    }
}

void IncentiveSettingTableWidget::addRow()
{
    int row = rowCount();
    insertRow(row);
    initRow(row);
}

void IncentiveSettingTableWidget::resetToDefault()
{
    setRowCount(0);
    addRow();
}

void IncentiveSettingTableWidget::removeSelectedRows()
{
    QModelIndexList selected = selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QList<int> rows;
    for (const QModelIndex& index : selected) {
        rows.append(index.row());
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());

    for (int row : rows) {
        removeRow(row);
    }
}

void IncentiveSettingTableWidget::setRowCount(int count)
{
    if (count < 0) count = 0;

    int current = rowCount();
    if (count == current) return;

    if (count > current) {
        for (int i = current; i < count; ++i) {
            insertRow(i);
            initRow(i);
        }
    }
    else {
        for (int i = current - 1; i >= count; --i) {
            removeRow(i);
        }
    }
}

int IncentiveSettingTableWidget::getRowID(int row) const
{
    if (row < 0 || row >= rowCount())
        return -1;

    // 第1列：节点ID；空行/占位符/非法输入视为无ID（返回-1），由调用方按空行直接跳过
    if (QTableWidgetItem* idItem = item(row, 0))
    {
        const QString text = idItem->text().trimmed();
        if (text.isEmpty() || text == "请输入")
            return -1;
        bool ok = false;
        const int id = text.toInt(&ok);
        if (!ok || id <= 0)
            return -1;
        return id;
    }
    return -1;
}

bool IncentiveSettingTableWidget::isItemChecked(int row, int col)
{
    if (col < 1 || col > 6) return false;
    QWidget* w = cellWidget(row, col);
    QCheckBox* cb = w->findChild<QCheckBox*>();
    return cb->isChecked();
}

IncentiveCurve IncentiveSettingTableWidget::getItemData(int row, int col)
{
    QWidget* w = cellWidget(row, col);
    SplitButtonEdit* be = w->findChild<SplitButtonEdit*>();
    return be->getCurveData();
}

double IncentiveSettingTableWidget::getWeightValue(int row, int col)
{
    QWidget* w = cellWidget(row, col);
    SplitButtonEdit* be = w->findChild<SplitButtonEdit*>();
    return be->text().toDouble();
}

/*****************************************ButtonHeaderView******************************************************/
ButtonHeaderView::ButtonHeaderView(Qt::Orientation orientation, QWidget* parent)
    : QHeaderView(orientation, parent)
    , m_buttonColumn(3)          // 第4列索引为3
    , m_text(QStringLiteral("工况组合"))
{
    setSectionsClickable(true);
}

void ButtonHeaderView::setButtonIcon(const QIcon& icon)
{
    m_buttonIcon = icon;
    viewport()->update();  // 请求重绘
}

void ButtonHeaderView::setText(const QString& text)
{
    m_text = text;
    viewport()->update();
}

QRect ButtonHeaderView::buttonRect(const QRect& sectionRect) const
{
    // 按钮位于该列右侧，留出边距，大小固定为 20x20（可根据需要调整）
    int size = 20;
    int margin = 35;
    return QRect(sectionRect.right() - size - margin,
        sectionRect.top() + (sectionRect.height() - size) / 2,
        size, size);
}

void ButtonHeaderView::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
    if (logicalIndex == m_buttonColumn) {
        // 1. 手动绘制背景和边框（与样式表一致）
        painter->save();
        painter->setPen(QColor("#d0d0d0"));
        painter->setBrush(QColor("#E8EBF2"));
        painter->drawRect(rect.adjusted(0, 0, -1, -1));
        painter->restore();

        // 2. 绘制文字（颜色为默认按钮文字颜色，不加粗）
        painter->save();
        painter->setPen(QColor("#A8ABB2"));
        QFont font = painter->font();
        // 不设置加粗，保持与其他列一致
        painter->setFont(font);
        QRect textRect = rect.adjusted(8, 0, -buttonRect(rect).width() - 12, 0);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_text);
        painter->restore();

        // 3. 绘制按钮
        QStyleOptionButton buttonOption;
        buttonOption.initFrom(this);
        buttonOption.rect = buttonRect(rect);
        buttonOption.icon = m_buttonIcon;
        buttonOption.iconSize = QSize(16, 16);
        buttonOption.text.clear();
        buttonOption.state = QStyle::State_Enabled;
        style()->drawControl(QStyle::CE_PushButton, &buttonOption, painter, this);
    }
    else {
        QHeaderView::paintSection(painter, rect, logicalIndex);
    }
}
void ButtonHeaderView::mousePressEvent(QMouseEvent* event)
{
    int logicalIndex = logicalIndexAt(event->pos());
    if (logicalIndex == m_buttonColumn) {
        QRect btnRect = buttonRect(sectionViewportPosition(logicalIndex) == -1
            ? QRect() : QRect(sectionViewportPosition(logicalIndex), 0,
                sectionSize(logicalIndex), height()));
        // 更精确：根据点击位置计算按钮矩形
        // 由于 buttonRect 需要 section 的矩形，我们可以通过 sectionViewportPosition 和 sectionSize 获得
        QRect sectionRect(sectionViewportPosition(logicalIndex), 0,
            sectionSize(logicalIndex), height());
        QRect btnRectActual = buttonRect(sectionRect);
        if (btnRectActual.contains(event->pos()) && event->button() == Qt::LeftButton) {
            emit buttonClicked(logicalIndex);
            event->accept();
            return;
        }
    }
    QHeaderView::mousePressEvent(event);
}