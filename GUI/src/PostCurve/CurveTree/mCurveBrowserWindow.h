#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 曲线树窗口（dock 容器，单例，UI设计图02左侧）
//
// 布局（UI设计图01.2 按节点类型筛选）：顶部为页下拉框（列出曲线页，快速切换，
// 与视图显示栏页下拉同款样式；选中即树上选中该页节点，联动切换对应视图页），
// 其下为节点类型筛选按钮行（单选：全部/曲线/图例/注释/基准线/坐标轴/页眉页脚，
// 对应设计稿面板00~06），再下为名称搜索框（占位"搜索"），
// 底部为曲线结构树（mCurveTreeView）。
// dock 注册：key "CurveTree"，在 mMechMainWindow 构造中经
// mDockLayoutManager::registerDock 注册，默认停靠在模型树右侧 tab；
// pch 导入创建图表、树右键新建图表等场景经 activateDock("CurveTree") 激活。
//
// 本类只是容器：树的交互逻辑见 mCurveTreeView，数据见 CurveDocument。
// =============================================================================

#include "../PostCurveGlobal.h"

//QT
#include <QWidget>

class QButtonGroup;
class QComboBox;
class QLineEdit;

namespace PostCurve
{
	class mCurveTreeView;

	class POSTCURVE_EXPORT mCurveBrowserWindow : public QWidget
	{
		Q_OBJECT
	public:
		/*
		* 获取单例指针（首次调用时创建，objectName "CurveTreeDock"，最小宽 265）
		*/
		static mCurveBrowserWindow* getInstance();

		/*
		* 析构函数
		*/
		~mCurveBrowserWindow();

		/*
		* 获取曲线树（外部联动选中节点时使用，如 mCurveViewportFactory）
		*/
		mCurveTreeView* getCurveTreeView() const;

	private slots:
		//切换语言（骨架阶段预留空实现）
		void slot_changeLanguage(int language);
		//页下拉框选择变化 -> 树选中该页节点（经选择中枢联动切换对应视图页）
		void slot_pageComboChanged(int index);
		//筛选按钮行单选切换（按钮id 0~6）-> 配置树的类型过滤代理
		void slot_filterChanged(int buttonId);
		//搜索框文本变化 -> 树按节点名称过滤
		void slot_searchChanged(const QString& text);

	private:
		/*
		* 默认构造函数（私有，单例）
		*/
		mCurveBrowserWindow(QWidget* parent = 0);

		/*
		* 创建节点类型筛选按钮行（7个单选 QToolButton，文字替代图标）
		*/
		QWidget* createFilterBar();

		/*
		* 刷新页下拉框选项（页增删/改名/选中联动后重列曲线页，
		* 并回显当前选中节点所属页）
		*/
		void refreshPageCombo();

	private:
		//单例指针
		static mCurveBrowserWindow* _instance;
		//页下拉框（与视图显示栏页下拉同款：列出曲线页，快速切换）
		QComboBox* _pageCombo = nullptr;
		//曲线结构树
		mCurveTreeView* _treeView;
		//筛选按钮组（单选；id 0~6：全部/曲线/图例/注释/基准线/坐标轴/页眉页脚）
		QButtonGroup* _filterGroup = nullptr;
		//名称搜索框
		QLineEdit* _searchEdit = nullptr;
	};
}
