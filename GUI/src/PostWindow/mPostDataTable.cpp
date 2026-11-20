#include "mPostDataTable.h"
#include "ui_mPostDataTable.h"
#include <QFileDialog>
#include <QTextStream>
#include <QStandardPaths>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QDate>
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mOtherFunction.h"
namespace PostWindow
{
	mPostDataTable::mPostDataTable(QWidget *parent)
		: MechMainWindow::mFramelessDialog(parent)
	{
		_ui = new Ui::mPostDataTable;
		//.ui 顶层布局装进内容容器再交给基类（基类 this 的布局已被标题栏占用，
		//直接 setupUi(this) 会被 Qt 拒绝挂载）
		QWidget* content = new QWidget(this);
		_ui->setupUi(content);
		setContentWidget(content);

		//初始化表格样式
		_ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_ui->tableWidget->verticalHeader()->setHidden(true);
		_ui->tableWidget->resizeColumnsToContents();
		_ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		_ui->tableWidget->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
		//_ui->tableWidget->horizontalHeader()->setHighlightSections(false);
		//_ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView:section{background:rgb(218,218,218);}");
		//连接信号与曹
		connect(_ui->pushButton, SIGNAL(clicked()), this, SLOT(slot_exportDatas()));
		connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
		slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);
	}

	mPostDataTable::~mPostDataTable()
	{
		if (_ui != nullptr)
		{
			delete _ui;
		}
	}

	void mPostDataTable::setTableData(const QStringList& names, const QString& xText, QVector<QString> yText, const QVector<double>& xval, const QList<QPair<QString, QVector<double>>>& yvals)
	{
		//清空表格
		_ui->tableWidget->clearContents();

		const int ncurve = names.size();
		if (ncurve == 0) return;
		if (yvals.size() < ncurve) return;

		// 保证 yText 长度与曲线列数一致，不足补空字符串
		if (yText.size() < ncurve)
		{
			const int oldSize = yText.size();
			yText.resize(ncurve);
			for (int i = oldSize; i < ncurve; ++i)
				yText[i] = QString();
		}

		_ui->tableWidget->setColumnCount(ncurve + 1);
		const int ndata = xval.size();
		_ui->tableWidget->setRowCount(ndata + 2);

		_ui->tableWidget->setItem(0, 0, new QTableWidgetItem(""));
		_ui->tableWidget->setItem(1, 0, new QTableWidgetItem(xText));

		// 设置第一行曲线名称，第二行 y 轴标题
		for (int i = 0; i < ncurve; ++i)
		{
			_ui->tableWidget->setItem(0, i + 1, new QTableWidgetItem(names.at(i)));
			_ui->tableWidget->setItem(1, i + 1, new QTableWidgetItem(yText.at(i)));
		}

		// 显示 X 值
		for (int i = 0; i < ndata; ++i)
		{
			double val = xval.at(i);
			_ui->tableWidget->setItem(i + 2, 0, new QTableWidgetItem(MxFunctions::mNumberToString(val)));
		}

		// 显示 Y 值
		for (int i = 0; i < ncurve; ++i)
		{
			const QVector<double>& datas = yvals.at(i).second;
			const int n = qMin(datas.size(), ndata);
			for (int j = 0; j < n; ++j)
			{
				double val = datas.at(j);
				_ui->tableWidget->setItem(j + 2, i + 1, new QTableWidgetItem(MxFunctions::mNumberToString(val)));
			}
		}
	}

	void mPostDataTable::setTableData(const QStringList& names, const QString& xText, QVector<QString> yText, const QStringList& xval, const QList<QPair<QString, QVector<double>>>& yvals)
	{
		//清空表格
		_ui->tableWidget->clearContents();

		const int ncurve = names.size();
		if (ncurve == 0) return;
		if (yvals.size() < ncurve) return;

		// 保证 yText 长度与曲线列数一致，不足补空字符串
		if (yText.size() < ncurve)
		{
			const int oldSize = yText.size();
			yText.resize(ncurve);
			for (int i = oldSize; i < ncurve; ++i)
				yText[i] = QString();
		}

		_ui->tableWidget->setColumnCount(ncurve + 1);
		const int ndata = xval.size();
		_ui->tableWidget->setRowCount(ndata + 2);

		_ui->tableWidget->setItem(0, 0, new QTableWidgetItem(""));
		_ui->tableWidget->setItem(1, 0, new QTableWidgetItem(xText));

		// 设置第一行曲线名称，第二行 y 轴标题
		for (int i = 0; i < ncurve; ++i)
		{
			_ui->tableWidget->setItem(0, i + 1, new QTableWidgetItem(names.at(i)));
			_ui->tableWidget->setItem(1, i + 1, new QTableWidgetItem(yText.at(i)));
		}

		// 显示 X 值
		for (int i = 0; i < ndata; ++i)
		{
			QString val = xval.at(i);
			_ui->tableWidget->setItem(i + 2, 0, new QTableWidgetItem(val));
		}

		// 显示 Y 值
		for (int i = 0; i < ncurve; ++i)
		{
			const QVector<double>& datas = yvals.at(i).second;
			const int n = qMin(datas.size(), ndata);
			for (int j = 0; j < n; ++j)
			{
				double val = datas.at(j);
				_ui->tableWidget->setItem(j + 2, i + 1, new QTableWidgetItem(MxFunctions::mNumberToString(val)));
			}
		}
	}

    void mPostDataTable::setTableData(const QStringList& names, const QString& xText, const QVector<double>& xval, const QVector<QVector<double>>& yvals)
    {
        // clear() 会彻底重置表格（包括所有表头、单元格、列状态），避免残留干扰
        _ui->tableWidget->clear();
        _ui->tableWidget->setRowCount(0);
        _ui->tableWidget->setColumnCount(0); // 关键：强制归零，清除旧的列缓存

        // ========== 健壮性校验（不变） ==========
        if (names.size() != yvals.size()) {
            //qWarning() << "setTableData: names count mismatch yvals count.";
            return;
        }
        if (xval.isEmpty()) {
            // 注意：此时表格已清空，直接返回即可
            return;
        }

        // ========== 阻断界面更新与信号 ==========
        _ui->tableWidget->setUpdatesEnabled(false);
        _ui->tableWidget->blockSignals(true);

        int rowCount = xval.size();
        int colCount = 1 + names.size();
        _ui->tableWidget->setRowCount(rowCount);
        _ui->tableWidget->setColumnCount(colCount);

        // ========== 构建列标题 ==========
        QStringList headers;
        headers << xText;
        for (int i = 0; i < names.size(); ++i) {
            headers << names[i];
        }

        // ========== 修复2：强制刷新表头并确保可见 ==========
        _ui->tableWidget->setHorizontalHeaderLabels(headers);
        // 某些 Qt 样式下表头可能被意外隐藏，强制显示
        _ui->tableWidget->horizontalHeader()->setVisible(true);
        // 强制视图重新计算表头布局（非常重要）
        _ui->tableWidget->horizontalHeader()->reset();

        // ========== 填充数据（代码不变） ==========
        for (int i = 0; i < rowCount; ++i) {
            QTableWidgetItem* xItem = new QTableWidgetItem(QString::number(xval[i]));
            _ui->tableWidget->setItem(i, 0, xItem);

            for (int j = 0; j < names.size(); ++j) {
                QString displayText;
                if (i < yvals[j].size()) {
                    displayText = QString::number(yvals[j][i]);
                }
                else {
                    displayText = "--";
                }
                QTableWidgetItem* yItem = new QTableWidgetItem(displayText);
                if (displayText == "--") {
                    yItem->setBackground(Qt::lightGray);
                    yItem->setTextAlignment(Qt::AlignCenter);
                }
                _ui->tableWidget->setItem(i, j + 1, yItem);
            }
        }

        // ========== 视觉优化 ==========
        _ui->tableWidget->setAlternatingRowColors(true);
        _ui->tableWidget->resizeColumnsToContents();
        _ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // ========== 恢复界面更新与信号 ==========
        _ui->tableWidget->blockSignals(false);
        _ui->tableWidget->setUpdatesEnabled(true);
        _ui->tableWidget->repaint(); // 强制重绘
    }

	void mPostDataTable::slot_exportDatas()
	{
		//获取当前工作目录
		//QString wd = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
		//QDir dir;
		//if (!dir.exists(wd))
		//{
		//	dir.mkpath(wd);
		//}
		//QString outfile;
		//if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		//{
		//	outfile = QFileDialog::getSaveFileName(this, QString("导出数据"), wd, QString("CSV File(*.csv);;Text File(*.txt)"));
		//}
		//else
		//{
		//	outfile = QFileDialog::getSaveFileName(this, QString("Export Data"), wd, QString("CSV File(*.csv);;Text File(*.txt)"));
		//}
		//if (outfile.isEmpty())
		//{
		//	return;
		//}
		//QFile file(outfile);
		//if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		//{
		//	QTextStream ts(&file);
		//	ts.setCodec("UTF-8");
		//	ts << "*Type1" << endl;
		//	int ncol = _ui->tableWidget->columnCount();
		//	for (int i = 0; i < ncol - 1; ++i)
		//	{
		//		QTableWidgetItem* item = _ui->tableWidget->item(0, i);
		//		if (item != nullptr)
		//		{
		//			QString title = _ui->tableWidget->item(0, i)->text().trimmed();
		//			ts << title << ",";
		//		}
		//		//<< qSetFieldWidth(10) 
		//	}                                                           // << qSetFieldWidth(0)
		//	QTableWidgetItem* lastHeaderItem = _ui->tableWidget->item(0, ncol - 1);
		//	if (lastHeaderItem != nullptr)
		//	{
		//		ts << lastHeaderItem->text().trimmed() << endl;
		//	}
		//	else
		//	{
		//		ts << endl;
		//	}

		//	const int nrow = _ui->tableWidget->rowCount();
		//	for (int i = 1; i < nrow; ++i)
		//	{
		//		//ts << qSetFieldWidth(10);
		//		for (int j = 0; j < ncol - 1; ++j)
		//		{
		//			QTableWidgetItem* valitem = _ui->tableWidget->item(i, j);
		//			if (valitem != nullptr)
		//			{
		//				QString str = valitem->text().trimmed();
		//				ts << valitem->text().trimmed() << ",";
		//			}
		//			else
		//			{
		//				ts << "" << ",";
		//			}
		//		}
		//		QTableWidgetItem* item = _ui->tableWidget->item(i, ncol - 1);
		//		if (item != nullptr)
		//		{       
		//			QString str = item->text().trimmed();
		//			ts << item->text().trimmed() << endl;
		//		}
		//		else
		//		{
		//			ts << endl;
		//		}
		//	}
		//}
		//file.close();

            // 1. 工作目录（带回退）
        QString wd = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        QDir dir;
        if (!dir.exists(wd) && !dir.mkpath(wd)) {
            wd = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            dir.mkpath(wd);
        }

        // 生成默认文件名（示例：固定名称 + 日期）
        QString defaultName = QString("export_data_%1.csv")
            .arg(QDate::currentDate().toString("yyyyMMdd"));
        QString defaultPath = QDir(wd).filePath(defaultName);
        // 2. 对话框标题（语言切换）
        QString dialogTitle = (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            ? QString("导出数据") : QString("Export Data");

        // 3. 获取保存路径
        QString outfile = QFileDialog::getSaveFileName(
            this, dialogTitle, defaultPath,
            QString("CSV File(*.csv);;Text File(*.txt)")
        );
        if (outfile.isEmpty())
            return;

        // 4. 检查表格尺寸
        int ncol = _ui->tableWidget->columnCount();
        int nrow = _ui->tableWidget->rowCount();
        if (ncol == 0 || nrow == 0) {
            MechMainWindow::mFramelessMessageBox::information(this, tr("Info"), tr("No data to export."));
            return;
        }

        // 5. 打开文件
        QFile file(outfile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            MechMainWindow::mFramelessMessageBox::warning(this, tr("Error"),
                tr("Cannot open file for writing:\n%1").arg(file.errorString()));
            return;
        }

        // 6. CSV转义函数
        auto escapeCsv = [](const QString& field) -> QString {
            if (field.contains(',') || field.contains('"') || field.contains('\n') || field.contains('\r')) {
                QString escaped = field;
                escaped.replace('"', "\"\"");
                return QString("\"%1\"").arg(escaped);
            }
            return field;
            };

        QTextStream ts(&file);
        ts.setCodec("UTF-8");
        //ts << "*Type1\n";

        // 7. 智能获取标题行
        QStringList headerList;
        bool useHorizontalHeader = false;

        // 7.1 尝试从水平表头获取标题
        for (int i = 0; i < ncol; ++i) {
            QTableWidgetItem* hdrItem = _ui->tableWidget->horizontalHeaderItem(i);
            if (hdrItem && !hdrItem->text().trimmed().isEmpty()) {
                useHorizontalHeader = true;
                headerList << escapeCsv(hdrItem->text().trimmed());
            }
            else {
                headerList << QString(); // 占位，保持列数
            }
        }

        // 7.2 如果水平表头全为空，则回退到第0行作为标题（原逻辑）
        if (!useHorizontalHeader) {
            headerList.clear();
            for (int i = 0; i < ncol; ++i) {
                QTableWidgetItem* item = _ui->tableWidget->item(0, i);
                headerList << escapeCsv(item ? item->text().trimmed() : QString());
            }
        }

        // 写入标题行
       ts << headerList.join(",") << "\n";

        // 8. 确定数据起始行
        int startRow = useHorizontalHeader ? 0 : 1; // 若使用水平表头，数据从0行开始；否则从1行开始

        // 9. 写入数据行
        for (int i = startRow; i < nrow; ++i) {
            QStringList rowList;
            for (int j = 0; j < ncol; ++j) {
                QTableWidgetItem* item = _ui->tableWidget->item(i, j);
                QString text = item ? item->text().trimmed() : QString();
                rowList << escapeCsv(text);
            }
            ts << rowList.join(",") << "\n";
        }
	}

	void mPostDataTable::slot_changeLanguage(int language)
	{
		if (language == 0)
		{
			setWindowTitle("数据");
			_ui->pushButton->setText("导出");
		}
		else if (language == 1)
		{
			setWindowTitle("Data");
			_ui->pushButton->setText("Export");
		}
	}

}
