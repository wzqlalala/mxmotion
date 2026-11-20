#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 模板曲线管理（.mxtpl 模板文件读写 + templateRecents，单例）
//
// 模板曲线特性（openspec add-template-curve）的文件与清单层：
//   - D1 文件格式："MXTPL" + formatVersion + 元信息(模板名/保存时间/软件版本)
//     + 参数文件清单(去重路径) + 页子树块（saveTemplateSubtree 整块，自描述
//     magic/版本/参数/页数/节点体；文件级参数清单为权威，块内为序列化自冗余）；
//   - D2 模板历史：Settings.ini [PostCurve] templateRecents 路径清单，按保存
//     时间正序（旧→新，最新在尾=列表默认选中项），去重、上限 20；
//   - 兼容纪律：只增字段不改语义，未知版本拒绝（对齐 mxses 兼容策略精神）。
// =============================================================================

#include "PostCurveGlobal.h"

//QT
#include <QObject>
#include <QString>
#include <QStringList>

class QWidget;

namespace PostCurve
{
	class CurveDocument;   //前置声明（实体在 CurveCore/CurveDocument.h）

	class POSTCURVE_EXPORT mCurveTemplateManager : public QObject
	{
		Q_OBJECT
	public:
		static mCurveTemplateManager* getInstance();

		mCurveTemplateManager(const mCurveTemplateManager&) = delete;
		mCurveTemplateManager& operator=(const mCurveTemplateManager&) = delete;

		//模板元信息（文件头，模板列表/标题栏显示用）
		struct TemplateMeta
		{
			QString name;          //模板名（另存为时的文件名去扩展名）
			qint64 savedMsecs = 0; //保存时间（1970 起毫秒，与 recents 排序无关联）
			QString appVersion;    //保存时软件版本（QCoreApplication::applicationVersion）
		};

		//--- 模板文件（纯函数，无状态） ---
		//收集 pageIds 页子树 → D1 文件落盘（元信息=templateName/当前时间/当前版本）。
		//页不存在或子树序列化失败返回 false（err 带原因，可为空）
		static bool saveTemplateFile(const CurveDocument* doc, const QList<int>& pageIds,
			const QString& templateName, const QString& filePath, QString* err = nullptr);

		//读文件头：元信息 + 参数清单（即模板参数表行集；不全量解析页子树）。
		//meta/paramFiles 均可为 nullptr（只验文件合法性）；缺失/magic 或版本
		//不符返回 false
		static bool readTemplateMeta(const QString& filePath, TemplateMeta* meta,
			QStringList* paramFiles, QString* err = nullptr);

		//整文件恢复：页子树块交 restoreTemplateSubtree（paramOverride 第 N 行
		//覆盖参数 N，缺行回填模板保存时原始路径=不替换语义）；restoredPageIds
		//带出新页 id（可为空）
		static bool restoreTemplateFile(const QString& filePath, CurveDocument* doc,
			const QStringList& paramOverride, QList<int>* restoredPageIds,
			QString* err = nullptr);

		//--- templateRecents（Settings.ini [PostCurve]，D2） ---
		//记录路径清单：时间正序（旧→新，最新在尾，模板列表底部默认选中）
		QStringList templateRecents() const;
		//登记：已存在先去重再插尾（"打开模板"外部路径同样入列尾）；上限 20
		//截最旧；空路径忽略
		void addTemplateRecent(const QString& filePath);
		//仅移除列表项（模板文件本身不删，对齐设计"仅删除当前选中的模板"）
		void removeTemplateRecent(const QString& filePath);

		//--- 保存链路（报告功能区"保存模板"入口，任务 2.1） ---
		//整文档保存流程：另存为对话框（过滤器"模板文件 (*.mxtpl)"）→
		//saveTemplateFile（全部页面，元信息=所选文件名去扩展名）→ 登记 recents。
		//取消返回空串；无页或落盘失败弹告警后返回空串。parent 供对话框定位
		QString saveCurrentAsTemplate(const CurveDocument* doc, QWidget* parent);

		//设置文件注入（测试用；默认 applicationDirPath()/../Setting/Settings.ini，
		//对齐 mMenuWidget 最近打开的既有约定）
		void setSettingsFile(const QString& iniPath);

	signals:
		//清单变化（模板曲线对话框列表刷新用）
		void sig_recentsChanged();

	private:
		explicit mCurveTemplateManager(QObject* parent = nullptr);

		static mCurveTemplateManager* _instance;   //单例

		QString _settingsFile;   //recents 所在 ini（空=默认路径）
	};
}
