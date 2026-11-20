#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"
#include "mMotionControlInput.h"

#include <QWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QStackedWidget>

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mMotionControlInputPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		mMotionControlInputPanel(QWidget* parent = nullptr);
		~mMotionControlInputPanel();

		void newControlInputPanel();
		void editControlInputPanel(int id = 0);
		void uneditControlInputPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionControlInputDataAboutToBeDestroyed(int id = -1);


	private:
		void connectSig();
		void disconnectSig();

		//把当前界面参数写入数据对象（类型/系数/曲线点/时间范围）
		void writeParamsToData();

		//按数据对象完整还原界面
		void restoreUIFromData();

	private slots:
		void slot_functionTypeChanged(int index);

	private:
		QVBoxLayout* m_mainLayout;

		// 基础信息
		QLineEdit* m_edtName;
		QComboBox* m_cmbFunctionType;

		// 堆叠容器：常量 / 线性 / 二次 / 三次 / 自定义曲线
		QStackedWidget* m_stkParams;

		// 常量
		QDoubleSpinBox* m_spbConstValue;

		// 线性: a + b*t
		QDoubleSpinBox* m_spbLinA;
		QDoubleSpinBox* m_spbLinB;

		// 二次: a + b*t + c*t^2
		QDoubleSpinBox* m_spbQuadA;
		QDoubleSpinBox* m_spbQuadB;
		QDoubleSpinBox* m_spbQuadC;

		// 三次: a + b*t + c*t^2 + d*t^3
		QDoubleSpinBox* m_spbCubicA;
		QDoubleSpinBox* m_spbCubicB;
		QDoubleSpinBox* m_spbCubicC;
		QDoubleSpinBox* m_spbCubicD;

		// 自定义曲线表格
		QTableWidget* m_tableCurve;

		// 时间范围
		QDoubleSpinBox* m_spbTimeStart;
		QDoubleSpinBox* m_spbTimeEnd;

		// 数据对象
		MechMotionData::mMotionControlInput* m_data = nullptr;

		// 表格批量填充期间抑制 itemChanged 写回
		bool m_blockTableSignal = false;
	};
}
