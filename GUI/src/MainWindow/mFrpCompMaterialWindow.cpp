#include "mFrpCompMaterialWindow.h"
#include "ui_mFrpCompMaterialWindow.h"
#include <QLineEdit>
#include <QAbstractButton>
#include <QTreeWidgetItemIterator>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QSet>
#include <QSignalBlocker>
#include <memory>
#include <utility>
#include <QFileDialog>
#include <QDialogButtonBox>
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mAppConfiguration.h"
#include "mCurveWindow.h"
#include "mIOFileManager.h"

#include "mFrpData_global.h"
#include "mFrpBaseData.h"
#include "mFrpManager.h"
#include "mFrpDerivedData.h"
#include "mFrpDataInStream.h"
#include "mFrpDataOutStream.h"
#include "mDataStatusManager.h"

/*====================================================================================
 * 属性页基类：表单样板 / 相类型下拉 / 材料选择器 / 数值校验 / 表格公共操作
 * ==================================================================================*/
mFrpCompPropertyPage::mFrpCompPropertyPage(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background: #FFFFFF;");
}

void mFrpCompPropertyPage::markLoading(bool b) { m_loading = b; }

bool mFrpCompPropertyPage::isLoading() const { return m_loading; }

void mFrpCompPropertyPage::clearDataBinding() { m_pCurData = nullptr; }

void mFrpCompPropertyPage::notifyChanged() { emit contentsChanged(); }

// 默认统一做“同一微观结构内比例类型一致”提示；各数值输入框的合法性由失焦即时校验负责，不在切页重复检查。
void mFrpCompPropertyPage::validateData() { checkRatioConsistency(); }

QLabel* mFrpCompPropertyPage::makeTitle(const QString& text)
{
    QLabel* lb = new QLabel(text, this);
    QFont f = lb->font(); f.setBold(true); lb->setFont(f);
    return lb;
}

QGridLayout* mFrpCompPropertyPage::makeFormGrid()
{
    QGridLayout* g = new QGridLayout(this);
    g->setContentsMargins(80, 20, 110, 20);
    g->setHorizontalSpacing(24);
    g->setVerticalSpacing(14);
    g->setColumnStretch(1, 1);
    return g;
}

void mFrpCompPropertyPage::addFormStretch(QGridLayout* g, int row, int cols)
{
    g->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), row, 0, 1, cols);
}

QWidget* mFrpCompPropertyPage::makeRow(const QList<QWidget*>& widgets, bool isStretch)
{
    QWidget* w = new QWidget(this);
    QHBoxLayout* lay = new QHBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(10);
    for (QWidget* it : widgets) lay->addWidget(it);
    if (isStretch)
        lay->addStretch();
    return w;
}

// 空白占位行：用于在网格里拉开纵向间距（与行内控件对齐无关）
QWidget* mFrpCompPropertyPage::makeRow()
{
    return new QWidget(this);
}

QComboBox* mFrpCompPropertyPage::createPhaseCombo(int currentKind)
{
    QComboBox* cmb = new QComboBox(this);
    cmb->addItem(QStringLiteral("基体"));
    cmb->addItem(QStringLiteral("掺杂"));
    cmb->addItem(QStringLiteral("空腔"));
    cmb->addItem(QStringLiteral("连续纤维"));
    cmb->setCurrentIndex(currentKind);
    connect(cmb, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this, cmb](int idx) {
            if (isLoading()) return;
            const FrpNodeType newType = phaseTypeOfIndex(idx);
            if (newType == FrpNode_None) return;
            // 每个微观结构N 只允许一个基体：其它相切换为基体时拦截，恢复原相类型并提示
            if (newType == FrpNode_Matrix && m_pCurData && m_pCurData->type != FrpNode_Matrix)
            {
                MechFrpData::mFrpBaseData* micro = m_pCurData->ancestorOfType(FrpNode_Micro1);
                if (!MechFrpData::mFrpManager::canAttachPhase(micro, FrpNode_Matrix, m_pCurData))
                {
                    outputWarn(micro->name, QStringLiteral("当前微观结构中只允许存在一个基体"));
                    cmb->blockSignals(true);
                    cmb->setCurrentIndex(switchPhaseComboBox(m_pCurData->as<MechFrpData::mFrpPhaseData>()->type));  // 恢复原相类型
                    cmb->blockSignals(false);
                    return;                                                       // 不发 morphRequested，节点保持原类型
                }
            }
            emit morphRequested(newType);
        });
    return cmb;
}

QToolButton* mFrpCompPropertyPage::makeMaterialAddButton()
{
    QToolButton* btn = new QToolButton(this);
    btn->setFixedSize(24, 24);
    btn->setToolTip(QStringLiteral("新增材料"));
    btn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/cailiao.png"));
    return btn;
}

void mFrpCompPropertyPage::bindMaterialPicker(QComboBox* cmb, QToolButton* btn)
{
    m_materialCombos.append(cmb);
    connect(btn, &QToolButton::clicked, this, [this, cmb] {
        m_pPendingMaterialCombo = cmb;
        emit requestAddMaterial();
        });
}

void mFrpCompPropertyPage::refillMaterialCombos()
{
    QStringList names;
    if (MechFrpData::mFrpBaseData* g = materialGroupOf(m_pCurData))
        for (MechFrpData::mFrpBaseData* m : g->children) names << m->name;
    for (QComboBox* cmb : m_materialCombos)
    {
        const QString keep = cmb->currentIndex() <= 0 ? QString() : cmb->currentText();
        cmb->blockSignals(true);
        cmb->clear();
        cmb->addItem(QStringLiteral("无"));
        for (const QString& n : names) cmb->addItem(n);
        const int idx = cmb->findText(keep);
        cmb->setCurrentIndex(idx >= 0 ? idx : 0);
        cmb->blockSignals(false);
    }
}

void mFrpCompPropertyPage::selectMaterial(QComboBox* cmb, const QString& name)
{
    const int idx = name.isEmpty() ? -1 : cmb->findText(name);
    cmb->setCurrentIndex(idx >= 0 ? idx : 0);
}

QString mFrpCompPropertyPage::materialName(const QComboBox* cmb) const
{
    return cmb->currentIndex() <= 0 ? QString() : cmb->currentText();
}

// 分布函数下拉：列出当前均质化模型“长细比函数”分组下的函数名，按名选中（空名/找不到回“无”）
void mFrpCompPropertyPage::refillDistFuncCombo(QComboBox* cmb, const QString& currentName)
{
    if (!cmb) return;
    QStringList names;
    if (m_pCurData)
    {
        if (MechFrpData::mFrpBaseData* an = m_pCurData->ancestorOfType(FrpNode_Analysis1))
            for (MechFrpData::mFrpBaseData* c : an->children)
                if (c->type == FrpNode_AspectRatio)
                    for (MechFrpData::mFrpBaseData* fn : c->children)
                        names << fn->name;
    }
    cmb->blockSignals(true);
    cmb->clear();
    cmb->addItem(QStringLiteral("无"));
    for (const QString& n : names) cmb->addItem(n);
    const int idx = currentName.isEmpty() ? 0 : cmb->findText(currentName);
    cmb->setCurrentIndex(idx >= 0 ? idx : 0);
    cmb->blockSignals(false);
}

QString mFrpCompPropertyPage::distFuncNameOf(const QComboBox* cmb) const
{
    return (cmb && cmb->currentIndex() > 0) ? cmb->currentText() : QString();
}

// 张量文件“浏览”按钮：点击选 xml，选中后把绝对路径填入只读输入框（textChanged 触发自动写回）
QToolButton* mFrpCompPropertyPage::makeTensorFileButton(QLineEdit* edit)
{
    QToolButton* btn = new QToolButton(this);
    btn->setFixedSize(24, 24);
    btn->setToolTip(QStringLiteral("选择张量文件"));
    btn->setText(QStringLiteral("..."));
    connect(btn, &QToolButton::clicked, this, [this, edit, btn] {
        const QString start = edit ? edit->text() : QString();
        const QString fn = QFileDialog::getOpenFileName(
            btn, QStringLiteral("选择取向张量文件"), start,
            QStringLiteral("取向张量文件 (*.xml);;所有文件 (*.*)"));
        if (!fn.isEmpty() && edit) edit->setText(fn);
        });
    return btn;
}

void mFrpCompPropertyPage::onMaterialAdded()
{
    QComboBox* target = m_pPendingMaterialCombo;
    refillMaterialCombos();                                    // 重建并保持各下拉原选中
    if (target) target->setCurrentIndex(target->count() - 1);  // 选中新材料，触发自动保存材料名
}

void mFrpCompPropertyPage::outputWarn(const QString& object, const QString& explain)
{
    getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_FRCMaterial, {-1});
    QList<outputInfoData> list;
    outputInfoData info;
    info._type = QStringLiteral("警告");
    info._module = QStringLiteral("纤维增强复合材料");
    info._object = object;
    info._explain = explain;
    info._ID = -1;
    list.append(info);
    MechCommon::mMechGlobalSignals::getInstance()->updataNormalInfo(list);
}

int mFrpCompPropertyPage::switchPhaseComboBox(FrpNodeType type)
{
    switch (type)
    {
    case 		FrpNode_Matrix:    return 0;
    case 		FrpNode_Inclusion: return 1;
    case 		FrpNode_Cavity:    return 2;
    case 		FrpNode_Fiber:     return 3;
    default: return -1;
    }
}

// 相类型下拉索引（基体0/掺杂1/空腔2/连续纤维3）-> 节点类型（switchPhaseComboBox 的逆映射）
FrpNodeType mFrpCompPropertyPage::phaseTypeOfIndex(int idx)
{
    switch (idx)
    {
    case 0: return FrpNode_Matrix;
    case 1: return FrpNode_Inclusion;
    case 2: return FrpNode_Cavity;
    case 3: return FrpNode_Fiber;
    default: return FrpNode_None;
    }
}

// 取节点所属均质化模型下的“材料”分组（找不到返回 nullptr）
MechFrpData::mFrpBaseData* mFrpCompPropertyPage::materialGroupOf(MechFrpData::mFrpBaseData* context)
{
    if (!context) return nullptr;
    MechFrpData::mFrpBaseData* an = context->ancestorOfType(FrpNode_Analysis1);
    if (!an) return nullptr;
    for (MechFrpData::mFrpBaseData* c : an->children)
        if (c->type == FrpNode_Material && c->isGroupData()) return c;
    return nullptr;
}

// 分数输入框失焦：为空不处理；非数字或超出 0~1 则警告并清空当前框（clear 触发 textChanged 自动写回空值）
void mFrpCompPropertyPage::bindFractionEdit(QLineEdit* edit, const QString& explain)
{
    connect(edit, &QLineEdit::editingFinished, this, [this, edit, explain] {
        const QString s = edit->text().trimmed();
        if (s.isEmpty()) return;
        bool ok = false;
        const double v = s.toDouble(&ok);
        if (!ok || v < -1e-9 || v > 1.0 + 1e-9)
        {
            outputWarn(m_pCurData ? m_pCurData->name : QString(), explain);
            edit->clear();
        }
        });
}

// 正数输入框（如长细比）失焦：为空不处理；非数字或 ≤0 则警告并清空当前框
void mFrpCompPropertyPage::bindPositiveEdit(QLineEdit* edit, const QString& explain)
{
    connect(edit, &QLineEdit::editingFinished, this, [this, edit, explain] {
        const QString s = edit->text().trimmed();
        if (s.isEmpty()) return;
        bool ok = false;
        const double v = s.toDouble(&ok);
        if (!ok || v <= 1e-9)
        {
            outputWarn(m_pCurData ? m_pCurData->name : QString(), explain);
            edit->clear();
        }
        });
}

// 分数总和 ≤1：主分数 + 当前勾选状态下参与求和的附加框；任一绑定框失焦即重算，超限清空刚编辑的框
void mFrpCompPropertyPage::bindFractionSum(
    QLineEdit* primary,
    const QList<QLineEdit*>& allExtras,
    const std::function<QList<QLineEdit*>()>& extrasProvider,
    const QString& explain)
{
    auto check = [this, primary, extrasProvider, explain](QLineEdit* just) {
        bool okMain = false;
        const double main = primary->text().trimmed().toDouble(&okMain);
        if (!okMain) return;
        double sum = main;
        for (QLineEdit* e : extrasProvider())
        {
            bool ok = false;
            const double v = e->text().trimmed().toDouble(&ok);
            if (ok) sum += v;
        }
        if (sum > 1.0 + 1e-9)
        {
            outputWarn(m_pCurData ? m_pCurData->name : QStringLiteral("材料"),
                explain.arg(QString::number(sum, 'g', 6)));
            just->clear();
        }
        };
    QList<QLineEdit*> all = allExtras;
    all.prepend(primary);
    for (QLineEdit* e : all)
        connect(e, &QLineEdit::editingFinished, this, [check, e] { check(e); });
}

// 6 分量取向张量三角矩阵弹窗：确定返回“a11;a12;a13;a22;a23;a33;”，取消返回原串
QString mFrpCompPropertyPage::showTensor6Dialog(const QString& stored)
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("取向张量"));
    dlg.setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    QGridLayout* g = new QGridLayout(&dlg);
    g->setHorizontalSpacing(24);
    g->setVerticalSpacing(16);
    const QStringList labels = { QStringLiteral("a[1,1]"), QStringLiteral("a[1,2]"), QStringLiteral("a[1,3]"),
                                QStringLiteral("a[2,2]"), QStringLiteral("a[2,3]"), QStringLiteral("a[3,3]") };
    // 参考为三角形：a[2,2] 与 a[1,2] 同列、a[3,3] 与 a[1,3] 同列
    const int rowIdx[6] = { 0, 0, 0, 1, 1, 2 };
    const int colIdx[6] = { 0, 1, 2, 1, 2, 2 };   // 每对占 2 列
    const QStringList oldVals = stored.split(';');
    QList<QLineEdit*> edits;
    for (int i = 0; i < 6; ++i)
    {
        g->addWidget(new QLabel(labels[i], &dlg), rowIdx[i], colIdx[i] * 2);
        QLineEdit* e = new QLineEdit(oldVals.value(i, QStringLiteral("0")).trimmed(), &dlg);
        e->setFixedWidth(90);                    // 参考弹窗输入框较窄
        g->addWidget(e, rowIdx[i], colIdx[i] * 2 + 1);
        edits << e;
    }
    QString result = stored;                     // 取消默认保持原串
    QDialogButtonBox* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dlg);
    btns->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    btns->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    g->addWidget(btns, 3, 0, 1, 6);
    connect(btns, &QDialogButtonBox::accepted, &dlg, [&dlg, edits, &result]() {
        QStringList vals;
        for (QLineEdit* e : edits)
        {
            bool ok = false;
            const QString v = e->text().trimmed();
            v.toDouble(&ok);                     // 空 / 非数值一律按 0
            vals << (ok ? v : QStringLiteral("0"));
        }
        result = vals.join(';') + ';';
        dlg.accept();
        });
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    // QDialogButtonBox 在显示时会强制把“确定”设为默认按钮，显示后再关一次，使回车不关闭弹窗
    QTimer::singleShot(0, &dlg, [btns]() {
        if (QPushButton* ok = btns->button(QDialogButtonBox::Ok))
        {
            ok->setAutoDefault(false);
            ok->setDefault(false);
        }
        });
    dlg.exec();
    return result;
}

// 表格某列按行重排 1..N（仅对已有 item 生效，避免覆盖控件单元格）
void mFrpCompPropertyPage::renumberTableColumn(QTableWidget* table, int col)
{
    if (!table) return;
    for (int i = 0; i < table->rowCount(); ++i)
        if (QTableWidgetItem* it = table->item(i, col))
            it->setText(QString::number(i + 1));
}

// 删选中行；无选中删最后一行；空表无操作
void mFrpCompPropertyPage::removeSelectedOrLastRow(QTableWidget* table)
{
    if (!table || table->rowCount() <= 0) return;
    int r = table->currentRow();
    if (r < 0) r = table->rowCount() - 1;
    table->removeRow(r);
}

// 同一微观结构N 内：所有掺杂/连续纤维的比例类型（质量分数/体积分数）与空腔（恒体积分数）应一致；
// 仅提示，不改动任何节点
void mFrpCompPropertyPage::checkRatioConsistency()
{
    if (!m_pCurData) return;
    MechFrpData::mFrpBaseData* micro = m_pCurData->ancestorOfType(FrpNode_Micro1);
    if (!micro) return;
    int type = -1;
    for (MechFrpData::mFrpBaseData* c : micro->children)
    {
        MechFrpData::mFrpPhaseData* p = c->as<MechFrpData::mFrpPhaseData>();
        if (!p) continue;
        int r = -1;
        if (c->type == FrpNode_Inclusion)      r = p->inc.ratioKind;
        else if (c->type == FrpNode_Fiber)     r = p->fib.ratioKind;
        else if (c->type == FrpNode_Cavity)    r = 1;   // 空腔只能用体积分数，参与一致性比较（以代码为准）
        else continue;                         // 基体无比例类型，不参与
        if (type < 0) type = r;
        else if (r != type)
        {
            outputWarn(m_pCurData->name, QStringLiteral("同一微观结构下各相的比例类型（质量分数/体积分数）应保持一致"));
            return;
        }
    }
}

/*====================================================================================
 * 属性面板调度
 * ==================================================================================*/
mMaterialPropertyPanel::mMaterialPropertyPanel(QStackedWidget* stack, QWidget* parent)
    : QWidget(parent), m_pStack(stack)
{
    // stack 本体
    m_pStack->setStyleSheet("QStackedWidget { background: #FFFFFF; }");
    // 兜底空页（.ui 里的 page_Empty，不经过基类，单独处理）
    m_pEmptyPage = m_pStack->widget(0);
    m_pEmptyPage->setAttribute(Qt::WA_StyledBackground, true);
    m_pEmptyPage->setStyleSheet("background: #FFFFFF;");
    registerPages();
    m_pStack->setCurrentWidget(m_pEmptyPage);
}

void mMaterialPropertyPanel::showPageForData(MechFrpData::mFrpBaseData* data)
{
    flushCurrent();                               // 1) 先把旧页面数据写回旧节点
    // 切走旧页面前做一次规则校验（仅切页时触发，避免输入过程中频繁弹警告）；此时当前页仍是旧页
    if (auto* oldPage = qobject_cast<mFrpCompPropertyPage*>(m_pStack->currentWidget()))
        oldPage->validateData();
    m_pCurData = data;
    if (!data || data->isGroupData())             // 空节点 / 分组节点（无界面无数据）-> 空页
    {
        m_pStack->setCurrentWidget(m_pEmptyPage);
        return;
    }
    mFrpCompPropertyPage* page = m_pages.value(data->type, nullptr);
    if (!page && m_factories.contains(data->type))
    {
        page = m_factories.value(data->type)();   // 首次选中才创建
        installAutoSave(page);                    // 控件一编辑就即时写回
        m_pStack->addWidget(page);
        m_pages.insert(data->type, page);         // 之后按类型缓存复用
    }
    if (!page)
    {
        m_pStack->setCurrentWidget(m_pEmptyPage);
        return;
    }
    m_pStack->setCurrentWidget(page);
    applyAnalysisLink(data, page);                // 2) 材料页：先按 Analysis1 类型约束下拉（须在 loadData 前）
    page->markLoading(true);
    page->loadData(data);                         // 3) 反显当前节点强类型数据
    page->markLoading(false);
}

void mMaterialPropertyPanel::flushCurrent()
{
    if (!m_pCurData) return;
    mFrpCompPropertyPage* page = m_pages.value(m_pCurData->type, nullptr);
    // 仅当该页面正在显示时才取回，避免后台缓存页的旧值覆盖节点数据
    if (page && m_pStack->currentWidget() == page)
        page->saveData(m_pCurData);
}

void mMaterialPropertyPanel::clearCurrent()
{
    m_pCurData = nullptr;
    for (mFrpCompPropertyPage* page : m_pages)
        page->clearDataBinding();
    m_pStack->setCurrentWidget(m_pEmptyPage);
}

// 递归把页面内所有输入控件的“被编辑”信号接到 contentsChanged -> flushCurrent
void mMaterialPropertyPanel::installAutoSave(mFrpCompPropertyPage* page)
{
    for (QComboBox* cb : page->findChildren<QComboBox*>())
        connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), page,
            [page](int) { if (!page->isLoading()) page->notifyChanged(); });
    for (QLineEdit* le : page->findChildren<QLineEdit*>())
        connect(le, &QLineEdit::textChanged, page,
            [page](const QString&) { if (!page->isLoading()) page->notifyChanged(); });
    for (QAbstractButton* btn : page->findChildren<QAbstractButton*>())
        connect(btn, &QAbstractButton::toggled, page,
            [page](bool) { if (!page->isLoading()) page->notifyChanged(); });
    connect(page, &mFrpCompPropertyPage::contentsChanged, this, [this] { flushCurrent(); });
    // 相页面把“相类型被用户切换”向上转发给窗口做节点变形
    connect(page, &mFrpCompPropertyPage::morphRequested, this,
        [this, page](FrpNodeType newType) { if (m_pCurData && !page->isLoading()) emit phaseMorphRequested(m_pCurData, newType); });
    // 相页面“新增材料”按钮：转发给窗口（窗口负责在所属模型材料分组下建材料）
    connect(page, &mFrpCompPropertyPage::requestAddMaterial, this,
        [this]() { if (m_pCurData) emit materialAddRequested(m_pCurData); });
}

