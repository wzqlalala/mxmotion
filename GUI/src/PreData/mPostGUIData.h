#pragma once
#pragma execution_character_set("utf-8")
#include "MPreData_global.h"
#include <QString>
#include <QPair>
#include <QStringList>
#include <QVector>
#include <QHash>
#include <set>
#include <QVector3D>
#include <QColor>

#include "mPostDataBase.h"
namespace MechPreData
{
	/** 切割图结构体 */
	struct ClipView
	{
		QString					_clipName{};									//名称
		bool					_clipStatus{};									//状态
		double					_clipValue{0.5};								//值
		QString					_clipPlane{};									//平面类型
		QString					_clipVector{};									//矢量值
		QVector3D				_clipNormal{};									//矢量
		QVector3D				_clipCenter{};									//点
		QVector3D				_clipMaxVal{};									//三个方向的最大值
		QVector3D				_clipMinVal{};									//三个方向的最小值
	};

	///** 切割图结构体 */
	//struct ContourView
	//{
	//	bool					_contourStatus{true};							//状态
	//	double					_contourValue{0.000000};						//值

	//};

	/** 截面剖切渲染参数（结果页"截面剖切"，mPostSectionCutPanel 已保存+激活记录的
	渲染投影；面板按 SectionCutDraft 折算写入，applySectionCuts 按此组装引擎参数，
	随云图视口 guidata 存续——向量图同款"参数存视口、切帧跟随重提交"模式） */
	struct SectionCutSpec
	{
		QString								_id{};										//面板记录 id（引擎台账对象按记录跟踪的键）
		QString								_name{};									//截面名称（超限提示用）
		QVector3D							_normal{ 0, 0, 1 };							//平面法向（已归一；三点/向量/起点-终点折算完成）
		QVector3D							_base{};									//基准点（偏移起点；随变形平移的作用点）
		int									_side{ 2 };									//保留侧 0 负向侧/1 两侧/2 正向侧（反向钮已折算）
		int									_styleMode{ 0 };							//剖切模式 0 单平面/1 截面带
		double								_bandwidth{ 1 };							//带宽（截面带；双平面对夹各偏 ±带宽/2）
		double								_offset{ 0 };								//剖切面偏移（沿法向）
		int									_configuration{ 0 };						//几何构型 0 固定平面/1 随变形移动
		bool								_fill{ true };								//填充截面（false 渲染暂等同 true，引擎封盖开关遗留）
		int									_colorMode{ 0 };							//填充方式 0 结果着色/1 统一颜色（引擎 E1）
		QColor								_color{ 0x16, 0x77, 0xC8 };					//统一颜色
		bool								_crop{ true };								//裁剪单元（false=该截面不参与模型裁剪，仅显示截面封盖/提示面）
		bool								_showPlane{ false };						//辅助显示-剖切平面（引擎半透明提示面 isShow）
	};

	/** 云图缓存快照（"保存云图缓存"按钮捕获，缓存云图节点创建时回放） */
	struct CloudPresetSnapshot
	{
		QString								_presetName{};								//缓存名称（变量_分量，无分量为 变量_无分量，同名追加序号）
		QString								_cloudCase{};								//工况
		QString								_cloudFrame{};								//帧
		QString								_cloudVarName{};							//变量名
		QString								_cloudCompo{};							//分量名
		QString								_cloudLayerName{};							//层名称
		QString								_cloudComplex{};							//复数
		QString								_smoothMethod{ "无" };						//磨平方式
		bool								_smoothAcrossParts{ false };				//跨部件磨平
		bool								_smoothVariationEnabled{ false };			//高级磨平变化率启用
		double								_smoothVariationLimit{ 0 };					//高级磨平变化率上限%
		QString								_rangeMode{ "所有部件" };					//图例模式
		double								_cloudMaxValue{};							//最大值
		double								_cloudMinValue{};							//最小值
		int									_cloudDiscrete{ 1 };						//离散开关
		int									_cloudDiscreteNum{ 9 };						//离散阶数
		int									_cloudIsEquivariance{ 0 };					//离散类型
		//——对象及坐标系——
		bool								_cloudCoordinate{ false };					//局部坐标系开关
		QString								_currentCoordinateName{};					//当前坐标系名
		QString								_renderCoordSystem{ "全局坐标系" };			//渲染坐标系
		//——图例与显隐标注——
		QString								_legendRange{ "逐帧" };					//图例范围（逐帧/全程/手动，默认逐帧=HyperView Dynamic Scale）
		bool								_cloudShowMax{ false };						//显示最大值
		bool								_cloudShowMin{ false };						//显示最小值
		bool								_cloudShowMaxLine{ false };					//显示最大值线
		bool								_cloudShowMinLine{ false };					//显示最小值线
	};

