#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QDataStream>

#include "mFrpData_global.h"
#include "mFrpBaseData.h"
#include "mFrpManager.h"
#include "mFrpDerivedData.h"

namespace MechFrpData
{
	/* ---------------- mFrpBaseData 公共字段（id/name/renamed） ---------------- */
	inline QDataStream& operator<<(QDataStream& out, const mFrpBaseData& obj)
	{
		out << obj.id << obj.name << obj.renamed;
		return out;
	}

	/* ---------------- Analysis1 根 ---------------- */
	inline QDataStream& operator<<(QDataStream& out, const mFrpAnalysisData& obj)
	{
		out << static_cast<const mFrpBaseData&>(obj);
		out << obj.analysisType << obj.homoModel << obj.multiPhase;
		return out;
	}

	/* ---------------- 材料 ---------------- */
	inline QDataStream& operator<<(QDataStream& out, const mFrpMaterialData& obj)
	{
		out << static_cast<const mFrpBaseData&>(obj);
		out << obj.m_kind << obj.matType;
		out << obj.young << obj.poisson << obj.density
			<< obj.thermalExp << obj.refTemp;
		out << qint32(obj.thermalExpType);
		out << obj.a1;
		return out;
	}

	/* ---------------- 相节点（基体/掺杂/空腔/连续纤维 四形态字段合集） ---------------- */
	inline QDataStream& operator<<(QDataStream& out, const mFrpPhaseData& obj)
	{
		out << static_cast<const mFrpBaseData&>(obj);
		out << obj.baseMaterial;

		out << obj.inc.material;
		out << qint32(obj.inc.ratioKind);
		out << obj.inc.ratioValue;
		out << qint32(obj.inc.shape);
		out << obj.inc.aspect;
		out << obj.inc.distFuncName;
		out << obj.inc.discreteCount;
		out << qint32(obj.inc.defMethod);
		out << obj.inc.theta << obj.inc.phi << obj.inc.tensor;
		out << obj.inc.tensorFilePath;
		out << obj.inc.advanced;
		out << qint32(obj.inc.featureType);
		out << obj.inc.featureMat << obj.inc.featureFrac
			<< obj.inc.fiberFrac1 << obj.inc.fiberFrac2
			<< obj.inc.featureAspect;

		out << obj.cav.volumeFraction;
		out << qint32(obj.cav.shape);
		out << obj.cav.aspect;
		out << obj.cav.distFuncName;
		out << obj.cav.discreteCount;
		out << qint32(obj.cav.defMethod);
		out << obj.cav.theta << obj.cav.phi << obj.cav.tensor;
		out << obj.cav.tensorFilePath;
		out << obj.cav.surface << obj.cav.surfaceMat << obj.cav.surfaceFrac;

		out << obj.fib.material;
		out << qint32(obj.fib.ratioKind);
		out << obj.fib.ratioValue << obj.fib.theta << obj.fib.phi
			<< obj.fib.coating << obj.fib.coatMat << obj.fib.coatFrac;
		return out;
	}

	/* ---------------- RVE ---------------- */
	inline QDataStream& operator<<(QDataStream& out, const mFrpRveData& obj)
	{
		out << static_cast<const mFrpBaseData&>(obj);
		out << obj.mode;
		out << obj.structure;
		out << quint32(obj.layers.size());
		for (const RveLayer& layer : obj.layers)
		{
			out << layer.micro << layer.thick << layer.angle << layer.value << layer.orient;
		}
		out << obj.angleTheta << obj.anglePhi << obj.tensor6;
		return out;
	}

	/* ---------------- 失效准则 ---------------- */
	inline QDataStream& operator<<(QDataStream& out, const mFrpFailureData& obj)
	{
		out << static_cast<const mFrpBaseData&>(obj);
		out << obj.model << obj.failType << obj.compCriterion;
		out << obj.tens << obj.comp2
			<< obj.axialTens << obj.axialComp << obj.inPlaneTens << obj.inPlaneComp << obj.shear
			<< obj.axialTensT << obj.inPlaneTensT << obj.shearT
			<< obj.xt << obj.yt << obj.zt << obj.xys << obj.yzs << obj.xzs
			<< obj.azAxTens << obj.azAxComp << obj.azIpTens << obj.azIpComp << obj.azShear
			<< obj.wuAxTens << obj.wuAxComp << obj.wuIpTens << obj.wuIpComp << obj.wuShear << obj.wuCouple
			<< obj.wu3d
			<< obj.wuTAxialTens << obj.wuTAxialComp << obj.wuTInPlaneTens << obj.wuTInPlaneComp << obj.wuTShear
			<< obj.mcAxialTens << obj.mcAxialComp << obj.mcInPlaneTens << obj.mcInPlaneComp << obj.mcShear;
		return out;
	}

	/* ---------------- 长细比函数 ---------------- */
	inline QDataStream& operator<<(QDataStream& out, const mFrpAspectRatioData& obj)
	{
		out << static_cast<const mFrpBaseData&>(obj);
		out << quint32(obj.rows.size());
		for (const auto& point : obj.rows)
			out << point.first << point.second;
		return out;
	}

	namespace
	{
		/* ---------------- 树写出（递归） ---------------- */
		inline bool writeFrpNode(QDataStream& out, const mFrpBaseData* node)
		{
			out << qint32(node->type);
			// 公共字段 + 各派生 payload（分组/微观结构N 为基类实例，仅公共字段）
			if (node->isGroupData() || node->type == FrpNode_Micro1)
				out << *node;
			else if (const mFrpAnalysisData* d = node->as<mFrpAnalysisData>()) out << *d;
			else if (const mFrpMaterialData* d = node->as<mFrpMaterialData>()) out << *d;
			else if (const mFrpPhaseData* d = node->as<mFrpPhaseData>()) out << *d;
			else if (const mFrpRveData* d = node->as<mFrpRveData>()) out << *d;
			else if (const mFrpFailureData* d = node->as<mFrpFailureData>()) out << *d;
			else if (const mFrpAspectRatioData* d = node->as<mFrpAspectRatioData>()) out << *d;
			else
				return false;
			out << quint32(node->children.size());
			for (const mFrpBaseData* child : node->children)
				if (!writeFrpNode(out, child))
					return false;
			return out.status() == QDataStream::Ok;
		}
	}

	/* ---------------- 管理器（整树写出：6 计数器 + 根列表） ---------------- */
	inline QDataStream& operator<<(QDataStream& out, const mFrpManager& mgr)
	{
		if (out.status() != QDataStream::Ok)
			return out;

		out << qint32(mgr.m_idSeq);
		out << qint32(mgr.m_analysisSeq);
		out << qint32(mgr.m_materialSeq);
		out << qint32(mgr.m_microSeq);
		out << qint32(mgr.m_failureSeq);
		out << qint32(mgr.m_funSeq);
		out << quint32(mgr.m_roots.size());
		for (const mFrpBaseData* root : mgr.m_roots)
		{
			if (!writeFrpNode(out, root))
			{
				if (out.status() == QDataStream::Ok)
					out.setStatus(QDataStream::WriteFailed);
				break;
			}
		}
		return out;
	}
}
