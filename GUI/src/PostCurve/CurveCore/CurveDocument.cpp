#include "CurveDocument.h"
#include "ChartTypeProfile.h"
#include "mCurveDocController.h"
#include <algorithm>
#include <QDebug>
#include <QPainter>
#include <QIcon>
#include <QLineF>
#include <QPolygon>

namespace PostCurve
{
	namespace
	{
		//--- 树节点图标（UI设计图02：每类型一枚；类型区用 UI 切图，绿色√状态指示仍程序化绘制） ---
		//（固定子节点"不可单独删除"仅由 removeNode 的 isFixedChildNode 守卫约束，
		//图标上不再叠加锁角标——对齐参考软件树的纯 √+类型图标形态）

		//类型字形：绘制到调用方画布的 (0,0)-(15,15) 逻辑坐标区
		//（调用方负责建位图/设置变换；坐标乘 DPR 后由 QPainter 缩放，保持高清）
		void paintTypeGlyph(QPainter& p, CurveNodeType type)
		{
			QPen pen(QColor(60, 64, 67), 1);
			p.setPen(pen);
			switch (type)
			{
			case CurveNodeType::Project:      //工程：蓝色文件夹
			{
				p.setBrush(QColor(66, 133, 244));
				p.setPen(Qt::NoPen);
				p.drawRect(1, 3, 6, 2);
				p.drawRect(1, 4, 14, 9);
				break;
			}
			case CurveNodeType::Page:         //页：白纸带折角与文字行
			{
				p.setBrush(Qt::white);
				p.drawRect(3, 1, 10, 14);
				pen.setColor(QColor(95, 99, 104));
				p.setPen(pen);
				p.drawLine(10, 1, 13, 4);
				pen.setColor(QColor(66, 133, 244));
				p.setPen(pen);
				p.drawLine(5, 7, 11, 7);
				p.drawLine(5, 10, 11, 10);
				break;
			}
			case CurveNodeType::Chart:        //视口：小图表框+折线
			{
				p.setBrush(Qt::white);
				p.drawRect(2, 3, 12, 10);
				pen.setColor(QColor(60, 64, 67));
				p.setPen(pen);
				p.drawLine(2, 3, 2, 13);
				p.drawLine(2, 13, 14, 13);
				pen.setColor(QColor(0, 102, 204));
				p.setPen(pen);
				p.drawLine(QLineF(4, 11, 7, 6));
				p.drawLine(QLineF(7, 6, 10, 9));
				p.drawLine(QLineF(10, 9, 13, 5));
				break;
			}
			case CurveNodeType::Curve:        //曲线：蓝色折线
			{
				pen.setColor(QColor(0, 102, 204));
				pen.setWidthF(1.6);
				p.setPen(pen);
				p.drawLine(QLineF(1, 12, 5, 4));
				p.drawLine(QLineF(5, 4, 9, 10));
				p.drawLine(QLineF(9, 10, 14, 3));
				break;
			}
		case CurveNodeType::XAxis:
		case CurveNodeType::XAxis2:      //顶部副轴：暂复用横轴字形
			pen.setColor(QColor(60, 64, 67));
			p.setPen(pen);
			p.drawLine(1, 8, 14, 8);
			p.drawLine(14, 8, 11, 6);
			p.drawLine(14, 8, 11, 10);
			p.drawLine(5, 7, 5, 9);
			p.drawLine(9, 7, 9, 9);
			break;
			case CurveNodeType::YAxis:        //纵轴：带箭头竖线+刻度
			{
				pen.setColor(QColor(60, 64, 67));
				p.setPen(pen);
				p.drawLine(8, 14, 8, 1);
				p.drawLine(8, 1, 6, 4);
				p.drawLine(8, 1, 10, 4);
				p.drawLine(7, 6, 9, 6);
				p.drawLine(7, 10, 9, 10);
				break;
			}
			case CurveNodeType::YAxis2:       //第二纵轴：橙色双竖线
			{
				pen.setColor(QColor(227, 116, 0));
				p.setPen(pen);
				p.drawLine(6, 14, 6, 2);
				p.drawLine(6, 2, 4, 5);
				p.drawLine(6, 2, 8, 5);
				pen.setColor(QColor(227, 116, 0, 140));
				p.setPen(pen);
				p.drawLine(11, 14, 11, 2);
				break;
			}
			case CurveNodeType::Legend:       //图例：小框内两行"线样+文字"
			{
				p.setBrush(QColor(255, 255, 255, 230));
				p.drawRect(2, 3, 12, 10);
				pen.setColor(QColor(0, 102, 204));
				p.setPen(pen);
				p.drawLine(4, 6, 8, 6);
				p.drawLine(4, 10, 8, 10);
				pen.setColor(QColor(130, 130, 130));
				p.setPen(pen);
				p.drawLine(9, 6, 13, 6);
				p.drawLine(9, 10, 13, 10);
				break;
			}
			case CurveNodeType::Header:       //页眉：顶部深色条+文字行
			{
				p.setPen(Qt::NoPen);
				p.setBrush(QColor(66, 133, 244));
				p.drawRect(2, 2, 12, 3);
				pen.setColor(QColor(130, 130, 130));
				p.setPen(pen);
				p.drawLine(2, 8, 14, 8);
				p.drawLine(2, 11, 11, 11);
				break;
			}
			case CurveNodeType::Footer:       //页脚：底部深色条+文字行
			{
				p.setPen(Qt::NoPen);
				p.setBrush(QColor(66, 133, 244));
				p.drawRect(2, 11, 12, 3);
				pen.setColor(QColor(130, 130, 130));
				p.setPen(pen);
				p.drawLine(2, 5, 14, 5);
				p.drawLine(2, 8, 11, 8);
				break;
			}
			case CurveNodeType::Baseline:     //基准线：蓝色实线+三角标记（默认线色蓝、实线）
			{
				pen.setColor(QColor(40, 90, 220));
				pen.setStyle(Qt::SolidLine);
				p.setPen(pen);
				p.drawLine(1, 10, 15, 10);
				p.setBrush(QColor(40, 90, 220));
				p.drawPolygon(QPolygon() << QPoint(8, 3) << QPoint(6, 7) << QPoint(10, 7));
				break;
			}
			case CurveNodeType::Annotation:   //注释：圆角文本框
			{
				p.setBrush(Qt::white);
				p.drawRoundedRect(2, 3, 12, 10, 2, 2);
				pen.setColor(QColor(150, 150, 150));
				p.setPen(pen);
				p.drawLine(5, 6, 11, 6);
				p.drawLine(5, 8, 11, 8);
				p.drawLine(5, 10, 9, 10);
				break;
			}
			case CurveNodeType::DatumPlane3D:  //三维基准面（R41）：半透明橙色平行四边形
			                                   //参考平面（透视倾斜面片；颜色=模型默认橙
			                                   //221,147,96，与基准线图标同"标记+图形"语言。
			                                   //资源库无基准面切图，走程序化字形）
			{
				pen.setColor(QColor(221, 147, 96));
				p.setPen(pen);
				p.setBrush(QColor(221, 147, 96, 110));
				p.drawPolygon(QPolygon() << QPoint(2, 12) << QPoint(6, 4)
					<< QPoint(14, 4) << QPoint(10, 12));
				break;
			}
			//曲线 x/y 数据节点无类型字形：名称本身即"x"/"y"，
			//再画字母图标会与文字重复（一行出现两个 X/Y）
			default:
				break;
			}
		}

		//节点类型图标资源
		//返回空串表示无对应切图，调用方退回程序化字形（曲线x/y数据直接不画，见 curveNodeIcon）
		//极坐标图表（chartType=3）主轴 X1/Y1 有专属切图（需求袋供图直用，禁自绘），
		//直角图表维持通用 X轴/Y轴 切图——polarAxis 按 data() 现算的父图表类型传入
		QString typeIconResource(CurveNodeType type, bool polarAxis)
		{
			switch (type)
			{
			case CurveNodeType::Project:    return QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/工程.svg");
			case CurveNodeType::Page:       return QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/页面.svg");
			case CurveNodeType::Chart:      return QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/图表.svg");
			case CurveNodeType::Curve:      return QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/曲线.svg");
			case CurveNodeType::XAxis:
				return polarAxis
					? QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/极坐标X1.svg")
					: QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/X轴.svg");
			case CurveNodeType::YAxis:
				return polarAxis
					? QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/极坐标Y1.svg")
					: QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/Y轴.svg");
			case CurveNodeType::YAxis2:     return QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/Y轴.svg");   //副轴无专属切图，暂复用
			case CurveNodeType::XAxis2:     return QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/X轴.svg");   //同上
			case CurveNodeType::Legend:     return QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/图例.svg");
			case CurveNodeType::Header:     return QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/页眉.svg");
			case CurveNodeType::Footer:     return QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/页脚.svg");
			case CurveNodeType::Baseline:   return QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/横向基准.svg"); //基准线节点未区分纵横，暂取横向
			case CurveNodeType::Annotation: return QStringLiteral(":/mMainWidgets/Icon/Curve/结构树/注释.svg");
			default:                        return QString();
			}
		}