	class MPREDATA_EXPORT mPostGUIData
	{
	public:
		/** 默认构造函数 */
		mPostGUIData();

		/** 析构函数 */
		~mPostGUIData();

		/** 判断切割图名称是否已存在 */
		bool isClipViewExsit(const QString& name);

		/** 根据切割图名称获取ID */
		int getClipId(const QString& name);

		/** 根据名称获取结构体数据 */
		ClipView getClipViewStruct(const QString& name);

	public:
        QString                                 _currentFileName{};                                         // 当前打开的Mxdb文件名称
		QStringList								_cloudFrames{};												//云图-帧（包含其他信息）
		QStringList								_cloudShortFrames{};										//云图-帧（只包含帧序号）
		QStringList								_cloudVarNames{};											//云图-变量名
		QStringList								_cloudCompos{};												//云图-分量名
		QStringList								_cloudLayerNames{};											//云图-层名称
		QStringList								_cloudCases{};												//云图-工况
		int										_cloudFrameIndex{};											//云图-帧名称索引
		int										_cloudFrame{};												//云图-帧号
		QString									_cloudVarName{};											//云图-变量名
		QString									_cloudCompo{};												//云图-分量名
		QString									_cloudLayerName{};											//云图-层名称
		QString									_cloudCase{};												//云图-工况
		QString									_cloudComplex{};											//云图-复数
        bool                                    _cloudPlanish{ false };                                     //云图-显示(磨平)
		bool									_cloudHaveComplex{ false };									//云图-存在复数
		bool									_cloudHaveLayer{ false };									//云图-存在层数
		bool									_cloudShowFreeNode{ false };								//云图-显示自由节点
		bool									_cloudDeleteFreeNode{ false };								//云图-删除自由节点
		bool									_cloudHaveFreeNode{ false };								//云图-是否存在自由节点
		bool									_cloudRelatedCloud{ false };								//云图-关联云图
		bool									_cloudShowSBR{ false };										//云图-壳杆梁实体化显示
		bool									_cloudInfoLabelVisible{ true };								//云图-信息标签（视口右上角文本）显隐
		bool									_cloudSmooth{ false };										//云图-是否磨平
		int										_rangeIndex{ 0 };											//云图-渲染范围
		double									_cloudMaxValue{};											//云图-最大值
		double									_cloudMinValue{};											//云图-最小值
		double									_cloudDefualtMax{};											//云图-默认最大值
		double									_cloudDefualtMin{};											//云图-默认最小值
		bool									_cloudShowMax{ false };										//云图-是否显示最大值
		bool									_cloudShowMin{ false };										//云图-是否显示最小值
		bool									_cloudShowMaxLine{ false };									//云图-是否显示最大值线
		bool									_cloudShowMinLine{ false };									//云图-是否显示最小值线
		int										_cloudDiscrete{ 1 };										//云图-离散开关
		int										_cloudIsEquivariance{ 0 };									//云图-离散类型
		int										_cloudDiscreteNum{};										//云图-离散阶数
		double									_pointSize{ 10 };											//云图-点大小
		bool									_filterOpen{ false };										//云图-过滤开关
		QString									_filterRule{ "之间" };										//云图-过滤规则
		double									_filterMin{};												//云图-过滤最小值
		double									_filterMax{};												//云图-过滤最大值
        QHash<QString, PostData::oneCaseInfo*>	_caseInfos{};                                               //key-工况  value-工况信息
        bool									_isColorTableShow = true;									//云图-颜色表显示
        QVector<QColor>					        _defaultColors;                                             //云图-默认颜色
        QVector<QString>				        _defaultValues;                                             //云图-默认值
        int                                      _lightType = 1;                                            //云图-光源类型
        QString                                  _lightPos{"0,0,0"};                                        //云图-光源位置
        //以下滑条档位默认值须与渲染装配默认（LightInfo：0.3/0.6/0.6/32）换算一致
        int                                      _lightAmbient = 3;                                         //云图-环境光
        int                                      _lightDiffuse = 6;                                         //云图-漫反射光
        int                                      _lightMirror = 6;                                          //云图-镜面光
        float                                    _lightIntensity = 32;                                      //云图-镜面反射强度

