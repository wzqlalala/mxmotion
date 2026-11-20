#include "mPostGUIData.h"

namespace MechPreData
{
	mPostGUIData::mPostGUIData()
	{
		//云图
		_cloudDiscrete = 1;
		_cloudDiscreteNum = 9;
		_cloudIsEquivariance = 0;
		_pointSize = 10;
		_rangeIndex = 0;

		//变形图
		_deformFactor = QVector3D(1, 1, 1);

		//动画
		_generateAnimate = false;
		_aniFrameInterval = 1;
		_aniFrameRate = 10;
		_aniLoopPlay = true;
		_aniEveryRange = QString("当前帧范围");
		_aniIsStart = true;
		_aniHideFailedElement = true;

		//流线图
		_hasStreamLineView = false;
		_streamLineRatio = 0.1;
		_streamLineNum = 500;
		_streamLineStyle = QString("线");
		_streamLineDirection = QString("两侧");
		_streamLineSphere = false;

		////矢量图
		//_hasVectorArrowView = false;
		//_vectorArrowMethod = QString("固定值");

		////等值线等值面图
		//_hasContourView = false;
		//_contourIsAutoGenerate = true;
		//_contourIsRealTimeGenerate = true;
	}

	mPostGUIData::~mPostGUIData()
	{

	}
	bool mPostGUIData::isClipViewExsit(const QString & name)
	{
		const int nclip = _clipViews.size();
		for (int i = 0; i < nclip; ++i)
		{
			ClipView cv = _clipViews.at(i);
			if (name == cv._clipName)
				return true;
		}
		return false;
	}

	int mPostGUIData::getClipId(const QString & name)
	{
		const int nclip = _clipViews.size();
		for (int i = 0; i < nclip; ++i)
		{
			ClipView cv = _clipViews.at(i);
			if (name == cv._clipName)
				return i;
		}
		return 0;
	}

	ClipView mPostGUIData::getClipViewStruct(const QString & name)
	{
		const int nclip = _clipViews.size();
		for (int i = 0; i < nclip; ++i)
		{
			ClipView cv = _clipViews.at(i);
			if (name == cv._clipName)
				return cv;
		}
		return ClipView{};
	}
}