		//节点状态图标（设计图02：绿色√=节点状态正常）。
		//仅为状态指示图标，非复选框、不参与交互；异常态（缺数据/失效等）后续扩展时替换字形。
		//按 2x DPI 绘制（高分辨率屏不发虚）；树视图 iconSize 须设为同等逻辑尺寸（28x16），
		//避免视图把宽图等比压进默认 16px 图标区导致整体缩小
		//polarAxis=true 用极坐标 X1/Y1 专属切图（仅 XAxis/YAxis 消费，缓存键并入极坐标档
		//——同一类型两种切图并存，直角档不受影响）
		QIcon curveNodeIcon(CurveNodeType type, bool polarAxis = false)
		{
			static QHash<int, QIcon> cache;   //按"类型+极坐标档"缓存（图标与节点状态无关）
			const int key = static_cast<int>(type) | (polarAxis ? 0x10000 : 0);
			auto it = cache.find(key);
			if (it == cache.end())
			{
				const qreal dpr = 2.0;
				const int checkW = 12;               //绿√区宽（含与类型图标的间隙）
				const QSize logical(checkW + 16, 16);
				QPixmap pm(qRound(logical.width() * dpr), qRound(logical.height() * dpr));
				pm.setDevicePixelRatio(dpr);
				pm.fill(Qt::transparent);
				QPainter p(&pm);
				p.setRenderHint(QPainter::Antialiasing);
				QPen pen(QColor(30, 142, 62), 2);    //绿色√
				pen.setCapStyle(Qt::RoundCap);
				pen.setJoinStyle(Qt::RoundJoin);
				p.setPen(pen);
				p.drawLine(QLineF(2, 8, 5, 11));
				p.drawLine(QLineF(5, 11, 10, 4));
				p.translate(checkW, 0);
			//曲线 x/y/yp 数据壳节点与三维图 x/y/z 轴壳节点对齐参考软件树的
			//x/y 行：仅状态√+纯文字"x"/"y"/"yp"/"z"，不画类型字形（字母图标与
			//文字重复）；画布仍占满 28x16 保证各行文字对齐
			if (type == CurveNodeType::CurveXData || type == CurveNodeType::CurveYData
				|| type == CurveNodeType::CurveY2Data
				|| type == CurveNodeType::AxisX3DData || type == CurveNodeType::AxisY3DData
				|| type == CurveNodeType::AxisZ3DData)
				{
					it = cache.insert(key, QIcon(pm));
					return it.value();
				}
				//类型区：优先用 UI 切图；按 2x 请求位图画进 16 逻辑px 区域，保证高分屏不发虚；
				//切图加载失败（资源缺失/插件异常）时退回程序化字形，保证树仍可用
				const QString res = typeIconResource(type, polarAxis);
				if (!res.isEmpty())
				{
					const QPixmap glyph = QIcon(res).pixmap(qRound(16 * dpr), qRound(16 * dpr));
					if (!glyph.isNull())
						p.drawPixmap(QRect(0, 0, 16, 16), glyph);
					else
						paintTypeGlyph(p, type);
				}
				else
					paintTypeGlyph(p, type);
				it = cache.insert(key, QIcon(pm));
			}
			return it.value();
		}

	//--- 子节点分区序已收口到 ChartTypeProfile（chartZoneRank，按图表类型查 zoneOrder） ---
}

	//节点状态图标对外导出（树/选取器共用；复用匿名命名空间内的合成与缓存）
	QIcon curveNodeStatusIcon(CurveNodeType type)
	{
		return curveNodeIcon(type);
	}

	CurveDocument::CurveDocument(QObject* parent)
		: QAbstractItemModel(parent)
	{
	_root = new CurveNode;
	_root->id = 0;
	_root->type = CurveNodeType::Project;
	_root->name = QStringLiteral("工程");
	_idMap.insert(0, _root);
	}

	CurveDocument::~CurveDocument()
	{
		freeNodeTree(_root);
	}

//--- QAbstractItemModel ---
//工程根（_root）作为树的唯一可见顶层节点：模型的"隐形根"是虚拟超根，
//顶层第0行即工程节点，页是工程的子节点（UI设计图02：工程 -> p1 : Page 1...）

QModelIndex CurveDocument::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	if (!parent.isValid())
		return createIndex(row, column, _root);   //唯一顶层行：工程

	const CurveNode* parentNode = static_cast<CurveNode*>(parent.internalPointer());
	if (!parentNode || row < 0 || row >= parentNode->children.size())
		return QModelIndex();
	return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex CurveDocument::parent(const QModelIndex& child) const
{
	if (!child.isValid())
		return QModelIndex();
	CurveNode* node = static_cast<CurveNode*>(child.internalPointer());
	if (!node || node == _root)
		return QModelIndex();                      //工程无父
	if (node->parent == _root)
		return createIndex(0, 0, _root);           //页的父 = 工程
	return indexFromNode(node->parent);
}

int CurveDocument::rowCount(const QModelIndex& parent) const
{
	if (!parent.isValid())
		return 1;                                  //顶层仅工程一行
	const CurveNode* node = static_cast<CurveNode*>(parent.internalPointer());
	return node ? node->children.size() : 0;
}

	int CurveDocument::columnCount(const QModelIndex& parent) const
	{
		Q_UNUSED(parent)
		return 1;
	}

	QVariant CurveDocument::data(const QModelIndex& index, int role) const
	{
		if (!index.isValid())
			return QVariant();
		CurveNode* node = static_cast<CurveNode*>(index.internalPointer());
		if (!node)
			return QVariant();
		switch (role)
		{
		case Qt::DisplayRole:
			//树显示名：页/视口带位置前缀 "p1 : "/"w1 : "（HyperGraph 风格，显示层现算不存储）
			return nodeDisplayText(node);
		case Qt::EditRole:
			//树内重命名编辑器取纯本名——前缀不进编辑器，不会被提交写回 node->name
			return node->name;
		case Qt::DecorationRole:
			//状态图标（绿色√=正常）+ 类型图标，仅指示非交互。
			//极坐标图表（chartType=3）主轴 X1/Y1 用专属切图（父图表类型现算，
			//副轴/直角图表轴不涉——极坐标无副轴）
			return curveNodeIcon(node->type,
				node->parent && node->parent->type == CurveNodeType::Chart
				&& node->parent->chart.chartType == 3
				&& (node->type == CurveNodeType::XAxis
					|| node->type == CurveNodeType::YAxis));
		case NodeTypeRole:
			return static_cast<int>(node->type);
		case NodeIdRole:
			return node->id;
		default:
			return QVariant();
		}
	}

	Qt::ItemFlags CurveDocument::flags(const QModelIndex& index) const
	{
		if (!index.isValid())
			return Qt::NoItemFlags;
		Qt::ItemFlags f = QAbstractItemModel::flags(index) | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
		CurveNode* node = static_cast<CurveNode*>(index.internalPointer());
		if (!node)
			return f;
		//树内重命名（双击/F2）：可命名节点见 CurveDefine.h::nodeTypeRenameable
		//（固定子节点（轴/图例/页眉页脚/x y）名固定不可编辑）
		if (nodeTypeRenameable(node->type))
			f |= Qt::ItemIsEditable;
		return f;
	}

	bool CurveDocument::setData(const QModelIndex& index, const QVariant& value, int role)
	{
		if (!index.isValid())
			return false;
		CurveNode* node = static_cast<CurveNode*>(index.internalPointer());
		if (!node)
			return false;
		mCurveDocController* controller = mCurveDocController::getInstance();
		if (role == Qt::EditRole)
		{
			//树内重命名：kName 命令（Curve 节点同步 series.name）；空名拒绝
			const QString name = value.toString().trimmed();
			if (name.isEmpty())
				return false;
			controller->setNodeProperty(node->id, CurveProp::kName, name);
			return true;
		}
		return QAbstractItemModel::setData(index, value, role);
	}

	//--- 节点查询 ---

	CurveNode* CurveDocument::node(int id) const
	{
		return _idMap.value(id, nullptr);
	}

	int CurveDocument::rootNodeId() const
	{
		return _root->id;
	}

	CurveNodeType CurveDocument::nodeType(int id) const
	{
		CurveNode* n = node(id);
		return n ? n->type : CurveNodeType::Project;
	}

	QString CurveDocument::nodeName(int id) const
	{
		CurveNode* n = node(id);
		return n ? n->name : QString();
	}

	QString CurveDocument::nodeDisplayName(int id) const
	{
		CurveNode* n = node(id);
		return n ? nodeDisplayText(n) : QString();
	}

	QString CurveDocument::nodeFullDisplayName(int id) const
	{
		CurveNode* n = node(id);
		if (!n || n->type != CurveNodeType::Chart || !n->parent)
			return nodeDisplayName(id);
		return QStringLiteral("p%1w%2 : %3")
			.arg(siblingPositionNumber(n->parent))
			.arg(siblingPositionNumber(n))
			.arg(n->name);
	}

	int CurveDocument::siblingPositionNumber(const CurveNode* n) const
	{
		if (!n || !n->parent)
			return 0;
		int no = 0;
		for (const CurveNode* c : n->parent->children)
		{
			if (c->type == n->type)
				++no;
			if (c == n)
				break;
		}
		return no;
	}

