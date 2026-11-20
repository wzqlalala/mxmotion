
#include "mCaseSettingPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"
#include "mInfoWindow.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QDebug>

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mCaseImplicitLinearStatic.h"
#include "mCaseImplicitModal.h"
#include "mCaseImplicitNonLinearStatic.h"
#include "mCaseImplicitLinearBuckling.h"
#include "mCaseImplicitTransient.h"
#include "mFunctionManager.h"
#include "mFunctionBase.h"
#include "mSetManager.h"
#include "mSetBase.h"

#include "mModelData.h"
#include "mMeshData.h"

#include "mGeneralPickUp.h"
#include "mMechGlobalSignals.h"
#include "mGetSetTypeTemplate.h"
#include "mGetSetFunction.h"

using namespace MechPreData;
using namespace MechCaseData;
using namespace MechSetData;
using namespace MechFunctionData;
using namespace MxFunctions;
namespace MechMainWindow
{
    QHash<QString, QHash<QString, OutputControlWidget::VariableType>> OutputControlWidget::m_sequenceValues;
    QStringList OutputControlWidget::m_sequence;

    // PickerShell：输出控制树节点上的拾取控件挂载壳。
    // 壳的生命周期由树管理（clear 重置 / setItemWidget 替换 / 整树级联销毁都是合法操作），
    // 真正的拾取视图归面板所有：析构时若视图仍挂在壳上，通知面板摘回寄存容器。
    // 依赖 C++ 析构顺序：~QWidget 体内子控件尚未销毁（~QObject 才删子控件），此时归还安全
    class PickerShell : public QWidget
    {
    public:
        PickerShell(QPointer<QWidget> view, std::function<void()> onDetach, QWidget* parent = nullptr)
            : QWidget(parent), m_view(std::move(view)), m_onDetach(std::move(onDetach))
        {
            QHBoxLayout* rowH = new QHBoxLayout(this);
            rowH->setContentsMargins(0, 0, 0, 0);
            rowH->setSpacing(4);
        }

        ~PickerShell() override
        {
            // 视图已被重挂到新壳（重建路径）时 parentWidget 不再是自己，跳过归还
            if (m_view && m_view->parentWidget() == this && m_onDetach)
                m_onDetach();
        }

    private:
        QPointer<QWidget> m_view;
        std::function<void()> m_onDetach;
    };

    OutputControlWidget::OutputControlWidget(QHash<QString, QStringList> structure,
        QHash<QString, QStringList> currentSelections,
        shared_ptr<QHash<QString, mPickWidgetController*>> pickers,
        QWidget* parent)
        : QWidget(parent), m_structure(structure), m_selections(currentSelections), m_pickers(pickers), m_isProgrammaticChange(false)
    {
        m_sequenceValues["基本结果"] = QHash<QString, VariableType>{
            {"位移", VariableType::NodeSet},
            {"速度",VariableType::NodeSet },
            {"加速度",VariableType::NodeSet}
        };
        m_sequenceValues["应力"] = QHash<QString, VariableType>{
            {"节点应力", VariableType::NodeSet},
            {"单元应力",VariableType::ElementSet },
            {"简单磨平应力",VariableType::NodeSet},
            {"单元高斯积分点应力",VariableType::None}
        };
        m_sequenceValues["应变"] = QHash<QString, VariableType>{
            {"单元应变", VariableType::ElementSet},
            {"节点应变",VariableType::NodeSet },
            {"节点等效塑性应变",VariableType::NodeSet},
            {"节点等效蠕变应变",VariableType::NodeSet},
            {"单元等效塑性应变", VariableType::ElementSet},
            {"简单磨平应变",VariableType::NodeSet },
            {"简单磨平等效塑性应变",VariableType::NodeSet}
        };
        m_sequenceValues["能量"] = QHash<QString, VariableType>{
            {"节点应变能", VariableType::NodeSet},
            {"单元应变能",VariableType::ElementSet },
            {"简单磨平应变能",VariableType::NodeSet},
            {"应变能密度",VariableType::NodeSet}
        };
        m_sequenceValues["热学"] = QHash<QString, VariableType>{
            {"温度", VariableType::NodeSet},
            {"节点温度梯度",VariableType::NodeSet },
            {"单元温度梯度",VariableType::ElementSet},
            {"节点热流密度",VariableType::NodeSet},
            {"单元热流密度",VariableType::ElementSet}
        };
        //m_sequenceValues["拓扑优化"] = QStringList{ "节点密度","单元密度" };

        m_sequenceValues["PSD谱分析"] = QHash<QString, VariableType>{
            {"节点位移功率谱", VariableType::NodeSet},
            {"节点速度功率谱",VariableType::NodeSet},
            {"节点加速度功率谱",VariableType::NodeSet},
            {"节点应变功率谱",VariableType::NodeSet},
            {"节点应力功率谱",VariableType::NodeSet},
            {"单元应力功率谱",VariableType::ElementSet},
            {"单元应变功率谱",VariableType::ElementSet},
            {"单元磨平应力功率谱",VariableType::NodeSet},
            {"单元磨平应变功率谱",VariableType::NodeSet}
        };
        m_sequenceValues["RMS谱分析"] = QHash<QString, VariableType>{
            {"节点位移1σ均方根", VariableType::NodeSet},
            {"节点位移2σ均方根",VariableType::NodeSet},
            {"节点位移3σ均方根",VariableType::NodeSet},
            {"单元磨平应力1σ均方根",VariableType::NodeSet},
            {"单元磨平应力2σ均方根",VariableType::NodeSet},
            {"单元磨平应力3σ均方根",VariableType::NodeSet},
            {"单元应力均方根",VariableType::ElementSet}
        };
        m_sequenceValues["复合材料"] = QHash<QString, VariableType>{
            {"单元复合应变", VariableType::ElementSet},
            {"单元复合应力",VariableType::ElementSet},
            {"单元失效指数",VariableType::ElementSet},
            {"hashin失效系数",VariableType::NodeSet},
            {"单元损伤状态变量",VariableType::ElementSet},
            {"铺层温度",VariableType::NodeSet},
            {"单元失效强度比",VariableType::ElementSet},
            {"单元铺层应力",VariableType::ElementSet},
            {"实体铺层节点应力",VariableType::NodeSet},
            {"铺层划分",VariableType::NodeSet},
        };
        m_sequenceValues["结构力与内力"] = QHash<QString, VariableType>{
            {"支反力", VariableType::NodeSet},
            {"多点约束力",VariableType::NodeSet},
            {"杆梁节点内力",VariableType::NodeSet},
            {"梁截面内力",VariableType::NodeSet},
            {"接触力",VariableType::None},
            {"602载荷",VariableType::NodeSet},
            {"轴力",VariableType::NodeSet},
            {"单元节点力",VariableType::NodeSet},
            {"刚性墙",VariableType::NodeSet},
        };
        m_sequenceValues["声学"] = QHash<QString, VariableType>{
            {"声压级(分贝)", VariableType::NodeSet},
            {"声压(帕斯卡)",VariableType::NodeSet }
        };
        m_sequenceValues["损伤"] = QHash<QString, VariableType>{
            {"损伤", VariableType::NodeSet},
            {"拉伸损伤",VariableType::NodeSet },
            {"压缩损伤",VariableType::NodeSet},
            {"损伤起始准则",VariableType::NodeSet},
            {"剪应力比",VariableType::NodeSet}
        };
        m_sequenceValues["节点贡献量"] = QHash<QString, VariableType>{
            {"结构节点贡献量", VariableType::NodeSet},
            {"声腔节点贡献量",VariableType::NodeSet }
        };

        m_sequence = QStringList({ "基本结果","应力","应变","能量","热学","PSD谱分析","RMS谱分析","复合材料","结构力与内力","声学","损伤","节点贡献量" });

        initUI();
        initTreeData();
        updateTopLabel(); // 初始化显示
    }

    void OutputControlWidget::initUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(2, 2, 2, 2);
        mainLayout->setSpacing(5);

        // 1. 创建顶部容器 Widget
        QWidget* topContainer = new QWidget(this);
        QHBoxLayout* topLayout = new QHBoxLayout(topContainer);
        topLayout->setContentsMargins(0, 0, 0, 0);
        topLayout->setSpacing(5);

        // 2. 左侧：显示 Label 
        m_displayLabel = new QLabel(this);
        m_displayLabel->setWordWrap(true);
        m_displayLabel->setStyleSheet("QLabel { border: 1px solid #ccc; background: #f9f9f9; padding: 4px; color: #333; }");
        m_displayLabel->setText("无选中项");
        m_displayLabel->setMinimumHeight(40);
        // 设为 Expanding，让它占据左侧主要空间
        m_displayLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        topLayout->addWidget(m_displayLabel);

        // 3. 右侧：垂直布局放按钮 + 弹簧
        QVBoxLayout* rightBtnLayout = new QVBoxLayout();
        rightBtnLayout->setContentsMargins(0, 0, 0, 0);
        rightBtnLayout->setSpacing(0);

        // 创建方形按钮
        m_resetBtn = new QToolButton(this);
        m_resetBtn->setFixedSize(24, 24); // 方形尺寸
        m_resetBtn->setToolTip("恢复默认设置");
        m_resetBtn->setIcon(QIcon(":/mMainWidgets/Icon/post/reset.png"));

