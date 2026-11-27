#include "NativeFunctionEditor.h"
#include "compat/UiMetrics.h"

#include "motion/functions/Function_Expr.h"
#include "motion/functions/Function_Table.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QTextCursor>
#include <QTextCodec>
#include <QVBoxLayout>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_text.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <memory>
#include <stdexcept>
#include <vector>

namespace MotionOcc {
namespace {
constexpr int maximumTableRows = 100000;
[[noreturn]] void fail(const QString& message) { throw std::runtime_error(message.toStdString()); }

double finiteNumber(const QString& text, const QString& context)
{
    bool ok = false;
    const double value = text.toDouble(&ok);
    if (!ok || !std::isfinite(value)) fail(context + QStringLiteral("：请输入有限数值。"));
    return value;
}

std::vector<double> tableNumbers(const QJsonArray& values, const QString& name)
{
    if (values.size() > maximumTableRows) fail(QStringLiteral("表格最多支持 100000 行。"));
    std::vector<double> result;
    result.reserve(values.size());
    for (int row = 0; row < values.size(); ++row) {
        if (!values[row].isDouble() || !std::isfinite(values[row].toDouble()))
            fail(QStringLiteral("%1第 %2 行不是有限数值。").arg(name).arg(row + 1));
        result.push_back(values[row].toDouble());
    }
    return result;
}
}

bool readNativeFunctionTable(const QString& path,QJsonArray& tableX,QJsonArray& tableY,QString& error)
{
    try {
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly))fail(QStringLiteral("无法读取表格：%1").arg(file.errorString()));
        if(file.size()>32*1024*1024)fail(QStringLiteral("表格文件超过 32 MiB，请减少文件大小。"));
        const auto bytes=file.readAll();
        QTextCodec::ConverterState conversion;
        auto* codec=QTextCodec::codecForUtfText(bytes,QTextCodec::codecForName("UTF-8"));
        QString text=codec->toUnicode(bytes.constData(),bytes.size(),&conversion);
        if(conversion.invalidChars||text.contains(QChar(0)))fail(QStringLiteral("表格编码无效，请使用 UTF-8 或带 BOM 的 UTF-16 文本。"));
        if(text.startsWith(QChar(0xfeff)))text.remove(0,1);