// 新材料入库后：让对应相页面重建材料下拉并选中新材料
void mMaterialPropertyPanel::notifyMaterialAdded(MechFrpData::mFrpBaseData* data)
{
    if (!data) return;
    if (mFrpCompPropertyPage* p = m_pages.value(data->type, nullptr))
        p->onMaterialAdded();
}

void mMaterialPropertyPanel::applyAnalysisLink(MechFrpData::mFrpBaseData* data, mFrpCompPropertyPage* page)
{
    if (data->type != FrpNode_Material) return;
    MaterialPage* mp = qobject_cast<MaterialPage*>(page);
    if (!mp) return;
    QString aType;                               // Analysis1 未选类型时为空（材料种类禁用）
    if (const MechFrpData::mFrpBaseData* an = data->ancestorOfType(FrpNode_Analysis1))
        if (const MechFrpData::mFrpAnalysisData* ad = an->as<MechFrpData::mFrpAnalysisData>())
            aType = ad->analysisType;
    mp->applyAnalysisType(aType);
}

void mMaterialPropertyPanel::registerPages()
{
    m_factories.insert(FrpNode_Analysis1, [] { return new Analysis1Page(); });
    m_factories.insert(FrpNode_Material, [] { return new MaterialPage(); });
    m_factories.insert(FrpNode_Matrix, [] { return new MicroDefPage(); });
    m_factories.insert(FrpNode_Inclusion, [] { return new InclusionPage(); });
    m_factories.insert(FrpNode_Fiber, [] { return new FiberPage(); });
    m_factories.insert(FrpNode_Cavity, [] { return new CavityPage(); });
    m_factories.insert(FrpNode_Rve, [] { return new RvePage(); });
    m_factories.insert(FrpNode_Failure, [] { return new FailurePage(); });
    m_factories.insert(FrpNode_AspectRatio, [] { return new AspectRatioFunPage(); });
}

/************************************************** Analysis1Page ******************************************************/
Analysis1Page::Analysis1Page(QWidget* parent)
    : mFrpCompPropertyPage(parent)
{
    QLabel* title = makeTitle(QStringLiteral("定义"));
    QLabel* lbType = new QLabel(QStringLiteral("类型"), this);
    QLabel* lbHomo = new QLabel(QStringLiteral("均质化模型"), this);
    QLabel* lbMulti = new QLabel(QStringLiteral("多相掺杂均质化"), this);
    m_cmbType = new QComboBox(this);
    m_cmbHomoModel = new QComboBox(this);
    m_cmbMultiPhase = new QComboBox(this);
    auto fillCombo = [](QComboBox* cb, const QStringList& items) {
        cb->addItems(items); cb->setCurrentIndex(0);
        };
    fillCombo(m_cmbType, { QStringLiteral("力学分析"), QStringLiteral("热-力学分析"), QStringLiteral("热学分析") });
    fillCombo(m_cmbHomoModel, { QStringLiteral("Mori Tanaka"), QStringLiteral("Double Inclusion") });
    fillCombo(m_cmbMultiPhase, { QStringLiteral("无"),QStringLiteral("Multi-Step"), QStringLiteral("Multi-Level") });
    // 样式统一：下拉伸展对齐
    for (QComboBox* cb : { m_cmbType, m_cmbHomoModel, m_cmbMultiPhase })
        cb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    for (QLabel* lb : { lbType, lbHomo, lbMulti })
        lb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QGridLayout* grid = makeFormGrid();
    int r = 0;
    grid->addWidget(title, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbType, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbType, r, 1); r++;
    grid->addWidget(lbHomo, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbHomoModel, r, 1); r++;
    grid->addWidget(lbMulti, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbMultiPhase, r, 1); r++;
    addFormStretch(grid, r, 2);
    // 控件自动保存统一由面板 installAutoSave() 递归连接，页面内不再单独 connect
}

void Analysis1Page::loadData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpAnalysisData* d = data ? data->as<MechFrpData::mFrpAnalysisData>() : nullptr;
    m_cmbType->setCurrentText(d ? d->analysisType : QString());
    m_cmbHomoModel->setCurrentText(d ? d->homoModel : QString());
    m_cmbMultiPhase->setCurrentText(d ? d->multiPhase : QString());
}

void Analysis1Page::saveData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpAnalysisData* d = data ? data->as<MechFrpData::mFrpAnalysisData>() : nullptr;
    if (!d) return;
    d->analysisType = m_cmbType->currentText();
    d->homoModel = m_cmbHomoModel->currentText();
    d->multiPhase = m_cmbMultiPhase->currentText();
}

/************************************************** 材料 ******************************************************/
MaterialPage::MaterialPage(QWidget* parent)
    : mFrpCompPropertyPage(parent)
{
    QLabel* title = makeTitle(QStringLiteral("材料定义"));
    QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
    // 材料种类
    QLabel* lbKind = new QLabel(QStringLiteral("材料种类"), this);
    m_cmbKind = new QComboBox(this);
    m_cmbKind->addItem(QStringLiteral("线弹性"));
    m_cmbKind->addItem(QStringLiteral("热学"));
    m_cmbKind->setCurrentIndex(0);
    // 材料类型
    QLabel* lbType = new QLabel(QStringLiteral("材料类型"), this);
    m_cmbType = new QComboBox(this);
    m_cmbType->addItem(QStringLiteral("各向同性"));
    m_cmbType->setCurrentIndex(0);
    // 各向同性 分区标题
    QLabel* lbSection = makeTitle(QStringLiteral("各向同性"));
    // 杨氏模量
    QLabel* lbYoung = new QLabel(QStringLiteral("杨氏模量"), this);
    m_editYoung = new mCalcLineEdit(CalcMode::Standard);
    m_editYoung->setText(QStringLiteral("请输入"));
    QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
    m_editYoung->setUnit(stressunit);
    // 泊松比
    QLabel* lbPoisson = new QLabel(QStringLiteral("泊松比"), this);
    m_editPoisson = new mCalcLineEdit(CalcMode::Standard);
    m_editPoisson->setText(QStringLiteral("请输入"));
    // 密度（参考图：密度字段上方单独一行“密度”标题，无输入框，普通字重）
    QLabel* lbDensityTitle = makeTitle(QStringLiteral("密度"));
    QLabel* lbDensity = new QLabel(QStringLiteral("密度"), this);
    m_editDensity = new mCalcLineEdit(CalcMode::Standard);
    m_editDensity->setText(QStringLiteral("请输入"));
    QString denunit = UnitsLibrary[(int)Unit::Density][sysunitidxs[0]];
    m_editDensity->setUnit(denunit);
    // 热膨胀系数（勾选框，默认不勾选）——保持原散排布局，仅把标签提升为成员，便于随 Analysis1 类型整体显隐
    m_chkThermalExp = new QCheckBox(QStringLiteral("热膨胀系数"), this);
    m_chkThermalExp->setChecked(false);
    QFont cf = m_chkThermalExp->font(); cf.setBold(true); m_chkThermalExp->setFont(cf);
    // 未勾选时参考温度/热膨胀系数/A1 三行常显但禁用，勾选后才可编辑
    connect(m_chkThermalExp, &QCheckBox::toggled, this, [this](bool on) {
        m_lbRefTemp->setEnabled(on);     m_editRefTemp->setEnabled(on);
        m_lbThermalExp->setEnabled(on);  m_cmbThermalExp->setEnabled(on);
        m_lbA1->setEnabled(on);          m_editA1->setEnabled(on);
        });
    // 参考温度
    QString unit1 = "";
    QString unit2 = "";
    if (sysunitidxs[0] >= 6)
    {
        unit1 = UnitsLibrary[(int)Unit::Temp][2];
        unit2 = UnitsLibrary[(int)Unit::TempReciprocal][2];
    }
    else
    {
        unit1 = UnitsLibrary[(int)Unit::Temp][sysunitidxs[3]];
        unit2 = UnitsLibrary[(int)Unit::TempReciprocal][sysunitidxs[3]];
    }
    m_lbRefTemp = new QLabel(QStringLiteral("参考温度"), this);
    m_editRefTemp = new mCalcLineEdit(CalcMode::Standard);
    m_editRefTemp->setText(QStringLiteral("0.0"));
    m_editRefTemp->setUnit(unit1);
    // 热膨胀系数（选择）
    m_lbThermalExp = new QLabel(QStringLiteral("热膨胀系数"), this);
    m_cmbThermalExp = new QComboBox(this);
    m_cmbThermalExp->addItem(QStringLiteral("各项同性热膨胀系数"));
    m_cmbThermalExp->setCurrentIndex(0);
    // A1
    m_lbA1 = new QLabel(QStringLiteral("A1"), this);
    m_editA1 = new mCalcLineEdit(CalcMode::Standard);
    m_editA1->setText(QStringLiteral("请输入"));
    m_editA1->setUnit(unit2);
    // 样式统一：标签右对齐、下拉伸展
    for (QLabel* lb : { lbKind, lbType, lbYoung, lbPoisson, lbDensity, m_lbRefTemp, m_lbThermalExp, m_lbA1 })
        lb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    for (QComboBox* cb : { m_cmbKind, m_cmbType, m_cmbThermalExp })
        cb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 布局：两列(名称|输入) + 单位列，和 Analysis1Page 保持一致（与原基线完全相同的散排网格）
    QGridLayout* grid = makeFormGrid();
    int r = 0;
    grid->addWidget(title, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbKind, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbKind, r, 1); r++;
    grid->addWidget(lbType, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbType, r, 1); r++;
    auto spacerRow1 = makeRow();
    grid->addWidget(spacerRow1, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbSection, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbYoung, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editYoung, r, 1);  r++;
    grid->addWidget(lbPoisson, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editPoisson, r, 1); r++;
    auto spacerRow2 = makeRow();
    grid->addWidget(spacerRow2, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbDensityTitle, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbDensity, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editDensity, r, 1); r++;
    auto temp3 = makeRow();
    grid->addWidget(temp3, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(m_chkThermalExp, r++, 0, 1, 3);
    grid->addWidget(m_lbRefTemp, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editRefTemp, r, 1);  r++;
    grid->addWidget(m_lbThermalExp, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbThermalExp, r, 1); r++;
    grid->addWidget(m_lbA1, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editA1, r, 1);  r++;
    addFormStretch(grid, r);
    // 初始先隐藏热膨胀区，进入页面时 applyAnalysisType() 会按所属 Analysis1 类型决定是否显示
    applyAnalysisType(QStringLiteral("力学分析"));
}

void MaterialPage::loadData(MechFrpData::mFrpBaseData* data)
{
    m_editYoung->setExpression("");
    m_editPoisson->setExpression("");
    m_editDensity->setExpression("");
    m_editRefTemp->setExpression("");
    m_editA1->setExpression("");
    MechFrpData::mFrpMaterialData* d = data ? data->as<MechFrpData::mFrpMaterialData>() : nullptr;
    // 注意：面板会先调用 applyAnalysisType() 重建“材料种类”下拉，再执行本函数回填
    m_cmbKind->setCurrentText(d ? d->m_kind : QString());
    m_cmbType->setCurrentText(d ? d->matType : QString());
    m_editYoung->setExpression(d ? d->young : QString());
    m_editPoisson->setExpression(d ? d->poisson : QString());
    m_editDensity->setExpression(d ? d->density : QString());
    m_chkThermalExp->setChecked(d ? d->thermalExp : false);
    m_editRefTemp->setExpression(d ? d->refTemp : QString());
    m_cmbThermalExp->setCurrentIndex(d ? d->thermalExpType : 0);
    m_editA1->setExpression(d ? d->a1 : QString());
}

void MaterialPage::saveData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpMaterialData* d = data ? data->as<MechFrpData::mFrpMaterialData>() : nullptr;
    if (!d) return;
    d->m_kind = m_cmbKind->currentText();
    d->matType = m_cmbType->currentText();
    d->young = m_editYoung->text();
    d->poisson = m_editPoisson->text();
    d->density = m_editDensity->text();
    d->thermalExp = m_chkThermalExp->isChecked();
    d->refTemp = m_editRefTemp->text();
    d->thermalExpType = m_cmbThermalExp->currentIndex();
    d->a1 = m_editA1->text();
}

void MaterialPage::applyAnalysisType(QString analysisType)
{
    auto rebuildKind = [this](bool elastic, bool thermal) {
        m_cmbKind->blockSignals(true);
        const int keep = m_cmbKind->currentIndex();
        m_cmbKind->clear();
        if (elastic) m_cmbKind->addItem(QStringLiteral("线弹性"));
        if (thermal) m_cmbKind->addItem(QStringLiteral("热学"));
        m_cmbKind->setCurrentIndex(keep < m_cmbKind->count() ? keep : 0);
        m_cmbKind->blockSignals(false);
        };
    bool kindEnabled = true;
    if (analysisType == QStringLiteral("力学分析") || analysisType == QStringLiteral("热-力学分析"))
    {
        rebuildKind(true, false);
    }
    else if (analysisType == QStringLiteral("热学分析"))
    {
        rebuildKind(false, true);
    }
    else
    {
        rebuildKind(false, false);
        kindEnabled = false;
    }
    m_cmbKind->setEnabled(kindEnabled);
    // 仅 Thermomechanical 才有热膨胀系数这一块（默认不勾选）
    const bool showThermal = (analysisType == QStringLiteral("热-力学分析"));
    m_chkThermalExp->setVisible(showThermal);
    m_lbRefTemp->setVisible(showThermal);
    m_editRefTemp->setVisible(showThermal);
    m_lbThermalExp->setVisible(showThermal);
    m_cmbThermalExp->setVisible(showThermal);
    m_lbA1->setVisible(showThermal);
    m_editA1->setVisible(showThermal);
    // 显示热膨胀区时，三行子项的可用状态还要跟随勾选框（默认不勾选=禁用）
    const bool thermalOn = showThermal && m_chkThermalExp->isChecked();
    m_lbRefTemp->setEnabled(thermalOn);     m_editRefTemp->setEnabled(thermalOn);
    m_lbThermalExp->setEnabled(thermalOn);  m_cmbThermalExp->setEnabled(thermalOn);
    m_lbA1->setEnabled(thermalOn);          m_editA1->setEnabled(thermalOn);
}

/************************************************** 基体 ******************************************************/
MicroDefPage::MicroDefPage(QWidget* parent)
    : mFrpCompPropertyPage(parent)
{
    QLabel* title = makeTitle(QStringLiteral("定义"));
    QLabel* lbPhase = new QLabel(QStringLiteral("相类型"), this);
    m_cmbPhase = createPhaseCombo(0);
    m_cmbPhase->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QLabel* lbMat = new QLabel(QStringLiteral("材料"), this);
    m_cmbMaterial = new QComboBox(this);
    m_cmbMaterial->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_btnMaterial = makeMaterialAddButton();
    bindMaterialPicker(m_cmbMaterial, m_btnMaterial);
    QWidget* wMat = makeRow({ m_cmbMaterial, m_btnMaterial });
    // 样式统一：标签右对齐
    lbPhase->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lbMat->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QGridLayout* grid = makeFormGrid();
    int r = 0;
    grid->addWidget(title, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbPhase, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbPhase, r, 1); r++;
    grid->addWidget(lbMat, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(wMat, r, 1); r++;
    addFormStretch(grid, r, 2);
}

void MicroDefPage::loadData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpPhaseData* d = data ? data->as<MechFrpData::mFrpPhaseData>() : nullptr;
    m_pCurData = data;
    m_pPendingMaterialCombo = nullptr;
    m_cmbPhase->setCurrentIndex(d ? switchPhaseComboBox(d->type) : -1);
    refillMaterialCombos();                       // 按所属模型材料分组重建
    selectMaterial(m_cmbMaterial, d ? d->baseMaterial : QString());
}

