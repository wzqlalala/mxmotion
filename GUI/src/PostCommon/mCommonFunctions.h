#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mcommon_global.h"

//MBasicFunction
#include "mBasicGUIEnum.h"

//QT
#include <QObject>
#include <QPushButton>
#include <QColor>
#include <QListWidget>
#include <QVector3D>
#include <QTreeWidgetItem>
#include <set>
#include <QTableWidget>
#include <QHeaderView>
#include <QPair>
#include <QPoint>
#include <QHash>
#include <QStackedWidget>
#include "mxr/view/REBUILD_PostRenderTypes.h"   // S5：G5 台账助手键（mxr::SceneObjectType，Qt-only 类型头）

namespace mxr { class REBUILD_PostRender; }

namespace PostCommon
{
	class POSTCOMMON_EXPORT mCommonFunctions : public QObject
	{
	public:
		/*
		* 设置颜色按钮的色块
		*/
		static void setButtonColor(QPushButton* button, const QColor& color);

		/*
		* 对传入的列表过滤，返回过滤后的列表（使用正则表达式匹配的过滤）
		*/
		static QStringList filterList(const QStringList& list1, const QString& filter);

		/*
		* 普通过滤，包含目标字符即可
		*/
		static QStringList filterList1(const QStringList& list1, const QString& filter);

		/*
		* 清空QStackedWidget
		*/
		static void clearStackedWidget(QStackedWidget* stackedwidge);

		/*
		* 清空QListWidget
		*/
		static void clearListWidget(QListWidget* listwidget);

		/*
		* 获取QListWidget的选中项
		*/
		static QStringList getSelectedList(QListWidget* listWidget);

		/*
		* 获取QListWidget的未选中项
		*/
		static QStringList getUnselectedList(QListWidget* listWidget);

		/*
		* 获取QListWidget选中项的索引
		*/
		static QList<int> getSelectedIndexes(QListWidget* listwidget);

		/*
		* 获取QListWidget选中项的名称
		*/
		static QString getSelectedString(QListWidget* listwidget);

		/*
		* 获取QListWidget所有Checked项的名称
		*/
		static QStringList getCheckedItemNames(QListWidget* listwidget);

		/*
		* 设置QListWidget所有项是否选中
		*/
		static void setListWidgetItemSelected(QListWidget* listwidget, bool isselect);

		/*
		* 设置QListWidget部分选中
		*/
		static void setListWidgetItemSelected(QListWidget* listwidget, const QStringList& names);

		/*
		* 设置QListWidget某些项Checked
		*/
		static void setListWidgetItemChecked(QListWidget* listwidget, const QStringList& names);

		/*
		* 创建有复选框的QListWidget
		*/
		static void addCheckableListWidget(QListWidget* listwidget, const QStringList& texts);

		/*
		* 设置QListWidget所有已选中的项未选中
		*/
		static void setListWidgetUnchecked(QListWidget* listwidget);

		/*
		* 设置QListWidget所有项未选中
		*/
		static void setListWidgetUnchecked1(QListWidget* listwidget);

		/*
		* 设置QListWidget最多只有一项选中
		*/
		static void setListWidgetItemState(QListWidget* listwidget, QListWidgetItem* item);

		/*
		* 判断QlistWidget中是否包含某项
		*/
		static bool isListWidgetContainItem(QListWidget* listwidget, const QString& itemtext);

		/*
		* 根据名称查找QListWidget中的项，并设置该项选中
		*/
		static QListWidgetItem* getListWidgetItem(QListWidget* widget, const QString& name);

		/*
		* 获取QListWidget选中项中的第一个
		*/
		static QListWidgetItem* getFirstSelectedItem(QListWidget* widget);

		/*
		* 获取QListWidget所有项的文本列表
		*/
		static QStringList getListWidgetText(QListWidget* lw);

		/*
		* 将ID列表转化成字符串
		* 参数type表示ID类型：节点(0)或单元(1)
		*/
		static QString transformListToString(const std::set<int> ids, int type);