        int commas=0,tabs=0,semicolons=0;bool quoted=false,haveText=false;
        for(int i=0;i<text.size();++i){const auto c=text[i];
            if(c=='"'){if(quoted&&i+1<text.size()&&text[i+1]=='"'){++i;continue;}quoted=!quoted;}
            if(!quoted){if(c==',')++commas;else if(c=='\t')++tabs;else if(c==';')++semicolons;
                else if(c=='\n'||c=='\r'){if(haveText)break;else continue;}}
            if(!c.isSpace())haveText=true;
        }
        const QChar separator=tabs?QChar('\t'):commas?QChar(','):semicolons?QChar(';'):QChar();
        const bool whitespace=separator.isNull();
        QJsonArray nextX,nextY;QStringList cells;QString field;
        int line=1,startLine=1;bool inQuotes=false,closedQuote=false,header=false;
        const auto appendRecord=[&]{
            if(!whitespace||!field.isEmpty()||closedQuote)cells.append(field);
            if(cells.isEmpty()||(cells.size()==1&&cells.front().trimmed().isEmpty())){cells.clear();field.clear();closedQuote=false;return;}
            if(cells.size()!=2)fail(QStringLiteral("第 %1 行应恰有两列，实际为 %2 列。").arg(startLine).arg(cells.size()));
            bool xOk=false,yOk=false;const auto first=cells[0].trimmed(),second=cells[1].trimmed();
            const double x=first.toDouble(&xOk),y=second.toDouble(&yOk);
            if(!header&&nextX.isEmpty()&&!xOk&&!yOk&&!second.isEmpty()
                &&QRegularExpression(QStringLiteral("^(time|t|时间|时刻|x)([_\\s\\[(（].*)?$"),QRegularExpression::CaseInsensitiveOption|QRegularExpression::DotMatchesEverythingOption).match(first).hasMatch()){
                header=true;cells.clear();field.clear();closedQuote=false;return;
            }
            if(!xOk||!yOk||!std::isfinite(x)||!std::isfinite(y))fail(QStringLiteral("第 %1 行包含空值、非数值或非有限数值。").arg(startLine));
            if(!nextX.isEmpty()&&x<=nextX.last().toDouble())fail(QStringLiteral("第 %1 行时间必须严格大于前一行；不自动排序或丢弃数据。").arg(startLine));
            if(nextX.size()>=maximumTableRows)fail(QStringLiteral("表格超过 100000 行。"));
            nextX.append(x);nextY.append(y);cells.clear();field.clear();closedQuote=false;
        };
        const auto appendCell=[&]{cells.append(field);field.clear();closedQuote=false;};
        for(int i=0;i<text.size();++i){const auto c=text[i];
            if(inQuotes){
                if(c=='"'){if(i+1<text.size()&&text[i+1]=='"'){field+='"';++i;}else{inQuotes=false;closedQuote=true;}}
                else{field+=c;if(c=='\n')++line;}
                continue;
            }
            if(c=='\r'||c=='\n'){
                appendRecord();if(c=='\r'&&i+1<text.size()&&text[i+1]=='\n')++i;
                ++line;startLine=line;continue;
            }
            if((!whitespace&&c==separator)||(whitespace&&c.isSpace())){
                if(!whitespace||!field.isEmpty()||closedQuote)appendCell();continue;
            }
            if(closedQuote){if(c.isSpace())continue;fail(QStringLiteral("第 %1 行的引号结束后存在无效字符。").arg(line));}
            if(c=='"'){
                if(!field.trimmed().isEmpty())fail(QStringLiteral("第 %1 行包含未转义的引号。").arg(line));
                field.clear();inQuotes=true;continue;
            }
            field+=c;
        }
        if(inQuotes)fail(QStringLiteral("第 %1 行开始的引号未闭合。").arg(startLine));
        if(!field.isEmpty()||!cells.isEmpty()||closedQuote)appendRecord();
        const motion::MotionFunction_Table function(tableNumbers(nextX,"时间列"),tableNumbers(nextY,"数值列"));
        if(!function.IsValid())fail(QStringLiteral("表格至少需要两行有效、严格递增的时间数据。"));
        tableX=nextX;tableY=nextY;error.clear();return true;
    }catch(const std::exception& failure){error=QString::fromUtf8(failure.what());return false;}
}

bool writeNativeFunctionTable(const QString& path,const QJsonArray& tableX,const QJsonArray& tableY,
                              const QString& valueUnits,QString& error)
{
    try{
        const auto x=tableNumbers(tableX,"时间列"),y=tableNumbers(tableY,"数值列");
        if(!motion::MotionFunction_Table(x,y).IsValid())fail(QStringLiteral("导出前请填写至少两行有限数值，并使时间严格递增。"));
        const bool txt=QFileInfo(path).suffix().compare("txt",Qt::CaseInsensitive)==0||QFileInfo(path).suffix().compare("tsv",Qt::CaseInsensitive)==0;
        const QChar separator=txt?QChar('\t'):QChar(',');
        QString label=valueUnits.isEmpty()?QStringLiteral("Value"):QStringLiteral("Value (%1)").arg(valueUnits);
        label.replace('"',"\"\"");
        QString text=QStringLiteral("\"Time_s\"%1\"%2\"\n").arg(separator).arg(label);
        for(std::size_t i=0;i<x.size();++i)text+=QString::number(x[i],'g',17)+separator+QString::number(y[i],'g',17)+'\n';
        QSaveFile file(path);
        if(!file.open(QIODevice::WriteOnly))fail(QStringLiteral("无法创建表格：%1").arg(file.errorString()));
        const QByteArray data=QByteArray::fromHex("efbbbf")+text.toUtf8();
        if(file.write(data)!=data.size()||!file.commit())fail(QStringLiteral("无法完整写入表格：%1").arg(file.errorString()));
        error.clear();return true;
    }catch(const std::exception& failure){error=QString::fromUtf8(failure.what());return false;}
}