void MicroDefPage::saveData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpPhaseData* d = data ? data->as<MechFrpData::mFrpPhaseData>() : nullptr;
    if (!d) return;
    d->baseMaterial = materialName(m_cmbMaterial);
}
/************************************************** 掺杂 ******************************************************/
InclusionPage::InclusionPage(QWidget* parent)
    : mFrpCompPropertyPage(parent)
{
    QLabel* title = makeTitle(QStringLiteral("定义"));
    // 相类型 / 材料 / 比例 / 掺杂形状
    QLabel* lbPhase = new QLabel(QStringLiteral("相类型"), this);
    m_cmbPhase = createPhaseCombo(1);
    auto wlbPhase = makeRow({ m_cmbPhase });
    QLabel* lbMat = new QLabel(QStringLiteral("材料"), this);
    m_cmbMaterial = new QComboBox(this);
    m_btnMaterial = makeMaterialAddButton();
    bindMaterialPicker(m_cmbMaterial, m_btnMaterial);
    m_wMaterial = makeRow({ m_cmbMaterial, m_btnMaterial });
    QLabel* lbRatio = new QLabel(QStringLiteral("比例"), this);
    m_cmbRatio = new QComboBox(this);
    m_cmbRatio->addItems({ QStringLiteral("质量分数"), QStringLiteral("体积分数") });
    m_cmbRatio->setCurrentIndex(0);
    m_editRatio = new QLineEdit(this);
    m_editRatio->setText(QStringLiteral("请输入范围0-1"));
    QWidget* wRatio = makeRow({ m_cmbRatio, m_editRatio });
    QLabel* lbShape = new QLabel(QStringLiteral("掺杂形状"), this);
    m_cmbShape = new QComboBox(this);
    m_cmbShape->addItems({ QStringLiteral("固定长细比"), QStringLiteral("离散长细比") });
    m_cmbShape->setCurrentIndex(0);
    auto wlbShape = makeRow({ m_cmbShape });
    // 固定长细比 -> 长细比
    m_lbAspect = new QLabel(QStringLiteral("长细比"), this);
    m_editAspect = new QLineEdit(this);
    m_editAspect->setText(QStringLiteral("请输入"));
    // 离散长细比 -> 分布函数 + 离散数量
    m_lbDist = new QLabel(QStringLiteral("分布函数"), this);
    m_cmbDistFunc = new QComboBox(this);
    m_cmbDistFunc->addItem(QStringLiteral("无"));
    m_lbCount = new QLabel(QStringLiteral("离散数量"), this);
    m_editDiscreteCount = new QLineEdit(this);
    m_editDiscreteCount->setText(QStringLiteral("请输入整数"));

    // 掺杂方向
    QLabel* lbDir = makeTitle(QStringLiteral("掺杂方向"));
    QLabel* lbMethod = new QLabel(QStringLiteral("定义方式"), this);
    m_cmbDefMethod = new QComboBox(this);
    m_cmbDefMethod->addItems({ QStringLiteral("无"), QStringLiteral("固定角度"),
                              QStringLiteral("固定取向张量"), QStringLiteral("取向张量文件"),
                              QStringLiteral("2D随机"), QStringLiteral("3D随机") });
    m_cmbDefMethod->setCurrentIndex(0);
    auto wlbMethod = makeRow({ m_cmbDefMethod });
    // 固定角度
    QLabel* lbFixed = new QLabel(QStringLiteral("固定角度"), this);
    QLabel* lbTheta = new QLabel(QStringLiteral("Theta"), this);
    m_editTheta = new QLineEdit(this);
    m_editTheta->setText(QStringLiteral("请输入"));
    QLabel* lbPhi = new QLabel(QStringLiteral("Phi"), this);
    m_editPhi = new QLineEdit(this);
    m_editPhi->setText(QStringLiteral("请输入"));
    m_wFixedAngle = makeRow({ lbTheta, m_editTheta, lbPhi, m_editPhi });
    // 固定取向张量
    QLabel* lbTensor = new QLabel(QStringLiteral("取向张量"), this);
    m_editTensor = new QLineEdit(this);
    m_editTensor->setText(QStringLiteral("0;0;0;0;0;0;"));
    m_editTensor->setReadOnly(true);
    m_btnView = new QPushButton(QStringLiteral("查看"), this);
    m_wTensor = makeRow({ m_editTensor, m_btnView });
    // 取向张量文件
    QLabel* lbTensorFile = new QLabel(QStringLiteral("张量文件"), this);
    m_editTensorFile = new QLineEdit(this);
    m_editTensorFile->setReadOnly(true);
    m_wTensorFile = makeRow({ lbTensorFile, m_editTensorFile, makeTensorFileButton(m_editTensorFile) });
    // 高级特征定义
    m_chkAdvanced = new QCheckBox(QStringLiteral("高级特征定义"), this);
    QFont ef = m_chkAdvanced->font(); ef.setBold(true); m_chkAdvanced->setFont(ef);
    m_chkAdvanced->setChecked(false);
    m_radioCoating = new QRadioButton(QStringLiteral("涂层"), this);
    m_radioFiber = new QRadioButton(QStringLiteral("纤维族"), this);
    QWidget* wFeatureRadio = makeRow({ m_radioCoating, m_radioFiber }, true);
    QLabel* lbFeatMat = new QLabel(QStringLiteral("材料"), this);
    m_cmbFeatMat = new QComboBox(this);
    m_btnFeatMat = makeMaterialAddButton();
    bindMaterialPicker(m_cmbFeatMat, m_btnFeatMat);
    QWidget* wFeatMat = makeRow({ m_cmbFeatMat, m_btnFeatMat });
    m_lbFeatFrac = new QLabel(QStringLiteral("体积分数/质量分数"), this);
    m_editFeatFrac = new QLineEdit(this);
    m_editFeatFrac->setText(QStringLiteral("请输入"));
    m_coatingWidgets = { lbFeatMat, wFeatMat, m_lbFeatFrac, m_editFeatFrac };
    m_lbFiberFrac1 = new QLabel(QStringLiteral("纤维簇质量分数"), this);
    m_editFiberFrac1 = new QLineEdit(this);
    m_editFiberFrac1->setPlaceholderText(QStringLiteral("请输入"));
    m_lbFiberFrac2 = new QLabel(QStringLiteral("纤维质量分数"), this);
    m_editFiberFrac2 = new QLineEdit(this);
    m_editFiberFrac2->setPlaceholderText(QStringLiteral("请输入"));
    QLabel* lbFeatAspect = new QLabel(QStringLiteral("长细比"), this);
    m_editFeatAspect = new QLineEdit(this);
    m_editFeatAspect->setText(QStringLiteral("请输入"));
    m_fiberWidgets = { m_lbFiberFrac1, m_editFiberFrac1, m_lbFiberFrac2, m_editFiberFrac2,
                       lbFeatAspect, m_editFeatAspect };
    // 样式统一：标签右对齐、下拉伸展
    for (QLabel* lb : { lbPhase, lbMat, lbRatio, lbShape, lbMethod, lbFixed, lbTensor, lbTensorFile,
                        lbFeatMat, m_lbFeatFrac, m_lbFiberFrac1, m_lbFiberFrac2, lbFeatAspect })
        lb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    for (QComboBox* cb : { m_cmbPhase, m_cmbMaterial, m_cmbRatio, m_cmbShape, m_cmbDistFunc,
                           m_cmbDefMethod, m_cmbFeatMat })
        cb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 布局
    QGridLayout* grid = new QGridLayout(this);
    grid->setContentsMargins(40, 20, 80, 20);
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(14);
    grid->setColumnStretch(1, 1);
    const int labelColumnWidth = qMax(
        m_lbFeatFrac->sizeHint().width(),
        qMax(m_lbFiberFrac1->sizeHint().width(), m_lbFiberFrac2->sizeHint().width()));
    grid->setColumnMinimumWidth(0, labelColumnWidth);
    int r = 0;
    grid->addWidget(title, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbPhase, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(wlbPhase, r, 1, 1, 1); r++;
    grid->addWidget(lbMat, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_wMaterial, r, 1, 1, 1); r++;
    grid->addWidget(lbRatio, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(wRatio, r, 1, 1, 1); r++;
    grid->addWidget(lbShape, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(wlbShape, r, 1, 1, 1); r++;
    grid->addWidget(m_lbAspect, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editAspect, r, 1, 1, 1); r++;
    grid->addWidget(m_lbDist, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbDistFunc, r, 1, 1, 1); r++;
    grid->addWidget(m_lbCount, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editDiscreteCount, r, 1, 1, 1); r++;
    auto spacerRow1 = makeRow();
    grid->addWidget(spacerRow1, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbDir, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbMethod, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(wlbMethod, r, 1, 1, 1); r++;
    grid->addWidget(lbFixed, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_wFixedAngle, r, 1, 1, 1); r++;
    grid->addWidget(lbTensor, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_wTensor, r, 1, 1, 1); r++;
    grid->addWidget(lbTensorFile, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_wTensorFile, r, 1, 1, 1); r++;
    auto spacerRow2 = makeRow();
    grid->addWidget(spacerRow2, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(m_chkAdvanced, r, 0, Qt::AlignRight | Qt::AlignVCenter); r++;
    grid->addWidget(wFeatureRadio, r, 1, 1, 2); r++;
    grid->addWidget(lbFeatMat, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(wFeatMat, r, 1); r++;
    grid->addWidget(m_lbFeatFrac, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editFeatFrac, r, 1); r++;
    grid->addWidget(m_lbFiberFrac1, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editFiberFrac1, r, 1); r++;
    grid->addWidget(m_lbFiberFrac2, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editFiberFrac2, r, 1); r++;
    grid->addWidget(lbFeatAspect, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editFeatAspect, r, 1); r++;
    addFormStretch(grid, r);
    // 联动
    connect(m_cmbShape, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InclusionPage::slot_shapeChanged);
    connect(m_cmbDefMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InclusionPage::slot_methodChanged);
    connect(m_cmbDefMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [lbFixed, lbTensor, lbTensorFile](int index) {
            lbFixed->setVisible(index == 1);
            lbTensor->setVisible(index == 2);
            lbTensorFile->setVisible(index == 3);
        });
    // 初始为“无”，与构造末尾 slot_methodChanged(0) 对应，标签先隐藏
    lbFixed->hide();
    lbTensor->hide();
    lbTensorFile->hide();

    connect(m_cmbRatio, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InclusionPage::slot_ratioChanged);
    connect(m_chkAdvanced, &QCheckBox::toggled, this, &InclusionPage::slot_advancedToggled);
    connect(m_radioCoating, &QRadioButton::toggled, this, [this](bool on) {
        if (on) {
            for (QWidget* widget : m_fiberWidgets) widget->setVisible(false);
            for (QWidget* widget : m_coatingWidgets) widget->setVisible(true);
        }
        });
    connect(m_radioFiber, &QRadioButton::toggled, this, [this](bool on) {
        if (on) {
            for (QWidget* widget : m_coatingWidgets) widget->setVisible(false);
            for (QWidget* widget : m_fiberWidgets) widget->setVisible(true);
        }
        });
    connect(m_btnView, &QPushButton::clicked, this, &InclusionPage::slot_viewTensor);
    // —— 失焦即时数值校验：单字段不合法先清空该框 ——
    bindFractionEdit(m_editRatio, QStringLiteral("比例必须是0到1之间的数"));
    bindFractionEdit(m_editFeatFrac, QStringLiteral("涂层分数必须是0到1之间的数"));
    bindFractionEdit(m_editFiberFrac1, QStringLiteral("纤维簇分数必须是0到1之间的数"));
    bindFractionEdit(m_editFiberFrac2, QStringLiteral("纤维分数必须是0到1之间的数"));
    bindPositiveEdit(m_editAspect, QStringLiteral("长细比必须是大于0的数"));
    bindPositiveEdit(m_editFeatAspect, QStringLiteral("高级特征定义中的长细比必须是大于0的数"));
    // 分数总和 ≤1：主比例 +（勾选高级特征时）涂层分数 或 纤维簇+纤维分数
    bindFractionSum(m_editRatio,
        { m_editFeatFrac, m_editFiberFrac1, m_editFiberFrac2 },
        [this]() -> QList<QLineEdit*> {
            if (!m_chkAdvanced->isChecked()) return {};
            if (m_radioCoating->isChecked()) return { m_editFeatFrac };
            return { m_editFiberFrac1, m_editFiberFrac2 };
        },
        QStringLiteral("比例与高级特征定义中的分数总和不能超过1（当前为%1）"));
    // 长细比 3 倍规则（仅纤维族）：任一长细比框失焦即重算，不满足清空“刚编辑”的那个框
    auto aspectCheck = [this](QLineEdit* just) {
        if (!(m_chkAdvanced->isChecked() && m_radioFiber->isChecked())) return;
        bool okA = false, okFA = false;
        const double a = m_editAspect->text().trimmed().toDouble(&okA);
        const double fa = m_editFeatAspect->text().trimmed().toDouble(&okFA);
        if (okA && okFA && a + 1e-9 < 3.0 * fa)
        {
            outputWarn(m_pCurData ? m_pCurData->name : QStringLiteral("掺杂"),
                QStringLiteral("定义中的长细比必须是高级特征定义中长细比的3倍或以上"));
            just->clear();
        }
        };
    connect(m_editAspect, &QLineEdit::editingFinished, this, [this, aspectCheck] { aspectCheck(m_editAspect); });
    connect(m_editFeatAspect, &QLineEdit::editingFinished, this, [this, aspectCheck] { aspectCheck(m_editFeatAspect); });
    // 初始显示状态
    m_radioCoating->setChecked(true);
    slot_shapeChanged(0);
    slot_methodChanged(0);
    slot_ratioChanged(0);
    slot_advancedToggled(false);
}

void InclusionPage::slot_shapeChanged(int index)
{
    bool fixed = (index == 0);                    // 固定长细比
    m_lbAspect->setVisible(fixed);
    m_editAspect->setVisible(fixed);
    m_lbDist->setVisible(!fixed);
    m_cmbDistFunc->setVisible(!fixed);
    m_lbCount->setVisible(!fixed);
    m_editDiscreteCount->setVisible(!fixed);
}

void InclusionPage::slot_methodChanged(int index)
{
    m_wFixedAngle->setVisible(index == 1);
    m_wTensor->setVisible(index == 2);
    m_wTensorFile->setVisible(index == 3);
}

void InclusionPage::slot_ratioChanged(int index)
{
    // 高级特征定义中的分数标签随顶部“比例”统一在质量分数 / 体积分数间切换
    const QString frac = index == 0 ? QStringLiteral("质量分数") : QStringLiteral("体积分数");
    m_lbFeatFrac->setText(frac);                               // 涂层
    m_lbFiberFrac1->setText(QStringLiteral("纤维簇") + frac);  // 纤维簇
    m_lbFiberFrac2->setText(QStringLiteral("纤维") + frac);    // 纤维
}

void InclusionPage::slot_advancedToggled(bool on)
{
    // 未勾选时控件仍显示，只是不可编辑
    m_radioCoating->setEnabled(on);
    m_radioFiber->setEnabled(on);
    for (QWidget* widget : m_coatingWidgets) widget->setEnabled(on);
    for (QWidget* widget : m_fiberWidgets) widget->setEnabled(on);
}

void InclusionPage::slot_viewTensor()
{
    const QString result = showTensor6Dialog(m_editTensor->text());
    if (result != m_editTensor->text())
    {
        m_editTensor->setText(result);             // 触发 textChanged -> 自动存回 MechFrpData::mFrpPhaseData::inc.tensor
        notifyChanged();
    }
}

void InclusionPage::loadData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpPhaseData* p = data ? data->as<MechFrpData::mFrpPhaseData>() : nullptr;
    const MechFrpData::mFrpPhaseData::Inc d = p ? p->inc : MechFrpData::mFrpPhaseData::Inc();
    m_pCurData = data;
    m_pPendingMaterialCombo = nullptr;
    m_cmbPhase->setCurrentIndex(p ? switchPhaseComboBox(p->type) : -1);
    refillMaterialCombos();
    selectMaterial(m_cmbMaterial, d.material);
    m_cmbRatio->setCurrentIndex(d.ratioKind);
    m_editRatio->setText(d.ratioValue);
    m_cmbShape->setCurrentIndex(d.shape);
    m_editAspect->setText(d.aspect);
    refillDistFuncCombo(m_cmbDistFunc, d.distFuncName);
    m_editDiscreteCount->setText(d.discreteCount);
    m_cmbDefMethod->setCurrentIndex(d.defMethod);
    m_editTheta->setText(d.theta);
    m_editPhi->setText(d.phi);
    m_editTensor->setText(d.tensor.isEmpty() ? QStringLiteral("0;0;0;0;0;0;") : d.tensor);
    m_editTensorFile->setText(d.tensorFilePath);
    m_chkAdvanced->setChecked(d.advanced);
    m_radioCoating->setChecked(d.featureType == 0);
    m_radioFiber->setChecked(d.featureType == 1);   // 补纤维族反显，否则两个单选都不选中
    selectMaterial(m_cmbFeatMat, d.featureMat);
    m_editFeatFrac->setText(d.featureFrac);
    m_editFiberFrac1->setText(d.fiberFrac1);
    m_editFiberFrac2->setText(d.fiberFrac2);
    m_editFeatAspect->setText(d.featureAspect);
}

void InclusionPage::saveData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpPhaseData* p = data ? data->as<MechFrpData::mFrpPhaseData>() : nullptr;
    if (!p) return;
    MechFrpData::mFrpPhaseData::Inc& d = p->inc;
    d.material = materialName(m_cmbMaterial);
    d.ratioKind = m_cmbRatio->currentIndex();
    d.ratioValue = m_editRatio->text();
    d.shape = m_cmbShape->currentIndex();
    d.aspect = m_editAspect->text();
    d.distFuncName = distFuncNameOf(m_cmbDistFunc);
    d.discreteCount = m_editDiscreteCount->text();
    d.defMethod = m_cmbDefMethod->currentIndex();
    d.theta = m_editTheta->text();
    d.phi = m_editPhi->text();
    d.tensor = m_editTensor->text();
    d.tensorFilePath = m_editTensorFile->text();
    d.advanced = m_chkAdvanced->isChecked();
    d.featureType = m_radioCoating->isChecked() ? 0 : 1;
    d.featureMat = materialName(m_cmbFeatMat);
    d.featureFrac = m_editFeatFrac->text();
    d.fiberFrac1 = m_editFiberFrac1->text();
    d.fiberFrac2 = m_editFiberFrac2->text();
    d.featureAspect = m_editFeatAspect->text();
}

/************************************************** 连续纤维 ******************************************************/
FiberPage::FiberPage(QWidget* parent)
    : mFrpCompPropertyPage(parent)
{
    QLabel* title = makeTitle(QStringLiteral("定义"));
    // 相类型（只读）
    QLabel* lbPhase = new QLabel(QStringLiteral("相类型"), this);
    m_cmbPhase = createPhaseCombo(3);
    m_cmbPhase->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 材料
    QLabel* lbMat = new QLabel(QStringLiteral("材料"), this);
    m_cmbMaterial = new QComboBox(this);
    m_cmbMaterial->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_btnMaterial = makeMaterialAddButton();
    bindMaterialPicker(m_cmbMaterial, m_btnMaterial);
    QWidget* wMat = makeRow({ m_cmbMaterial, m_btnMaterial });
    // 比例
    QLabel* lbRatio = new QLabel(QStringLiteral("比例"), this);
    m_cmbRatio = new QComboBox(this);
    m_cmbRatio->addItems({ QStringLiteral("质量分数"), QStringLiteral("体积分数") });
    m_cmbRatio->setCurrentIndex(0);
    m_cmbRatio->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_editRatio = new QLineEdit(this);
    m_editRatio->setText(QStringLiteral("请输入范围0-1"));
    QWidget* wRatio = makeRow({ m_cmbRatio, m_editRatio });
    // 纤维方向
    QLabel* lbDir = makeTitle(QStringLiteral("纤维方向"));
    QLabel* lbFixed = new QLabel(QStringLiteral("固定角度"), this);
    QLabel* lbTheta = new QLabel(QStringLiteral("Theta"), this);
    lbTheta->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_editTheta = new QLineEdit(this);
    m_editTheta->setText(QStringLiteral("请输入"));
    QLabel* lbPhi = new QLabel(QStringLiteral("Phi"), this);
    lbPhi->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_editPhi = new QLineEdit(this);
    m_editPhi->setText(QStringLiteral("请输入"));
    QWidget* wFixedAngle = makeRow({ lbTheta, m_editTheta, lbPhi, m_editPhi });
    // 纤维涂层定义（默认不勾选，控件显示但禁用）
    m_chkCoating = new QCheckBox(QStringLiteral("纤维涂层定义"), this);
    QFont coatF = m_chkCoating->font(); coatF.setBold(true); m_chkCoating->setFont(coatF);
    m_chkCoating->setChecked(false);
    QLabel* lbCoatMat = new QLabel(QStringLiteral("材料"), this);
    m_cmbCoatMat = new QComboBox(this);
    m_cmbCoatMat->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_btnCoatMat = makeMaterialAddButton();
    bindMaterialPicker(m_cmbCoatMat, m_btnCoatMat);
    m_wCoatMat = makeRow({ m_cmbCoatMat, m_btnCoatMat });
    QLabel* lbCoatFrac = new QLabel(QStringLiteral("质量分数"), this);
    m_editCoatFrac = new QLineEdit(this);
    m_editCoatFrac->setText(QStringLiteral("请输入"));
    m_coatingWidgets = { lbCoatMat, m_wCoatMat, lbCoatFrac, m_editCoatFrac };
    // 样式统一：标签右对齐
    for (QLabel* lb : { lbPhase, lbMat, lbRatio, lbFixed, lbCoatMat, lbCoatFrac })
        lb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    // 布局
    QGridLayout* grid = makeFormGrid();
    int r = 0;
    grid->addWidget(title, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbPhase, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbPhase, r, 1); r++;
    grid->addWidget(lbMat, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(wMat, r, 1); r++;
    grid->addWidget(lbRatio, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(wRatio, r, 1); r++;
    auto temp = makeRow();
    grid->addWidget(temp, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbDir, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbFixed, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(wFixedAngle, r, 1); r++;
    auto spacerRow1 = makeRow();
    grid->addWidget(spacerRow1, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(m_chkCoating, r, 0, 1, 2); r++;
    grid->addWidget(lbCoatMat, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_wCoatMat, r, 1); r++;
    grid->addWidget(lbCoatFrac, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editCoatFrac, r, 1); r++;
    addFormStretch(grid, r);
    // 未勾选时显示但禁用
    connect(m_chkCoating, &QCheckBox::toggled, this, &FiberPage::slot_coatingToggled);
    // 比例（质量/体积分数）与纤维涂层定义的分数标签联动，两者保持一致
    connect(m_cmbRatio, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [lbCoatFrac](int index) {
            lbCoatFrac->setText(index == 0 ? QStringLiteral("质量分数") : QStringLiteral("体积分数"));
        });
    // —— 失焦即时数值校验：单字段不合法先清空该框 ——
    bindFractionEdit(m_editRatio, QStringLiteral("比例必须是0到1之间的数"));
    bindFractionEdit(m_editCoatFrac, QStringLiteral("纤维涂层分数必须是0到1之间的数"));
    // 主比例 + 涂层分数 ≤1
    bindFractionSum(m_editRatio,
        { m_editCoatFrac },
        [this]() -> QList<QLineEdit*> {
            return m_chkCoating->isChecked() ? QList<QLineEdit*>{ m_editCoatFrac } : QList<QLineEdit*>();
        },
        QStringLiteral("比例与纤维涂层定义中的分数总和不能超过1（当前为%1）"));
    slot_coatingToggled(false);
}

void FiberPage::loadData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpPhaseData* p = data ? data->as<MechFrpData::mFrpPhaseData>() : nullptr;
    const MechFrpData::mFrpPhaseData::Fib d = p ? p->fib : MechFrpData::mFrpPhaseData::Fib();
    m_pCurData = data;
    m_pPendingMaterialCombo = nullptr;
    m_cmbPhase->setCurrentIndex(p ? switchPhaseComboBox(p->type) : -1);
    refillMaterialCombos();
    selectMaterial(m_cmbMaterial, d.material);
    m_cmbRatio->setCurrentIndex(d.ratioKind);
    m_editRatio->setText(d.ratioValue);
    m_editTheta->setText(d.theta);
    m_editPhi->setText(d.phi);
    m_chkCoating->setChecked(d.coating);
    selectMaterial(m_cmbCoatMat, d.coatMat);
    m_editCoatFrac->setText(d.coatFrac);
    slot_coatingToggled(m_chkCoating->isChecked());
}

void FiberPage::saveData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpPhaseData* p = data ? data->as<MechFrpData::mFrpPhaseData>() : nullptr;
    if (!p) return;
    MechFrpData::mFrpPhaseData::Fib& d = p->fib;
    d.material = materialName(m_cmbMaterial);
    d.ratioKind = m_cmbRatio->currentIndex();
    d.ratioValue = m_editRatio->text();
    d.theta = m_editTheta->text();
    d.phi = m_editPhi->text();
    d.coating = m_chkCoating->isChecked();
    d.coatMat = materialName(m_cmbCoatMat);
    d.coatFrac = m_editCoatFrac->text();
}

void FiberPage::slot_coatingToggled(bool checked)
{
    for (QWidget* widget : m_coatingWidgets)
        widget->setEnabled(checked);
}
/************************************************** 空腔 ******************************************************/
CavityPage::CavityPage(QWidget* parent)
    : mFrpCompPropertyPage(parent)
{
    QLabel* title = makeTitle(QStringLiteral("定义"));
    // 相类型
    QLabel* lbPhase = new QLabel(QStringLiteral("相类型"), this);
    m_cmbPhase = createPhaseCombo(2);
    m_cmbPhase->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 体积分数（无比例下拉，直接体积分数）
    QLabel* lbFrac = new QLabel(QStringLiteral("体积分数"), this);
    m_editVolumeFraction = new QLineEdit(this);
    m_editVolumeFraction->setText(QStringLiteral("请输入范围0-1之间"));
    // 掺杂形状
    QLabel* lbShape = new QLabel(QStringLiteral("掺杂形状"), this);
    m_cmbShape = new QComboBox(this);
    m_cmbShape->addItems({ QStringLiteral("固定长细比"), QStringLiteral("离散长细比") });
    m_cmbShape->setCurrentIndex(0);
    m_cmbShape->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_lbAspect = new QLabel(QStringLiteral("长细比"), this);
    m_lbAspect->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_editAspect = new QLineEdit(this);
    m_editAspect->setText(QStringLiteral("请输入"));
    m_lbDist = new QLabel(QStringLiteral("分布函数"), this);
    m_lbDist->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_cmbDistFunc = new QComboBox(this);
    m_cmbDistFunc->addItem(QStringLiteral("无"));
    m_cmbDistFunc->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_lbCount = new QLabel(QStringLiteral("离散数量"), this);
    m_lbCount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_editDiscreteCount = new QLineEdit(this);
    m_editDiscreteCount->setText(QStringLiteral("请输入整数"));
    // 掺杂方向
    QLabel* lbDir = makeTitle(QStringLiteral("掺杂方向"));
    QLabel* lbMethod = new QLabel(QStringLiteral("定义方式"), this);
    m_cmbDefMethod = new QComboBox(this);
    m_cmbDefMethod->addItems({ QStringLiteral("无"), QStringLiteral("固定角度"),
                               QStringLiteral("固定取向张量"), QStringLiteral("取向张量文件"),
                               QStringLiteral("2D随机"), QStringLiteral("3D随机") });
    m_cmbDefMethod->setCurrentIndex(0);
    m_cmbDefMethod->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QLabel* lbFixed = new QLabel(QStringLiteral("固定角度"), this);
    lbFixed->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QLabel* lbTheta = new QLabel(QStringLiteral("Theta"), this);
    lbTheta->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_editTheta = new QLineEdit(this);
    m_editTheta->setText(QStringLiteral("请输入"));
    QLabel* lbPhi = new QLabel(QStringLiteral("Phi"), this);
    lbPhi->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_editPhi = new QLineEdit(this);
    m_editPhi->setText(QStringLiteral("请输入"));
    m_wFixedAngle = makeRow({ lbTheta, m_editTheta, lbPhi, m_editPhi });
    // 固定取向张量（与掺杂页一致：只读主框 + “查看”三角矩阵弹窗）
    QLabel* lbTensor = new QLabel(QStringLiteral("取向张量"), this);
    lbTensor->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_editTensor = new QLineEdit(this);
    m_editTensor->setText(QStringLiteral("0;0;0;0;0;0;"));
    m_editTensor->setReadOnly(true);
    m_btnView = new QPushButton(QStringLiteral("查看"), this);
    m_wTensor = makeRow({ m_editTensor, m_btnView });
    // 取向张量文件
    QLabel* lbTensorFile = new QLabel(QStringLiteral("张量文件"), this);
    lbTensorFile->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_editTensorFile = new QLineEdit(this);
    m_editTensorFile->setReadOnly(true);
    m_wTensorFile = makeRow({ lbTensorFile, m_editTensorFile, makeTensorFileButton(m_editTensorFile) });
    // 空腔表层定义
    m_chkSurface = new QCheckBox(QStringLiteral("空腔表层定义"), this);
    QFont sf = m_chkSurface->font(); sf.setBold(true); m_chkSurface->setFont(sf);
    m_chkSurface->setChecked(false);
    QLabel* lbMat = new QLabel(QStringLiteral("材料"), this);
    m_cmbSurfaceMat = new QComboBox(this);
    m_cmbSurfaceMat->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_btnSurfaceMat = makeMaterialAddButton();
    bindMaterialPicker(m_cmbSurfaceMat, m_btnSurfaceMat);
    m_wSurfaceMat = makeRow({ m_cmbSurfaceMat, m_btnSurfaceMat });
    QLabel* lbSurfFrac = new QLabel(QStringLiteral("体积分数"), this);
    m_editSurfaceFrac = new QLineEdit(this);
    m_editSurfaceFrac->setText(QStringLiteral("请输入范围0-1之间"));
    m_surfaceWidgets = { lbMat, m_wSurfaceMat, lbSurfFrac, m_editSurfaceFrac };
    // 样式统一：标签右对齐
    for (QLabel* lb : { lbPhase, lbFrac, lbShape, lbMethod, lbMat, lbSurfFrac })
        lb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    // 主布局
    QGridLayout* grid = makeFormGrid();
    int r = 0;
    grid->addWidget(title, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbPhase, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbPhase, r, 1); r++;
    grid->addWidget(lbFrac, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editVolumeFraction, r, 1); r++;
    grid->addWidget(lbShape, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbShape, r, 1); r++;
    grid->addWidget(m_lbAspect, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editAspect, r, 1); r++;
    grid->addWidget(m_lbDist, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbDistFunc, r, 1); r++;
    grid->addWidget(m_lbCount, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editDiscreteCount, r, 1); r++;
    auto spacerRow1 = makeRow();
    grid->addWidget(spacerRow1, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbDir, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbMethod, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_cmbDefMethod, r, 1); r++;
    grid->addWidget(lbFixed, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_wFixedAngle, r, 1, 1, 1); r++;
    grid->addWidget(lbTensor, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_wTensor, r, 1, 1, 1); r++;
    grid->addWidget(lbTensorFile, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_wTensorFile, r, 1, 1, 1); r++;
    auto spacerRow2 = makeRow();
    grid->addWidget(spacerRow2, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(m_chkSurface, r, 0, 1, 2); r++;
    grid->addWidget(lbMat, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_wSurfaceMat, r, 1); r++;
    grid->addWidget(lbSurfFrac, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(m_editSurfaceFrac, r, 1); r++;
    addFormStretch(grid, r);
    // 联动
    connect(m_cmbShape, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CavityPage::slot_shapeChanged);
    connect(m_cmbDefMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CavityPage::slot_methodChanged);
    // 标签移到标签列后，随定义方式与对应容器同步显隐
    connect(m_cmbDefMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [lbFixed, lbTensor, lbTensorFile](int index) {
            lbFixed->setVisible(index == 1);
            lbTensor->setVisible(index == 2);
            lbTensorFile->setVisible(index == 3);
        });
    // 初始为“无”，与构造末尾 slot_methodChanged(0) 对应，标签先隐藏
    lbFixed->hide();
    lbTensor->hide();
    lbTensorFile->hide();

    connect(m_chkSurface, &QCheckBox::toggled, this, &CavityPage::slot_surfaceToggled);
    connect(m_btnView, &QPushButton::clicked, this, &CavityPage::slot_viewTensor);
    // —— 失焦即时数值校验（切页不再重复检查）：单字段不合法先清空该框 ——
    bindFractionEdit(m_editVolumeFraction, QStringLiteral("体积分数必须是0到1之间的数"));
    bindFractionEdit(m_editSurfaceFrac, QStringLiteral("空腔表层体积分数必须是0到1之间的数"));
    bindPositiveEdit(m_editAspect, QStringLiteral("长细比必须是大于0的数"));
    // 体积分数总和 ≤1：主体积分数 +（勾选表层时）表层体积分数
    bindFractionSum(m_editVolumeFraction,
        { m_editSurfaceFrac },
        [this]() -> QList<QLineEdit*> {
            return m_chkSurface->isChecked() ? QList<QLineEdit*>{ m_editSurfaceFrac } : QList<QLineEdit*>();
        },
        QStringLiteral("这里的体积分数的和不能大于1（当前为%1）"));
    slot_shapeChanged(0);
    slot_methodChanged(0);
    slot_surfaceToggled(false);
}

void CavityPage::loadData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpPhaseData* p = data ? data->as<MechFrpData::mFrpPhaseData>() : nullptr;
    const MechFrpData::mFrpPhaseData::Cav d = p ? p->cav : MechFrpData::mFrpPhaseData::Cav();
    m_pCurData = data;
    m_pPendingMaterialCombo = nullptr;
    m_cmbPhase->setCurrentIndex(p ? switchPhaseComboBox(p->type) : -1);
    m_editVolumeFraction->setText(d.volumeFraction);
    m_cmbShape->setCurrentIndex(d.shape);
    m_editAspect->setText(d.aspect);
    refillDistFuncCombo(m_cmbDistFunc, d.distFuncName);
    m_editDiscreteCount->setText(d.discreteCount);
    m_cmbDefMethod->setCurrentIndex(d.defMethod);
    m_editTheta->setText(d.theta);
    m_editPhi->setText(d.phi);
    m_editTensor->setText(d.tensor.isEmpty() ? QStringLiteral("0;0;0;0;0;0;") : d.tensor);
    m_editTensorFile->setText(d.tensorFilePath);
    m_chkSurface->setChecked(d.surface);
    refillMaterialCombos();
    selectMaterial(m_cmbSurfaceMat, d.surfaceMat);
    m_editSurfaceFrac->setText(d.surfaceFrac);
    slot_shapeChanged(m_cmbShape->currentIndex());
    slot_methodChanged(m_cmbDefMethod->currentIndex());
    slot_surfaceToggled(m_chkSurface->isChecked());
}

void CavityPage::saveData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpPhaseData* p = data ? data->as<MechFrpData::mFrpPhaseData>() : nullptr;
    if (!p) return;
    MechFrpData::mFrpPhaseData::Cav& d = p->cav;
    d.volumeFraction = m_editVolumeFraction->text();
    d.shape = m_cmbShape->currentIndex();
    d.aspect = m_editAspect->text();
    d.distFuncName = distFuncNameOf(m_cmbDistFunc);
    d.discreteCount = m_editDiscreteCount->text();
    d.defMethod = m_cmbDefMethod->currentIndex();
    d.theta = m_editTheta->text();
    d.phi = m_editPhi->text();
    d.tensor = m_editTensor->text();
    d.tensorFilePath = m_editTensorFile->text();
    d.surface = m_chkSurface->isChecked();
    d.surfaceMat = materialName(m_cmbSurfaceMat);
    d.surfaceFrac = m_editSurfaceFrac->text();
}

void CavityPage::slot_shapeChanged(int index)
{
    const bool fixed = (index == 0);        // 固定长细比
    m_lbAspect->setVisible(fixed);
    m_editAspect->setVisible(fixed);
    m_lbDist->setVisible(!fixed);
    m_cmbDistFunc->setVisible(!fixed);
    m_lbCount->setVisible(!fixed);
    m_editDiscreteCount->setVisible(!fixed);
}

void CavityPage::slot_methodChanged(int index)
{
    m_wFixedAngle->setVisible(index == 1);   // 固定角度：Theta / Phi
    m_wTensor->setVisible(index == 2);       // 固定取向张量：只读张量 + 查看弹窗
    m_wTensorFile->setVisible(index == 3);   // 取向张量文件：张量文件下拉
}

void CavityPage::slot_viewTensor()
{
    const QString result = showTensor6Dialog(m_editTensor->text());
    if (result != m_editTensor->text())
    {
        m_editTensor->setText(result);       // 触发 textChanged -> 自动存回 MechFrpData::mFrpPhaseData::cav.tensor
        notifyChanged();
    }
}

void CavityPage::slot_surfaceToggled(bool checked)
{
    // 未勾选时控件仍显示，只是不可编辑
    for (QWidget* widget : m_surfaceWidgets)
        widget->setEnabled(checked);
}

/************************************************** RVE ******************************************************/
RvePage::RvePage(QWidget* parent)
    : mFrpCompPropertyPage(parent), m_editRow(-1)
{
    QLabel* title = makeTitle(QStringLiteral("定义"));
    // 分析方式
    QLabel* lbMode = new QLabel(QStringLiteral("分析方式"), this);
    m_cmbMode = new QComboBox(this);
    m_cmbMode->addItems({ QStringLiteral("单层分析"), QStringLiteral("多层分析") });
    m_cmbMode->setCurrentIndex(0);
    m_cmbMode->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto wlbMode = makeRow({ m_cmbMode }, true);
    // ========== 单层分析 ==========
    m_lbStruct = new QLabel(QStringLiteral("结构"), this);
    m_cmbStructure = new QComboBox(this);
    auto wlbStruct = makeRow({ m_cmbStructure }, true);
    m_cmbStructure->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // ========== 多层分析 ==========
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    QStringList headers;
    headers << QStringLiteral("微观结构")
        << QStringLiteral("厚度")
        << QStringLiteral("角度")
        << QStringLiteral("纤维取向")
        << QString();
    m_table->setHorizontalHeaderLabels(headers);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 85);
    m_table->setColumnWidth(3, 105);
    m_table->setColumnWidth(4, 120);
    m_btnAdd = new QPushButton(); m_btnAdd->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); m_btnAdd->setToolTip("增加");
    connect(m_btnAdd, &QPushButton::clicked, this, [this]() {
        addRow(m_table->currentRow() < 0 ? m_table->rowCount() : m_table->currentRow() + 1);
        if (!isLoading()) notifyChanged();
        });
    m_btnRemove = new QPushButton(); m_btnRemove->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); m_btnRemove->setToolTip("减少");
    connect(m_btnRemove, &QPushButton::clicked, this, [this]() {
        removeSelectedOrLastRow(m_table);            // 删选中行；无选中删最后一行；空表无操作
        if (!isLoading()) notifyChanged();
        });
    m_btnAdd->setMaximumWidth(60);
    m_btnRemove->setMaximumWidth(60);
    m_wbtn = makeRow({ m_btnAdd, m_btnRemove }, true);
    const int maxH = 15 * m_table->verticalHeader()->defaultSectionSize() + m_table->horizontalHeader()->height() + m_table->frameWidth() * 2;
    m_table->setFixedHeight(maxH);
    m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(QStringLiteral(
        "QHeaderView::section {background - color: #F7F8FA;color: #2c3e50;font - weight: normal;padding: 4px;border: 1px solid #dcdcdc;}"
        "QTableWidget { gridline-color:#d0e6f7; alternate-background-color:#ECF5FF;"
        " selection-background-color:#add8e6; selection-color:black; }"
        "QTableWidget::item { background:transparent; }"
        "QTableWidget:disabled { background:#F2F2F2; }"
        "QTableWidget::item:disabled { color:#9A9A9A; }"
        "QTableWidget QLineEdit { border:none; background:transparent; }"
        "QTableWidget QComboBox { border:1px solid #b8b8b8; border-radius:0px; padding:2px; margin:2px; }"
        "QTableWidget QComboBox::drop-down { border:none; width:18px; }"
        "QTableWidget QComboBox QAbstractItemView { border:1px solid #b8b8b8; outline:none; }"
        "QTableWidget QPushButton { border:1px solid #b8b8b8; border-radius:0px; background:#ffffff; padding:2px; margin:2px; }"
        "QTableWidget QPushButton:disabled { background-color:#E6E6E6; color:#9A9A9A; border:1px solid #C8C8C8; }"
        "QTableWidget QLineEdit:disabled, QTableWidget QComboBox:disabled { background-color:#F3F3F3; color:#888; }"));

    // 样式统一：标签右对齐
    lbMode->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lbStruct->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    // 布局
    QGridLayout* grid = new QGridLayout(this);
    grid->setContentsMargins(24, 20, 24, 20);
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(10);
    grid->setColumnStretch(1, 1);
    int r = 0;
    grid->addWidget(title, r++, 0, 1, 3);
    grid->addWidget(lbMode, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(wlbMode, r, 1, 1, 1); r++;
    grid->addWidget(m_lbStruct, r, 0, Qt::AlignRight | Qt::AlignVCenter); grid->addWidget(wlbStruct, r, 1, 1, 1); r++;
    grid->addWidget(m_wbtn, r, 0, 1, 2); r++;
    grid->addWidget(m_table, r, 0, 1, 3); r++;
    grid->setColumnStretch(1, 1);
    grid->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), r, 0, 1, 3);
    // 创建弹窗
    createAngleDialog();
    createTensorDialog();
    // 联动
    connect(m_cmbMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        const bool single = (index == 0);
        m_table->setVisible(!single);
        m_lbStruct->setVisible(single);
        m_cmbStructure->setVisible(single);
        m_wbtn->setVisible(!single);
        if (!isLoading()) notifyChanged();
        });
    // 初始单层分析：addItems 后 currentIndex 已为 0 且不会再发信号，这里显式设置初始显隐
    m_table->setVisible(false);
    m_wbtn->setVisible(false);          // 单层分析：隐藏多层表格与增删按钮行（“结构”行保持可见）
}

// 添加一层（init 非空时按保存值回填，用于反显）
// 纤维取向类型下拉索引：0 请选择 / 1 From Phase / 2 固定角度 / 3 Random 2D / 4 Random 3D / 5 固定取向张量
void RvePage::addRow(int row, const MechFrpData::RveLayer* init)
{
    if (row == -1)
        row = m_table->rowCount();
    m_table->insertRow(row);
    // 由单元格控件反推它当前所在行（中间插入/删除行后索引会变，不能捕获固定行号）
    auto rowOf = [this](QWidget* w) {
        return m_table->indexAt(w->mapTo(m_table->viewport(), QPoint(0, 0))).row();
        };
    // 列0：微观结构（下拉，选本均质化模型下的微观结构N；存名字）
    QComboBox* cmbMicro = new QComboBox(m_table);
    cmbMicro->addItem(QStringLiteral("无"));
    cmbMicro->addItems(microNames());
    m_table->setCellWidget(row, 0, cmbMicro);
    // 列1：厚度；列2：角度
    QLineEdit* editThick = new QLineEdit(m_table);
    m_table->setCellWidget(row, 1, editThick);
    QLineEdit* editAngle = new QLineEdit(m_table);
    m_table->setCellWidget(row, 2, editAngle);
    // 列3：纤维取向类型
    QComboBox* cmbOrient = new QComboBox(m_table);
    cmbOrient->addItems({
        QStringLiteral("无"),
        QStringLiteral("固定角度"),
        QStringLiteral("Random 2D"),
        QStringLiteral("Random 3D"),
        QStringLiteral("固定取向张量")
        });
    cmbOrient->setCurrentIndex(0);
    m_table->setCellWidget(row, 3, cmbOrient);
    // 列4：取向值（只读）+ 编辑；仅固定角度 / 固定取向张量时显示
    QWidget* wVal = new QWidget(m_table);
    QHBoxLayout* hVal = new QHBoxLayout(wVal);
    hVal->setContentsMargins(1, 1, 1, 1);
    hVal->setSpacing(1);
    QLineEdit* editValue = new QLineEdit(wVal);
    editValue->setReadOnly(true);
    QPushButton* btnEdit = new QPushButton(QStringLiteral("编辑"), wVal);
    btnEdit->setFixedWidth(32);
    hVal->addWidget(editValue);
    hVal->addWidget(btnEdit);
    m_table->setCellWidget(row, 4, wVal);
    // 取向类型联动：固定角度/固定取向张量才启用“编辑”并给默认串；其余禁用并清空。
    auto syncValueRow = [editValue, btnEdit](const QString& o) {
        const bool hasVal = (o == QStringLiteral("固定角度") || o == QStringLiteral("固定取向张量"));
        btnEdit->setEnabled(hasVal);
        const QVariant lastVar = editValue->property("lastOrientType");
        const bool firstRun = !lastVar.isValid();
        const QString lastType = lastVar.toString();
        if (hasVal)
        {
            // 真正切换类型时重置默认串；首次（反显）不覆盖已回填的保存值
            if ((!firstRun && lastType != o) || editValue->text().isEmpty())
                editValue->setText(o == QStringLiteral("固定角度")
                    ? QStringLiteral("0;0;") : QStringLiteral("0;0;0;0;0;0;"));
        }
        else
            editValue->clear();
        editValue->setProperty("lastOrientType", hasVal ? QVariant(o) : QVariant());
        };

    btnEdit->setEnabled(false);
    // 编辑触发整页自动保存（回填期间 isLoading 屏蔽）
    connect(editThick, &QLineEdit::textChanged, this, [this](const QString&) { if (!isLoading()) notifyChanged(); });
    connect(editAngle, &QLineEdit::textChanged, this, [this](const QString&) { if (!isLoading()) notifyChanged(); });
    // 改微观结构：重新判定该行纤维取向是否可编辑，不满足则重置
    connect(cmbMicro, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, rowOf, cmbMicro](int) {
        if (isLoading()) return;
        notifyChanged();
        const int rr = rowOf(cmbMicro);
        if (rr >= 0) applyOrientEditable(rr, true);
        });
    connect(cmbOrient, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, cmbOrient, syncValueRow](int) {
        if (isLoading()) return;
        notifyChanged();
        syncValueRow(cmbOrient->currentText());
        });
    // 编辑：先把该行当前值载入对应弹窗，模态打开；点保存时在 createXxxDialog 内写回本行
    connect(btnEdit, &QPushButton::clicked, this, [this, rowOf, wVal, cmbOrient, editValue]() {
        const int rr = rowOf(wVal);
        if (rr < 0) return;
        m_editRow = rr;
        const QString o = cmbOrient->currentText();
        const QStringList p = editValue->text().split(';');
        if (o == QStringLiteral("固定角度"))
        {
            m_editTheta->setText(p.value(0).trimmed().isEmpty() ? QStringLiteral("0") : p.value(0).trimmed());
            m_editPhi->setText(p.value(1).trimmed().isEmpty() ? QStringLiteral("0") : p.value(1).trimmed());
            m_dlgAngle->exec();
        }
        else if (o == QStringLiteral("固定取向张量"))
        {
            for (int i = 0; i < m_tensorEdits.size(); ++i)
            {
                const QString t = p.value(i).trimmed();
                m_tensorEdits[i]->setText(t.isEmpty() ? QStringLiteral("0") : t);
            }
            m_dlgTensor->exec();
        }
        });
    if (init)
    {
        const int mi = cmbMicro->findText(init->micro);
        cmbMicro->setCurrentIndex(mi >= 0 ? mi : 0);
        editThick->setText(init->thick);
        editAngle->setText(init->angle);
        const int oi = cmbOrient->findText(init->orient);   // 空串/未知返回 -1
        cmbOrient->setCurrentIndex(oi >= 0 ? oi : 0);       // 兜底到“无”
        editValue->setText(init->value);
    }
    applyOrientEditable(row, true);
    syncValueRow(cmbOrient->currentText());
}

