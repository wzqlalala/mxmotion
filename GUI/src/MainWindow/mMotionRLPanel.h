#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"
#include "mMotionRL.h"

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QToolButton>
#include <QCheckBox>
#include <QProgressBar>

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mMotionRLPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		mMotionRLPanel(QWidget* parent = nullptr);
		~mMotionRLPanel();

		void newRLPanel();
		void editRLPanel(int id = 0);
		void uneditRLPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionRLDataAboutToBeDestroyed(int id = -1);


	private:
		void connectSig();
		void disconnectSig();

	private slots:
		void slot_browseDatasetPath();
		void slot_browseModelPath();
		void slot_startTraining();
		void slot_stopTraining();

	private:
		QVBoxLayout* m_mainLayout;

		// 任务定义
		QLineEdit* m_edtTaskName;
		QComboBox* m_cmbAlgorithm;      // PPO / SAC / DDPG / TD3
		QComboBox* m_cmbEnvType;        // 单刚体 / 多体 / 柔性体

		// 状态空间
		QTableWidget* m_tableStateSpace;

		// 动作空间
		QTableWidget* m_tableActionSpace;

		// 网络参数
		QSpinBox* m_spbHiddenUnits;
		QSpinBox* m_spbNumLayers;
		QDoubleSpinBox* m_spbLearningRate;
		QDoubleSpinBox* m_spbDiscountFactor;

		// 训练参数
		QSpinBox* m_spbMaxEpisodes;
		QSpinBox* m_spbMaxSteps;
		QDoubleSpinBox* m_spbRewardScale;
		QCheckBox* m_chkUseCurriculum;

		// 路径与操作
		QLineEdit* m_edtDatasetPath;
		QToolButton* m_btnBrowseDataset;
		QLineEdit* m_edtSaveModelPath;
		QToolButton* m_btnBrowseModelPath;
		QToolButton* m_btnStartTrain;
		QToolButton* m_btnStopTrain;
		QProgressBar* m_progressTrain;

		// 数据对象
		MechMotionData::mMotionRL* m_data = nullptr;
	};
}
