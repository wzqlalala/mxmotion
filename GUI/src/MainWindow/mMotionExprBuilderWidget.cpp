#include "mMotionExprBuilderWidget.h"
#include "motion/functions/Function_Expr.h"

#include "mCurveWindow.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mMotionScenarioManager.h"
#include "mMotionScenario.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QTextCursor>

#include <vector>

namespace MechMainWindow
{
    namespace {
        // ---- 函数目录：分类 → 条目（insert 为插入模板，offset 为插入后光标回退数） ----
        struct FnEntry {
            const char* display;   ///< 下拉显示（签名）
            const char* insert;    ///< 插入模板
            int cursorBack;        ///< 插入后光标从末尾回退的字符数（落进括号内）
            const char* desc;      ///< 中文说明
        };
        struct FnCategory {
            const char* name;
            std::vector<FnEntry> entries;
        };
        const std::vector<FnCategory>& Catalog()
        {
            static const std::vector<FnCategory> catalog = {
                { "数学函数", {
                    { "sin(x)",      "sin()",      1, "正弦" },
                    { "cos(x)",      "cos()",      1, "余弦" },
                    { "tan(x)",      "tan()",      1, "正切" },
                    { "asin(x)",     "asin()",     1, "反正弦" },
                    { "acos(x)",     "acos()",     1, "反余弦" },
                    { "atan(x)",     "atan()",     1, "反正切" },
                    { "atan2(y, x)", "atan2(,)",   2, "反正切（双参）" },
                    { "sqrt(x)",     "sqrt()",     1, "平方根" },
                    { "abs(x)",      "abs()",      1, "绝对值" },
                    { "exp(x)",      "exp()",      1, "e 的 x 次幂" },
                    { "log(x)",      "log()",      1, "自然对数" },
                    { "log10(x)",    "log10()",    1, "常用对数" },
                    { "pow(a, b)",   "pow(,)",     2, "a 的 b 次幂" },
                    { "min(a, b)",   "min(,)",     2, "较小值" },
                    { "max(a, b)",   "max(,)",     2, "较大值" },
                    { "step(x,x0,h0,x1,h1)", "step(,,,,)", 5, "间歇函数：x<x0 为 h0，x>x1 为 h1，区间内三次平滑过渡（x0=x1 为硬跳变）" },
                } },
                { "变量与常数", {
                    { "t（时间）",   "t",    0, "自变量：时间（秒）" },
                    { "pi（圆周率）", "pi",  0, "3.14159..." },
                    { "x（变形量）", "x",    0, "弹簧/力上下文：变形量或相对位移（驱动表达式勿用）" },
                    { "v（变形速度）", "v",  0, "弹簧/力上下文：变形速度（驱动表达式勿用）" },
                } },
            };
            return catalog;
        }
    }

    mMotionExprBuilderWidget::mMotionExprBuilderWidget(QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* mainLay = new QVBoxLayout(this);
        mainLay->setContentsMargins(0, 0, 0, 0);
        mainLay->setSpacing(2);

        // ---- 表达式编辑区 ----
        m_edit = new QPlainTextEdit();
        m_edit->setPlaceholderText(QStringLiteral(
            "例如: 10*sin(2*pi*t)\n支持 + - * / ^ ( ) 与函数组合，t 为时间（秒）"));
        m_edit->setFixedHeight(64);
        mainLay->addWidget(m_edit);

        // ---- 校验状态 ----
        m_lblStatus = new QLabel(QStringLiteral(""));
        m_lblStatus->setWordWrap(true);
        mainLay->addWidget(m_lblStatus);

        // ---- 函数目录行 ----
        QHBoxLayout* catLay = new QHBoxLayout();
        catLay->setContentsMargins(0, 0, 0, 0);
        m_cmbCategory = new QComboBox();
        for (const auto& cat : Catalog())
            m_cmbCategory->addItem(QString::fromUtf8(cat.name));
        m_cmbFunction = new QComboBox();
        m_btnInsert = new QPushButton(QStringLiteral("插入"));
        m_btnInsert->setFixedWidth(48);
        catLay->addWidget(m_cmbCategory, 1);
        catLay->addWidget(m_cmbFunction, 2);
        catLay->addWidget(m_btnInsert);
        mainLay->addLayout(catLay);

        // ---- 运算符/变量快捷按钮 ----
        QHBoxLayout* opLay = new QHBoxLayout();
        opLay->setContentsMargins(0, 0, 0, 0);
        const QString ops[] = { "+", "-", "*", "/", "^", "(", ")", "," };
        for (const QString& op : ops) {
            QPushButton* btn = new QPushButton(op);
            btn->setFixedWidth(30);
            connect(btn, &QPushButton::clicked, this, [this, op]() {
                insertAtCursor(op, 0);
                });
            opLay->addWidget(btn);
        }
        opLay->addStretch();
        mainLay->addLayout(opLay);

        // ---- 预览 ----
        m_btnPreview = new QPushButton(QStringLiteral("预览曲线"));
        mainLay->addWidget(m_btnPreview);

        // ---- 信号 ----
        connect(m_cmbCategory, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { refreshFunctionList(); });
        connect(m_btnInsert, &QPushButton::clicked, this, [this]() {
            const int catIdx = m_cmbCategory->currentIndex();
            const int fnIdx = m_cmbFunction->currentIndex();
            if (catIdx < 0 || fnIdx < 0)
                return;
            const auto& entry = Catalog()[catIdx].entries[fnIdx];
            insertAtCursor(QString::fromUtf8(entry.insert), entry.cursorBack);
            });
        connect(m_btnPreview, &QPushButton::clicked, this, [this]() {
            const QString expr = expression();
            // 内置预览：按第一个求解方案的总时长采样（弹簧/力的 x/v 变量按 0 切片）
            motion::MotionFunction_Expr f(expr.toStdString());
            if (f.IsValid() && getCVM) {
                double T = 10.0;
                auto& scenarioMgr = getCVM->getModelData()
                    ->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario);
                const auto ids = scenarioMgr.getAllIDs();
                if (!ids.empty()) {
                    auto* scenario = scenarioMgr.getData(ids.front());
                    if (scenario && scenario->getEndTime() > 0.0)
                        T = scenario->getEndTime();
                }
                const int N = 400;
                QVector<double> xs, ys;
                xs.reserve(N);
                ys.reserve(N);
                for (int i = 0; i <= N; ++i) {
                    const double t = T * i / N;
                    xs.push_back(t);
                    ys.push_back(f.Evaluate(t, 0.0, 0.0));
                }
                auto* curveWindow = mCurveWindow::getInstance();
                curveWindow->setPlotTitle(QStringLiteral("表达式预览"));
                curveWindow->SetAxisTitle(QStringLiteral("时间 t (s)"), true);
                curveWindow->updateCurve(xs, ys, PlotView::LineChart, expr, true, true);
            }
            emit previewRequested(expr);
            });