// 本均质化模型下全部微观结构N 的名字（均质化模型 -> “微观结构”分组 -> 微观结构N）
QStringList RvePage::microNames()
{
    QStringList names;
    MechFrpData::mFrpBaseData* an = m_pCurData ? m_pCurData->ancestorOfType(FrpNode_Analysis1) : nullptr;
    if (!an) return names;
    for (MechFrpData::mFrpBaseData* c : an->children)
    {
        if (c->type != FrpNode_Micro) continue;       // “微观结构”分组
        for (MechFrpData::mFrpBaseData* ms : c->children)           // 微观结构N
            if (ms->type == FrpNode_Micro1) names << ms->name;
    }
    return names;
}
// 按名字找微观结构N
MechFrpData::mFrpBaseData* RvePage::findMicro(const QString& name)
{
    if (name.isEmpty() || name == QStringLiteral("无")) return nullptr;
    MechFrpData::mFrpBaseData* an = m_pCurData ? m_pCurData->ancestorOfType(FrpNode_Analysis1) : nullptr;
    if (!an) return nullptr;
    for (MechFrpData::mFrpBaseData* c : an->children)
    {
        if (c->type != FrpNode_Micro) continue;
        for (MechFrpData::mFrpBaseData* ms : c->children)
            if (ms->type == FrpNode_Micro1 && ms->name == name) return ms;
    }
    return nullptr;
}