bool sampleNativeFunction(const QString& expression, const QJsonArray& tableX,
                          const QJsonArray& tableY, double start, double end,
                          int sampleCount, NativeFunctionSamples& output, QString& error,
                          bool allowStateVariables, double x, double v)
{
    try {
        if (!std::isfinite(start) || !std::isfinite(end) || end <= start)
            fail(QStringLiteral("预览结束时间必须大于开始时间。"));
        if (!std::isfinite(x) || !std::isfinite(v)) fail(QStringLiteral("预览切片 x、v 必须为有限数值。"));
        if (sampleCount < 2 || sampleCount > 10000) fail(QStringLiteral("预览采样点数应在 2–10000 之间。"));
        const bool useTable = !tableX.isEmpty() || !tableY.isEmpty();
        std::unique_ptr<motion::MotionFunction_Expr> formula;
        std::unique_ptr<motion::MotionFunction_Table> table;
        QVector<double> times;
        for (int i = 0; i < sampleCount; ++i)
            times.push_back(start + (end - start) * (double(i) / (sampleCount - 1)));
        // Keep endpoints exact rather than letting floating-point subtraction
        // move the last preview point slightly beyond the requested range.
        times.front() = start;
        times.back() = end;
        if (useTable) {
            const auto xs = tableNumbers(tableX, QStringLiteral("时间列"));
            const auto ys = tableNumbers(tableY, QStringLiteral("数值列"));
            table = std::make_unique<motion::MotionFunction_Table>(xs, ys);
            if (!table->IsValid()) fail(QStringLiteral("表格至少需要两行；时间与数值行数须一致，时间须严格递增。"));
            for (double time : xs) if (time >= start && time <= end) times.push_back(time);
            std::sort(times.begin(), times.end());
            times.erase(std::unique(times.begin(), times.end()), times.end());
        } else {
            const QString text = expression.trimmed();
            if (text.isEmpty()) fail(QStringLiteral("表达式不能为空。"));
            if (!allowStateVariables && QRegularExpression(QStringLiteral("\\b[xv]\\b"),
                    QRegularExpression::CaseInsensitiveOption).match(text).hasMatch())
                fail(QStringLiteral("时间驱动只使用 t（秒）；x、v 仅用于弹簧或力元的变形与速度上下文。"));
            formula = std::make_unique<motion::MotionFunction_Expr>(text.toStdString());
            if (!formula->IsValid()) fail(QString::fromStdString(formula->GetCompileError()));
        }
        NativeFunctionSamples next;
        next.time = times;
        next.value.reserve(times.size());
        for (double time : times) {
            const double value = useTable ? table->Get_y(time) : formula->Evaluate(time, x, v);
            if (!std::isfinite(value)) fail(QStringLiteral("函数在 t=%1 s 处不是有限值。").arg(time, 0, 'g', 17));
            next.value.push_back(value);
        }
        output = std::move(next);
        error.clear();
        return true;
    } catch (const std::exception& failure) {
        error = QString::fromUtf8(failure.what());
        return false;
    }
}