	QString CurveDocument::nodeDisplayText(const CurveNode* n) const
	{
		if (!n || !n->parent)
			return n ? n->name : QString();
		if (n->type == CurveNodeType::Page || n->type == CurveNodeType::Chart
			|| n->type == CurveNodeType::Curve)
		{
			//位置序只数同类型兄弟（根下只有页；页下只有视口；图表下的曲线区），
			//与 mDefineCurveDlg 的 p1w1c1 向量引用编号同一套规则——用户在树上
			//看到的序即可直接写引用；曲线序从 c1 起（第一条 c1、第二条 c2...）
			const QString prefix = (n->type == CurveNodeType::Page) ? QStringLiteral("p")
				: (n->type == CurveNodeType::Chart) ? QStringLiteral("w") : QStringLiteral("c");
			//复数对曲线（命名约定 -MAG|方向/-PHA|方向 等）：树显示去分量令牌，
			//"对象 - 方向"（设计稿样式）；模型名不变（渲染层配对键不受影响）
			QString name = n->name;
			if (n->type == CurveNodeType::Curve)
			{
				const int bar = name.indexOf('|');
				if (bar > 0)
				{
					static const QString kToks[] = { QStringLiteral("-MAG"),
						QStringLiteral("-PHA"), QStringLiteral("-REAL"),
						QStringLiteral("-IMAG") };
					const QString head = name.left(bar);
					for (const QString& tok : kToks)
					{
						if (head.endsWith(tok, Qt::CaseInsensitive))
						{
							name = head.left(head.size() - tok.size())
								+ QStringLiteral(" - ") + name.mid(bar + 1);
							break;
						}
					}
				}
			}
			return QStringLiteral("%1%2 : %3")
				.arg(prefix, QString::number(siblingPositionNumber(n)), name);
		}
		//固定子节点显示名按所属图表的类型声明覆写（ChartTypeProfile，原 chartType==2
		//特判已收口进注册表）：先动态规则（复数图 y 子节点 ym/yp），再静态覆写
		//（复数图 Y 轴 Magnitude）；都空或节点不在声明内则显示本名
		const CurveNode* chart = n->parent;
		while (chart && chart->type != CurveNodeType::Chart)
			chart = chart->parent;
		if (chart)
		{
			const ChartTypeProfile& prof = chartTypeProfile(chart->chart.chartType);
			if (prof.displayOverride)
			{
				const QString dynamic = prof.displayOverride(this, n);
				if (!dynamic.isEmpty())
					return dynamic;
			}
			//复数图 Y 轴行显示名随显示格式联动（设计图01/参考实现：幅值/相位格式
			//显示 Magnitude/Phase，实部/虚部格式显示 Real/Imaginary；与下面板轴
			//标题"Phase"/"Imaginary"联动同源，均为显示层现算，模型名不变）
			if (chart->chart.chartType == 2
				&& (n->type == CurveNodeType::YAxis || n->type == CurveNodeType::YAxis2))
			{
				const bool magPhase = chart->chart.complexDisplayFormat == 0;
				if (n->type == CurveNodeType::YAxis)
					return magPhase ? QStringLiteral("Magnitude") : QStringLiteral("Real");
				return magPhase ? QStringLiteral("Phase") : QStringLiteral("Imaginary");
			}
			for (const ChartFixedChild& fc : prof.fixedChildren)
			{
				if (fc.type == n->type && !fc.displayName.isEmpty())
					return fc.displayName;
			}
		}
		return n->name;
	}

	void CurveDocument::refreshNumberedSiblings(CurveNode* parent)
	{
		if (!parent || parent->children.isEmpty())
			return;
		const QModelIndex parentIndex = indexFromNode(parent);
		if (!parentIndex.isValid())
			return;
		const QModelIndex first = index(0, 0, parentIndex);
		const QModelIndex last = index(parent->children.size() - 1, 0, parentIndex);
		if (first.isValid() && last.isValid())
			emit dataChanged(first, last);
	}

	int CurveDocument::parentNodeId(int id) const
	{
		CurveNode* n = node(id);
		return (n && n->parent) ? n->parent->id : -1;
	}

	QList<int> CurveDocument::childNodeIds(int id) const
	{
		QList<int> ids;
		CurveNode* n = node(id);
		if (n)
		{
			for (CurveNode* c : n->children)
				ids.append(c->id);
		}
		return ids;
	}

	int CurveDocument::chartNodeId(int id) const
	{
		CurveNode* n = node(id);
		while (n)
		{
			if (n->type == CurveNodeType::Chart)
				return n->id;
			n = n->parent;
		}
		return -1;
	}

	QStringList CurveDocument::chartCategoryTickLabels(int chartId) const
	{
		for (int childId : childNodeIds(chartId))
		{
			CurveNode* n = node(childId);
			if (n && n->type == CurveNodeType::Curve && !n->series.xTickLabels.isEmpty())
				return n->series.xTickLabels;
		}
		return QStringList();
	}

	QVector<double> CurveDocument::chartBarXValues(int chartId) const
	{
		for (int childId : childNodeIds(chartId))
		{
			CurveNode* n = node(childId);
			if (!n || n->type != CurveNodeType::Curve)
				continue;
			//升序去重：起始/终止按"从小到大的可选值"展示与回显（xData 顺序
			//不保证有序，重复值只留一项）
			QVector<double> vals = n->series.xData;
			std::sort(vals.begin(), vals.end());
			vals.erase(std::unique(vals.begin(), vals.end()), vals.end());
			return vals;
		}
		return QVector<double>();
	}

QModelIndex CurveDocument::indexFromNode(const CurveNode* node) const
{
	if (!node || !_idMap.contains(node->id))
		return QModelIndex();
	if (node == _root)
		return createIndex(0, 0, _root);   //工程根位于顶层第0行（可见）
		//节点在父中的行号
		const CurveNode* p = node->parent;
		int row = p ? p->children.indexOf(const_cast<CurveNode*>(node)) : -1;
		if (row < 0)
			return QModelIndex();
		return createIndex(row, 0, const_cast<CurveNode*>(node));
	}

	QModelIndex CurveDocument::indexFromNodeId(int id) const
	{
		CurveNode* n = node(id);
		return n ? indexFromNode(n) : QModelIndex();
	}

	//--- 属性读写 ---