// 纤维取向仅当该微观结构“恰好 1 基体 + 1 掺杂”（无空腔 / 连续纤维 / 第二个掺杂）时才可编辑
bool RvePage::orientEditable(const QString& name)
{
    MechFrpData::mFrpBaseData* ms = findMicro(name);
    if (!ms) return false;
    int nMatrix = 0, nInc = 0, nOther = 0;
    for (MechFrpData::mFrpBaseData* ph : ms->children)
    {
        if (ph->type == FrpNode_Matrix) ++nMatrix;
        else if (ph->type == FrpNode_Inclusion) ++nInc;
        else ++nOther;
    }
    return nMatrix == 1 && nInc == 1 && nOther == 0;
}

// 设置某行纤维取向的启用态；reset=true 且不满足条件时，重置为“请选择”并清空取值
void RvePage::applyOrientEditable(int row, bool reset)
{
    QComboBox* cmbMicro = qobject_cast<QComboBox*>(m_table->cellWidget(row, 0));
    QComboBox* cmbOrient = qobject_cast<QComboBox*>(m_table->cellWidget(row, 3));
    QWidget* wVal = m_table->cellWidget(row, 4);
    if (!cmbMicro || !cmbOrient) return;
    const bool ok = orientEditable(cmbMicro->currentText());
    cmbOrient->setEnabled(ok);
    if (!ok && reset && cmbOrient->currentIndex() != 0)
    {
        cmbOrient->blockSignals(true);
        cmbOrient->setCurrentIndex(0);
        cmbOrient->blockSignals(false);
        if (wVal)
        {
            if (QLineEdit* v = wVal->findChild<QLineEdit*>()) v->clear();
            if (QPushButton* b = wVal->findChild<QPushButton*>()) b->setEnabled(false);
        }
        notifyChanged();
    }
}

// 创建固定角度弹窗
void RvePage::createAngleDialog()
{
    m_dlgAngle = new QDialog(this);
    m_dlgAngle->setWindowTitle(QStringLiteral("固定角度"));
    m_dlgAngle->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    QGridLayout* g = new QGridLayout(m_dlgAngle);
    g->setHorizontalSpacing(24);
    g->setVerticalSpacing(16);
    QLabel* lbFixed = new QLabel(QStringLiteral("固定角度"), m_dlgAngle);
    QLabel* lbTheta = new QLabel(QStringLiteral("Theta"), m_dlgAngle);
    m_editTheta = new QLineEdit(QStringLiteral("0"), m_dlgAngle);
    m_editTheta->setFixedWidth(80);
    QLabel* lbPhi = new QLabel(QStringLiteral("Phi"), m_dlgAngle);
    m_editPhi = new QLineEdit(QStringLiteral("0"), m_dlgAngle);
    m_editPhi->setFixedWidth(80);
    QWidget* wAngleRow = makeRow({ lbTheta, m_editTheta, lbPhi, m_editPhi });
    QPushButton* btnCancel = new QPushButton(QStringLiteral("取消"), m_dlgAngle);
    QPushButton* btnSave = new QPushButton(QStringLiteral("保存"), m_dlgAngle);
    btnCancel->setAutoDefault(false);
    btnSave->setAutoDefault(false);
    btnSave->setDefault(false);
    g->addWidget(lbFixed, 0, 0, 1, 1);
    g->addWidget(wAngleRow, 0, 1, 1, 3);
    g->addWidget(btnCancel, 1, 2);
    g->addWidget(btnSave, 1, 3);
    connect(btnCancel, &QPushButton::clicked, m_dlgAngle, &QDialog::reject);
    // 保存：归一化后按 “Theta;Phi;” 写回当前编辑行（m_editRow）的只读值框
    connect(btnSave, &QPushButton::clicked, this, [this]() {
        auto norm = [](QLineEdit* e) { bool ok = false; const QString t = e->text().trimmed(); t.toDouble(&ok); return ok ? t : QStringLiteral("0"); };
        if (m_editRow >= 0 && m_editRow < m_table->rowCount())
            if (QWidget* wVal = m_table->cellWidget(m_editRow, 4))
                if (QLineEdit* v = wVal->findChild<QLineEdit*>())
                    v->setText(norm(m_editTheta) + ';' + norm(m_editPhi) + ';');
        notifyChanged();
        m_dlgAngle->accept();
        });
}

// 创建取向张量弹窗（三角形布局）
void RvePage::createTensorDialog()
{
    m_dlgTensor = new QDialog(this);
    m_dlgTensor->setWindowTitle(QStringLiteral("固定取向张量"));
    m_dlgTensor->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    QGridLayout* g = new QGridLayout(m_dlgTensor);
    g->setHorizontalSpacing(24);
    g->setVerticalSpacing(16);
    const QStringList labels = {
        QStringLiteral("a[1,1]"), QStringLiteral("a[1,2]"), QStringLiteral("a[1,3]"),
        QStringLiteral("a[2,2]"), QStringLiteral("a[2,3]"),
        QStringLiteral("a[3,3]")
    };
    const int rowPos[6] = { 0, 0, 0, 1, 1, 2 };
    const int colPos[6] = { 0, 2, 4, 2, 4, 4 };
    m_tensorEdits.clear();
    for (int i = 0; i < 6; ++i) {
        QLabel* label = new QLabel(labels[i], m_dlgTensor);
        QLineEdit* e = new QLineEdit(QStringLiteral("0"), m_dlgTensor);
        e->setFixedWidth(70);
        QWidget* wValueRow = makeRow({ label, e });
        g->addWidget(wValueRow, rowPos[i], colPos[i], 1, 2);
        m_tensorEdits.append(e);
    }
    QPushButton* btnCancel = new QPushButton(QStringLiteral("取消"), m_dlgTensor);
    QPushButton* btnSave = new QPushButton(QStringLiteral("保存"), m_dlgTensor);
    btnCancel->setAutoDefault(false);
    btnSave->setAutoDefault(false);
    btnSave->setDefault(false);
    g->addWidget(btnCancel, 3, 3);
    g->addWidget(btnSave, 3, 4);
    connect(btnCancel, &QPushButton::clicked, m_dlgTensor, &QDialog::reject);
    // 保存：6 分量归一化后按 “a11;a12;a13;a22;a23;a33;” 写回当前编辑行（m_editRow）的只读值框
    connect(btnSave, &QPushButton::clicked, this, [this]() {
        if (m_editRow >= 0 && m_editRow < m_table->rowCount())
            if (QWidget* wVal = m_table->cellWidget(m_editRow, 4))
                if (QLineEdit* v = wVal->findChild<QLineEdit*>())
                {
                    QStringList vals;
                    for (QLineEdit* e : m_tensorEdits)
                    {
                        bool ok = false;
                        const QString t = e->text().trimmed();
                        t.toDouble(&ok);
                        vals << (ok ? t : QStringLiteral("0"));
                    }
                    v->setText(vals.join(';') + ';');
                }
        notifyChanged();
        m_dlgTensor->accept();
        });
}

void RvePage::loadData(MechFrpData::mFrpBaseData* data)
{
    m_pCurData = data;        // 页面基类当前节点（microNames / orientEditable 都依赖它，必须先赋值）
    MechFrpData::mFrpRveData* d = data ? data->as<MechFrpData::mFrpRveData>() : nullptr;
    // 分析方式（setCurrentIndex 会触发模式联动恢复单/多层显隐）
    m_cmbMode->setCurrentText(d ? d->mode : QString());
    // 单层“结构”：动态列出本模型下的微观结构N，空引用或失效引用保持未选中
    m_cmbStructure->blockSignals(true);
    m_cmbStructure->clear();
    m_cmbStructure->addItems(microNames());
    const int si = m_cmbStructure->findText(d ? d->structure : QString());
    m_cmbStructure->setCurrentIndex(si);
    m_cmbStructure->blockSignals(false);
    // 多层表格：清空数据行后逐层重建（addRow 内部按层数据回填）
    m_table->setRowCount(0);
    if (d)
    {
        for (const MechFrpData::RveLayer& l : d->layers)
            addRow(-1, &l);
    }
}

void RvePage::saveData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpRveData* d = data ? data->as<MechFrpData::mFrpRveData>() : nullptr;
    if (!d) return;
    d->mode = m_cmbMode->currentText();
    d->structure = m_cmbStructure->currentIndex() < 0 ? QString() : m_cmbStructure->currentText();
    d->layers.clear();
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        MechFrpData::RveLayer l;
        // 列0：微观结构下拉（首项“请选择”记为空串）
        if (QComboBox* mc = qobject_cast<QComboBox*>(m_table->cellWidget(row, 0)))
            l.micro = (mc->currentIndex() == 0) ? QString() : mc->currentText();
        // 列1厚度 / 列2角度：文本框
        auto lineText = [this, row](int c) -> QString {
            if (QLineEdit* e = qobject_cast<QLineEdit*>(m_table->cellWidget(row, c)))
                return e->text();
            return QString();
            };
        l.thick = lineText(1);
        l.angle = lineText(2);
        // 列3：纤维取向类型；列4：只读取向值
        if (QComboBox* cb = qobject_cast<QComboBox*>(m_table->cellWidget(row, 3)))
            l.orient = cb->currentText();
        if (QWidget* wVal = m_table->cellWidget(row, 4))
            if (QLineEdit* e = wVal->findChild<QLineEdit*>())
                l.value = e->text();
        d->layers.append(l);
    }
}