namespace {
class FunctionDialog final : public QDialog {
public:
    FunctionDialog(QWidget* parent, const QString& expression, const QJsonArray& tableX,
                   const QJsonArray& tableY, const QString& units, double duration,
                   bool allowStateVariables, bool allowTables)
        : QDialog(parent), m_originalExpression(expression), m_allowStateVariables(allowStateVariables), m_allowTables(allowTables)
    {
        setWindowTitle(QStringLiteral("函数编辑与预览"));
        setObjectName(QStringLiteral("nativeFunctionEditor"));
        QFont uiFont(MxFunctions::UiMetrics::uiFontFamily());
        uiFont.setPixelSize(MxFunctions::UiMetrics::bodyFontSize());
        setFont(uiFont);
        resize(680, 720);
        auto* layout = new QVBoxLayout(this);
        auto* modeForm = new QFormLayout;
        m_mode = new QComboBox;
        m_mode->setObjectName(QStringLiteral("nativeFunctionMode"));
        m_mode->addItems({QStringLiteral("常数值"), QStringLiteral("函数表达式"),
            QStringLiteral("函数模板"), QStringLiteral("时间—数值表格")});
        if (!allowTables) if (auto* model=qobject_cast<QStandardItemModel*>(m_mode->model()))
            model->item(3)->setEnabled(false);
        modeForm->addRow(QStringLiteral("输入模式"), m_mode);
        modeForm->addRow(QStringLiteral("数值单位"), new QLabel(units.isEmpty() ? QStringLiteral("由当前载荷／驱动定义") : units));
        layout->addLayout(modeForm);
        m_pages = new QStackedWidget;
        layout->addWidget(m_pages);

        m_constant = new QLineEdit;
        m_constant->setObjectName(QStringLiteral("nativeFunctionConstant"));
        bool isNumber = false;
        const double number = expression.toDouble(&isNumber);
        const bool isFiniteConstant = isNumber && std::isfinite(number);
        m_constant->setText(isFiniteConstant ? expression : QStringLiteral("0"));
        m_pages->addWidget(m_constant);

        auto* expressionPage = new QWidget;
        auto* expressionLayout = new QVBoxLayout(expressionPage);
        expressionLayout->setContentsMargins(0, 0, 0, 0);
        m_expression = new QPlainTextEdit(expression);
        m_expression->setObjectName(QStringLiteral("nativeFunctionExpression"));
        m_expression->setPlaceholderText(QStringLiteral("例如：10*sin(2*pi*t)，时间 t 单位为秒"));
        m_expression->setMaximumHeight(85);
        expressionLayout->addWidget(m_expression);
        auto* catalogRow = new QHBoxLayout;
        auto* catalog = new QComboBox;
        catalog->setObjectName(QStringLiteral("nativeFunctionCatalog"));
        const std::array<std::pair<const char*, const char*>, 19> functions{{
            {"sin(x)", "sin(t)"}, {"cos(x)", "cos(t)"}, {"tan(x)", "tan(t)"},
            {"asin(x)", "asin(0)"}, {"acos(x)", "acos(0)"}, {"atan(x)", "atan(t)"},
            {"atan2(y,x)", "atan2(0,1)"}, {"sqrt(x)", "sqrt(t)"}, {"abs(x)", "abs(t)"},
            {"exp(x)", "exp(t)"}, {"log(x)", "log(t+1)"}, {"log10(x)", "log10(t+1)"},
            {"pow(a,b)", "pow(t,2)"}, {"min(a,b)", "min(t,1)"}, {"max(a,b)", "max(t,0)"},
            {"step(t,t0,h0,t1,h1)", "step(t,0,0,1,1)"}, {"t", "t"}, {"pi", "pi"}, {"time", "time"}
        }};
        for (const auto& entry : functions) catalog->addItem(QString::fromUtf8(entry.first), QString::fromUtf8(entry.second));
        if (allowStateVariables) { catalog->addItem(QStringLiteral("x（位移／变形）"), "x"); catalog->addItem(QStringLiteral("v（相对速度）"), "v"); }
        auto* insert = new QPushButton(QStringLiteral("插入函数"));
        catalogRow->addWidget(catalog, 1); catalogRow->addWidget(insert);
        expressionLayout->addLayout(catalogRow);
        connect(insert, &QPushButton::clicked, this, [this, catalog] {
            m_expression->insertPlainText(catalog->currentData().toString()); m_expression->setFocus();
        });
        auto* operators = new QHBoxLayout;
        for (const auto& op : {"+", "-", "*", "/", "^", "(", ")", ","}) {
            auto* button = new QPushButton(QString::fromLatin1(op));
            button->setMaximumWidth(44); operators->addWidget(button);
            connect(button, &QPushButton::clicked, this, [this, op] { m_expression->insertPlainText(QString::fromLatin1(op)); m_expression->setFocus(); });
        }
        operators->addStretch(); expressionLayout->addLayout(operators);
        m_pages->addWidget(expressionPage);

        auto* templatePage = new QWidget;
        auto* templateForm = new QFormLayout(templatePage);
        templateForm->setContentsMargins(0, 0, 0, 0);
        m_template = new QComboBox;
        m_template->setObjectName(QStringLiteral("nativeFunctionTemplate"));
        m_template->addItems({QStringLiteral("正弦"), QStringLiteral("余弦"), QStringLiteral("斜坡"),
            QStringLiteral("平滑阶跃"), QStringLiteral("三次多项式")});
        templateForm->addRow(QStringLiteral("模板"), m_template);
        for (int i = 0; i < 4; ++i) {
            m_parameterLabels[i] = new QLabel;
            m_parameters[i] = new QLineEdit(i == 0 ? "10" : i == 1 ? "1" : "0");
            m_parameters[i]->setObjectName(QStringLiteral("nativeFunctionParameter%1").arg(i));
            templateForm->addRow(m_parameterLabels[i], m_parameters[i]);
            connect(m_parameters[i], &QLineEdit::textChanged, this, [this] { updateTemplate(); });
        }
        m_templateFormula = new QLabel;
        m_templateFormula->setObjectName(QStringLiteral("nativeFunctionTemplateExpression"));
        m_templateFormula->setWordWrap(true);
        templateForm->addRow(QStringLiteral("生成表达式"), m_templateFormula);
        connect(m_template, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateTemplate(); });
        m_pages->addWidget(templatePage);

