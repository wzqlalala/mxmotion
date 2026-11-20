#pragma once
#pragma execution_character_set("utf-8")

#include <QString>
#include "mBasicEnum.h"
#include "guifoundation_global.h"
namespace MechMainWindow
{
	class GUIFOUNDATION_EXPORT mViewTreeFunction
	{
	public:
		/*
		*  删除所有模型及其视口
		*/
		static void deleteAllViewportModel();

		/*
		* 基于当前的模型新建视口
		*/
		static void newViewport();

		/*
		* 新建视口根据类型(Viewport3D/ViewportPost3D)
		*/
		static void newViewport(MxFunctions::ViewportType type, QString name = "");

		/*
		* 新建模型和视口
		*/
		static int newViewportModel(bool fullScreen = false);

		/*
		* 新建后处理模型和视口
		*/
		static int newPostViewportModel();

		/*
		* 删除指定模型的所有视口（不保留任何视口和该模型）
		*/
		static void deleteViewportModel(int viewportModelId);

		/*
		* 删除当前视口
		*/
		static void deleteViewport(int viewportId);

		/*
		* 删除指定模型下的指定名称的后处理视口(viewportModelId=-1表示当前模型，其余为指定模型)
		*/
		static void deletePostViewport(QString name, int viewportModelId = -1);

		/*
		* 删除指定模型下的所有后处理视口
		*/
		static void deletePostViewport(int viewportModelId);

		/*
		* 激活前处理视口(根据当前模型),若要指定模型可先使用setCurrentViewportModel
		*/
		static void activatePreViewport();

		/*
		* 激活后处理视口根据名称(根据当前模型),若要指定模型可先使用setCurrentViewportModel
		*/
		static void activatePostViewport(QString name = "");
	};

}



