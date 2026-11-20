#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mModelEnum.h"

#include <QObject>
#include <QMetaObject>
#include <QList>
#include <functional>


#include <QString>
#include <QVector>
#include <vector>
#include <unordered_map>

using namespace MxFunctions;

namespace MechMainWindow
{
	/*
	* 管理器/引用面板的类型策略基类：五类对象（载荷/边界/预定义场/接触/模型修改）
	* 仅在数据访问与跳转接口上存在差异，通过Traits统一收敛，面板主体由模板基类实现。
	* 所有方法均为静态方法，默认实现覆盖载荷/边界/预定义场的通用通路，
	* 接触与模型修改通过特化覆盖映射读写方式。
	*/
	struct MMAINWINDOW_EXPORT mManagerPanelTraitsBase
	{
		//折叠块标题（如"载荷管理器"）
		static QString managerTitle();
		//引用折叠块标题（如"载荷引用"）
		static QString referenceTitle();
		//名称筛选占位文本（如"筛选载荷名称"）
		static QString nameFilterPlaceholder();
		//名称列表表头（如"载荷名称"）
		static QString nameHeader();
		//矩阵入口按钮文本（如"配置工况载荷"）
		static QString matrixEntryText();
		//矩阵表格对应的树节点类型（用于mManagerTableWidget::refreshTable）
		static BrowserTreeType matrixTreeType();

		//全部对象id（按id顺序）
		static std::vector<int> allIds();
		//对象名称
		static QString nameOf(int id);
		//对象类型名（用于筛选下拉与表格类型列）
		static QString typeNameOf(int id);

		//对象当前是否挂载到工况（引用面板勾选状态）
		static bool isReferenced(int id, int caseID);
		//挂载对象到工况（dataChange由引用面板触发）
		static void reference(int id, int caseID);
		//取消工况对对象的挂载
		static void unreference(int id, int caseID);

		//该工况是否支持引用该对象（类型兼容性校验）
		static bool isSupportedByCase(int id, int caseID);
		//该对象是否强制挂载到所有工况且不可取消（接触除过盈接触外）
		static bool isForceReferenced(int id);
		//对象是否被抑制（被抑制对象在引用面板中禁用勾选）
		static bool isSuppressedObject(int id);

		//该类型的编辑面板是否提供按工况缩放系数
		static bool hasScaleFactor(int id);
		//读取按工况缩放系数（不支持时返回1.0）
		static double scaleOf(int id, int caseID);
		//写按工况缩放系数（不支持时忽略）
		static void setScale(int id, int caseID, double value);

		//管理器面板删除按钮：发信号，由mTreeView统一完成数据删除与树同步
		static void emitDelete(const QVector<int>& ids);
		//引用面板可见时监听的增删信号连接（返回建立的数据刷新连接）
		static QList<QMetaObject::Connection> connectRefreshSignals(QObject* context, const std::function<void()>& refresh);

		//编辑跳转：caseID为0/-1表示无工况语境（管理器入口）
		static void editObject(int id, int caseID);
		//管理器面板编辑后选中管理器树下节点
		static void selectManagerItem(int id);
		//引用面板编辑后选中工况树下节点
		static void selectCaseItem(int caseID, int id);
		//取对象第一个挂载工况id（管理器编辑面板的工况语境；未挂载返回0）
		static int firstCaseId(int id);
	};
}