        auto* tablePage = new QWidget;
        auto* tableLayout = new QVBoxLayout(tablePage);
        tableLayout->setContentsMargins(0, 0, 0, 0);
        m_table = new QTableWidget(std::max(tableX.size(), tableY.size()), 2);
        m_table->setObjectName(QStringLiteral("nativeFunctionTable"));
        m_table->setHorizontalHeaderLabels({QStringLiteral("时间 t (s)"), units.isEmpty() ? QStringLiteral("数值") : units});
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_table->setMinimumHeight(125);
        const auto cellText = [](const QJsonValue& value) {
            return value.isDouble() ? QString::number(value.toDouble(), 'g', 17)
                : value.isString() ? value.toString() : QString{};
        };
        for (int row = 0; row < m_table->rowCount(); ++row) {
            m_table->setItem(row, 0, new QTableWidgetItem(row < tableX.size() ? cellText(tableX[row]) : QString{}));
            m_table->setItem(row, 1, new QTableWidgetItem(row < tableY.size() ? cellText(tableY[row]) : QString{}));
        }
        tableLayout->addWidget(m_table);
        auto* tableActions = new QHBoxLayout;
        auto* add = new QPushButton(QStringLiteral("添加行"));
        add->setObjectName(QStringLiteral("nativeFunctionAddRow"));
        auto* remove = new QPushButton(QStringLiteral("删除行"));
        tableActions->addWidget(add); tableActions->addWidget(remove); tableLayout->addLayout(tableActions);
        connect(add, &QPushButton::clicked, this, [this] {
            const int row = m_table->rowCount(); if (row >= maximumTableRows) return;
            m_table->insertRow(row);
            bool valid = false;
            const double last = row && m_table->item(row - 1, 0) ? m_table->item(row - 1, 0)->text().toDouble(&valid) : 0;
            m_table->setItem(row, 0, new QTableWidgetItem(QString::number(valid ? last + 1 : row, 'g', 17)));
            m_table->setItem(row, 1, new QTableWidgetItem("0"));
        });
        connect(remove, &QPushButton::clicked, this, [this] {
            std::vector<int> rows;
            for (const auto& index : m_table->selectionModel()->selectedIndexes()) rows.push_back(index.row());
            std::sort(rows.begin(), rows.end()); rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
            if (rows.empty() && m_table->currentRow() >= 0) rows.push_back(m_table->currentRow());
            for (auto it = rows.rbegin(); it != rows.rend(); ++it) m_table->removeRow(*it);
        });
        auto* importTable=new QPushButton(QStringLiteral("导入 CSV/TXT…"));importTable->setObjectName("nativeFunctionImportTable");
        auto* exportTable=new QPushButton(QStringLiteral("导出 CSV/TXT…"));exportTable->setObjectName("nativeFunctionExportTable");
        tableActions->addWidget(importTable);tableActions->addWidget(exportTable);
        connect(importTable,&QPushButton::clicked,this,[this]{
            const auto path=QFileDialog::getOpenFileName(this,QStringLiteral("导入时间—数值表格"),{},QStringLiteral("数据表格 (*.csv *.tsv *.txt);;所有文件 (*)"));
            if(path.isEmpty())return;QJsonArray x,y;QString error;
            if(!readNativeFunctionTable(path,x,y,error)){message(error,false);return;}
            {const QSignalBlocker blocker(m_table);m_table->setRowCount(x.size());
                for(int row=0;row<x.size();++row){m_table->setItem(row,0,new QTableWidgetItem(QString::number(x[row].toDouble(),'g',17)));
                    m_table->setItem(row,1,new QTableWidgetItem(QString::number(y[row].toDouble(),'g',17)));}}
            validateAndPreview();
        });
        connect(exportTable,&QPushButton::clicked,this,[this,units]{
            QJsonArray x,y;try{readTable(x,y);}catch(const std::exception& failure){message(QString::fromUtf8(failure.what()),false);return;}
            auto path=QFileDialog::getSaveFileName(this,QStringLiteral("导出时间—数值表格"),"motion-table.csv",QStringLiteral("CSV (*.csv);;制表符文本 (*.txt *.tsv)"));
            if(path.isEmpty())return;if(QFileInfo(path).suffix().isEmpty())path+=".csv";QString error;
            if(!writeNativeFunctionTable(path,x,y,units,error))message(error,false);
            else message(QStringLiteral("已导出 %1 行时间数据。").arg(x.size()),true);
        });
        auto* tableHint = new QLabel(QStringLiteral("时间须严格递增；段内线性插值，表外保持端点值。"));
        tableHint->setWordWrap(true); tableLayout->addWidget(tableHint);
        m_pages->addWidget(tablePage);

