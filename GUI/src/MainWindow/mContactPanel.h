#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>

//QT
#include <QWidget>
#include <QMdiSubWindow>

#include "mCalcLineEdit.h"

#include "mPanelBase.h"
#include "mPickWidgetController.h"

#include "mPhysicalFieldEnum.h"
#include "mMaterialPropBase.h"

using namespace MxFunctions;
namespace MechContactData
{
	class mContactBase;
}
namespace MechPreData
{
	class mViewportPrivate;
}
namespace MechMainWindow
{
    //截面属性面板
    class MMAINWINDOW_EXPORT mContactPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mContactPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mContactPanel();

        template<typename T>
        T* toType(MechContactData::mContactBase* base)
        {
            return dynamic_cast<T*>(base);
        };

        /*
        * 新建接触
        */
        void newContactPanel(ContactType type);

        /*
        * 编辑接触(从树上点击过来的)
        */
        void editContactPanel(int propertyID = 0);

        /*
        * 取消编辑接触
        */
        void uneditContactPanel();

        //接触数据即将被外部销毁（树上删除接触/删除全部接触）时的安全收尾：
        //必须在 deleteByPhysicalType 之前调用；传 -1 表示即将删除全部接触
        void onContactDataAboutToBeDestroyed(int contactID);


    private slots:

        //材料下拉框变化
        void slot_matComboBoxChanged(const QString& text);

        //表格数据变化
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

        // 刷新主面接触渲染
        void slot_commitMasterContactData();

        // 刷新从面接触渲染
        void slot_commitSlaveContactData();

        //刷新接触面渲染
        void slot_commitSelfContactData();
    protected:

        /*
        * 初始化界面
        */
        void initUI();

        /*
        * 加载数据到界面
        */
        void loadDataToUI();

        /*
        * 更新界面
        */
        void updatePanel();

        // 核心显隐逻辑
        void updateFoldVisibility(); // 控制大折叠块显隐
        void updateRowVisibility();  // 控制折叠块内部行的显隐
        void updateContactPairUI();//更新接触对ui
        void updateContactPairUI(MechContactData::mContactBase* _contactData);//过盈接触用更新接触对ui

        //部件透明度
        void resetPartAlpha(bool isClear = true);
        void transparentPart();

        //清空临时高亮
        void clearFaceHighlight();

        /*
        * 连接信号
        */
        void connectSig();

        /*
        * 断开连接信号
        */
        void disconnectSig();

        // 激活/失活通用拾取控件统一出口
        void deactivatePickAndCommit();

        // 从映射获取接触面 MixSet 数据
        void loadContactDataFromSet(MxFunctions::MapRelationType mapType, PickWidgetData& outData);

        // 增量提交单个字段到 contact API（isCreate=false），并静默记录对应的 Python 命令（不执行、不回显）
        void commitContactField(const QString& fieldKey, const QString& fieldValue);
        void commitContactField(const QString& fieldKey, const QStringList& fieldValues);

        // 从面是否处于节点表面模式（拾取类型仍为节点，集合选择为节点面集）：
        // 统一以数据对象 slave_region 判定（仅点面接触可为节点表面，
        // 从面拾取控件为面面/点面共用，须结合接触类型区分）
        bool slavePickIsNodeMode() const;

        // 反显"节点表面/单元面"下拉框：直接显示 slave_region，屏蔽信号不触发切换槽
        void syncSlaveRegionCombo();

        // 应用从面类型：刷新集合选择列表；从面激活时立即重设拾取工具栏类型过滤
        void applySlaveRegionPickTypes();

