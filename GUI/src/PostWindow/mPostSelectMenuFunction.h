#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//QT
#include <QObject>

namespace mxr
{
	class REBUILD_PostRend;
}

namespace PostData
{
	class mPostModelData;
}

namespace PostWindow
{
	class mPostIdLineEdit;

	/*
	* 后处理"选择"菜单功能（单例）：按照Id选择节点/单元、反向选择、清空拾取。
	* 自前处理 mSelectMenuFunction 分离移植（后处理"选择"页为独立Ribbon页，
	* PostWindow 不依赖 MainWindow），拾取容器/高亮刷新沿用前后处理共用设施
	* （mxr::SelectionDataContainer / MechPreData::UpdatePostWidgetHighlight）
	*/
	class mPostSelectMenuFunction : public QObject
	{
		Q_OBJECT

	public:
		/*
		* 获取单例指针
		*/
		static mPostSelectMenuFunction* getInstance();

		/*
		* 析构函数
		*/
		~mPostSelectMenuFunction();

		//节点ID输入行编辑框（由贡献者放入"选择"页网格组）
		mPostIdLineEdit* getNodeInputLineEdit() const { return _nodeLineEdit; }

		//单元ID输入行编辑框
		mPostIdLineEdit* getEleInputLineEdit() const { return _eleLineEdit; }

	public slots:
		//按照Id选择节点（输入框内容解析为ID，过滤掉结果数据中不存在的节点）
		void slot_nodeIDSelect();

		//按照Id选择单元（按结果数据反查所属部件，过滤不存在的单元）
		void slot_eleIDSelect();

		//反向选择（当前拾取的补集；按拾取类型分单元/节点）
		void slot_reverseSelect();

		//清空所有拾取
		void slot_clearPick();

	private:
		/*
		* 默认构造函数
		*/
		explicit mPostSelectMenuFunction(QObject* parent = nullptr);

		/*
		* 取当前后处理视口的模型数据（视口/GUI数据/结果库任一为空返回false）
		*/
		bool getPostModelData(mxr::REBUILD_PostRend*& postRend, PostData::mPostModelData*& modelData);

	private:
		mPostIdLineEdit* _nodeLineEdit = nullptr;
		mPostIdLineEdit* _eleLineEdit = nullptr;
};
}
