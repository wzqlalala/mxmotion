#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSetData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mSetBase.h"
#include "mModelStruct.h"

namespace MechSetData
{
	class MXSETDATA_EXPORT mPanel : public mSetBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mPanel(int id, MxFunctions::SetType type, const QString& name = "", bool internal = false, bool isUserRename = false);

		/*
		* 仅用于输入输出流
		*/
		mPanel() {}

		/*
		* 析构函数
		*/
		~mPanel();

		/*
		* 拷贝分析集合
		*/
		mPanel* copySet(const int& id, const QString& name) override;

		/*
		* 获取集合数据
		*/
		std::vector<RenderPickResultData> getNodeData();

		/*
		* 设置集合数据
		*/
		void setNodeData(std::vector<RenderPickResultData> data);

		/*
		* 是否排序
		*/
		bool getOrder();

		/*
		* 设置排序
		*/
		void setOrder(bool order);

		/*
		* 获取面板集合类型（0: 等效辐射功率面板集合, 1: 单元/节点集合）
		*/
		int getPanelCollectionType() const;

		/*
		* 设置面板集合类型
		*/
		void setPanelCollectionType(int type);

		/*
		* 是否为单元/节点集合（选项二）
		*/
		bool isCollectNode() const;

		/*
		* 单元集名称
		*/
		QList<QString> elenames();

		/*
		* 设置单元集名称
		*/
		void setElenames(QList<QString> eleNames);
		void appendEle(QString eleName);
		void deleteEle(QString eleName);
		void clearEles();

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;


		friend QDataStream& operator>>(QDataStream& in, mPanel& obj);
		friend QDataStream& InStreamPanelV3(QDataStream& in, mPanel& obj);

		friend QDataStream& operator<<(QDataStream& out, mPanel& obj);

	private:
		//是否排序，从小到大
		bool _isOrder = false;

		std::vector<RenderPickResultData> _ids;

		
		QList<QString> _elenames;

	public:
	private:
		// 面板集合类型：0 等效辐射功率面板集合（仅单元集），1 单元/节点集合
		int _panelCollectionType = 0;

	public:
		QList<QString> nodeNames() const;
		void setNodeNames(const QList<QString>& nodeNames);
		void appendNodeName(const QString& nodeName);
		void removeNodeName(const QString& nodeName);
		void clearNodeNames();

		QList<QString> _nodeNames;

	};
}
