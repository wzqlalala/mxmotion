#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QDataStream>
#include <utility>
#include "mFrpData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mFrpBaseData.h"

using namespace MxFunctions;
namespace MechFrpData
{
	// —— Analysis1 根 ——
	class MXFRPDATA_EXPORT mFrpAnalysisData : public mFrpBaseData
	{
	friend QDataStream& InStreammFrpAnalysisDataV3(QDataStream& in, mFrpAnalysisData& obj);
	public:
		mFrpAnalysisData();
		mFrpBaseData* clone(mFrpBaseData* parentNode = nullptr) const override;

		QString analysisType;   // Analysis1 类型："力学分析" 、"热-力学分析" 、"热学分析"
		QString homoModel;      // 均质化模型"Mori Tanaka"、"Double Inclusion"
		QString multiPhase;     // 多相掺杂均质化"Multi-Step"、"Multi-Level"
	};

	// —— 材料 ——
	class MXFRPDATA_EXPORT mFrpMaterialData : public mFrpBaseData
	{
	friend QDataStream& InStreammFrpMaterialDataV3(QDataStream& in, mFrpMaterialData& obj);
	public:
		mFrpMaterialData();
		mFrpBaseData* clone(mFrpBaseData* parentNode = nullptr) const override;

		QString m_kind;			  // 材料种类"线弹性"、"热学"
		QString matType;        // 材料类型：各向同性
		QString young;            // 杨氏模量
		QString poisson;          // 泊松比
		QString density;          // 密度
		bool thermalExp = false;  // 热膨胀系数勾选（默认不勾）
		QString refTemp;          // 参考温度
		int thermalExpType = 0;   // 热膨胀系数选择
		QString a1;               // A1
	};

	// —— 相节点（基体/掺杂/空腔/连续纤维 四形态字段合集）——
	class MXFRPDATA_EXPORT mFrpPhaseData : public mFrpBaseData
	{
	friend QDataStream& InStreammFrpPhaseDataV3(QDataStream& in, mFrpPhaseData& obj);
	public:
		mFrpPhaseData(MxFunctions::FrpNodeType type);
		mFrpBaseData* clone(mFrpBaseData* parentNode = nullptr) const override;

		// 基体（MicroDefPage）
		QString baseMaterial;   // 基体引用的材料名

		// 掺杂（InclusionPage）
		class Inc {
		public:
			QString material;  int ratioKind = 0;   // material：引用的材料名（空串=请选择）
			QString ratioValue;
			int shape = 0;  QString aspect;
			QString distFuncName; QString discreteCount;
			int defMethod = 0; QString theta, phi, tensor;
			QString tensorFilePath;
			bool advanced = false;
			int featureType = 0;  QString featureMat;
			QString featureFrac, fiberFrac1, fiberFrac2, featureAspect;
		} inc;

		// 空腔（CavityPage）
		class Cav {
		public:
			QString volumeFraction;
			int shape = 0; QString aspect;
			QString distFuncName; QString discreteCount;
			int defMethod = 0; QString theta, phi, tensor;   // 新增 tensor
			QString tensorFilePath;
			bool surface = false; QString surfaceMat, surfaceFrac;
		} cav;

		// 连续纤维（FiberPage）
		class Fib {
		public:
			QString material;  int ratioKind = 0;
			QString ratioValue, theta, phi;
			bool coating = false; QString coatMat, coatFrac;
		} fib;
	};


	// —— RVE ——
	struct RveLayer { QString micro, thick, angle, value, orient; };
	class MXFRPDATA_EXPORT mFrpRveData : public mFrpBaseData
	{
	friend QDataStream& InStreammFrpRveDataV3(QDataStream& in, mFrpRveData& obj);
	public:
		mFrpRveData();
		mFrpBaseData* clone(mFrpBaseData* parentNode = nullptr) const override;

		QString mode;                     // 分析方式：0 单层分析 / 1 多层分析
		QString structure;             // 单层分析选中的微观结构名（存名字，增删微观结构后仍可对应）
		QVector<RveLayer> layers;      // 多层分析：每层一行（微观结构/厚度/角度/纤维取向及取向值）
		// 单层分析取向（UI 待接入，先随工程持久化保留，避免后续补功能时破坏文件版本）
		QString angleTheta = QStringLiteral("0");
		QString anglePhi = QStringLiteral("0");
		QString tensor6 = QStringLiteral("0;0;0;0;0;0");  // 取向张量 6 项
	};

	// —— 失效准则（九个模型字段）——
	class MXFRPDATA_EXPORT mFrpFailureData : public mFrpBaseData
	{
	friend QDataStream& InStreammFrpFailureDataV3(QDataStream& in, mFrpFailureData& obj);
	public:
		mFrpFailureData();
		mFrpBaseData* clone(mFrpBaseData* parentNode = nullptr) const override;

		QString model;            // 失效模型
		QString failType;     // 类型（注意：节点类型用基类 type，这里是失效“类型”下拉）
		QString compCriterion;             // 压缩方式/应力分量
		QString tens, comp2;                              // Maximum
		QString axialTens, axialComp, inPlaneTens, inPlaneComp, shear;            // Tsai Hill 2D
		QString axialTensT, inPlaneTensT, shearT;        // Tsai Hill 3D Transversely
		QString xt, yt, zt, xys, yzs, xzs;              // Tsai Hill 3D
		QString azAxTens, azAxComp, azIpTens, azIpComp, azShear;                  // Azzi 2D
		QString wuAxTens, wuAxComp, wuIpTens, wuIpComp, wuShear, wuCouple;        // Tsai Wu 2D
		QString wu3d;                                    // Tsai Wu 3D 串
		// Tsai Wu 3D Transversely：复用 Tsai Hill 2D 的 5 框页面，但数据独立存储
		QString wuTAxialTens, wuTAxialComp, wuTInPlaneTens, wuTInPlaneComp, wuTShear;
		// Multi Component 2D：复用 Azzi 的 5 框页面，但数据独立存储
		QString mcAxialTens, mcAxialComp, mcInPlaneTens, mcInPlaneComp, mcShear;
	};

	// —— 长细比函数 ——
	class MXFRPDATA_EXPORT mFrpAspectRatioData : public mFrpBaseData
	{
	friend QDataStream& InStreammFrpAspectRatioDataV3(QDataStream& in, mFrpAspectRatioData& obj);
	public:
		mFrpAspectRatioData();
		mFrpBaseData* clone(mFrpBaseData* parentNode = nullptr) const override;

		QVector<std::pair<QString, QString>> rows;
	};
}
