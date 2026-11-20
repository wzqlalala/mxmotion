#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QMap>
#include <QString>
#include <QStringList>
#include <QJsonObject>

namespace StrengthReport
{
	//绑定键人话字典（T1/T2 共用，00-绑定键整体方案 §4）：三族规范键（model_stats./
	//case./manual_fields.）→界面说明（措辞=UI-05 B 类模板原词）。未知键/族外前缀
	//返回空串——调用方显示键名原文，不静默失实
	MPOSTWINDOW_EXPORT QString bindingKeyDescription(const QString& bindKey);

	//字典全部键清单（与 bindingKeyDescription 同一数据源；步③建议键名格下拉候选）
	MPOSTWINDOW_EXPORT QStringList bindingKeyKeys();

	/*
	* 项目级配置持久化（FR-45/D-12/B10/H-15，03 §4.5）：
	* 落盘 appDir/Report/strength-report/<项目标识>/project_config.json（QJsonDocument），
	* 配置按项目/模型标识隔离存储与回显，切换项目互不污染（D-12）；独立于既有面板配置（E10）。
	* appDir/Report/ 既有习惯区，规避发布形态根目录无写权限（B10）。
	* load/save 均 ok/err 返回语义（A-08，勿复制恒 return true 形态）；
	* 配置损坏（JSON 解析失败）→回退空配置+err 告警，不阻断对话框打开（H-15）。
	*/
	class MPOSTWINDOW_EXPORT mStrengthReportProjectConfig
	{
	public:
		//项目标识净化唯一实现（CB-13：Config 配置目录与 Collector 截图目录必须共用同一规则，
		//否则模型名含非法字符/点号时两处目录名对不上）：Windows 文件名非法字符替换 '_'、
		//结尾点剥离（Windows 目录名不得以点结尾）；段内点保留（v1.2 与截图目录既有口径一致）
		static QString sanitizeProjectId(const QString& raw);

		//切换项目标识后需重新 load（各项目配置互不共享——D-12）
		void setProjectId(const QString& id) { _projectId = id; }
		QString projectId() const { return _projectId; }

		//配置文件绝对路径（项目标识做文件名净化：Windows 非法字符替换 '_'）
		QString configFilePath() const;

		bool load(QString& err);   //文件不存在=首次使用场景，按空配置成功返回
		bool save(QString& err);

		//上次规则模板包路径与输出路径记忆（02 §3.3：打开对话框即已加载、已回显）
		QString containerPath;
		QString outputPath;
		//页签①人工补录（FR-45；element_type=U-06 补录；project_subject="XXX"对象名 D-24②；
		//software_name 首期单键单值、页签①可录入覆盖 D-24①）
		QString softwareName;
		QString constraintDesc;
		QString loadCaseDesc;
		QString loadType;
		QString calcMethod;
		QString elementType;
		QString projectSubject;
		QString missingPolicy;                  //缺项策略（页签④三选一，默认留"-"并告警；
		                                        //二期 FR-211：值随数据集 missing_policy_override 发射）
		QMap<QString, QString> imagePaths;      //截图槽键→图片路径（页签③抓取/导入产物回显）
		QMap<QString, QString> imageCaptions;   //截图槽键→题注文本（A-10 题注录入持久化）

		//二期批次 B（FR-241 录入版，09 §4.3）：页签②两组录入面持久化（D-12 按项目隔离）。
		//值对象键=数据集契约键（QJsonDocument 序列化原样落盘）；null=未录。
		//当前模型不存在的历史条目保留在配置但不发射（08 §2.3），页签②黄字提示
		QMap<QString, QJsonObject> caseResults;     //工况名→results 四键（stress_peak/
		                                            //displacement_max/buckling_mode/reserve_factor）
		QMap<QString, QJsonObject> materialValues;  //材料名→{max_stress, reserve_factor}

		//——二期批次 C（FR-231/232 去水印 + FR-221 半自动试点，09 §4.3）——
		bool watermarkOff = false;                  //页签④截图水印全局开关：false=带水印（默认，
		                                            //一期行为不变）；true=新抓取截图在抓取瞬间隐藏
		                                            //logo 水印（08 §2.4 Q2-4=A，开关即时生效）
		bool semiAutoGrab = false;                  //页签③半自动抓取试点开关：默认关闭（关闭=
		                                            //页签③与一期同交互，BUG-04 未闭环风险闸门）
		QMap<QString, int> slotFrameMap;            //槽键→目标帧标识（D3-3：首版仅帧标识，默认空；
		                                            //变量对齐=用户主窗口手动切换）；帧号≥1
		QMap<QString, bool> imageWatermarks;        //逐槽追溯：截图落槽时的水印档（true=无水印档/
		                                            //false=带水印档；Q2-4 开关即时生效的逐槽追溯）

		//——三期批次 B（13 §4.2 配置扩容：批次 B 先落配置面，批次 C 消费）——
		//槽键→四元组映射（FR-332：{"case":..,"frame":int,"var":..,"compo":..}）。
		//PA-12 口径订正：旧 slotFrameMap int 形态→四元组的 load 迁移不在批次 B 实现
		//(slotFrameMap/slotMappings 当前各自独立装载、互不迁移），随批次 C 消费面
		//接线时一并定义迁移语义（var="SolidColor" 帧维语义=网格图槽帧维应用，O3-2/R3-9）；
			//autoCapture=false 时批次 C 链不消费（默认开，D-06；无持久化键时才落默认）
		QMap<QString, QJsonObject> slotMappings;
		bool autoCapture = true;                    //自动截图总开关（D-06 默认开：仅"配置文件
		                                            //无 autoCapture 键"时生效——load 类型门
		                                            //见键存在即覆盖，存量显式 false 仍被尊重；
		                                            //与 semiAutoGrab 合一：load 见旧
		                                            //semiAutoGrab=true 且无 autoCapture 键时迁移
		                                            //值，O3-2）

		//E2.2：load 时清洗的非法历史录入值条目数（黄字提示用；不持久化，仅 load 期填充）
		int lastSanitizedCount = 0;

		//LG-07：load 时丢弃的非法 slotMappings 条目数（帧号非法且非 SolidColor 无帧
		//合法形态——黄字提示用；不持久化，仅 load 期填充，E2.2 同精神消静默丢弃）
		int lastMappingDroppedCount = 0;

	private:
		QString _projectId;
	};
}