        auto* previewOptions = new QHBoxLayout;
        m_start = new QDoubleSpinBox; m_end = new QDoubleSpinBox;
        for (auto* spin : {m_start, m_end}) { spin->setDecimals(8); spin->setRange(-1e9, 1e9); }
        m_start->setObjectName(QStringLiteral("nativeFunctionPreviewStart"));
        m_end->setObjectName(QStringLiteral("nativeFunctionPreviewEnd"));
        m_start->setValue(0); m_end->setValue(std::isfinite(duration) && duration > 0 ? duration : 10);
        previewOptions->addWidget(new QLabel(QStringLiteral("预览时间 (s)"))); previewOptions->addWidget(m_start); previewOptions->addWidget(new QLabel("—")); previewOptions->addWidget(m_end);
        auto* preview = new QPushButton(QStringLiteral("校验并预览"));
        preview->setObjectName(QStringLiteral("nativeFunctionPreview"));
        previewOptions->addWidget(preview); layout->addLayout(previewOptions);
        if (allowStateVariables) {
            auto* state = new QHBoxLayout;
            m_x = new QDoubleSpinBox; m_v = new QDoubleSpinBox;
            for (auto* spin : {m_x, m_v}) { spin->setDecimals(8); spin->setRange(-1e12, 1e12); }
            state->addWidget(new QLabel(QStringLiteral("预览切片 x"))); state->addWidget(m_x);
            state->addWidget(new QLabel(QStringLiteral("v"))); state->addWidget(m_v);
            layout->addLayout(state);
        }
        m_status = new QLabel;
        m_status->setObjectName(QStringLiteral("nativeFunctionStatus"));
        m_status->setWordWrap(true); layout->addWidget(m_status);
        m_plot = new QwtPlot;
        m_plot->setObjectName(QStringLiteral("nativeFunctionPlot"));
        m_plot->setFont(uiFont);
        m_plot->setMinimumHeight(200);
        m_plot->setCanvasBackground(Qt::white);
        {
            QFont tickFont(MxFunctions::UiMetrics::uiFontFamily());
            tickFont.setPixelSize(MxFunctions::UiMetrics::helperFontSize());
            for (int axis = 0; axis < QwtPlot::axisCnt; ++axis)
                m_plot->setAxisFont(axis, tickFont);
            QFont axisTitleFont = uiFont;
            axisTitleFont.setBold(true);
            QwtText xTitle(QStringLiteral("时间 t (s)"));
            xTitle.setFont(axisTitleFont);
            QwtText yTitle(units.isEmpty() ? QStringLiteral("数值") : units);
            yTitle.setFont(axisTitleFont);
            m_plot->setAxisTitle(QwtPlot::xBottom, xTitle);
            m_plot->setAxisTitle(QwtPlot::yLeft, yTitle);
        }
        auto* grid = new QwtPlotGrid; grid->setMajorPen(QPen(QColor(220, 225, 230), 0, Qt::DotLine)); grid->attach(m_plot);
        m_curve = new QwtPlotCurve(QStringLiteral("激励函数"));
        m_curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
        m_curve->setPen(QPen(QColor(30, 112, 204), 2)); m_curve->attach(m_plot);
        layout->addWidget(m_plot, 1);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttons->setObjectName(QStringLiteral("nativeFunctionButtons"));
        buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("应用"));
        buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::accepted, this, [this] { if (validateAndPreview()) accept(); });
        connect(preview, &QPushButton::clicked, this, [this] { validateAndPreview(); });
        connect(m_mode, QOverload<int>::of(&QComboBox::currentIndexChanged), m_pages, &QStackedWidget::setCurrentIndex);
        connect(m_mode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { clearPreview(); });
        connect(m_expression, &QPlainTextEdit::textChanged, this, [this] { clearPreview(); validateExpressionSyntax(); });
        connect(m_constant, &QLineEdit::textChanged, this, [this] { clearPreview(); });
        connect(m_table, &QTableWidget::itemChanged, this, [this] { clearPreview(); });
        for (auto* spin : {m_start, m_end, m_x, m_v}) if (spin)
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this] { clearPreview(); });
        updateTemplate();
        m_mode->setCurrentIndex(!tableX.isEmpty() || !tableY.isEmpty() ? 3 : isFiniteConstant ? 0 : 1);
        m_pages->setCurrentIndex(m_mode->currentIndex());
        validateAndPreview();
    }

    QString resultExpression;
    QJsonArray resultTableX, resultTableY;

