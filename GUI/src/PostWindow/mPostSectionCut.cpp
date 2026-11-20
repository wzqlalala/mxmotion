#include "mPostSectionCut.h"
#include "mFbdColorPicker.h"
#include "mPostObjectEditors.h"

#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QHeaderView>
#include <QTableWidget>
#include <QListWidget>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QRegExp>
#include <QScreen>
#include <QGuiApplication>
#include <QCursor>
#include <QtMath>
#include <algorithm>
#include <functional>

#include "mGroupBox.h"
#include "mChoiceGroup.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mPhysicalFieldEnum.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/PostSelectionCalculator.h"
#include "PickCommonFunction.h"   // MechPreData::ClearPostWidgetMeshPick（退出拾取清高亮）
#include "mPostGUIData.h"
#include "mPostModelData.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;

namespace
{
	//令牌（--std-* 标准档；出处 V1.3 原型 CSS 变量定义块）
	constexpr int kControl = 26;		//--std-control 控件外高
	constexpr int kInner = 24;			//连体内控件高（--std-control - 2）
	constexpr int kActionW = 40;		//--std-manager-action-width 应用钮/色钮宽
	constexpr int kChoiceGap = 4;		//--std-choice-gap
	constexpr int kGroupInset = 7;		//--std-group-inset
	constexpr int kRadius = 4;			//--std-radius（V1.3：2→4）
	constexpr int kGap = 6;				//--std-gap
	constexpr int kColumnGap = 8;		//--std-column-gap

	const char* kTextColor = "#303313";		//--std-text / --std-label（V1.3 统一主文本色）
	const char* kMuted = "#788391";			//--std-muted
	const char* kDisabledFg = "#A6AFBB";		//--std-disabled
	const char* kCtrlBorder = "#DCE2E9";		//--std-border
	const char* kHoverBg = "#F0F7FF";			//--std-hover
	const char* kSelectedBg = "#E8F3FF";		//--std-selected
	const char* kPrimary = "#1890FF";			//--std-primary / --std-focus
	const char* kError = "#BF3434";			//--std-error
	const char* kPanelBg = "#F5F7FA";			//--std-panel-color（禁用底）

	//2x 采样绘制线条图标（近似原型 inline SVG 语义）
	QIcon makeIcon(int px, const QColor& color, const std::function<void(QPainter&)>& paint)
	{
		QPixmap pm(px * 2, px * 2);
		pm.setDevicePixelRatio(2.0);
		pm.fill(Qt::transparent);
		QPainter p(&pm);
		p.setRenderHint(QPainter::Antialiasing);
		QPen pen(color, 1.4);
		pen.setCapStyle(Qt::RoundCap);
		pen.setJoinStyle(Qt::RoundJoin);
		p.setPen(pen);
		paint(p);
		return QIcon(pm);
	}

	//十字准星（原型 pickIcon 16 viewBox：四角括号+十字）
	QIcon crosshairIcon(const QColor& color)
	{
		return makeIcon(20, color, [](QPainter& p)
			{
				p.scale(20.0 / 16.0, 20.0 / 16.0);
				p.drawLine(QPointF(6, 2), QPointF(2, 2));
				p.drawLine(QPointF(2, 2), QPointF(2, 6));
				p.drawLine(QPointF(10, 2), QPointF(14, 2));
				p.drawLine(QPointF(14, 2), QPointF(14, 6));
				p.drawLine(QPointF(2, 10), QPointF(2, 14));
				p.drawLine(QPointF(2, 14), QPointF(6, 14));
				p.drawLine(QPointF(14, 10), QPointF(14, 14));
				p.drawLine(QPointF(14, 14), QPointF(10, 14));
				p.drawLine(QPointF(8, 4), QPointF(8, 12));
				p.drawLine(QPointF(4, 8), QPointF(12, 8));
			});
	}

	//反向双箭头（原型 reverse 16 viewBox "M3 11 11 3M7 3h4v4M13 5 5 13M9 13H5V9"）
	QIcon reverseIcon(const QColor& color)
	{
		return makeIcon(16, color, [](QPainter& p)
			{
				p.drawLine(QPointF(3, 11), QPointF(11, 3));
				p.drawLine(QPointF(7, 3), QPointF(11, 3));
				p.drawLine(QPointF(11, 3), QPointF(11, 7));
				p.drawLine(QPointF(13, 5), QPointF(5, 13));
				p.drawLine(QPointF(9, 13), QPointF(5, 13));
				p.drawLine(QPointF(5, 13), QPointF(5, 9));
			});
	}

	//数字输入+单位后缀（原型 .section-cut-offset>span：muted 11px 绝对定位右内侧浮）
	class SuffixEdit : public QLineEdit
	{
	public:
		SuffixEdit(const QString& suffix, QWidget* parent = nullptr) : QLineEdit(parent)
		{
			setFixedHeight(kControl);
			setStyleSheet(QString("QLineEdit{background:#FFFFFF;border:1px solid %1;"
				"border-radius:%2px;padding:0 %3px;color:%4;}")
				.arg(kCtrlBorder).arg(kRadius).arg(kGroupInset).arg(kTextColor));
			_suffix = new QLabel(suffix, this);
			_suffix->setStyleSheet(QString("color:%1;font-size:11px;background:transparent;").arg(kMuted));
			_suffix->setAttribute(Qt::WA_TransparentForMouseEvents);
			setTextMargins(kGroupInset, 0, 30, 0);
		}

	protected:
		void resizeEvent(QResizeEvent* event) override
		{
			QLineEdit::resizeEvent(event);
			_suffix->adjustSize();
			_suffix->move(width() - _suffix->width() - 5, (height() - _suffix->height()) / 2);
		}

	private:
		QLabel* _suffix = nullptr;
	};

	//V1.3 蓝条拾取入口钮（lcs-point-pick：#e7f3ff 底 + #1685ff 描边 + 左 4px 主色竖条，
	//原型 ::before 蓝条 Qt 无伪元素 → paintEvent 自绘；armed(checked) 描 focus 色；
	//禁用整钮灰——与曲线提取/标签面板同族形态，按 lcs-workbench 取蓝底变体）
	class PointPickButton : public QPushButton
	{
	public:
		explicit PointPickButton(QWidget* parent = nullptr) : QPushButton(parent)
		{
			setCursor(Qt::PointingHandCursor);
			setCheckable(true);
			setStyleSheet(
				QString("QPushButton{background:#E7F3FF;border:1px solid #1685FF;border-radius:%1px;"
					"color:%2;text-align:left;padding-left:12px;padding-right:%3px;}"
					"QPushButton:checked{border-color:%4;}"
					"QPushButton:disabled{background:#F5F7FA;border-color:%5;color:%6;}")
				.arg(kRadius).arg(kTextColor).arg(kChoiceGap + 20).arg(kPrimary, kCtrlBorder, kDisabledFg));
		}

	protected:
		void paintEvent(QPaintEvent* event) override
		{
			QPushButton::paintEvent(event);
			QPainter p(this);
			p.setRenderHint(QPainter::Antialiasing);
			p.setPen(Qt::NoPen);
			p.setBrush(QColor(isEnabled() ? kPrimary : kDisabledFg));
			p.drawRoundedRect(QRectF(4, 5, 3, height() - 10), 1.5, 1.5);
		}
	};

	//连体增减步长（原型 .section-cut-step：容器 1px 边框圆角 4，内部 −/输入/＋ 直角、
	//竖线分隔、1:2:1；文字居中）——原型 isosurface-adjustment 同族
	QWidget* buildStepField(QPushButton*& dec, QLineEdit*& edit, QPushButton*& inc)
	{
		QWidget* field = new QWidget;
		field->setObjectName("sectionCutStepField");
		field->setFixedHeight(kControl);
		QHBoxLayout* lay = new QHBoxLayout(field);
		lay->setContentsMargins(0, 0, 0, 0);
		lay->setSpacing(0);

		const char* btnQss =
			"QPushButton{border:0;border-radius:0;background:transparent;color:#303313;font-size:14px;}"
			"QPushButton:hover{background:#F0F7FF;}"
			"QPushButton:disabled{color:#A6AFBB;}";
		const char* editQss =
			"QLineEdit{border:0;border-radius:0;padding:0 4px;color:#303313;background:transparent;}";
		const char* sepQss = "border-left:1px solid #DCE2E9;border-right:1px solid #DCE2E9;";

		dec = new QPushButton(QStringLiteral("−"), field);
		dec->setObjectName("sectionCutDecrease");
		dec->setStyleSheet(QString(btnQss) + sepQss);
		dec->setFixedHeight(kInner);
		edit = new QLineEdit(field);
		edit->setObjectName("sectionCutStep");
		edit->setStyleSheet(editQss);
		edit->setFixedHeight(kInner);
		edit->setAlignment(Qt::AlignCenter);
		inc = new QPushButton(QStringLiteral("＋"), field);
		inc->setObjectName("sectionCutIncrease");
		inc->setStyleSheet(QString(btnQss) + sepQss);
		inc->setFixedHeight(kInner);

		lay->addWidget(dec, 1);
		lay->addWidget(edit, 2);
		lay->addWidget(inc, 1);

		field->setStyleSheet(QString("QWidget#sectionCutStepField{background:#FFFFFF;"
			"border:1px solid %1;border-radius:%2px;}").arg(kCtrlBorder).arg(kRadius));
		return field;
	}

	//拾取行辅助按钮图标（V1.3 r-pick-selected-icon：16 viewBox 描边 1.5；两钮恒禁用，
	//按 disabled 口径 #A6AFBB 绘制）：查看=圆圈+右向尖号；清空=垃圾桶
	QIcon pickAuxIcon(int kind)
	{
		return makeIcon(20, QColor(kDisabledFg), [kind](QPainter& p)
			{
				p.scale(20.0 / 16.0, 20.0 / 16.0);
				if (kind == 0)
				{
					p.drawEllipse(QPointF(8, 8), 6.0, 6.0);
					p.drawLine(QPointF(6.5, 5), QPointF(9.5, 8));
					p.drawLine(QPointF(9.5, 8), QPointF(6.5, 11));
				}
				else
				{
					p.drawLine(QPointF(3, 4.5), QPointF(13, 4.5));
					p.drawLine(QPointF(6, 4), QPointF(6, 2.5));
					p.drawLine(QPointF(6, 2.5), QPointF(10, 2.5));
					p.drawLine(QPointF(10, 2.5), QPointF(10, 4));
					p.drawLine(QPointF(4, 5), QPointF(4.5, 13));
					p.drawLine(QPointF(4.5, 13), QPointF(11.5, 13));
					p.drawLine(QPointF(11.5, 13), QPointF(12, 5));
					p.drawLine(QPointF(6.5, 6.5), QPointF(6.5, 11));
					p.drawLine(QPointF(9.5, 6.5), QPointF(9.5, 11));
				}
			});
	}

	//范围拾取连体钮框（原型 .section-cut-scope 内的 post-general-pick：入口钮占满 +
	//查看/清空两钮叠于框内右侧 26×24，resizeEvent 手动布局——曲线提取 PickEntryRow
	//连体变体：无圆角、无蓝条、左侧竖线由外层 QSS 提供）
	class ScopePickHost : public QWidget
	{
	public:
		explicit ScopePickHost(QWidget* parent = nullptr) : QWidget(parent)
		{
			setObjectName("sectionCutScopePickHost");
			setFixedHeight(kInner);
			entry = new QPushButton(this);
			entry->setCheckable(true);
			entry->setCursor(Qt::PointingHandCursor);
			inspect = new QPushButton(this);
			inspect->setIcon(pickAuxIcon(0));
			clear = new QPushButton(this);
			clear->setIcon(pickAuxIcon(1));
			for (QPushButton* btn : { inspect, clear })
			{
				btn->setEnabled(false);
				btn->setFixedSize(kControl, kInner);
				btn->setCursor(Qt::PointingHandCursor);
				btn->setStyleSheet(
					"QPushButton{background:transparent;border:0;border-radius:0;}"
					"QPushButton:hover{background:#F0F7FF;}");
			}
			layoutChildren();
		}

		QPushButton* entry = nullptr;
		QPushButton* inspect = nullptr;
		QPushButton* clear = nullptr;

	protected:
		void resizeEvent(QResizeEvent* event) override
		{
			QWidget::resizeEvent(event);
			layoutChildren();
		}

	private:
		void layoutChildren()
		{
			entry->setGeometry(0, 0, width(), kInner);
			inspect->setGeometry(width() - 2 * kControl - 1, 0, kControl, kInner);
			clear->setGeometry(width() - kControl - 1, 0, kControl, kInner);
		}
	};
}

namespace PostWindow
{
    /******** detail 纯逻辑实现（P1 抽取；面板非限定调用点经 using 直用） ********/
    using detail::parsePoint;
    using detail::finiteText;

    QList<double> detail::parsePoint(const QString& text)
    {
        //原型 parsePoint：中英文逗号或空白分隔，三个有限数
        QString raw = text.trimmed();
        QStringList parts;
        if (raw.contains(QLatin1Char(',')) || raw.contains(QChar(0xFF0C)))
        {
            parts = raw.split(QRegExp("[,，]"), QString::SkipEmptyParts);
            for (QString& part : parts) part = part.trimmed();
            parts.removeAll(QString());
        }
        else
        {
            parts = raw.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        }
        if (parts.size() != 3)
            return {};
        QList<double> values;
        for (const QString& part : parts)
        {
            bool ok = false;
            const double value = part.toDouble(&ok);
            if (!ok || !qIsFinite(value))
                return {};
            values.append(value);
        }
        return values;
    }

    bool detail::finiteText(const QString& text)
    {
        if (text.trimmed().isEmpty())
            return false;
        bool ok = false;
        const double value = text.trimmed().toDouble(&ok);
        return ok && qIsFinite(value);
    }

    detail::NormalIssue detail::normalIssue(const SectionCutDraft& d)
    {
        //原型 createLoadGeneralDirection validate：仅向量/起点-终点产生文案
        if (d.normalMode == 5)
        {
            const QList<double> v = parsePoint(d.vectorText);
            if (v.isEmpty())
                return NormalIssue::InvalidComponents;
            if (v.at(0) == 0 && v.at(1) == 0 && v.at(2) == 0)
                return NormalIssue::ZeroDirection;
        }
        else if (d.normalMode == 3)
        {
            const QList<double> a = parsePoint(d.startText);
            const QList<double> b = parsePoint(d.endText);
            if (a.isEmpty() || b.isEmpty())
                return NormalIssue::InvalidComponents;
            if (b.at(0) == a.at(0) && b.at(1) == a.at(1) && b.at(2) == a.at(2))
                return NormalIssue::ZeroDirection;
        }
        return NormalIssue::None;
    }

