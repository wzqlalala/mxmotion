#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//QT
#include <QString>
#include <QStringList>

#include "mcommon_global.h"

/*
* 后处理结果服务接口（前处理 → 后处理 的同步调用契约，docs/superpowers/plans/2026-10-10）
* 实现方：PostWindow::mPostBrowserWindow 在构造时经 postResultServiceRef() 自注册；
* 调用方：前处理（MainWindow/MxMech 层）判空调用，不 include 任何 PostWindow 头。
*
* key 契约：createPostResult 返回创建实例的 modelDataID 十进制串（值为模型数据ID，
* 仍按不透明句柄对待），与"第几次创建"绑定（同一文件多次导入各自返回不同key）、
* 进程内有效、永不复用、不持久化（唯一性由模型数据ID静态计数器保证，换ID路径已被
* clearViewportModel 的后处理守卫封死）。调用方只保存/透传，不得解析后调视口/模型
* 管理API——服务方保留将来更换铸造源（如UUID）的权利。实例删除的被动通知走总线信号
* mMechGlobalSignals::postResultRemovedSig（本接口为请求-应答通道，两者互补）。
*
* 线程契约：两方法都会创建/激活窗口部件与视口，仅限UI线程调用。
*/
namespace PostCommon
{
	//创建选项（预留扩展：后续新增选项在此追加，带默认值，老调用方签名零改动）
	struct PostResultCreateOptions
	{
		bool activateNow = true;   //false=仅创建不切换视口，留待后续 showPostResult 跳转

		//op2 伴随网格文件（.bdf/.nas，需拍平、不含 INCLUDE）。求解器输出的 bdf 与 op2
		//不同名且 op2 无内嵌几何时，同名探测兜不住，靠本字段显式指定。
		//优先级：显式 modelFilePath > resultFilePaths 内多选的 .bdf > 同名自动探测；
		//仅对 op2 生效（其余原生格式的读取库不消费伴随网格）。
		//传入路径必须存在且后缀为 .bdf/.nas，否则创建同步失败（空key，原因经
		//syncErrorDetail 带出）——不等到读取线程才失败，调用方可即时感知
		QString modelFilePath;
	};

	//showPostResult 错误码（不抛C++异常：跨DLL边界抛异常不符Qt惯例，用显式枚举返回；
	//枚举后续追加新值不影响已编译调用方）
	enum class PostResultError
	{
		Ok = 0,     //切换成功（含mtime变化后"删旧重建该key实例"再切换的情形，key仍有效）
		InvalidKey, //key未登记或已失效（模型被删/ID漂移，登记已移除）——报错，不自动重建
	};

	class IPostResultService
	{
	public:
		virtual ~IPostResultService() = default;

		/*
		* 创建后处理实例（新建后处理模型+树根+导入结果；失败内部回滚本次创建的模型）。
		* 成功返回实例唯一的不透明key（同一文件多次调用各自返回不同key）；失败返回空串。
		* options.activateNow=false 时仅创建不切换视口
		*
		* syncErrorDetail（可选出参，进函数即清空）：同步失败（未指定文件/恢复中拒绝/
		* 格式分发失败/显式modelFilePath非法）时填入中文原因，供调用方提示用户；
		* 为 nullptr 时行为不变。注意：原生格式（op2 等）的解析在读取线程内异步进行，
		* 失败发生在本函数返回之后，无法经此出参带出——错误文案见输出窗口，实例被
		* 回滚回收、key 经总线 postResultCreateFailedSig/postResultRemovedSig 作废
		*/
		virtual QString createPostResult(const QStringList& resultFilePaths,
			const PostResultCreateOptions& options = {},
			QString* syncErrorDetail = nullptr) = 0;

		/*
		* 按key幂等切换到对应后处理模型树/视口（视口激活自动联动 Ribbon/dock/树作用域）。
		* 返回 InvalidKey 时调用方应提示用户"该结果已失效或被删除"并作废入口，
		* 不自动重建——重新导入是用户的显式动作（再次 createPostResult 拿新key）
		*/
		virtual PostResultError showPostResult(const QString& resultKey) = 0;
	};

	/*
	* 全局服务引用（mPostBrowserWindow 构造时自注册；未注册时为 nullptr，调用方必须判空）
	*/
	POSTCOMMON_EXPORT IPostResultService*& postResultServiceRef();
}
