#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 模板曲线应用对话框（功能弹窗，单例）
// 入口：报告功能区·模板组"模板曲线"按钮（mPostMenuContributor::buildReportPage，
// 原"模板编辑"脚手架更名）。设计图：1212/13~18（模板列表/绘图文件参数表/选项区）。
//
// 结构（任务 3.1~3.3）：
//   - 模板列表：templateRecents 时间序（旧→新，最新在底并默认选中），单选；
//     "打开模板"经文件对话框选择外部 .mxtpl 入列尾并选中；"删除模板"仅从
//     列表移除（文件不删，对齐设计"仅删除当前选中的模板"）；"勾"按钮=通道
//     映射对话框（任务 3.4/4.6~4.8，结果按模板缓存供应用复用）；"图层"按钮=
//     曲线图层管理对话框（任务 5.2，设计 D2）；
//   - 绘图文件参数区（需求图行式）：带边框容器，行=模板参数文件
//     （readTemplateMeta 提取）：绘图文件N：+ ☑当前参数值（勾=该行以当前
//     参数值参与替换）+ 只读路径框 + 浏览按钮（弹文件选择替换）+ 默认参数值
//     （只读灰底，模板保存时原始路径）；
//   - 选项带（需求图全宽横排，组间竖分隔线）：选项/显示（自动缩放X轴/
//     自动缩放Y轴/使用报告颜色，默认勾；"自动放置注释"需求图无 UI 项，
//     管线恒走 v1 默认勾）| 通道匹配检查（竖排 校验通道/选择最佳匹配/
//     跳过验证，默认跳过验证）| 模式设置（竖排 叠加(子项"不叠加基准线/
//     参考曲线"默认勾、灰显，仅叠加模式可用)/追加(默认)/替换）；
//   - 底部：关闭/应用(主蓝)。勾选状态随单例实例存活=会话内记忆（关闭重开
//     保持上次勾选）。
//
// "应用"管线（任务 4.1/4.3~4.5）：追加=appendTemplate；叠加/替换先经
// "选择起始页面"（4.2）取目标模板页面组（无组告警终止），再按模式分派；
// 校验通道策略按缓存映射表（无缓存则先弹通道映射对话框）装配 channelMap。
//
// 风格照 mApplyStyleDlg：无边框白卡片 + M徽标标题栏 + ✕，非模态、可拖动缩放，
// DlgGeom::bind("TemplateCurveDlg") 记忆大小/位置。
// =============================================================================

#include "PostCurveGlobal.h"
#include "mChannelMapCore.h"

//QT
#include <QWidget>

class QCheckBox;
class QLineEdit;
class QListWidget;
class QRadioButton;
class QSizeGrip;
class QToolButton;
class QVBoxLayout;

namespace PostCurve
{
	class POSTCURVE_EXPORT mTemplateCurveDlg : public QWidget
	{
		Q_OBJECT
	public:
		//入口：打开对话框单例（已打开则刷新列表并保持勾选状态）。
		//parent 仅首次创建时使用，内部沿 parentWidget 链走到顶层主窗口
		static void showInstance(QWidget* parent);

		mTemplateCurveDlg(const mTemplateCurveDlg&) = delete;
		mTemplateCurveDlg& operator=(const mTemplateCurveDlg&) = delete;

		//--- 应用管线消费的选项集（任务 4.1~4.3 从本类读取） ---
		struct ApplyOptions
		{
			QString templatePath;        //当前选中模板路径（空=未选）
			QStringList paramOverride;   //逐行当前参数值（勾选行参与替换，第 N 行对应参数 N）
			bool autoScaleX = true;      //自动缩放X轴
			bool autoScaleY = true;      //自动缩放Y轴
			bool useReportColors = true; //使用报告颜色（勾=模板颜色回放）
			bool autoAnnotation = true;  //自动放置注释（需求图无 UI 项，恒 v1 默认勾）
			int channelStrategy = 0;     //0跳过验证(默认)/1校验通道/2选择最佳匹配
			int autoFillMode = 0;        //自动填充档：需求图无 UI，恒0(无)（任务 4.9 功能保留）
			int applyMode = 1;           //0叠加/1追加(默认)/2替换
			bool skipBaselines = false;  //叠加模式子项"不叠加基准线/参考曲线"（勾=独立成层+判重，D10）
		};
		//收集当前界面选项（应用管线入口统一走这里取值）
		ApplyOptions collectOptions() const;