/************************************************** 失效准则 ******************************************************/
FailurePage::FailurePage(QWidget* parent)
    : mFrpCompPropertyPage(parent)
{
    QLabel* title = makeTitle(QStringLiteral("定义"));
    // 模型
    QLabel* lbModel = new QLabel(QStringLiteral("模型"), this);
    m_cmbModel = new QComboBox(this);
    m_cmbModel->addItems({
        QStringLiteral("Maximum"),
        QStringLiteral("Tsai Hill 2D"),
        QStringLiteral("Tsai Hill 3D Transversely"),
        QStringLiteral("Tsai Hill 3D"),
        QStringLiteral("Azzi Tsai Hill 2D"),
        QStringLiteral("Tsai Wu 2D"),
        QStringLiteral("Tsai Wu 3D Transversely"),
        QStringLiteral("Tsai Wu 3D"),
        QStringLiteral("Multi Component 2D")
        });
    m_cmbModel->setCurrentIndex(0);          // Maximum
    // 类型
    m_lbType = new QLabel(QStringLiteral("类型"), this);
    m_cmbType = new QComboBox(this);
    m_cmbType->addItems({ QStringLiteral("Stress"), QStringLiteral("Strain") });
    m_cmbType->setCurrentIndex(0);           // Stress
    QLabel* lbParam = new QLabel(QStringLiteral("参数"), this);
    QFont pf = lbParam->font(); pf.setBold(true); lbParam->setFont(pf);
    // ===== Maximum 模型页：应力分量 + 抗拉/抗压强度 =====
    m_lbComp = new QLabel(QStringLiteral("应力分量"), this);
    m_cmbComp = new QComboBox(this);
    m_cmbComp->addItems({
        QStringLiteral("无"), QStringLiteral("11"), QStringLiteral("12"),
        QStringLiteral("13"), QStringLiteral("22"), QStringLiteral("23"),
        QStringLiteral("33")
        });
    m_lbTens = new QLabel(QStringLiteral("抗拉强度"), this);
    m_editTens = new QLineEdit(this);
    m_editTens->setText(QStringLiteral("请输入"));
    m_lbComp2 = new QLabel(QStringLiteral("抗压强度"), this);
    m_editComp2 = new QLineEdit(this);
    m_editComp2->setText(QStringLiteral("请输入"));
    QWidget* pageMax = new QWidget(this);
    QGridLayout* gMax = new QGridLayout(pageMax);
    gMax->setContentsMargins(0, 0, 0, 0);
    gMax->setHorizontalSpacing(24);
    gMax->setVerticalSpacing(14);
    gMax->addWidget(m_lbComp, 0, 0); gMax->addWidget(m_cmbComp, 0, 1);
    gMax->addWidget(m_lbTens, 1, 0); gMax->addWidget(m_editTens, 1, 1);
    gMax->addWidget(m_lbComp2, 2, 0); gMax->addWidget(m_editComp2, 2, 1);
    gMax->setColumnStretch(1, 1);
    gMax->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding),
        3, 0, 1, 2);
    gMax->setRowStretch(3, 1);
    // ===== Tsai Hill 2D 模型页：5 个强度框（Tsai Wu 3D Transversely 复用本页） =====
    m_lbAxialTens = new QLabel(QStringLiteral("轴向抗拉强度"), this);
    m_editAxialTens = new QLineEdit(this); m_editAxialTens->setText(QStringLiteral("请输入"));
    m_lbAxialComp = new QLabel(QStringLiteral("轴向抗压强度"), this);
    m_editAxialComp = new QLineEdit(this); m_editAxialComp->setText(QStringLiteral("请输入"));
    m_lbInPlaneTens = new QLabel(QStringLiteral("面内抗拉强度"), this);
    m_editInPlaneTens = new QLineEdit(this); m_editInPlaneTens->setText(QStringLiteral("请输入"));
    m_lbInPlaneComp = new QLabel(QStringLiteral("面内抗压强度"), this);
    m_editInPlaneComp = new QLineEdit(this); m_editInPlaneComp->setText(QStringLiteral("请输入"));
    m_lbShear = new QLabel(QStringLiteral("横向剪切强度"), this);
    m_editShear = new QLineEdit(this); m_editShear->setText(QStringLiteral("请输入"));
    QWidget* pageTsai = new QWidget(this);
    QGridLayout* gTsai = new QGridLayout(pageTsai);
    gTsai->setContentsMargins(0, 0, 0, 0);
    gTsai->setHorizontalSpacing(24);
    gTsai->setVerticalSpacing(14);
    gTsai->addWidget(m_lbAxialTens, 0, 0); gTsai->addWidget(m_editAxialTens, 0, 1);
    gTsai->addWidget(m_lbAxialComp, 1, 0); gTsai->addWidget(m_editAxialComp, 1, 1);
    gTsai->addWidget(m_lbInPlaneTens, 2, 0); gTsai->addWidget(m_editInPlaneTens, 2, 1);
    gTsai->addWidget(m_lbInPlaneComp, 3, 0); gTsai->addWidget(m_editInPlaneComp, 3, 1);
    gTsai->addWidget(m_lbShear, 4, 0); gTsai->addWidget(m_editShear, 4, 1);
    gTsai->setColumnStretch(1, 1);
    gTsai->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding),
        5, 0, 1, 2);
    gTsai->setRowStretch(5, 1);
    // ===== Tsai Hill 3D Transversely 模型页：3 个强度框 =====
    m_lbAxialTensT = new QLabel(QStringLiteral("轴向抗拉强度"), this);
    m_editAxialTensT = new QLineEdit(this); m_editAxialTensT->setText(QStringLiteral("请输入"));
    m_lbInPlaneTensT = new QLabel(QStringLiteral("面内抗拉强度"), this);
    m_editInPlaneTensT = new QLineEdit(this); m_editInPlaneTensT->setText(QStringLiteral("请输入"));
    m_lbShearT = new QLabel(QStringLiteral("横向剪切强度"), this);
    m_editShearT = new QLineEdit(this); m_editShearT->setText(QStringLiteral("请输入"));
    QWidget* pageTsai3DTrans = new QWidget(this);
    QGridLayout* gTsai3DT = new QGridLayout(pageTsai3DTrans);
    gTsai3DT->setContentsMargins(0, 0, 0, 0);
    gTsai3DT->setHorizontalSpacing(24);
    gTsai3DT->setVerticalSpacing(14);
    gTsai3DT->addWidget(m_lbAxialTensT, 0, 0); gTsai3DT->addWidget(m_editAxialTensT, 0, 1);
    gTsai3DT->addWidget(m_lbInPlaneTensT, 1, 0); gTsai3DT->addWidget(m_editInPlaneTensT, 1, 1);
    gTsai3DT->addWidget(m_lbShearT, 2, 0); gTsai3DT->addWidget(m_editShearT, 2, 1);
    gTsai3DT->setColumnStretch(1, 1);
    gTsai3DT->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding),
        3, 0, 1, 2);
    gTsai3DT->setRowStretch(3, 1);
    // ===== Tsai Hill 3D 模型页：6 个强度框 =====
    m_lbXt = new QLabel(QStringLiteral("X向抗拉强度"), this);
    m_editXt = new QLineEdit(this); m_editXt->setText(QStringLiteral("请输入"));
    m_lbYt = new QLabel(QStringLiteral("Y向抗拉强度"), this);
    m_editYt = new QLineEdit(this); m_editYt->setText(QStringLiteral("请输入"));
    m_lbZt = new QLabel(QStringLiteral("Z向抗拉强度"), this);
    m_editZt = new QLineEdit(this); m_editZt->setText(QStringLiteral("请输入"));
    m_lbXYs = new QLabel(QStringLiteral("XY向剪切强度"), this);
    m_editXYs = new QLineEdit(this); m_editXYs->setText(QStringLiteral("请输入"));
    m_lbYZs = new QLabel(QStringLiteral("YZ向剪切强度"), this);
    m_editYZs = new QLineEdit(this); m_editYZs->setText(QStringLiteral("请输入"));
    m_lbXZs = new QLabel(QStringLiteral("XZ向剪切强度"), this);
    m_editXZs = new QLineEdit(this); m_editXZs->setText(QStringLiteral("请输入"));
    QWidget* pageTsai3D = new QWidget(this);
    QGridLayout* gTsai3D = new QGridLayout(pageTsai3D);
    gTsai3D->setContentsMargins(0, 0, 0, 0);
    gTsai3D->setHorizontalSpacing(24);
    gTsai3D->setVerticalSpacing(14);
    gTsai3D->addWidget(m_lbXt, 0, 0); gTsai3D->addWidget(m_editXt, 0, 1);
    gTsai3D->addWidget(m_lbYt, 1, 0); gTsai3D->addWidget(m_editYt, 1, 1);
    gTsai3D->addWidget(m_lbZt, 2, 0); gTsai3D->addWidget(m_editZt, 2, 1);
    gTsai3D->addWidget(m_lbXYs, 3, 0); gTsai3D->addWidget(m_editXYs, 3, 1);
    gTsai3D->addWidget(m_lbYZs, 4, 0); gTsai3D->addWidget(m_editYZs, 4, 1);
    gTsai3D->addWidget(m_lbXZs, 5, 0); gTsai3D->addWidget(m_editXZs, 5, 1);
    gTsai3D->setColumnStretch(1, 1);
    gTsai3D->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding),
        6, 0, 1, 2);
    gTsai3D->setRowStretch(6, 1);
    // ===== Azzi Tsai Hill 2D 模型页：5 个强度框（Multi Component 2D 复用本页） =====
    m_lbAzAxTens = new QLabel(QStringLiteral("轴向抗拉强度"), this);
    m_editAzAxTens = new QLineEdit(this); m_editAzAxTens->setText(QStringLiteral("请输入"));
    m_lbAzAxComp = new QLabel(QStringLiteral("轴向抗压强度"), this);
    m_editAzAxComp = new QLineEdit(this); m_editAzAxComp->setText(QStringLiteral("请输入"));
    m_lbAzIpTens = new QLabel(QStringLiteral("面内抗拉强度"), this);
    m_editAzIpTens = new QLineEdit(this); m_editAzIpTens->setText(QStringLiteral("请输入"));
    m_lbAzIpComp = new QLabel(QStringLiteral("面内抗压强度"), this);
    m_editAzIpComp = new QLineEdit(this); m_editAzIpComp->setText(QStringLiteral("请输入"));
    m_lbAzShear = new QLabel(QStringLiteral("横向剪切强度"), this);
    m_editAzShear = new QLineEdit(this); m_editAzShear->setText(QStringLiteral("请输入"));
    QWidget* pageAzzi = new QWidget(this);
    QGridLayout* gAzzi = new QGridLayout(pageAzzi);
    gAzzi->setContentsMargins(0, 0, 0, 0);
    gAzzi->setHorizontalSpacing(24);
    gAzzi->setVerticalSpacing(14);
    gAzzi->addWidget(m_lbAzAxTens, 0, 0); gAzzi->addWidget(m_editAzAxTens, 0, 1);
    gAzzi->addWidget(m_lbAzAxComp, 1, 0); gAzzi->addWidget(m_editAzAxComp, 1, 1);
    gAzzi->addWidget(m_lbAzIpTens, 2, 0); gAzzi->addWidget(m_editAzIpTens, 2, 1);
    gAzzi->addWidget(m_lbAzIpComp, 3, 0); gAzzi->addWidget(m_editAzIpComp, 3, 1);
    gAzzi->addWidget(m_lbAzShear, 4, 0); gAzzi->addWidget(m_editAzShear, 4, 1);
    gAzzi->setColumnStretch(1, 1);
    gAzzi->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding),
        5, 0, 1, 2);
    gAzzi->setRowStretch(5, 1);
    // ===== Tsai Wu 2D 模型页：6 个强度框 =====
    m_lbWuAxTens = new QLabel(QStringLiteral("轴向抗拉强度"), this);
    m_editWuAxTens = new QLineEdit(this); m_editWuAxTens->setText(QStringLiteral("请输入"));
    m_lbWuAxComp = new QLabel(QStringLiteral("轴向抗压强度"), this);
    m_editWuAxComp = new QLineEdit(this); m_editWuAxComp->setText(QStringLiteral("请输入"));
    m_lbWuIpTens = new QLabel(QStringLiteral("面内抗拉强度"), this);
    m_editWuIpTens = new QLineEdit(this); m_editWuIpTens->setText(QStringLiteral("请输入"));
    m_lbWuIpComp = new QLabel(QStringLiteral("面内抗压强度"), this);
    m_editWuIpComp = new QLineEdit(this); m_editWuIpComp->setText(QStringLiteral("请输入"));
    m_lbWuShear = new QLabel(QStringLiteral("横向剪切强度"), this);
    m_editWuShear = new QLineEdit(this); m_editWuShear->setText(QStringLiteral("请输入"));
    m_lbWuCouple = new QLabel(QStringLiteral("轴向-平面耦合强度"), this);
    m_editWuCouple = new QLineEdit(this); m_editWuCouple->setText(QStringLiteral("请输入"));
    QWidget* pageWu2D = new QWidget(this);
    QGridLayout* gWu2D = new QGridLayout(pageWu2D);
    gWu2D->setContentsMargins(0, 0, 0, 0);
    gWu2D->setHorizontalSpacing(24);
    gWu2D->setVerticalSpacing(14);
    gWu2D->addWidget(m_lbWuAxTens, 0, 0); gWu2D->addWidget(m_editWuAxTens, 0, 1);
    gWu2D->addWidget(m_lbWuAxComp, 1, 0); gWu2D->addWidget(m_editWuAxComp, 1, 1);
    gWu2D->addWidget(m_lbWuIpTens, 2, 0); gWu2D->addWidget(m_editWuIpTens, 2, 1);
    gWu2D->addWidget(m_lbWuIpComp, 3, 0); gWu2D->addWidget(m_editWuIpComp, 3, 1);
    gWu2D->addWidget(m_lbWuShear, 4, 0); gWu2D->addWidget(m_editWuShear, 4, 1);
    gWu2D->addWidget(m_lbWuCouple, 5, 0); gWu2D->addWidget(m_editWuCouple, 5, 1);
    gWu2D->setColumnStretch(1, 1);
    gWu2D->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding),
        6, 0, 1, 2);
    gWu2D->setRowStretch(6, 1);
    // ===== Tsai Wu 3D 模型页：参数 + 查看按钮 =====
    QLabel* lbWu3D = new QLabel(QStringLiteral("参数"), this);
    m_editWu3D = new QLineEdit(this);
    m_editWu3D->setReadOnly(true);
    m_editWu3D->setText(QStringLiteral("0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0"));
    m_btnWu3DView = new QPushButton(QStringLiteral("查看"), this);
    QWidget* pageWu3D = new QWidget(this);
    QGridLayout* gWu3D = new QGridLayout(pageWu3D);
    gWu3D->setContentsMargins(0, 0, 0, 0);
    gWu3D->setHorizontalSpacing(24);
    gWu3D->setVerticalSpacing(14);
    gWu3D->addWidget(lbWu3D, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    gWu3D->addWidget(m_editWu3D, 0, 1);
    gWu3D->addWidget(m_btnWu3DView, 0, 2);
    gWu3D->setColumnStretch(1, 1);
    gWu3D->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding),
        1, 0, 1, 3);
    gWu3D->setRowStretch(1, 1);
    // 样式统一：模型/类型标签右对齐、下拉伸展
    lbModel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lbType->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_cmbModel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_cmbType->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_cmbComp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 参数区栈：0=Maximum，1=Tsai Hill 2D（与模型下拉 addItems 顺序一致）
    m_stackParams = new QStackedWidget(this);
    m_stackParams->addWidget(pageMax);
    m_stackParams->addWidget(pageTsai);
    m_stackParams->addWidget(pageTsai3DTrans);
    m_stackParams->addWidget(pageTsai3D);
    m_stackParams->addWidget(pageAzzi);
    m_stackParams->addWidget(pageWu2D);
    m_stackParams->addWidget(pageWu3D);
    // 主布局
    QGridLayout* grid = makeFormGrid();
    int r = 0;
    grid->addWidget(title, r++, 0, 1, 3);
    grid->addWidget(lbModel, r, 0, Qt::AlignRight | Qt::AlignVCenter);  grid->addWidget(m_cmbModel, r, 1); r++;
    grid->addWidget(m_lbType, r, 0, Qt::AlignRight | Qt::AlignVCenter);   grid->addWidget(m_cmbType, r, 1); r++;
    auto temp = makeRow();
    grid->addWidget(temp, r++, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(lbParam, r, 0, Qt::AlignRight | Qt::AlignVCenter); r++;
    grid->addWidget(m_stackParams, r, 0, 1, 2); r++;
    grid->setColumnStretch(1, 1);
    grid->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding),
        r, 0, 1, 3);
    // 数据字段桥接表：edit/mp 为空的条目仅参与 Stress/Strain 文案切换
    buildFieldBridges();
    // 模型联动：切参数页（下拉顺序与 addItems 一致：0=Maximum,1=Tsai Hill 2D,...）
    connect(m_cmbModel, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [=](int idx) {
            MechFrpData::mFrpFailureData* data = m_pCurData ? m_pCurData->as<MechFrpData::mFrpFailureData>() : nullptr;
            // 先把当前控件值存回旧模型字段（避免复用页之间互相污染）
            storeSharedParams(data, m_lastModel);
            // 切参数页 + 类型行显隐 + 从新模型字段回填共用控件
            applyModelUi(idx);
            loadSharedParams(data, idx);
            m_lastModel = idx;
        });
    // 类型联动：所有标签在 Stress/Strain 间切换
    connect(m_cmbType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int idx) {
        const bool stress = (idx == 0);
        for (const FieldBridge& f : m_fields)
            if (f.label)
                f.label->setText(stress ? f.stressText : f.strainText);
        });
    // Tsai Wu 3D 查看弹窗：H 张量 6 个 + G 张量 21 个，共 27 个分量
    connect(m_btnWu3DView, &QPushButton::clicked, this, [=] {
        QDialog dlg(this);
        dlg.setWindowTitle(QStringLiteral("Tsai Wu 3D 参数"));
        dlg.setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
        QGridLayout* gl = new QGridLayout(&dlg);
        gl->setHorizontalSpacing(12);
        gl->setVerticalSpacing(10);
        m_wu3dEdits.clear();
        auto add = [&](const QString& name, int r, int c) {
            QLabel* lb = new QLabel(name, &dlg);
            QLineEdit* ed = new QLineEdit(&dlg);
            ed->setText(QStringLiteral("0"));
            ed->setFixedWidth(60);
            gl->addWidget(lb, r, c);
            gl->addWidget(ed, r, c + 1);
            m_wu3dEdits << ed;
            };
        // H 张量 6 个（三角排列）
        add("H11", 0, 0); add("H22", 1, 2); add("H33", 2, 4);
        add("H12", 0, 2); add("H13", 0, 4);
        add("H23", 1, 4);

        // G 张量 21 个
        add("G11", 3, 0);
        add("G12", 3, 2); add("G22", 4, 2);
        add("G13", 3, 4); add("G23", 4, 4); add("G33", 5, 4);
        add("G14", 3, 6); add("G24", 4, 6); add("G34", 5, 6); add("G44", 6, 6);
        add("G15", 3, 8); add("G25", 4, 8); add("G35", 5, 8); add("G45", 6, 8); add("G55", 7, 8);
        add("G16", 3, 10); add("G26", 4, 10); add("G36", 5, 10); add("G46", 6, 10); add("G56", 7, 10); add("G66", 8, 10);
        // 打开时回填：按顺序解析主框中的 27 个值，缺项补 "0"
        const QStringList saved = m_editWu3D->text().split(';');
        for (int i = 0; i < m_wu3dEdits.size(); ++i)
            m_wu3dEdits[i]->setText(saved.value(i).trimmed().isEmpty()
                ? QStringLiteral("0") : saved.value(i).trimmed());
        // 保存 / 取消
        QPushButton* btnSave = new QPushButton(QStringLiteral("保存"), &dlg);
        QPushButton* btnCancel = new QPushButton(QStringLiteral("取消"), &dlg);
        QHBoxLayout* hb = new QHBoxLayout();
        hb->setSpacing(12);
        hb->addStretch();
        hb->addWidget(btnSave);
        hb->addWidget(btnCancel);
        gl->addLayout(hb, 9, 0, 1, 12);
        // 保存：逐框校验，空 / 非数字一律按 0，按序拼回主框并关闭
        connect(btnSave, &QPushButton::clicked, this, [&dlg, this] {
            QStringList vals;
            for (QLineEdit* ed : m_wu3dEdits)
            {
                bool ok = false;
                const QString t = ed->text().trimmed();
                t.toDouble(&ok);
                vals << (ok ? t : QStringLiteral("0"));
            }
            m_editWu3D->setText(vals.join("; "));   // 触发 textChanged -> 自动存回 MechFrpData::mFrpFailureData::wu3d
            dlg.accept();
            });
        // 取消：直接关闭，不写回
        connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
        dlg.exec();
        m_wu3dEdits.clear();   // 弹窗已析构，清空避免残留悬垂指针
        });
}

// 建立“标签 <-> 输入框 <-> 数据字段”桥接表；复用页共用控件被多个模型字段映射（见 loadSharedParams）
void FailurePage::buildFieldBridges()
{
    m_fields.clear();
    auto add = [this](QLabel* lb, QLineEdit* ed, QString MechFrpData::mFrpFailureData::* mp,
        const QString& sText, const QString& tText) {
            FieldBridge f;
            f.label = lb; f.edit = ed; f.mp = mp;
            f.stressText = sText; f.strainText = tText;
            m_fields.append(f);
        };
    // Maximum 页
    add(m_lbTens, m_editTens, &MechFrpData::mFrpFailureData::tens,
        QStringLiteral("抗拉强度"), QStringLiteral("最大拉伸应变"));
    add(m_lbComp2, m_editComp2, &MechFrpData::mFrpFailureData::comp2,
        QStringLiteral("抗压强度"), QStringLiteral("最大压缩应变"));
    // Tsai Hill 2D 页（Tsai Wu 3D Transversely 复用本页控件，字段按模型另存）
    add(m_lbAxialTens, m_editAxialTens, &MechFrpData::mFrpFailureData::axialTens,
        QStringLiteral("轴向抗拉强度"), QStringLiteral("最大轴向拉伸应变"));
    add(m_lbAxialComp, m_editAxialComp, &MechFrpData::mFrpFailureData::axialComp,
        QStringLiteral("轴向抗压强度"), QStringLiteral("最大轴向压缩应变"));
    add(m_lbInPlaneTens, m_editInPlaneTens, &MechFrpData::mFrpFailureData::inPlaneTens,
        QStringLiteral("面内抗拉强度"), QStringLiteral("最大面内拉伸应变"));
    add(m_lbInPlaneComp, m_editInPlaneComp, &MechFrpData::mFrpFailureData::inPlaneComp,
        QStringLiteral("面内抗压强度"), QStringLiteral("最大面内压缩应变"));
    add(m_lbShear, m_editShear, &MechFrpData::mFrpFailureData::shear,
        QStringLiteral("横向剪切强度"), QStringLiteral("最大横向剪切应变"));
    // Tsai Hill 3D Transversely 页（3 框）
    add(m_lbAxialTensT, m_editAxialTensT, &MechFrpData::mFrpFailureData::axialTensT,
        QStringLiteral("轴向抗拉强度"), QStringLiteral("最大轴向拉伸应变"));
    add(m_lbInPlaneTensT, m_editInPlaneTensT, &MechFrpData::mFrpFailureData::inPlaneTensT,
        QStringLiteral("面内抗拉强度"), QStringLiteral("最大面内拉伸应变"));
    add(m_lbShearT, m_editShearT, &MechFrpData::mFrpFailureData::shearT,
        QStringLiteral("横向剪切强度"), QStringLiteral("最大横向剪切应变"));
    // Tsai Hill 3D 页（6 框）
    add(m_lbXt, m_editXt, &MechFrpData::mFrpFailureData::xt, QStringLiteral("X向抗拉强度"), QStringLiteral("X向最大拉伸应变"));
    add(m_lbYt, m_editYt, &MechFrpData::mFrpFailureData::yt, QStringLiteral("Y向抗拉强度"), QStringLiteral("Y向最大拉伸应变"));
    add(m_lbZt, m_editZt, &MechFrpData::mFrpFailureData::zt, QStringLiteral("Z向抗拉强度"), QStringLiteral("Z向最大拉伸应变"));
    add(m_lbXYs, m_editXYs, &MechFrpData::mFrpFailureData::xys, QStringLiteral("XY向剪切强度"), QStringLiteral("XY向最大剪切应变"));
    add(m_lbYZs, m_editYZs, &MechFrpData::mFrpFailureData::yzs, QStringLiteral("YZ向剪切强度"), QStringLiteral("YZ向最大剪切应变"));
    add(m_lbXZs, m_editXZs, &MechFrpData::mFrpFailureData::xzs, QStringLiteral("XZ向剪切强度"), QStringLiteral("XZ向最大剪切应变"));
    // Azzi 页（Multi Component 2D 复用本页控件，字段按模型另存）
    add(m_lbAzAxTens, m_editAzAxTens, &MechFrpData::mFrpFailureData::azAxTens,
        QStringLiteral("轴向抗拉强度"), QStringLiteral("最大轴向拉伸应变"));
    add(m_lbAzAxComp, m_editAzAxComp, &MechFrpData::mFrpFailureData::azAxComp,
        QStringLiteral("轴向抗压强度"), QStringLiteral("最大轴向压缩应变"));
    add(m_lbAzIpTens, m_editAzIpTens, &MechFrpData::mFrpFailureData::azIpTens,
        QStringLiteral("面内抗拉强度"), QStringLiteral("最大面内拉伸应变"));
    add(m_lbAzIpComp, m_editAzIpComp, &MechFrpData::mFrpFailureData::azIpComp,
        QStringLiteral("面内抗压强度"), QStringLiteral("最大面内压缩应变"));
    add(m_lbAzShear, m_editAzShear, &MechFrpData::mFrpFailureData::azShear,
        QStringLiteral("横向剪切强度"), QStringLiteral("最大横向剪切应变"));
    // Tsai Wu 2D 页（6 框，含耦合强度；耦合强度仅 Stress 文案）
    add(m_lbWuAxTens, m_editWuAxTens, &MechFrpData::mFrpFailureData::wuAxTens,
        QStringLiteral("轴向抗拉强度"), QStringLiteral("最大轴向拉伸应变"));
    add(m_lbWuAxComp, m_editWuAxComp, &MechFrpData::mFrpFailureData::wuAxComp,
        QStringLiteral("轴向抗压强度"), QStringLiteral("最大轴向压缩应变"));
    add(m_lbWuIpTens, m_editWuIpTens, &MechFrpData::mFrpFailureData::wuIpTens,
        QStringLiteral("面内抗拉强度"), QStringLiteral("最大面内拉伸应变"));
    add(m_lbWuIpComp, m_editWuIpComp, &MechFrpData::mFrpFailureData::wuIpComp,
        QStringLiteral("面内抗压强度"), QStringLiteral("最大面内压缩应变"));
    add(m_lbWuShear, m_editWuShear, &MechFrpData::mFrpFailureData::wuShear,
        QStringLiteral("横向剪切强度"), QStringLiteral("最大横向剪切应变"));
    add(m_lbWuCouple, m_editWuCouple, &MechFrpData::mFrpFailureData::wuCouple,
        QStringLiteral("轴向-平面耦合强度"), QStringLiteral("轴向-平面耦合强度"));
}

// 按模型索引切换参数页并控制“类型”行显隐
void FailurePage::applyModelUi(int idx)
{
    // Azzi Tsai Hill 2D（索引4）/ Tsai Wu 3D（7）/ Multi Component 2D（8）无“类型”行
    const bool hasType = (idx != 4 && idx != 7 && idx != 8);
    m_lbType->setVisible(hasType);
    m_cmbType->setVisible(hasType);
    // Tsai Wu 3D Transversely（索引6）参数与 Tsai Hill 2D（索引1）完全相同，复用同一页；
    // Multi Component 2D（索引8）复用 Azzi 页（索引4）。
    int pageIdx = idx;
    if (idx == 6) pageIdx = 1;
    if (idx == 7) pageIdx = 6;
    if (idx == 8) pageIdx = 4;
    if (pageIdx < m_stackParams->count())
        m_stackParams->setCurrentIndex(pageIdx);
}

// 复用页（Tsai Hill 2D 页 / Azzi 页）按当前模型索引加载各自独立数据
void FailurePage::loadSharedParams(MechFrpData::mFrpFailureData* data, int model)
{
    if (!data) return;
    if (model == 1 || model == 6)          // Tsai Hill 2D / Tsai Wu 3D Transversely 共用同一页 5 框
    {
        const bool wuT = (model == 6);
        // 控件当前值来自各自字段：wuT 用 wuT* 字段，否则用 axial* 字段
        QLineEdit* eds[5] = { m_editAxialTens, m_editAxialComp, m_editInPlaneTens, m_editInPlaneComp, m_editShear };
        const QString vals[5] = {
            wuT ? data->wuTAxialTens : data->axialTens,
            wuT ? data->wuTAxialComp : data->axialComp,
            wuT ? data->wuTInPlaneTens : data->inPlaneTens,
            wuT ? data->wuTInPlaneComp : data->inPlaneComp,
            wuT ? data->wuTShear : data->shear
        };
        for (int i = 0; i < 5; ++i)
            eds[i]->setText(vals[i]);
    }
    else if (model == 4 || model == 8)     // Azzi / Multi Component 2D 共用同一页 5 框
    {
        const bool mc = (model == 8);
        QLineEdit* eds[5] = { m_editAzAxTens, m_editAzAxComp, m_editAzIpTens, m_editAzIpComp, m_editAzShear };
        const QString vals[5] = {
            mc ? data->mcAxialTens : data->azAxTens,
            mc ? data->mcAxialComp : data->azAxComp,
            mc ? data->mcInPlaneTens : data->azIpTens,
            mc ? data->mcInPlaneComp : data->azIpComp,
            mc ? data->mcShear : data->azShear
        };
        for (int i = 0; i < 5; ++i)
            eds[i]->setText(vals[i]);
    }
}