private:
    void message(const QString& text, bool valid)
    {
        m_status->setText(text); m_status->setStyleSheet(valid ? QStringLiteral("color:#237a35;") : QStringLiteral("color:#b42318;"));
    }

    void clearPreview()
    {
        if (!m_curve) return;
        m_curve->setSamples(QVector<double>{}, QVector<double>{}); m_plot->replot();
        message(QStringLiteral("参数已修改，请重新校验并预览。"), true);
    }

    void validateExpressionSyntax()
    {
        if (m_mode->currentIndex() != 1) return;
        const auto text = m_expression->toPlainText().trimmed();
        if (text.isEmpty()) { message(QStringLiteral("表达式不能为空。"), false); return; }
        motion::MotionFunction_Expr function(text.toStdString());
        if (!function.IsValid()) message(QString::fromStdString(function.GetCompileError()), false);
        else if (!m_allowStateVariables && QRegularExpression(QStringLiteral("\\b[xv]\\b"), QRegularExpression::CaseInsensitiveOption).match(text).hasMatch())
            message(QStringLiteral("时间驱动不接受变形变量 x、v。"), false);
        else message(QStringLiteral("语法通过；请预览检查所选时间区间的数值。"), true);
    }

    QString templateExpression() const
    {
        std::array<QString, 4> p{{"0", "0", "0", "0"}};
        const int parameterCount = m_template->currentIndex() == 2 ? 2 : 4;
        for (int i = 0; i < parameterCount; ++i) p[i] = QString::number(finiteNumber(m_parameters[i]->text(), QStringLiteral("模板参数 %1").arg(i + 1)), 'g', 17);
        switch (m_template->currentIndex()) {
        case 0: return QStringLiteral("(%1)*sin(2*pi*(%2)*t+(%3))+(%4)").arg(p[0], p[1], p[2], p[3]);
        case 1: return QStringLiteral("(%1)*cos(2*pi*(%2)*t+(%3))+(%4)").arg(p[0], p[1], p[2], p[3]);
        case 2: return QStringLiteral("(%1)*t+(%2)").arg(p[0], p[1]);
        case 3: {
            const double transition = finiteNumber(m_parameters[2]->text(), QStringLiteral("过渡时间"));
            if (transition < 0) fail(QStringLiteral("阶跃过渡时间不得为负；0 表示硬阶跃。"));
            return QStringLiteral("step(t,(%2),(%4),(%2)+(%3),(%4)+(%1))").arg(p[0], p[1], p[2], p[3]);
        }
        default: return QStringLiteral("(%1)+(%2)*t+(%3)*t^2+(%4)*t^3").arg(p[0], p[1], p[2], p[3]);
        }
    }

    void updateTemplate()
    {
        const std::array<QStringList, 5> names{{
            {QStringLiteral("幅值"), QStringLiteral("频率 (Hz)"), QStringLiteral("相位 (rad)"), QStringLiteral("偏置")},
            {QStringLiteral("幅值"), QStringLiteral("频率 (Hz)"), QStringLiteral("相位 (rad)"), QStringLiteral("偏置")},
            {QStringLiteral("斜率（每秒）"), QStringLiteral("初值")},
            {QStringLiteral("变化量"), QStringLiteral("起始时间 (s)"), QStringLiteral("过渡时间 (s)"), QStringLiteral("初值")},
            {QStringLiteral("a₀"), QStringLiteral("a₁"), QStringLiteral("a₂"), QStringLiteral("a₃")}
        }};
        const auto& labels = names.at(std::size_t(m_template->currentIndex()));
        for (int i = 0; i < 4; ++i) {
            m_parameterLabels[i]->setVisible(i < labels.size()); m_parameters[i]->setVisible(i < labels.size());
            if (i < labels.size()) m_parameterLabels[i]->setText(labels[i]);
        }
        try { m_templateFormula->setText(templateExpression()); }
        catch (const std::exception& error) { m_templateFormula->setText(QString::fromUtf8(error.what())); }
        clearPreview();
    }

    bool validateAndPreview()
    {
        try {
            QString expression;
            QJsonArray tableX, tableY;
            switch (m_mode->currentIndex()) {
            case 0: expression = QString::number(finiteNumber(m_constant->text(), QStringLiteral("常数值")), 'g', 17); break;
            case 1: expression = m_expression->toPlainText().trimmed(); break;
            case 2: expression = templateExpression(); break;
            default:
                if (!m_allowTables) fail(QStringLiteral("此力元函数不使用时间表格；请使用表达式或参数模板。"));
                readTable(tableX,tableY);
                expression = m_originalExpression;
            }
            NativeFunctionSamples samples; QString error;
            if (!sampleNativeFunction(expression, tableX, tableY, m_start->value(), m_end->value(), 401,
                    samples, error, m_allowStateVariables, m_x ? m_x->value() : 0, m_v ? m_v->value() : 0)) fail(error);
            m_curve->setSamples(samples.time, samples.value);
            m_plot->setAxisAutoScale(QwtPlot::xBottom); m_plot->setAxisAutoScale(QwtPlot::yLeft); m_plot->replot();
            resultExpression = expression; resultTableX = tableX; resultTableY = tableY;
            message(QStringLiteral("引擎校验通过；已采样 %1 点。%2").arg(samples.time.size()).arg(m_allowStateVariables ?
                QStringLiteral("此预览使用当前 x、v 切片。") : QStringLiteral("预览使用当前驱动的数值单位。")), true);
            return true;
        } catch (const std::exception& failure) {
            m_curve->setSamples(QVector<double>{}, QVector<double>{}); m_plot->replot();
            message(QString::fromUtf8(failure.what()), false); return false;
        }
    }

    void readTable(QJsonArray& x,QJsonArray& y) const
    {
        if(m_table->rowCount()<2)fail(QStringLiteral("表格至少需要两行。"));
        if(m_table->rowCount()>maximumTableRows)fail(QStringLiteral("表格最多支持 100000 行。"));
        for(int row=0;row<m_table->rowCount();++row){
            const auto text=[&](int column){const auto* item=m_table->item(row,column);return item?item->text():QString{};};
            x.append(finiteNumber(text(0),QStringLiteral("表格第 %1 行时间").arg(row+1)));
            y.append(finiteNumber(text(1),QStringLiteral("表格第 %1 行数值").arg(row+1)));
        }
    }

    QString m_originalExpression;
    bool m_allowStateVariables;
    bool m_allowTables;
    QComboBox *m_mode = nullptr, *m_template = nullptr;
    QStackedWidget* m_pages = nullptr;
    QLineEdit* m_constant = nullptr;
    QPlainTextEdit* m_expression = nullptr;
    std::array<QLineEdit*, 4> m_parameters{};
    std::array<QLabel*, 4> m_parameterLabels{};
    QLabel *m_templateFormula = nullptr, *m_status = nullptr;
    QTableWidget* m_table = nullptr;
    QDoubleSpinBox *m_start = nullptr, *m_end = nullptr, *m_x = nullptr, *m_v = nullptr;
    QwtPlot* m_plot = nullptr;
    QwtPlotCurve* m_curve = nullptr;
};
}

bool editNativeFunction(QWidget* parent, QString& expression, QJsonArray& tableX,
                        QJsonArray& tableY, const QString& units, double duration,
                        bool allowStateVariables, bool allowTables)
{
    // Never silently discard a table that the caller's force model cannot use.
    if (!allowTables && (!tableX.isEmpty() || !tableY.isEmpty())) return false;
    FunctionDialog dialog(parent, expression, tableX, tableY, units, duration, allowStateVariables, allowTables);
    if (dialog.exec() != QDialog::Accepted) return false;
    expression = dialog.resultExpression;
    tableX = dialog.resultTableX;
    tableY = dialog.resultTableY;
    return true;
}

} // namespace MotionOcc