        m_cmbCategory->setCurrentIndex(0);
        refreshFunctionList();

        connect(m_edit, &QPlainTextEdit::textChanged, this, &mMotionExprBuilderWidget::slotValidate);
    }

    QString mMotionExprBuilderWidget::expression() const
    {
        return m_edit->toPlainText().trimmed();
    }

    bool mMotionExprBuilderWidget::isValid() const
    {
        const QString text = expression();
        if (text.isEmpty())
            return false;
        motion::MotionFunction_Expr f(text.toStdString());
        return f.IsValid();
    }

    void mMotionExprBuilderWidget::setExpression(const QString& expr)
    {
        const QSignalBlocker b(m_edit);
        m_edit->setPlainText(expr);
        // 更新状态显示但不触发 expressionAccepted（程序回填不算用户编辑）
        const QString text = expr.trimmed();
        if (text.isEmpty()) {
            m_lblStatus->setStyleSheet("color:#FF4D4F;");
            m_lblStatus->setText(QStringLiteral("表达式为空"));
            return;
        }
        motion::MotionFunction_Expr f(text.toStdString());
        if (f.IsValid()) {
            m_lblStatus->setStyleSheet("color:#67C23A;");
            m_lblStatus->setText(QStringLiteral("语法正确"));
            m_lastAccepted = text;
        }
        else {
            m_lblStatus->setStyleSheet("color:#FF4D4F;");
            m_lblStatus->setText(QString::fromStdString(f.GetCompileError()));
        }
    }

    void mMotionExprBuilderWidget::slotValidate()
    {
        const QString text = expression();
        emit expressionChanged(text);

        if (text.isEmpty()) {
            m_lblStatus->setStyleSheet("color:#FF4D4F;");
            m_lblStatus->setText(QStringLiteral("表达式为空"));
            return;
        }
        motion::MotionFunction_Expr f(text.toStdString());
        if (f.IsValid()) {
            m_lblStatus->setStyleSheet("color:#67C23A;");
            m_lblStatus->setText(QStringLiteral("语法正确"));
            if (text != m_lastAccepted) {
                m_lastAccepted = text;
                emit expressionAccepted(text);
            }
        }
        else {
            m_lblStatus->setStyleSheet("color:#FF4D4F;");
            m_lblStatus->setText(QString::fromStdString(f.GetCompileError()));
        }
    }

    void mMotionExprBuilderWidget::insertAtCursor(const QString& text, int cursorBackOffset)
    {
        QTextCursor cursor = m_edit->textCursor();
        cursor.insertText(text);
        if (cursorBackOffset > 0)
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, cursorBackOffset);
        m_edit->setTextCursor(cursor);
        m_edit->setFocus();
    }

    void mMotionExprBuilderWidget::refreshFunctionList()
    {
        const QSignalBlocker b(m_cmbFunction);
        m_cmbFunction->clear();
        const int catIdx = m_cmbCategory->currentIndex();
        if (catIdx < 0 || catIdx >= static_cast<int>(Catalog().size()))
            return;
        for (const auto& entry : Catalog()[catIdx].entries)
            m_cmbFunction->addItem(QString::fromUtf8(entry.display));
        if (m_cmbFunction->count() > 0)
            m_cmbFunction->setCurrentIndex(0);
    }
}
