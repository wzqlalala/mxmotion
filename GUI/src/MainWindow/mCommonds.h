#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
#include <QString>

//BasicData
#include "mMaterialBase.h"
//#include "mLoadBase.h"
#include "mLoadBoundaryBase.h"
#include "mConnectorBase.h"

//Material
#include "mMaterialPanel.h"


namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mCommondBase
	{
	public:
		enum class CommondType {
			//材料
			MAT_CREATEUNDO,
			MAT_CREATEREDO,
			MAT_DELUNDO,
			MAT_DELREDO,
			MAT_PROPADDUNDO,
			MAT_PROPADDREDO,
			MAT_PROPDELUNDO,
			MAT_PROPDELREDO,
			MAT_PROPMODUNDO,
			MAT_PROPMODREDO,
			//载荷
			LOAD_CREATEUNDO,
			LOAD_CREATEREDO,
			LOAD_DELUNDO,
			LOAD_DELREDO,
			//边界
			BOUNDARY_CREATEUNDO,
			BOUNDARY_CREATEREDO,
			BOUNDARY_DELUNDO,
			BOUNDARY_DELREDO,
			//连接
			CONNECTOR_CREATEUNDO,
			CONNECTOR_CREATEREDO,
			CONNECTOR_DELUNDO,
			CONNECTOR_DELREDO,
		};
		/*
		* 构造函数
		*/
		mCommondBase(CommondType commond);

		/*
		* 析构函数
		*/
		virtual ~mCommondBase();

		/*
		* 状态切换
		*/
		virtual void switchState() = 0;

		/*
		* 获取命令类型
		*/
		CommondType getCommondType() { return _commond; };

		/*
		* 注册回调
		*/
		void regCallBackFunc(const mCommondBase::CommondType& commond, std::function<void()>func);

		/*
		* 回调
		*/
		void callBackFunc(const mCommondBase::CommondType& commond);

		/*
		* 体系共用回调函数表
		*/
		static std::unordered_map <mCommondBase::CommondType, std::function<void()>>& globalCallbacks();

	protected:
		CommondType _commond;

	};

	/********************************************************材料**********************************************************/
	class MMAINWINDOW_EXPORT mCommondMaterial :public mCommondBase
	{
	public:
		/*
		* 构造函数
		*/
		mCommondMaterial(CommondType commond);
		
		/*
		* 析构函数
		*/
		~mCommondMaterial();

		/*
		* 存储快照数据
		*/
		void storeSnap(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

		/*
		* 获取快照数据
		*/
		std::shared_ptr<MechMaterialData::mMaterialBase>getSnap();

		/*
		* 状态切换
		*/
		void switchState()override;

	private:
		/*
		* 材料的创建是从无到有的过程，所以创建这一步存的命令需要存快照数据，
		* 将材料数据用智能指针存储，只存一份。
		*/
		std::shared_ptr<MechMaterialData::mMaterialBase> _matData;
	};

	class MMAINWINDOW_EXPORT mCommondMaterialProp :public mCommondBase
	{
	public:
		/*
		* 构造函数
		*/
		mCommondMaterialProp(CommondType commond);

		/*
		* 析构函数
		*/
		~mCommondMaterialProp();

		/*
		* 存储属性
		*/
		void storeProp(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& prop);

		/*
		* 获取属性
		*/
		QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>> getProp();

		/*
		* 存储材料ID
		*/
		void storeMatID(const int& matid);

		/*
		* 获取材料ID
		*/
		int getMatID();

		/*
		* 存储材料类型
		*/
		void storeMatType(const MxFunctions::MaterialType& mattype);

		/*
		* 获取材料类型
		*/
		MxFunctions::MaterialType getMatType();

		/*
		* 状态切换
		*/
		void switchState()override;

	private:
		int _matID;
		QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>> _prop;
		MxFunctions::MaterialType _mattype;
	};

	/********************************************************载荷**********************************************************/
	class MMAINWINDOW_EXPORT mCommondLoad :public mCommondBase
	{
	public:
		/*
		* 构造函数
		*/
		mCommondLoad(CommondType commond);

		/*
		* 析构函数
		*/
		~mCommondLoad();

		/*
		* 状态切换
		*/
		void switchState()override;

	public:
		std::shared_ptr<MechCaseData::mLoadBoundaryBase> _loadData;

	};

	/********************************************************边界**********************************************************/
	class MMAINWINDOW_EXPORT mCommondBoundary :public mCommondBase
	{
	public:
		/*
		* 构造函数
		*/
		mCommondBoundary(CommondType commond);

		/*
		* 析构函数
		*/
		~mCommondBoundary();

		/*
		* 状态切换
		*/
		void switchState()override;

	public:
		std::shared_ptr<MechCaseData::mLoadBoundaryBase> _boundaryData;

	};

	/********************************************************连接**********************************************************/
	class MMAINWINDOW_EXPORT mCommondConnector :public mCommondBase
	{
	public:
		/*
		* 构造函数
		*/
		mCommondConnector(CommondType commond);

		/*
		* 析构函数
		*/
		~mCommondConnector();

		/*
		* 状态切换
		*/
		void switchState()override;

	public:
		std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData;

	};
}