    detail::GeomIssue detail::geometryIssue(const SectionCutDraft& d)
    {
        //任一点有未应用编辑：先应用（原型 dirty 检查）
        if (d.basePoint.dirty || d.point1.dirty || d.point2.dirty || d.point3.dirty)
            return GeomIssue::PendingPoints;

        if (d.definition == 0)
        {
            if (parsePoint(d.basePoint.text).isEmpty())
                return GeomIssue::InvalidBase;
            //法向有效性：轴模式恒有效；选择对象无可用对象；向量/起点-终点按 L13 校验
            if (d.normalMode == 4)
                return GeomIssue::NoDirectionObject;
            if (normalIssue(d) != NormalIssue::None)
                return GeomIssue::InvalidNormal;
            return GeomIssue::None;
        }

        //三点平面
        QList<QList<double>> pts;
        for (const QString& text : { d.point1.text, d.point2.text, d.point3.text })
        {
            const QList<double> p = parsePoint(text);
            if (p.isEmpty())
                return GeomIssue::InvalidThreePoints;
            pts.append(p);
        }
        const QList<double>& a = pts.at(0);
        const QList<double>& b = pts.at(1);
        const QList<double>& c = pts.at(2);
        double u[3], v[3];
        for (int i = 0; i < 3; ++i)
        {
            u[i] = b.at(i) - a.at(i);
            v[i] = c.at(i) - a.at(i);
            if (!qIsFinite(u[i]) || !qIsFinite(v[i]))
                return GeomIssue::PointsOutOfRange;
        }
        const double us = qMax(qMax(qAbs(u[0]), qAbs(u[1])), qAbs(u[2]));
        const double vs = qMax(qMax(qAbs(v[0]), qAbs(v[1])), qAbs(v[2]));
        if (us == 0.0 || vs == 0.0)
            return GeomIssue::CollinearPoints;
        double x[3], y[3];
        for (int i = 0; i < 3; ++i)
        {
            x[i] = u[i] / us;
            y[i] = v[i] / vs;
        }
        const double cross[3] = {
            x[1] * y[2] - x[2] * y[1],
            x[2] * y[0] - x[0] * y[2],
            x[0] * y[1] - x[1] * y[0]
        };
        if (qSqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]) <= 1e-12)
            return GeomIssue::CollinearPoints;
        return GeomIssue::None;
    }

    QString detail::sectionCutSignature(const SectionCutDraft& d)
    {
        //原型 normalize+JSON：全部草稿字段的规范化串（含未应用点编辑标记与拾取暂存）
        auto point = [](const SectionCutPointDraft& p) { return p.text + QChar(p.dirty ? 1 : 0); };
        return d.name + QChar(1) + QString::number(d.scope) + QChar(1)
            + d.scopeParts.join(QChar(2)) + QChar(1) + d.scopeElements.join(QChar(2)) + QChar(1)
            + QString::number(d.definition) + QChar(1) + QString::number(d.normalMode) + QChar(1)
            + QString::number(d.reversed ? 1 : 0) + QChar(1)
            + d.vectorText + QChar(1) + d.startText + QChar(1) + d.endText + QChar(1)
            + point(d.basePoint) + QChar(1) + point(d.point1) + QChar(1)
            + point(d.point2) + QChar(1) + point(d.point3) + QChar(1)
            + QString::number(d.configuration) + QChar(1) + QString::number(d.side) + QChar(1)
            + d.offset + QChar(1) + d.step + QChar(1)
            + QString::number(d.styleMode) + QChar(1) + d.bandwidth + QChar(1)
            + QString::number(d.fill ? 1 : 0) + QChar(1) + QString::number(d.colorMode) + QChar(1)
            + d.color.name() + QChar(1)
            + QString::number(d.crop ? 1 : 0) + QChar(1) + QString::number(d.ghost ? 1 : 0) + QChar(1)
            + QString::number(d.showPlane ? 1 : 0) + QChar(1) + QString::number(d.showNormal ? 1 : 0)
            + QChar(1) + QString::number(d.showMesh ? 1 : 0) + QChar(1) + QString::number(d.showEdges ? 1 : 0);
    }

    bool detail::sectionCutNormal(const SectionCutDraft& d, QVector3D& normal)
    {
        if (d.definition == 1)
        {
            //三点平面：(P2−P1)×(P3−P1) 归一
            const QList<double> p1 = parsePoint(d.point1.text);
            const QList<double> p2 = parsePoint(d.point2.text);
            const QList<double> p3 = parsePoint(d.point3.text);
            if (p1.isEmpty() || p2.isEmpty() || p3.isEmpty())
                return false;
            const QVector3D u(p2.at(0) - p1.at(0), p2.at(1) - p1.at(1), p2.at(2) - p1.at(2));
            const QVector3D v(p3.at(0) - p1.at(0), p3.at(1) - p1.at(1), p3.at(2) - p1.at(2));
            const QVector3D n = QVector3D::crossProduct(u, v);
            const float len = n.length();
            if (!qIsFinite(len) || len <= 0.0f)
                return false;
            normal = n / len;
            return true;
        }

        switch (d.normalMode)
        {
        case 0: normal = QVector3D(1, 0, 0); return true;
        case 1: normal = QVector3D(0, 1, 0); return true;
        case 2: normal = QVector3D(0, 0, 1); return true;
        case 5:
        {
            const QList<double> v = parsePoint(d.vectorText);
            if (v.isEmpty())
                return false;
            const QVector3D n(v.at(0), v.at(1), v.at(2));
            const float len = n.length();
            if (!qIsFinite(len) || len <= 0.0f)
                return false;
            normal = n / len;
            return true;
        }
        case 3:
        {
            const QList<double> a = parsePoint(d.startText);
            const QList<double> b = parsePoint(d.endText);
            if (a.isEmpty() || b.isEmpty())
                return false;
            const QVector3D n(b.at(0) - a.at(0), b.at(1) - a.at(1), b.at(2) - a.at(2));
            const float len = n.length();
            if (!qIsFinite(len) || len <= 0.0f)
                return false;
            normal = n / len;
            return true;
        }
        default:
            return false;   //选择对象：暂无数据源（引擎接入遗留项）
        }
    }

    bool detail::sectionCutBasePoint(const SectionCutDraft& d, QVector3D& base)
    {
        const QList<double> p = parsePoint(d.definition == 1 ? d.point1.text : d.basePoint.text);
        if (p.isEmpty())
            return false;
        base = QVector3D(p.at(0), p.at(1), p.at(2));
        return true;
    }

    QVector3D detail::sectionCutVertex(const SectionCutDraft& d, const QVector3D& normal,
        const QVector3D& deformShift)
    {
        QVector3D base;
        if (!sectionCutBasePoint(d, base))
            return QVector3D();
        const double offset = d.offset.trimmed().toDouble();
        return base + normal * static_cast<float>(offset) + deformShift;
    }

    int detail::effectiveKeepSide(int side, bool reversed)
    {
        if (side < 0 || side > 2)
            side = 2;
        if (reversed && side != 1)
            side = 2 - side;    //两侧不受反向影响
        return side;
    }

    bool detail::effectiveClipModel(bool crop, int side)
    {
        //裁剪单元关或保留两侧=不参与模型裁剪（E2 零方程占位，仅截面封盖/提示面）
        return crop && side != 1;
    }

    QPair<QVector3D, QVector3D> detail::bandVertices(const SectionCutDraft& d, const QVector3D& normal,
        const QVector3D& deformShift)
    {
        QVector3D base;
        if (!sectionCutBasePoint(d, base))
            return qMakePair(QVector3D(), QVector3D());
        const double offset = d.offset.trimmed().toDouble();
        const double half = d.bandwidth.trimmed().toDouble() / 2.0;
        const QVector3D origin = base + normal * static_cast<float>(offset) + deformShift;
        return qMakePair(origin - normal * static_cast<float>(half),
            origin + normal * static_cast<float>(half));
    }

    QVector3D detail::nearestNodeDisplacement(const QVector3D& base,
        const QHash<int, QVector3D>& nodePositions, const QHash<int, QVector3D>& displacements)
    {
        if (nodePositions.isEmpty())
            return QVector3D();
        int bestId = 0;
        float bestDist = -1.0f;
        for (auto it = nodePositions.cbegin(); it != nodePositions.cend(); ++it)
        {
            const float dist = (it.value() - base).lengthSquared();
            if (bestDist < 0.0f || dist < bestDist)
            {
                bestDist = dist;
                bestId = it.key();
            }
        }
        return displacements.value(bestId);
    }

    /******** 渲染三入口（向量图同款：参数存视口 guidata，切帧/重绘跟随重提交） ********/

    namespace
    {
        //提交单个裁剪平面（旧切面图 mPostSection 三步序列：建面→N|V|R 落位→提示面显隐；
        //保留负向侧经 Mask_ReverseNormal 翻转平面方程；E1 统一颜色/E2 不裁剪随掩码下发）
        int submitCuttingPlane(const std::shared_ptr<mxr::REBUILD_PostRender>& postRender,
            const QVector3D& normal, const QVector3D& vertex, float maxR,
            bool keepNegative, bool showPlane,
            const QColor* solidColor = nullptr, bool clipModel = true)
        {
            mxr::CuttingPlaneParams create;
            create.normal = normal;
            create.vertex = vertex;
            create.clipModel = clipModel;   // E2：addObject 即按此登记真实/零方程
            const int id = postRender->addObject(create);
            if (id < 0)
                return -1;
            mxr::CuttingPlaneParams data;
            data.normal = normal;
            data.vertex = vertex;
            data.maxR = maxR;
            data.updateMask = mxr::CuttingPlaneParams::Mask_Normal | mxr::CuttingPlaneParams::Mask_Vertex
                | mxr::CuttingPlaneParams::Mask_MaxR;
            if (keepNegative)
            {
                data.reverseNormal = true;
                data.updateMask |= mxr::CuttingPlaneParams::Mask_ReverseNormal;
            }
            postRender->updateObject(id, data);
            //E1：统一颜色填充（默认按结果值着色不下发=引擎默认关）
            if (solidColor != nullptr)
            {
                mxr::CuttingPlaneParams solid;
                solid.useSolidColor = true;
                solid.solidColor[0] = solidColor->redF();
                solid.solidColor[1] = solidColor->greenF();
                solid.solidColor[2] = solidColor->blueF();
                solid.updateMask = mxr::CuttingPlaneParams::Mask_SolidColor;
                postRender->updateObject(id, solid);
            }
            mxr::CuttingPlaneParams hint;
            hint.isShow = showPlane;
            hint.updateMask = mxr::CuttingPlaneParams::Mask_IsShow;
            postRender->updateObject(id, hint);
            return id;
        }

        //随变形平移近似：基点就近节点的当前帧位移（原始坐标数组直扫，数组下标=连续节点id，
        //与位移哈希键同域；detail::nearestNodeDisplacement 的免建哈希大批量变体）
        QVector3D sectionCutDeformShift(const PostData::mPostModelData* modeldata,
            const QHash<int, QVector3D>& displacements, const QVector3D& base)
        {
            const PostData::postnodedata& nd = modeldata->_nodedata;
            if (nd.nodeamount <= 0 || (nd.isDle ? nd.nodeposD == nullptr : nd.nodeposF == nullptr))
                return QVector3D();
            int best = -1;
            double bestD2 = -1.0;
            for (int i = 0; i < nd.nodeamount; ++i)
            {
                double x, y, z;
                if (nd.isDle)
                {
                    x = nd.nodeposD[i * 3];
                    y = nd.nodeposD[i * 3 + 1];
                    z = nd.nodeposD[i * 3 + 2];
                }
                else
                {
                    x = nd.nodeposF[i * 3];
                    y = nd.nodeposF[i * 3 + 1];
                    z = nd.nodeposF[i * 3 + 2];
                }
                const double dx = x - base.x();
                const double dy = y - base.y();
                const double dz = z - base.z();
                const double d2 = dx * dx + dy * dy + dz * dz;
                if (best < 0 || d2 < bestD2)
                {
                    bestD2 = d2;
                    best = i;
                }
            }
            return best >= 0 ? displacements.value(best) : QVector3D();
        }
    }

    bool applySectionCuts(REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata)
    {
        if (postRend == nullptr || guidata == nullptr) return false;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return false;

        //先按台账 id 精删旧对象（与旧切面图共用 CuttingPlane 家族，禁全家删）
        for (int id : guidata->_sectionCutEngineIds)
            postRender->removeObject(id);
        guidata->_sectionCutEngineIds.clear();

        guidata->_sectionCutActive = !guidata->_sectionCutSpecs.isEmpty();
        if (!guidata->_sectionCutActive)
        {
            postRend->updatePaint();
            return true;
        }

        //数据上下文：结果库+当前帧（随变形位移场读取；无库/无帧时随变形记录退化为固定平面）
        const QString postName = postRend->getPostDataName();
        PostData::mPostDataBase* postDB = getCVM->getModelData()
            ->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;

        bool anyFollowDeform = false;
        for (const MechPreData::SectionCutSpec& spec : guidata->_sectionCutSpecs)
            if (spec._configuration == 1) { anyFollowDeform = true; break; }
        PostData::mPostModelData* modeldata = nullptr;
        QHash<int, QVector3D> displacements;
        if (postDB != nullptr && anyFollowDeform)
        {
            modeldata = postDB->getModelData(guidata->_cloudFrame);
            if (modeldata != nullptr)
                displacements = postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
        }

        //提示面覆盖尺寸（maxR）：当前模型 AABB 对角线长（旧切面图同口径）
        float maxR = 1.0f;
        {
            const mxr::AABB aabb = postRender->getCurrentModelData();
            const double dx = aabb.maxEdge[0] - aabb.minEdge[0];
            const double dy = aabb.maxEdge[1] - aabb.minEdge[1];
            const double dz = aabb.maxEdge[2] - aabb.minEdge[2];
            const double diagonal = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (diagonal > 0 && qIsFinite(diagonal))
                maxR = static_cast<float>(diagonal);
        }

        //GL_CLIP_DISTANCE0..6 共 7 个有效槽位（截面带占 2），超限截断提示
        const int slotBudget = 7;
        int used = 0;
        bool truncated = false;
        for (const MechPreData::SectionCutSpec& spec : guidata->_sectionCutSpecs)
        {
            QVector3D normal = spec._normal;
            if (normal.lengthSquared() < 1e-12f)
                continue;
            normal.normalize();

            QVector3D shift;
            if (spec._configuration == 1 && modeldata != nullptr)
                shift = sectionCutDeformShift(modeldata, displacements, spec._base);
            const QVector3D origin = spec._base + normal * static_cast<float>(spec._offset) + shift;

            const int need = (spec._styleMode == 1) ? 2 : 1;
            if (used + need > slotBudget)
            {
                truncated = true;
                break;
            }

            if (spec._styleMode == 1)
            {
                //截面带=双平面对夹：负侧平面保留其上、正侧平面反向保留其下→仅带内保留；
                //裁剪单元关=双平面零方程占位（仅封盖/提示面，不裁模型）
                const float half = static_cast<float>(spec._bandwidth / 2.0);
                const QColor* solid = (spec._colorMode == 1) ? &spec._color : nullptr;
                int id = submitCuttingPlane(postRender, normal, origin - normal * half, maxR,
                    false, spec._showPlane, solid, spec._crop);
                if (id >= 0)
                    guidata->_sectionCutEngineIds.append(id);
                id = submitCuttingPlane(postRender, normal, origin + normal * half, maxR,
                    true, spec._showPlane, solid, spec._crop);
                if (id >= 0)
                    guidata->_sectionCutEngineIds.append(id);
                used += 2;
            }
            else
            {
                //保留侧：2 正向侧=引擎默认（法向正侧保留）；0 负向侧=翻转方程；
                //1 两侧=E2 不裁剪（零方程占位，只显示截面封盖/提示面）；
                //裁剪单元关=一律不裁（detail::effectiveClipModel 折算）
                const QColor* solid = (spec._colorMode == 1) ? &spec._color : nullptr;
                const int id = submitCuttingPlane(postRender, normal, origin, maxR,
                    spec._side == 0, spec._showPlane, solid,
                    detail::effectiveClipModel(spec._crop, spec._side));
                if (id >= 0)
                    guidata->_sectionCutEngineIds.append(id);
                ++used;
            }
        }

        if (truncated)
            mGlobalSignals::getInstance()->outputMessageSig(2, isChinese
                ? QString::fromUtf8("截面剖切平面数量超出渲染上限（最多 %1 个平面，截面带占 2 个），超出部分已忽略。").arg(slotBudget)
                : QString("Section cut planes exceed the render limit (max %1 planes; a band uses 2). Excess sections are ignored.").arg(slotBudget),
                true);

        postRend->updatePaint();
        return true;
    }

    void removeSectionCuts(REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata)
    {
        if (guidata != nullptr)
            guidata->_sectionCutActive = false;
        if (postRend == nullptr || guidata == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;
        for (int id : guidata->_sectionCutEngineIds)
            postRender->removeObject(id);
        guidata->_sectionCutEngineIds.clear();
        postRend->updatePaint();
    }

    void refreshSectionCutIfActive(REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata)
    {
        if (postRend == nullptr || guidata == nullptr) return;
        if (!guidata->_sectionCutActive) return;
        //重提交即重切：切帧/变量/变形系数变化后截面按当前几何与结果值重建
        applySectionCuts(postRend, guidata);
    }

	mPostSectionCutPanel::mPostSectionCutPanel(QWidget* parent) : mPanelBase(parent)
	{
		_langEn = (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1);
		setObjectName("sectionCutParameters");
		setupUI();
		applyLanguageTexts();

		//语言切换全量刷新
		connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)),
			this, SLOT(slot_changeLanguage(int)));

		//Escape 结束拾取武装态（原型 keydown Escape endPick）
		bindShortcut(QKeySequence(Qt::Key_Escape), [this]() { endPick(); });

		updateSectionCutPanel();
		this->setMinimumWidth(319);	//原型侧栏宽 319（与标签/测量面板同族）
	}

	void mPostSectionCutPanel::setupUI()
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);
		_mainLayout->setSpacing(kGap);
		setStyleSheet(
			"mPostSectionCutPanel{background-color:#FFFFFF;}"
			"QComboBox QAbstractItemView{background-color:#FFFFFF;color:#303313;"
			"selection-background-color:#E8F3FF;selection-color:#303313;}");

		/******** 折叠组：截面管理（V1.3 #sectionCutManagerSection） ********/
		setupManagerSection();

		/******** 编辑器宿主：三折叠组 + 参数操作区（无记录时隐藏） ********/
		setupEditorHost();

		/******** 折叠组：剖切定义 ********/
		_definitionFold = new FoldSection(QString::fromUtf8("剖切定义"), this);

		//截面名称 + 截面颜色（原型 .section-cut-name-color：1fr + 26 色钮 gap 6）
		QWidget* nameColorRow = new QWidget(_definitionFold);
		QHBoxLayout* nameLayout = new QHBoxLayout(nameColorRow);
		nameLayout->setContentsMargins(0, 0, 0, 0);
		nameLayout->setSpacing(kGap);
		_nameEdit = new QLineEdit(nameColorRow);
		_nameEdit->setObjectName("sectionCutName");
		_nameEdit->setFixedHeight(kControl);
		_nameEdit->setStyleSheet(
			QString("QLineEdit{background:#FFFFFF;border:1px solid %1;border-radius:%2px;"
				"padding:0 %3px;color:%4;}")
			.arg(kCtrlBorder).arg(kRadius).arg(kGroupInset).arg(kTextColor));
		_colorSwatch = new QPushButton(nameColorRow);
		_colorSwatch->setObjectName("sectionCutStyleColor");
		_colorSwatch->setFixedSize(kControl, kControl);
		_colorSwatch->setCursor(Qt::PointingHandCursor);
		nameLayout->addWidget(_nameEdit, 1);
		nameLayout->addWidget(_colorSwatch);
		PanelLabel* labelName = new PanelLabel;
		regLabel(labelName, "截面名称", "Name");
		_definitionFold->addRow(labelName, nameColorRow);

		//剖切范围：下拉 + 连体拾取钮（原型 .section-cut-scope：复合外框 1px 直角、
		//内部零间距+竖线分隔、内高 24；拾取钮框内右侧叠 查看/清空 两钮 26×24——
		//原型 CAEPostGeneralPick 装配的 r-pick 条目，禁用占位同曲线提取口径）
		QWidget* scopeField = new QWidget(_definitionFold);
		scopeField->setObjectName("sectionCutScopeField");
		scopeField->setFixedHeight(kControl);
		scopeField->setStyleSheet(QString(
			"QWidget#sectionCutScopeField{border:1px solid %1;border-radius:0;background:#FFFFFF;}")
			.arg(kCtrlBorder));
		QHBoxLayout* scopeLayout = new QHBoxLayout(scopeField);
		scopeLayout->setContentsMargins(1, 1, 1, 1);
		scopeLayout->setSpacing(0);
		_scopeCombo = new QComboBox(scopeField);
		_scopeCombo->setObjectName("sectionCutScope");
		_scopeCombo->setStyleSheet(QString(
			"QComboBox{border:0;border-radius:0;background:transparent;color:%1;padding:0 %2px;}"
			"QComboBox:disabled{color:%3;}")
			.arg(kTextColor).arg(kGroupInset).arg(kDisabledFg));
		_scopeCombo->setFixedHeight(kInner);
		ScopePickHost* scopePickHost = new ScopePickHost(scopeField);
		_scopePickHost = scopePickHost;
		_scopePick = scopePickHost->entry;
		_scopePick->setObjectName("sectionCutPick");
		_scopePick->setStyleSheet(QString(
			"QPushButton{border:0;border-radius:0;background:transparent;color:%1;text-align:left;"
			"padding:0 %2px 0 %3px;}"
			"QPushButton:checked{background:#E8F3FF;}"
			"QPushButton:disabled{color:%4;}")
			.arg(kTextColor).arg(2 * kControl + kGroupInset).arg(kGroupInset).arg(kDisabledFg));
		scopePickHost->setStyleSheet(QString(
			"QWidget#sectionCutScopePickHost{border:0;border-left:1px solid %1;border-radius:0;background:transparent;}")
			.arg(kCtrlBorder));
		_scopePickInspect = scopePickHost->inspect;
		_scopePickInspect->setObjectName("sectionCutPickInspect");
		_scopePickClear = scopePickHost->clear;
		_scopePickClear->setObjectName("sectionCutPickClear");
		scopeLayout->addWidget(_scopeCombo, 1);
		scopeLayout->addWidget(_scopePickHost, 2);
		PanelLabel* labelScope = new PanelLabel;
		regLabel(labelScope, "剖切范围", "Scope");
		_definitionFold->addRow(labelScope, scopeField);

		_definitionFold->refreshLayout();
		_editorHost->layout()->addWidget(_definitionFold);

		/******** 折叠组：剖切几何定义 ********/
		_geometryFold = new FoldSection(QString::fromUtf8("剖切几何定义"), this);

		//基准剖切平面细线框（原型 fieldset.section-cut-plane；mGroupBox 无勾选图标=纯细线框）
		_planeBox = new MxFunctions::mGroupBox(QString::fromUtf8("基准剖切平面"), QPixmap(), QPixmap(), _geometryFold);
		QGridLayout* planeGrid = _planeBox->grid();
		planeGrid->setHorizontalSpacing(kColumnGap);
		planeGrid->setVerticalSpacing(kGap);
		planeGrid->setColumnStretch(0, 0);
		planeGrid->setColumnStretch(1, 1);

		_definitionChoice = new MxFunctions::mChoiceGroup(
		{ QString::fromUtf8("法向轴"), QString::fromUtf8("三点平面") }, _planeBox);
		_definitionChoice->setObjectName("sectionCutDefinition");
		PanelLabel* labelDefinition = new PanelLabel;
		regLabel(labelDefinition, "定义方式", "Definition");
		planeGrid->addWidget(labelDefinition, 0, 0);
		planeGrid->addWidget(_definitionChoice, 0, 1);

		//法向轴分支：平面法向（L13）+ 条件行 + 基点
		_axisBranch = new QWidget(_planeBox);
		QGridLayout* axisGrid = new QGridLayout(_axisBranch);
		axisGrid->setContentsMargins(0, 0, 0, 0);
		axisGrid->setHorizontalSpacing(kColumnGap);
		axisGrid->setVerticalSpacing(kGap);
		axisGrid->setColumnStretch(0, 0);
		axisGrid->setColumnStretch(1, 1);
		{
			QWidget* head = new QWidget(_axisBranch);
			QHBoxLayout* headLayout = new QHBoxLayout(head);
			headLayout->setContentsMargins(0, 0, 0, 0);
			headLayout->setSpacing(kGap);
			_normalCombo = new QComboBox(head);
			_normalCombo->setObjectName("sectionCutNormal");
			_normalCombo->setFixedHeight(kControl);
			_normalCombo->setStyleSheet(
				QString("QComboBox{border:1px solid %1;border-radius:%2px;background:#FFFFFF;"
					"color:%3;padding:0 %4px;}"
					"QComboBox:disabled{background:#F5F7FA;color:%5;}")
				.arg(kCtrlBorder).arg(kRadius).arg(kTextColor).arg(kGroupInset).arg(kDisabledFg));
			_reverseBtn = new QPushButton(head);
			_reverseBtn->setObjectName("sectionCutReverse");
			_reverseBtn->setCheckable(true);
			_reverseBtn->setFixedSize(kControl, kControl);
			_reverseBtn->setIcon(reverseIcon(QColor(kTextColor)));
			_reverseBtn->setCursor(Qt::PointingHandCursor);
			_reverseBtn->setStyleSheet(
				QString("QPushButton{border:1px solid %1;border-radius:%2px;background:#FFFFFF;}"
					"QPushButton:checked{background:%3;border-color:%4;}"
					"QPushButton:hover{background:%5;}")
				.arg(kCtrlBorder).arg(kRadius).arg(kSelectedBg, kPrimary, kHoverBg));
			headLayout->addWidget(_normalCombo, 1);
			headLayout->addWidget(_reverseBtn);
			PanelLabel* labelNormal = new PanelLabel;
			regLabel(labelNormal, "平面法向", "Normal");
			axisGrid->addWidget(labelNormal, 0, 0);
			axisGrid->addWidget(head, 0, 1);

			//平面法向条件行（原型 L13 extra：向量/起点-终点/选择对象按模式显隐）
			auto conditionRow = [this, axisGrid](QWidget* rowWidget, int gridRow)
			{
				axisGrid->addWidget(rowWidget, gridRow, 0, 1, 2);
			};
			auto labeledEdit = [this](const char* cn, const char* en, QLineEdit*& edit, const char* objectName)
			{
				QWidget* row = new QWidget(_axisBranch);
				QHBoxLayout* lay = new QHBoxLayout(row);
				lay->setContentsMargins(0, 0, 0, 0);
				lay->setSpacing(kColumnGap);
				edit = new QLineEdit(row);
				edit->setObjectName(objectName);
				edit->setFixedHeight(kControl);
				edit->setStyleSheet(
					QString("QLineEdit{background:#FFFFFF;border:1px solid %1;border-radius:%2px;"
						"padding:0 %3px;color:%4;}")
					.arg(kCtrlBorder).arg(kRadius).arg(kGroupInset).arg(kTextColor));
				PanelLabel* label = new PanelLabel;
				regLabel(label, cn, en);
				lay->addWidget(label);
				lay->addWidget(edit, 1);
				return row;
			};
			_vectorRow = labeledEdit("方向向量", "Vector", _vectorEdit, "sectionCutNormalVector");
			_startRow = labeledEdit("起点", "Start", _startEdit, "sectionCutNormalStart");
			_endRow = labeledEdit("终点", "End", _endEdit, "sectionCutNormalEnd");
			conditionRow(_vectorRow, 1);
			conditionRow(_startRow, 2);
			conditionRow(_endRow, 3);

			_objectRow = new QWidget(_axisBranch);
			QHBoxLayout* objectLayout = new QHBoxLayout(_objectRow);
			objectLayout->setContentsMargins(0, 0, 0, 0);
			objectLayout->setSpacing(kColumnGap);
			QPushButton* objectPick = new QPushButton(_objectRow);
			objectPick->setObjectName("sectionCutNormalObject");
			objectPick->setEnabled(false);	//原型 dataset.unavailable：真实方向对象拾取待接入
			objectPick->setIcon(crosshairIcon(QColor(kDisabledFg)));
			objectPick->setFixedHeight(kControl);
			objectPick->setStyleSheet(
				QString("QPushButton{border:1px solid %1;border-radius:%2px;background:#F5F7FA;"
					"color:%3;text-align:left;padding:0 %4px;}")
				.arg(kCtrlBorder).arg(kRadius).arg(kDisabledFg).arg(kGroupInset));
			PanelLabel* labelObject = new PanelLabel;
			regLabel(labelObject, "选择对象", "Object");
			objectLayout->addWidget(labelObject);
			objectLayout->addWidget(objectPick, 1);
			conditionRow(_objectRow, 4);

			_normalError = new QLabel(_axisBranch);
			_normalError->setObjectName("sectionCutNormalError");
			_normalError->setStyleSheet(
				QString("QLabel{color:%1;font-size:11px;background:transparent;border:0;}").arg(kError));
			_normalError->setVisible(false);
			axisGrid->addWidget(_normalError, 5, 0, 1, 2);

			//基点点行（index 0）
			int row = 6;
			buildPointRows(axisGrid, row, 0);
		}

		//三点平面分支：点1..3（默认隐藏）
		_threeBranch = new QWidget(_planeBox);
		_threeBranch->setToolTip(QString::fromUtf8("点1为基点，(点2−点1)×(点3−点1)确定法向"));
		QGridLayout* threeGrid = new QGridLayout(_threeBranch);
		threeGrid->setContentsMargins(0, 0, 0, 0);
		threeGrid->setHorizontalSpacing(kColumnGap);
		threeGrid->setVerticalSpacing(kGap);
		threeGrid->setColumnStretch(0, 0);
		threeGrid->setColumnStretch(1, 1);
		{
			int row = 0;
			for (int i = 0; i < 3; ++i)
				buildPointRows(threeGrid, row, i + 1);
		}
		_axisBranch->setVisible(true);
		_threeBranch->setVisible(false);

		planeGrid->addWidget(_axisBranch, 1, 0, 1, 2);
		planeGrid->addWidget(_threeBranch, 2, 0, 1, 2);

		_definitionError = new QLabel(_planeBox->contentWidget());
		_definitionError->setObjectName("sectionCutDefinitionError");
		_definitionError->setStyleSheet(
			QString("QLabel{color:%1;font-size:11px;background:transparent;border:0;}").arg(kError));
		_definitionError->setVisible(false);
		planeGrid->addWidget(_definitionError, 3, 0, 1, 2);

		_geometryFold->addRow(_planeBox);

		//剖切面偏移（mm 后缀）
		_offsetEdit = new SuffixEdit(QStringLiteral("mm"), _geometryFold);
		_offsetEdit->setObjectName("sectionCutOffset");
		PanelLabel* labelOffset = new PanelLabel;
		regLabel(labelOffset, "剖切面偏移", "Offset");
		_geometryFold->addRow(labelOffset, _offsetEdit);

		//移动步长（连体 −/输入/＋）
		QWidget* stepField = buildStepField(_stepDec, _stepEdit, _stepInc);
		PanelLabel* labelStep = new PanelLabel;
		regLabel(labelStep, "移动步长", "Step");
		_geometryFold->addRow(labelStep, stepField);

		_configChoice = new MxFunctions::mChoiceGroup(
		{ QString::fromUtf8("固定平面"), QString::fromUtf8("随变形移动") }, _geometryFold);
		_configChoice->setObjectName("sectionCutConfiguration");
		PanelLabel* labelConfig = new PanelLabel;
		regLabel(labelConfig, "几何构型", "Config");
		_geometryFold->addRow(labelConfig, _configChoice);

		_sideChoice = new MxFunctions::mChoiceGroup(
		{ QString::fromUtf8("负向侧"), QString::fromUtf8("两侧"), QString::fromUtf8("正向侧") }, _geometryFold);
		_sideChoice->setObjectName("sectionCutSide");
		PanelLabel* labelSide = new PanelLabel;
		regLabel(labelSide, "保留侧", "Keep side");
		_geometryFold->addRow(labelSide, _sideChoice);

		_geometryFold->refreshLayout();
		_editorHost->layout()->addWidget(_geometryFold);

		/******** 参数操作区（V1.3 统一编辑契约；原型操作区位于截面样式上方） ********/
		_editor = new mPostObjectEditController(this);
		_editor->setSnapshotFn([this]() { return detail::sectionCutSignature(working()); });
		_editor->setSaveFn([this](bool immediate) { return commitEdit(immediate); });
		_editor->setCancelFn([this]() { cancelEdit(); });
		_editor->setCurrentNameFn([this]()
			{
				const SectionRecord* rec = currentRecord();
				return rec ? rec->name : QString::fromUtf8("当前截面");
			});
		//注：空表时整个编辑器宿主（含操作区）隐藏=原型 detach 语义，不经 setEditorHost 禁用
		connect(_editor, &mPostObjectEditController::saveRequested, this,
			[this](bool immediate) { if (!immediate) commitEdit(false); });
		connect(_editor, &mPostObjectEditController::cancelRequested, this, [this]() { cancelEdit(); });
		_editor->attachFooter(_editorHost->layout(), -1, _editorHost);

		/******** 折叠组：截面样式 ********/
		_styleFold = new FoldSection(QString::fromUtf8("截面样式"), this);

		_modeChoice = new MxFunctions::mChoiceGroup(
		{ QString::fromUtf8("单平面"), QString::fromUtf8("截面带") }, _styleFold);
		_modeChoice->setObjectName("sectionCutStyleMode");
		PanelLabel* labelMode = new PanelLabel;
		regLabel(labelMode, "剖切模式", "Section mode");
		_styleFold->addRow(labelMode, _modeChoice);

		//带宽（截面带模式条件行；行显隐走 FoldSection::setRowVisible）
		_bandwidthEdit = new SuffixEdit(QStringLiteral("mm"), _styleFold);
		_bandwidthEdit->setObjectName("sectionCutStyleBandwidth");
		PanelLabel* labelBand = new PanelLabel;
		regLabel(labelBand, "带宽", "Bandwidth");
		_styleFold->addRow(labelBand, _bandwidthEdit);

		//填充截面（复选作行标签；原型 checkbox 位于字段内缩起点 x=29，左内边距 12 对齐）
		_fillCheck = new QCheckBox(QString::fromUtf8("填充截面"), _styleFold);
		_fillCheck->setObjectName("sectionCutStyleFill");
		_fillCheck->setChecked(true);
		_fillCheck->setStyleSheet("QCheckBox{padding-left:19px;background:transparent;}");
		_colorModeChoice = new MxFunctions::mChoiceGroup(
		{ QString::fromUtf8("结果着色"), QString::fromUtf8("统一颜色") }, _styleFold);
		_colorModeChoice->setObjectName("sectionCutStyleColorMode");
		_styleFold->addRow(_fillCheck, _colorModeChoice);

		//裁剪显示细线框（2 列复选网格）
		_cropBox = new MxFunctions::mGroupBox(QString::fromUtf8("裁剪显示"), QPixmap(), QPixmap(), _styleFold);
		_cropCheck = new QCheckBox(QString::fromUtf8("裁剪单元"), _cropBox->contentWidget());
		_cropCheck->setObjectName("sectionCutStyleCrop");
		_cropCheck->setChecked(true);
		_ghostCheck = new QCheckBox(QString::fromUtf8("剔除侧半透明"), _cropBox->contentWidget());
		_ghostCheck->setObjectName("sectionCutStyleGhost");
		_cropBox->grid()->addWidget(_cropCheck, 0, 0);
		_cropBox->grid()->addWidget(_ghostCheck, 0, 1);
		_cropBox->grid()->setColumnStretch(0, 1);
		_cropBox->grid()->setColumnStretch(1, 1);
		_styleFold->addRow(_cropBox);

		//辅助显示细线框（2×2 复选网格）
		_auxBox = new MxFunctions::mGroupBox(QString::fromUtf8("辅助显示"), QPixmap(), QPixmap(), _styleFold);
		const char* auxNames[4] = { "剖切平面", "法向", "截面网格", "模型特征边" };
		const char* auxObjects[4] = { "sectionCutStylePlane", "sectionCutStyleNormal",
			"sectionCutStyleMesh", "sectionCutStyleEdges" };
		for (int i = 0; i < 4; ++i)
		{
			_auxChecks[i] = new QCheckBox(QString::fromUtf8(auxNames[i]), _auxBox->contentWidget());
			_auxChecks[i]->setObjectName(auxObjects[i]);
			_auxChecks[i]->setChecked(i == 3);
			_auxBox->grid()->addWidget(_auxChecks[i], i / 2, i % 2);
		}
		_auxBox->grid()->setColumnStretch(0, 1);
		_auxBox->grid()->setColumnStretch(1, 1);
		_styleFold->addRow(_auxBox);

		_styleFold->refreshLayout();
		_editorHost->layout()->addWidget(_styleFold);
		_mainLayout->addStretch();

		//折叠区注册到基类统一管理
		m_foldWidgets.append(_definitionFold);
		m_foldWidgets.append(_geometryFold);
		m_foldWidgets.append(_styleFold);

		/******** 接线 ********/
		connect(_nameEdit, &QLineEdit::textChanged, this, [this](const QString&)
			{ if (!_loadingDraft) storeUIToDraft(); });
		connect(_scopeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mPostSectionCutPanel::slot_scopeChanged);
		connect(_scopePick, &QPushButton::toggled, this, &mPostSectionCutPanel::slot_scopePickClicked);
		connect(_scopePickInspect, &QPushButton::clicked, this, [this]() { showScopePickList(); });
		connect(_scopePickClear, &QPushButton::clicked, this, [this]() { clearScopePick(); });
		connect(_colorSwatch, &QPushButton::clicked, this, &mPostSectionCutPanel::slot_colorSwatchClicked);

		connect(_definitionChoice, &MxFunctions::mChoiceGroup::currentIndexChanged,
			this, &mPostSectionCutPanel::slot_definitionChanged);
		connect(_normalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mPostSectionCutPanel::slot_normalModeChanged);
		connect(_reverseBtn, &QPushButton::toggled, this, &mPostSectionCutPanel::slot_reverseClicked);
		for (QLineEdit* edit : { _vectorEdit, _startEdit, _endEdit })
			connect(edit, &QLineEdit::textChanged, this, [this](const QString&)
				{ if (!_loadingDraft) { storeUIToDraft(); validate(); } });

		connect(_stepDec, &QPushButton::clicked, this, &mPostSectionCutPanel::slot_stepDecrease);
		connect(_stepInc, &QPushButton::clicked, this, &mPostSectionCutPanel::slot_stepIncrease);
		connect(_offsetEdit, &QLineEdit::textChanged, this, [this](const QString&)
			{ if (!_loadingDraft) { storeUIToDraft(); validate(); } });
		connect(_stepEdit, &QLineEdit::textChanged, this, [this](const QString&)
			{ if (!_loadingDraft) { storeUIToDraft(); validate(); } });
		connect(_configChoice, &MxFunctions::mChoiceGroup::currentIndexChanged, this, [this](int)
			{ if (!_loadingDraft) storeUIToDraft(); });
		connect(_sideChoice, &MxFunctions::mChoiceGroup::currentIndexChanged, this, [this](int)
			{ if (!_loadingDraft) storeUIToDraft(); });

		connect(_modeChoice, &MxFunctions::mChoiceGroup::currentIndexChanged,
			this, &mPostSectionCutPanel::slot_styleModeChanged);
		connect(_bandwidthEdit, &QLineEdit::textChanged, this, [this](const QString&)
			{ if (!_loadingDraft) { storeUIToDraft(); refreshStyle(); } });
		connect(_fillCheck, &QCheckBox::toggled, this, &mPostSectionCutPanel::slot_fillToggled);
		connect(_colorModeChoice, &MxFunctions::mChoiceGroup::currentIndexChanged, this, [this](int)
			{ if (!_loadingDraft) storeUIToDraft(); });
		connect(_cropCheck, &QCheckBox::toggled, this, &mPostSectionCutPanel::slot_cropToggled);
		connect(_ghostCheck, &QCheckBox::toggled, this, [this](bool)
			{ if (!_loadingDraft) storeUIToDraft(); });
		for (QCheckBox* check : _auxChecks)
			connect(check, &QCheckBox::toggled, this, [this](bool)
				{ if (!_loadingDraft) storeUIToDraft(); });

		//折叠组收起即退出拾取（原型互斥模式：面板/组关闭 endPick）
		for (FoldSection* fold : { _definitionFold, _geometryFold, _styleFold })
			connect(fold, &FoldSection::foldExpandChanged, this, [this](bool expand)
				{ if (!expand) endPick(); });
	}

	void mPostSectionCutPanel::setupEditorHost()
	{
		_editorHost = new QWidget(this);
		QVBoxLayout* hostLayout = new QVBoxLayout(_editorHost);
		hostLayout->setContentsMargins(0, 0, 0, 0);
		hostLayout->setSpacing(kGap);
		_mainLayout->addWidget(_editorHost);
	}

	void mPostSectionCutPanel::setupManagerSection()
	{
		_managerFold = new FoldSection(QString::fromUtf8("截面管理"), this);

		//搜索框（原型 #sectionCutSearch：宽 100%、高 26、底边距 6）
		_searchEdit = new QLineEdit(_managerFold);
		_searchEdit->setObjectName("sectionCutSearch");
		_searchEdit->setFixedHeight(kControl);
		_searchEdit->setStyleSheet(
			QString("QLineEdit{background:#FFFFFF;border:1px solid %1;border-radius:%2px;"
				"padding:0 %3px;color:%4;}")
			.arg(kCtrlBorder).arg(kRadius).arg(kGroupInset).arg(kTextColor));
		_managerFold->addRow(_searchEdit);

		//管理表（原型 #sectionCutTable：复选 26 / 名称 / 激活 62；行高 26；
		//最小六行 184、最大十行 288——M2/CAEManagerTableHeight 口径）
		_managerTable = new QTableWidget(0, 3, _managerFold);
		_managerTable->setObjectName("sectionCutTable");
		_managerTable->setMinimumHeight(184);	//最小六行（表头+6 行+边框，M2 口径）
		_managerTable->setMaximumHeight(288);	//最大十行，超出滚动
		_managerTable->setStyleSheet(QString(
			"QTableWidget{border:1px solid %1;background:#FFFFFF;gridline-color:%1;}"
			"QTableWidget::item{padding:0 7px;border:0;}"
			"QHeaderView::section{background:%2;color:%3;border:0;border-bottom:1px solid %1;"
			"padding:0 4px;font-size:12px;}"
			"QTableCornerButton::section{background:%2;border:0;border-bottom:1px solid %1;}")
			.arg(kCtrlBorder, "#E9EDF3", "#5C6674"));
		_managerTable->setHorizontalHeaderLabels(QStringList()
			<< QString() << QString::fromUtf8("名称") << QString::fromUtf8("激活"));
		_managerTable->verticalHeader()->hide();
		_managerTable->verticalHeader()->setDefaultSectionSize(kControl);
		QHeaderView* header = _managerTable->horizontalHeader();
		header->setFixedHeight(kControl);
		header->setSectionResizeMode(0, QHeaderView::Fixed);
		header->resizeSection(0, kControl);
		header->setSectionResizeMode(1, QHeaderView::Stretch);
		header->setSectionResizeMode(2, QHeaderView::Fixed);
		header->resizeSection(2, 62);
		header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		_managerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		_managerTable->setSelectionMode(QAbstractItemView::NoSelection);
		_managerTable->setFocusPolicy(Qt::NoFocus);
		_managerFold->addRow(_managerTable);

		//操作行：全选/计数（左） + 删除/新建（右）
		QWidget* actions = new QWidget(_managerFold);
		QHBoxLayout* actionsLayout = new QHBoxLayout(actions);
		actionsLayout->setContentsMargins(0, 0, 0, 0);
		actionsLayout->setSpacing(kGap);
		const char* btnQss =
			"QPushButton{border:1px solid %1;border-radius:4px;background:#FFFFFF;color:#303313;}"
			"QPushButton:hover{background:#F0F7FF;}"
			"QPushButton:disabled{background:#F5F7FA;color:#A6AFBB;}";
		_selectAllBtn = new QPushButton(QString::fromUtf8("全选"), actions);
		_selectAllBtn->setObjectName("sectionCutSelectAll");
		_selectAllBtn->setStyleSheet(QString(btnQss).arg(kCtrlBorder));
		_selectAllBtn->setFixedSize(kActionW, kControl);
		_selectAllBtn->setCursor(Qt::PointingHandCursor);
		_selectedCount = new QLabel(actions);
		_selectedCount->setObjectName("sectionCutSelectionCount");
		_selectedCount->setStyleSheet(QString("QLabel{color:%1;font-size:11px;background:transparent;}")
			.arg(kMuted));
		_deleteBtn = new QPushButton(QString::fromUtf8("删除"), actions);
		_deleteBtn->setObjectName("sectionCutDelete");
		_deleteBtn->setStyleSheet(QString(btnQss).arg(kCtrlBorder));
		_deleteBtn->setFixedSize(kActionW, kControl);
		_deleteBtn->setCursor(Qt::PointingHandCursor);
		_deleteBtn->setToolTip(QString::fromUtf8("删除勾选截面，包含搜索范围外已勾选项"));
		_newBtn = new QPushButton(QString::fromUtf8("新建"), actions);
		_newBtn->setObjectName("sectionCutNew");
		_newBtn->setStyleSheet(
			QString("QPushButton{border:1px solid %1;border-radius:4px;background:%1;color:#FFFFFF;}"
				"QPushButton:hover{background:#1677C8;}"
				"QPushButton:disabled{background:#F5F7FA;color:#A6AFBB;border-color:#DCE2E9;}")
			.arg(kPrimary));
		_newBtn->setFixedSize(kActionW, kControl);
		_newBtn->setCursor(Qt::PointingHandCursor);
		actionsLayout->addWidget(_selectAllBtn);
		actionsLayout->addWidget(_selectedCount);
		actionsLayout->addStretch(1);
		actionsLayout->addWidget(_deleteBtn);
		actionsLayout->addWidget(_newBtn);
		_managerFold->addRow(actions);

		//状态行（校验问题；空文案收起）
		_managerStatus = new QLabel(_managerFold);
		_managerStatus->setObjectName("sectionCutManagerStatus");
		_managerStatus->setStyleSheet(QString("QLabel{color:%1;font-size:11px;background:transparent;}")
			.arg(kError));
		_managerStatus->setVisible(false);
		_managerFold->addRow(_managerStatus);

		_managerFold->refreshLayout();
		_mainLayout->addWidget(_managerFold);

		//接线
		connect(_searchEdit, &QLineEdit::textChanged, this, &mPostSectionCutPanel::slot_searchChanged);
		connect(_selectAllBtn, &QPushButton::clicked, this, &mPostSectionCutPanel::slot_selectAllClicked);
		connect(_deleteBtn, &QPushButton::clicked, this, &mPostSectionCutPanel::slot_deleteClicked);
		connect(_newBtn, &QPushButton::clicked, this, &mPostSectionCutPanel::slot_newClicked);
		connect(header, &QHeaderView::sectionClicked, this, &mPostSectionCutPanel::slot_headerClicked);
		connect(_managerTable, &QTableWidget::cellClicked, this, &mPostSectionCutPanel::slot_cellClicked);
	}

	void mPostSectionCutPanel::regLabel(PanelLabel* label, const char* cn, const char* en)
	{
		_labelTexts.append({ label, cn, en });
		label->setText(QString::fromUtf8(cn));
	}

	void mPostSectionCutPanel::buildPointRows(QGridLayout* grid, int& row, int index)
	{
		PointField& field = (index == 0) ? _basePoint : _threePoints[index - 1];
		PanelLabel* label = new PanelLabel;
		if (index == 0)
			regLabel(label, "基点", "Base point");
		else
		{
			static const char* cnNames[3] = { "点1", "点2", "点3" };
			static const char* enNames[3] = { "Point 1", "Point 2", "Point 3" };
			regLabel(label, cnNames[index - 1], enNames[index - 1]);
		}

		//拾取（蓝条入口 1fr）+ 应用（40px）
		QWidget* head = new QWidget;
		QHBoxLayout* headLayout = new QHBoxLayout(head);
		headLayout->setContentsMargins(0, 0, 0, 0);
		headLayout->setSpacing(kGap);
		field.pick = new PointPickButton(head);
		field.pick->setObjectName(index == 0 ? "sectionCutPointPick"
			: QString("sectionCutPlanePoint%1Pick").arg(index));
		field.pick->setIcon(crosshairIcon(QColor(kTextColor)));
		field.pick->setIconSize(QSize(20, 20));
		field.apply = new QPushButton(QString::fromUtf8("应用"), head);
		field.apply->setObjectName(index == 0 ? "sectionCutPointApply"
			: QString("sectionCutPlanePoint%1Apply").arg(index));
		field.apply->setFixedSize(kActionW, kControl);
		field.apply->setCursor(Qt::PointingHandCursor);
		field.apply->setStyleSheet(
			QString("QPushButton{border:1px solid %1;border-radius:%2px;background:#FFFFFF;color:%3;}"
				"QPushButton:hover{background:%4;}"
				"QPushButton:disabled{background:#F5F7FA;border-color:%1;color:%5;}")
			.arg(kCtrlBorder).arg(kRadius).arg(kTextColor, kHoverBg, kDisabledFg));
		headLayout->addWidget(field.pick, 1);
		headLayout->addWidget(field.apply);

		field.edit = new QLineEdit;
		field.edit->setObjectName(index == 0 ? "sectionCutPointCoordinates"
			: QString("sectionCutPlanePoint%1Coordinates").arg(index));
		field.edit->setPlaceholderText(QStringLiteral("X, Y, Z"));
		field.edit->setFixedHeight(kControl);
		field.edit->setStyleSheet(
			QString("QLineEdit{background:#FFFFFF;border:1px solid %1;border-radius:%2px;"
				"padding:0 %3px;color:%4;}")
			.arg(kCtrlBorder).arg(kRadius).arg(kGroupInset).arg(kTextColor));

		field.error = new QLabel;
		field.error->setObjectName(index == 0 ? "sectionCutPointError"
			: QString("sectionCutPlanePoint%1Error").arg(index));
		field.error->setStyleSheet(
			QString("QLabel{color:%1;font-size:11px;background:transparent;border:0;}").arg(kError));
		field.error->setVisible(false);

		grid->addWidget(label, row, 0);
		grid->addWidget(head, row, 1);
		++row;
		grid->addWidget(field.edit, row, 1);	//坐标行占字段列（原型 grid-column:2）
		++row;
		grid->addWidget(field.error, row, 0, 1, 2);
		++row;

		//接线（lambda 捕获 index：armed 互斥/应用提交/文本编辑）
			connect(field.pick, &QPushButton::toggled, this, [this, index](bool checked)
				{
					if (_loadingDraft) return;
					if (checked)
					{
						endPick();	//退出其他武装（范围拾取/别的点）
						REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
						if (armPick(mxr::PickFilter::PickMeshNode) && rend != nullptr
							&& rend->getSelectionCaculator() != nullptr)
						{
							_activePoint = index;
							_pickConn = connect(rend->getSelectionCaculator(),
								&mxr::PostSelectionCalculator::signals_FinishPick, this,
								[this, index](mxr::FinishPickInfo) { handlePointPicked(index); });
						}
						//武装失败（无后处理视口）：_activePoint 保持 -1，
						//refreshAllPointFields 将回弹按钮勾选态（toggled(false)→endPick 空转）
					}
					else if (_activePoint == index)
					{
						endPick();
					}
					refreshAllPointFields();
				});
		connect(field.apply, &QPushButton::clicked, this, [this, index]()
			{
				if (_loadingDraft) return;
				SectionCutPointDraft& pd = pointDraftAt(index);
				const QList<double> values = parsePoint(pd.text);
				if (!pd.dirty || values.isEmpty()) return;
				pd.text = QString("%1, %2, %3").arg(values.at(0)).arg(values.at(1)).arg(values.at(2));
				pd.dirty = false;
				endPick();
				refreshAllPointFields();
				validate();
			});
		connect(field.edit, &QLineEdit::textChanged, this, [this, index](const QString& text)
			{
				if (_loadingDraft || _syncingUI) return;
				SectionCutPointDraft& pd = pointDraftAt(index);
				pd.text = text;
				pd.dirty = true;
				refreshAllPointFields();
				validate();
			});
		connect(field.edit, &QLineEdit::textEdited, this, [this]()
			{
				//进入编辑即结束拾取武装态（原型 onFocus endPick）
				endPick();
			});
	}

	QString mPostSectionCutPanel::currentDraftKey() const
	{
		//草稿按结果模型独立（原型"每个结果模型独立保留会话草稿"）：以后处理视口
		//当前结果库名为键；无后处理视口时用公共默认草稿
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend == nullptr || rend->getPostRender() == nullptr)
			return QString();
		const QString postname = rend->getPostDataName();
		return postname;
	}

	void mPostSectionCutPanel::applyDefaultPointTexts(SectionCutDraft& d)
	{
		//默认几何锚定当前模型：基点=包围盒中心（与旧切面图/applySectionCuts 的
		//getCurrentModelData 同口径），三点平面以中心为锚——默认平面必过模型内部，
		//杜绝默认基点落在模型外致整模型被裁；无视口/空模型时保持原点
		double cx = 0.0, cy = 0.0, cz = 0.0;
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend != nullptr && rend->getPostRender() != nullptr)
		{
			const mxr::AABB aabb = rend->getPostRender()->getCurrentModelData();
			const double dx = aabb.maxEdge[0] - aabb.minEdge[0];
			const double dy = aabb.maxEdge[1] - aabb.minEdge[1];
			const double dz = aabb.maxEdge[2] - aabb.minEdge[2];
			const double diagonal = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (diagonal > 0 && qIsFinite(diagonal))
			{
				cx = (aabb.minEdge[0] + aabb.maxEdge[0]) / 2.0;
				cy = (aabb.minEdge[1] + aabb.maxEdge[1]) / 2.0;
				cz = (aabb.minEdge[2] + aabb.maxEdge[2]) / 2.0;
			}
		}
		const auto coord = [](double v) { return QString::number(v, 'g', 10); };
		const QString base = coord(cx) + QStringLiteral(", ") + coord(cy)
			+ QStringLiteral(", ") + coord(cz);
		d.basePoint.text = base;
		d.point1.text = base;
		d.point2.text = coord(cx + 1.0) + QStringLiteral(", ") + coord(cy)
			+ QStringLiteral(", ") + coord(cz);
		d.point3.text = coord(cx) + QStringLiteral(", ") + coord(cy + 1.0)
			+ QStringLiteral(", ") + coord(cz);
	}

	mPostSectionCutPanel::ModelSession& mPostSectionCutPanel::session()
	{
		const QString key = currentDraftKey();
		_sessionKey = key;
		ModelSession& s = _sessions[key];
		//空会话不自建记录（原型空态：表格空+编辑器禁用展示默认参数，"新建"才创建）；
		//当前记录失效时回退首条
		if (!s.currentId.isEmpty()
			&& std::find_if(s.records.cbegin(), s.records.cend(),
				[&s](const SectionRecord& r) { return r.id == s.currentId; }) == s.records.cend())
			s.currentId = s.records.first().id;
		return s;
	}

	const mPostSectionCutPanel::ModelSession& mPostSectionCutPanel::session() const
	{
		static const ModelSession empty;
		const auto it = _sessions.constFind(currentDraftKey());
		return it != _sessions.constEnd() ? it.value() : empty;
	}

	mPostSectionCutPanel::SectionRecord* mPostSectionCutPanel::currentRecord()
	{
		ModelSession& s = session();
		for (SectionRecord& rec : s.records)
			if (rec.id == s.currentId)
				return &rec;
		return nullptr;
	}

	const mPostSectionCutPanel::SectionRecord* mPostSectionCutPanel::currentRecord() const
	{
		const ModelSession& s = session();
		for (const SectionRecord& rec : s.records)
			if (rec.id == s.currentId)
				return &rec;
		return nullptr;
	}

	SectionCutDraft& mPostSectionCutPanel::working()
	{
		SectionRecord* rec = currentRecord();
		static SectionCutDraft defaultDraft;	//无记录时只读兜底
		return rec ? rec->editorDraft : defaultDraft;
	}

	const SectionCutDraft& mPostSectionCutPanel::working() const
	{
		const SectionRecord* rec = currentRecord();
		static const SectionCutDraft defaultDraft;
		return rec ? rec->editorDraft : defaultDraft;
	}

	QString mPostSectionCutPanel::nameError() const
	{
		//原型 nameError：空名/与其他记录重名
		const SectionCutDraft& d = working();
		if (d.name.trimmed().isEmpty())
			return _langEn ? QString("Section name cannot be empty.") : QString::fromUtf8("截面名称不能为空");
		const ModelSession& s = session();
		for (const SectionRecord& rec : s.records)
			if (rec.id != s.currentId && rec.name == d.name.trimmed())
				return _langEn ? QString("Section name already exists.") : QString::fromUtf8("截面名称已存在");
		return QString();
	}

	void mPostSectionCutPanel::updateSectionCutPanel()
	{
		ModelSession& s = session();	//空会话不建记录；当前记录失效时回退首条
		if (s.currentId.isEmpty())
		{
			//原型空态：表格空（"暂无截面"）+ 编辑器可见但整体禁用（展示默认参数，
			//"新建"才创建记录；禁用即不可交互，控件信号不会写兜底草稿）
			_editor->endEdit();
			_editorHost->setVisible(true);
			_editorHost->setEnabled(false);
			SectionCutDraft draft;
			applyDefaultPointTexts(draft);
			loadDraftToUI(draft);
			renderManagerTable();
			refreshManagerState();
			refreshManagerStatus();
			return;
		}
		_editorHost->setVisible(true);
		_editorHost->setEnabled(true);
		//同一记录重复进入不重取基线（保留未保存编辑的 dirty 状态；原型编辑器实例常驻）
		if (_begunKey != _sessionKey || _begunId != s.currentId)
		{
			loadDraftToUI(currentRecord()->editorDraft);
			_editor->beginEdit();
			_begunKey = _sessionKey;
			_begunId = s.currentId;
		}
		renderManagerTable();
		refreshManagerStatus();
		//打开/切换视口即对齐渲染：激活集投影到当前视口 guidata 并提交
		//（重复进入幂等——apply 全量重建）
		syncActiveSectionsToRender();
	}

	void mPostSectionCutPanel::loadDraftToUI(const SectionCutDraft& d)
	{
		_loadingDraft = true;

		_nameEdit->setText(d.name);
		applySwatchColor(_colorSwatch, d.color);
		_scopeCombo->setCurrentIndex(d.scope);
		_definitionChoice->setCurrentIndex(d.definition);
		_normalCombo->setCurrentIndex(d.normalMode);
		_reverseBtn->setChecked(d.reversed);
		_vectorEdit->setText(d.vectorText);
		_startEdit->setText(d.startText);
		_endEdit->setText(d.endText);
		_basePoint.edit->setText(d.basePoint.text);
		_threePoints[0].edit->setText(d.point1.text);
		_threePoints[1].edit->setText(d.point2.text);
		_threePoints[2].edit->setText(d.point3.text);
		_offsetEdit->setText(d.offset);
		_stepEdit->setText(d.step);
		_configChoice->setCurrentIndex(d.configuration);
		_sideChoice->setCurrentIndex(d.side);
		_modeChoice->setCurrentIndex(d.styleMode);
		_bandwidthEdit->setText(d.bandwidth);
		_fillCheck->setChecked(d.fill);
		_colorModeChoice->setCurrentIndex(d.colorMode);
		_cropCheck->setChecked(d.crop);
		_ghostCheck->setChecked(d.ghost);
		for (int i = 0; i < 4; ++i)
			_auxChecks[i]->setChecked(i == 0 ? d.showPlane : i == 1 ? d.showNormal
				: i == 2 ? d.showMesh : d.showEdges);

		//文本装载会触发 textChanged 把 dirty 置位，按原型载入即已应用语义复位
		pointDraftAt(0).dirty = false;
		for (int i = 1; i <= 3; ++i)
			pointDraftAt(i).dirty = false;

		_loadingDraft = false;
		syncDefinitionBranch();
		syncDirectionExtras();
		refreshScopePick();
		refreshStyle();
		refreshAllPointFields();
		validate();
	}

	void mPostSectionCutPanel::storeUIToDraft()
	{
		if (currentRecord() == nullptr)
			return;		//空态无记录：禁写（防兜底草稿被污染，mPostSectionForce 同款守卫）
		SectionCutDraft& d = working();
		d.name = _nameEdit->text();
		d.scope = _scopeCombo->currentIndex();
		d.definition = _definitionChoice->currentIndex();
		d.normalMode = _normalCombo->currentIndex();
		d.reversed = _reverseBtn->isChecked();
		d.vectorText = _vectorEdit->text();
		d.startText = _startEdit->text();
		d.endText = _endEdit->text();
		d.offset = _offsetEdit->text();
		d.step = _stepEdit->text();
		d.configuration = _configChoice->currentIndex();
		d.side = _sideChoice->currentIndex();
		d.styleMode = _modeChoice->currentIndex();
		d.bandwidth = _bandwidthEdit->text();
		d.fill = _fillCheck->isChecked();
		d.colorMode = _colorModeChoice->currentIndex();
		d.crop = _cropCheck->isChecked();
		d.ghost = _ghostCheck->isChecked();
		d.showPlane = _auxChecks[0]->isChecked();
		d.showNormal = _auxChecks[1]->isChecked();
		d.showMesh = _auxChecks[2]->isChecked();
		d.showEdges = _auxChecks[3]->isChecked();

		//统一编辑契约：任意字段变更通知（dirty 重算/即时保存调度）
		if (_editor)
		{
			_editor->notifyEditChanged();
			refreshManagerStatus();
		}
	}

	QString mPostSectionCutPanel::normalErrorMessage() const
	{
		//detail::normalIssue 的问题码 → 面板文案（原型 createLoadGeneralDirection validate）
		switch (detail::normalIssue(working()))
		{
		case detail::NormalIssue::InvalidComponents:
			return _langEn ? QString("Enter three finite coordinate components.")
				: QString::fromUtf8("请输入三个有限坐标分量。");
		case detail::NormalIssue::ZeroDirection:
			return _langEn ? QString("Direction cannot be zero; start and end must differ.")
				: QString::fromUtf8("方向不能为零向量，起点与终点不能重合。");
		default:
			return QString();
		}
	}

	QString mPostSectionCutPanel::geometryError() const
	{
		//detail::geometryIssue 的问题码 → 面板文案（判定顺序与原型一致）
		switch (detail::geometryIssue(working()))
		{
		case detail::GeomIssue::PendingPoints:
			return _langEn ? QString("Apply the pending point positions first.")
				: QString::fromUtf8("请先应用有效的点位置。");
		case detail::GeomIssue::InvalidBase:
			return _langEn ? QString("Enter three finite base point coordinates.")
				: QString::fromUtf8("请输入三个有限的基点坐标。");
		case detail::GeomIssue::NoDirectionObject:
			return _langEn ? QString("No direction objects available yet.")
				: QString::fromUtf8("暂无可用方向对象。");
		case detail::GeomIssue::InvalidNormal:
			return _langEn ? QString("Define a valid non-zero normal direction.")
				: QString::fromUtf8("请定义有效的非零法向方向。");
		case detail::GeomIssue::InvalidThreePoints:
			return _langEn ? QString("Enter three finite coordinates for each point.")
				: QString::fromUtf8("请输入三个点的有限坐标。");
		case detail::GeomIssue::PointsOutOfRange:
			return _langEn ? QString("Point coordinate differences out of range.")
				: QString::fromUtf8("点间坐标差超出有效数值范围。");
		case detail::GeomIssue::CollinearPoints:
			return _langEn ? QString("The three points must not coincide or be collinear.")
				: QString::fromUtf8("三点不能重合或共线。");
		default:
			return QString();
		}
	}

	bool mPostSectionCutPanel::validate()
	{
		const SectionCutDraft& d = working();
		const bool stepOk = finiteText(d.step) && d.step.trimmed().toDouble() > 0;
		const bool offsetOk = finiteText(d.offset);
		const QString message = geometryError();
		const QString normalMessage = normalErrorMessage();

		_stepDec->setEnabled(stepOk && offsetOk && message.isEmpty());
		_stepInc->setEnabled(stepOk && offsetOk && message.isEmpty());

		//原型 DefinitionError 显隐：几何错误存在时显示，但"轴模式+基点有效+法向错误行
		//已在显"时不重复显示（方向细节文案由 _normalError 承担）
		const bool pointValid = parsePoint(d.basePoint.text).size() == 3;
		const bool hideDefinitionError = message.isEmpty()
			|| (d.definition == 0 && pointValid && !normalMessage.isEmpty());
		_definitionError->setText(message);
		_definitionError->setVisible(!hideDefinitionError);

		_normalError->setText(normalMessage);
		_normalError->setVisible(!normalMessage.isEmpty());

		return stepOk && offsetOk && message.isEmpty();
	}

	void mPostSectionCutPanel::moveOffset(int direction)
	{
		if (!validate())
			return;
		SectionCutDraft& d = working();
		const double offset = d.offset.trimmed().toDouble();
		const double step = d.step.trimmed().toDouble();
		const double value = offset + direction * step;
		if (!qIsFinite(value))
			return;
		d.offset = QString::number(value, 'g', 14);	//原型 toPrecision(14)+Number 归一
		_offsetEdit->setText(d.offset);
	}

	void mPostSectionCutPanel::syncDefinitionBranch()
	{
		const int definition = _definitionChoice->currentIndex();
		_axisBranch->setVisible(definition == 0);
		_threeBranch->setVisible(definition == 1);
		//分支隐藏时禁用内部控件（原型 syncDefinition 全控件启停）
		for (QWidget* branch : { _axisBranch, _threeBranch })
		{
			const bool enabled = (definition == 0) == (branch == _axisBranch);
			branch->setEnabled(enabled);
		}
		refreshAllPointFields();
		validate();
	}

	void mPostSectionCutPanel::syncDirectionExtras()
	{
		const int mode = _normalCombo->currentIndex();
		_vectorRow->setVisible(mode == 5);
		_startRow->setVisible(mode == 3);
		_endRow->setVisible(mode == 3);
		_objectRow->setVisible(mode == 4);
		validate();
	}

	void mPostSectionCutPanel::refreshAllPointFields()
	{
		const int definition = _definitionChoice->currentIndex();
		for (int index = 0; index <= 3; ++index)
		{
			PointField& field = (index == 0) ? _basePoint : _threePoints[index - 1];
			SectionCutPointDraft& pd = pointDraftAt(index);
			const bool enabled = (definition == 0) == (index == 0);
			const bool armed = (_activePoint == index);
			const bool valid = parsePoint(pd.text).size() == 3;

			field.pick->setEnabled(enabled);
			field.edit->setEnabled(enabled);
			field.apply->setEnabled(enabled && pd.dirty && valid);
			//程序化同步勾选态须屏蔽信号：否则 toggled 再入处理器的 endPick/再武装
			//形成无限递归（V1.3 存量隐患，真实拾取接入后必踩）
			field.pick->blockSignals(true);
			field.pick->setChecked(armed);
			field.pick->blockSignals(false);
			field.pick->setText(_langEn
				? (armed ? QString("Picking…") : QString("Pick point"))
				: (armed ? QString::fromUtf8("正在拾取…") : QString::fromUtf8("拾取点")));
			if (field.edit->text() != pd.text)
			{
				_syncingUI = true;
				field.edit->setText(pd.text);
				_syncingUI = false;
			}
			field.edit->setToolTip(pd.text);

			const QString error = (pd.dirty && !valid)
				? (_langEn ? QString("Enter three finite coordinates (X, Y, Z).")
					: QString::fromUtf8("请输入三个有限坐标（X, Y, Z）。"))
				: QString();
			field.error->setText(error);
			field.error->setVisible(enabled && !error.isEmpty());
		}
	}

	SectionCutPointDraft& mPostSectionCutPanel::pointDraftAt(int index)
	{
		SectionCutDraft& d = working();
		switch (index)
		{
		case 0: return d.basePoint;
		case 1: return d.point1;
		case 2: return d.point2;
		default: return d.point3;
		}
	}

	const SectionCutPointDraft& mPostSectionCutPanel::pointDraftAt(int index) const
	{
		const SectionCutDraft& d = working();
		switch (index)
		{
		case 0: return d.basePoint;
		case 1: return d.point1;
		case 2: return d.point2;
		default: return d.point3;
		}
	}

	void mPostSectionCutPanel::refreshScopePick()
	{
		const SectionCutDraft& d = working();
		const bool all = (d.scope == 0);
		const QStringList& items = (d.scope == 2) ? d.scopeElements : d.scopeParts;
		_scopePick->setEnabled(!all);
		_scopePickInspect->setEnabled(!all && !items.isEmpty());
		_scopePickClear->setEnabled(!all && !items.isEmpty());
		_scopePickInspect->setToolTip(!all && !items.isEmpty()
			? (_langEn ? QString("View the picked list") : QString::fromUtf8("查看已选清单"))
			: (_langEn ? QString("No picked objects yet") : QString::fromUtf8("暂无已选对象")));
		_scopePickClear->setToolTip(!all && !items.isEmpty()
			? (_langEn ? QString("Clear the picked selection") : QString::fromUtf8("清空已选对象"))
			: (_langEn ? QString("No picked objects yet") : QString::fromUtf8("暂无已选对象")));
		if (all)
		{
			_scopePick->setText(_langEn ? QString("Pick objects") : QString::fromUtf8("拾取对象"));
			_scopePick->setToolTip(_langEn ? QString("No picking needed for the full scope.")
				: QString::fromUtf8("全部范围无需拾取"));
		}
		else
		{
			const bool element = (d.scope == 2);
			QString text;
			if (!items.isEmpty())
				text = _langEn ? QString("Selected %1").arg(items.size())
				: QString::fromUtf8("已选 %1 项").arg(items.size());
			else
				text = element ? (_langEn ? QString("Pick elements") : QString::fromUtf8("拾取单元"))
				: (_langEn ? QString("Pick parts") : QString::fromUtf8("拾取部件"));
			_scopePick->setText(text);
			if (!_scopePicking)
				_scopePick->setToolTip(element
					? (_langEn ? QString("Pick elements") : QString::fromUtf8("拾取单元"))
					: (_langEn ? QString("Pick parts") : QString::fromUtf8("拾取部件")));
		}
	}

	void mPostSectionCutPanel::refreshStyle()
	{
		const SectionCutDraft& d = working();
		const bool band = (d.styleMode == 1);
		//带宽条件行（行级摘除/插回并同步高度帽）
		_styleFold->setRowVisible(_bandwidthEdit, band);
		_bandwidthEdit->setEnabled(band);

		_colorModeChoice->setEnabled(d.fill);
		_colorSwatch->setEnabled(d.fill);
		_colorSwatch->setToolTip((_langEn ? QString("Section color ") : QString::fromUtf8("截面颜色 "))
			+ d.color.name().toUpper());

		_ghostCheck->setEnabled(d.crop);
	}

	void mPostSectionCutPanel::endPick()
	{
		_scopePicking = false;
		if (_scopePick->isChecked())
			_scopePick->setChecked(false);
		_activePoint = -1;
		disarmPick();
		refreshScopePick();
		refreshAllPointFields();
	}

	bool mPostSectionCutPanel::armPick(mxr::PickFilter filter)
	{
		//先解除既有武装（重复武装=换目标：恢复过滤器后再下发新过滤器）
		disarmPick();
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend == nullptr || rend->getPostRender() == nullptr)
			return false;
		_savedPickFilter = rend->getPickFilter();
		_pickFilterSaved = true;
		rend->setPickFilter(filter);
		return true;
	}

	void mPostSectionCutPanel::disarmPick()
	{
		if (_pickConn)
		{
			disconnect(_pickConn);
			_pickConn = QMetaObject::Connection();
		}
		if (!_pickFilterSaved)
			return;
		_pickFilterSaved = false;
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend == nullptr)
			return;
		rend->setPickFilter(_savedPickFilter);
		MechPreData::ClearPostWidgetMeshPick(rend);
		rend->updatePaint();
	}

	void mPostSectionCutPanel::handlePointPicked(int index)
	{
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend == nullptr || rend->getSelectionCaculator() == nullptr)
			return;
		//单点即走：本次点击的计算结果取首个节点
		const auto compute = rend->getSelectionCaculator()->getMeshSelectComputeData();
		const auto it = compute.find(mxr::SelectionInfoType::MeshNode);
		if (it == compute.end() || it->second.empty())
			return;
		const int nodeId = it->second.front().data_id.node_id;

		//坐标口径同测量工具：原始坐标+当前帧位移（变形显示位置）
		const QString postName = rend->getPostDataName();
		PostData::mPostDataBase* postDB = getCVM->getModelData()
			->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (postDB == nullptr)
			return;
		MechPreData::mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rend);
		PostData::mPostModelData* modeldata = guidata != nullptr
			? postDB->getModelData(guidata->_cloudFrame) : nullptr;
		if (modeldata == nullptr || guidata == nullptr)
			return;
		const QHash<int, QVector3D> dis = postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
		const int cId = modeldata->_nodedata.nodeidmap.value(nodeId, -1);
		const QVector3D pos = modeldata->getVectorById(nodeId) + dis.value(cId);

		SectionCutPointDraft& pd = pointDraftAt(index);
		pd.text = QString("%1, %2, %3").arg(pos.x()).arg(pos.y()).arg(pos.z());
		pd.dirty = false;	//拾取回填=已应用（与点行"应用"钮同语义）
		endPick();
		refreshAllPointFields();
		validate();
		if (_editor)
		{
			_editor->notifyEditChanged();
			refreshManagerStatus();
		}
	}

	void mPostSectionCutPanel::handleScopePicked()
	{
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend == nullptr || rend->getSelectionContainer() == nullptr)
			return;
		SectionCutDraft& d = working();
		const bool element = (d.scope == 2);
		const auto& selected = rend->getSelectionContainer()->getMeshSelectData()
			->getSelectDataByType(element ? mxr::SelectionInfoType::MeshElement
				: mxr::SelectionInfoType::MeshPart);
		QStringList& target = element ? d.scopeElements : d.scopeParts;
		target = _scopeCarriedIds;	//携带集打底，本轮容器拾取并回（减选只作用本轮；携带项经清空钮移除）
		for (const mxr::MeshSelectInfoData& info : selected)
			target.append(QString::number(element ? info.data_id.element_id : info.part_id));
		target.removeDuplicates();
		refreshScopePick();
		if (_editor)
		{
			_editor->notifyEditChanged();
			refreshManagerStatus();
		}
	}

	void mPostSectionCutPanel::showScopePickList()
	{
		const SectionCutDraft& d = working();
		const bool element = (d.scope == 2);
		const QStringList& items = element ? d.scopeElements : d.scopeParts;
		if (items.isEmpty())
			return;

		//轻量弹层（Qt::Popup 挂面板子树）：可滚动 id 列表+计数头
		QWidget* popup = new QWidget(this, Qt::Popup);
		popup->setAttribute(Qt::WA_DeleteOnClose);
		popup->setFixedWidth(200);
		QVBoxLayout* lay = new QVBoxLayout(popup);
		lay->setContentsMargins(6, 6, 6, 6);
		lay->setSpacing(4);
		QLabel* head = new QLabel(_langEn
			? QString("%1 selected: %2").arg(element ? QString("Elements") : QString("Parts")).arg(items.size())
			: QString::fromUtf8("已选%1：%2 项").arg(element ? QString::fromUtf8("单元") : QString::fromUtf8("部件")).arg(items.size()),
			popup);
		head->setStyleSheet(QString("QLabel{color:%1;font-size:11px;background:transparent;}").arg(kMuted));
		lay->addWidget(head);
		QListWidget* list = new QListWidget(popup);
		list->setMaximumHeight(180);
		list->setStyleSheet("QListWidget{border:1px solid #DCE2E9;border-radius:4px;background:#FFFFFF;}"
			"QListWidget::item{padding:1px 6px;color:#303313;}");
		for (const QString& id : items)
			list->addItem((element ? QString::fromUtf8("单元 ") : QString::fromUtf8("部件 ")) + id);
		lay->addWidget(list);
		popup->adjustSize();

		//锚定拾取钮下方（越界收回屏内；popupUnder 为 FbdColorPicker 专用，此处同规则内联）
		const QPoint global = _scopePick->mapToGlobal(QPoint(0, _scopePick->height() + 2));
		QScreen* screen = QGuiApplication::screenAt(global);
		if (!screen) screen = QGuiApplication::primaryScreen();
		QPoint pos = global;
		const QRect avail = screen->availableGeometry();
		if (pos.x() + popup->width() > avail.right())
			pos.setX(avail.right() - popup->width());
		if (pos.y() + popup->height() > avail.bottom())
			pos.setY(global.y() - _scopePick->height() - 2 - popup->height());
		popup->move(pos);
		popup->show();
	}

	void mPostSectionCutPanel::clearScopePick()
	{
		SectionCutDraft& d = working();
		d.scopeParts.clear();
		d.scopeElements.clear();
		_scopeCarriedIds.clear();
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend != nullptr)
		{
			MechPreData::ClearPostWidgetMeshPick(rend);
			rend->updatePaint();
		}
		refreshScopePick();
		if (_editor)
		{
			_editor->notifyEditChanged();
			refreshManagerStatus();
		}
	}

	void mPostSectionCutPanel::applySwatchColor(QPushButton* swatch, const QColor& color)
	{
		swatch->setStyleSheet(QString(
			"QPushButton{background:%1;border:1px solid %2;border-radius:%3px;}"
			"QPushButton:disabled{background:%1;color:%4;}")
			.arg(color.name(), kCtrlBorder).arg(kRadius).arg(kDisabledFg));
	}

	void mPostSectionCutPanel::popupUnder(FbdColorPicker* popup, QWidget* anchor)
	{
		//标签面板同款：锚点下方弹出，越界收回屏内
		const QPoint global = anchor->mapToGlobal(QPoint(0, anchor->height() + 2));
		QScreen* screen = QGuiApplication::screenAt(global);
		if (!screen) screen = QGuiApplication::primaryScreen();
		QPoint pos = global;
		const QRect avail = screen->availableGeometry();
		if (pos.x() + popup->width() > avail.right())
			pos.setX(avail.right() - popup->width());
		if (pos.y() + popup->height() > avail.bottom())
			pos.setY(global.y() - anchor->height() - 2 - popup->height());
		popup->popupAt(pos);
	}

	void mPostSectionCutPanel::slot_changeLanguage(int language)
	{
		_langEn = (language == 1);
		applyLanguageTexts();
	}

	void mPostSectionCutPanel::slot_scopeChanged(int index)
	{
		if (_loadingDraft) return;
		endPick();
		storeUIToDraft();
		refreshScopePick();
	}

	void mPostSectionCutPanel::slot_scopePickClicked(bool checked)
	{
		if (_loadingDraft) return;
		if (checked)
		{
			//退出其他武装但不动本钮勾选态（endPick 会取消本钮致重入）：手动清点拾取
			_activePoint = -1;
			disarmPick();
			refreshAllPointFields();

			const bool element = (_scopeCombo->currentIndex() == 2);
			REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
			if (armPick(element ? mxr::PickFilter::PickMeshElement : mxr::PickFilter::PickMeshPart)
				&& rend != nullptr && rend->getSelectionContainer() != nullptr)
			{
				_scopePicking = true;
				//携带集：上轮已选 id 并回本轮（退出武装时容器被清，草稿不丢）
				_scopeCarriedIds = element ? working().scopeElements : working().scopeParts;
				//累计通道：视口增选（Ctrl 减选）实时回读容器 → 草稿 id 集
				_pickConn = connect(rend->getSelectionContainer(),
					&mxr::SelectionDataContainer::ReadyToReadContainer, this,
					[this]() { handleScopePicked(); });
				_scopePick->setToolTip(_langEn
					? QString("Click the viewport to pick; Ctrl-click to remove; click again to finish.")
					: QString::fromUtf8("点击视口拾取%1（Ctrl 点击减选），再次点击结束")
						.arg(element ? QString::fromUtf8("单元") : QString::fromUtf8("部件")));
			}
			else
			{
				//无可用视口：回弹勾选态（toggled(false)→endPick 空转收尾）
				_scopePicking = false;
				_scopePick->setChecked(false);
				refreshScopePick();
			}
		}
		else
		{
			_scopePicking = false;
			endPick();
		}
	}

	void mPostSectionCutPanel::slot_definitionChanged(int)
	{
		if (_loadingDraft) return;
		endPick();
		storeUIToDraft();
		syncDefinitionBranch();
	}

	void mPostSectionCutPanel::slot_normalModeChanged(int)
	{
		if (_loadingDraft) return;
		storeUIToDraft();
		syncDirectionExtras();
	}

	void mPostSectionCutPanel::slot_reverseClicked(bool)
	{
		if (_loadingDraft) return;
		storeUIToDraft();
		validate();
	}

	void mPostSectionCutPanel::slot_stepDecrease()
	{
		moveOffset(-1);
	}

	void mPostSectionCutPanel::slot_stepIncrease()
	{
		moveOffset(1);
	}

	void mPostSectionCutPanel::slot_styleModeChanged(int)
	{
		if (_loadingDraft) return;
		storeUIToDraft();
		refreshStyle();
		validate();
	}

	void mPostSectionCutPanel::slot_fillToggled(bool)
	{
		if (_loadingDraft) return;
		storeUIToDraft();
		refreshStyle();
	}

	void mPostSectionCutPanel::slot_cropToggled(bool)
	{
		if (_loadingDraft) return;
		storeUIToDraft();
		refreshStyle();
	}

	void mPostSectionCutPanel::slot_colorSwatchClicked()
	{
		if (!_colorPicker)
		{
			_colorPicker = new FbdColorPicker(this);
			connect(_colorPicker, &FbdColorPicker::colorChanged, this, [this](const QColor& color)
				{
					working().color = color;
					applySwatchColor(_colorSwatch, color);
					_colorSwatch->setToolTip((_langEn ? QString("Section color ")
						: QString::fromUtf8("截面颜色 ")) + color.name().toUpper());
				});
		}
		_colorPicker->setCurrentColor(working().color);
		popupUnder(_colorPicker, _colorSwatch);
	}

	/******** 截面管理表 + 统一编辑动作（V1.3） ********/

	QList<const mPostSectionCutPanel::SectionRecord*> mPostSectionCutPanel::visibleRecords() const
	{
		//原型 visibleRecords：搜索过滤 + 名称排序（localeAwareCompare，numeric）
		const ModelSession& s = session();
		const QString query = _searchEdit ? _searchEdit->text().trimmed() : QString();
		QList<const SectionRecord*> result;
		for (const SectionRecord& rec : s.records)
			if (query.isEmpty() || rec.name.contains(query, Qt::CaseInsensitive))
				result.append(&rec);
		if (_sortOrder != 0)
		{
			std::stable_sort(result.begin(), result.end(), [this](const SectionRecord* a, const SectionRecord* b)
				{
					const int cmp = a->name.localeAwareCompare(b->name);
					return _sortOrder == 1 ? cmp < 0 : cmp > 0;
				});
		}
		return result;
	}

	void mPostSectionCutPanel::renderManagerTable()
	{
		_renderingTable = true;
		_managerTable->clearContents();
		_managerTable->setRowCount(0);

		const ModelSession& s = session();
		const bool hasCurrent = !s.currentId.isEmpty();
		const QList<const SectionRecord*> visible = visibleRecords();
		int currentRow = -1;
		for (const SectionRecord* rec : visible)
		{
			const int row = _managerTable->rowCount();
			_managerTable->insertRow(row);
			const bool isCurrent = (rec->id == s.currentId);
			if (isCurrent)
				currentRow = row;

			//复选列（13px 复选居中；勾选仅用于批量删除）
			QWidget* checkCell = new QWidget;
			QHBoxLayout* checkLayout = new QHBoxLayout(checkCell);
			checkLayout->setContentsMargins(0, 0, 0, 0);
			checkLayout->setAlignment(Qt::AlignCenter);
			QCheckBox* check = new QCheckBox(checkCell);
			check->setCursor(Qt::PointingHandCursor);
			check->setChecked(s.checkedIds.contains(rec->id));
			check->setStyleSheet("QCheckBox{background:transparent;}");
			checkLayout->addWidget(check);
			_managerTable->setCellWidget(row, 0, checkCell);
			connect(check, &QCheckBox::toggled, this, [this, id = rec->id](bool on)
				{
					if (_renderingTable) return;
					ModelSession& sess = session();
					if (on) sess.checkedIds.removeAll(id);
					else sess.checkedIds.removeAll(id);
					if (on) sess.checkedIds.append(id);
					refreshManagerState();
				});

			//名称列（只读文本；当前行蓝字）
			QTableWidgetItem* nameItem = new QTableWidgetItem(rec->name);
			nameItem->setFlags(Qt::ItemIsEnabled);
			nameItem->setData(Qt::UserRole, rec->id);
			nameItem->setToolTip(rec->name + (isCurrent && _langEn ? QString(" (editing)")
				: (isCurrent ? QString::fromUtf8("（当前编辑）") : QString())));
			if (isCurrent)
			{
				nameItem->setForeground(QColor(0x17, 0x6B, 0xA6));
				nameItem->setBackground(QColor(kSelectedBg));
			}
			_managerTable->setItem(row, 1, nameItem);

			//激活列（50×24 钮；多截面可同时激活；激活读已保存配置）
			QWidget* activeCell = new QWidget;
			QHBoxLayout* activeLayout = new QHBoxLayout(activeCell);
			activeLayout->setContentsMargins(0, 0, 0, 0);
			activeLayout->setAlignment(Qt::AlignCenter);
			const bool active = s.activeIds.contains(rec->id);
			QPushButton* activateBtn = new QPushButton(
				active ? (_langEn ? QString("Active") : QString::fromUtf8("已激活"))
					: (_langEn ? QString("Activate") : QString::fromUtf8("激活")), activeCell);
			activateBtn->setCheckable(true);
			activateBtn->setChecked(active);
			activateBtn->setFixedSize(50, kInner);
			activateBtn->setCursor(Qt::PointingHandCursor);
			activateBtn->setStyleSheet(QString(
				"QPushButton{border:1px solid %1;border-radius:4px;background:#FFFFFF;color:#303313;}"
				"QPushButton:hover{background:#F0F7FF;}")
				.arg(kCtrlBorder)
				+ QString("QPushButton:checked{background:%1;color:#176BA6;border-color:%2;}")
				.arg(kSelectedBg, kPrimary));
			activeLayout->addWidget(activateBtn);
			_managerTable->setCellWidget(row, 2, activeCell);
			connect(activateBtn, &QPushButton::toggled, this, [this, id = rec->id](bool on)
				{
					if (_renderingTable) return;
					setActive(id, on);
				});
		}

		if (visible.isEmpty())
		{
			const int row = _managerTable->rowCount();
			_managerTable->insertRow(row);
			QTableWidgetItem* emptyItem = new QTableWidgetItem(
				s.records.isEmpty() ? (_langEn ? QString("No sections yet") : QString::fromUtf8("暂无截面"))
					: (_langEn ? QString("No matching sections") : QString::fromUtf8("无匹配截面")));
			emptyItem->setFlags(Qt::ItemIsEnabled);
			emptyItem->setForeground(QColor(kMuted));
			emptyItem->setTextAlignment(Qt::AlignCenter);
			_managerTable->setItem(row, 0, emptyItem);
			_managerTable->setSpan(row, 0, 1, 3);
		}
		_renderingTable = false;
		refreshManagerState();
		if (currentRow >= 0)
			_managerTable->scrollToItem(_managerTable->item(currentRow, 1));
	}

	void mPostSectionCutPanel::refreshManagerState()
	{
		const ModelSession& s = session();
		const QList<const SectionRecord*> visible = visibleRecords();
		const bool all = !visible.isEmpty()
			&& std::all_of(visible.cbegin(), visible.cend(), [&s](const SectionRecord* r)
				{ return s.checkedIds.contains(r->id); });
		_selectAllBtn->setText(all ? QString::fromUtf8("取消") : QString::fromUtf8("全选"));
		_selectAllBtn->setEnabled(!visible.isEmpty());
		_deleteBtn->setEnabled(!s.checkedIds.isEmpty());
		const int hidden = static_cast<int>(std::count_if(s.checkedIds.cbegin(), s.checkedIds.cend(),
			[&visible](const QString& id)
			{ return std::none_of(visible.cbegin(), visible.cend(),
				[&id](const SectionRecord* r) { return r->id == id; }); }));
		_selectedCount->setText(hidden > 0
			? (_langEn ? QString("Selected %1 (%2 hidden by filter)").arg(s.checkedIds.size()).arg(hidden)
				: QString::fromUtf8("已选 %1 项（筛选外 %2）").arg(s.checkedIds.size()).arg(hidden))
			: (_langEn ? QString("Selected %1").arg(s.checkedIds.size())
				: QString::fromUtf8("已选 %1 项").arg(s.checkedIds.size())));
	}

	void mPostSectionCutPanel::refreshManagerStatus()
	{
		//原型 refreshStatus：当前记录存在且表单无效时显示问题文案
		const SectionRecord* rec = currentRecord();
		QString problem;
		if (rec)
		{
			problem = nameError();
			if (problem.isEmpty())
				problem = geometryError();
			if (problem.isEmpty() && !validate())
				problem = _langEn ? QString("Complete the valid parameters of this section first.")
					: QString::fromUtf8("请完善当前截面的有效参数。");
		}
		_managerStatus->setText(problem);
		_managerStatus->setVisible(!problem.isEmpty());
	}

	void mPostSectionCutPanel::beginCreate()
	{
		//原型 beginCreate：新建立即登记唯一 ID+默认名并选中激活、清搜索、回填、滚到行
		const auto action = [this]()
		{
			ModelSession& s = session();
			SectionRecord rec;
			rec.id = QStringLiteral("section-cut-") + QString::number(s.nextId);
			QStringList used;
			for (const SectionRecord& r : s.records) used.append(r.name);
			rec.name = postObjectUniqueName(
				QStringLiteral("Section_") + QString::number(s.nextId), used);
			s.nextId++;
			SectionCutDraft draft;
			applyDefaultPointTexts(draft);		//默认基点=当前模型包围盒中心
			rec.saved = rec.editorDraft = draft;
			rec.saved.name = rec.editorDraft.name = rec.name;
			const QString id = rec.id;
			s.records.append(rec);
			s.activeIds.insert(id);
			s.currentId = id;
			_editorHost->setVisible(true);
			_editorHost->setEnabled(true);
			_searchEdit->blockSignals(true);
			_searchEdit->clear();
			_searchEdit->blockSignals(false);
			loadDraftToUI(currentRecord()->editorDraft);
			renderManagerTable();
			_editor->beginEdit();
			refreshManagerStatus();
			_nameEdit->setFocus();
			//新建即激活：默认参数（X 轴法向、基点=模型中心）立即投影提交
			syncActiveSectionsToRender();
		};
		_editor->checkSwitch(action);
	}

	void mPostSectionCutPanel::selectRecord(const QString& id)
	{
		if (session().currentId == id)
			return;
		//原型 select：切换前 detach 当前编辑器，装载目标记录编辑缓冲（含未保存编辑）
		const auto action = [this, id]()
		{
			ModelSession& s = session();
			const auto it = std::find_if(s.records.begin(), s.records.end(),
				[&id](const SectionRecord& r) { return r.id == id; });
			if (it == s.records.end())
				return;
			endPick();
			s.currentId = id;
			loadDraftToUI(it->editorDraft);
			renderManagerTable();
			_editor->beginEdit();
			refreshManagerStatus();
		};
		_editor->checkSwitch(action);
	}

	void mPostSectionCutPanel::cancelEdit()
	{
		//原型 cancelEdit：恢复当前记录的已保存值
		SectionRecord* rec = currentRecord();
		if (!rec)
			return;
		endPick();
		rec->editorDraft = rec->saved;
		loadDraftToUI(rec->editorDraft);
		renderManagerTable();
		_editor->markSaved();
		refreshManagerStatus();
	}

	bool mPostSectionCutPanel::commitEdit(bool immediate)
	{
		SectionRecord* rec = currentRecord();
		if (!rec)
			return false;
		storeUIToDraft();	//确保编辑缓冲与 UI 一致
		SectionCutDraft& buf = working();

		//校验（原型 isValid：名称 + 几何/步长/偏移 + 样式带宽）
		const bool bandOk = buf.styleMode == 0
			|| (finiteText(buf.bandwidth) && buf.bandwidth.trimmed().toDouble() > 0);
		const QString nameProblem = nameError();
		if (!nameProblem.isEmpty() || !validate() || !bandOk)
		{
			refreshManagerStatus();
			if (!immediate && !nameProblem.isEmpty())
				_nameEdit->setFocus();
			return false;
		}

		buf.name = buf.name.trimmed();
		rec->name = buf.name;
		rec->saved = buf;
		if (!immediate)
		{
			//保存后回填归一（名称 trim）；即时保存路径不回填，保留焦点/光标
			rec->editorDraft = rec->saved;
			loadDraftToUI(rec->editorDraft);
		}
		renderManagerTable();
		_editor->markSaved();
		refreshManagerStatus();

		//激活态记录保存后按新参数重投影提交（未激活记录不触碰渲染）
		if (session().activeIds.contains(rec->id))
		{
			syncActiveSectionsToRender();
			//剖切范围限定（部件/局部）引擎暂按全模型剖切——保存即提示一次
			if (rec->saved.scope != 0)
				mGlobalSignals::getInstance()->outputMessageSig(1, _langEn
					? QString("Section cut scope (parts/elements) is currently applied to the whole model; scoped clipping arrives with the render engine update.")
					: QString::fromUtf8("剖切范围（部件/局部）当前按全模型剖切渲染，按范围剖切随渲染引擎更新接入。"),
					true);
		}
		return true;
	}

	void mPostSectionCutPanel::removeChecked()
	{
		//原型 removeSelected：删勾选（含搜索范围外）；当前被删→选首条；删空→收编辑器
		ModelSession& s = session();
		const bool removedCurrent = s.checkedIds.contains(s.currentId);
		for (int i = s.records.size() - 1; i >= 0; --i)
		{
			if (s.checkedIds.contains(s.records.at(i).id))
			{
				s.activeIds.remove(s.records.at(i).id);
				s.records.removeAt(i);
			}
		}
		if (s.checkedIds.isEmpty() && !removedCurrent)
			return;
		s.checkedIds.clear();
		if (removedCurrent)
			s.currentId = s.records.isEmpty() ? QString() : s.records.first().id;
		if (!s.records.isEmpty())
		{
			_editorHost->setEnabled(true);
			loadDraftToUI(currentRecord()->editorDraft);
			_editor->beginEdit();
		}
		else
		{
			//删空=回原型空态（与首开一致）：编辑器可见但禁用、展示默认参数
			_editor->endEdit();
			SectionCutDraft draft;
			applyDefaultPointTexts(draft);
			loadDraftToUI(draft);
			_editorHost->setEnabled(false);
			_managerStatus->setVisible(false);
		}
		_editorHost->setVisible(true);
		renderManagerTable();
		//被删记录可能处于激活态：按剩余激活集重投影（全删=清渲染）
		syncActiveSectionsToRender();
	}

	void mPostSectionCutPanel::setActive(const QString& id, bool on)
	{
		//原型 setActive：激活读已保存配置；编辑/勾选/激活三者独立
		ModelSession& s = session();
		const auto it = std::find_if(s.records.begin(), s.records.end(),
			[&id](const SectionRecord& r) { return r.id == id; });
		if (it == s.records.end())
			return;
		if (on) s.activeIds.insert(id);
		else s.activeIds.remove(id);
		renderManagerTable();
		syncActiveSectionsToRender();
	}

	void mPostSectionCutPanel::syncActiveSectionsToRender()
	{
		//无后处理视口（公共默认会话）时不渲染：会话保留在面板内存
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend == nullptr || rend->getPostRender() == nullptr)
			return;
		MechPreData::mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rend);
		if (guidata == nullptr)
			return;

		//激活记录的已保存配置 → 渲染投影（几何经 detail 折算；已保存记录必过校验，
		//折算失败仅防御性跳过该条）
		QList<MechPreData::SectionCutSpec> specs;
		const ModelSession& s = session();
		for (const SectionRecord& rec : s.records)
		{
			if (!s.activeIds.contains(rec.id))
				continue;
			QVector3D normal;
			QVector3D base;
			if (!detail::sectionCutNormal(rec.saved, normal)
				|| !detail::sectionCutBasePoint(rec.saved, base))
				continue;
			MechPreData::SectionCutSpec spec;
			spec._id = rec.id;
			spec._name = rec.name;
			spec._normal = normal;
			spec._base = base;
			spec._side = detail::effectiveKeepSide(rec.saved.side, rec.saved.reversed);
			spec._styleMode = rec.saved.styleMode;
			spec._bandwidth = rec.saved.bandwidth.trimmed().toDouble();
			spec._offset = rec.saved.offset.trimmed().toDouble();
			spec._configuration = rec.saved.configuration;
			spec._fill = rec.saved.fill;
			spec._colorMode = rec.saved.colorMode;
			spec._color = rec.saved.color;
			spec._crop = rec.saved.crop;
			spec._showPlane = rec.saved.showPlane;
			specs.append(spec);
		}
		guidata->_sectionCutSpecs = specs;
		applySectionCuts(rend, guidata);
	}

	void mPostSectionCutPanel::slot_searchChanged(const QString&)
	{
		renderManagerTable();
	}

	void mPostSectionCutPanel::slot_selectAllClicked()
	{
		//全选仅作用可见记录，隐藏勾选保留（原型 updateExplosionSelection 同口径）
		ModelSession& s = session();
		const QList<const SectionRecord*> visible = visibleRecords();
		const bool all = !visible.isEmpty()
			&& std::all_of(visible.cbegin(), visible.cend(), [&s](const SectionRecord* r)
				{ return s.checkedIds.contains(r->id); });
		for (const SectionRecord* rec : visible)
		{
			if (all) s.checkedIds.removeAll(rec->id);
			else if (!s.checkedIds.contains(rec->id))
				s.checkedIds.append(rec->id);
		}
		renderManagerTable();
	}

	void mPostSectionCutPanel::slot_deleteClicked()
	{
		removeChecked();
	}

	void mPostSectionCutPanel::slot_newClicked()
	{
		beginCreate();
	}

	void mPostSectionCutPanel::slot_headerClicked(int section)
	{
		//名称列排序：原序→升→降→原序（原型 CAEManagerSort；其余列不排序）
		if (section != 1)
			return;
		_sortOrder = (_sortOrder + 1) % 3;
		QHeaderView* header = _managerTable->horizontalHeader();
		if (_sortOrder == 0)
			header->setSortIndicator(-1, Qt::AscendingOrder);
		else
			header->setSortIndicator(1, _sortOrder == 1 ? Qt::AscendingOrder : Qt::DescendingOrder);
		renderManagerTable();
	}

	void mPostSectionCutPanel::slot_cellClicked(int row, int column)
	{
		//名称单元格点击=选中编辑（复选/激活钮有各自控件，不进此分支）
		QTableWidgetItem* item = _managerTable->item(row, column);
		if (!item || column != 1)
			return;
		const QString id = item->data(Qt::UserRole).toString();
		if (!id.isEmpty())
			selectRecord(id);
	}

	void mPostSectionCutPanel::applyLanguageTexts()
	{
		//语言刷新会重建 choice 选项并回设索引，其间触发的信号不得写回草稿
		//（否则空 UI 覆盖新草稿：名称/偏移被清空的 0.1 轮实证）
		_loadingDraft = true;

		//折叠组标题
		_managerFold->setTitle(_langEn ? QString("Section Management") : QString::fromUtf8("截面管理"));
		_definitionFold->setTitle(_langEn ? QString("Section Definition") : QString::fromUtf8("剖切定义"));
		_geometryFold->setTitle(_langEn ? QString("Section Geometry") : QString::fromUtf8("剖切几何定义"));
		_styleFold->setTitle(_langEn ? QString("Section Style") : QString::fromUtf8("截面样式"));

		//截面管理区文案
		_searchEdit->setPlaceholderText(_langEn ? QString("Search section name")
			: QString::fromUtf8("搜索截面名称"));
		_managerTable->setHorizontalHeaderLabels(QStringList()
			<< QString() << (_langEn ? QString("Name") : QString::fromUtf8("名称"))
			<< (_langEn ? QString("Active") : QString::fromUtf8("激活")));
		_deleteBtn->setToolTip(_langEn ? QString("Delete checked sections, including checked items outside the search scope.")
			: QString::fromUtf8("删除勾选截面，包含搜索范围外已勾选项"));
		_newBtn->setText(_langEn ? QString("New") : QString::fromUtf8("新建"));
		if (_editor)
			_editor->retranslate(_langEn);

		//全部行标签经 regLabel 显式登记刷新（findChildren 序号随嵌套结构漂移，弃用）
		for (const LabelText& lt : _labelTexts)
			lt.label->setText(_langEn ? QString::fromLatin1(lt.en) : QString::fromUtf8(lt.cn));

		//剖切范围下拉
		const int keepScope = _scopeCombo->currentIndex();
		_scopeCombo->blockSignals(true);
		_scopeCombo->clear();
		_scopeCombo->addItems(_langEn
			? QStringList{ QString("All"), QString("Parts"), QString("Elements") }
		: QStringList{ QString::fromUtf8("全部"), QString::fromUtf8("部件"), QString::fromUtf8("局部") });
		_scopeCombo->setCurrentIndex(keepScope < 0 ? 0 : keepScope);
		_scopeCombo->blockSignals(false);

		//截面名称/颜色提示（查看/清空钮提示由 refreshScopePick 按选择态维护）
		_nameEdit->setPlaceholderText(_langEn ? QString("Section_1") : QString("Section_1"));
		applySwatchColor(_colorSwatch, working().color);
		_colorSwatch->setToolTip((_langEn ? QString("Section color ") : QString::fromUtf8("截面颜色 "))
			+ working().color.name().toUpper());

		//基准剖切平面框标题
		_planeBox->setTitle(_langEn ? QString("Reference Plane") : QString::fromUtf8("基准剖切平面"));

		//平面法向下拉（L13 六模式）
		const int keepNormal = _normalCombo->currentIndex();
		_normalCombo->blockSignals(true);
		_normalCombo->clear();
		_normalCombo->addItems(_langEn
			? QStringList{ QString("X axis"), QString("Y axis"), QString("Z axis"),
				QString("Start-End"), QString("Pick object"), QString("Vector") }
		: QStringList{ QString::fromUtf8("X轴"), QString::fromUtf8("Y轴"), QString::fromUtf8("Z轴"),
			QString::fromUtf8("起点-终点"), QString::fromUtf8("选择对象"), QString::fromUtf8("向量") });
		_normalCombo->setCurrentIndex(keepNormal < 0 ? 0 : keepNormal);
		_normalCombo->blockSignals(false);
		_reverseBtn->setToolTip(_langEn ? QString("Reverse normal") : QString::fromUtf8("平面法向反向"));

		//点行应用钮文案
		_basePoint.apply->setText(_langEn ? QString("Apply") : QString::fromUtf8("应用"));
		for (int i = 0; i < 3; ++i)
			_threePoints[i].apply->setText(_langEn ? QString("Apply") : QString::fromUtf8("应用"));

		//定义方式/几何构型/保留侧 共框单选
		_definitionChoice->setOptions(_langEn
			? QStringList{ QString("Normal axis"), QString("3 points") }
		: QStringList{ QString::fromUtf8("法向轴"), QString::fromUtf8("三点平面") });
		_definitionChoice->setCurrentIndex(working().definition);
		_configChoice->setOptions(_langEn
			? QStringList{ QString("Fixed plane"), QString("Follow deformation") }
		: QStringList{ QString::fromUtf8("固定平面"), QString::fromUtf8("随变形移动") });
		_configChoice->setCurrentIndex(working().configuration);
		_sideChoice->setOptions(_langEn
			? QStringList{ QString("Negative"), QString("Both"), QString("Positive") }
		: QStringList{ QString::fromUtf8("负向侧"), QString::fromUtf8("两侧"), QString::fromUtf8("正向侧") });
		_sideChoice->setCurrentIndex(working().side);

		_fillCheck->setText(_langEn ? QString("Fill section") : QString::fromUtf8("填充截面"));

		_modeChoice->setOptions(_langEn
			? QStringList{ QString("Single plane"), QString("Band") }
		: QStringList{ QString::fromUtf8("单平面"), QString::fromUtf8("截面带") });
		_modeChoice->setCurrentIndex(working().styleMode);
		_colorModeChoice->setOptions(_langEn
			? QStringList{ QString("Result color"), QString("Uniform color") }
		: QStringList{ QString::fromUtf8("结果着色"), QString::fromUtf8("统一颜色") });
		_colorModeChoice->setCurrentIndex(working().colorMode);

		_cropBox->setTitle(_langEn ? QString("Cropping") : QString::fromUtf8("裁剪显示"));
		_cropCheck->setText(_langEn ? QString("Crop elements") : QString::fromUtf8("裁剪单元"));
		_ghostCheck->setText(_langEn ? QString("Ghost removed side") : QString::fromUtf8("剔除侧半透明"));
		_auxBox->setTitle(_langEn ? QString("Helpers") : QString::fromUtf8("辅助显示"));
		const char* auxCn[4] = { "剖切平面", "法向", "截面网格", "模型特征边" };
		const char* auxEn[4] = { "Section plane", "Normal", "Section mesh", "Feature edges" };
		for (int i = 0; i < 4; ++i)
			_auxChecks[i]->setText(_langEn ? QString::fromLatin1(auxEn[i]) : QString::fromUtf8(auxCn[i]));

		//动态文案随语言重刷
		refreshScopePick();
		refreshAllPointFields();
		validate();
		renderManagerTable();
		_loadingDraft = false;
	}

	void mPostSectionCutPanel::hideEvent(QHideEvent* event)
	{
		//面板退出=退出拾取武装态（宿主 setMainWidget 切换/复位时触发）
		endPick();
		mPanelBase::hideEvent(event);
	}
}