	private:
		explicit mTemplateCurveDlg(QWidget* parent);
		~mTemplateCurveDlg() override;

		//--- UI 构建 ---
		void buildUi();
		QWidget* buildTemplatePanel();    //左栏：工具行（蓝底占位钮）+ 模板列表 + 选项/显示组
		QWidget* buildParamPanel();       //右上：绘图文件参数区（行式，边框容器）
		QWidget* buildOptionPanel();      //选项带：选项/显示 | 通道匹配检查 | 模式设置（三组横排+竖分隔线）

		//--- 模板列表 ---
		//按 templateRecents 重建列表（时间序），保留仍存在路径的选中；
		//默认选中最新（尾部）——首开与删除后均落此规则
		void reloadTemplateList();
		//选中模板变化 → 重读元信息刷新参数表（失败清空并告警状态）
		void onTemplateSelected();
		//"打开模板"：文件对话框选 .mxtpl → addTemplateRecent（插尾去重）
		void onOpenTemplateClicked();
		//"删除模板"：removeTemplateRecent（仅列表项，文件不动）
		void onDeleteTemplateClicked();

		//--- 绘图文件参数区 ---
		//按参数清单重建参数行（默认参数值=模板保存时原始路径，当前参数值
		//初始=默认值，行首勾选默认全勾）
		void reloadParamTable(const QStringList& paramFiles);
		//清空参数行（宿主 Widget 延迟删；切模板/重载前调用）
		void clearParamRows();
		//浏览按钮 → 文件对话框换第 row 行当前参数值（同时确保该行勾选）
		void browseParamFile(int row);

	protected:
		//标题栏拖动/缩放手柄跟随（形态与 mApplyStyleDlg 一致）
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void mouseDoubleClickEvent(QMouseEvent* event) override;
		void resizeEvent(QResizeEvent* event) override;

	private slots:
		void onApplyClicked();          //应用（三模式 + 通道策略分派，4.1/4.3~4.5）
		void onChannelMapClicked();     //"勾"：预开通道映射对话框（3.4/4.6~4.8）

	private:
		static mTemplateCurveDlg* _instance;   //单例（实例存活=选项会话内记忆）

		bool _needCenter = true;       //无已存几何时首次打开居中于父窗口

		QWidget* _titleBar = nullptr;  //标题栏（拖动热区）
		QPoint _dragOffset;
		QPoint _pressPos;
		bool _dragging = false;
		QSizeGrip* _grip = nullptr;    //右下角缩放手柄（贴角跟随，不占布局行）

		QListWidget* _templateList = nullptr;    //模板列表（recents 时间序）

		//绘图文件参数行控件集（行式布局，随模板重载整体重建）
		struct ParamRow
		{
			QCheckBox* check = nullptr;    //☑当前参数值（勾=该行参与替换）
			QLineEdit* curEdit = nullptr;  //当前参数值只读路径框
		};
		QVBoxLayout* _paramBoxLayout = nullptr;  //参数区边框容器布局（行宿主挂此）
		QList<ParamRow> _paramRows;              //当前参数行控件（与参数清单对齐）
		QStringList _paramFiles;                 //当前模板参数清单（默认参数值=保存时原始路径）
		QStringList _currentParamValues;         //各行当前参数值（勾选行参与替换）
		QCheckBox* _autoScaleXCheck = nullptr;   //自动缩放X轴
		QCheckBox* _autoScaleYCheck = nullptr;   //自动缩放Y轴
		QCheckBox* _reportColorCheck = nullptr;  //使用报告颜色（默认勾）
		QRadioButton* _skipVerifyRadio = nullptr;   //跳过验证（默认）
		QRadioButton* _checkChannelRadio = nullptr; //校验通道
		QRadioButton* _bestMatchRadio = nullptr;    //选择最佳匹配
		QRadioButton* _overlayRadio = nullptr;      //叠加
		QRadioButton* _appendRadio = nullptr;       //追加（默认）
		QRadioButton* _replaceRadio = nullptr;      //替换
		QCheckBox* _skipBaselineCheck = nullptr;    //叠加子项：不叠加基准线/参考曲线
		//（需求图默认勾+灰显；"自动填充"需求图无 UI，autoFillMode 恒 0）

		//通道映射行缓存（"勾"预开或应用时生成）：按模板路径对齐，切模板即失效；
		//应用（校验通道策略）直接消费缓存，避免同一模板重复弹窗
		QList<mChannelMapCore::Row> _channelRows;
		QString _channelRowsTemplate;
	};
}
