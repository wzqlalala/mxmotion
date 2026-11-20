#include "mCurveStatisticsDlg.h"
#include "CurveCore/CurveDocument.h"
#include "CurveCore/mCurveDocController.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleValidator>
#include <QFile>
#include <QFileDialog>
#include <QHash>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include "mFramelessMessageBox.h"
#include "mCommonFunctions.h"   //resolveBundledPythonExe：python 可执行文件回退探测
#include <QMessageBox>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtGlobal>
#include <cmath>

namespace PostCurve
{
    mCurveStatisticsDlg* mCurveStatisticsDlg::_instance = nullptr;

    namespace
    {
        //统计数值格式：'g'6 与曲线标签统计占位符 {max(y)} 的读数一致
        QString statNumber(double value)
        {
            return QString::number(value, 'g', 6);
        }

        //曲线形式单选钮的文案 -> 导出标题用类别名
        QString curveTypeTitle(const QRadioButton* radio)
        {
            return radio->text();
        }

        //单条超限频段（连续超限采样点合并；单点自成段 f1=f2）
        struct OverrunBand
        {
            double f1 = 0.0;        //段首频率
            double f2 = 0.0;        //段尾频率
            double peakFreq = 0.0;  //极值所在频率（同极值取先出现者）
            double peakValue = 0.0; //段内极值（低于模式=最小值，高于模式=最大值）
        };

        //单条实测曲线的超限统计（行式一项=一行；矩阵一项=一个(行,列)格子来源）
        struct CurveOverrun
        {
            QString label;              //行式=工况列；矩阵=行键（来源缺失退化为曲线名）
            QString columnKey;          //矩阵列键=对象号尾 token（行式为空）
            QList<OverrunBand> bands;   //超限频段（按采样点顺序）
        };

        //分量令牌的方向词："MAG|Z-Trans" -> "Z"、"REL|FX" -> "FX"（'|' 后取 '-' 前）
        QString directionWordOf(const QString& component)
        {
            return component.section(QLatin1Char('|'), 1)
                .section(QLatin1Char('-'), 0, 0).trimmed();
        }

        //对象文本尾 token："Point id 1008" -> "1008"（激励/响应点号）
        QString objectTailOf(const QString& object)
        {
            return object.trimmed().section(QLatin1Char(' '), -1);
        }

        //pch 子工况名尾部自带的激励描述括号段剥离（如 "Subcase 101 (1001:+X<>…:LHS:+X)"
        //-> "Subcase 101"）：工况列对齐需求图样例 "Subcase 1 (1008_Z)"（2026-11-02
        //真机导出审计收紧；激励由子工况号区分，无尾组/组内嵌套/剥后为空时保持原名）
        QString normalizeSubcase(const QString& subcase)
        {
            const QString s = subcase.trimmed();
            if (!s.endsWith(QLatin1Char(')')))
                return s;
            const int open = s.lastIndexOf(QLatin1Char('('));
            if (open <= 0 || s.indexOf(QLatin1Char('('), open + 1) >= 0)
                return s;
            const QString stripped = s.left(open).trimmed();
            return stripped.isEmpty() ? s : stripped;
        }

        //超限频段单元格：(f1-f2)-(f极值@极值)，数值 'g'6
        //（对齐设计稿样例 (159-167)-(160@59.9)：段内最差值及所在频率）
        QString bandCellText(const OverrunBand& b)
        {
            return QStringLiteral("(%1-%2)-(%3@%4)").arg(statNumber(b.f1),
                statNumber(b.f2), statNumber(b.peakFreq), statNumber(b.peakValue));
        }

        //跳过统计的实测曲线说明（导出成功提示附注；空=无跳过）
        QString skippedNoteText(int noTarget, int unknownForm)
        {
            QStringList parts;
            if (noTarget > 0)
                parts << QStringLiteral("%1 条实测曲线因无同图目标值曲线未参与统计").arg(noTarget);
            if (unknownForm > 0)
                parts << QStringLiteral("%1 条实测曲线因来源结果类型不可识别未参与统计").arg(unknownForm);
            return parts.join(QStringLiteral("；"));
        }
    }

    mCurveStatisticsDlg::mCurveStatisticsDlg(QWidget* parent)
        : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        buildUi();
        //设计图：小体量两行表单弹窗，尺寸固定 480x200（无可调语义，不做缩放手柄/几何记忆）
        setFixedSize(480, 200);

