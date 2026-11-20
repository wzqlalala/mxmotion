#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 通道选择器（数据集通道四级联动共享控件）
//
// "从文件创建曲线"/"定义曲线"文件页的通道选择形态抽出为共享控件，供参数栏
// x/y 数据页"更改…"（设计图13 更改曲线）等新宿主复用：
//   数据文件下拉（+"导入..."按钮，支持 pch/csv/txt）-> 工况 -> 结果类型 ->
//   对象 -> 分量 四级联动。
//
// R22 csv/txt（AsciiColumn 数据集）同口径适配：四级退化为三级——工况列整列
// 隐藏，结果类型=[Unknown]、对象=[Block 1] 唯一项自动选中，分量=列名清单。
// 分流收口在 asciiFileMode() 一处判定，内嵌与弹出（pickChannel）两宿主形态
// 共用同一套填充逻辑、同时生效。
//
// 使用规约：
//   - 数据现取现用 mCurveDataSourceStore（每次用每次取，不缓存 file()/index()
//     返回的内部指针——任何 addFile 都可能使 QHash value 指针失效）；
//   - 控件只维护选择态，不写文档模型；落模由调用方在确认动作里完成；
//   - 两种宿主形态：内嵌（addWidget 进页面）/ 弹出（pickChannel，Qt::Popup
//     白卡片 + 确定/取消，现建现销无状态残留）。
// =============================================================================

#include "../PostCurveGlobal.h"

//QT
#include <QWidget>
#include <QString>

class QComboBox;
class QListWidget;

namespace PostCurve
{
	//通道选择值（id 形态五元组）。与曲线来源五元组（CurveSeriesData 的
	//sourceFile/subcase/schema/object/component 显示串）经 fromModelTexts/
	//subcaseText/objectText 互转，显示串格式与两弹窗写模型时一致
	struct ChannelSelection
	{
		QString filePath;      //数据文件路径（mCurveDataSourceStore 登记键，规范化）
		int subcaseId = 0;     //工况 id（ascii 数据集恒为 asciiSubcaseId()，UI 不显示该级）
		QString schema;        //结果类型（如 "ACCELERATION"；ascii 恒 "Unknown"）
		int pointId = 0;       //对象 id
		QString component;     //分量（"MAG|X-Trans" 形态；ascii=列名）
		bool complete = false; //各级是否均有选中行（value() 填写；ascii 不含隐藏的工况级）

		bool isValid() const { return complete; }
	};

	class POSTCURVE_EXPORT mCurveChannelSelector : public QWidget
	{
		Q_OBJECT
	public:
		explicit mCurveChannelSelector(QWidget* parent = nullptr);

		//程序回填（模型->控件）：逐级定位，某级失配停在该级取首行并联动下级
		//（回显旧来源时文件/通道可能已不在仓库，不整体失败）。
		//回填过程不发信号风暴，结束时发一次 sig_selectionChanged
		void setValue(const ChannelSelection& sel);
		ChannelSelection value() const;

		//模型来源显示串 -> ChannelSelection（static 纯函数，页面接线共用）。
		//"Subcase %1"/"Point id %1" 取串尾数字反解 id（宽容前缀差异）
		static ChannelSelection fromModelTexts(const QString& filePath, const QString& subcaseText,
			const QString& schema, const QString& objectText, const QString& component);
		//显示串格式（与"从文件创建曲线"/"定义曲线"写模型一致）
		static QString subcaseText(int subcaseId);
		static QString objectText(int pointId);

		//弹出式通道选择（按钮入口）：Qt::Popup 白卡片锚定 anchor 下方（越出
		//屏幕底部时改到上方），确定且选择完整时写 *out 返回 true；
		//取消/点外关闭返回 false。卡片与内部选择器现建现销
		static bool pickChannel(QWidget* anchor, const ChannelSelection& current,
			ChannelSelection* out);

	signals:
		//任一级选择变化（用户操作/导入完成/setValue 回填完成）。
		//级联重填全在控件内，宿主收到信号后读 value() 即可
		void sig_selectionChanged();

	private slots:
		void onFileComboChanged();
		void onSubcaseRowChanged();
		void onSchemaRowChanged();
		void onObjectRowChanged();
		void onImportFile();

	private:
		void buildUi();
		//当前文件是否 ascii 列数据（R22 分流收口点）：四级列表的 ascii 三级化
		//（工况列隐藏/Unknown/Block 1/列名）全部以本判定为唯一分流依据，内嵌
		//与弹出两宿主形态共用
		bool asciiFileMode() const;
		//各级重填：读上级当前选择 + 共享仓库现取索引，blockSignals 阻断 +
		//非空时选首行；不级联（下级重填由 onXxx 槽显式驱动）
		void refillFileCombo(const QString& selectPath);
		void refillSubcases();
		void refillSchemas();
		void refillObjects();
		void refillComponents();
		//导入成功续接（原 onImportFile 内联刷新链路抽出共用）：重建下拉选中
		//新文件并联动四级列表 + 发一次 sig_selectionChanged
		void refillAfterImport(const QString& filePath);
		//当前各级选中值（无选中行：id 级返回 0，文本级返回空串）
		int currentSubcaseId() const;
		QString currentSchema() const;
		int currentPointId() const;

		QComboBox* _fileCombo = nullptr;
		QWidget* _subcaseCol = nullptr;   //工况列容器（ascii 数据集整列隐藏用）
		QListWidget* _subcaseList = nullptr;
		QListWidget* _schemaList = nullptr;
		QListWidget* _objectList = nullptr;
		QListWidget* _componentList = nullptr;
	};
}