	QVariant CurveDocument::nodeProperty(int id, const QString& prop) const
	{
		CurveNode* n = node(id);
		if (!n)
			return QVariant();
		const QString p = prop;
		if (p == CurveProp::kName)
			return n->name;
		//x/y/yp 数据壳节点：数据实际存在父曲线节点的 series.xRef/yRef/complexRef
		//（设计图03.2 与复数曲线"yp"），读写重定向到父 Curve；来源四键按侧重定向
		//到侧独立来源（X 侧 series.xSide*，Y 侧 series.ySide*——Y 侧旧数据未记录
		//时回退创建时写入的五元组，yp 侧 series.complexSide* 建时必写），子工况
		//在横坐标形态下回显锚通道工况
		if (n->type == CurveNodeType::CurveXData || n->type == CurveNodeType::CurveYData
			|| n->type == CurveNodeType::CurveY2Data)
		{
			CurveNode* curve = n->parent;
			if (!curve)
				return QVariant();
			const CurveDataRefData& ref = (n->type == CurveNodeType::CurveXData)
				? curve->series.xRef
				: (n->type == CurveNodeType::CurveYData)
				? curve->series.yRef : curve->series.complexRef;
			if (p == CurveProp::kScale)      return ref.scale;
			if (p == CurveProp::kOffset)     return ref.offset;
			if (p == CurveProp::kPhase)      return ref.phase;
			if (p == CurveProp::kClipEnabled) return ref.clipEnabled;
			if (p == CurveProp::kClipMin)    return ref.clipMin;
			if (p == CurveProp::kClipMax)    return ref.clipMax;
			if (p == CurveProp::kExpression) return ref.expression;
			if (p == CurveProp::kAxisAssign) return ref.axisAssign;
			if (p == CurveProp::kUnit)       return ref.unit;
			if (p == CurveProp::kSourceFile) return curve->series.sourceFile;
			if (n->type == CurveNodeType::CurveXData)
			{
				if (p == CurveProp::kSubcase)   return curve->series.xSideSubcase.isEmpty()
					? curve->series.subcase : curve->series.xSideSubcase;
				if (p == CurveProp::kSchema)    return curve->series.xSideSchema;
				if (p == CurveProp::kObject)    return curve->series.xSideObject;
				if (p == CurveProp::kComponent) return curve->series.xSideComponent;
			}
			else if (n->type == CurveNodeType::CurveY2Data)
			{
				//yp 壳：下面板侧来源（complexSide* 创建时必写，不回退五元组）
				if (p == CurveProp::kSubcase)   return curve->series.complexSideSubcase;
				if (p == CurveProp::kSchema)    return curve->series.complexSideSchema;
				if (p == CurveProp::kObject)    return curve->series.complexSideObject;
				if (p == CurveProp::kComponent) return curve->series.complexSideComponent;
			}
			else
			{
				const bool hasYSide = !curve->series.ySideSchema.isEmpty();
				if (p == CurveProp::kSubcase)   return hasYSide ? curve->series.ySideSubcase : curve->series.subcase;
				if (p == CurveProp::kSchema)    return hasYSide ? curve->series.ySideSchema : curve->series.schema;
				if (p == CurveProp::kObject)    return hasYSide ? curve->series.ySideObject : curve->series.object;
				if (p == CurveProp::kComponent) return hasYSide ? curve->series.ySideComponent : curve->series.component;
			}
			return QVariant();
		}
		//三维图轴壳（AxisX3DData/Y/Z，R28 入树）：轴状态存所属图表 chart.axis3d
		//[轴下标]（图表级唯一——同图表多曲线共享同轴组），读写重定向跨两级
		//（壳->曲线->图表）
		if (n->type == CurveNodeType::AxisX3DData || n->type == CurveNodeType::AxisY3DData
			|| n->type == CurveNodeType::AxisZ3DData)
		{
			CurveNode* curve = n->parent;
			CurveNode* chart = curve ? curve->parent : nullptr;
			if (!chart || chart->type != CurveNodeType::Chart)
				return QVariant();
			const CurveAxis3DState& ax = chart->chart.axis3d.axes[
				(n->type == CurveNodeType::AxisX3DData) ? 0
				: (n->type == CurveNodeType::AxisY3DData) ? 1 : 2];
			if (p == CurveProp::kAxis3DLabel)        return ax.label;
			if (p == CurveProp::kAxis3DMin)          return ax.min;
			if (p == CurveProp::kAxis3DMax)          return ax.max;
			if (p == CurveProp::kAxis3DTickCount)    return ax.tickCount;
			if (p == CurveProp::kAxis3DGridVisible)  return ax.gridVisible;
			if (p == CurveProp::kAxis3DAutoFit)      return ax.autoFit;
			if (p == CurveProp::kAxis3DScaleType)    return ax.scaleType;   //刻度类型（R43）
			return QVariant();
		}
		//页眉/页脚数据实际挂在图表节点上：Header/Footer 子节点读写重定向到父 Chart
		if (n->type == CurveNodeType::Header || n->type == CurveNodeType::Footer)
			n = n->parent;
		if (!n)
			return QVariant();
		switch (n->type)
		{
		case CurveNodeType::Page:
			if (p == CurveProp::kWidth)            return n->page.width;
			if (p == CurveProp::kFont)             return n->page.font;
			if (p == CurveProp::kLayout)           return n->page.layout;
			if (p == CurveProp::kPageTitle)        return n->page.pageTitle;
			if (p == CurveProp::kPublishLayout)    return n->page.publishLayout;
			if (p == CurveProp::kPublishPageTitle) return n->page.publishPageTitle;
			if (p == CurveProp::kPublishContentPos) return n->page.publishContentPos;
			break;
		case CurveNodeType::Chart:
			if (p == CurveProp::kChartVisible)  return n->chart.visible;
			if (p == CurveProp::kChartType)     return n->chart.chartType;
			if (p == CurveProp::kBackgroundColor) return n->chart.backgroundColor;
			if (p == CurveProp::kGridColor)     return n->chart.gridColor;
			if (p == CurveProp::kComplexDisplayFormat) return n->chart.complexDisplayFormat;
			if (p == CurveProp::kComplexSwapped) return n->chart.complexSwapped;
			if (p == CurveProp::kDatum3DVisible) return n->chart.datum3DVisible;   //三维基准面图表级总开关（R41）
			if (p == CurveProp::kAnnotationsVisible) return n->chart.annotationsVisible;   //注释图表级总开关（R42）
			//三维图图表级网格样式/背景面配色（R44，存 chart.axis3d；任意轴壳页
			//编辑全图生效=图表级语义）
			if (p == CurveProp::kChart3DGridStyle) return n->chart.axis3d.gridStyle;
			if (p == CurveProp::kChart3DBackdropColor) return n->chart.axis3d.backdropColor;
			if (p == CurveProp::kChart3DBackdropGridColor) return n->chart.axis3d.backdropGridColor;
			if (p == CurveProp::kHeaderVisible) return n->header.visible;
			if (p == CurveProp::kHeaderText)    return n->header.text;
			if (p == CurveProp::kHeaderAlign)   return n->header.align;
			if (p == CurveProp::kHeaderFont)    return n->header.font;
			if (p == CurveProp::kHeaderFont2)   return n->header.font2;
			if (p == CurveProp::kHeaderFont3)   return n->header.font3;
			if (p == CurveProp::kHeaderEmphLine) return n->header.emphLine;
			if (p == CurveProp::kHeaderColor)   return n->header.color;
			if (p == CurveProp::kFooterVisible) return n->footer.visible;
			if (p == CurveProp::kFooterText)    return n->footer.text;
			if (p == CurveProp::kFooterAlign)   return n->footer.align;
			if (p == CurveProp::kFooterFont)    return n->footer.font;
			if (p == CurveProp::kFooterFont2)   return n->footer.font2;
			if (p == CurveProp::kFooterFont3)   return n->footer.font3;
			if (p == CurveProp::kFooterColor)   return n->footer.color;
			break;
		case CurveNodeType::Curve:
			if (p == CurveProp::kVisible)     return n->series.visible;
			if (p == CurveProp::kColor)       return n->series.color;
			if (p == CurveProp::kLineStyle)   return n->series.lineStyle;
			if (p == CurveProp::kLineWidth)   return n->series.lineWidth;
			if (p == CurveProp::kShowSymbol)  return n->series.showSymbol;
			if (p == CurveProp::kSymbolSize)  return n->series.symbolSize;
			if (p == CurveProp::kSymbolColor) return n->series.symbolColor;
			if (p == CurveProp::kSymbolStyle) return n->series.symbolStyle;
			if (p == CurveProp::kSymbolDensity) return n->series.symbolDensity;
			if (p == CurveProp::kSymbolScale) return n->series.symbolScale;
			if (p == CurveProp::kCurveCategory) return static_cast<int>(n->series.curveCategory);
			if (p == CurveProp::kLabelVisible) return n->series.labelVisible;
			if (p == CurveProp::kLabelContent) return n->series.labelContent;
			if (p == CurveProp::kLabelText)    return n->series.labelText;
			if (p == CurveProp::kLabelPrefix)  return n->series.labelPrefix;
			if (p == CurveProp::kLabelSuffix)  return n->series.labelSuffix;
			if (p == CurveProp::kLabelPrefixVisible) return n->series.labelPrefixVisible;
			if (p == CurveProp::kLabelSuffixVisible) return n->series.labelSuffixVisible;
			if (p == CurveProp::kAreaFill)     return n->series.areaFill;
			if (p == CurveProp::kAreaFillOpacity) return n->series.areaFillOpacity;
			if (p == CurveProp::kContour)      return n->series.contour;
			if (p == CurveProp::kContourVar)   return n->series.contourVar;
			if (p == CurveProp::kDrawStyle)    return n->series.drawStyle;
			break;
		case CurveNodeType::XAxis:
		case CurveNodeType::YAxis:
		case CurveNodeType::YAxis2:
		case CurveNodeType::XAxis2:
			if (p == CurveProp::kTitle)     return n->axis.title;
			if (p == CurveProp::kTitleVisible) return n->axis.titleVisible;
			if (p == CurveProp::kTitleColor) return n->axis.titleColor;
			if (p == CurveProp::kVisible)   return n->axis.visible;
			if (p == CurveProp::kScaleMode) return n->axis.scaleMode;
			if (p == CurveProp::kWeightMode) return n->axis.weightMode;
			if (p == CurveProp::kAutoScale) return n->axis.autoScale;
			if (p == CurveProp::kMin)       return n->axis.min;
			if (p == CurveProp::kMax)       return n->axis.max;
			if (p == CurveProp::kTitleFont) return n->axis.titleFont;
			if (p == CurveProp::kLabelFont) return n->axis.labelFont;
			if (p == CurveProp::kUnit)      return n->axis.unit;
			if (p == CurveProp::kAxisOffset) return n->axis.offset;
			if (p == CurveProp::kMajorStep) return n->axis.majorStep;
			if (p == CurveProp::kMinorCount) return n->axis.minorCount;
			if (p == CurveProp::kDbRef) return n->axis.dbRef;
			if (p == CurveProp::kLabelFormat) return n->axis.labelFormat;
			if (p == CurveProp::kTickGenMode) return n->axis.tickGenMode;
			if (p == CurveProp::kTickCount) return n->axis.tickCount;
			if (p == CurveProp::kDecadeTickCount) return n->axis.decadeTickCount;
			if (p == CurveProp::kDecadeMinorCount) return n->axis.decadeMinorCount;
			if (p == CurveProp::kPrecision) return n->axis.precision;
			if (p == CurveProp::kCategoryLabelAlign) return n->axis.categoryLabelAlign;
			if (p == CurveProp::kTickValues) return QVariant::fromValue(n->axis.tickValues);   //QVector<double> 值类型（列表标注法）
			break;
		case CurveNodeType::Legend:
			if (p == CurveProp::kVisible)      return n->legend.visible;
			if (p == CurveProp::kAutoPosition) return n->legend.autoPosition;
			if (p == CurveProp::kPosition)     return n->legend.position;
			if (p == CurveProp::kPosX)         return n->legend.posX;   //画布百分比
			if (p == CurveProp::kPosY)         return n->legend.posY;
			if (p == CurveProp::kBorderStyle)  return n->legend.borderStyle;
			if (p == CurveProp::kLegendBorderWidth) return n->legend.borderWidth;
			if (p == CurveProp::kBorderColor)  return n->legend.borderColor;
			if (p == CurveProp::kLegendFont)   return n->legend.font;
			if (p == CurveProp::kTextColorMode) return n->legend.textColorMode;
			if (p == CurveProp::kTextColor)    return n->legend.textColor;
			if (p == CurveProp::kBgColor)      return n->legend.bgColor;
			if (p == CurveProp::kLeaderPos)    return n->legend.leaderPos;
			break;
		case CurveNodeType::Baseline:
			if (p == CurveProp::kLabel)       return n->baseline.label;
			if (p == CurveProp::kOrientation) return n->baseline.orientation;
			if (p == CurveProp::kValue)       return n->baseline.value;
			if (p == CurveProp::kColor)       return n->baseline.color;
			if (p == CurveProp::kLineStyle)   return n->baseline.lineStyle;
			if (p == CurveProp::kLineWidth)   return n->baseline.lineWidth;
			if (p == CurveProp::kVisible)     return n->baseline.visible;
			if (p == CurveProp::kLayer)       return n->baseline.layer;
			if (p == CurveProp::kLabelFont)   return n->baseline.labelFont;
			if (p == CurveProp::kLabelPos)    return n->baseline.labelPos;
			if (p == CurveProp::kLink)        return n->baseline.link;
			break;
		case CurveNodeType::Annotation:
			if (p == CurveProp::kText)  return n->annotation.text;
			if (p == CurveProp::kPosX)  return n->annotation.posX;
			if (p == CurveProp::kPosY)  return n->annotation.posY;
			if (p == CurveProp::kColor) return n->annotation.color;
			if (p == CurveProp::kVisible)    return n->annotation.visible;
			if (p == CurveProp::kAlign)      return n->annotation.align;
			if (p == CurveProp::kFont)       return n->annotation.font;
			if (p == CurveProp::kBorderMode)  return n->annotation.borderMode;
			if (p == CurveProp::kBorderColor) return n->annotation.borderColor;
			if (p == CurveProp::kBorderWidth) return n->annotation.borderWidth;
			if (p == CurveProp::kBorderShow)  return n->annotation.borderShow;   //显示边框（R42 二期）
			if (p == CurveProp::kPosZ)        return n->annotation.posZ;   //3D 锚点 Z（R42 修订；键复用基准面侧同名，分发按类型）
			if (p == CurveProp::kBgColor)     return n->annotation.bgColor;
			if (p == CurveProp::kAutoPos)    return n->annotation.autoPos;
			if (p == CurveProp::kAutoAnchor) return n->annotation.autoAnchor;
			if (p == CurveProp::kAutoOffset) return n->annotation.autoOffset;
			if (p == CurveProp::kAttachMode)  return n->annotation.attachMode;
			if (p == CurveProp::kAttachCurve) return n->annotation.attachCurve;
			if (p == CurveProp::kAttachIndex) return n->annotation.attachIndex;
			//3D 框位置三键（R46；NaN=未手动摆位，框=锚点+固定偏移。键为本类型专属）
			if (p == CurveProp::kBoxPosX) return n->annotation.boxPosX;
			if (p == CurveProp::kBoxPosY) return n->annotation.boxPosY;
			if (p == CurveProp::kBoxPosZ) return n->annotation.boxPosZ;
			if (p == CurveProp::kMinimized)      return n->annotation.minimized;
			if (p == CurveProp::kMinimizeRadius) return n->annotation.minimizeRadius;
			if (p == CurveProp::kMinimizeColor)  return n->annotation.minimizeColor;
			if (p == CurveProp::kScalarFmtOn)        return n->annotation.scalarFmtOn;
			if (p == CurveProp::kScalarFmtMode)      return n->annotation.scalarFmtMode;
			if (p == CurveProp::kScalarFmtPrecision) return n->annotation.scalarFmtPrecision;
			break;
		case CurveNodeType::DatumPlane3D:
			//三维基准面（R41）：kName 走函数头公共分支；kVisible/kColor/kText/kLabelPos/
			//kPosX/kPosY 复用既有键（分发按节点类型区分），其余为本类型专属键
			if (p == CurveProp::kVisible)      return n->datumPlane3D.visible;
			if (p == CurveProp::kColor)        return n->datumPlane3D.color;
			if (p == CurveProp::kText)         return n->datumPlane3D.text;
			if (p == CurveProp::kLabelPos)     return n->datumPlane3D.labelPos;
			if (p == CurveProp::kPosX)         return n->datumPlane3D.posX;   //参考坐标（非画布百分比语义）
			if (p == CurveProp::kPosY)         return n->datumPlane3D.posY;
			if (p == CurveProp::kDatumType)    return n->datumPlane3D.type;
			if (p == CurveProp::kNormalAxis)   return n->datumPlane3D.normalAxis;
			if (p == CurveProp::kPosZ)         return n->datumPlane3D.posZ;
			if (p == CurveProp::kDatumOpacity) return n->datumPlane3D.opacity;
			if (p == CurveProp::kReverseClip)  return n->datumPlane3D.reverseClip;
			break;
		default:
			break;
		}
		return QVariant();
	}

