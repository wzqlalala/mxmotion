#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPhysicalFieldEnum.h"
#include "mMotionContact.h"

#include <memory>
#include <set>
#include <string>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>
#include <QComboBox>
#include <QStackedWidget>
#include <QDoubleSpinBox>
#include "mCalcLineEdit.h"
#include <QCheckBox>
#include <QColor>

#include "mPanelBase.h"

namespace MechPreData
{
	class mViewportModel;
}
namespace MechMotionData
{
    class mMotionContact;
}
namespace MechMainWindow
{

	//3D 接触面板
	class MMAINWINDOW_EXPORT mMotionContactPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMotionContactPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionContactPanel();

		/*
		* 新建接触
		*/
		void newMotionContactPanel(MechMotionData::MotionContactType type);

		/*
		* 编辑接触(从树上点击过来的)
		*/
		void editMotionContactPanel(int id = 0);

		/*
		* 取消编辑接触
		*/
		void uneditMotionContactPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionContactDataAboutToBeDestroyed(int id = -1);


	private:

        void refreshBodyComboBoxes();

        void updateUIFromData();

        /**
         * @brief 高亮单个运动体
         */
        void highlightBody(int bodyId, const std::string& key, const QColor& color);

        /**
         * @brief 清除动作/基本体的临时高亮
         */
        void clearBodyHighlights();

		/*
		* 连接信号
		*/
		void connectSig();

		/*
		* 断开连接信号
		*/
		void disconnectSig();

        // 更新摩擦控件可见性
        void updateFrictionVisibility();

        // 更新动作体高级控件可见性
        void updateActionFacetVisibility();

        // 更新基本体高级控件可见性
        void updateBaseFacetVisibility();

	private slots:
		//切换视口	
		void slot_subWindowActivated(QMdiSubWindow* subWindow);

		//拾取信号
		void slot_finishedPick();
	protected:

		//真正的接触数据
	       MechMotionData::mMotionContact* m_motionContactData;

		//总布局
		QVBoxLayout* m_mainLayout;

        // ==================== 界面控件 ====================
        // 1. 基础信息层
        QComboBox* m_cmbContactType;    ///< [CAD接触 / 小面接触]

        // 2. 接触对象层
        QComboBox* m_cmbActionBody;     ///< 动作体选择
        QToolButton* m_btnPickActionBody; ///< 拾取动作体
        QComboBox* m_cmbBaseBody;       ///< 基本体选择
        QComboBox* m_cmbActionGeometry; ///< 动作体接触几何（凸包/精确网格）
        QComboBox* m_cmbBaseGeometry;   ///< 基本体接触几何（凸包/精确网格）
        QToolButton* m_btnPickBaseBody; ///< 拾取基本体

        // 3. 基本参数层
        MxFunctions::mCalcLineEdit* m_spbStiffness;         ///< 刚度
        MxFunctions::mCalcLineEdit* m_spbForceExponent;     ///< 力指数
        MxFunctions::mCalcLineEdit* m_spbMaterialDamping;   ///< 材料阻尼
        MxFunctions::mCalcLineEdit* m_spbPenetrationDepth;  ///< 最大穿透深度

        // 4. 摩擦参数层
        QCheckBox* m_chkFrictionEnabled;        ///< 启用库仑摩擦
        MxFunctions::mCalcLineEdit* m_spbStaticFriction;    ///< 静摩擦系数
        MxFunctions::mCalcLineEdit* m_spbDynamicFriction;   ///< 动摩擦系数

        // 5. 高级参数层
        FoldSection* m_foldAdvanced;            ///< 高级参数折叠面板
        QComboBox* m_cmbForceModel;             ///< 力模型
        MxFunctions::mCalcLineEdit* m_spbForceModelTol;     ///< 力模型公差

        // 6. 动作体小平面参数
        FoldSection* m_foldActionFacet;         ///< 动作体小平面折叠面板
        QCheckBox* m_chkActionUseFacetFile;     ///< 使用细分文件
        MxFunctions::mCalcLineEdit* m_spbActionDistTol;     ///< 距离公差
        MxFunctions::mCalcLineEdit* m_spbAngleTol;          ///< 角度公差
        QCheckBox* m_chkActionUseMaxFacet;      ///< 启用最大小平面大小
        MxFunctions::mCalcLineEdit* m_spbActionMaxFacet;    ///< 最大小平面大小

        // 7. 基本体小平面参数
        FoldSection* m_foldBaseFacet;           ///< 基本体小平面折叠面板
        QCheckBox* m_chkBaseUseFacetFile;       ///< 使用细分文件
        MxFunctions::mCalcLineEdit* m_spbBaseDistTol;       ///< 距离公差
        QCheckBox* m_chkBaseUseMaxFacet;        ///< 启用最大小平面大小
        MxFunctions::mCalcLineEdit* m_spbBaseMaxFacet;      ///< 最大小平面大小

  //拾取高亮数据改变的连接
  QMetaObject::Connection conn_geo;
	};

}
