#pragma once
#include "mmainwindow_global.h"

#include <QWidget>
#include <QStackedWidget>
#include <QHash>
#include <QStringList>
#include <QList>
#include <QVector>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <QToolButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QTableWidget>
#include <QDialog>
#include <QTreeWidget>
#include <QMenu>
#include <functional>
#include <utility>

#include "mFrpBaseData.h"
#include "mFrpDerivedData.h"
#include "mCalcLineEdit.h"

QT_BEGIN_NAMESPACE
namespace Ui { class mFrpCompMaterialWindowClass; };
QT_END_NAMESPACE

using namespace MxFunctions;
using CalcMode = MxFunctions::mCalcLineEdit::CalcMode;

// 树项只挂数据节点指针：item <-> 节点互查全靠它（不再挂 id / 类型）
#define FRP_NODE_PTR_ROLE (Qt::UserRole)
Q_DECLARE_METATYPE(MechFrpData::mFrpBaseData*)

namespace MechFrpData { class mFrpManager; }   // 窗口仅以引用方式现取，不持有实例
/*====================================================================================
 * 四、属性页基类 / 属性面板调度
 * ==================================================================================*/
 // 所有右侧页面的统一基类
class mFrpCompPropertyPage : public QWidget
{
    Q_OBJECT
public:
    explicit mFrpCompPropertyPage(QWidget* parent = nullptr);
    // 直接以数据节点为参数（节点即数据）：load 从 node 反显，save 把控件写回 node
    virtual void loadData(MechFrpData::mFrpBaseData* data) = 0;
    virtual void saveData(MechFrpData::mFrpBaseData* data) = 0;
    // loadData 回填会触发控件信号，用此标记屏蔽“自动保存/相变形”，避免回填中途误触发
    void markLoading(bool b);
    bool isLoading() const;
    void clearDataBinding();
    void notifyChanged();
    // 新材料入库后：重建本页所有材料下拉，并把触发新增的那个下拉选中新材料
    virtual void onMaterialAdded();
    // 默认统一做“同一微观结构内比例类型一致”提示；各数值输入框的合法性由失焦即时校验负责，不在切页重复检查。
    virtual void validateData();
    static void outputWarn(const QString& object, const QString& explain);  // 向工程信息窗口输出一条“材料”模块警告
    int switchPhaseComboBox(FrpNodeType type);
    // 相类型下拉索引（基体0/掺杂1/空腔2/连续纤维3）-> 节点类型；非法索引返回 FrpNode_None
    static FrpNodeType phaseTypeOfIndex(int idx);
    // 取节点所属均质化模型下的“材料”分组（找不到返回 nullptr）；由界面层提供，数据层不再承载
    static MechFrpData::mFrpBaseData* materialGroupOf(MechFrpData::mFrpBaseData* context);
signals:
    void contentsChanged();          // 任一控件被编辑 -> 面板立即写回当前节点
    void morphRequested(FrpNodeType newType);   // 相页面：相类型被用户改成 newType，请求窗口把节点“变形”
    void requestAddMaterial();       // 相页面：请求窗口在所属均质化模型的“材料”分组下新增一个材料
protected:
    // —— 表单样板 ——
    QLabel* makeTitle(const QString& text);                          // 加粗标题
    QGridLayout* makeFormGrid();                                     // 统一页边距/间距的表单网格
    void addFormStretch(QGridLayout* grid, int row, int cols = 3);   // 网格底部弹簧
    QWidget* makeRow(const QList<QWidget*>& widgets, bool isStretch = false); // 一行横向排列的控件容器（末尾带弹簧）
    QWidget* makeRow();                                              // 空白占位行（用于拉开纵向间距）
    // “相类型”四选一下拉（已连变形信号）
    QComboBox* createPhaseCombo(int currentKind);
    // —— 材料下拉 + 新增小按钮（基体/掺杂/空腔/纤维统一实现）——
    QToolButton* makeMaterialAddButton();                            // 材料行末尾的“新增材料”小按钮
    void bindMaterialPicker(QComboBox* cmb, QToolButton* btn);       // 登记下拉并把按钮接到新增信号
    void refillMaterialCombos();                                     // 按所属模型材料分组重建所有材料下拉
    void selectMaterial(QComboBox* cmb, const QString& name);        // 按材料名选中，空名/找不到回“请选择”
    QString materialName(const QComboBox* cmb) const;                // 第0项“请选择”->空串，否则当前材料名
    void refillDistFuncCombo(QComboBox* cmb, const QString& currentName);  // 按当前模型“长细比函数”重建分布函数下拉并选中
    QString distFuncNameOf(const QComboBox* cmb) const;                    // “无”->空串，否则当前函数名
    QToolButton* makeTensorFileButton(QLineEdit* edit);  // 张量文件“浏览”按钮：选 xml，绝对路径填入只读输入框
    // —— 数值输入框失焦即时校验（程序回填不触发）：不合格发警告并清空当前输入框 ——
    void bindFractionEdit(QLineEdit* edit, const QString& explain);  // 分数：须为 0~1
    void bindPositiveEdit(QLineEdit* edit, const QString& explain);  // 长细比等：须为正数
    // 分数总和 ≤1：primary 为必填主分数；allExtras 为可能参与求和的全部附加框；
    // extrasProvider 返回当前勾选状态下真正参与求和的附加框；任一绑定框失焦即重算，超限清空刚编辑的框。
    // explain 中用 %1 占位实际总和。
    void bindFractionSum(QLineEdit* primary,
        const QList<QLineEdit*>& allExtras,
        const std::function<QList<QLineEdit*>()>& extrasProvider,
        const QString& explain);
    // —— 6 分量取向张量三角矩阵弹窗：传入主框当前串，确定返回新串（以 ';' 连接、末尾带 ';'），取消返回原串 ——
    QString showTensor6Dialog(const QString& stored);
    // —— 表格公共操作 ——
    static void renumberTableColumn(QTableWidget* table, int col);   // 某列按行重排 1..N（仅对已有 item 生效）
    static void removeSelectedOrLastRow(QTableWidget* table);        // 删选中行；无选中删最后一行；空表无操作
    void checkRatioConsistency();   // 同微观结构内掺杂/连续纤维/空腔比例类型不一致时发警告（仅提示，不联动）
    MechFrpData::mFrpBaseData* m_pCurData = nullptr;                                // 当前反显的节点
    QList<QComboBox*> m_materialCombos;                              // 本页所有材料下拉
    QComboBox* m_pPendingMaterialCombo = nullptr;                     // 最近点击“新增材料”的下拉
private:
    bool m_loading = false;
};
using PageFactory = std::function<mFrpCompPropertyPage* ()>;
class mMaterialPropertyPanel : public QWidget
{
    Q_OBJECT
public:
    explicit mMaterialPropertyPanel(QStackedWidget* stack, QWidget* parent = nullptr);
    void showPageForData(MechFrpData::mFrpBaseData* data);   // 切换：先存旧节点，再反显新节点（分组节点->空页）
    void flushCurrent();                   // 把当前页面数据立即写回当前节点
    void clearCurrent();                   // 删除场景：丢弃当前节点引用（不写回）
    void notifyMaterialAdded(MechFrpData::mFrpBaseData* data);   // 新材料入库后刷新当前基体页下拉
signals:
    // 相页面请求变形（窗口接到后改节点类型/名字并重新切页）
    void phaseMorphRequested(MechFrpData::mFrpBaseData* data, FrpNodeType newType);
    void materialAddRequested(MechFrpData::mFrpBaseData* data);
private:
    void registerPages();
    void installAutoSave(mFrpCompPropertyPage* page);
    void applyAnalysisLink(MechFrpData::mFrpBaseData* data, mFrpCompPropertyPage* page); // Analysis1 类型 -> 材料页约束
    MechFrpData::mFrpBaseData* m_pCurData = nullptr;
    QStackedWidget* m_pStack;
    QWidget* m_pEmptyPage;
    QHash<int, PageFactory> m_factories;
    QHash<int, mFrpCompPropertyPage*> m_pages;
};
/************************************************** Analysis1 **************************************************/
class Analysis1Page : public mFrpCompPropertyPage
{
    Q_OBJECT
public:
    Analysis1Page(QWidget* parent = nullptr);
    void loadData(MechFrpData::mFrpBaseData* data) override;
    void saveData(MechFrpData::mFrpBaseData* data) override;
private:
    QComboBox* m_cmbType;
    QComboBox* m_cmbHomoModel;
    QComboBox* m_cmbMultiPhase;
};
/************************************************** 材料 **************************************************/
class MaterialPage : public mFrpCompPropertyPage
{
    Q_OBJECT
public:
    explicit MaterialPage(QWidget* parent = nullptr);
    void loadData(MechFrpData::mFrpBaseData* data) override;
    void saveData(MechFrpData::mFrpBaseData* data) override;
    // 按所属 Analysis1 的类型(0请选择/1Mechanical/2Thermomechanical/3Thermal)约束材料种类与热膨胀区
    void applyAnalysisType(QString analysisType);
private:
    QComboBox* m_cmbKind;
    QComboBox* m_cmbType;
    mCalcLineEdit* m_editYoung;
    mCalcLineEdit* m_editPoisson;
    mCalcLineEdit* m_editDensity;
    // 热膨胀区（散排在主 grid 中，仅 Thermomechanical 时整体显示；标签也需成员指针以便一起显隐）
    QCheckBox* m_chkThermalExp;
    QLabel* m_lbRefTemp;
    mCalcLineEdit* m_editRefTemp;
    QLabel* m_lbThermalExp;
    QComboBox* m_cmbThermalExp;
    QLabel* m_lbA1;
    mCalcLineEdit* m_editA1;
};
/************************************************** 相-基体 **************************************************/
class MicroDefPage : public mFrpCompPropertyPage
{
    Q_OBJECT
public:
    explicit MicroDefPage(QWidget* parent = nullptr);
    void loadData(MechFrpData::mFrpBaseData* data) override;
    void saveData(MechFrpData::mFrpBaseData* data) override;
private:
    QComboBox* m_cmbPhase;
    QComboBox* m_cmbMaterial;
    QToolButton* m_btnMaterial;
};
/************************************************** 相-掺杂 **************************************************/
class InclusionPage : public mFrpCompPropertyPage
{
    Q_OBJECT
public:
    explicit InclusionPage(QWidget* parent = nullptr);
    void loadData(MechFrpData::mFrpBaseData* data) override;
    void saveData(MechFrpData::mFrpBaseData* data) override;
private slots:
    void slot_shapeChanged(int index);
    void slot_methodChanged(int index);
    void slot_ratioChanged(int index);
    void slot_advancedToggled(bool on);
    void slot_viewTensor();
private:
    QComboBox* m_cmbPhase;
    QComboBox* m_cmbMaterial;
    QToolButton* m_btnMaterial;
    QWidget* m_wMaterial;
    QComboBox* m_cmbRatio;
    QLineEdit* m_editRatio;
    QComboBox* m_cmbShape;
    QLabel* m_lbAspect;
    QLineEdit* m_editAspect;
    QLabel* m_lbDist;
    QComboBox* m_cmbDistFunc;
    QLabel* m_lbCount;
    QLineEdit* m_editDiscreteCount;
    QComboBox* m_cmbDefMethod;
    QWidget* m_wFixedAngle;
    QLineEdit* m_editTheta;
    QLineEdit* m_editPhi;
    QWidget* m_wTensor;
    QLineEdit* m_editTensor;
    QPushButton* m_btnView;
    QWidget* m_wTensorFile;
    QLineEdit* m_editTensorFile;
    QCheckBox* m_chkAdvanced;
    QRadioButton* m_radioCoating;
    QRadioButton* m_radioFiber;
    QComboBox* m_cmbFeatMat;
    QToolButton* m_btnFeatMat;
    QLabel* m_lbFeatFrac;
    QLineEdit* m_editFeatFrac;
    QList<QWidget*> m_coatingWidgets;
    QLabel* m_lbFiberFrac1;       // 纤维簇分数标签（随“比例”在质量/体积分数间切换）
    QLineEdit* m_editFiberFrac1;
    QLabel* m_lbFiberFrac2;       // 纤维分数标签（随“比例”在质量/体积分数间切换）
    QLineEdit* m_editFiberFrac2;
    QLineEdit* m_editFeatAspect;
    QList<QWidget*> m_fiberWidgets;
};
/************************************************** 相-空腔 **************************************************/
class CavityPage : public mFrpCompPropertyPage
{
    Q_OBJECT
public:
    explicit CavityPage(QWidget* parent = nullptr);
    void loadData(MechFrpData::mFrpBaseData* data) override;
    void saveData(MechFrpData::mFrpBaseData* data) override;
private slots:
    void slot_shapeChanged(int);
    void slot_methodChanged(int);
    void slot_surfaceToggled(bool);
    void slot_viewTensor();
private:
    QComboBox* m_cmbPhase;
    QLineEdit* m_editVolumeFraction;
    QComboBox* m_cmbShape;
    QLabel* m_lbAspect;
    QLineEdit* m_editAspect;
    QLabel* m_lbDist;
    QComboBox* m_cmbDistFunc;
    QLabel* m_lbCount;
    QLineEdit* m_editDiscreteCount;
    QComboBox* m_cmbDefMethod;
    QWidget* m_wFixedAngle;
    QLineEdit* m_editTheta;
    QLineEdit* m_editPhi;
    QWidget* m_wTensor;
    QLineEdit* m_editTensor;
    QPushButton* m_btnView;
    QWidget* m_wTensorFile;
    QLineEdit* m_editTensorFile;
    QCheckBox* m_chkSurface;
    QWidget* m_wSurfaceMat;
    QComboBox* m_cmbSurfaceMat;
    QToolButton* m_btnSurfaceMat;
    QLineEdit* m_editSurfaceFrac;
    QList<QWidget*> m_surfaceWidgets;
};
/************************************************** 相-连续纤维 **************************************************/
class FiberPage : public mFrpCompPropertyPage
{
    Q_OBJECT
public:
    explicit FiberPage(QWidget* parent = nullptr);
    void loadData(MechFrpData::mFrpBaseData* data) override;
    void saveData(MechFrpData::mFrpBaseData* data) override;
private slots:
    void slot_coatingToggled(bool checked);
private:
    QComboBox* m_cmbPhase;
    QComboBox* m_cmbMaterial;
    QToolButton* m_btnMaterial;
    QComboBox* m_cmbRatio;
    QLineEdit* m_editRatio;
    QLineEdit* m_editTheta;
    QLineEdit* m_editPhi;
    QCheckBox* m_chkCoating;
    QComboBox* m_cmbCoatMat;
    QWidget* m_wCoatMat;
    QToolButton* m_btnCoatMat;
    QLineEdit* m_editCoatFrac;
    QList<QWidget*> m_coatingWidgets;
};
/************************************************** RVE **************************************************/
class RvePage : public mFrpCompPropertyPage
{
    Q_OBJECT
public:
    explicit RvePage(QWidget* parent = nullptr);
    void loadData(MechFrpData::mFrpBaseData* data) override;
    void saveData(MechFrpData::mFrpBaseData* data) override;
private:
    void addRow(int row = -1, const MechFrpData::RveLayer* init = nullptr);  // 添加层（init 非空时按保存值回填）
    void createAngleDialog();
    void createTensorDialog();
    QStringList microNames();                        // 本均质化模型下全部微观结构N 的名字
    MechFrpData::mFrpBaseData* findMicro(const QString& name);     // 按名字找微观结构N；找不到返回 nullptr
    bool orientEditable(const QString& name);        // 该微观结构是否“恰好 1 基体 + 1 掺杂”（纤维取向才可编辑）
    void applyOrientEditable(int row, bool reset);   // 按规则启用/禁用某行纤维取向；reset=true 时不满足则重置为“请选择”
    QLabel* m_lbStruct;
    QComboBox* m_cmbMode;
    QComboBox* m_cmbStructure;
    QPushButton* m_btnAdd;       // 顶部：增加
    QPushButton* m_btnRemove;    // 顶部：减少
    QWidget* m_wbtn;
    QTableWidget* m_table;
    QDialog* m_dlgAngle;
    QLineEdit* m_editTheta;
    QLineEdit* m_editPhi;
    QDialog* m_dlgTensor;
    QVector<QLineEdit*> m_tensorEdits;
    int m_editRow;
};
/************************************************** 失效准则 **************************************************/
class FailurePage : public mFrpCompPropertyPage
{
    Q_OBJECT
public:
    explicit FailurePage(QWidget* parent = nullptr);
    void loadData(MechFrpData::mFrpBaseData* data) override;
    void saveData(MechFrpData::mFrpBaseData* data) override;
private:
    // 标签文案（Stress/Strain 切换）与数据字段的桥接表：edit/mp 为空表示该条目仅参与文案切换
    struct FieldBridge {
        QLabel* label = nullptr;
        QLineEdit* edit = nullptr;
        QString MechFrpData::mFrpFailureData::* mp = nullptr;
        QString stressText;
        QString strainText;
    };
    void buildFieldBridges();        // 构造结束后建立桥接表
    void applyModelUi(int idx);      // 按模型索引切换参数页并控制“类型”行显隐（构造/loadData/切换共用）
    void loadSharedParams(MechFrpData::mFrpFailureData* data, int model);   // 复用页（Tsai Hill 2D / Azzi）按模型加载各自独立数据
    void storeSharedParams(MechFrpData::mFrpFailureData* data, int model);  // 复用页按模型把控件值存回各自独立数据
    QComboBox* m_cmbModel;
    QLabel* m_lbType;
    QComboBox* m_cmbType;
    QStackedWidget* m_stackParams;
    QLabel* m_lbComp;    QComboBox* m_cmbComp;
    QLabel* m_lbTens;    QLineEdit* m_editTens;
    QLabel* m_lbComp2;   QLineEdit* m_editComp2;
    QLabel* m_lbAxialTens;   QLineEdit* m_editAxialTens;
    QLabel* m_lbAxialComp;   QLineEdit* m_editAxialComp;
    QLabel* m_lbInPlaneTens; QLineEdit* m_editInPlaneTens;
    QLabel* m_lbInPlaneComp; QLineEdit* m_editInPlaneComp;
    QLabel* m_lbShear;       QLineEdit* m_editShear;
    QLabel* m_lbAxialTensT;    QLineEdit* m_editAxialTensT;
    QLabel* m_lbInPlaneTensT;  QLineEdit* m_editInPlaneTensT;
    QLabel* m_lbShearT;        QLineEdit* m_editShearT;
    QLabel* m_lbXt;  QLineEdit* m_editXt;
    QLabel* m_lbYt;  QLineEdit* m_editYt;
    QLabel* m_lbZt;  QLineEdit* m_editZt;
    QLabel* m_lbXYs; QLineEdit* m_editXYs;
    QLabel* m_lbYZs; QLineEdit* m_editYZs;
    QLabel* m_lbXZs; QLineEdit* m_editXZs;
    QLabel* m_lbAzAxTens; QLineEdit* m_editAzAxTens;
    QLabel* m_lbAzAxComp; QLineEdit* m_editAzAxComp;
    QLabel* m_lbAzIpTens; QLineEdit* m_editAzIpTens;
    QLabel* m_lbAzIpComp; QLineEdit* m_editAzIpComp;
    QLabel* m_lbAzShear;  QLineEdit* m_editAzShear;
    QLabel* m_lbWuAxTens; QLineEdit* m_editWuAxTens;
    QLabel* m_lbWuAxComp; QLineEdit* m_editWuAxComp;
    QLabel* m_lbWuIpTens; QLineEdit* m_editWuIpTens;
    QLabel* m_lbWuIpComp; QLineEdit* m_editWuIpComp;
    QLabel* m_lbWuShear;  QLineEdit* m_editWuShear;
    QLabel* m_lbWuCouple; QLineEdit* m_editWuCouple;
    QLineEdit* m_editWu3D;
    QPushButton* m_btnWu3DView;
    QVector<QLineEdit*> m_wu3dEdits;
    QVector<FieldBridge> m_fields;
    int m_lastModel;     // 上一次选中的模型索引，切模型时用于把控件值存回旧模型字段
};
/************************************************** 长细比函数 **************************************************/
class AspectRatioFunPage : public mFrpCompPropertyPage
{
    Q_OBJECT
public:
    explicit AspectRatioFunPage(QWidget* parent = nullptr);
    void loadData(MechFrpData::mFrpBaseData* data) override;
    void saveData(MechFrpData::mFrpBaseData* data) override;

private slots:
    void slot_importData();
    void slot_exportData();
    void slot_plotCurve();     // 绘制曲线

private:

    void addRow(int row = -1, const std::pair<QString, QString> init = std::pair<QString, QString>());  // init 非空时按保存值回填
    bool collectPoints(QVector<double>& xs, QVector<double>& ys) const;  // 收集并校验表格 X/Y

    QTableWidget* m_table;
    QPushButton* m_btnAdd;       // 顶部：增加
    QPushButton* m_btnRemove;    // 顶部：减少
    QPushButton* m_btnImport;    
    QPushButton* m_btnExport;    
    QPushButton* m_btnPlot;      
};

/*====================================================================================
 * 六、外层窗口
 * ==================================================================================*/
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mFrpCompMaterialWindow : public QWidget
    {
        Q_OBJECT

    public:

        mFrpCompMaterialWindow(QWidget* parent = nullptr);
        ~mFrpCompMaterialWindow();

    private:
        void initUI();
        void initConnect();
        void initMaterialTree();
        QTreeWidgetItem* buildTreeItems(MechFrpData::mFrpBaseData* baseData, QTreeWidgetItem* parentItem);
        QTreeWidgetItem* findItemByData(MechFrpData::mFrpBaseData* baseData);
        MechFrpData::mFrpBaseData* dataOfItem(QTreeWidgetItem* item);   // 树项 -> 数据节点
        MechFrpData::mFrpBaseData* currentData();                       // 当前选中树项 -> 数据节点

        // —— 数据驱动：不持有 manager，操作时现取全局模型引用 ——
        MechFrpData::mFrpManager& frpMgr() const;
        // 全量按全局数据树重建视图；selectNode 给出重建后应选中的节点（nullptr 显示空页）
        void rebuildTree(MechFrpData::mFrpBaseData* selectNode);
        // 界面层新增规则（数据层不再提供 ruleOf）
        struct AddRule { bool allowed; bool asSibling; FrpNodeType newType; };
        static AddRule ruleForNode(MechFrpData::mFrpBaseData* selected);
        // 删除前计算重建后应选中的节点（下一兄弟 -> 上一兄弟 -> 父）
        MechFrpData::mFrpBaseData* siblingAfterRemove(MechFrpData::mFrpBaseData* data);
        void showTreeContextMenu(const QPoint& pos);            // 树右键菜单
        void refreshAddRemoveState();
        void doPhaseMorph(MechFrpData::mFrpBaseData* baseData, FrpNodeType newType);  // 相节点“变形”：改类型/名字并重切页
        void addAnalysisRootItem();
        void clearChildrenOfCurrent();
        void startInlineRename(QTreeWidgetItem* item);   // 树项内联重命名
        void deleteNodeItem(MechFrpData::mFrpBaseData* data);          // 通用删除：删指定节点自身（根/实例/相）
        void addByRule();                                // 右键“新增”：按原规则新增
        void addPhaseToCurrent(FrpNodeType phaseKind);           // 右键微观N：新增指定相
        void duplicateCurrent();                         // 右键“复制”

    private slots:
        void slot_showSelectedNodePage(const QItemSelection& selected, const QItemSelection& deselected);
        void slot_removeAnalysis();
        void slot_commitInlineRename(QTreeWidgetItem* item, int column);
        void slot_addMaterialForPhase(MechFrpData::mFrpBaseData* phaseData);
        void slot_exportComposite();
        void slot_readMfFinished();

    private:
        Ui::mFrpCompMaterialWindowClass* ui;

        mMaterialPropertyPanel* m_pPropertyPanel = nullptr;
        bool m_inlineRenaming = false;   // 是否处于菜单触发的内联重命名（拦截程序化改名）

    };

}