	bool CurveDocument::setNodeProperty(int id, const QString& prop, const QVariant& value, QVariant* oldValue)
	{
		CurveNode* n = node(id);
		if (!n)
			return false;

		//先取旧值并校验属性是否支持
		QVariant oldVal = nodeProperty(id, prop);
		if (!oldVal.isValid() && prop != CurveProp::kName)
			return false;
		if (oldValue)
			*oldValue = oldVal;

		//页眉/页脚数据实际挂在图表节点上：Header/Footer 子节点写入重定向到父 Chart
		//（变更信号仍以原节点 id 发出，属性页按自身绑定 id 正常回填）；
		//x/y/yp 数据壳节点同理重定向到父 Curve（写入 series.xRef/yRef/complexRef）；
		//三维图轴壳（AxisX3DData/Y/Z，R28）重定向跨两级到所属图表（轴状态
		//chart.axis3d[轴下标]，图表级唯一），axis3dIdx>=0 标记轴下标
		CurveNode* shellSide = nullptr;
		int axis3dIdx = -1;
		if (n->type == CurveNodeType::Header || n->type == CurveNodeType::Footer
			|| n->type == CurveNodeType::CurveXData || n->type == CurveNodeType::CurveYData
			|| n->type == CurveNodeType::CurveY2Data)
		{
			if (n->type == CurveNodeType::CurveXData || n->type == CurveNodeType::CurveYData
				|| n->type == CurveNodeType::CurveY2Data)
				shellSide = n;
			n = n->parent;
			if (!n)
				return false;
		}
		if (n->type == CurveNodeType::AxisX3DData || n->type == CurveNodeType::AxisY3DData
			|| n->type == CurveNodeType::AxisZ3DData)
		{
			//三维图轴壳不支持重命名（nodeTypeRenameable 恒 false，防御树外入口）
			if (prop == CurveProp::kName)
				return false;
			axis3dIdx = (n->type == CurveNodeType::AxisX3DData) ? 0
				: (n->type == CurveNodeType::AxisY3DData) ? 1 : 2;
			n = n->parent ? n->parent->parent : nullptr;   //壳->曲线->图表
			if (!n || n->type != CurveNodeType::Chart)
				return false;
		}

		const QString p = prop;
		if (p == CurveProp::kName)
		{
			n->name = value.toString();
			//曲线重命名联动渲染键（图例/曲线名，ReadMe 待办）：series.name 与节点名保持一致
			if (n->type == CurveNodeType::Curve)
				n->series.name = value.toString();
		}
		else
		{
			if (shellSide)
			{
				//x/y/yp 数据属性：按壳节点侧别写入 xRef/yRef/complexRef；来源四键写入
				//侧独立来源（参数栏 x/y 数据页换源；横坐标形态下 object/component 写空；
				//yp 侧写 complexSide*）。五元组仍由 setSeriesSource 维护（"更改曲线"
				//整线换源/撤回快照链路）
				CurveDataRefData& ref = (shellSide->type == CurveNodeType::CurveXData)
					? n->series.xRef
					: (shellSide->type == CurveNodeType::CurveYData)
					? n->series.yRef : n->series.complexRef;
				if (p == CurveProp::kScale)       ref.scale = value.toDouble();
				else if (p == CurveProp::kOffset)      ref.offset = value.toDouble();
				else if (p == CurveProp::kPhase)       ref.phase = value.toDouble();
				else if (p == CurveProp::kClipEnabled) ref.clipEnabled = value.toBool();
				else if (p == CurveProp::kClipMin)     ref.clipMin = value.toDouble();
				else if (p == CurveProp::kClipMax)     ref.clipMax = value.toDouble();
				else if (p == CurveProp::kExpression)  ref.expression = value.toString();
				else if (p == CurveProp::kAxisAssign)  ref.axisAssign = value.toInt();
				else if (p == CurveProp::kUnit)        ref.unit = value.toString();
				else if (p == CurveProp::kSubcase)
				{
					if (shellSide->type == CurveNodeType::CurveXData) n->series.xSideSubcase = value.toString();
					else if (shellSide->type == CurveNodeType::CurveY2Data) n->series.complexSideSubcase = value.toString();
					else n->series.ySideSubcase = value.toString();
				}
				else if (p == CurveProp::kSchema)
				{
					if (shellSide->type == CurveNodeType::CurveXData) n->series.xSideSchema = value.toString();
					else if (shellSide->type == CurveNodeType::CurveY2Data) n->series.complexSideSchema = value.toString();
					else n->series.ySideSchema = value.toString();
				}
				else if (p == CurveProp::kObject)
				{
					if (shellSide->type == CurveNodeType::CurveXData) n->series.xSideObject = value.toString();
					else if (shellSide->type == CurveNodeType::CurveY2Data) n->series.complexSideObject = value.toString();
					else n->series.ySideObject = value.toString();
				}
				else if (p == CurveProp::kComponent)
				{
					if (shellSide->type == CurveNodeType::CurveXData) n->series.xSideComponent = value.toString();
					else if (shellSide->type == CurveNodeType::CurveY2Data) n->series.complexSideComponent = value.toString();
					else n->series.ySideComponent = value.toString();
				}
				else
					return false;
			}
			else if (axis3dIdx >= 0)
			{
				//三维图轴状态（图表级，T7 轴属性页提交入口；渲染联动=m3DChartView
				//按壳节点 id 的 nodePropertyChanged 重建）
				CurveAxis3DState& ax = n->chart.axis3d.axes[axis3dIdx];
				if (p == CurveProp::kAxis3DLabel)        ax.label = value.toString();
				else if (p == CurveProp::kAxis3DMin)         ax.min = value.toDouble();
				else if (p == CurveProp::kAxis3DMax)         ax.max = value.toDouble();
				else if (p == CurveProp::kAxis3DTickCount)   ax.tickCount = value.toInt();
				else if (p == CurveProp::kAxis3DGridVisible) ax.gridVisible = value.toBool();
				else if (p == CurveProp::kAxis3DAutoFit)     ax.autoFit = value.toBool();
				//刻度类型（R43）：沿既有键口径写侧不做钳制（页侧保证 0..3，
				//R38 评审备案口径；序列化读侧钳制防脏数据）
				else if (p == CurveProp::kAxis3DScaleType)   ax.scaleType = value.toInt();
				else
					return false;
			}
			else switch (n->type)
			{
			case CurveNodeType::Page:
				if (p == CurveProp::kWidth) n->page.width = value.toDouble();
				else if (p == CurveProp::kFont) n->page.font = value.value<QFont>();
				else if (p == CurveProp::kLayout) n->page.layout = value.toInt();
				else if (p == CurveProp::kPageTitle) n->page.pageTitle = value.toString();
				else if (p == CurveProp::kPublishLayout) n->page.publishLayout = value.toBool();
				else if (p == CurveProp::kPublishPageTitle) n->page.publishPageTitle = value.toBool();
				else if (p == CurveProp::kPublishContentPos) n->page.publishContentPos = value.toInt();
				break;
			case CurveNodeType::Chart:
				if (p == CurveProp::kChartVisible) n->chart.visible = value.toBool();
				else if (p == CurveProp::kChartType) n->chart.chartType = value.toInt();
				else if (p == CurveProp::kBackgroundColor) n->chart.backgroundColor = value.value<QColor>();
				else if (p == CurveProp::kGridColor) n->chart.gridColor = value.value<QColor>();
				else if (p == CurveProp::kComplexDisplayFormat) n->chart.complexDisplayFormat = value.toInt();
				else if (p == CurveProp::kComplexSwapped) n->chart.complexSwapped = value.toBool();
				else if (p == CurveProp::kDatum3DVisible) n->chart.datum3DVisible = value.toBool();   //三维基准面图表级总开关（R41）
				else if (p == CurveProp::kAnnotationsVisible) n->chart.annotationsVisible = value.toBool();   //R42 二期
				//三维图图表级网格样式/背景面配色（R44）：gridStyle 写侧不钳制
				//（页侧保证 0..1，沿既有键口径；序列化读侧钳制防脏数据），两色
				//经 QVariant value.value<QColor>() 直存
				else if (p == CurveProp::kChart3DGridStyle) n->chart.axis3d.gridStyle = value.toInt();
				else if (p == CurveProp::kChart3DBackdropColor) n->chart.axis3d.backdropColor = value.value<QColor>();
				else if (p == CurveProp::kChart3DBackdropGridColor) n->chart.axis3d.backdropGridColor = value.value<QColor>();
				else if (p == CurveProp::kHeaderVisible) n->header.visible = value.toBool();
				else if (p == CurveProp::kHeaderText) n->header.text = value.toString();
				else if (p == CurveProp::kHeaderAlign) n->header.align = value.toInt();
				else if (p == CurveProp::kHeaderFont) n->header.font = value.value<QFont>();
				else if (p == CurveProp::kHeaderFont2) n->header.font2 = value.value<QFont>();
				else if (p == CurveProp::kHeaderFont3) n->header.font3 = value.value<QFont>();
				else if (p == CurveProp::kHeaderEmphLine) n->header.emphLine = value.toInt();
				else if (p == CurveProp::kHeaderColor) n->header.color = value.value<QColor>();
				else if (p == CurveProp::kFooterVisible) n->footer.visible = value.toBool();
				else if (p == CurveProp::kFooterText) n->footer.text = value.toString();
				else if (p == CurveProp::kFooterAlign) n->footer.align = value.toInt();
				else if (p == CurveProp::kFooterFont) n->footer.font = value.value<QFont>();
				else if (p == CurveProp::kFooterFont2) n->footer.font2 = value.value<QFont>();
				else if (p == CurveProp::kFooterFont3) n->footer.font3 = value.value<QFont>();
				else if (p == CurveProp::kFooterColor) n->footer.color = value.value<QColor>();
				break;
			case CurveNodeType::Curve:
				if (p == CurveProp::kVisible) n->series.visible = value.toBool();
				else if (p == CurveProp::kColor) n->series.color = value.value<QColor>();
				else if (p == CurveProp::kLineStyle) n->series.lineStyle = value.toInt();
				else if (p == CurveProp::kLineWidth) n->series.lineWidth = value.toDouble();
				else if (p == CurveProp::kShowSymbol) n->series.showSymbol = value.toBool();
					else if (p == CurveProp::kSymbolSize) n->series.symbolSize = value.toInt();
					else if (p == CurveProp::kSymbolColor) n->series.symbolColor = value.value<QColor>();
					else if (p == CurveProp::kSymbolStyle) n->series.symbolStyle = value.toInt();
					else if (p == CurveProp::kSymbolDensity) n->series.symbolDensity = value.toInt();
					else if (p == CurveProp::kSymbolScale) n->series.symbolScale = value.toDouble();
					else if (p == CurveProp::kCurveCategory) n->series.curveCategory = static_cast<CurveCategory>(value.toInt());
					else if (p == CurveProp::kLabelVisible) n->series.labelVisible = value.toBool();
					else if (p == CurveProp::kLabelContent) n->series.labelContent = value.toInt();
					else if (p == CurveProp::kLabelText) n->series.labelText = value.toString();
					else if (p == CurveProp::kLabelPrefix) n->series.labelPrefix = value.toString();
					else if (p == CurveProp::kLabelSuffix) n->series.labelSuffix = value.toString();
					else if (p == CurveProp::kLabelPrefixVisible) n->series.labelPrefixVisible = value.toBool();
					else if (p == CurveProp::kLabelSuffixVisible) n->series.labelSuffixVisible = value.toBool();
					else if (p == CurveProp::kAreaFill) n->series.areaFill = value.toBool();
					else if (p == CurveProp::kAreaFillOpacity) n->series.areaFillOpacity = value.toInt();
					else if (p == CurveProp::kContour) n->series.contour = value.toBool();
					else if (p == CurveProp::kContourVar) n->series.contourVar = value.toInt();
					else if (p == CurveProp::kDrawStyle) n->series.drawStyle = value.toInt();
					break;
				case CurveNodeType::XAxis:
				case CurveNodeType::YAxis:
				case CurveNodeType::YAxis2:
				case CurveNodeType::XAxis2:
					if (p == CurveProp::kTitle) n->axis.title = value.toString();
					else if (p == CurveProp::kTitleVisible) n->axis.titleVisible = value.toBool();
					else if (p == CurveProp::kTitleColor) n->axis.titleColor = value.value<QColor>();
					else if (p == CurveProp::kVisible) n->axis.visible = value.toBool();
				else if (p == CurveProp::kScaleMode) n->axis.scaleMode = value.toInt();
				else if (p == CurveProp::kWeightMode) n->axis.weightMode = value.toInt();
				else if (p == CurveProp::kAutoScale) n->axis.autoScale = value.toBool();
				else if (p == CurveProp::kMin) n->axis.min = value.toDouble();
				else if (p == CurveProp::kMax) n->axis.max = value.toDouble();
				else if (p == CurveProp::kTitleFont) n->axis.titleFont = value.value<QFont>();
				else if (p == CurveProp::kLabelFont) n->axis.labelFont = value.value<QFont>();
				else if (p == CurveProp::kUnit) n->axis.unit = value.toString();
				else if (p == CurveProp::kAxisOffset) n->axis.offset = value.toDouble();
				else if (p == CurveProp::kMajorStep) n->axis.majorStep = value.toDouble();
				else if (p == CurveProp::kMinorCount) n->axis.minorCount = value.toInt();
				else if (p == CurveProp::kDbRef) n->axis.dbRef = value.toDouble();
				else if (p == CurveProp::kLabelFormat) n->axis.labelFormat = value.toInt();
				else if (p == CurveProp::kTickGenMode) n->axis.tickGenMode = value.toInt();
				else if (p == CurveProp::kTickCount) n->axis.tickCount = value.toDouble();
				else if (p == CurveProp::kDecadeTickCount) n->axis.decadeTickCount = value.toInt();
				else if (p == CurveProp::kDecadeMinorCount) n->axis.decadeMinorCount = value.toInt();
				else if (p == CurveProp::kPrecision) n->axis.precision = value.toInt();
				else if (p == CurveProp::kCategoryLabelAlign) n->axis.categoryLabelAlign = value.toInt();
				else if (p == CurveProp::kTickValues) n->axis.tickValues = value.value<QVector<double>>();   //列表标注法（QVector<double> 值类型）
				break;
			case CurveNodeType::Legend:
				if (p == CurveProp::kVisible) n->legend.visible = value.toBool();
				else if (p == CurveProp::kAutoPosition) n->legend.autoPosition = value.toBool();
				else if (p == CurveProp::kPosition) n->legend.position = value.toInt();
				else if (p == CurveProp::kPosX) n->legend.posX = value.toDouble();   //画布百分比
				else if (p == CurveProp::kPosY) n->legend.posY = value.toDouble();
				else if (p == CurveProp::kBorderStyle) n->legend.borderStyle = value.toInt();
				else if (p == CurveProp::kLegendBorderWidth) n->legend.borderWidth = value.toDouble();
				else if (p == CurveProp::kBorderColor) n->legend.borderColor = value.value<QColor>();
				else if (p == CurveProp::kLegendFont) n->legend.font = value.value<QFont>();
				else if (p == CurveProp::kTextColorMode) n->legend.textColorMode = value.toInt();
				else if (p == CurveProp::kTextColor) n->legend.textColor = value.value<QColor>();
				else if (p == CurveProp::kBgColor) n->legend.bgColor = value.value<QColor>();
				else if (p == CurveProp::kLeaderPos) n->legend.leaderPos = value.toInt();
				break;
			case CurveNodeType::Baseline:
				if (p == CurveProp::kLabel) n->baseline.label = value.toString();
				else if (p == CurveProp::kOrientation) n->baseline.orientation = value.toInt();
				else if (p == CurveProp::kValue) n->baseline.value = value.toDouble();
				else if (p == CurveProp::kColor) n->baseline.color = value.value<QColor>();
				else if (p == CurveProp::kLineStyle) n->baseline.lineStyle = value.toInt();
				else if (p == CurveProp::kLineWidth) n->baseline.lineWidth = value.toDouble();
				else if (p == CurveProp::kVisible) n->baseline.visible = value.toBool();
				else if (p == CurveProp::kLayer) n->baseline.layer = value.toString();
				else if (p == CurveProp::kLabelFont) n->baseline.labelFont = value.value<QFont>();
				else if (p == CurveProp::kLabelPos) n->baseline.labelPos = value.toInt();
				else if (p == CurveProp::kLink) n->baseline.link = value.toString();
				break;
			case CurveNodeType::Annotation:
				if (p == CurveProp::kText) n->annotation.text = value.toString();
				else if (p == CurveProp::kPosX) n->annotation.posX = value.toDouble();
				else if (p == CurveProp::kPosY) n->annotation.posY = value.toDouble();
				else if (p == CurveProp::kColor) n->annotation.color = value.value<QColor>();
				else if (p == CurveProp::kVisible) n->annotation.visible = value.toBool();
				else if (p == CurveProp::kAlign) n->annotation.align = value.toInt();
				else if (p == CurveProp::kFont) n->annotation.font = value.value<QFont>();
				else if (p == CurveProp::kBorderMode) n->annotation.borderMode = value.toInt();
				else if (p == CurveProp::kBorderColor) n->annotation.borderColor = value.value<QColor>();
				else if (p == CurveProp::kBorderWidth) n->annotation.borderWidth = value.toDouble();
				else if (p == CurveProp::kBorderShow) n->annotation.borderShow = value.toBool();   //R42 二期
				else if (p == CurveProp::kPosZ) n->annotation.posZ = value.toDouble();   //R42 修订（键复用基准面侧同名，分发按类型）
				else if (p == CurveProp::kBgColor) n->annotation.bgColor = value.value<QColor>();
				else if (p == CurveProp::kAutoPos) n->annotation.autoPos = value.toBool();
				else if (p == CurveProp::kAutoAnchor) n->annotation.autoAnchor = value.toInt();
				else if (p == CurveProp::kAutoOffset) n->annotation.autoOffset = value.toDouble();
				else if (p == CurveProp::kAttachMode) n->annotation.attachMode = value.toInt();
				else if (p == CurveProp::kAttachCurve) n->annotation.attachCurve = value.toString();
				else if (p == CurveProp::kAttachIndex) n->annotation.attachIndex = value.toInt();
				else if (p == CurveProp::kBoxPosX) n->annotation.boxPosX = value.toDouble();   //R46 框位置三键
				else if (p == CurveProp::kBoxPosY) n->annotation.boxPosY = value.toDouble();
				else if (p == CurveProp::kBoxPosZ) n->annotation.boxPosZ = value.toDouble();
				else if (p == CurveProp::kMinimized) n->annotation.minimized = value.toBool();
				else if (p == CurveProp::kMinimizeRadius) n->annotation.minimizeRadius = value.toDouble();
				else if (p == CurveProp::kMinimizeColor) n->annotation.minimizeColor = value.value<QColor>();
				else if (p == CurveProp::kScalarFmtOn) n->annotation.scalarFmtOn = value.toBool();
				else if (p == CurveProp::kScalarFmtMode) n->annotation.scalarFmtMode = value.toInt();
				else if (p == CurveProp::kScalarFmtPrecision) n->annotation.scalarFmtPrecision = value.toInt();
				break;
			case CurveNodeType::DatumPlane3D:
				//三维基准面（R41）：键→字段映射对齐既有分支形态（同值短路等语义在
				//函数头公共段已处理）；数值键越界钳制在此收口（模型永不存脏值）
				if (p == CurveProp::kVisible) n->datumPlane3D.visible = value.toBool();
				else if (p == CurveProp::kColor) n->datumPlane3D.color = value.value<QColor>();
				else if (p == CurveProp::kText) n->datumPlane3D.text = value.toString();
				else if (p == CurveProp::kLabelPos) n->datumPlane3D.labelPos = qBound(0, value.toInt(), 8);
				else if (p == CurveProp::kPosX) n->datumPlane3D.posX = value.toDouble();
				else if (p == CurveProp::kPosY) n->datumPlane3D.posY = value.toDouble();
				else if (p == CurveProp::kDatumType) n->datumPlane3D.type = qBound(0, value.toInt(), 0);
				else if (p == CurveProp::kNormalAxis) n->datumPlane3D.normalAxis = qBound(0, value.toInt(), 2);
				else if (p == CurveProp::kPosZ) n->datumPlane3D.posZ = value.toDouble();
				else if (p == CurveProp::kDatumOpacity) n->datumPlane3D.opacity = qBound(0.0, value.toDouble(), 1.0);
				else if (p == CurveProp::kReverseClip) n->datumPlane3D.reverseClip = value.toBool();
				break;
			default:
				return false;
			}
		}

		//通知视图：树（dataChanged）+ 图表/属性页（nodePropertyChanged）
		QModelIndex idx = indexFromNode(n);
		if (idx.isValid())
			emit dataChanged(idx, idx);
		//复数图 Y 轴子行显示名随宿主图表的显示格式/图表类型联动（nodeDisplayText
		//动态规则）。子行自身无属性变更，不补发 dataChanged 树文本就不刷新
		//（含撤销/重做回切）；kChartType 时 YAxis2 行的增删由骨架协调的结构事件
		//负责，这里只覆盖现存 Y 轴行的文本变化
		if (n->type == CurveNodeType::Chart
			&& (p == CurveProp::kComplexDisplayFormat || p == CurveProp::kChartType))
		{
			for (CurveNode* c : n->children)
			{
				if (c->type != CurveNodeType::YAxis && c->type != CurveNodeType::YAxis2)
					continue;
				const QModelIndex ci = indexFromNode(c);
				if (ci.isValid())
					emit dataChanged(ci, ci);
			}
		}
		emit nodePropertyChanged(id, prop);
		return true;
	}

