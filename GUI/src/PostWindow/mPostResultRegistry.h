#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//QT
#include <QString>
#include <QStringList>
#include <QHash>
#include <QFileInfo>
#include <QDateTime>   //currentMonitoredMtime：lastModified 返回值仅前向声明，取秒需完整类型
#include <functional>

#include "mpostwindow_global.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostWindow
{
/*
* 结果跳转注册表（key → 后处理实例记录）
* key 取创建实例的 modelDataID 十进制串（漏斗 createAndRegisterResult 以
* QString::number(postModelID) 铸造），创建后冻结：唯一/永不复用由模型数据 ID 静态
* 计数器保证，换ID路径已被 clearViewportModel 的后处理守卫封死（后处理模型清空=
* 删除重导）；mtime"删旧重建该key实例"后 key 保留、记录内 postModelID 换绑到新模型
* ——此后 key 不再等于当前模型 ID，查模型一律走记录 postModelID，勿拿 key 当 ID 用。
* 进程内有效、不持久化；调用方（前处理经 IPostResultService）只保存/透传，
* 不得解析后调视口/模型管理 API。
*
* 纯数据类：无QObject/无UI依赖，可headless单测。条目因删除被移除时触发
* onKeyInvalidated（mPostBrowserWindow接线到总线 postResultRemovedSig，单点发射）；
* mtime"删旧重建该key实例"（key保留）走 takeByKey 静默取出+原key重登记，不触发回调。
*/
	class MPOSTWINDOW_EXPORT mPostResultRegistry
	{
	public:
		//一条"创建实例"记录（前处理上下文 ↔ 已创建后处理模型/入口视口）
		struct PostJumpRecord
		{
			QStringList filePaths;   //导入的结果文件列表（mtime检测 + 失效后人工重导入提示用）
			int postModelID = -1;    //该实例对应的后处理模型（mViewportModelManager 数据ID）
			int mainViewportID = -1; //导入时创建的入口云图视口（全局视口ID）
			qint64 fileMtime = 0;    //登记时受监控文件 mtime 最大值（重算覆盖检测，由 currentMonitoredMtime 铸造）
			QString modelFilePath;   //创建时显式传入的op2伴随网格（空=未显式指定）：
			                         //mtime"删旧重建该key实例"重放时沿用；进程内有效、不持久化

			/*
			* 全部受监控文件（filePaths + 非空 modelFilePath）当前 mtime 的最大值（秒）；
			* 缺失/不可取的文件计 0，空记录返回 0。
			* 登记侧（fileMtime 铸造）与 showPostResult 漂移检测侧必须同用本口径——
			* 两侧不对称会导致误重建（bdf 比 op2 新时每次跳转都删旧重建）或
			* 漏重建（bdf 重写但 mtime 仍早于 op2 时重放不触发）
			*/
			qint64 currentMonitoredMtime() const
			{
				qint64 mtime = 0;
				for (const QString& f : filePaths)
				{
					const qint64 t = QFileInfo(f).lastModified().toSecsSinceEpoch();
					if (t > mtime) mtime = t;
				}
				if (!modelFilePath.isEmpty())
				{
					const qint64 t = QFileInfo(modelFilePath).lastModified().toSecsSinceEpoch();
					if (t > mtime) mtime = t;
				}
				return mtime;
			}
		};

	/*
	* 登记（key 由漏斗以 QString::number(创建实例 modelDataID) 铸造；同key重复登记覆盖旧记录，不触发回调）
	*/
		void insert(const QString& key, const PostJumpRecord& record);

		/*
		* 按key查（未命中返回nullptr；记录归注册表所有，后续增删会使指针失效，勿持有）
		*/
		const PostJumpRecord* find(const QString& key) const;

		/*
		* 按后处理模型ID反查记录（未命中返回nullptr；会话保存采集原始导入清单用。
		* 记录归注册表所有，勿持有）
		*/
		const PostJumpRecord* findByPostModelID(int postModelID) const;

		/*
		* 静默取出并移除记录（不触发失效回调）：供"删旧重建该key实例"流程在删模型前
		* 先收回登记——删除链路会经 removeByPostModelID 反查作废，不先取出会误发失效信号；
		* 重建完成后按原key重新insert即可
		*/
		bool takeByKey(const QString& key, PostJumpRecord* outRecord = nullptr);

		/*
		* 作废单个key（showPostResult发现模型已删/ID漂移时调用）：移除并触发 onKeyInvalidated
		*/
		bool invalidateKey(const QString& key);

		/*
		* 按后处理模型ID反查作废（模型删除路径调用）：返回被移除的key列表（无匹配为空），
		* 每移除一条触发一次 onKeyInvalidated
		*/
		QStringList removeByPostModelID(int postModelID);

		int size() const { return _table.size(); }

		/*
		* 失效回调（默认空；PostWindow侧接线到总线信号，单测直接置lambda断言）
		*/
		std::function<void(const QString& key)> onKeyInvalidated;

private:
	QHash<QString, PostJumpRecord> _table;
	};
}