		/*
		* 将ID列表转化成字符串（混合类型）
		*/
		static QString transformListToString(const std::set<int> nids, const std::set<int> eids);


		/*
		* 将ID字符串转化成列表
		* 返回值为ID类型：节点(0)或单元(1)
		* 参数ids为字符串解析的ID列表
		*/
		static int transformStringToList(const QString& idstring, QList<int>& ids);

		/*
		* 将ID字符串转化成列表
		* 返回值为ID类型：节点(0)或单元(1)
		* 参数ids为字符串解析的ID列表
		*/
		static int transformStringToList(const QString& idstring, std::set<int>& ids);

		/*
		* 将ID字符串转换成列表
		* 参数idstring为ID字符串
		* 参数nids为节点ID列表，参数eids为单元ID列表
		*/
		static bool transformStringToList(const QString& idstring, std::set<int>& nids, std::set<int>& eids);

		/*
		* 将ID字符串转化为列表
		* 形如1,2,5-10
		*/
		static bool transformMeshStringToSet(const QString& idstring, std::set<int>& mids);

		/*
		* 将Set<int>类型转化成Qlist<int>类型
		*/
		static bool transformSetToList(const std::set<int> id_set, QList<int>& id_list);
		/*
		 将Qlist<int>类型转化成Set<int>类型
		*/
		static bool transformListToSet(const QList<int> id_list, std::set<int>& id_set);
		/*
		* 将QVector<int>转换为std::set<int>
		*/
		static std::set<int> transformQVectorToSet(const QVector<int> vec);

		/*
		* 将QStringList转换成QString，用逗号分割
		*/
		static QString transformQStringListToQString(const QStringList& strlist);

		/*
		* 通过三点计算角度，p3为角点
		*/
		static double getThreePointAngle(const QVector3D p1, const QVector3D p2, const QVector3D p3);

		/*
		* 获取两点的中点坐标
		*/
		static QVector3D getCenterPointCoord(const QVector3D p1, const QVector3D p2);

		/*
		* 获取三角形的形心
		*/
		static QVector3D getCenterPointCoord(const QVector3D p1, const QVector3D p2, const QVector3D p3);

		/*
		* 获取四边形的形心
		*/
		static QVector3D getCenterPointCoord(const QVector3D p1, const QVector3D p2, const QVector3D p3, const QVector3D p4);

		/*
		* 获取两点沿轴向收缩后坐标
		* 参数p1和p2分别为收缩前的两点坐标
		* 参数val为收缩系数，范围为0-1，0表示不收缩，1表示同时收缩到中点
		*/
		static void getShrinkPosition(QVector3D& p1, QVector3D& p2, double val);

		/*
		* 设置某父节点下的所有子节点选中状态
		*/
		static void setChildrenItemState(QTreeWidgetItem* pitem, bool ischeck);

		/*
		* QVector3D 类型转换为 QString类型
		*/
		static QString transformVector3DToString(const QVector3D pos);
		/*
		* QVector(QVector3D) 类型转换为 QString类型
		*/
		static QString transformVector_Vector3DToString(const QVector<QVector3D> pos);

		/*
		*  QString 类型转换为 QVector3D类型
		*/
		static QVector3D transformStringToVector3D(const QString pos);
		/*
		*  QString 类型转换为 QVector(QVector3D)类型
		*/
		static  QVector<QVector3D> transformStringToVector_Vector3D(const QString pos);
		/*
		*  判断三维线段是否相交
		*/
		static bool LineIntersect3D(const QVector3D &line1P1, const QVector3D &line1P2, const QVector3D &line2P1, const QVector3D &line2P2, QVector3D &intersectP);

		/*
		* 获取变量的分量列表
		*/
		static QStringList getVariableComponents(const QString& var);

		/*
		* 根据变量名获取后处理渲染的类型，0-节点，1-单元
		*/
		static int getPostRendType(const QString& var);

		/*
		*初始化材料表格
		*/
		static void initMatTableWidget(QTableWidget* tw, const QStringList& headers);