	//--- 结构变更 ---

	int CurveDocument::allocateNodeId()
	{
		return _nextId++;
	}

	int CurveDocument::nextDefaultNumber(CurveNodeType type)
	{
		switch (type)
		{
		case CurveNodeType::Page:       return ++_pageCount;
		case CurveNodeType::Chart:      return ++_chartCount;
		case CurveNodeType::Curve:      return ++_curveCount;
		case CurveNodeType::Baseline:   return ++_baselineCount;
		case CurveNodeType::Annotation: return ++_annotationCount;
		case CurveNodeType::DatumPlane3D: return ++_datumPlane3DCount;
		default:                        return 0;
		}
	}

	CurveNode* CurveDocument::createPage(int pageId, const QString& name)
	{
		return createNode(CurveNodeType::Page, pageId, _root->id, name);
	}

	CurveNode* CurveDocument::createChart(int chartId, int pageId, const QString& name)
	{
		CurveNode* chart = createNode(CurveNodeType::Chart, chartId, pageId, name);
		if (!chart)
			return nullptr;
	//视口固定骨架按图表类型的声明创建（ChartTypeProfile.fixedChildren，顺序=树显示顺序：
	//曲线区之后是图例，轴/页眉/页脚排尾部；曲线/基准/注释按分区插入到图例之前）。
	//主轴固定名 X1/Y1（HyperGraph 位置编号，与副轴 X2/Y2 同构）
	const ChartTypeProfile& prof = chartTypeProfile(chart->chart.chartType);
	for (const ChartFixedChild& fc : prof.fixedChildren)
	{
		CurveNode* n = createNode(fc.type, allocateNodeId(), chart->id, fc.name);
		//轴默认可见性是模型默认值（profile 注册值；当前全部 true=现状零影响）
		if (n && (n->type == CurveNodeType::XAxis || n->type == CurveNodeType::YAxis
			|| n->type == CurveNodeType::YAxis2 || n->type == CurveNodeType::XAxis2))
			n->axis.visible = fc.visible;
	}
		return chart;
	}

