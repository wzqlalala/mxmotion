#include "mPostResultServiceProvider.h"

//后处理结果服务实例持有（mPostBrowserWindow构造时注册，见头注释）
namespace PostCommon
{
	IPostResultService*& postResultServiceRef()
	{
		static IPostResultService* inst = nullptr;
		return inst;
	}
}
