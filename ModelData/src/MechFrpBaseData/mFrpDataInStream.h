#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QDataStream>
#include <memory>

#include "mFrpData_global.h"
#include "mFrpBaseData.h"
#include "mFrpManager.h"
#include "mFrpDerivedData.h"

namespace MechFrpData
{
	/* ---------------- mFrpBaseData 公共字段（id/name/renamed） ---------------- */
	inline QDataStream& operator>>(QDataStream& in, mFrpBaseData& obj)
	{
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammFrpBaseDataV3(in, obj);
	    }
	    else
	    {
	        InStreammFrpBaseDataV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammFrpBaseDataV3(QDataStream& in, mFrpBaseData& obj)
	{
		in >> obj.id >> obj.name >> obj.renamed;
		return in;
	}


	/* ---------------- Analysis1 根 ---------------- */
	inline QDataStream& operator>>(QDataStream& in, mFrpAnalysisData& obj)
	{
	    in >> static_cast<mFrpBaseData&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammFrpAnalysisDataV3(in, obj);
	    }
	    else
	    {
	        InStreammFrpAnalysisDataV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammFrpAnalysisDataV3(QDataStream& in, mFrpAnalysisData& obj)
	{
		in >> obj.analysisType >> obj.homoModel >> obj.multiPhase;
		return in;
	}


	/* ---------------- 材料 ---------------- */
	inline QDataStream& operator>>(QDataStream& in, mFrpMaterialData& obj)
	{
	    in >> static_cast<mFrpBaseData&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammFrpMaterialDataV3(in, obj);
	    }
	    else
	    {
	        InStreammFrpMaterialDataV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammFrpMaterialDataV3(QDataStream& in, mFrpMaterialData& obj)
	{
		in >> obj.m_kind >> obj.matType;
		in >> obj.young >> obj.poisson >> obj.density
			>> obj.thermalExp >> obj.refTemp;
		qint32 thermalExpType = 0;
		in >> thermalExpType;
		obj.thermalExpType = thermalExpType;
		in >> obj.a1;
		return in;
	}


	/* ---------------- 相节点（基体/掺杂/空腔/连续纤维 四形态字段合集） ---------------- */
	inline QDataStream& operator>>(QDataStream& in, mFrpPhaseData& obj)
	{
	    in >> static_cast<mFrpBaseData&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammFrpPhaseDataV3(in, obj);
	    }
	    else
	    {
	        InStreammFrpPhaseDataV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammFrpPhaseDataV3(QDataStream& in, mFrpPhaseData& obj)
	{
		in >> obj.baseMaterial;

		qint32 intVal = 0;
		in >> obj.inc.material;
		in >> intVal; obj.inc.ratioKind = intVal;
		in >> obj.inc.ratioValue;
		in >> intVal; obj.inc.shape = intVal;
		in >> obj.inc.aspect;
		in >> obj.inc.distFuncName;
		in >> obj.inc.discreteCount;
		in >> intVal; obj.inc.defMethod = intVal;
		in >> obj.inc.theta >> obj.inc.phi >> obj.inc.tensor;
		in >> obj.inc.tensorFilePath;
		in >> obj.inc.advanced;
		in >> intVal; obj.inc.featureType = intVal;
		in >> obj.inc.featureMat >> obj.inc.featureFrac
			>> obj.inc.fiberFrac1 >> obj.inc.fiberFrac2
			>> obj.inc.featureAspect;

		in >> obj.cav.volumeFraction;
		in >> intVal; obj.cav.shape = intVal;
		in >> obj.cav.aspect;
		in >> obj.cav.distFuncName;
		in >> obj.cav.discreteCount;
		in >> intVal; obj.cav.defMethod = intVal;
		in >> obj.cav.theta >> obj.cav.phi >> obj.cav.tensor;
		in >> obj.cav.tensorFilePath;
		in >> obj.cav.surface >> obj.cav.surfaceMat >> obj.cav.surfaceFrac;

		in >> obj.fib.material;
		in >> intVal; obj.fib.ratioKind = intVal;
		in >> obj.fib.ratioValue >> obj.fib.theta >> obj.fib.phi
			>> obj.fib.coating >> obj.fib.coatMat >> obj.fib.coatFrac;
		return in;
	}


	/* ---------------- RVE ---------------- */
	inline QDataStream& operator>>(QDataStream& in, mFrpRveData& obj)
	{
	    in >> static_cast<mFrpBaseData&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammFrpRveDataV3(in, obj);
	    }
	    else
	    {
	        InStreammFrpRveDataV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammFrpRveDataV3(QDataStream& in, mFrpRveData& obj)
	{
		in >> obj.mode;
		in >> obj.structure;
		quint32 layerCount = 0;
		in >> layerCount;
		obj.layers.reserve(int(layerCount));
		for (quint32 i = 0; i < layerCount; ++i)
		{
			RveLayer layer;
			in >> layer.micro >> layer.thick >> layer.angle >> layer.value >> layer.orient;
			if (in.status() != QDataStream::Ok)
				return in;
			obj.layers.append(layer);
		}
		in >> obj.angleTheta >> obj.anglePhi >> obj.tensor6;
		return in;
	}


	/* ---------------- 失效准则 ---------------- */
	inline QDataStream& operator>>(QDataStream& in, mFrpFailureData& obj)
	{
	    in >> static_cast<mFrpBaseData&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammFrpFailureDataV3(in, obj);
	    }
	    else
	    {
	        InStreammFrpFailureDataV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammFrpFailureDataV3(QDataStream& in, mFrpFailureData& obj)
	{
		in >> obj.model >> obj.failType >> obj.compCriterion;
		in >> obj.tens >> obj.comp2
			>> obj.axialTens >> obj.axialComp >> obj.inPlaneTens >> obj.inPlaneComp >> obj.shear
			>> obj.axialTensT >> obj.inPlaneTensT >> obj.shearT
			>> obj.xt >> obj.yt >> obj.zt >> obj.xys >> obj.yzs >> obj.xzs
			>> obj.azAxTens >> obj.azAxComp >> obj.azIpTens >> obj.azIpComp >> obj.azShear
			>> obj.wuAxTens >> obj.wuAxComp >> obj.wuIpTens >> obj.wuIpComp >> obj.wuShear >> obj.wuCouple
			>> obj.wu3d
			>> obj.wuTAxialTens >> obj.wuTAxialComp >> obj.wuTInPlaneTens >> obj.wuTInPlaneComp >> obj.wuTShear
			>> obj.mcAxialTens >> obj.mcAxialComp >> obj.mcInPlaneTens >> obj.mcInPlaneComp >> obj.mcShear;
		return in;
	}


	/* ---------------- 长细比函数 ---------------- */
	inline QDataStream& operator>>(QDataStream& in, mFrpAspectRatioData& obj)
	{
	    in >> static_cast<mFrpBaseData&>(obj);
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammFrpAspectRatioDataV3(in, obj);
	    }
	    else
	    {
	        InStreammFrpAspectRatioDataV3(in, obj);
	    }
	    return in;
}
	inline QDataStream& InStreammFrpAspectRatioDataV3(QDataStream& in, mFrpAspectRatioData& obj)
	{
		quint32 rowCount = 0;
		in >> rowCount;
		obj.rows.reserve(int(rowCount));
		for (quint32 i = 0; i < rowCount; ++i)
		{
			std::pair<QString, QString> point;
			in >> point.first >> point.second;
			if (in.status() != QDataStream::Ok)
				return in;
			obj.rows.append(point);
		}
		return in;
	}


	namespace
	{
		/* ---------------- 树读取（递归） ---------------- */
		inline std::unique_ptr<mFrpBaseData> readFrpNode(QDataStream& in, mFrpBaseData* parent)
		{
			qint32 type = 0;
			in >> type;
			if (in.status() != QDataStream::Ok)
				return nullptr;

			// 流中按 qint32 存储，转回枚举再构造（int 不能隐式转换为 FrpNodeType）
			const FrpNodeType nodeType = static_cast<FrpNodeType>(type);
			const bool groupRecord =
				(parent && parent->type == FrpNode_Analysis1
					&& (nodeType == FrpNode_Material || nodeType == FrpNode_Micro
						|| nodeType == FrpNode_Failure || nodeType == FrpNode_AspectRatio))
				|| nodeType == FrpNode_Micro1;
			std::unique_ptr<mFrpBaseData> node(
				groupRecord ? new mFrpBaseData(nodeType)
				: mFrpBaseData::createByType(nodeType));
			if (!node)
			{
				in.setStatus(QDataStream::ReadCorruptData);
				return nullptr;
			}

			// 公共字段 + 各派生 payload（分组/微观结构N 为基类实例，仅公共字段）
			if (groupRecord)
				in >> *node;
			else if (mFrpAnalysisData* d = node->as<mFrpAnalysisData>()) in >> *d;
			else if (mFrpMaterialData* d = node->as<mFrpMaterialData>()) in >> *d;
			else if (mFrpPhaseData* d = node->as<mFrpPhaseData>()) in >> *d;
			else if (mFrpRveData* d = node->as<mFrpRveData>()) in >> *d;
			else if (mFrpFailureData* d = node->as<mFrpFailureData>()) in >> *d;
			else if (mFrpAspectRatioData* d = node->as<mFrpAspectRatioData>()) in >> *d;
			else
			{
				in.setStatus(QDataStream::ReadCorruptData);
				return nullptr;
			}
			if (in.status() != QDataStream::Ok)
				return nullptr;

			quint32 childCount = 0;
			in >> childCount;
			for (quint32 i = 0; i < childCount; ++i)
			{
				std::unique_ptr<mFrpBaseData> child = readFrpNode(in, node.get());
				if (!child)
					return nullptr;
				node->addChild(child.release());
			}
			return node;
		}
	}

	/* ---------------- 管理器（整树读入，全部读入完成后换入目标） ---------------- */
	inline QDataStream& operator>>(QDataStream& in, mFrpManager& mgr)
	{
	    //版本号高先判断，兼容所有低版本与高版本
	    if (FILE_VERSION >= 0x3000)
	    {
	        InStreammFrpManagerV3(in, mgr);
	    }
	    else
	    {
	        InStreammFrpManagerV3(in, mgr);
	    }
	    return in;
}
	inline QDataStream& InStreammFrpManagerV3(QDataStream& in, mFrpManager& mgr)
	{
		if (in.status() != QDataStream::Ok)
			return in;

		mFrpManager candidate;
		qint32 seqVal = 0;
		in >> seqVal; candidate.m_idSeq = seqVal;
		in >> seqVal; candidate.m_analysisSeq = seqVal;
		in >> seqVal; candidate.m_materialSeq = seqVal;
		in >> seqVal; candidate.m_microSeq = seqVal;
		in >> seqVal; candidate.m_failureSeq = seqVal;
		in >> seqVal; candidate.m_funSeq = seqVal;
		if (in.status() != QDataStream::Ok)
			return in;

		quint32 rootCount = 0;
		in >> rootCount;
		candidate.m_roots.reserve(int(rootCount));
		for (quint32 i = 0; i < rootCount; ++i)
		{
			std::unique_ptr<mFrpBaseData> root = readFrpNode(in, nullptr);
			if (!root)
				return in;
			candidate.m_roots.append(root.release());
		}
		if (in.status() != QDataStream::Ok)
			return in;

		// 全部读入成功后才换入目标管理器（失败时原管理器数据保持不变）
		std::swap(candidate.m_roots, mgr.m_roots);
		std::swap(candidate.m_idSeq, mgr.m_idSeq);
		std::swap(candidate.m_analysisSeq, mgr.m_analysisSeq);
		std::swap(candidate.m_materialSeq, mgr.m_materialSeq);
		std::swap(candidate.m_microSeq, mgr.m_microSeq);
		std::swap(candidate.m_failureSeq, mgr.m_failureSeq);
		std::swap(candidate.m_funSeq, mgr.m_funSeq);
		return in;
	}

}