	//创建三维图视口（chartType=5：固定骨架按 profile(5) 仅 Legend）。创建时
	//先置 chartType 再取骨架（createChart 公共路径按节点当前类型查表，表达不了
	//"带类型创建"）；轴初值=axis3d 默认态（X/Y/Z + 0..10 + 刻度6 + 网格开 +
	//自动适配开），范围随数据由视图按自动适配现算并集（R28 入树）。
	//**信号时序**：createNode 尾部即发 nodeAdded，而 chartType/固定骨架此刻
	//未就绪——视口工厂同步物化会读到默认类型(0=XY)选错视图类（R28 实测缺陷：
	//三维图开成 2D 画布）。故抑制期间组装，骨架完成后补发根节点信号
	CurveNode* CurveDocument::create3DChart(int chartId, int pageId, const QString& name)
	{
		_suppressNodeAdded = true;   //组装期抑制（首个 createNode 的 nodeAdded 起即抑制）
		CurveNode* chart = createNode(CurveNodeType::Chart, chartId, pageId, name);
		if (!chart)
		{
			_suppressNodeAdded = false;
			return nullptr;
		}
		chart->chart.chartType = 5;
		const ChartTypeProfile& prof = chartTypeProfile(chart->chart.chartType);
		for (const ChartFixedChild& fc : prof.fixedChildren)
		{
			createNode(fc.type, allocateNodeId(), chart->id, fc.name);
		}
		_suppressNodeAdded = false;
		emit nodeAdded(chart->id);   //骨架/类型就绪后补发（工厂据此选 m3DChartView）
		return chart;
	}

	//创建复数曲线（曲线 + x/"ym"/"yp" 三个数据壳，复数图目标导入/定义曲线用；
	//两路数据经控制器 setComplexSeriesData 直写），nodeId 需先经 allocateNodeId 取得
	CurveNode* CurveDocument::createComplexCurve(int chartId, int nodeId, const QString& name)
	{
		CurveNode* n = createNode(CurveNodeType::Curve, nodeId, chartId, name);
		if (!n)
			return nullptr;
		//复数曲线三壳：x / ym / yp（设计图 Complex Plot；y 壳更名 ym 后追加 yp 壳。
		//壳名即树显示名，ChartTypeProfile.displayOverride 不再按父名令牌判侧别）
		for (CurveNode* c : n->children)
		{
			if (c->type == CurveNodeType::CurveYData)
			{
				c->name = QStringLiteral("ym");
				//壳行已在 createNode 内插入（endInsertRows 完成），更名补发 dataChanged
				const QModelIndex idx = indexFromNode(c);
				if (idx.isValid())
					emit dataChanged(idx, idx);
				break;
			}
		}
		createNode(CurveNodeType::CurveY2Data, allocateNodeId(), n->id, QStringLiteral("yp"));
		return n;
	}

	//创建柱状曲线（曲线 + 仅 "y" 一个数据壳：柱状图无横坐标数据语义，横轴为
	//类目轴），nodeId 需先经 allocateNodeId 取得
	CurveNode* CurveDocument::createBarCurve(int chartId, int nodeId, const QString& name)
	{
		CurveNode* n = createNode(CurveNodeType::Curve, nodeId, chartId, name, false);
		if (!n)
			return nullptr;
		createNode(CurveNodeType::CurveYData, allocateNodeId(), n->id, QStringLiteral("y"));
		return n;
	}

	//创建三维曲线（曲线 + "x"/"y"/"z" 三个轴壳：壳属性经重定向读写所属图表
	//axis3d 轴组，数据经控制器 set3DSeriesData 直写），nodeId 需先经
	//allocateNodeId 取得
	CurveNode* CurveDocument::create3DCurve(int chartId, int nodeId, const QString& name)
	{
		CurveNode* n = createNode(CurveNodeType::Curve, nodeId, chartId, name, false);
		if (!n)
			return nullptr;
		createNode(CurveNodeType::AxisX3DData, allocateNodeId(), n->id, QStringLiteral("x"));
		createNode(CurveNodeType::AxisY3DData, allocateNodeId(), n->id, QStringLiteral("y"));
		createNode(CurveNodeType::AxisZ3DData, allocateNodeId(), n->id, QStringLiteral("z"));
		return n;
	}