		bool									_generateAnimate{ false };									//动画-是否生成
		bool									_aniHideFailedElement{ true };								//动画-是否隐藏失效单元
		QString									_aniType{};													//动画-类型
		QString									_aniControl{};												//动画-控制变量
		QString									_aniRangeVal1{};											//动画-范围起始值
		QString									_aniRangeVal2{};											//动画-范围结束值
		int										_aniCurrentFrame{};											//动画-当前帧
		int										_aniStartFrame{ 1 };										//动画-起始帧
		int										_aniEndFrame{ 99 };											//动画-结束帧
		int										_aniFrameInterval{ 1 };										//动画-帧间隔
		double									_aniCurrentFrameTime{};										//动画-当前帧时间
		double									_aniStartFrameTime{};										//动画-起始帧时间
		double									_aniEndFrameTime{};											//动画-结束帧时间
		double									_aniFrameIntervalTime{};									//动画-帧间隔时间
		int										_aniFrameNum{};												//动画-动画帧数
		bool									_aniIsStart{};												//动画-是否开启
		QString									_aniEveryRange{};											//动画-渲染范围
		int										_aniFrameRate{};											//动画-帧速率
		bool									_aniLoopPlay{};												//动画-是否循环播放
		int										_aniLinearCount{};											//动画-线性总帧数
		int										_aniFrameAmount{};										//动画-生成的动画帧数
		int										_aniShotFrame{ 1 };											//动画-输出截图帧数
		int										_aniVideoStartFrame{ 1 };									//动画-输出视频开始帧数
		int										_aniVideoEndFrame{};										//动画-输出视频阶数帧数
		//（2026-11-13 删除 _linearFrames/_linearFrameRatios：批次C 后全仓零读写的死字段，
		//生成参数已在 store 定义里；mPostGUIData 布局变更=全消费方重编，恢复构建后收口）

		int										_scalarOperationOp = 0;							// 0:+, 1:-, 2:*, 3:/
		double									_scalarOperationValue = 0.0;							// 标量值
		bool									_hasScalarOperation = false;							// 是否已经生成过标量运算
		int										_scalarOperationFrame = -1;								// 作用帧号

		QString									_cloudPart{};												//云图-部件类型（全部/选择）
		QList<int>								_cloudPartIds{};											//云图-部件Id
		bool									_cloudCoordinate{ false };
		QStringList								_cloudCoordinateNames{};
		QString									_currentCoordinateName{};
		int										_resultType{};												//云图-结果值类型 0-节点 1-单元
		bool									_isQuery{ false };											// 是否查询
		int										_queryIndex{ 0 };											// 查询的类型
		QString									_shapeType{};												//变形图-构型类型
		QString									_deformType{};												//变形图-变形类型
		QVector3D								_deformFactor{};											//变形图-变形系数