		/*
		* 设置表格样式
		*/
		static void setTableWidgetStyle(QTableWidget* tw, const QStringList& headers);

		/*
		* 获取表格中的XY数据
		*/
		static void getTableWidgetDatas(QTableWidget* tw, QList<QPair<double, double>>& datas);

		/*
		* 设置表格数据
		*/
		static void setTableWidgetDatas(QTableWidget* tw, QList<QPair<double, double>>& datas);

		/*
		* 清空表格
		*/
		static void clearTableWidgetDatas(QTableWidget* tw);

		/*
		* 获取QTableWidget所有项的文本列表
		*/
		static QStringList getTableWidgetText(QTableWidget* tw);

		/*
		* 获取数据管理器中选中的所有项的数据名称（仅限名称在第一列）
		*/
		static QStringList getAllManagerSelectedName(QTableWidget* tw, QStringList& namelist);

		/*
		* 获取数据管理器中选中的所有项的数据名称（仅限名称在第二列）
		*/
		static QStringList getAllManagerSelectedNameInSecond(QTableWidget* tw, QStringList& namelist);

		/*
		*判断表格数据是否为空或者存在零行
		*/
		static bool HaveEmptyOrZeroRow(QTableWidget* tw);

		/*
		* 通过鼠标点击的位置获取弹窗的位置
		*/
		static void setWidgetPosByMouse(QWidget *parnetWidget, QWidget* popWidget);

		/*
		* 通过模型的最值和滑条的比例确定平面的经过的点
		*/
		static QVector3D caculatePlaneRange(float minx,float miny,float minz,float maxx, float maxy, float maxz, QVector3D normal,float ratio);

		/*
		* 通过模型的最值和平面经过的点确定滑条的比例
		*/
		static float caculatePlaneRatio(float minx, float miny, float minz, float maxx, float maxy, float maxz, QVector3D normal, QVector3D vertex);

		/*
		* 通过模型的最值确定模型的中心和最大半径
		*/
		static void caculateModelCenterAndRaduis(float minx, float miny, float minz, float maxx, float maxy, float maxz, QVector3D &center, float &radius);

		/*
		* 使用正则表达式提取括号中的内容
		*/
		static QString getParenthesesContent(const QString& text);

		/*
		* 复制表格内容到剪切板
		*/
		static void copyTableToBoard(QTableWidget* tw);

		/*
		* 将剪切板内容粘贴到表格中
		* 参数isone表示是否限制表格仅为一行（材料模块需要）
		*/
		static void pasteBoardToTable(QTableWidget* tw, bool isone);

		/*
		* 将单元类型字符串转换为int型，并获取单元包含的节点数、单元面数、每个面包含的节点数
		*/
		static void transformElementStringToInt(const QString& typestr, int vals[4]);

		/*
		* 将int型字符串转换为单元类型，并获取单元包含的节点数
		*/
		static void transformIntToElementString(int ElementEnum, QString& typestr);

		/*
		* 根据int型单元类型，获取每个单元的单元面数、每个面包含的节点数、所有面包含的节点总数
		*/
		static void getMeshFaceInfo(int type, int vals[3]);

		/*
		* 根据int型单元类型，获取该类型单元的维度
		*/
		static int getMeshDimension(int type);

		/*
		* 对于名称重复的，以name_1的形式进行重命名
		*/
		static QString getNonRepetiveName(const QString& name, const QStringList& namelist);

		/*
		* 获取数据类型字符串
		*/
		static QString getDataTypeStr(DATATYPE type);

		/*
		* 将QStringList排序进行后输出
		*/
		static::QStringList getSortStringList(const QStringList& name);

		/*
		* 计算三角形面积
		*/
		static double calculateTriangleArea(const QVector3D& A, const QVector3D& B, const QVector3D& C);

		/*
		* 计算四边形面积
		*/
		static double calculateQuadrilateralArea(const QVector3D& A, const QVector3D& B, const QVector3D& C, const QVector3D& D);