	void CurveDocument::setCurveBarData(int curveId, const QVector<double>& xData,
		const QVector<double>& yData, const QStringList& tickLabels)
	{
		CurveNode* n = node(curveId);
		if (!n || n->type != CurveNodeType::Curve)
			return;
		n->series.xData = xData;
		n->series.yData = yData;
		n->series.xTickLabels = tickLabels;
		emit nodePropertyChanged(curveId, QStringLiteral("data"));
	}

	CurveNode* CurveDocument::createNode(CurveNodeType type, int nodeId, int parentNodeId, const QString& name)
	{
		return createNode(type, nodeId, parentNodeId, name, true);
	}

	//createNode 内部变体：autoDataShells=false 时不自动创建 x/y 数据壳
	//（柱状曲线走 createBarCurve 自建单 y 壳），其余行为一致
	CurveNode* CurveDocument::createNode(CurveNodeType type, int nodeId, int parentNodeId,
		const QString& name, bool autoDataShells)
	{
		CurveNode* parent = node(parentNodeId);
		if (!parent || _idMap.contains(nodeId))
			return nullptr;

		CurveNode* n = new CurveNode;
		n->id = nodeId;
		n->type = type;
		n->name = name;
		n->parent = parent;

		QModelIndex parentIndex = indexFromNode(parent);
		int row = childInsertionRow(parent, type);
		beginInsertRows(parentIndex, row, row);
		parent->children.insert(row, n);
		_idMap.insert(nodeId, n);
		endInsertRows();
		//页/视口/曲线的 p/w/c 位置序随同层插入变化：既有兄弟行补发 dataChanged
		if (type == CurveNodeType::Page || type == CurveNodeType::Chart
			|| type == CurveNodeType::Curve)
			refreshNumberedSiblings(parent);

		//默认值按类型初始化；曲线自动创建 x/y 数据固定子节点（设计图03.2，
		//id 每次 redo 重新分配——固定子节点无外部引用，与视口固定子节点同策略）
		if (type == CurveNodeType::Curve)
		{
			n->series.name = name;
			if (autoDataShells)
			{
				createNode(CurveNodeType::CurveXData, allocateNodeId(), n->id, QStringLiteral("x"));
				createNode(CurveNodeType::CurveYData, allocateNodeId(), n->id, QStringLiteral("y"));
			}
		}
		else if (type == CurveNodeType::Baseline)
		{
			//标签初值=基准线名称（设计图14：线上标签创建时即显示"Datum N"，
			//此后独立编辑；重命名不跟随）
			n->baseline.label = name;
		}

		//nodeAdded 抑制期（create3DChart 组装中）：不逐节点发，由调用方在
		//骨架/类型就绪后补发根节点信号（视口工厂据此选对视图类）
		if (!_suppressNodeAdded)
			emit nodeAdded(nodeId);
		return n;
	}

	int CurveDocument::childInsertionRow(const CurveNode* parent, CurveNodeType type) const
	{
		if (!parent)
			return 0;
		if (type == CurveNodeType::YAxis2 || type == CurveNodeType::XAxis2)
		{
			//副轴紧随既有轴节点之后（设计图顺序 ...X1, Y1, X2, Y2, X3...，页眉页脚之前）；
			//同侧副轴按创建顺序追加=渲染堆叠由内到外（首枚占引擎轴，其余为自绘条带）
			int row = -1;
			for (int i = 0; i < parent->children.size(); ++i)
			{
				CurveNodeType t = parent->children.at(i)->type;
				if (t == CurveNodeType::XAxis || t == CurveNodeType::YAxis
					|| t == CurveNodeType::YAxis2 || t == CurveNodeType::XAxis2)
					row = i;
			}
			return row >= 0 ? row + 1 : parent->children.size();
		}
		//分区插入（曲线->图例->基准->注释->轴/页眉页脚）：插到第一个
		//"分区序更大"的子节点之前，同类追加到本分区末尾。
		//分区序按所属图表的类型声明查表（chartZoneRank：zoneOrder 下标，
		//不在表内=尾部区）；parent 非图表后代（建页）时按默认 XY 声明，行为不变
		const int chartId = chartNodeId(parent->id);
		const ChartTypeProfile& prof = chartTypeProfile(
			chartId >= 0 ? node(chartId)->chart.chartType : 0);
		const int rank = chartZoneRank(prof, type);
		for (int i = 0; i < parent->children.size(); ++i)
		{
			if (chartZoneRank(prof, parent->children.at(i)->type) > rank)
				return i;
		}
		return parent->children.size();
	}

	void CurveDocument::removeNodeTree(int nodeId)
	{
		CurveNode* n = node(nodeId);
		if (!n || n == _root)
			return;

		emit nodeAboutToBeRemoved(nodeId);

		CurveNode* parent = n->parent;
		const CurveNodeType removedType = n->type;
		int row = parent->children.indexOf(n);
		QModelIndex parentIndex = indexFromNode(parent);
		beginRemoveRows(parentIndex, row, row);
		parent->children.removeAt(row);
		unregisterNodeTree(n);
		endRemoveRows();
		//页/视口/曲线删除后兄弟的 p/w/c 位置序重排，补发 dataChanged（先于
		//freeNodeTree，与撤销恢复 insertNodeTree 的刷新对称）
		if (removedType == CurveNodeType::Page || removedType == CurveNodeType::Chart
			|| removedType == CurveNodeType::Curve)
			refreshNumberedSiblings(parent);

		freeNodeTree(n);
		emit nodeRemoved(nodeId);
	}

	bool CurveDocument::moveChildNode(int chartId, int nodeId, int toRow)
	{
		CurveNode* chart = node(chartId);
		CurveNode* n = node(nodeId);
		if (!chart || !n || n->parent != chart)
			return false;
		const int from = chart->children.indexOf(n);
		const int count = chart->children.size();
		//原位/下移一格均为无操作（同层下移一格行序不变）
		if (from < 0 || toRow < 0 || toRow >= count || toRow == from || toRow == from + 1)
			return false;
		const QModelIndex parentIdx = indexFromNode(chart);
		//行移动用 remove+insert 两组行信号而非 beginMoveRows：QSortFilterProxyModel
		//不处理 rowsAboutToBeMoved（已知 Qt 局限），代理映射会残留失效索引致树操作
		//崩溃。节点对象全程自持不释放（不触碰 _idMap，也不发 nodeAdded/Removed——
		//节点并未增删，仅显示行移动）
		beginRemoveRows(parentIdx, from, from);
		chart->children.removeAt(from);
		endRemoveRows();
		beginInsertRows(parentIdx, toRow, toRow);
		chart->children.insert(toRow, n);
		endInsertRows();
		return true;
	}

	CurveNode* CurveDocument::cloneNodeTree(const CurveNode* src)
	{
		if (!src)
			return nullptr;
		CurveNode* copy = new CurveNode(*src);   //浅拷贝全部字段
		copy->parent = nullptr;
		copy->children.clear();
		for (const CurveNode* c : src->children)
		{
			CurveNode* childCopy = cloneNodeTree(c);
			childCopy->parent = copy;
			copy->children.append(childCopy);
		}
		return copy;
	}

	CurveNode* CurveDocument::insertNodeTree(CurveNode* subtree, int parentNodeId)
	{
		CurveNode* parent = node(parentNodeId);
		if (!parent || !subtree)
			return nullptr;

		QModelIndex parentIndex = indexFromNode(parent);
		//按分区序插回（撤销删除时保持设计图02的树顺序，不落到末尾）
		int row = childInsertionRow(parent, subtree->type);
		beginInsertRows(parentIndex, row, row);
		subtree->parent = parent;
		parent->children.insert(row, subtree);
		registerNodeTree(subtree);
		endInsertRows();
		//撤销恢复同样触发兄弟 p/w/c 序重排（与 removeNodeTree 对称）
		if (subtree->type == CurveNodeType::Page || subtree->type == CurveNodeType::Chart
			|| subtree->type == CurveNodeType::Curve)
			refreshNumberedSiblings(parent);

		emit nodeAdded(subtree->id);
		return subtree;
	}

	void CurveDocument::reassignNodeTreeIds(CurveNode* node)
	{
		if (!node)
			return;
		node->id = allocateNodeId();
		for (CurveNode* c : node->children)
			reassignNodeTreeIds(c);
	}

	void CurveDocument::freeNodeTree(CurveNode* node)
	{
		if (!node)
			return;
		for (CurveNode* c : node->children)
			freeNodeTree(c);
		delete node;
	}

	void CurveDocument::registerNodeTree(CurveNode* node)
	{
		if (!node)
			return;
		_idMap.insert(node->id, node);
		for (CurveNode* c : node->children)
			registerNodeTree(c);
	}

	void CurveDocument::unregisterNodeTree(CurveNode* node)
	{
		if (!node)
			return;
		_idMap.remove(node->id);
		for (CurveNode* c : node->children)
			unregisterNodeTree(c);
	}

	CurveDocument::CurveDocCounters CurveDocument::snapshotCounters() const
	{
		CurveDocCounters c;
		c.nextId = _nextId;
		c.pageCount = _pageCount;
		c.chartCount = _chartCount;
		c.curveCount = _curveCount;
		c.baselineCount = _baselineCount;
		c.annotationCount = _annotationCount;
		c.datumPlane3DCount = _datumPlane3DCount;
		return c;
	}

	void CurveDocument::restoreCounters(const CurveDocCounters& counters)
	{
		_nextId = qMax(_nextId, counters.nextId);
		_pageCount = qMax(_pageCount, counters.pageCount);
		_chartCount = qMax(_chartCount, counters.chartCount);
		_curveCount = qMax(_curveCount, counters.curveCount);
		_baselineCount = qMax(_baselineCount, counters.baselineCount);
		_annotationCount = qMax(_annotationCount, counters.annotationCount);
		_datumPlane3DCount = qMax(_datumPlane3DCount, counters.datumPlane3DCount);
	}
}