		//--------- 云图面板新字段（对齐前后处理整合原型；部分字段渲染端尚未对接，先落UI持久化） ---------
		bool									_useCornerData{ false };									//云图-使用角点数据（渲染TODO）
		QString									_renderObjectType{ "部件" };									//云图-渲染对象类型（单元/部件，渲染TODO）
		QString									_renderObjectScope{ "所有部件" };								//云图-渲染对象范围（渲染TODO）
		QString									_renderCoordSystem{ "全局坐标系" };							//云图-渲染坐标系（渲染TODO）
		bool									_useTrackingCoord{ false };									//云图-使用跟踪坐标（渲染TODO）
		bool									_midsideValues{ true };										//云图-边中点值（渲染TODO）
		QString									_smoothMethod{ "无" };										//云图-磨平方式（无/简单磨平/最大值/最小值/高级磨平/差值磨平/角点最大值/角点最小值/角点极值）
		bool									_smoothAcrossParts{ false };									//云图-跨部件磨平（渲染TODO）
		bool									_smoothVariationEnabled{ false };							//云图-高级磨平变化率启用（渲染TODO）
		double									_smoothVariationLimit{ 0 };									//云图-高级磨平变化率上限%（渲染TODO）
		QString									_legendRange{ "逐帧" };										//云图-图例范围（逐帧/全程/手动，默认逐帧）
		bool									_cloudRenderApplied{ false };								//云图-是否已应用渲染（清除渲染按钮状态源，随视口切换同步）
		bool									_pendingFitView{ false };									//云图-导入首显挂起视角适配（异步链下收尾 FitView 因帧未就绪早退时置位，slot_showCloud 就绪尾部补偿一次即复位，防切帧跳相机）
		QString									_labelVisible{ "显示" };										//云图-信息标签显隐（渲染TODO）
		QList<CloudPresetSnapshot>				_cloudPresets{};											//云图-缓存快照列表