        connect(m_resetBtn, &QPushButton::clicked, this, &OutputControlWidget::onResetClicked);

        rightBtnLayout->addWidget(m_resetBtn);
        rightBtnLayout->addStretch(); // 【关键】垂直弹簧，把按钮顶上去

        topLayout->addLayout(rightBtnLayout);

        // 将顶部容器加入主布局
        mainLayout->addWidget(topContainer);

        // 2. 中间：过滤输入框
        m_filterEdit = new QLineEdit(this);
        m_filterEdit->setPlaceholderText("搜索变量名称...");
        m_filterEdit->setClearButtonEnabled(true);
        connect(m_filterEdit, &QLineEdit::textChanged, this, &OutputControlWidget::onFilterTextChanged);
        mainLayout->addWidget(m_filterEdit);

        // 3. 下方：QFrame 框住的 QTreeWidget
        QFrame* treeFrame = new QFrame(this);
        treeFrame->setMinimumHeight(400);
        treeFrame->setFrameShape(QFrame::StyledPanel);
        QVBoxLayout* frameLayout = new QVBoxLayout(treeFrame);
        frameLayout->setContentsMargins(0, 0, 0, 0);

        m_treeWidget = new QTreeWidget(treeFrame);
        m_treeWidget->setStyleSheet(
            "QTreeWidget{font-size: 14px;}"
            "QTreeWidget::item{height:30px;margin-bottom: 2px;}"
            "QTreeView::indicator { width: 18px; height: 18px;}"
            "QTreeView::indicator:unchecked { image: url(:/mMainWidgets/Icon/uncheck3x.png); }"
            "QTreeView::indicator:indeterminate {  image: url(:/mMainWidgets/Icon/partcheck3x.png);}"
            "QTreeView::indicator:checked   { image: url(:/mMainWidgets/Icon/check3x.png); }"
            //"QTreeView::indicator:indeterminate { image: url(:/mMainWidgets/Icon/check3x.png); }"
        );
        m_treeWidget->setUniformRowHeights(true);
        m_treeWidget->setHeaderHidden(true); // 隐藏表头
        m_treeWidget->setFocusPolicy(Qt::NoFocus);
        if (m_pickers.lock() != nullptr)
        {
            m_treeWidget->setColumnCount(2);
            m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Interactive);
        }
        else {
            m_treeWidget->setColumnCount(1);
        }
        connect(m_treeWidget, &QTreeWidget::itemChanged, this, &OutputControlWidget::onItemChanged);

        frameLayout->addWidget(m_treeWidget);
        mainLayout->addWidget(treeFrame);
    }

    void OutputControlWidget::initTreeData()
    {
        refreshTreeState();
    }

    void OutputControlWidget::setDefaultOutputs(const QHash<QString, QStringList>& defaults)
    {
        m_defaults = defaults;
        onResetClicked();
    }

    void OutputControlWidget::resetTreeData(const QHash<QString, QStringList>& structure, const QHash<QString, QStringList>& defaults){
        m_structure = structure;
        m_defaults = defaults;
        //initUI();
        onResetClicked();
    }

    OutputControlWidget::VariableType OutputControlWidget::getVariableType(const QString& key, const QString& value) {
        if (!m_sequenceValues.contains(key) || !m_sequenceValues.value(key).contains(value))return OutputControlWidget::VariableType::None;
        return m_sequenceValues.value(key).value(value);
    }

    void OutputControlWidget::setTreeMinimumHeight(int height)
    {
        if (m_treeWidget)
        {
            m_treeWidget->setMinimumHeight(height);
        }
    }

    void OutputControlWidget::onResetClicked()
    {
        m_selections = m_defaults;

        refreshTreeState();
        emit outputsChanged(getFinalOutputs());
    }

    void OutputControlWidget::onItemChanged(QTreeWidgetItem* item, int column)
    {
        // 如果是代码内部自动调整（比如父节点带子节点），不重复触发信号
        if (m_isProgrammaticChange)
            return;

        m_isProgrammaticChange = true; // 上锁

        if (item->childCount() > 0) {
            updateChildrenState(item);
            for (int i = 0; i < item->childCount(); ++i) {
                updatePickWidgetVisibility(item->child(i));
            }
        }
        else {
            updatePickWidgetVisibility(item);
            if (item->parent()) {
                updateParentState(item->parent());
            }
        }
        updateTopLabel();

        m_isProgrammaticChange = false; // 解锁

        auto output = getFinalOutputs();
        emit outputsChanged(output);
    }
    // 创建空的挂载壳；视图由面板在挂载时放入，壳被树销毁时自动通知面板归还
    QWidget* OutputControlWidget::createPickerShell(const QString& var)
    {
        auto pickers = m_pickers.lock();
        if (pickers == nullptr)return nullptr;
        auto it = pickers->constFind(var);
        if (it == pickers->constEnd() || !it.value() || !it.value()->widget())return nullptr;
        QPointer<QWidget> view = it.value()->widget();
        auto unmounter = m_pickerUnmounter;
        return new PickerShell(view, [var, unmounter] { if (unmounter) unmounter(var, QString()); });
    }

    //按树顺序查找 varName 之外下一个已勾选的拾取变量（接力激活目标）
    QString OutputControlWidget::nextCheckedPickVar(const QString& varName) const
    {
        auto pickers = m_pickers.lock();
        if (pickers == nullptr || m_treeWidget == nullptr)return QString();
        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* root = m_treeWidget->topLevelItem(i);
            for (int j = 0; j < root->childCount(); ++j) {
                QTreeWidgetItem* child = root->child(j);
                const QString name = child->text(0);
                if (name == varName)continue;
                if (!pickers->contains(name))continue;
                if (child->checkState(0) == Qt::Checked)return name;
            }
        }
        return QString();
    }

    //按树顺序查找是否存在其他拾取变量（激活第一个目标）
    bool OutputControlWidget::hasOtherCheckedPickVar(const QString& varName) const
    {
        auto pickers = m_pickers.lock();
        if (pickers == nullptr || m_treeWidget == nullptr)return true;
        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* root = m_treeWidget->topLevelItem(i);
            for (int j = 0; j < root->childCount(); ++j) {
                QTreeWidgetItem* child = root->child(j);
                const QString name = child->text(0);
                if (name == varName)continue;
                if (!pickers->contains(name))continue;
                if (pickers->value(name) != nullptr && pickers->value(name)->isPickActive())return true;
            }
        }
        return false;
    }

    void OutputControlWidget::updatePickWidgetVisibility(QTreeWidgetItem* item) {
        if (item == nullptr)return;
        auto pickers = m_pickers.lock();
        if (pickers == nullptr)return;
        QString varName = item->text(0);
        if (!pickers->contains(varName))return;

        if (item->checkState(0) == Qt::Checked) {
            //已挂载的行直接跳过：全选父节点时未变化的子节点也会被显式调用，避免无谓的拆壳重挂
            if (m_treeWidget->itemWidget(item, 1) != nullptr)return;
            //重复挂载前先卸下旧视图，保证行上始终只有当前壳（卸载幂等，无接力目标）
            if (m_pickerUnmounter) m_pickerUnmounter(varName, QString());
            if (!m_pickerMounter)return;
            bool checked = !hasOtherCheckedPickVar(varName);
            m_pickerMounter(varName, nullptr, false);      //先确保控制器/视图存在（失效会重建并刷新哈希）
            QWidget* shell = createPickerShell(varName);
            if (shell == nullptr)return;
            m_pickerMounter(varName, shell, checked);        //视图挂进壳
            m_treeWidget->setItemWidget(item, 1, shell);
        }
        else {
            //取消勾选：若是激活态拾取控件所在行，接力给下一个已勾选的拾取变量；
            //程序化批量变更（父节点联动/复位重建）期间不接力，由末次失活统一延迟恢复
            const QString nextVar = nextCheckedPickVar(varName);
            if (m_pickerUnmounter) m_pickerUnmounter(varName, nextVar);
            m_treeWidget->setItemWidget(item, 1, nullptr);
        }
    }

    void OutputControlWidget::setPickerMounter(const std::function<void(const QString& var, QWidget* host, bool checked)>& mount) {
        m_pickerMounter = mount;
    }

    void OutputControlWidget::setPickerUnmounter(const std::function<void(const QString& var, const QString& nextVar)>& unmount) {
        m_pickerUnmounter = unmount;
    }

    void OutputControlWidget::refreshTreeState()
    {
        m_treeWidget->setUpdatesEnabled(false); //重建期间抑制重绘，减少批量构建时的卡顿
        m_isProgrammaticChange = true; // 暂停信号，防止重置过程中频繁触发
        
        m_treeWidget->clear(); // 清空当前树
        for (auto category : m_sequence)
        {
            if (!m_structure.contains(category))
            {
                continue;
            }
            auto VariableToType = m_sequenceValues.value(category);
            QStringList sequenceItems = VariableToType.keys();
            QStringList items = m_structure.value(category);
           
            QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
            rootItem->setText(0, category);
            rootItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            rootItem->setCheckState(0, Qt::Unchecked);

            // 获取该类别下的选中项
            QStringList selectedItemsInCat;
            if (m_selections.contains(category)) {
                selectedItemsInCat = m_selections[category];
            }

            for (const QString& itemName : sequenceItems) {
                if (!items.contains(itemName))continue;
                QTreeWidgetItem* childItem = new QTreeWidgetItem(rootItem);
                childItem->setText(0, itemName);
                childItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);


                if (selectedItemsInCat.contains(itemName)) {
                    childItem->setCheckState(0, Qt::Checked);
                }
                else {
                    childItem->setCheckState(0, Qt::Unchecked);
                }
                updatePickWidgetVisibility(childItem);
            }
            updateParentState(rootItem);
        }

        m_treeWidget->collapseAll();
        const QFontMetrics fm(m_treeWidget->font());
        int col0Width = 0;
        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* topItem = m_treeWidget->topLevelItem(i);
            col0Width = qMax(col0Width, fm.horizontalAdvance(topItem->text(0)));
            for (int j = 0; j < topItem->childCount(); ++j) {
                col0Width = qMax(col0Width, fm.horizontalAdvance(topItem->child(j)->text(0)) + m_treeWidget->indentation());
            }
        }
        col0Width += 2 * m_treeWidget->indentation() + 64;
        m_treeWidget->setColumnWidth(0, col0Width);
        updateTopLabel(); // 刷新文字显示

        m_isProgrammaticChange = false; // 恢复信号
        m_treeWidget->setUpdatesEnabled(true);
    }

    void OutputControlWidget::updateChildrenState(QTreeWidgetItem* parentItem)
    {
        Qt::CheckState state = parentItem->checkState(0);

        // 如果父节点被点击变成部分选中（通常UI点击不会直接变部分选中，但为了逻辑严谨），视为全选
        if (state == Qt::PartiallyChecked) state = Qt::Checked;

        for (int i = 0; i < parentItem->childCount(); ++i) {
            parentItem->child(i)->setCheckState(0, state);
        }
    }
    // 检查所有子节点，反向更新父节点为 选中/不选/半选
    void OutputControlWidget::updateParentState(QTreeWidgetItem* parentItem)
    {
        int checkedCount = 0;
        int childCount = parentItem->childCount();

        for (int i = 0; i < childCount; ++i) {
            if (parentItem->child(i)->checkState(0) == Qt::Checked) {
                checkedCount++;
            }
        }

        if (checkedCount == 0) {
            parentItem->setCheckState(0, Qt::Unchecked);
        }
        else if (checkedCount == childCount) {
            parentItem->setCheckState(0, Qt::Checked);
        }
        else {
            parentItem->setCheckState(0, Qt::PartiallyChecked);
        }
    }

    void OutputControlWidget::onFilterTextChanged(const QString& text)
    {
        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* rootItem = m_treeWidget->topLevelItem(i);
            bool rootMatch = rootItem->text(0).contains(text, Qt::CaseInsensitive);
            bool anyChildMatch = false;

            for (int j = 0; j < rootItem->childCount(); ++j) {
                QTreeWidgetItem* child = rootItem->child(j);
                bool childMatch = child->text(0).contains(text, Qt::CaseInsensitive);

                // 如果搜索词为空，或者匹配，显示；否则隐藏
                child->setHidden(!text.isEmpty() && !childMatch && !rootMatch);

                if (childMatch) anyChildMatch = true;
            }

            // 如果父节点匹配，显示父节点；如果有子节点匹配，也显示父节点
            rootItem->setHidden(!text.isEmpty() && !rootMatch && !anyChildMatch);

            // 如果正在搜索，展开树以便查看结果
            if (!text.isEmpty()) rootItem->setExpanded(true);
        }
    }
    QStringList OutputControlWidget::getSelectSetListFromTreeItem(QTreeWidgetItem* item) {
        if (!item)return QStringList();
        auto pickers = m_pickers.lock();
        QString text = item->text(0);
        if (text.isEmpty() || pickers == nullptr || !pickers->contains(text))
            return {};
        auto picker = pickers->value(text);
        if (picker == nullptr) {
            return {};
        }
        auto PickSetList = picker->getAllData().selectedSetNames;
        QStringList SetList;
        for (auto setName : PickSetList) {
            SetList.append(setName);
        }
        return SetList;
    }
    QString OutputControlWidget::buildVariableStrFromTreeItem(QTreeWidgetItem* item) {
        if (!item)return QString();
        auto pickers = m_pickers.lock();
        QString text = item->text(0);
        if (text.isEmpty() || pickers == nullptr || !pickers->contains(text))
            return text;
        auto picker = pickers->value(text);
        if (picker == nullptr) {
            return text;
        }
        auto PickSetList = picker->getAllData().selectedSetNames;
        QStringList SetList;
        for (auto setName : PickSetList) {
            SetList.append(setName);
        }
        if (SetList.isEmpty())
            return text;
        return text + "(" + SetList.join(",") + ")";
    }
    // 收集所有选中的项显示在顶部
    void OutputControlWidget::updateTopLabel()
    {
        QStringList allSelected;

        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* root = m_treeWidget->topLevelItem(i);
            for (int j = 0; j < root->childCount(); ++j) {
                QTreeWidgetItem* child = root->child(j);
                if (child->checkState(0) == Qt::Checked) {
                    // 格式可以自己定，比如 "节点应变" 或者 "应变-节点应变"
                    //allSelected.append(child->text(0));
                    allSelected.append(buildVariableStrFromTreeItem(child));
                }
            }
        }

        if (allSelected.isEmpty()) {
            m_displayLabel->setText("无选中项");
            m_displayLabel->setStyleSheet("QLabel { border: 1px solid #ccc; background: #f9f9f9; padding: 4px; color: #999; }");
        }
        else {
            QString text = allSelected.join(", ");
            // 简单的截断处理，防止太长
            if (text.length() > 100) text = text.left(97) + "...";
            m_displayLabel->setText(text);
            m_displayLabel->setStyleSheet("QLabel { border: 1px solid #4caf50; background: #e8f5e9; padding: 4px; color: #000; }");
        }

        emit outputsChanged(getFinalOutputs());
    }

    QHash<QString, QHash<QString, QStringList>> OutputControlWidget::getFinalOutputs()
    {
        QHash<QString, QHash<QString, QStringList>> result;

        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* root = m_treeWidget->topLevelItem(i);
            QString category = root->text(0);
            QHash<QString, QStringList> list;

            for (int j = 0; j < root->childCount(); ++j) {
                auto child = root->child(j);
                if (child->checkState(0) == Qt::Checked) {
                    list[child->text(0)] = getSelectSetListFromTreeItem(child);
                }
            }

            if (!list.isEmpty()) {
                result[category] = list;
            }
        }
        return result;
    }

    // ============================================================
    // HistoryOutputControlWidget 实现
    // ============================================================
    HistoryOutputControlWidget::HistoryOutputControlWidget(
        QHash<QString, QStringList> structure,
        QHash<QString, QStringList> currentSelections,
        QHash<QString, QStringList> kdSelections,
        QWidget* parent)
        : QWidget(parent), m_structure(structure), m_selections(currentSelections), m_kdSelections(kdSelections),
        m_isProgrammaticChange(false),
        m_kdRootNode(nullptr), m_areaValueItem(nullptr)
    {
        m_sequence = QStringList{ "基本结果", "应变", "应力", "动刚度结果" };
        m_kdItems = QStringList{ "平均法", "指数法", "面积法" };

        initUI();
        initTreeStructure(); // 只在构造函数中创建一次树结构
        refreshTreeState();  // 只更新勾选状态
        updateTopLabel();
        updateKdAreaVisibility();
    }

    void HistoryOutputControlWidget::initUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(2, 2, 2, 2);
        mainLayout->setSpacing(8);

        // ---- 1. 顶部：显示已选项 + 复位按钮 ----
        QWidget* topContainer = new QWidget(this);
        QHBoxLayout* topLayout = new QHBoxLayout(topContainer);
        topLayout->setContentsMargins(0, 0, 0, 0);
        topLayout->setSpacing(5);

        m_displayLabel = new QLabel(this);
        m_displayLabel->setWordWrap(true);
        m_displayLabel->setStyleSheet("QLabel { border: 1px solid #ccc; background: #f9f9f9; padding: 4px; color: #333; }");
        m_displayLabel->setText("无选中项");
        m_displayLabel->setMinimumHeight(40);
        m_displayLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        topLayout->addWidget(m_displayLabel);

        QVBoxLayout* rightBtnLayout = new QVBoxLayout();
        rightBtnLayout->setContentsMargins(0, 0, 0, 0);
        rightBtnLayout->setSpacing(0);

        m_resetBtn = new QToolButton(this);
        m_resetBtn->setFixedSize(24, 24);
        m_resetBtn->setToolTip("恢复默认设置");
        m_resetBtn->setIcon(QIcon(":/mMainWidgets/Icon/post/reset.png"));
        connect(m_resetBtn, &QPushButton::clicked, this, &HistoryOutputControlWidget::onResetClicked);

        rightBtnLayout->addWidget(m_resetBtn);
        rightBtnLayout->addStretch();
        topLayout->addLayout(rightBtnLayout);

        mainLayout->addWidget(topContainer);

        // ---- 2. 上方：三个下拉框（文件格式、数据类型、输出集合） ----
        m_comboFileType = new QComboBox(this);
        m_comboFileType->addItems({ "csv", "mxh" });

        m_comboDataType = new QComboBox(this);
        m_comboDataType->addItems({ "无", "实数", "复数", "幅值", "相位", "全部" });

        m_comboSet = new QComboBox(this);

        // 使用QFormLayout布局让下拉框占满宽度
        QFormLayout* formLayout = new QFormLayout();
        formLayout->setContentsMargins(0, 0, 0, 0);
        formLayout->setSpacing(8);
        formLayout->addRow("文件格式", m_comboFileType);
        formLayout->addRow("数据类型", m_comboDataType);
        formLayout->addRow("输出集合", m_comboSet);
        mainLayout->addLayout(formLayout);

        // ---- 3. 中间：过滤输入框 ----
        m_filterEdit = new QLineEdit(this);
        m_filterEdit->setPlaceholderText("搜索变量名称...");
        m_filterEdit->setClearButtonEnabled(true);
        connect(m_filterEdit, &QLineEdit::textChanged, this, &HistoryOutputControlWidget::onFilterTextChanged);
        mainLayout->addWidget(m_filterEdit);

        // ---- 4. 下方：QFrame 框住的 QTreeWidget（所有结果在一棵树中） ----
        QFrame* treeFrame = new QFrame(this);
        treeFrame->setFrameShape(QFrame::StyledPanel);
        QVBoxLayout* frameLayout = new QVBoxLayout(treeFrame);
        frameLayout->setContentsMargins(0, 0, 0, 0);

        m_treeWidget = new QTreeWidget(treeFrame);
        m_treeWidget->setStyleSheet(
            "QTreeWidget::item{height:20px;}"
            "QTreeView::indicator:unchecked { image: url(:/mMainWidgets/Icon/uncheck3x.png); }"
            "QTreeView::indicator:indeterminate { image: url(:/mMainWidgets/Icon/partcheck3x.png);}"
            "QTreeView::indicator:checked { image: url(:/mMainWidgets/Icon/check3x.png); }"
        );
        m_treeWidget->setHeaderHidden(true);
        m_treeWidget->setColumnCount(1);
        connect(m_treeWidget, &QTreeWidget::itemChanged, this, &HistoryOutputControlWidget::onItemChanged);

        frameLayout->addWidget(m_treeWidget);
        mainLayout->addWidget(treeFrame);

        // 动刚度值输入（在面积法子项下显示，通过独立子项实现）
        m_lblDynamicStiff = new QLabel("动刚度值");
        m_dynamicStiffLineEdit = new QLineEdit(this);
        m_dynamicStiffLineEdit->setPlaceholderText("请输入目标动刚度值");
        connect(m_dynamicStiffLineEdit, &QLineEdit::textChanged, this, [=](const QString& text) {
            emit dynamicStiffChanged(text);
            });

        m_kdAreaContainer = new QWidget(m_treeWidget);
        QHBoxLayout* kdLayout = new QHBoxLayout(m_kdAreaContainer);
        kdLayout->setContentsMargins(20, 0, 0, 0);
        kdLayout->setSpacing(5);
        kdLayout->addWidget(m_lblDynamicStiff);
        kdLayout->addWidget(m_dynamicStiffLineEdit);
        kdLayout->addStretch();
        m_kdAreaContainer->hide();
    }

    void HistoryOutputControlWidget::initTreeStructure()
    {
        m_isProgrammaticChange = true;
        m_treeWidget->clear();
        m_categoryNodes.clear();
        m_itemNodes.clear();
        m_kdRootNode = nullptr;
        m_kdItemNodes.clear();
        m_areaValueItem = nullptr;

        for (auto category : m_sequence)
        {
            if (category == "动刚度结果") {
                // 动刚度结果特殊处理
                m_kdRootNode = new QTreeWidgetItem(m_treeWidget);
                m_kdRootNode->setText(0, category);
                m_kdRootNode->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                m_kdRootNode->setCheckState(0, Qt::Unchecked);
                m_categoryNodes[category] = m_kdRootNode;

                for (const QString& itemName : m_kdItems) {
                    QTreeWidgetItem* childItem = new QTreeWidgetItem(m_kdRootNode);
                    childItem->setText(0, itemName);
                    childItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                    childItem->setCheckState(0, Qt::Unchecked);
                    m_kdItemNodes[itemName] = childItem;
                }

                // 面积法值输入子项（紧跟面积法后面，勾选时显示）
                m_areaValueItem = new QTreeWidgetItem(m_kdRootNode);
                m_areaValueItem->setFlags(Qt::ItemIsEnabled);
                m_areaValueItem->setText(0, "");
                m_treeWidget->setItemWidget(m_areaValueItem, 0, m_kdAreaContainer);
                m_areaValueItem->setHidden(true);
                // 将输入框节点添加到动刚度根节点下
                m_kdRootNode->addChild(m_areaValueItem);

                continue;
            }

            if (!m_structure.contains(category))
                continue;

            QStringList items = m_structure.value(category);

            QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
            rootItem->setText(0, category);
            rootItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            rootItem->setCheckState(0, Qt::Unchecked);
            m_categoryNodes[category] = rootItem;

            QHash<QString, QTreeWidgetItem*> itemMap;
            for (const QString& itemName : items) {
                QTreeWidgetItem* childItem = new QTreeWidgetItem(rootItem);
                childItem->setText(0, itemName);
                childItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                childItem->setCheckState(0, Qt::Unchecked);
                itemMap[itemName] = childItem;
            }
            m_itemNodes[category] = itemMap;
        }

        m_treeWidget->collapseAll();
        m_isProgrammaticChange = false;
    }

    void HistoryOutputControlWidget::setDefaultOutputs(const QHash<QString, QStringList>& defaults)
    {
        m_defaults = defaults;
    }

    void HistoryOutputControlWidget::setKdDefaultOutputs(const QStringList& defaults)
    {
        if (!defaults.isEmpty()) { // 只有当defaults非空时才更新
            m_kdDefaults.clear();
            m_kdDefaults["动刚度结果"] = defaults;
            refreshTreeState();
            updateKdAreaVisibility();
        }
    }

    void HistoryOutputControlWidget::onResetClicked()
    {
        // 清空所有普通输出
        m_selections.clear();

        // 清空所有动刚度输出
        m_kdSelections.clear();

        // 刷新UI
        refreshTreeState();
        updateKdAreaVisibility();
        updateTopLabel();

        emit outputsChanged(getFinalOutputs());
        emit kdOutputsChanged(getKdOutputs());
    }

    void HistoryOutputControlWidget::onItemChanged(QTreeWidgetItem* item, int column)
    {
        if (m_isProgrammaticChange)
            return;

        m_isProgrammaticChange = true;

        if (item->childCount() > 0) {
            updateChildrenState(item);
        }
        else {
            if (item->parent()) {
                updateParentState(item->parent());
            }
        }
        updateTopLabel();

        m_isProgrammaticChange = false;

        // 处理动刚度结果勾选变化（点击根节点或子项都要更新）
        if (item == m_kdRootNode || (item->parent() && item->parent() == m_kdRootNode)) {
            updateKdAreaVisibility();
            emit kdOutputsChanged(getKdOutputs());
        }

        emit outputsChanged(getFinalOutputs());
    }

    void HistoryOutputControlWidget::updateKdAreaVisibility()
    {
        // 使用缓存的节点指针，避免每次遍历查找
        if (m_kdItemNodes.contains("面积法") && m_areaValueItem) {
            QTreeWidgetItem* areaItem = m_kdItemNodes["面积法"];
            if (areaItem->checkState(0) == Qt::Checked) {
                m_areaValueItem->setHidden(false);
                m_kdAreaContainer->show();
            }
            else {
                m_areaValueItem->setHidden(true);
                m_kdAreaContainer->hide();
            }
        }
    }

    void HistoryOutputControlWidget::refreshTreeState()
    {
        m_isProgrammaticChange = true;

        // 更新普通类别和子项的勾选状态
        for (const QString& category : m_categoryNodes.keys()) {
            if (category == "动刚度结果")
                continue;

            QTreeWidgetItem* rootItem = m_categoryNodes[category];
            if (!rootItem)
                continue;

            QStringList selectedItemsInCat;
            if (m_selections.contains(category)) {
                selectedItemsInCat = m_selections[category];
            }

            if (m_itemNodes.contains(category)) {
                QHash<QString, QTreeWidgetItem*>& itemMap = m_itemNodes[category];
                for (const QString& itemName : itemMap.keys()) {
                    QTreeWidgetItem* childItem = itemMap[itemName];
                    if (selectedItemsInCat.contains(itemName)) {
                        childItem->setCheckState(0, Qt::Checked);
                    }
                    else {
                        childItem->setCheckState(0, Qt::Unchecked);
                    }
                }
            }

            updateParentState(rootItem);
        }

        // 更新动刚度结果的勾选状态
        if (m_kdRootNode) {
            QStringList selectedKdItems;
            if (m_kdSelections.contains("动刚度结果")) {
                selectedKdItems = m_kdSelections["动刚度结果"];
            }

            for (const QString& itemName : m_kdItemNodes.keys()) {
                QTreeWidgetItem* childItem = m_kdItemNodes[itemName];
                if (selectedKdItems.contains(itemName)) {
                    childItem->setCheckState(0, Qt::Checked);
                }
                else {
                    childItem->setCheckState(0, Qt::Unchecked);
                }
            }

            updateParentState(m_kdRootNode);
        }

        updateTopLabel();
        m_isProgrammaticChange = false;
    }

    void HistoryOutputControlWidget::updateChildrenState(QTreeWidgetItem* parentItem)
    {
        Qt::CheckState state = parentItem->checkState(0);
        if (state == Qt::PartiallyChecked) state = Qt::Checked;
        for (int i = 0; i < parentItem->childCount(); ++i) {
            QTreeWidgetItem* child = parentItem->child(i);
            // 跳过面积法输入框节点，它不应该被勾选
            if (child == m_areaValueItem)
                continue;
            child->setCheckState(0, state);
        }
    }

    void HistoryOutputControlWidget::updateParentState(QTreeWidgetItem* parentItem)
    {
        int checkedCount = 0;
        int totalCheckableCount = 0;
        for (int i = 0; i < parentItem->childCount(); ++i) {
            QTreeWidgetItem* child = parentItem->child(i);
            // 跳过面积法输入框节点，它不参与父节点状态计算
            if (child == m_areaValueItem)
                continue;
            totalCheckableCount++;
            if (child->checkState(0) == Qt::Checked) {
                checkedCount++;
            }
        }
        if (totalCheckableCount == 0) {
            parentItem->setCheckState(0, Qt::Unchecked);
        }
        else if (checkedCount == 0) {
            parentItem->setCheckState(0, Qt::Unchecked);
        }
        else if (checkedCount == totalCheckableCount) {
            parentItem->setCheckState(0, Qt::Checked);
        }
        else {
            parentItem->setCheckState(0, Qt::PartiallyChecked);
        }
    }

    void HistoryOutputControlWidget::onFilterTextChanged(const QString& text)
    {
        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* rootItem = m_treeWidget->topLevelItem(i);
            bool rootMatch = rootItem->text(0).contains(text, Qt::CaseInsensitive);
            bool anyChildMatch = false;

            for (int j = 0; j < rootItem->childCount(); ++j) {
                QTreeWidgetItem* child = rootItem->child(j);
                bool childMatch = child->text(0).contains(text, Qt::CaseInsensitive);
                child->setHidden(!text.isEmpty() && !childMatch && !rootMatch);
                if (childMatch) anyChildMatch = true;
            }
            rootItem->setHidden(!text.isEmpty() && !rootMatch && !anyChildMatch);
            if (!text.isEmpty()) rootItem->setExpanded(true);
        }
    }

    void HistoryOutputControlWidget::updateTopLabel()
    {
        QStringList allSelected;
        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* root = m_treeWidget->topLevelItem(i);
            for (int j = 0; j < root->childCount(); ++j) {
                QTreeWidgetItem* child = root->child(j);
                // 跳过面积法输入框节点
                if (child == m_areaValueItem)
                    continue;
                if (child->checkState(0) == Qt::Checked) {
                    allSelected.append(child->text(0));
                }
            }
        }
        if (allSelected.isEmpty()) {
            m_displayLabel->setText("无选中项");
            m_displayLabel->setStyleSheet("QLabel { border: 1px solid #ccc; background: #f9f9f9; padding: 4px; color: #999; }");
        }
        else {
            QString text = allSelected.join(", ");
            if (text.length() > 100) text = text.left(97) + "...";
            m_displayLabel->setText(text);
            m_displayLabel->setStyleSheet("QLabel { border: 1px solid #4caf50; background: #e8f5e9; padding: 4px; color: #000; }");
        }
    }

    QHash<QString, QStringList> HistoryOutputControlWidget::getFinalOutputs() const
    {
        QHash<QString, QStringList> result;
        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* root = m_treeWidget->topLevelItem(i);
            QString category = root->text(0);
            if (category == "动刚度结果")
                continue; // 动刚度结果单独处理
            QStringList list;
            for (int j = 0; j < root->childCount(); ++j) {
                QTreeWidgetItem* child = root->child(j);
                // 跳过面积法输入框节点
                if (child == m_areaValueItem)
                    continue;
                if (child->checkState(0) == Qt::Checked) {
                    list.append(child->text(0));
                }
            }
            if (!list.isEmpty()) {
                result[category] = list;
            }
        }
        return result;
    }

    QStringList HistoryOutputControlWidget::getKdOutputs() const
    {
        QStringList result;
        if (m_kdRootNode) {
            for (int j = 0; j < m_kdRootNode->childCount(); ++j) {
                QTreeWidgetItem* child = m_kdRootNode->child(j);
                // 跳过面积法输入框节点
                if (child == m_areaValueItem)
                    continue;
                if (child->checkState(0) == Qt::Checked) {
                    result.append(child->text(0));
                }
            }
        }
        return result;
    }

    QString HistoryOutputControlWidget::getDynamicStiff() const
    {
        return m_dynamicStiffLineEdit->text();
    }

    void HistoryOutputControlWidget::setDynamicStiff(const QString& val)
    {
        m_dynamicStiffLineEdit->setText(val);
    }

    // ============================================================
    // mCaseSettingPanel 实现
    // ============================================================
    mCaseSettingPanel::mCaseSettingPanel(QWidget* parent, QString type)
        : mPanelBase(parent), controlWidget(nullptr), pchControlWidget(nullptr)
    {
        _mainLayout = new QVBoxLayout(this);

        binder = new DataBinder;
        uiCtrl = new UiController;

        _preloadCombo = nullptr;
        _functionCombo = nullptr;
        _genericParamCombo = nullptr;

        m_pickers = make_shared<QHash<QString, mPickWidgetController*>>();
        _pch_pickers = make_shared<QHash<QString, mPickWidgetController*>>();

        _defineFold = new FoldSection("工况定义", this);
        _defineFold->addRow(new QLabel("工况类型:"), new QLabel(type));
    }

	mCaseSettingPanel::~mCaseSettingPanel()
	{
		//显式按序释放拾取控件（同步 delete），先于成员析构与 Qt 父子级联执行：
		//控制器/视图先行销毁、哈希清空后，壳的析构回调因视图已空自动跳过，
		//不再依赖子控件级联顺序与 QPointer 守卫时机
		releaseOutputPickers(false, true);
		releaseOutputPickers(true, true);

		if (_mainLayout!=nullptr)
		{
			delete _mainLayout;
			_mainLayout = nullptr;
		}
        if (binder != nullptr)
        {
            delete binder;
            binder = nullptr;
        }
        if (uiCtrl != nullptr)
        {
            delete uiCtrl;
            uiCtrl = nullptr;
        }
        //if (_currentViewportModel != nullptr)
        //{
        //    _currentViewportModel = nullptr;
        //}
        if (_caseData != nullptr)
        {
            _caseData = nullptr;
        }
	}
    int mCaseSettingPanel::getCaseID()
    {
        if (_caseData)
            return _caseData->getCaseId();
        return 1;
    }

    const MechCaseData::mCaseBase* mCaseSettingPanel::getCaseData() {
        return this->_caseData;
    }

    void mCaseSettingPanel::newCasePanel(int caseID)
    {
        auto caseData = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
        if (caseData == nullptr)
        {
            return;
        }
        _caseData = caseData;
        setEditing(true);
        updatePanel();
    }
    void mCaseSettingPanel::uneditCasePanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        _caseData = nullptr;
    }

    void mCaseSettingPanel::updateMapRelationData()
    {
        updatePreloadCombobox(_preloadCombo);
        updateFunctionCombobox(_functionCombo);
        updateGenericParamCombobox(_genericParamCombo);
        updateSetCombobox(nullptr);
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, false));
    }
    void mCaseSettingPanel::newCasePanel()
    {
        setEditing(true);
    }
    void mCaseSettingPanel::updatePanel()
    {

    }

    void mCaseSettingPanel::updatePreloadCombobox(QComboBox* combobox, QList<CaseType> caseTypes)
    {
        if (_caseTypes.empty())
        {
            _caseTypes = caseTypes;
        }

        if (_caseData == nullptr)
        {
            return;
        }
        if (combobox == nullptr)
        {
            return;
        }
        if (_preloadCombo != nullptr)
        {
            disconnect(_preloadCombo, &QComboBox::currentTextChanged, this, nullptr);
            connect(_preloadCombo, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                if (getCVM == nullptr)
                {
                    return;
                }
                if (_caseData == nullptr)
                {
                    return;
                }
                auto& caseManager = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);
                auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
                int caseId = caseManager.getIDByCaseName(text);
                int id = _caseData->getCaseId();
                if (caseId == -1)
                {
                    _preloadCombo->setCurrentIndex(0);
                    mapRelation.appendMapRelation({ Physical_Case, id }, { Physical_Case, caseId });
                    return;
                }
                _preloadCombo->setCurrentText(text);
                //设置id
                mapRelation.appendMapRelation({ Physical_Case, id }, { Physical_Case, caseId });
                });
        }
        _preloadCombo = combobox;

        auto& caseManager = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int id = mapRelation.getMapRelation<int>({ Physical_Case, _caseData->getCaseId()}, Physical_Case);
        QString currentText = caseManager.getCaseNameByID(id);
        combobox->blockSignals(true);
        combobox->clear(); 
        combobox->addItem("");
        for (auto caseType : _caseTypes)
        {
            combobox->addItems(getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getAllCaseNamesByTypes(caseType));
        }

        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        }
        else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(0); // 默认不选
        }
        combobox->blockSignals(false);
    }
    void mCaseSettingPanel::updateFunctionCombobox(QComboBox* combobox)
    {
        if (_caseData == nullptr)
        {
            return;
        }
        if (combobox == nullptr)
        {
            return;
        }
        _functionCombo = combobox;
        //QString currentText = combobox->currentText();
        combobox->blockSignals(true);
        combobox->clear(); 
        combobox->addItem("");
        auto& functionManager = getCVM->getModelData()->getManager<mFunctionManager>(Physical_Function);
        combobox->addItems(functionManager.getAllFunctionNames());


        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int id = mapRelation.getMapRelation<int>({ Physical_Case, _caseData->getCaseId() }, Physical_Function);
        QString currentText = functionManager.getFunctionNameByID(id);
        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        }
        else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(0); // 默认不选
        }
        combobox->blockSignals(false);
    }
    void mCaseSettingPanel::updateGenericParamCombobox(QComboBox* combobox)
    {
        if (_caseData == nullptr)
        {
            return;
        }
        if (combobox == nullptr)
        {
            return;
        }
        _genericParamCombo = combobox;
        //QString currentText = combobox->currentText();
        combobox->blockSignals(true);
        combobox->clear();
        combobox->addItem("");
        QStringList partList = getCVM->getModelData()->getAllNames(Physical_GenericParam);
        combobox->addItems(partList);


        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int id = mapRelation.getMapRelation<int>({ Physical_Case, _caseData->getCaseId() }, Physical_GenericParam);
        QString currentText = getCVM->getModelData()->getNameByPhyID(Physical_GenericParam, id);
        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        }
        else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(0); // 默认不选
        }
        combobox->blockSignals(false);
    }

    void mCaseSettingPanel::initPchOutputSection(QHash<QString, QStringList> defaultData)
    {
        static QHash<QString, QStringList> pchOutputCheckboxs = {
            {"基本结果", QStringList{"位移","速度","加速度"}},
            {"声学", QStringList{"声压(帕斯卡)"}},
        };
        _outputPchFoldWidget = new FoldSection("PCH输出控制", this);
        _outputPchFoldWidget->setCheckable(true);
        QHash<QString, QStringList> outputs;
        if (pchControlWidget != nullptr) {
            pchControlWidget->deleteLater();
            pchControlWidget = nullptr;
        }
        pchControlWidget = new OutputControlWidget(pchOutputCheckboxs, outputs, _pch_pickers, this);
        //守卫：删除工况时 panel 被同步析构，其成员先于子控件级联销毁；panel 一进入 ~QObject 守卫即置空，
        //避免壳的析构回调访问已析构的成员
        QPointer<mCaseSettingPanel> pchSelf = this;
        pchControlWidget->setPickerMounter([pchSelf](const QString& var, QWidget* host, bool checked) { if (pchSelf) pchSelf->mountOutputPicker(var, true, host, checked); });
        pchControlWidget->setPickerUnmounter([pchSelf](const QString& var, const QString& nextVar) { if (pchSelf) pchSelf->unmountOutputPicker(var, true, nextVar); });
        buildPchPickWidgets(pchOutputCheckboxs);
        pchControlWidget->setDefaultOutputs(defaultData);

        connect(pchControlWidget, &OutputControlWidget::outputsChanged, this, &mCaseSettingPanel::onPchOutputChanged);
        connect(pchControlWidget, &OutputControlWidget::firstNodeChecked, this, [this](const QString& name) {
            auto picker = ensureOutputPicker(name, true);
            if (picker == nullptr || picker->widget() == nullptr)return;
            picker->setPickActive(true);
            });

        _outputPchFoldWidget->addRow(pchControlWidget);
    }

    void mCaseSettingPanel::onPchOutputChanged(const QHash<QString, QHash<QString, QStringList>>& newData) {
        // 这里是实时更新数据的地方
        if (_caseData == nullptr)return;
        QHash<QString, QStringList> outputData;
        for (auto key : newData.keys()) {
            auto data = newData.value(key);
            outputData[key] = data.keys();
        }
        _caseData->setPchOutputs(outputData);
        _caseData->setPchSelectedSet(newData);
    }

    void mCaseSettingPanel::onOutputChanged(const QHash<QString, QHash<QString, QStringList>>& newData) {
        // 这里是实时更新数据的地方
        if (_caseData == nullptr)return;
        QHash<QString, QStringList> outputData;
        for (auto key : newData.keys()) {
            auto data = newData.value(key);
            outputData[key] = data.keys();
        }
        _caseData->setOutputs(outputData);
        _caseData->setSelectedSet(newData);
    }

    void mCaseSettingPanel::initOutputSection(QHash<QString, QStringList> defaultData)
    {
        _outputFoldWidget = new FoldSection("云图输出控制", this);
        
        QHash<QString, QStringList> outputs/* = _caseData->getOutputs()*/;//这是数据获取
        //_caseData->setOutputs(outputs);//这是数据存储
        //QHash<QString, QStringList> _outputCheckboxs;//这是要转换为uiCheckbox和树的，输出控制的对（键为根节点，值为子节点），只有两层，第一层复选框有三个状态（根据子节点的状态变化），第二层复选框有两个状态
        //_outputCheckboxs["应变"] = QStringList{ "节点应变", "单元应变" };
        //_outputCheckboxs["应力"] = QStringList{ "节点应力", "单元应力" };
        //_outputCheckboxs["位移"] = QStringList{ "节点位移", "单元位移" };
        //最上方是显示已选中的变量名称outputs的所有值，第二个是一行控件（输入框过滤），最下方是一个QFrame框住一个树，
        //_outputFoldWidget->addRow(new QLayout());
        if (controlWidget != nullptr) {
            controlWidget->deleteLater();
            controlWidget = nullptr;
        }
        controlWidget = new OutputControlWidget(_outputCheckboxs, outputs, m_pickers, this);
        //守卫：删除工况时 panel 被同步析构，其成员先于子控件级联销毁；panel 一进入 ~QObject 守卫即置空，
        //避免壳的析构回调访问已析构的成员
        QPointer<mCaseSettingPanel> self = this;
        controlWidget->setPickerMounter([self](const QString& var, QWidget* host, bool checked) { if (self) self->mountOutputPicker(var, false, host, checked); });
        controlWidget->setPickerUnmounter([self](const QString& var, const QString& nextVar) { if (self) self->unmountOutputPicker(var, false, nextVar); });
        auto _outTypeLable = new QLabel(this);
        auto _outTypeCombo = new QComboBox(this);
        _outTypeLable->setText("输出文件类型");
        _outTypeCombo->addItems({ "mxdb","op2","h5" });
        _outTypeCombo->setCurrentIndex(0);
        QString type = _caseData->getOutputType();
        type = type.isEmpty() ? "mxdb" : type;
        _outTypeCombo->setCurrentText(type);
        slot_outputTypeChanged(type);
        controlWidget->setDefaultOutputs(defaultData);

        connect(controlWidget, &OutputControlWidget::outputsChanged, this, &mCaseSettingPanel::onOutputChanged);
        connect(_outTypeCombo, &QComboBox::currentTextChanged, this, &mCaseSettingPanel::slot_outputTypeChanged);

        _outputFoldWidget->addRow(_outTypeLable, _outTypeCombo);
        _outputFoldWidget->addRow(controlWidget);
    }
    
    void mCaseSettingPanel::slot_outputTypeChanged(const QString& type) {
        if (!_typeOfoutputCheckBoxs.contains(type))
            return;
        _outputCheckboxs = _typeOfoutputCheckBoxs.value(type);
        QHash<QString, QStringList> type_defaultData = {};
        if (_typeOfDefault.contains(type)) {
            type_defaultData = _typeOfDefault.value(type);
        }
        if (_caseData != nullptr)
            _caseData->setOutputType(type);
        if (controlWidget != nullptr)
            buildPickWidgets(_outputCheckboxs);
            controlWidget->resetTreeData(_outputCheckboxs, type_defaultData);
    }

    void mCaseSettingPanel::initHistoryOutputSection(QHash<QString, QStringList> defaultData, QHash<QString, QStringList> defaultKdData, bool defaultOpen)
    {
        _historyOutputFoldWidget = new FoldSection("历史输出设置", this, QColor("#909399"), defaultOpen);
        
        // 绑定折叠框勾选状态到数据类
        if (_caseData)
        {
            _historyOutputFoldWidget->setCheckable(true);
            _historyOutputFoldWidget->setChecked(_caseData->getHistoryOutputEnabled());
            _historyOutputFoldWidget->onCheckBoxToggled(_caseData->getHistoryOutputEnabled());
            connect(_historyOutputFoldWidget, &FoldWidget::enabledChanged, this, [=](bool checked) {
                if (_caseData) {
                    _caseData->setHistoryOutputEnabled(checked);
                }
                });
        }

        // 从数据类读取已保存的历史输出数据
        QHash<QString, QStringList> outputs;
        QHash<QString, QStringList> kdOutputs;
        if (_caseData)
        {
            outputs = _caseData->getHistoryOutputs();
            kdOutputs["动刚度结果"] = _caseData->getHistoryKdOutputs();
        }

        _historyOutputControlWidget = new HistoryOutputControlWidget(_historyOutputCheckboxs, outputs, kdOutputs, this);
        _historyOutputControlWidget->setDefaultOutputs(defaultData);
        _historyOutputControlWidget->setKdDefaultOutputs(defaultKdData.value("动刚度结果"));

        // 树形选择变更保存到数据类
        connect(_historyOutputControlWidget, &HistoryOutputControlWidget::outputsChanged, this, [=](const QHash<QString, QStringList>& newData) {
            if (_caseData) {
                _caseData->setHistoryOutputs(newData);
            }
            });
        connect(_historyOutputControlWidget, &HistoryOutputControlWidget::kdOutputsChanged, this, [=](const QStringList& newKdOutputs) {
            if (_caseData) {
                _caseData->setHistoryKdOutputs(newKdOutputs);
            }
            });

        _historyOutputFoldWidget->addRow(_historyOutputControlWidget);
    }

    void mCaseSettingPanel::updateSetCombobox(QComboBox* combobox, QList<SetType> setTypes, MapRelationType type)
    {
        if (_caseData == nullptr)
        {
            return;
        }
        if (combobox != nullptr)
        {
            _setCombos.insert(combobox, { setTypes, type });
            //QString currentText = combobox->currentText();
            combobox->blockSignals(true);
            combobox->clear();
            combobox->addItem("");
            QStringList names;
            auto& setManager = getCVM->getModelData()->getManager<mSetManager>(Physical_Set);
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            if (setTypes.isEmpty())
            {
                names = setManager.getAllSetNames();
            }
            else
            {
                for (auto setType : setTypes)
                {
                    names.append(setManager.getAllSetNamesByTypes(setType));
                }
            }
            combobox->addItems(names);

            int id = mapRelation.getMapRelation<int>({ Physical_Case, _caseData->getCaseId() }, Physical_Set, type);
            QString currentText = setManager.getSetNameByID(id);
            int idx = combobox->findText(currentText);
            if (idx >= 0) {
                combobox->setCurrentIndex(idx);
            }
            else if (combobox->model()->rowCount() > 0) {
                combobox->setCurrentIndex(0);
            }
            connect(combobox, &QComboBox::currentTextChanged, this, [=](const QString& text) {
               updateSetMapRelation(combobox, type);
                });
            combobox->blockSignals(false);

        }
        else
        {
            auto setNames = getCVM->getModelData()->getManager<mSetManager>(Physical_Set).getAllSetNames();
            QHashIterator<QComboBox*, QPair<QList<SetType>, MapRelationType>> iter(_setCombos);
            while (iter.hasNext())
            {
                iter.next();

                auto tempCombo = iter.key();
                auto setTypes = iter.value().first;
                tempCombo->blockSignals(true);
                tempCombo->clear();
                tempCombo->addItem("");
                QStringList names;
                auto& setManager = getCVM->getModelData()->getManager<mSetManager>(Physical_Set);
                auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
                if (setTypes.isEmpty())
                {
                    names = getCVM->getModelData()->getManager<mSetManager>(Physical_Set).getAllSetNames();
                }
                else
                {
                    for (auto setType : setTypes)
                    {
                        names.append(getCVM->getModelData()->getManager<mSetManager>(Physical_Set).getAllSetNamesByTypes(setType));
                    }
                }
                tempCombo->addItems(names);
                int id = mapRelation.getMapRelation<int>({ Physical_Case, _caseData->getCaseId() }, Physical_Set, iter.value().second);
                QString currentText = setManager.getSetNameByID(id);
                int idx = tempCombo->findText(currentText);
                if (idx >= 0) {
                    tempCombo->setCurrentIndex(idx);
                }
                else if (tempCombo->model()->rowCount() > 0) {
                    tempCombo->setCurrentIndex(0);
                }
                tempCombo->blockSignals(false);
            }
        }
       
    }

    void mCaseSettingPanel::updateSetMapRelation(QComboBox* combobox, MapRelationType type)
    {
        if (combobox == nullptr)
        {
            return;
        }
        if (getCVM == nullptr)
        {
            return;
        }
        if (_caseData == nullptr)
        {
            return;
        }
        auto& setManager = getCVM->getModelData()->getManager<mSetManager>(Physical_Set);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int setId = setManager.getIDBySetName(combobox->currentText());
        //设置id
        int id = _caseData->getCaseId();
        mapRelation.appendMapRelation({ Physical_Case, id }, { Physical_Set, setId }, type);
    }

    void mCaseSettingPanel::createNodePickController(mPickWidgetController*& picker, mColorBlock colorBlock, const QString& tip, const QStringList& selected) {
        picker = mGeneralPickUp::createNoCommitPickWidget(colorBlock, this);
        picker->setDefultTip(tip);
        
        QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoPoint,SetType::Node });
        picker->setSelectSetList(setToType);

        PickWidgetData pickerData;
        for (auto set : selected) {
            if (setToType.contains(set))
                pickerData.selectedSetNames.insert(set);
        }

        picker->displayData(pickerData);

    }
    void mCaseSettingPanel::buildPickWidgets(const QHash<QString, QStringList>& defaultData) {
        if (defaultData.isEmpty() || m_pickers == nullptr || _caseData == nullptr)return;

        //释放旧控件（内部先失活已激活部件，防止切换后点击其他部件访问已销毁对象），再按新类型重新登记
        releaseOutputPickers(false);

        for (auto category : defaultData.keys()) {
            for (auto var : defaultData.value(category)) {
                auto type = OutputControlWidget::getVariableType(category, var);
                if (type == MechMainWindow::OutputControlWidget::None) {
                    continue;
                }
                m_pickerVarTypes.insert(var, type);
                m_pickers->insert(var, nullptr);
            }
        }
    }

    void mCaseSettingPanel::buildPchPickWidgets(const QHash<QString, QStringList>& defaultData) {
        if (defaultData.isEmpty() || _pch_pickers == nullptr || _caseData == nullptr)return;

        //释放旧控件（内部先失活已激活部件，防止切换后点击其他部件访问已销毁对象），再按新类型重新登记
        releaseOutputPickers(true);

        for (auto category : defaultData.keys()) {
            for (auto var : defaultData.value(category)) {
                auto type = OutputControlWidget::getVariableType(category, var);
                if (type == MechMainWindow::OutputControlWidget::None) {
                    continue;
                }
                _pch_pickerVarTypes.insert(var, type);
                _pch_pickers->insert(var, nullptr);
            }
        }
    }

    mPickWidgetController* mCaseSettingPanel::createOutputPickerOnDemand(const QString& var, bool isPch) {
        if (_caseData == nullptr)return nullptr;
        auto& types = isPch ? _pch_pickerVarTypes : m_pickerVarTypes;
        if (!types.contains(var))return nullptr;
        auto type = types.value(var);

        //从工况数据取该变量已保存的选中集合
        QStringList selected;
        auto selectedMap = isPch ? _caseData->getPchSelectedSet() : _caseData->getSelectedSet();
        for (auto it = selectedMap.begin(); it != selectedMap.end(); ++it) {
            if (it.value().contains(var)) {
                selected = it.value().value(var);
                break;
            }
        }

        mPickWidgetController* picker = nullptr;
        if (type == MechMainWindow::OutputControlWidget::NodeSet)
            createNodePickController(picker, CBNone, "全模型", selected);
        else if (type == MechMainWindow::OutputControlWidget::ElementSet)
            createCellPickController(picker, CBNone, "全模型", selected);
        if (picker == nullptr || picker->widget() == nullptr)return nullptr;

        auto& pickers = isPch ? _pch_pickers : m_pickers;
        pickers->insert(var, picker);

        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::createUpdateSetSig, this, [this, var, type, isPch](bool isCreate, int setID) {
            auto picker = isPch ? _pch_pickers->value(var) : m_pickers->value(var);
            if (picker) {
                QHash<QString, QString> setToType;
                if (type == MechMainWindow::OutputControlWidget::NodeSet)
                    setToType = getSetNamesList({ SetType::GeoPoint,SetType::Node });
                else if (type == MechMainWindow::OutputControlWidget::ElementSet)
                    setToType = getSetNamesList({ SetType::GeoFace,SetType::Element, SetType::ElementSurface });
                picker->setSelectSetList(setToType);
            }
            });
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::importSuccessful, this, [this, var, type, isPch]() {
            auto picker = isPch ? _pch_pickers->value(var) : m_pickers->value(var);
            if (picker) {
                QHash<QString, QString> setToType;
                if (type == MechMainWindow::OutputControlWidget::NodeSet)
                    setToType = getSetNamesList({ SetType::GeoPoint,SetType::Node });
                else if (type == MechMainWindow::OutputControlWidget::ElementSet)
                    setToType = getSetNamesList({ SetType::GeoFace,SetType::Element, SetType::ElementSurface });
                picker->setSelectSetList(setToType);
            }
            });
        connect(picker, &mPickWidgetController::dataChanged, this, [this, isPch]() {
            if (isPch) {
                if (pchControlWidget)
                    pchControlWidget->updateTopLabel();
            }
            else {
                if (controlWidget)
                    controlWidget->updateTopLabel();
            }
            });
        connect(picker, &mPickWidgetController::pickActiveChanged, this, [this, var, isPch](bool isActive) {
            auto picker = isPch ? _pch_pickers->value(var) : m_pickers->value(var);
            if (picker == nullptr)return;
            if (isActive) {
                //记录最近激活的变量：进入面板（showEvent）时按它恢复激活态
                if (isPch) m_lastActivePchVar = var;
                else m_lastActiveVar = var;
                picker->setAllPickTypesDisabled(isActive);
            }
            });
        return picker;
    }

    // 拾取控件挂载/卸载/释放：视图父子关系的改动全部集中在此，
    // 输出控制树上的挂载壳可被 Qt 随意销毁，视图始终归面板所有
    QWidget* mCaseSettingPanel::ensurePickerPark()
    {
        if (m_pickerParkWidget == nullptr) {
            m_pickerParkWidget = new QWidget(this);
            m_pickerParkWidget->hide();
        }
        return m_pickerParkWidget;
    }

    mPickWidgetController* mCaseSettingPanel::ensureOutputPicker(const QString& var, bool isPch)
    {
        auto& pickers = isPch ? _pch_pickers : m_pickers;
        if (pickers == nullptr)return nullptr;
        auto it = pickers->constFind(var);
        if (it != pickers->constEnd() && it.value() && it.value()->widget())
            return it.value();                    //已存在且视图有效，直接复用
        //首次挂载，或视图已失效（_view 为 QPointer，销毁后自动置空）：创建/重建并覆盖哈希中的旧条目
        return createOutputPickerOnDemand(var, isPch);
    }

    void mCaseSettingPanel::mountOutputPicker(const QString& var, bool isPch, QWidget* host, bool checked)
    {
        ensurePickerPark();                                 //首次挂载即建寄存容器，保证卸载路径永远有去处
        auto picker = ensureOutputPicker(var, isPch);
        if (picker == nullptr || picker->widget() == nullptr || host == nullptr)return;
        host->layout()->addWidget(picker->widget());
        if (checked)
            picker->setPickActive(true);
        picker->widget()->show();
    }

    void mCaseSettingPanel::unmountOutputPicker(const QString& var, bool isPch, const QString& nextVar)
    {
        auto& pickers = isPch ? _pch_pickers : m_pickers;
        if (pickers == nullptr)return;
        auto it = pickers->constFind(var);
        if (it == pickers->constEnd() || !it.value() || !it.value()->widget())return;
        if (m_pickerParkWidget == nullptr)return;           //寄存容器不存在：面板正在析构，一切即将销毁，无需寄存
        const bool wasActive = it.value()->isPickActive();
        if (wasActive && !nextVar.isEmpty()) {
            //接力：卸载激活态控件前失活，随即激活下一个已勾选的拾取变量，拾取会话无缝转移（工具栏保持置灰）
            it.value()->setPickActive(false);
            it.value()->widget()->setParent(m_pickerParkWidget);
            it.value()->widget()->hide();
            activateOutputPicker(nextVar, isPch);
            return;
        }
        if (wasActive) {
            //没有可接力的拾取变量：正常失活卸载，拾取会话结束，清除最近激活记录
            it.value()->setPickActive(false);
            if (isPch) m_lastActivePchVar.clear();
            else m_lastActiveVar.clear();
        }
        it.value()->widget()->setParent(m_pickerParkWidget);
        it.value()->widget()->hide();
    }

    //失活全部处于激活态的输出/PCH拾取控件，结束其视口拾取会话；
    //失活路径（pickActiveChanged=false）会统一恢复被置灰的拾取工具栏按钮
    void mCaseSettingPanel::deactivateActiveOutputPickers()
    {
        for (const auto& hash : { m_pickers, _pch_pickers }) {
            if (hash == nullptr)continue;
            for (auto it = hash->constBegin(); it != hash->constEnd(); ++it) {
                if (it.value() && it.value()->isPickActive())
                    it.value()->setPickActive(false);
            }
        }
    }

    //接力激活下一个已勾选变量的拾取控件（视图随勾选已挂载，控制器按需创建）
    void mCaseSettingPanel::activateOutputPicker(const QString& var, bool isPch)
    {
        auto picker = ensureOutputPicker(var, isPch);
        if (picker == nullptr || picker->widget() == nullptr)return;
        //已寄存（所在行已取消勾选）的控件不可激活，否则出现隐藏控件接收视口拾取的僵尸会话
        if (picker->widget()->parentWidget() == m_pickerParkWidget)return;
        picker->setPickActive(true);
    }

    //恢复拾取工具栏按钮（全部可选）
    void mCaseSettingPanel::restoreOutputPickToolbar()
    {
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }

    //面板进入：mInfoWindow::setMainWidget 切换面板时旧面板 hide()、新面板 show()。
    //全局当前没有激活中的拾取控件时，恢复上一次激活的输出拾取控件（保留跨面板切换前的激活状态）；
    //有其他激活控件（如其他面板的拾取会话仍在进行）则不抢占
    void mCaseSettingPanel::showEvent(QShowEvent* event)
    {
        if (!mPickWidgetController::hasActivePickController()) {
            if (!m_lastActiveVar.isEmpty())
                activateOutputPicker(m_lastActiveVar, false);
            else if (!m_lastActivePchVar.isEmpty())
                activateOutputPicker(m_lastActivePchVar, true);
        }
        mPanelBase::showEvent(event);
    }

    //面板退出：mInfoWindow::setMainWidget 切换面板/退出编辑态时会对旧面板 hide()（没有专门的面板退出事件，
    //隐藏事件即约定钩子）。此时失活激活态输出拾取控件并恢复被置灰的拾取工具栏按钮；
    //存在拾取变量定义时无条件兜底恢复，覆盖"最后一个拾取变量被取消勾选后延迟恢复"等无激活控件的场景
    void mCaseSettingPanel::hideEvent(QHideEvent* event)
    {
        deactivateActiveOutputPickers();
        if ((m_pickers && !m_pickers->isEmpty()) || (_pch_pickers && !_pch_pickers->isEmpty()))
            restoreOutputPickToolbar();
        mPanelBase::hideEvent(event);
    }

    void mCaseSettingPanel::releaseOutputPickers(bool isPch, bool immediate)
    {
        auto& pickers = isPch ? _pch_pickers : m_pickers;
        auto& varTypes = isPch ? _pch_pickerVarTypes : m_pickerVarTypes;
        if (pickers == nullptr)return;

        for (auto it = pickers->begin(); it != pickers->end(); ++it) {
            if (it.value() == nullptr)continue;
            //先失活已激活部件，否则销毁后点击其他部件，会因为激活的原因触发对已销毁对象的访问而崩溃。
            //keep=false：失活时只清空高亮、不重画临时高亮——重画走的是创建控件时捕获的渲染窗口，
            //在面板析构/类型切换上下文里该窗口可能已失效，是此前的崩溃源
            if (it.value()->isPickActive()) {
                it.value()->setKeepHighlightOnDeactivate(false);
                it.value()->setPickActive(false);
            }
            if (it.value()->widget()) {
                if (immediate)
                    delete it.value()->widget();    //同步销毁视图；壳内 QPointer 随之置空，壳析构回调自动跳过
                else
                    it.value()->widget()->deleteLater();    //延迟删除防重入（类型切换路径）
            }
            if (immediate)
                delete it.value();
            else
                it.value()->deleteLater();
        }
        pickers->clear();
        varTypes.clear();
    }

    inline QHash<QString, QString> mCaseSettingPanel::getSetNamesList(QVector<MxFunctions::SetType>types)
    {
        QStringList allSet = getTarSet(types);
        QHash<QString, QString> setToType;
        for (const auto& name : allSet)
        {
            auto* setBase = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(name);
            if (setBase)
            {
                setToType[name] = SetTypeToString(setBase->getSetType());
            }
            else
            {
                setToType[name] = "集合";
            }
        }
        return setToType;
    }
    void mCaseSettingPanel::createCellPickController(mPickWidgetController*& picker, mColorBlock colorBlock, const QString& tip, const QStringList& selected) {
        picker = mGeneralPickUp::createNoCommitPickWidget(colorBlock, this);
        picker->setDefultTip(tip);

        QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoFace,SetType::Element, SetType::ElementSurface });
        picker->setSelectSetList(setToType);

        PickWidgetData pickerData;
        for (auto set : selected) {
            if (setToType.contains(set))
                pickerData.selectedSetNames.insert(set);
        }
        picker->displayData(pickerData);

    }
}