// 复用页：把控件当前值写回当前模型字段（切走前调用，避免复用页互相污染）
void FailurePage::storeSharedParams(MechFrpData::mFrpFailureData* data, int model)
{
    if (!data) return;
    if (model == 1)          // Tsai Hill 2D 页 -> axial* 字段
    {
        data->axialTens = m_editAxialTens->text();
        data->axialComp = m_editAxialComp->text();
        data->inPlaneTens = m_editInPlaneTens->text();
        data->inPlaneComp = m_editInPlaneComp->text();
        data->shear = m_editShear->text();
    }
    else if (model == 6)     // Tsai Hill 2D 页 -> wuT* 字段
    {
        data->wuTAxialTens = m_editAxialTens->text();
        data->wuTAxialComp = m_editAxialComp->text();
        data->wuTInPlaneTens = m_editInPlaneTens->text();
        data->wuTInPlaneComp = m_editInPlaneComp->text();
        data->wuTShear = m_editShear->text();
    }
    else if (model == 4)     // Azzi 页 -> az* 字段
    {
        data->azAxTens = m_editAzAxTens->text();
        data->azAxComp = m_editAzAxComp->text();
        data->azIpTens = m_editAzIpTens->text();
        data->azIpComp = m_editAzIpComp->text();
        data->azShear = m_editAzShear->text();
    }
    else if (model == 8)     // Azzi 页 -> mc* 字段
    {
        data->mcAxialTens = m_editAzAxTens->text();
        data->mcAxialComp = m_editAzAxComp->text();
        data->mcInPlaneTens = m_editAzIpTens->text();
        data->mcInPlaneComp = m_editAzIpComp->text();
        data->mcShear = m_editAzShear->text();
    }
}

void FailurePage::loadData(MechFrpData::mFrpBaseData* data)
{
    m_pCurData = data;
    MechFrpData::mFrpFailureData* d = data ? data->as<MechFrpData::mFrpFailureData>() : nullptr;
    // 恢复模型：blockSignals 避免触发切换 lambda 的 storeSharedParams（防止跨节点污染），
    // 随后手动应用页面切换
    m_cmbModel->blockSignals(true);
    m_cmbModel->setCurrentText(d ? d->model : QString());
    m_lastModel = m_cmbModel->currentIndex();
    m_cmbModel->blockSignals(false);
    applyModelUi(m_lastModel);
    m_cmbType->setCurrentText(d ? d->failType : QString());
    m_cmbComp->setCurrentText(d ? d->compCriterion : QString());
    m_editTens->setText(d ? d->tens : QString());
    m_editComp2->setText(d ? d->comp2 : QString());
    m_editAxialTensT->setText(d ? d->axialTensT : QString());
    m_editInPlaneTensT->setText(d ? d->inPlaneTensT : QString());
    m_editShearT->setText(d ? d->shearT : QString());
    m_editXt->setText(d ? d->xt : QString());
    m_editYt->setText(d ? d->yt : QString());
    m_editZt->setText(d ? d->zt : QString());
    m_editXYs->setText(d ? d->xys : QString());
    m_editYZs->setText(d ? d->yzs : QString());
    m_editXZs->setText(d ? d->xzs : QString());
    m_editWuAxTens->setText(d ? d->wuAxTens : QString());
    m_editWuAxComp->setText(d ? d->wuAxComp : QString());
    m_editWuIpTens->setText(d ? d->wuIpTens : QString());
    m_editWuIpComp->setText(d ? d->wuIpComp : QString());
    m_editWuShear->setText(d ? d->wuShear : QString());
    m_editWuCouple->setText(d ? d->wuCouple : QString());
    static const QString kWu3dDefault =
        QStringLiteral("0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0");
    m_editWu3D->setText((d && !d->wu3d.isEmpty()) ? d->wu3d : kWu3dDefault);
    // 复用页（Tsai Hill 2D 页 / Azzi 页）从当前模型字段回填共用控件
    loadSharedParams(d, m_lastModel);
}

void FailurePage::saveData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpFailureData* d = data ? data->as<MechFrpData::mFrpFailureData>() : nullptr;
    if (!d) return;
    d->model = m_cmbModel->currentText();
    d->failType = m_cmbType->currentText();
    d->compCriterion = m_cmbComp->currentText();
    d->tens = m_editTens->text();
    d->comp2 = m_editComp2->text();
    d->axialTensT = m_editAxialTensT->text();
    d->inPlaneTensT = m_editInPlaneTensT->text();
    d->shearT = m_editShearT->text();
    d->xt = m_editXt->text();
    d->yt = m_editYt->text();
    d->zt = m_editZt->text();
    d->xys = m_editXYs->text();
    d->yzs = m_editYZs->text();
    d->xzs = m_editXZs->text();
    d->wuAxTens = m_editWuAxTens->text();
    d->wuAxComp = m_editWuAxComp->text();
    d->wuIpTens = m_editWuIpTens->text();
    d->wuIpComp = m_editWuIpComp->text();
    d->wuShear = m_editWuShear->text();
    d->wuCouple = m_editWuCouple->text();
    d->wu3d = m_editWu3D->text();
    // 复用页按当前模型把共用控件写回各自字段
    storeSharedParams(d, m_cmbModel->currentIndex());
}

/************************************************** 长细比 ******************************************************/
AspectRatioFunPage::AspectRatioFunPage(QWidget* parent)
    : mFrpCompPropertyPage(parent)
{
    // 顶部：标题 + 增/删 + 右侧导出/绘制
    QHBoxLayout* top = new QHBoxLayout();
    QLabel* lbTitle = new QLabel(QStringLiteral("表格数据"), this);
    QFont tf = lbTitle->font(); tf.setBold(true); lbTitle->setFont(tf);
    m_btnAdd = new QPushButton(); m_btnAdd->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); m_btnAdd->setToolTip("增加");
    connect(m_btnAdd, &QPushButton::clicked, this, [this]() {
        addRow(m_table->currentRow() < 0 ? m_table->rowCount() : m_table->currentRow() + 1);
        if (!isLoading()) notifyChanged();
        });
    m_btnRemove = new QPushButton(); m_btnRemove->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); m_btnRemove->setToolTip("减少");
    connect(m_btnRemove, &QPushButton::clicked, this, [this]() {
        removeSelectedOrLastRow(m_table);            // 删选中行；无选中删最后一行；空表无操作
        renumberTableColumn(m_table, 0);
        if (!isLoading()) notifyChanged();
        });
    m_btnAdd->setMaximumWidth(60);
    m_btnRemove->setMaximumWidth(60);
    m_btnImport = new QPushButton(QStringLiteral("导入数据"), this);
    m_btnExport = new QPushButton(QStringLiteral("导出数据"), this);
    m_btnPlot = new QPushButton(QStringLiteral("绘制曲线"), this);
    m_btnExport->setMaximumWidth(80);
    m_btnPlot->setMaximumWidth(80);
    connect(m_btnImport, &QPushButton::clicked, this, &AspectRatioFunPage::slot_importData);
    connect(m_btnExport, &QPushButton::clicked, this, &AspectRatioFunPage::slot_exportData);
    connect(m_btnPlot, &QPushButton::clicked, this, &AspectRatioFunPage::slot_plotCurve);
    top->addWidget(lbTitle);
    top->addStretch();
    top->addWidget(m_btnImport);
    top->addWidget(m_btnExport);
    top->addWidget(m_btnPlot);
    QHBoxLayout* top2 = new QHBoxLayout();
    top2->addWidget(m_btnAdd);
    top2->addWidget(m_btnRemove);
    top2->addStretch();

    // 表格（本基线为 3 列：序号 / X / Y，无末尾“+”行，增删走顶部按钮）
    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("序号"), QStringLiteral("X"), QStringLiteral("Y"),
        });
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 80);
    m_table->setShowGrid(true);
    const int maxH = 17 * m_table->verticalHeader()->defaultSectionSize() + m_table->horizontalHeader()->height() + m_table->frameWidth() * 2;
    m_table->setFixedHeight(maxH);
    m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(QStringLiteral(
        "QHeaderView::section {background - color: #F7F8FA;color: #2c3e50;font - weight: normal;padding: 4px;border: 1px solid #dcdcdc;}"
        "QTableWidget { gridline-color:#d0e6f7; alternate-background-color:#ECF5FF;"
        " selection-background-color:#add8e6; selection-color:black; }"
        "QTableWidget::item { background:transparent; }"
        "QTableWidget:disabled { background:#F2F2F2; }"
        "QTableWidget::item:disabled { color:#9A9A9A; }"
        "QTableWidget QLineEdit { border:none; background:transparent; }"
        "QTableWidget QComboBox { border:1px solid #b8b8b8; border-radius:0px; padding:2px; margin:2px; }"
        "QTableWidget QComboBox::drop-down { border:none; width:18px; }"
        "QTableWidget QComboBox QAbstractItemView { border:1px solid #b8b8b8; outline:none; }"
        "QTableWidget QPushButton { border:1px solid #b8b8b8; border-radius:0px; background:#ffffff; padding:2px; margin:2px; }"
        "QTableWidget QPushButton:disabled { background-color:#E6E6E6; color:#9A9A9A; border:1px solid #C8C8C8; }"
        "QTableWidget QLineEdit:disabled, QTableWidget QComboBox:disabled { background-color:#F3F3F3; color:#888; }"));

    // 主布局
    QVBoxLayout* v = new QVBoxLayout(this);
    v->setContentsMargins(24, 20, 24, 20);
    v->setSpacing(10);
    v->addLayout(top);
    v->addLayout(top2);
    v->addWidget(m_table);
    v->addStretch();
}

void AspectRatioFunPage::addRow(int row, const std::pair<QString, QString> init)
{
    if (row == -1)
        row = m_table->rowCount();
    m_table->insertRow(row);
    // 0 序号
    QTableWidgetItem* itNo = new QTableWidgetItem(QString::number(row + 1));
    itNo->setFlags(itNo->flags() & ~Qt::ItemIsEditable);
    m_table->setItem(row, 0, itNo);
    // 1 X
    QLineEdit* editX = new QLineEdit(this);
    editX->setText("");
    m_table->setCellWidget(row, 1, editX);
    // 2 Y
    QLineEdit* editY = new QLineEdit(this);
    editY->setText("");
    m_table->setCellWidget(row, 2, editY);
    // 编辑即时触发自动保存（回填期间 isLoading 屏蔽）
    connect(editX, &QLineEdit::textChanged, this, [this](const QString&) { if (!isLoading()) notifyChanged(); });
    connect(editY, &QLineEdit::textChanged, this, [this](const QString&) { if (!isLoading()) notifyChanged(); });
    // 反显时按保存值回填
    editX->setText(init.first);
    editY->setText(init.second);
}

void AspectRatioFunPage::loadData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpAspectRatioData* d = data ? data->as<MechFrpData::mFrpAspectRatioData>() : nullptr;
    // 本基线无“+”行：清空全部行后逐点重建（addRow 内部回填 X/Y）
    m_table->setRowCount(0);
    if (d)
    {
        for (const std::pair<QString, QString>& p : d->rows)
            addRow(-1, p);
    }
    // 重排序号
    renumberTableColumn(m_table, 0);
}

void AspectRatioFunPage::saveData(MechFrpData::mFrpBaseData* data)
{
    MechFrpData::mFrpAspectRatioData* d = data ? data->as<MechFrpData::mFrpAspectRatioData>() : nullptr;
    if (!d) return;
    d->rows.clear();
    for (int row = 0; row < m_table->rowCount(); ++row)   // 全部为数据行
    {
        std::pair<QString, QString> p;
        if (QLineEdit* ex = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 1)))
            p.first = ex->text();
        if (QLineEdit* ey = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 2)))
            p.second = ey->text();
        d->rows.append(p);
    }
}

// 收集表格 X/Y：空行跳过；任一格非数字或整表为空则警告并返回 false
bool AspectRatioFunPage::collectPoints(QVector<double>& xs, QVector<double>& ys) const
{
    xs.clear();
    ys.clear();
    const QString obj = m_pCurData ? m_pCurData->name : QStringLiteral("长细比函数");
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        QString sx, sy;
        if (QLineEdit* ex = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 1)))
            sx = ex->text().trimmed();
        if (QLineEdit* ey = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 2)))
            sy = ey->text().trimmed();
        if (sx.isEmpty() && sy.isEmpty())
            continue;
        bool okX = false, okY = false;
        const double xv = sx.toDouble(&okX);
        const double yv = sy.toDouble(&okY);
        if (!okX || !okY)
        {
            outputWarn(obj, QStringLiteral("第 %1 行数据不合法").arg(row + 1));
            return false;
        }
        xs << xv;
        ys << yv;
    }
    if (xs.isEmpty())
    {
        outputWarn(obj, QStringLiteral("表格中没有数据"));
        return false;
    }
    return true;
}

void AspectRatioFunPage::slot_importData()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this, QStringLiteral("导入长细比数据"), QString(),
        QStringLiteral("数据文件 (*.txt *.csv);;所有文件 (*.*)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        outputWarn(m_pCurData ? m_pCurData->name : QString(), QStringLiteral("导入失败：文件无法打开！"));
        return;
    }

    // 逐行解析：跳过空行与 # 注释行；每行取前两个数值字段，非数值行跳过
    QVector<std::pair<QString, QString>> rows;
    QTextStream in(&file);
    in.setCodec("UTF-8");
    const QRegularExpression sep(QStringLiteral("[,;\\t ]+"));
    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) continue;
        const QStringList parts = line.split(sep, QString::SkipEmptyParts);
        if (parts.size() < 2) continue;
        bool okX = false, okY = false;
        parts[0].trimmed().toDouble(&okX);
        parts[1].trimmed().toDouble(&okY);
        if (!okX || !okY) continue;                 // 表头/注释等非数值行自然跳过
        rows.append(std::make_pair(parts[0].trimmed(), parts[1].trimmed()));
    }
    file.close();

    if (rows.isEmpty())
    {
        outputWarn(m_pCurData ? m_pCurData->name : QString(),
            QStringLiteral("导入失败：文件中没有有效的数值行（每行需 X、Y 两个数值，可用逗号/分号/制表符/空格分隔）"));
        return;
    }

    // 替换式导入：清空后逐行回填（addRow 内部编辑信号会触发自动保存链路）
    m_table->setRowCount(0);
    for (const std::pair<QString, QString>& p : rows)
        addRow(-1, p);
    renumberTableColumn(m_table, 0);
    if (!isLoading()) notifyChanged();
}
void AspectRatioFunPage::slot_exportData()
{
    QString selectedFilter;
    const QString fileName = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出长细比数据"), QString(),
        QStringLiteral("CSV 文件 (*.csv);;文本文件 (*.txt);;所有文件 (*.*)"), &selectedFilter);
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        outputWarn(m_pCurData ? m_pCurData->name : QString(), QStringLiteral("导出失败：文件无法写入！"));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        QString x, y;
        if (QLineEdit* ex = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 1)))
            x = ex->text().trimmed();
        if (QLineEdit* ey = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 2)))
            y = ey->text().trimmed();
        out << x << ", " << y << '\n';
    }
    file.close();
}

void AspectRatioFunPage::slot_plotCurve()
{
    QVector<double> xs, ys;
    if (!collectPoints(xs, ys))
        return;
    MechMainWindow::mCurveWindow::getInstance()->updateCurve(
        xs, ys, PlotView::SmoothChart,
        m_pCurData ? m_pCurData->name : QStringLiteral("长细比函数"),
        true, false);
    MechMainWindow::mCurveWindow::getInstance()->setViewStyleVisible(true);
}

/************************************************** mFrpCompMaterialWindow ******************************************************/
MechMainWindow::mFrpCompMaterialWindow::mFrpCompMaterialWindow(QWidget* parent)
    : QWidget(parent), ui(new Ui::mFrpCompMaterialWindowClass())
{
    ui->setupUi(this);
    this->setWindowTitle("纤维增强复合材料");
    this->setWindowIcon(QIcon(":/mMainWidgets/Icon/logo.png"));
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);
    initUI();
    initConnect();
    initMaterialTree();
}

MechMainWindow::mFrpCompMaterialWindow::~mFrpCompMaterialWindow()
{
    delete ui;
}

void MechMainWindow::mFrpCompMaterialWindow::initUI()
{
    ui->line->setStyleSheet("color: #D0D0D0;");
    ui->line_2->setStyleSheet("color: #D0D0D0;");
    m_pPropertyPanel = new mMaterialPropertyPanel(ui->stackWidget_Property, this);
    m_pPropertyPanel->hide();
}

void MechMainWindow::mFrpCompMaterialWindow::initConnect()
{
    connect(ui->treeWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &mFrpCompMaterialWindow::slot_showSelectedNodePage);
    auto applyTreeFilter = [this](const QString& text) {
        // 全层级递归过滤：匹配则保留，不匹配则按“自身/后代是否命中”显隐
        auto matchNode = [text](QTreeWidgetItem* it) {
            return it->text(0).contains(text, Qt::CaseInsensitive);
            };
        std::function<bool(QTreeWidgetItem*)> filterNode = [&](QTreeWidgetItem* it) -> bool {
            bool hit = matchNode(it);
            bool childHit = false;
            for (int i = 0; i < it->childCount(); ++i)
                if (filterNode(it->child(i)))
                    childHit = true;
            const bool visible = hit || childHit;
            it->setHidden(!visible);
            if (visible) it->setExpanded(true);
            return visible;
            };
        if (text.isEmpty())
        {
            // 清空过滤：递归恢复所有层级可见。
            // 注意不能用默认 QTreeWidgetItemIterator——它的默认标志会跳过隐藏项，
            // 过滤期被 setHidden(true) 的节点遍历不到，导致“显示不全”。
            std::function<void(QTreeWidgetItem*)> unhideAll = [&](QTreeWidgetItem* it) {
                it->setHidden(false);
                for (int i = 0; i < it->childCount(); ++i)
                    unhideAll(it->child(i));
                };
            for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
                unhideAll(ui->treeWidget->topLevelItem(i));
        }
        else
        {
            // 有关键字：自顶向下递归，节点自身命中或任一后代命中即保留，并展开命中的分支
            for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
                filterNode(ui->treeWidget->topLevelItem(i));
        }
        };
    connect(ui->lineEdit_Filter, &QLineEdit::textChanged, this, applyTreeFilter);
    // 右键菜单：空白处与 RVE 无菜单；实例节点=重命名/复制/新增/删除；分组节点=新增/删除全部子节点；
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested,
        this, &mFrpCompMaterialWindow::showTreeContextMenu);
    connect(ui->pushButton_Remove, &QPushButton::clicked, this, &mFrpCompMaterialWindow::slot_removeAnalysis);
    connect(ui->pushButton_Add, &QPushButton::clicked, this, &mFrpCompMaterialWindow::addAnalysisRootItem);
    connect(ui->pushButton_Output, &QPushButton::clicked, this, &mFrpCompMaterialWindow::slot_exportComposite);
    connect(ui->pushButton_Input, &QPushButton::clicked, this, [this, applyTreeFilter]() {    //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = R"(MxSim(*.mf))";
        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("导入复合材料"),
            defaultDir,
            filesuffix
        );
        if (fileName.isEmpty())
            return;   // 用户在对话框点了取消：不读写、不提示
        MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();

        m_pPropertyPanel->flushCurrent();
        MechIOFile::mIOFileManager::getInstance()->slot_importMfFile(fileName);

        });
    connect(MechIOFile::mIOFileManager::getInstance(), SIGNAL(readMfFinishedSig()), this, SLOT(slot_readMfFinished()));

    // 相页面里“相类型”被切换 -> 把该节点变形为对应相（改类型/名字并重切页面）
    connect(m_pPropertyPanel, &mMaterialPropertyPanel::phaseMorphRequested, this, &mFrpCompMaterialWindow::doPhaseMorph);
    // 基体页“新增材料”按钮：在所属均质化模型的材料分组下建一个 Material_N，并在下拉中选中
    connect(m_pPropertyPanel, &mMaterialPropertyPanel::materialAddRequested,
        this, &mFrpCompMaterialWindow::slot_addMaterialForPhase);
    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &mFrpCompMaterialWindow::slot_commitInlineRename);
    connect(ui->treeWidget->itemDelegate(), qOverload<QWidget*, QAbstractItemDelegate::EndEditHint>(&QAbstractItemDelegate::closeEditor),
        this, [this](QWidget*, QAbstractItemDelegate::EndEditHint) { m_inlineRenaming = false; });
}

void MechMainWindow::mFrpCompMaterialWindow::initMaterialTree()
{
    ui->treeWidget->setStyleSheet(R"(
        QTreeWidget {
	        font-size:12px;
            background: #FFFFFF;
            border: none;
	    }
	    QTreeWidget::item {
            height: 24px;
            padding: 1px 0;
	    }
	    QTreeWidget::item:hover {
	        color:#303313;
	        background-color: #F0F3F6;
	    }
	    QTreeWidget::item:selected {
	        color:#303313;
	        background-color: #D0E7FD;
	    }
	    QTreeWidget::branch:hover {
	        height:30px;	/*item高度*/
	        margin:0px;		/*item间距*/
	        border-left:0px;
	        border-top:0px;
	        margin-left:0px;		/*item间距*/
	        margin-right:0px;		/*item间距*/
	        margin-top:0px;		/*item间距*/
	        margin-bottom:0px;		/*item间距*/
	        background-color: transparent;
	    }
	    QTreeWidget::branch:selected {
	        height:30px;	/*item高度*/
	        margin:0px;		/*item间距*/
	        border-left:0px;
	        border-top:0px;
	        margin-left:0px;		/*item间距*/
	        margin-right:0px;		/*item间距*/
	        margin-top:0px;		/*item间距*/
	        margin-bottom:0px;		/*item间距*/
	        background-color: #FFFFFF;
	    }
        QTreeWidget::branch:open:has-children:hover {
            image: url(:/mMainWidgets/Icon/expand.png);
        }
        QTreeWidget::branch:open:has-children:selected {
            image: url(:/mMainWidgets/Icon/expand.png);
        }
        QTreeWidget::branch:closed:has-children:hover {
            image: url(:/mMainWidgets/Icon/fold.png);
        }
        QTreeWidget::branch:closed:has-children:selected {
            image: url(:/mMainWidgets/Icon/fold.png);
        }
    )");

    ui->treeWidget->setFrameShape(QFrame::NoFrame);
    ui->treeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->treeWidget->setHeaderHidden(true);
    ui->treeWidget->setRootIsDecorated(true);
    ui->treeWidget->setUniformRowHeights(true);
    ui->treeWidget->setIndentation(10);
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    // 再把整个窗口背景兜底设白：
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);
    // 数据驱动：窗口不持有模型副本，直接渲染全局模型树；
    // 仅当全局树为空（首次打开）时补一个默认根（Analysis1 + 5 个空分组），不每次开窗清空。
    if (frpMgr().roots().isEmpty())
        frpMgr().addAnalysisRoot();
    rebuildTree(frpMgr().roots().first());
}

