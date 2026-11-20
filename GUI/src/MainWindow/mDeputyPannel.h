#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <QObject>
#include <memory>
#include "mPanelBase.h"
#include "mMixSet.h"

//BasicData
#include "mDeputyBase.h"
#include "mDeputyManager.h"

#include "mDeputy.h"

using namespace MechConnectorData;
using namespace MechSetData;
namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mDeputyPannel : public mPanelBase
	{
		Q_OBJECT

	public:
		mDeputyPannel(QWidget* parent = nullptr);
		~mDeputyPannel();

		/*
		* 创建连接副
		*/
		void createDeputy(MxFunctions::DeputyType type);

		/*
		* 编辑连接副
		*/
		void editDeputy(const int& deputyId);

		/*
		* 取消连接副编辑
		*/
		void unEditDeputy();

	private:
		/*
		* 连接副面板
		*/
		void deputyPanel(MxFunctions::DeputyType type);

		/*
		* 连接信号
		*/
		void connectSig(mDeputyPanelBase* panel);

		/*
		* 断开信号
		*/
		void disconnectSig(mDeputyPanelBase* panel);

        /*
        * 创建内部集合
        */
        mMixSet* createSet();

        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> convertPickDataToMap(const std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& pickData);


	public slots:
		/*
		* 编辑完成
		*/
		void slot_finishDeputyEdit();

        /*
        * 拾取完成
        */
		void slot_finishDeputyPick();

	private:
		QVBoxLayout* _mainLayout;

		std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData;
	};

}