        //窗口细边框：白底弹窗加描边保证轮廓可辨识。须用 objectName 选择器：
        //本类在 PostCurve 命名空间内，QSS 裸类名选择器按 metaObject className
        //（PostCurve::mCurveStatisticsDlg）匹配不上（同 mApplyStyleDlg）
        setObjectName("curveStatisticsDlg");
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("QWidget#curveStatisticsDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");
    }

    mCurveStatisticsDlg::~mCurveStatisticsDlg()
    {
        if (_instance == this)
            _instance = nullptr;
    }

    void mCurveStatisticsDlg::showInstance(QWidget* parent)
    {
        //父窗口须为真正的顶层主窗口：调用方传的 window() 在停靠/MDI 环境里可能
        //是承载弹窗请求的某个小面板——沿父链走到顶层；顶层不是 QMainWindow 时
        //兜底取应用主窗口（同 mOpenCurveDlg/mApplyStyleDlg 的取父策略）
        QWidget* host = parent;
        while (host && host->parentWidget())
            host = host->parentWidget();
        if (!qobject_cast<QMainWindow*>(host))
        {
            const QWidgetList tops = QApplication::topLevelWidgets();
            for (QWidget* w : tops)
            {
                if (qobject_cast<QMainWindow*>(w))
                {
                    host = w;
                    break;
                }
            }
        }
        if (!host)
            host = parent;

        if (!_instance)
            _instance = new mCurveStatisticsDlg(host);
        //本次运行首开居中于父窗口（主窗口）：无边框窗口无系统定位，缺省落点随环境
        if (_instance->_needCenter)
        {
            _instance->_needCenter = false;
            if (QWidget* pw = _instance->parentWidget())
                _instance->move(pw->frameGeometry().center()
                    - QPoint(_instance->width() / 2, _instance->height() / 2));
        }
        if (_instance->isMinimized())
            _instance->showNormal();
        _instance->show();
        _instance->raise();
        _instance->activateWindow();
    }

    void mCurveStatisticsDlg::buildUi()
    {
        QVBoxLayout* outer = new QVBoxLayout(this);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);

        //标题栏（可拖动）：白底 + M徽标 + 深灰标题 + 深色关闭按钮（照 mApplyStyleDlg）
        _titleBar = new QWidget(this);
        _titleBar->setObjectName("curveStatisticsTitleBar");
        _titleBar->setFixedHeight(52);
        _titleBar->setStyleSheet("QWidget#curveStatisticsTitleBar{background:#FFFFFF;}");
        QHBoxLayout* titleLayout = new QHBoxLayout(_titleBar);
        titleLayout->setContentsMargins(20, 20, 20, 10);
        titleLayout->setSpacing(3);
        QLabel* logoLabel = new QLabel(_titleBar);
        logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
            .scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        QLabel* titleLabel = new QLabel(QStringLiteral("曲线统计"), _titleBar);
        titleLabel->setStyleSheet(
            "QLabel{color:#333333; font-size:13px; font-weight:600; background:transparent;}");
        QToolButton* closeBtn = new QToolButton(_titleBar);
        closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
        closeBtn->setIconSize(QSize(24, 24));
        closeBtn->setToolTip(QStringLiteral("关闭"));
        closeBtn->setStyleSheet(R"(
            QToolButton{
                background:transparent; border:none;
            }
            QToolButton:hover{ background:#EFF1F2; border-radius:2px; }
        )");
        //close() 而非 hide()：对齐既有功能弹窗的关闭语义（进程退出联动）
        connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);
        titleLayout->addWidget(logoLabel);
        titleLayout->addWidget(titleLabel, 1);
        titleLayout->addWidget(closeBtn);

        //主体：曲线形式行 + 频率范围行 + 底部按钮行
        QWidget* body = new QWidget(this);
        body->setStyleSheet("QWidget{background:#FFFFFF;}");
        QVBoxLayout* bodyLayout = new QVBoxLayout(body);
        //480x200 固定尺寸下的紧凑行距：标题栏52 + 上下边距28 + 两行控件/按钮
        //+ 行距16x2 后留少量余量，避免布局挤压
        bodyLayout->setContentsMargins(20, 12, 20, 16);
        bodyLayout->setSpacing(16);

        const QString rowLabelStyle =
            "QLabel{color:#333333; font-size:13px; background:transparent;}";

        //行1：曲线形式（设计图①：动刚度默认选中）
        QHBoxLayout* typeRow = new QHBoxLayout();
        typeRow->setSpacing(0);
        QLabel* typeLabel = new QLabel(QStringLiteral("曲线形式"), body);
        typeLabel->setStyleSheet(rowLabelStyle);
        typeLabel->setFixedWidth(76);
        typeRow->addWidget(typeLabel);
        _stiffnessRadio = new QRadioButton(QStringLiteral("动刚度"), body);
        _vtfRadio = new QRadioButton(QStringLiteral("VTF"), body);
        _ntfRadio = new QRadioButton(QStringLiteral("NTF"), body);
        _stiffnessRadio->setChecked(true);
        for (QRadioButton* radio : { _stiffnessRadio, _vtfRadio, _ntfRadio })
        {
            radio->setStyleSheet("QRadioButton{color:#333333; font-size:13px; background:transparent;}");
        }
        typeRow->addWidget(_stiffnessRadio);
        typeRow->addSpacing(40);
        typeRow->addWidget(_vtfRadio);
        typeRow->addSpacing(40);
        typeRow->addWidget(_ntfRadio);
        typeRow->addStretch(1);
        bodyLayout->addLayout(typeRow);

        //行2：频率范围（设计图②：两输入框以"—"相连，留空侧=不限制）
        QHBoxLayout* freqRow = new QHBoxLayout();
        freqRow->setSpacing(12);
        QLabel* freqLabel = new QLabel(QStringLiteral("频率范围"), body);
        freqLabel->setStyleSheet(rowLabelStyle);
        freqLabel->setFixedWidth(76);
        freqRow->addWidget(freqLabel);
        const auto freqValidator = [](QLineEdit* edit)
        {
            //非负频率即可（允许科学计数法）；留空由保存时按"不限制"处理
            auto* validator = new QDoubleValidator(0.0, 1e30, 8, edit);
            validator->setNotation(QDoubleValidator::ScientificNotation);
            edit->setValidator(validator);
        };
        _freqMinEdit = new QLineEdit(body);
        _freqMinEdit->setPlaceholderText(QStringLiteral("起始频率"));
        _freqMinEdit->setFixedSize(150, 28);
        _freqMinEdit->setStyleSheet(
            "QLineEdit{border:1px solid #C9CDD4; border-radius:4px; padding:2px 8px; background:#FFFFFF;}"
            "QLineEdit:focus{border:1px solid #0066CC;}");
        freqValidator(_freqMinEdit);
        QLabel* dashLabel = new QLabel(QStringLiteral("—"), body);
        dashLabel->setStyleSheet(rowLabelStyle);
        _freqMaxEdit = new QLineEdit(body);
        _freqMaxEdit->setPlaceholderText(QStringLiteral("终止频率"));
        _freqMaxEdit->setFixedSize(150, 28);
        _freqMaxEdit->setStyleSheet(_freqMinEdit->styleSheet());
        freqValidator(_freqMaxEdit);
        freqRow->addWidget(_freqMinEdit);
        freqRow->addWidget(dashLabel);
        freqRow->addWidget(_freqMaxEdit);
        freqRow->addStretch(1);
        bodyLayout->addLayout(freqRow);

        //底部按钮行：关闭（次要）/保存（蓝底主按钮，设计图③导出入口）
        QHBoxLayout* buttonRow = new QHBoxLayout();
        buttonRow->setSpacing(12);
        QPushButton* closeDlgBtn = new QPushButton(QStringLiteral("关闭"), body);
        QPushButton* saveBtn = new QPushButton(QStringLiteral("保存"), body);
        closeDlgBtn->setFixedSize(54, 26);
        saveBtn->setFixedSize(54, 26);
        closeDlgBtn->setStyleSheet(R"(
            QPushButton{
                background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;
                color:#333333; padding:2px 14px;
            }
            QPushButton:hover{ background:#EFF1F2; }
        )");
        saveBtn->setStyleSheet(R"(
            QPushButton{
                background:#0066CC; border:1px solid #0066CC; border-radius:4px;
                color:#FFFFFF; padding:2px 14px;
            }
            QPushButton:hover{ background:#0073E0; }
        )");
        connect(closeDlgBtn, &QPushButton::clicked, this, &QWidget::close);
        connect(saveBtn, &QPushButton::clicked, this, &mCurveStatisticsDlg::onSave);
        buttonRow->addStretch(1);
        buttonRow->addWidget(closeDlgBtn);
        buttonRow->addWidget(saveBtn);
        bodyLayout->addLayout(buttonRow);

        outer->addWidget(_titleBar);
        outer->addWidget(body, 1);
    }

    bool mCurveStatisticsDlg::parseFreqRange(double* freqMin, double* freqMax, bool* hasAny)
    {
        const QString minText = _freqMinEdit->text().trimmed();
        const QString maxText = _freqMaxEdit->text().trimmed();
        const bool hasMin = !minText.isEmpty();
        const bool hasMax = !maxText.isEmpty();
        *hasAny = hasMin || hasMax;

        bool minOk = true;
        bool maxOk = true;
        *freqMin = hasMin ? minText.toDouble(&minOk) : 0.0;
        *freqMax = hasMax ? maxText.toDouble(&maxOk) : 0.0;
        if ((hasMin && !minOk) || (hasMax && !maxOk))
        {
            MechMainWindow::mFramelessMessageBox::warning(this, QStringLiteral("提示"),
                QStringLiteral("频率范围输入无效，请输入数字（可带科学计数法）。"));
            return false;
        }
        if (hasMin && hasMax && *freqMin > *freqMax)
        {
            MechMainWindow::mFramelessMessageBox::warning(this, QStringLiteral("提示"),
                QStringLiteral("频率范围下限不能大于上限，请重新输入。"));
            return false;
        }
        return true;
    }

    bool mCurveStatisticsDlg::collectStatistics(StatsExportTable* ctx)
    {
        double freqMin = 0.0;
        double freqMax = 0.0;
        bool hasRange = false;
        if (!parseFreqRange(&freqMin, &freqMax, &hasRange))
            return false;
        //留空侧不限制：单侧限定按输入框有无文本判定（parseFreqRange 已保证数字合法）
        const bool hasMinLimit = !_freqMinEdit->text().trimmed().isEmpty();
        const bool hasMaxLimit = !_freqMaxEdit->text().trimmed().isEmpty();

        //统计对象 = "曲线形式"所选类别的实测曲线（动刚度/VTF/NTF，生成功能经
        //setCurveCategory 打标；目标值曲线为 TargetCurve，普通导入/定义曲线与
        //升级前生成的旧曲线（无标记）不在统计范围）
        const CurveCategory wantedKind = _vtfRadio->isChecked() ? CurveCategory::VTF
            : (_ntfRadio->isChecked() ? CurveCategory::NTF : CurveCategory::DynamicStiffness);
        const QString wantedName = curveTypeTitle(_vtfRadio->isChecked() ? _vtfRadio
            : (_ntfRadio->isChecked() ? _ntfRadio : _stiffnessRadio));
        //布局：动刚度=行式（设计稿01-03，一行一条实测曲线）；VTF/NTF=矩阵
        //（设计稿04-05，行=工况+(方向)，列=对象号）
        const bool rowsLayout = (wantedKind == CurveCategory::DynamicStiffness);

        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller->document();
        QList<CurveOverrun> stats;
        int skippedNoTarget = 0;      //同图无目标值曲线跳过
        int skippedUnknownForm = 0;   //来源结果类型不可识别跳过
        double actualMinFreq = 0.0;   //参与统计点的实际频率范围（标题留空侧取用）
        double actualMaxFreq = 0.0;
        bool hasActual = false;

        for (int pageId : doc->childNodeIds(doc->rootNodeId()))
        {
            if (doc->nodeType(pageId) != CurveNodeType::Page)
                continue;
            for (int chartId : doc->childNodeIds(pageId))
            {
                if (doc->nodeType(chartId) != CurveNodeType::Chart)
                    continue;

                //同图目标值曲线配对（一条实测配一条目标；多条目标取第一条并告警）
                const CurveSeriesData* target = nullptr;
                bool extraTarget = false;
                for (int childId : doc->childNodeIds(chartId))
                {
                    if (doc->nodeType(childId) != CurveNodeType::Curve)
                        continue;
                    const CurveSeriesData& s = doc->node(childId)->series;
                    if (s.curveCategory != CurveCategory::TargetCurve)
                        continue;
                    if (!target)
                        target = &s;
                    else
                        extraTarget = true;
                }
                if (extraTarget)
                    controller->reportWarning(QStringLiteral("[曲线] 曲线统计：图表“%1”内存在多条目标值曲线，统计取第一条")
                        .arg(doc->nodeDisplayName(chartId)));

                for (int curveId : doc->childNodeIds(chartId))
                {
                    if (doc->nodeType(curveId) != CurveNodeType::Curve)
                        continue;
                    const CurveSeriesData& series = doc->node(curveId)->series;
                    //类别不符/隐藏曲线不统计；柱状曲线无横坐标数据，不在频率
                    //统计语义内，一并跳过
                    if (series.curveCategory != wantedKind
                        || !series.visible
                        || series.xData.isEmpty() || series.yData.isEmpty())
                        continue;

                    //超限方向自来源结果类型判定（schema=PchReader::toString 同源
                    //文本）：动刚度+位移形式（DISPLACEMENTS）低于目标为超限、段内
                    //取最小值；其余（速度 VELOCITIES/加速度 ACCELERATIONS、VTF/NTF）
                    //高于目标为超限、段内取最大值。schema 缺失/不可识别跳过并计数
                    bool belowTarget = false;
                    if (wantedKind == CurveCategory::DynamicStiffness)
                    {
                        const QString schemaUpper = series.schema.toUpper();
                        if (schemaUpper.contains(QStringLiteral("DISPLACEMENT")))
                            belowTarget = true;
                        else if (schemaUpper.contains(QStringLiteral("VELOCITY"))
                            || schemaUpper.contains(QStringLiteral("ACCELERATION")))
                            belowTarget = false;
                        else
                        {
                            ++skippedUnknownForm;
                            continue;
                        }
                    }

                    if (!target)
                    {
                        ++skippedNoTarget;
                        continue;
                    }

                    //目标曲线参与插值的采样：剔除隐藏区间与非有限点（pch 频率
                    //升序，过滤子集仍升序）；实测点频率在目标范围外不插值
                    QVector<double> tx;
                    QVector<double> ty;
                    tx.reserve(qMin(target->xData.size(), target->yData.size()));
                    ty.reserve(tx.capacity());
                    for (int i = 0; i < target->xData.size() && i < target->yData.size(); ++i)
                    {
                        if (target->isPointHidden(i))
                            continue;
                        const double x = target->xData.at(i);
                        const double y = target->yData.at(i);
                        if (!qIsFinite(x) || !qIsFinite(y))
                            continue;
                        tx.append(x);
                        ty.append(y);
                    }
                    //目标取值：按实测点频率在目标 X 上线性插值（目标 X 升序；
                    //区间外跳过）。参考语义：设计稿样例 (159-167)-(160@59.9)
                    auto targetValueAt = [&tx, &ty](double x, double* out) -> bool
                    {
                        if (tx.isEmpty() || x < tx.first() || x > tx.last())
                            return false;
                        int lo = 0;
                        int hi = tx.size() - 1;
                        while (hi - lo > 1)
                        {
                            const int mid = (lo + hi) / 2;
                            if (tx.at(mid) <= x)
                                lo = mid;
                            else
                                hi = mid;
                        }
                        const double x0 = tx.at(lo);
                        const double x1 = tx.at(hi);
                        const double y0 = ty.at(lo);
                        const double y1 = ty.at(hi);
                        if (x1 <= x0)   //单点/重合频率：取节点值
                        {
                            *out = y0;
                            return true;
                        }
                        *out = y0 + (y1 - y0) * (x - x0) / (x1 - x0);
                        return true;
                    };

                    //工况列拼装（来源五元组，创建时 setSeriesSource 写入）：
                    //行式 = 工况 (对象号_方向)，如 "Subcase 1 (1008_Z)"；
                    //矩阵 = 行键 工况 (方向)（设计稿行内激励点号来源中不存在，
                    //按现有数据能力退化）+ 列键 对象号。来源缺失退化为曲线名
                    CurveOverrun stat;
                    stat.label = series.name;
                    stat.columnKey = series.name;
                    const QString dirWord = directionWordOf(series.component);
                    const QString objTail = objectTailOf(series.object);
                    const QString subcaseLabel = normalizeSubcase(series.subcase);
                    if (rowsLayout)
                    {
                        if (!series.subcase.isEmpty() && !objTail.isEmpty() && !dirWord.isEmpty())
                            stat.label = QStringLiteral("%1 (%2_%3)")
                                .arg(subcaseLabel, objTail, dirWord);
                    }
                    else
                    {
                        if (!series.subcase.isEmpty() && !dirWord.isEmpty())
                            stat.label = QStringLiteral("%1 (%2)").arg(subcaseLabel, dirWord);
                        if (!objTail.isEmpty())
                            stat.columnKey = objTail;
                    }

                    const int n = qMin(series.xData.size(), series.yData.size());
                    bool inBand = false;
                    OverrunBand cur;
                    int participated = 0;
                    auto closeBand = [&inBand, &cur, &stat]()
                    {
                        if (!inBand)
                            return;
                        inBand = false;
                        stat.bands.append(cur);
                    };
                    for (int i = 0; i < n; ++i)
                    {
                        //非参与点（隐藏区间/非有限/范围外/目标插值失败）与不超限
                        //点一样断段：频段只由连续超限的参与点构成
                        if (series.isPointHidden(i))
                        {
                            closeBand();
                            continue;
                        }
                        const double x = series.xData.at(i);
                        const double y = series.yData.at(i);
                        if (!qIsFinite(x) || !qIsFinite(y))
                        {
                            closeBand();
                            continue;
                        }
                        if ((hasMinLimit && x < freqMin) || (hasMaxLimit && x > freqMax))
                        {
                            closeBand();
                            continue;
                        }
                        double t = 0.0;
                        if (!targetValueAt(x, &t))
                        {
                            closeBand();
                            continue;
                        }
                        ++participated;
                        if (!hasActual || x < actualMinFreq)
                            actualMinFreq = x;
                        if (!hasActual || x > actualMaxFreq)
                            actualMaxFreq = x;
                        hasActual = true;
                        //超限判定：低于模式 y<目标、高于模式 y>目标（相等不超限）；
                        //连续超限点合并为一段，单点自成段（f1=f2）
                        const bool exceed = belowTarget ? (y < t) : (y > t);
                        if (!exceed)
                        {
                            closeBand();
                            continue;
                        }
                        if (!inBand)
                        {
                            inBand = true;
                            cur = OverrunBand{ x, x, x, y };
                        }
                        else
                        {
                            cur.f2 = x;
                            //同极值取先出现者：更新用严格不等号
                            if (belowTarget ? (y < cur.peakValue) : (y > cur.peakValue))
                            {
                                cur.peakValue = y;
                                cur.peakFreq = x;
                            }
                        }
                    }
                    closeBand();
                    if (participated == 0)
                        continue;   //频率范围内无参与点，不占统计行（对齐原口径）
                    stats.append(stat);
                }
            }
        }

        if (stats.isEmpty())
        {
            //打标缺失属预期场景（旧曲线无标记），提示带生成指引后不导出
            QString msg = QStringLiteral("当前文档没有“%1”类型的可统计曲线。\n"
                "请先用“动刚度曲线/VTF曲线/NTF曲线”按钮生成带标记的曲线再统计"
                "（升级前生成的旧曲线无类别标记，需重新生成一次）。").arg(wantedName);
            const QString skipNote = skippedNoteText(skippedNoTarget, skippedUnknownForm);
            if (!skipNote.isEmpty())
                msg += QStringLiteral("\n") + skipNote + QStringLiteral("。");
            MechMainWindow::mFramelessMessageBox::information(this, QStringLiteral("提示"), msg);
            return false;
        }

        //标题（仅行式，矩阵无标题行）：<频率下限>-<频率上限>Hz<形式名>统计结果
        //（如 50-1000Hz动刚度统计结果）；留空侧用参与统计数据的实际最小/最大
        //频率，全空时同样取实际范围
        const QString lowText = hasMinLimit ? _freqMinEdit->text().trimmed()
                                            : statNumber(actualMinFreq);
        const QString highText = hasMaxLimit ? _freqMaxEdit->text().trimmed()
                                             : statNumber(actualMaxFreq);
        ctx->title = rowsLayout
            ? QStringLiteral("%1-%2Hz%3统计结果").arg(lowText, highText, wantedName)
            : QString();
        ctx->corner = QStringLiteral("工况");
        ctx->layout = rowsLayout ? QStringLiteral("rows") : QStringLiteral("matrix");
        ctx->note = skippedNoteText(skippedNoTarget, skippedUnknownForm);

        //频段个数 N = 明细表列数（不含首列角标）：行式=最长行频段格数，
        //矩阵=各组跨列数之和（分布局分支内赋值，避免双份口径漂移）
        int detailColCount = 0;

        if (rowsLayout)
        {
            //行式（设计稿01-03）：数据行 = [工况, 段1, 段2, ...]（列数=最大段数+1），
            //无列头行/汇总列——角标与标题合并行由脚本排版；无超限的曲线保留空行
            int maxSegs = 0;
            for (const CurveOverrun& s : stats)
                maxSegs = qMax(maxSegs, s.bands.size());
            detailColCount = maxSegs;
            for (const CurveOverrun& s : stats)
            {
                QStringList row;
                row << s.label;
                for (const OverrunBand& b : s.bands)
                    row << bandCellText(b);
                while (row.size() < 1 + maxSegs)
                    row << QString();
                ctx->rows.append(row);
            }
        }
        else
        {
            //矩阵（设计稿04-05）：行=工况+(方向)，列=对象号分组合并（组宽=该组
            //各行最大频段数，下取 1；行/列均按文档遍历序首次出现定序）；频段按
            //组内子列从左排布、空缺留白，无汇总列
            QStringList rowKeys;
            QStringList colKeys;
            QHash<QString, int> rowIndexOf;
            QHash<QString, int> colIndexOf;
            QList<QList<QStringList>> cellParts;   //[行][列] = 频段串列表
            auto ensureKey = [](const QString& key, QStringList* keys, QHash<QString, int>* index)
            {
                auto it = index->find(key);
                if (it != index->end())
                    return *it;
                const int idx = keys->size();
                index->insert(key, idx);
                keys->append(key);
                return idx;
            };
            for (const CurveOverrun& s : stats)
            {
                const int r = ensureKey(s.label, &rowKeys, &rowIndexOf);
                const int c = ensureKey(s.columnKey, &colKeys, &colIndexOf);
                while (cellParts.size() <= r)
                    cellParts.append(QList<QStringList>());
                while (cellParts[r].size() <= c)
                    cellParts[r].append(QStringList());
                for (const OverrunBand& b : s.bands)
                    cellParts[r][c].append(bandCellText(b));
            }

            //组宽 = 该组各行最大频段数
            QList<int> spans;
            for (int c = 0; c < colKeys.size(); ++c)
            {
                int span = 1;
                for (int r = 0; r < rowKeys.size(); ++r)
                {
                    if (r < cellParts.size() && c < cellParts.at(r).size())
                        span = qMax(span, cellParts.at(r).at(c).size());
                }
                spans.append(span);
            }
            ctx->groups = colKeys;
            ctx->spans = spans;
            for (int span : spans)
                detailColCount += span;

            for (int r = 0; r < rowKeys.size(); ++r)
            {
                QStringList row;
                row << rowKeys.at(r);
                for (int c = 0; c < colKeys.size(); ++c)
                {
                    const QStringList parts = (r < cellParts.size() && c < cellParts.at(r).size())
                        ? cellParts.at(r).at(c) : QStringList();
                    for (int j = 0; j < spans.at(c); ++j)
                        row << (j < parts.size() ? parts.at(j) : QString());
                }
                ctx->rows.append(row);
            }
        }

        //频段统计表（第二张表，CurveStatsBand 装配复用已收集频段不二次遍历）：
        //频率范围两端均填写才装配；行标签与明细同行一致（矩阵同明细行键的
        //曲线由装配合并为一条频段行，行数/行序/标签三表一致——2026-11-04
        //真机口径）。留空侧跳过原因在此补，起止相等/范围无效由装配返回
        if (hasMinLimit && hasMaxLimit)
        {
            QList<CurveStatsBand::CurveBands> bandCurves;
            bandCurves.reserve(stats.size());
            for (const CurveOverrun& s : stats)
            {
                CurveStatsBand::CurveBands bc;
                bc.label = s.label;
                bc.columnKey = rowsLayout ? QString() : s.columnKey;
                bc.bandStarts.reserve(s.bands.size());
                for (const OverrunBand& b : s.bands)
                    bc.bandStarts.append(b.f1);
                bandCurves.append(bc);
            }
            //2026-11-04 口径修正：矩阵每组单独分段（组段宽=范围/组列数，曲线
            //按 columnKey 归组、计数只落本组槽位）；行式无组=单组全局等宽
            ctx->bandStats = CurveStatsBand::assembleBandStats(
                bandCurves, freqMin, freqMax,
                rowsLayout ? QStringList() : ctx->groups,
                rowsLayout ? QList<int>{ detailColCount } : ctx->spans);
        }
        else
        {
            ctx->bandStats = CurveStatsBand::BandStatsResult();
            ctx->bandStats.skipReason = QStringLiteral("频率范围未填全，未生成频段统计表");
        }
        if (!ctx->bandStats.generated)
        {
            //跳过原因并入导出成功提示附注（既有跳过曲线说明之后，；分隔）
            if (!ctx->note.isEmpty())
                ctx->note += QStringLiteral("；");
            ctx->note += ctx->bandStats.skipReason;
        }
        return true;
    }

    bool mCurveStatisticsDlg::exportStatistics(const QString& outputPath,
        const StatsExportTable& ctx, QString* err)
    {
        QString tempDir = QDir::temp().absoluteFilePath("mx_stats_export_tmp");
        QDir(tempDir).mkpath(".");

        //统计上下文 -> JSON（图面排版契约见 resource/scripts/export_stats.py 头
        //注释；形如数字的单元格由脚本按数值写出，其余为文本）
        QJsonObject ctxJson;
        ctxJson.insert("title", ctx.title);
        ctxJson.insert("layout", ctx.layout);
        ctxJson.insert("corner", ctx.corner);
        QJsonArray groupsJson;
        for (const QString& group : ctx.groups)
            groupsJson.append(group);
        ctxJson.insert("groups", groupsJson);
        QJsonArray spansJson;
        for (int span : ctx.spans)
            spansJson.append(span);
        ctxJson.insert("spans", spansJson);
        QJsonArray rowsJson;
        for (const QStringList& row : ctx.rows)
        {
            QJsonArray rowJson;
            for (const QString& cell : row)
                rowJson.append(cell);
            rowsJson.append(rowJson);
        }
        ctxJson.insert("rows", rowsJson);

        //频段统计表上下文（第二张表）：generated=false 不写 band_stats 键，
        //脚本按键缺席不加 Sheet/页（契约见 CurveStatsBand::bandStatsToJson）
        const QJsonObject bandJson = CurveStatsBand::bandStatsToJson(ctx.bandStats);
        if (!bandJson.isEmpty())
            ctxJson.insert("band_stats", bandJson);

        QString jsonPath = QDir(tempDir).absoluteFilePath("stats_ctx.json");
        QFile jsonFile(jsonPath);
        if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            *err = QStringLiteral("统计上下文写入失败: %1").arg(jsonPath);
            return false;
        }
        jsonFile.write(QJsonDocument(ctxJson).toJson());
        jsonFile.close();

        //生成前先清理已存在的同名文件，避免旧文件残留导致误判导出成功
        if (QFile::exists(outputPath) && !QFile::remove(outputPath))
        {
            *err = QStringLiteral("无法覆盖已存在的文件: %1（文件可能正被打开）").arg(outputPath);
            return false;
        }

        //python 可执行文件解析带回退：Debug 部署的 Conan python 包只有 python_d.exe，
        //依次探测 python.exe/python_d.exe 取第一个存在的——与报告导出 mPostPptReportRenderer 同一机制
        QStringList probedPythons;
        QString pythonExe = PostCommon::mCommonFunctions::resolveBundledPythonExe(&probedPythons);
        if (pythonExe.isEmpty())
        {
            *err = QStringLiteral("找不到可用的 python 可执行文件（已探测: %1）")
                       .arg(probedPythons.join(QStringLiteral("; ")));
            return false;
        }

        //脚本定位与释放：优先程序目录磁盘副本，缺脚本时从 qrc 释放到临时目录
        //（python 进程需要真实磁盘文件）。临时副本每次删除重释放：qrc 内嵌脚本
        //随构建更新，复用旧副本会导致升级程序后仍按旧脚本导出。注意 qrc 释放出
        //的副本继承资源的只读属性，Windows 上 QFile::remove 删不动只读文件，
        //须先清写权限再删（2026-11-02 真机复现：只读残留副本致释放恒败）
        QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts/export_stats.py";
        if (!QFile::exists(scriptPath))
        {
            QString scriptTmp = QDir(tempDir).absoluteFilePath("export_stats.py");
            QFile tmpFile(scriptTmp);
            if (tmpFile.exists())
            {
                tmpFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                    | QFile::ReadUser | QFile::WriteUser
                    | QFile::ReadGroup | QFile::WriteGroup
                    | QFile::ReadOther | QFile::WriteOther);
            }
            QFile::remove(scriptTmp);
            if (!QFile::copy(":/mMainWidgets/scripts/export_stats.py", scriptTmp))
            {
                *err = QStringLiteral("曲线统计导出脚本缺失: %1 且 qrc 释放失败").arg(scriptPath);
                return false;
            }
            scriptPath = scriptTmp;
        }

        QProcess process;
        process.start(pythonExe, QStringList{ scriptPath, jsonPath, outputPath });
        if (!process.waitForStarted())
        {
            *err = QStringLiteral("无法启动导出进程（python: %1）").arg(pythonExe);
            return false;
        }
        if (!process.waitForFinished(-1))
        {
            *err = QStringLiteral("导出进程异常: %1").arg(process.errorString());
            return false;
        }
        const QString stderrOutput = QString::fromLocal8Bit(process.readAllStandardError());
        if (process.exitCode() != 0 || !stderrOutput.isEmpty())
        {
            *err = QStringLiteral("导出失败: %1").arg(stderrOutput);
            return false;
        }
        if (!QFile::exists(outputPath))
        {
            *err = QStringLiteral("导出文件未生成: %1").arg(outputPath);
            return false;
        }
        return true;
    }

    void mCurveStatisticsDlg::onSave()
    {
        //前置：频率范围合法 + 有可统计数据（无数据时内部弹提示）
        StatsExportTable ctx;
        if (!collectStatistics(&ctx))
            return;

        //设计图③：弹出对话框选择保存路径、名称、格式（excel/ppt）
        QString selectedFilter;
        const QString filePath = QFileDialog::getSaveFileName(this,
            QStringLiteral("保存曲线统计"),
            QDir::homePath() + "/" + QStringLiteral("曲线统计"),
            QStringLiteral("Excel 工作簿 (*.xlsx);;PowerPoint 演示文稿 (*.pptx)"),
            &selectedFilter);
        if (filePath.isEmpty())
            return;
        //getSaveFileName 不自动补后缀：按所选过滤器补默认后缀
        const QString suffix = selectedFilter.contains("pptx") ? QStringLiteral(".pptx")
                                                               : QStringLiteral(".xlsx");
        QString outputPath = filePath;
        if (!outputPath.endsWith(suffix, Qt::CaseInsensitive))
            outputPath += suffix;

        QString err;
        if (exportStatistics(outputPath, ctx, &err))
        {
            //导出成功提示附注跳过统计的实测曲线说明（无目标/形式不可识别）
            QString msg = QStringLiteral("统计结果已保存到:\n%1").arg(outputPath);
            if (!ctx.note.isEmpty())
                msg += QStringLiteral("\n\n") + ctx.note + QStringLiteral("。");
            MechMainWindow::mFramelessMessageBox::information(this, QStringLiteral("曲线统计"), msg);
        }
        else
        {
            MechMainWindow::mFramelessMessageBox::warning(this, QStringLiteral("曲线统计"), err);
        }
    }

    void mCurveStatisticsDlg::mousePressEvent(QMouseEvent* event)
    {
        //仅记录按下点：拖动延迟到 mouseMoveEvent 中移动超过系统拖动阈值后才
        //真正开始（双击微动不再挪动弹窗）
        if (event->button() == Qt::LeftButton && _titleBar->underMouse())
            _pressPos = event->globalPos();
        QWidget::mousePressEvent(event);
    }

    void mCurveStatisticsDlg::mouseMoveEvent(QMouseEvent* event)
    {
        //只有用户拖动才改变位置：左键按住且自按下点（双击则自双击点）移动超过
        //系统拖动阈值才进入拖动并固定偏移，此后跟随手部移动
        if ((event->buttons() & Qt::LeftButton) && !_dragging
            && _titleBar->underMouse()
            && (event->globalPos() - _pressPos).manhattanLength()
                >= QApplication::startDragDistance())
        {
            _dragging = true;
            _dragOffset = event->globalPos() - frameGeometry().topLeft();
        }
        if (_dragging && (event->buttons() & Qt::LeftButton))
            move(event->globalPos() - _dragOffset);
        QWidget::mouseMoveEvent(event);
    }

    void mCurveStatisticsDlg::mouseReleaseEvent(QMouseEvent* event)
    {
        _dragging = false;   //松手结束拖动（双击的第二击不延续上一次拖动态）
        QWidget::mouseReleaseEvent(event);
    }

    void mCurveStatisticsDlg::mouseDoubleClickEvent(QMouseEvent* event)
    {
        //双击不进入拖动链路：默认实现会转发 mousePressEvent 重新置位拖动状态，
        //配合手部微动会挪动弹窗；此处仅刷新阈值起算点
        if (event->button() == Qt::LeftButton)
            _pressPos = event->globalPos();
    }
}