// 全量按全局数据树重建视图：清空树项 -> 逐根递归构建 -> 全展开 -> 选中 selectNode 并反显其页面。
// 所有增删/变形/复制操作改完数据后统一调用本函数，视图永远是数据的投影（调用前自行 flush/clearCurrent）。
void MechMainWindow::mFrpCompMaterialWindow::rebuildTree(MechFrpData::mFrpBaseData* selectNode)
{
    ui->treeWidget->blockSignals(true);
    ui->treeWidget->clear();
    for (MechFrpData::mFrpBaseData* root : frpMgr().roots())
        buildTreeItems(root, nullptr);
    ui->treeWidget->expandAll();
    QTreeWidgetItem* it = findItemByData(selectNode);
    ui->treeWidget->blockSignals(false);
    if (it)
    {
        ui->treeWidget->setCurrentItem(it);   // 触发 selectionChanged -> showPageForData
    }
    else
    {
        m_pPropertyPanel->showPageForData(selectNode);
        refreshAddRemoveState();
    }
}

// 由模型节点递归构建树节点：树项只挂节点指针（FRP_NODE_PTR_ROLE），类型/名字都从指针现取
QTreeWidgetItem* MechMainWindow::mFrpCompMaterialWindow::buildTreeItems(MechFrpData::mFrpBaseData* baseData, QTreeWidgetItem* parentItem)
{
    QTreeWidgetItem* item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(ui->treeWidget);
    item->setText(0, baseData->name);
    item->setData(0, FRP_NODE_PTR_ROLE, QVariant::fromValue(baseData));
    item->setFlags(item->flags() | Qt::ItemIsEditable);  // 仅允许菜单触发的内联重命名，双击仍由 NoEditTriggers 禁止
    for (MechFrpData::mFrpBaseData* c : baseData->children)
        buildTreeItems(c, item);
    return item;
}

// 按节点指针反查树节点
QTreeWidgetItem* MechMainWindow::mFrpCompMaterialWindow::findItemByData(MechFrpData::mFrpBaseData* baseData)
{
    if (!baseData) return nullptr;
    std::function<QTreeWidgetItem* (QTreeWidgetItem*)> search = [&](QTreeWidgetItem* it) -> QTreeWidgetItem* {
        if (!it) return nullptr;
        if (it->data(0, FRP_NODE_PTR_ROLE).value<MechFrpData::mFrpBaseData*>() == baseData)
            return it;
        for (int i = 0; i < it->childCount(); ++i)
            if (QTreeWidgetItem* hit = search(it->child(i)))
                return hit;
        return nullptr;
        };
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
        if (QTreeWidgetItem* hit = search(ui->treeWidget->topLevelItem(i)))
            return hit;
    return nullptr;
}

MechFrpData::mFrpBaseData* MechMainWindow::mFrpCompMaterialWindow::dataOfItem(QTreeWidgetItem* item)
{
    return item ? item->data(0, FRP_NODE_PTR_ROLE).value<MechFrpData::mFrpBaseData*>() : nullptr;
}

MechFrpData::mFrpBaseData* MechMainWindow::mFrpCompMaterialWindow::currentData()
{
    return dataOfItem(ui->treeWidget->currentItem());
}

// 树右键菜单：空白处与 RVE 无菜单；实例=重命名/复制/新增/删除；分组=新增/删除全部子节点；微观N 的新增为四相子菜单
void MechMainWindow::mFrpCompMaterialWindow::showTreeContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = ui->treeWidget->itemAt(pos);
    if (!item) return;                                      // 树空白处：无右键菜单
    ui->treeWidget->setCurrentItem(item);
    MechFrpData::mFrpBaseData* data = dataOfItem(item);
    if (!data || data->type == FrpNode_Rve) return;         // RVE：无右键菜单

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu {"
        "   background-color: #ffffff;"
        "   border-radius: 4px;"
        "   padding: 4px 8px;"
        "}"
        "QMenu::item {"
        "   background-color:transparent;"
        "   min-width: 100px;"
        "   max-height: 18px;"
        "   padding: 4px 8px;"
        "   margin: 0px;"
        "   border-radius: 0px;"
        "}"
        "QMenu::item:selected {"
        "   background-color:#e7e7e7;"
        "   border:0px solid transparent;"
        "}"
    );

    const bool isGroup = data->isGroupData();
    const bool isMicro1 = (data->type == FrpNode_Micro1);

    QAction* actRename = nullptr;
    QAction* actCopy = nullptr;
    if (!isGroup)
    {
        actRename = menu.addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), QStringLiteral("重命名"));
        actCopy = menu.addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), QStringLiteral("复制"));
        menu.addSeparator();
    }

    QAction* actAdd = nullptr;
    QAction* actAddMatrix = nullptr; QAction* actAddInc = nullptr;
    QAction* actAddCav = nullptr;    QAction* actAddFib = nullptr;
    if (isMicro1)
    {
        QMenu* addMenu = menu.addMenu(
            QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), QStringLiteral("新增"));
        actAddMatrix = addMenu->addAction(QStringLiteral("基体"));
        actAddInc = addMenu->addAction(QStringLiteral("掺杂"));
        actAddCav = addMenu->addAction(QStringLiteral("空腔"));
        actAddFib = addMenu->addAction(QStringLiteral("连续纤维"));
    }
    else if (ruleForNode(data).allowed)
    {
        actAdd = menu.addAction(
            QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), QStringLiteral("新增"));
    }

    QAction* actDelete = nullptr;
    if (!isGroup)
        actDelete = menu.addAction(
            QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), QStringLiteral("删除"));

    QAction* actClear = nullptr;
    if (data->m_pParent && !data->children.isEmpty())
        actClear = menu.addAction(
            QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), QStringLiteral("删除全部子节点"));

    QAction* picked = menu.exec(ui->treeWidget->viewport()->mapToGlobal(pos));
    if (!picked) return;
    if (picked == actRename)         startInlineRename(item);
    else if (picked == actCopy)      duplicateCurrent();
    else if (picked == actAdd)       addByRule();
    else if (picked == actAddMatrix) addPhaseToCurrent(FrpNode_Matrix);
    else if (picked == actAddInc)    addPhaseToCurrent(FrpNode_Inclusion);
    else if (picked == actAddCav)    addPhaseToCurrent(FrpNode_Cavity);
    else if (picked == actAddFib)    addPhaseToCurrent(FrpNode_Fiber);
    else if (picked == actDelete)    deleteNodeItem(data);
    else if (picked == actClear)     clearChildrenOfCurrent();
}

void MechMainWindow::mFrpCompMaterialWindow::refreshAddRemoveState()
{
    QList<QTreeWidgetItem*> sel = ui->treeWidget->selectedItems();
    QTreeWidgetItem* item = sel.isEmpty() ? nullptr : sel.first();
    ui->pushButton_Remove->setEnabled(item != nullptr);  // 底部删除：删选中节点所属的整个均质化模型N
}

void MechMainWindow::mFrpCompMaterialWindow::slot_showSelectedNodePage(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);
    QList<QTreeWidgetItem*> selecteditems = ui->treeWidget->selectedItems();
    if (selecteditems.isEmpty())
    {
        refreshAddRemoveState();                  // 空选中：新增仍恒可用，删除禁用
        m_pPropertyPanel->showPageForData(nullptr);
        return;
    }
    // 按新增规则刷新“新增/删除”按钮可用性
    refreshAddRemoveState();
    // 右侧切到对应页（面板内部完成“旧节点存盘 -> 新节点反显”）
    m_pPropertyPanel->showPageForData(dataOfItem(selecteditems.first()));
}

void MechMainWindow::mFrpCompMaterialWindow::slot_removeAnalysis()
{
    MechFrpData::mFrpBaseData* sel = currentData();
    if (!sel) return;
    // 无论选中哪个节点，都删除它所属的整个均质化模型N（Analysis 根）
    MechFrpData::mFrpBaseData* root = (sel->type == FrpNode_Analysis1)
        ? sel : sel->ancestorOfType(FrpNode_Analysis1);
    deleteNodeItem(root);
}

void MechMainWindow::mFrpCompMaterialWindow::deleteNodeItem(MechFrpData::mFrpBaseData* data)
{
    if (!data) return;
    MechFrpData::mFrpBaseData* target = siblingAfterRemove(data);   // 必须在 removeNode 前算：此时 data 仍在兄弟列
    m_pPropertyPanel->flushCurrent();
    m_pPropertyPanel->clearCurrent();
    frpMgr().removeNode(data);
    rebuildTree(target);
}

// 相节点“变形”：相类型下拉被改成 newType 后，切换节点类型/页面；未手动重命名则树名跟随相类型
void MechMainWindow::mFrpCompMaterialWindow::doPhaseMorph(MechFrpData::mFrpBaseData* baseData, FrpNodeType newType)
{
    if (!baseData) return;
    switch (baseData->type)                            // 只有四个相节点允许变形
    {
    case FrpNode_Matrix: case FrpNode_Inclusion:
    case FrpNode_Cavity: case FrpNode_Fiber: break;
    default: return;
    }
    if (newType == baseData->type) return;
    MechFrpData::mFrpBaseData* micro = baseData->ancestorOfType(FrpNode_Micro1);
    if (!MechFrpData::mFrpManager::canAttachPhase(micro, newType, baseData))
    {
        //outputWarn(micro ? micro->name : baseData->name,QStringLiteral("当前微观结构中只允许存在一个基体"));
        m_pPropertyPanel->clearCurrent();
        m_pPropertyPanel->showPageForData(baseData);
        return;
    }
    m_pPropertyPanel->flushCurrent();              // 先把当前相页面（含刚改的字段）存回
    if (!baseData->as<MechFrpData::mFrpPhaseData>()) return;
    baseData->type = newType;
    if (!baseData->renamed)                        // 用户没手动改过名：显示名自动跟随相类型
        baseData->name = MechFrpData::mFrpManager::phaseName(newType);
    rebuildTree(baseData);                         // 重建树（同步名字/类型）并按新类型反显对应相页面
}

void MechMainWindow::mFrpCompMaterialWindow::addAnalysisRootItem()
{
    m_pPropertyPanel->flushCurrent();                 // 空白时无当前页，面板内部直接返回
    MechFrpData::mFrpBaseData* root = frpMgr().addAnalysisRoot();
    rebuildTree(root);
}

void MechMainWindow::mFrpCompMaterialWindow::clearChildrenOfCurrent()
{
    MechFrpData::mFrpBaseData* node = currentData();
    if (!node || node->children.isEmpty()) return;
    m_pPropertyPanel->clearCurrent();                 // 丢弃当前页引用，避免向被删子节点写回
    frpMgr().clearChildren(node);                     // 模型：删除全部直接子节点（虚析构连带子树）
    rebuildTree(node);                                // 仍选中该节点：材料等分组 / 微观N -> 空页
}

// 右键“新增”：按界面规则表新增（Analysis 根→新建根；各分组→对应实例；微观N→基体）
void MechMainWindow::mFrpCompMaterialWindow::addByRule()
{
    MechFrpData::mFrpBaseData* sel = currentData();
    if (!sel) { addAnalysisRootItem(); return; }
    const AddRule rule = ruleForNode(sel);
    if (!rule.allowed) return;
    m_pPropertyPanel->flushCurrent();
    MechFrpData::mFrpBaseData* created = rule.asSibling
        ? frpMgr().addAnalysisRoot()
        : frpMgr().addChildOfType(sel, rule.newType);
    if (!created) return;
    rebuildTree(created);
}

// 右键微观结构N → 新增指定相
void MechMainWindow::mFrpCompMaterialWindow::addPhaseToCurrent(FrpNodeType phaseKind)
{
    MechFrpData::mFrpBaseData* micro = currentData();
    if (!micro || micro->type != FrpNode_Micro1) return;
    if (!MechFrpData::mFrpManager::canAttachPhase(micro, phaseKind))
    {
        // 每个微观结构N 只允许一个基体：点击“基体”给出警告（菜单不置灰）
        //outputWarn(micro->name, QStringLiteral("当前微观结构中只允许存在一个基体"));
        return;
    }
    m_pPropertyPanel->flushCurrent();
    MechFrpData::mFrpBaseData* created = frpMgr().addChildOfType(micro, phaseKind);
    if (!created) return;
    rebuildTree(created);
}

// 右键“复制”
void MechMainWindow::mFrpCompMaterialWindow::duplicateCurrent()
{
    MechFrpData::mFrpBaseData* sel = currentData();
    if (!sel) return;
    m_pPropertyPanel->flushCurrent();
    MechFrpData::mFrpBaseData* copy = frpMgr().duplicateNode(sel);
    if (!copy)
    {
        // 基体唯一等不允许复制的情形（数据层 duplicateNode 返回 nullptr）
        //outputWarn(sel->name, QStringLiteral("当前节点不可复制"));
        return;
    }
    rebuildTree(copy);
}

// 菜单“重命名”：进入内联编辑
void MechMainWindow::mFrpCompMaterialWindow::startInlineRename(QTreeWidgetItem* item)
{
    if (!item) return;
    m_inlineRenaming = true;
    ui->treeWidget->editItem(item, 0);
}

// 内联重命名提交（程序化构建/相变形改名因 m_inlineRenaming=false 被拦截）
void MechMainWindow::mFrpCompMaterialWindow::slot_commitInlineRename(QTreeWidgetItem* item, int column)
{
    if (column != 0 || !m_inlineRenaming) return;
    m_inlineRenaming = false;
    MechFrpData::mFrpBaseData* d = dataOfItem(item);
    if (!d) return;
    const QString oldName = d->name;
    const QString name = item->text(0).trimmed();
    if (name == oldName) { item->setText(0, oldName); return; }      // 未改动：恢复
    // 空名/同级重名由数据层 renameNode 统一拒绝（材料、微观结构按名引用，重名会产生歧义）
    if (!frpMgr().renameNode(d, name))
    {
        item->setText(0, oldName);
        //outputWarn(oldName, QStringLiteral("名称不能为空或与同级节点重名：%1").arg(name));
        return;
    }
    d->renamed = true;
}

void MechMainWindow::mFrpCompMaterialWindow::slot_addMaterialForPhase(MechFrpData::mFrpBaseData* phaseData)
{
    if (!phaseData) return;
    MechFrpData::mFrpBaseData* matGroup = mFrpCompPropertyPage::materialGroupOf(phaseData);
    if (!matGroup) return;
    m_pPropertyPanel->flushCurrent();               // 先存相页当前编辑
    MechFrpData::mFrpBaseData* created = frpMgr().addChildOfType(matGroup, FrpNode_Material);   // 自动命名 Material_N、计序号、分配 id
    if (!created) return;
    // 此处只做局部插入、不整树重建：必须停在相页，让 notifyMaterialAdded 刷新材料下拉并选中新材料
    if (QTreeWidgetItem* gItem = findItemByData(matGroup))
    {
        buildTreeItems(created, gItem);            // 视图同步
        gItem->setExpanded(true);                  // 展开材料分组，让新材料可见
    }
    m_pPropertyPanel->notifyMaterialAdded(phaseData);         // 下拉重建并选中新材料（自动存回 baseMaterial）
    refreshAddRemoveState();
}

void MechMainWindow::mFrpCompMaterialWindow::slot_exportComposite()
{
    // 选择导出文件夹
    const QString startDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
    const QString defaultDir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择导出文件夹"), startDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (defaultDir.isEmpty())
        return;   // 用户点取消：不导出、不提示
    MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(defaultDir).absolutePath();

    m_pPropertyPanel->flushCurrent();
    MechIOFile::mIOFileManager::getInstance()->slot_exportMfFile(defaultDir);
}

void MechMainWindow::mFrpCompMaterialWindow::slot_readMfFinished()
{
    auto applyTreeFilter = [this](const QString& text) {
        // 全层级递归过滤：匹配则保留，不匹配则按“自身/后代是否命中”显隐
        auto matchNode = [text](QTreeWidgetItem* it) {
            return it->text(0).contains(text, Qt::CaseInsensitive);
            };
        std::function<bool(QTreeWidgetItem*)> filterNode = [&](QTreeWidgetItem* it) -> bool {
            bool hit = matchNode(it);
            bool childHit = false;
            for (int i = 0; i < it->childCount(); ++i)
                if (filterNode(it->child(i)))
                    childHit = true;
            const bool visible = hit || childHit;
            it->setHidden(!visible);
            if (visible) it->setExpanded(true);
            return visible;
            };
        if (text.isEmpty())
        {
            // 清空过滤：递归恢复所有层级可见。
            // 注意不能用默认 QTreeWidgetItemIterator——它的默认标志会跳过隐藏项，
            // 过滤期被 setHidden(true) 的节点遍历不到，导致“显示不全”。
            std::function<void(QTreeWidgetItem*)> unhideAll = [&](QTreeWidgetItem* it) {
                it->setHidden(false);
                for (int i = 0; i < it->childCount(); ++i)
                    unhideAll(it->child(i));
                };
            for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
                unhideAll(ui->treeWidget->topLevelItem(i));
        }
        else
        {
            // 有关键字：自顶向下递归，节点自身命中或任一后代命中即保留，并展开命中的分支
            for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
                filterNode(ui->treeWidget->topLevelItem(i));
        }
        };
    // 载入成功后全量重建视图并选中第一个根；失败仅提示，不动当前树。
    MechFrpData::mFrpBaseData* selectNode = frpMgr().roots().isEmpty() ? nullptr : frpMgr().roots().first();
    rebuildTree(selectNode);
    applyTreeFilter(ui->lineEdit_Filter->text());

    mGlobalSignals::getInstance()->setProgressBarValue(100);
    mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
}

/*====================================================================================
 * 数据驱动辅助：窗口不持有 manager 副本，所有操作现取全局模型引用；界面规则与删除后选中留在界面层
 * ==================================================================================*/
MechFrpData::mFrpManager& MechMainWindow::mFrpCompMaterialWindow::frpMgr() const
{
    return getCVM->getModelData()->getManager<MechFrpData::mFrpManager>(Physical_FRCMaterial);
}

MechMainWindow::mFrpCompMaterialWindow::AddRule MechMainWindow::mFrpCompMaterialWindow::ruleForNode(MechFrpData::mFrpBaseData* sel)
{
    AddRule r{ false, false, FrpNode_None };
    if (!sel) return r;
    switch (sel->type)
    {
    case FrpNode_Analysis1:                 // Analysis1：加同级根
        r = { true, true, FrpNode_Analysis1 };
        break;
    case FrpNode_Micro:                     // 微观结构分组 -> 微观结构N
        r = { true, false, FrpNode_Micro1 };
        break;
    case FrpNode_Micro1:                    // 微观结构N -> 新相节点（默认基体；基体唯一由数据层 canAttachPhase 兜底）
        if (MechFrpData::mFrpManager::canAttachPhase(sel, FrpNode_Matrix))
            r = { true, false, FrpNode_Matrix };
        break;
    case FrpNode_Material:                  // 仅“材料”分组 -> 材料实例
        if (sel->isGroupData()) r = { true, false, FrpNode_Material };
        break;
    case FrpNode_Failure:                   // 仅“失效”分组 -> 失效准则实例
        if (sel->isGroupData()) r = { true, false, FrpNode_Failure };
        break;
    case FrpNode_AspectRatio:               // 仅“长细比函数”分组 -> Fun 实例
        if (sel->isGroupData()) r = { true, false, FrpNode_AspectRatio };
        break;
    default:
        break;
    }
    return r;
}

// 删除后选中：下一个兄弟 -> 上一个兄弟 -> 父节点（顶层根同理，没有则 nullptr）。
// 必须在 removeNode 之前调用，此时 data 仍挂在兄弟列表中。
MechFrpData::mFrpBaseData* MechMainWindow::mFrpCompMaterialWindow::siblingAfterRemove(MechFrpData::mFrpBaseData* data)
{
    if (!data) return nullptr;
    const QVector<MechFrpData::mFrpBaseData*>& sibs =
        data->m_pParent ? data->m_pParent->children : frpMgr().roots();
    const int idx = sibs.indexOf(data);
    if (idx < 0) return data->m_pParent;
    if (idx + 1 < sibs.size()) return sibs[idx + 1];
    if (idx - 1 >= 0) return sibs[idx - 1];
    return data->m_pParent;
}