	//--------- 变形控制面板字段 ---------
	QString									_deformVariable{};											//变形控制-变形变量（空或等于 _deformVarNames 首项=文件位移变量；否则按该变量矢量场变形）
	QStringList								_deformVarNames{};											//变形控制-变形变量列表（当前帧节点变量且分量数≥3，云图面板切帧时同步）
	QHash<int, QVector3D>					_origDisplacement{};										//变形控制-切非默认变量前缓存的原始位移场（键为连续节点id，与位移哈希同域）
	int										_origDisplacementFrame{ -1 };								//变形控制-原始位移缓存对应帧号（切帧失效重取）
		QString									_deformScaleStrategy{ "变形系数" };							//变形控制-缩放策略（变形系数/模型比例/模型单位；后两档渲染TODO）
		QString									_deformCoordSystem{ "全局坐标系" };							//变形控制-坐标系（渲染TODO）
		double									_deformUniformScale{ 1 };									//变形控制-等比例系数（与分方向XYZ _deformFactor 分开保存，切换模式各自恢复）
		QString									_initialDisplayMode{ "不显示" };								//变形控制-初始构型显示（不显示/线框/边线/特征线/仅显示初始构型；仅显示初始构型=变形系数置0）
		QString									_initialColorMode{ "部件" };									//变形控制-初始构型颜色（部件=部件色表/网格=默认黑/自定义=用户色）
		QColor									_initialCustomColor{ "#1890ff" };							//变形控制-初始构型自定义颜色（渲染TODO）
		QString									_aniRange{};												//动画-范围
		float									_aniRangeMin{};												//动画-最小值（仅自定义范围可用）
		float									_aniRangeMax{};												//动画-最大值（仅自定义范围可用）
		QList<ClipView>							_clipViews{};												//切割图
		QString									_clipType{"切割图"};										//切割图-切割类型
		QString									_clipStyle{"透明"};											//切割图-切面样式
		bool									_hasStreamLineView{};									//流线图-是否有流线图
		QString									_streamLineCenter{};									//流线图-球心
		QString									_streamLineRadius{};									//流线图-半径
		QString									_streamLineVectorName{};								//流线图-流线图矢量名字
		int										_streamLineNum{};										//流线图-流线密度
		float									_streamLineRatio{};									//流线图-流线积分比例
		QString									_streamLineStyle{};									//流线图-流线样式
		QString									_streamLineDirection{};								//流线图-积分方向
		bool									_streamLineSphere{};									//流线图-积分球
		QString									_streamLineElementType{"全部"};							//流线图-单元类型（全部或者拾取）
		std::set<int>							_streamLineElementIDs{};								//流线图-拾取单元ID
		//--------- 向量图（结果页"向量图"，源视口直作用）字段 ---------
		bool									_vectorPlotActive{ false };								//向量图-是否已应用渲染（切帧/重绘跟随重建的开关，随云图视口 guidata 存续）
		QString									_vectorPlotVarName{};									//向量图-矢量变量名（实际结果变量，数据驱动列表选中项）
		QVector<QPair<QString, QVector3D>>		_vectorPlotLabels{};									//向量图-渲染通道表（通道名X/Y/Z/XY/YZ/XZ/XYZ→颜色0-255，引擎VectorGraphParams.labels同域）
		double									_vectorPlotScale{ 1.0 };								//向量图-符号尺寸倍率（渲染端像素级统一长度×scale）
		int										_vectorPlotArrowType{ 1 };								//向量图-着色方式（0=自定义通道色 CommonArrow，1=数值 MinMax色表；引擎arrowType同域）
		bool									_vectorPlotShowText{ false };							//向量图-是否显示数值文字（通道模长）
		QString									_vectorPlotLegendRange{ "逐帧" };						//向量图-图例范围（逐帧/全程/手动，独立于云图_legendRange防语义越界）
		double									_vectorPlotLegendMax{ 1.0 };								//向量图-手动档上限缓存（apply 时锁定下发）
		double									_vectorPlotLegendMin{ 0.0 };								//向量图-手动档下限缓存
		bool									_vectorPlotEquivariance{ true };						//向量图-分布方式是否等差（true=等差 false=等比，引擎 _isEquivariance 同口径）
		double									_vectorPlotResolvedMax{ 0.0 };							//向量图-最近一次应用的解析值域上限（逐帧随帧/全程缓存/手动锁定；边界行回显与复位源）
		double									_vectorPlotResolvedMin{ 0.0 };							//向量图-最近一次应用的解析值域下限
		//--------- 截面剖切（结果页"截面剖切"，源视口直作用）字段 ---------
		bool									_sectionCutActive{ false };							//截面剖切-是否有激活截面（切帧/重绘跟随重建的开关，随云图视口 guidata 存续）
		QList<SectionCutSpec>					_sectionCutSpecs{};									//截面剖切-激活记录渲染投影表（面板激活/保存/删除时全量重投影）
		QVector<int>							_sectionCutEngineIds{};								//截面剖切-已提交引擎台账对象 id（按 id 精删防误伤旧切面图同族对象）
		//bool									_hasVectorArrowView{};										//是否有矢量图
		//QString									_vectorArrowVectorName{};									//矢量图-矢量图矢量名字
		//QString									_vectorArrowNodeType{"全部"};								//矢量图-节点类型（全部或者拾取）
		//std::set<int>							_vectorArrowNodeIDs{};										//矢量图-拾取节点ID
		//QVector<QPair<QString, QVector3D>>		_vectorArrowTypeColor{};									//矢量图-矢量图的箭头类型和颜色
		//QString									_vectorArrowMethod{};										//箭头大小的方式
		//double									_vectorArrowSize{1.0f};										//矢量图-矢量图的箭头大小
		//bool									_hasContourView{};											//是否有等值线等值面图图
		//bool									_contourIsAutoGenerate{};									//等值线等值面图-是否自动生成
		//bool									_contourIsRealTimeGenerate{};								//等值线等值面图-是否实时生成
		//QVector<ContourView>					_contourViews{};											//等值线等值面图-数据
	};
}