		/*
		* 计算单元面积
		*/
		static double getElementAera(QList<QVector3D> Points);

		/*
		* 获取场变量的节点输出
		*/
		static QStringList getFNodeOutPut(QStringList output);

		/*
		* 获取历程变量的节点输出
		*/
		static QStringList getHNodeOutPut(QStringList output);

		/*
		* 解析随程序部署的 python 可执行文件完整路径
		* 依次探测 <程序目录>/python/python.exe、<程序目录>/python/python_d.exe，返回第一个存在的；
		* 两者都不存在时返回空串，并把已探测的候选路径写入 searchedPaths（供失败提示，可为 nullptr）
		* 注：Conan python 包(3.7.17@mxsim/stable) Windows Release 变体部署 python.exe，
		* Debug 变体只有调试版 python_d.exe——回退探测才能让 Debug 构建启动导出进程
		*/
		static QString resolveBundledPythonExe(QStringList* searchedPaths = nullptr);
	};

	// —— S5 GUI 迁移（门面直接替换批）：G5 对象台账寻址助手 ——
	// 旧切面/矢量/标量/等值线/刚体墙直调族迁 addObject/updateObject/removeObject
	// 后，各面板按类型在门面台账寻 id（家族单例型矢量/标量/等值线/等值面寻首个
	// 条目 id 即可；removeObject 内部承担家族渲染删除+清台账）
	// 取指定类型的首个台账对象 id（无条目返回 -1）
	POSTCOMMON_EXPORT int firstSceneObjectId(mxr::REBUILD_PostRender* facade, mxr::SceneObjectType type);
	// 登记型寻址：无该类型台账条目时按 createParams addObject 登记并返回新 id
	//（D9 载体：类型取 createParams.type；RigidWall=登记型：id 承载 updateObject
	// 操作分派与 removeObject 寻址）
	POSTCOMMON_EXPORT int ensureSceneObjectId(mxr::REBUILD_PostRender* facade,
		const mxr::ISceneObjectParams& createParams);
	// 刚体墙操作型分派一步式（D9 载体）：ensureSceneObjectId(RigidWall) 寻/登 id 后
	// updateObject(RigidWallParams{operate, partNames})——partNames 非空=单墙操作，
	// 空/缺省=全墙操作（旧 UpdateOne/AllRigidWallOperate 语义承接）。
	// 注：收 std::set<QString>（Qt 5.13.2 无 QVector 迭代器对构造，集合内转）
	POSTCOMMON_EXPORT void updateRigidWallOperate(mxr::REBUILD_PostRender* facade, int operate,
		const std::set<QString>& partNames = std::set<QString>());
	// 家族删除：摘除该类型全部台账条目（旧 deleteVectorGraph/deleteScalarGraph/
	// deleteContourGraph()/deleteIsosurfaceByCoord 无参全删语义）
	POSTCOMMON_EXPORT void removeAllSceneObjects(mxr::REBUILD_PostRender* facade, mxr::SceneObjectType type);
	// 按外部槽位寻切面台账 id（externalIndex=addObject 建面时槽位；无匹配返回 -1）
	// ——旧 deleteCuttingPlane(num)/setIsShowCuttingPlane(num,b)/setPlaneData(num,...)
	// 的 num 槽位寻址承接
	POSTCOMMON_EXPORT int cuttingPlaneIdByIndex(mxr::REBUILD_PostRender* facade, int externalIndex);
	// 单实例型 ensure-or-update 一步式（unify-addobject-arch）：台账无该型→addObject
	// 建账（params 全量语义），有→updateObject(id, params) 改参。适用新六族单实例型
	//（MinMaxLabel/ExplodedView/FullCycle/PartArray/ColorLegend）；多条目型
	//（MeasureAnnotation）按 id 逐条寻址不走本助手（C3）
	POSTCOMMON_EXPORT void ensureUpdateSceneObject(mxr::REBUILD_PostRender* facade,
		const mxr::ISceneObjectParams& params);
}