        // 延迟重建 extra 网格高亮渲染器，使颜色点图元最终置于 ui_root 顶层：
        // 节点点图元与主拾取高亮的点图元同挂 ui_root 且每次更新都重建追加到末尾，
        // 须在主渲染器最后一次更新（共享高亮服务 0ms 防抖）之后再重建一次本渲染器。
        // hops：排到防抖之后所需的 0ms 定时器跳数——dataChanged 路径防抖已启动(1)；
        // pickActiveChanged 早于 applyHighlight 启动防抖(2)。meshData 所有权转移
        void deferExtraMeshHighlightRebuild(mxr::CCSMRendWidget* widgetPtr, const char* colorKey,
            int r, int g, int b, MeshSelectMemoryData* meshData, int hops);

        // 控件激活后保证已拾取节点的颜色点不被激活流程重建的主拾取高亮点盖住
        void deferActivateNodePointTopUp(mPickWidgetController* picker);

        // 生成命令前缀：Contact("名称")
        QString contactCmdPrefix() const;

    protected:

        //接触数据
        MechContactData::mContactBase* _contactData = nullptr;

        //总布局
        QVBoxLayout* _mainLayout;

        // ================= 控件定义 =================
        // 为了消除间隙，必须持有 Label 的指针

        // 0. 类型
        QComboBox* cmb_type = nullptr;

        // 1. 接触对 - 通用拾取控件
        mPickWidgetController* pick_master = nullptr; // 主面
        mPickWidgetController* pick_slave = nullptr;  // 从面
        mPickWidgetController* pick_face = nullptr;   // 自接触面

        // 从面拾取类型（节点/单元面，仅点面接触显示，位于从面拾取控件之前）
        QComboBox* cmb_slaveRegion = nullptr;

        // 共用
        QToolButton* btn_swap = nullptr;      // 交换按钮

        //透明显示
        QCheckBox* chk_transparent = nullptr;
        QWidget* wd_pairContainer = nullptr;
        QWidget* wd_contactRow = nullptr;

        // 主面/从面/自接触面标签（跟随拾取控件显示）
        QLabel* lb_master = nullptr;
        QLabel* lb_slave = nullptr;
        QLabel* lb_face = nullptr;

        // 2. 接触参数
        QLabel* lb_smallSlip = nullptr;   QComboBox* cmb_smallSlip = nullptr;

        // 3. 法向 (都可以用）
        QLabel* lb_nodeOver = nullptr;      QComboBox* cmb_nodeOver = nullptr;
        QLabel* lb_nodeConst = nullptr;     QComboBox* cmb_nodeConst = nullptr;
        QLabel* lb_nodeScale = nullptr;     mCalcLineEdit* le_nodeScale = nullptr;
        QLabel* lb_nodePenalty = nullptr;     mCalcLineEdit* le_nodePenalty = nullptr;

        // 4. 切向
        //QCheckBox* chk_tangent = nullptr;
        QLabel* lb_tanFric = nullptr;       QComboBox* cmb_tanFric = nullptr;
        QLabel* lb_tanCoef = nullptr;       mCalcLineEdit* le_tanCoef = nullptr;

        // 5. 热接触
        QLabel* lb_heat = nullptr;
        QToolButton* btn_heat = nullptr;

        // 6. 接触间隙
        //QCheckBox* chk_gap = nullptr;
        QLabel* lb_gapVal = nullptr;        mCalcLineEdit* le_gapVal = nullptr;

        // 7. 几何调整
        //QCheckBox* chk_geom = nullptr;
        QLabel* lb_geomVal = nullptr;       mCalcLineEdit* le_geomVal = nullptr;

        //8.作用对象
        QComboBox* cmb_obj = nullptr;

        //9.定义
        QLabel* lb_masterd = nullptr;
        QLabel* lb_slaved = nullptr;

        //10.参数定义
        QComboBox* cmb_paradefine = nullptr;
        QLabel* lb_paraSize = nullptr;
        mCalcLineEdit* le_paraSize = nullptr;
        comboBoxWithPushBtn* combofunc = nullptr;

        //12.接触压力均匀性评估指标
        QCheckBox* chk_mp , * chk_sd, * chk_cv, * chk_pc, * chk_r;
    };
}
