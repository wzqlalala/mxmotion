#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//QT
#include <QString>
#include <QToolButton>

namespace MechMainWindow
{
	class mGroupWidget;
	class RibbonPage;

	/*
	* 后处理独立页键（"显示"/"选择"/"模型"）：PostWindow 贡献的后处理页与前处理同名页
	* 完全分离，页键加 "#Post" 后缀避免与 m_StaWidMap 中前处理同名页冲突；
	* 页签文本仍为"显示"/"选择"/"模型"（页签名单用显示名），mMenuWidget 按当前工作
	* 模式把页签文本解析到对应页键（见 mMenuWidget::pageForTabText）
	*/
	inline constexpr const char* kPostDisplayPageKey = "显示#Post";
	inline constexpr const char* kPostSelectPageKey = "选择#Post";
	inline constexpr const char* kPostModelPageKey = "模型#Post";

	/*
	* Ribbon宿主接口（mMenuWidget实现）
	* 用途：后处理菜单页自mMenuWidget增量迁出（mPostMenuContributor），
	* 迁出的页面构建代码只依赖本接口构建Ribbon页，不依赖mMenuWidget本身；
	* 后续Ribbon基础类下沉共享层时，本接口随之下沉，实现方不变。
	*/
	class IRibbonHost
	{
	public:
		virtual ~IRibbonHost() = default;

		/*
		* 创建Ribbon页并注册页名（同名页已存在时直接复用，不重复创建）
		*/
		virtual RibbonPage* createRibbonPage(const QString& name) = 0;

		/*
		* 在指定页上新增选项组（页不存在返回nullptr）
		*/
		virtual mGroupWidget* addGroup(QString menuName, QString txtGroupTitle) = 0;

		/*
		* 统一设置工具按钮图标/悬浮提示，尺寸由按钮形态与主题档位管理
		*（QAbstractButton 兼容 QToolButton 与全自绘 mRibbonButton）
		*/
		virtual void setQToolButton(QToolButton* btn, QString name, QString image, QString describe,
			Qt::ToolButtonStyle style = Qt::ToolButtonTextUnderIcon) = 0;

		/*
		* 切换当前显示页（常驻页切固定页签，非常驻页占最右动态页签位）
		*/
		virtual void switchToPage(const QString& name) = 0;

		/*
		* 为控件安装悬浮提示（封装宿主侧 TooltipInstaller，贡献者无需依赖提示控件实现）
		*/
		virtual void installTooltip(QWidget* target, const QString& name, const QString& image,
			const QString& describe, int msec = 200) = 0;
	};
}
