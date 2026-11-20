#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QLabel>

#include "mPanelBase.h"
#include "mMeshData.h"
#include "mMeshPanelBase.h"


namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mGlobalDimPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mGlobalDimPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mGlobalDimPanel();
        //设置默认尺寸 
        void setDefaultSize();
        //初始化默认尺寸
        void initSize();
        //预览全局尺寸
        void apply();
        //点击尺寸输入框后的按钮：剖分并显示白点预览
        void onSizeButtonClicked();
		/*
		* 设置数据
		*/

		void newGlobalDimPanel();

		void updatePanel();

		void editGlobalDimPanel(int meshId);

		double _globalSize = 2;
	private:

		//数据
		MeshPanelData::mMeshPanelBase* _meshPanelData = nullptr;
		//全局尺寸
		QLabel* _GlobalSizeLabel;
		QLineEdit* _GlobalSizeEdit;
		QToolButton* _GlobalSizeButton;

		//MeshData::mFunctionBase* _funcData = nullptr;
		FoldSection* m_rangeFold = nullptr;

	};
}