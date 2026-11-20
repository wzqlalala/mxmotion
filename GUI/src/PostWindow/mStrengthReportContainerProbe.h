#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QJsonDocument>
#include <QObject>
#include <QStringList>

class QProcess;
class QTimer;

namespace StrengthReport
{
	/*
	* 加载态栏用容器快检（03 §4.5 mStrengthReportContainerProbe）：
	* ①结构层（纯 C++ 同步快检）：zip 目录结构扫描（EOCD+中央目录条目名清单，零解压），
	*   核对 manifest.json/rules/rules.json 在位——"容器损坏"快速负例；
	* ②内容层（子进程异步，全量校验仍由引擎 M1 负责）：<python> tdr_validate.py <container>
	*   （批次 1 工具，三层校验同套逻辑）→文本输出解析状态分类：
	*   已加载/校验失败 N 条/容器损坏/版本不符（错误文案含"版本不符"判据，FR-12④ 提示口径）；
	* ③槽位清单层（子进程异步）：<python> tdr_rules.py get <container> <tmp.json> 取出规则层
	*   （02 §2.3 步骤 3 的 get 子命令），QJsonDocument 解析供页签①文本槽预览与页签③
	*   图片槽位清单驱动（T-08：UI 不写死槽数）。
	* 子进程协议（批次 1 落地实况，05-实施记录批次 1）：tdr_validate 退出码 0=通过/1=校验失败；
	*   "[validate] 校验失败：<msg>"+"  - <条目>" bullets；"[validate] 模板=<template_id> | ..."。
	*/
	class MPOSTWINDOW_EXPORT mStrengthReportContainerProbe : public QObject
	{
		Q_OBJECT
	public:
		enum Status
		{
			NotLoaded,        //未选择规则模板包
			Loaded,           //已加载·校验通过
			IntegrityFailed,  //加载失败·容器损坏（结构层/manifest 完整性）
			VersionMismatch,  //加载失败·格式版本不符（C3-05/H-15）
			ValidationFailed  //加载失败·规则三层校验未过（FR-13）
		};

		struct Result
		{
			Status status = NotLoaded;
			QString message;             //状态行主文案（含模板名/失败原因）
			QStringList errors;          //校验失败条目（ValidationFailed 时 N 条，条目级定位）
			QString templateName;        //manifest.template_id（validate 输出）
			QJsonDocument rulesDocument; //rules/rules.json 内容（Loaded 时有效；槽位清单驱动）
		};

		explicit mStrengthReportContainerProbe(QObject* parent = nullptr);
		~mStrengthReportContainerProbe() override;   //CB-11：析构 kill 运行中校验子进程（Qt5 QProcess 析构不终止子进程，防孤儿 python）；先断开信号防析构期回调

		//异步探测（结构层同步完成，内容层经两段子进程链）；进行中重复调用忽略并置 err。
		//datasetJsonPath=GUI 侧数据集 JSON（tdr_validate 第 2 可选参数——S5-2 instance_key
		//数组存在性探测需要 cases 键在位，缺数据集会误报"校验失败 1 条"，实测批次 1 容器确认）；
		//空串=不传数据集（仅作降级兼容）
		void probe(const QString& containerPath, const QString& datasetJsonPath, QString& err);
		bool isRunning() const;
		QString containerPath() const { return _containerPath; }

		//tdr_tools 磁盘定位（两级确定性，与 Runner python 探测同构）：
		//①发布形态 appDir/../tools/tdr_tools；②开发树 appDir/../../resource/scripts/tdr_tools
		static QString resolveToolsDir(QString& err);

	signals:
		void finished(const StrengthReport::mStrengthReportContainerProbe::Result& result);

	private:
		//zip 结构层快检：EOCD 定位+中央目录条目名枚举（只读文件头/目录区，零解压零子进程）
		bool structuralCheck(const QString& path, QStringList& entryNames, QString& err);
		void runValidateStep();
		void runRulesStep();
		void emitResult(const Result& result);
		void startWatchdog();   //CB-11：校验段子进程看门狗（与引擎 Runner 同 120s 口径）

		QString _containerPath;
		QString _datasetJsonPath;
		Result _pending;
		QProcess* _process = nullptr;
		QProcess* _activeProcess = nullptr;  //CB-11：当前运行段子进程（校验/取规则两段共用，供看门狗与析构定位）
		QTimer* _watchdog = nullptr;         //CB-11：120s 看门狗（python 挂死→kill，状态机不再永久阻塞）
		bool _timedOut = false;              //CB-11：超时标志（区分超时失败与正常失败文案）
		bool _running = false;
	};
}
