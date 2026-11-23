#include "NativeFunctionEditor.h"

#include <QApplication>
#include <QAbstractItemModel>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextCodec>
#include <QTimer>

#include <cmath>
#include <stdexcept>

using namespace MotionOcc;
namespace {
void require(bool ok, const QString& message) { if (!ok) throw std::runtime_error(message.toStdString()); }
void expectNear(double value, double expected, double tolerance, const QString& name)
{
    require(std::isfinite(value) && std::abs(value - expected) <= tolerance,
        QStringLiteral("%1: got %2, expected %3").arg(name).arg(value, 0, 'g', 17).arg(expected, 0, 'g', 17));
}
}

int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    try {
        NativeFunctionSamples samples; QString error;
        require(sampleNativeFunction("10*sin(2*pi*t)", {}, {}, 0, 1, 5, samples, error), error);
        expectNear(samples.value[1], 10, 1e-10, "sine quarter period");
        expectNear(samples.value[3], -10, 1e-10, "sine three quarters");
        const auto before = samples;
        require(!sampleNativeFunction("unknown(t)", {}, {}, 0, 1, 5, samples, error), "invalid expression accepted");
        require(samples.time == before.time && samples.value == before.value, "invalid preview changed existing samples");
        require(!sampleNativeFunction("1/(t-0.5)", {}, {}, 0, 1, 5, samples, error), "nonfinite law accepted");
        require(!sampleNativeFunction("x+t", {}, {}, 0, 1, 5, samples, error), "motor accepted deformation variable");
        require(sampleNativeFunction("2*x+v+t", {}, {}, 0, 1, 5, samples, error, true, 3, 4), error);
        expectNear(samples.value.back(), 11, 1e-12, "force preview explicit state slice");
        // A very short pulse must retain both knots and its actual peak even
        // when neither knot falls on the regular preview grid.
        const QJsonArray tx{0, .1001, .1002, .1003, 1}, ty{0, 0, 80, 0, 0};
        require(sampleNativeFunction("inactive_invalid(\"", tx, ty, -.1, 1.1, 5, samples, error), error);
        const int peak = samples.time.indexOf(.1002);
        require(peak >= 0, "table peak knot omitted from preview");
        expectNear(samples.value[peak], 80, 1e-12, "piecewise linear peak");
        expectNear(samples.value.front(), 0, 1e-12, "table holds first sample");
        expectNear(samples.value.back(), 0, 1e-12, "table holds last sample");
        require(!sampleNativeFunction("0", QJsonArray{0,0}, QJsonArray{1,2}, 0, 1, 5, samples, error), "duplicate table time accepted");
        QJsonArray longX,longY;
        for(int i=0;i<1025;++i){longX.append(i*.001);longY.append(i==513?12.5:0.0);}
        require(sampleNativeFunction("inactive",longX,longY,0,1.024,5,samples,error),error);
        const int narrowPeak=samples.time.indexOf(longX[513].toDouble());
        require(samples.time.size()>=1025&&narrowPeak>=0,"A table beyond the former 1000-row UI limit lost sample knots");
        expectNear(samples.value[narrowPeak],12.5,1e-12,"1025-knot narrow road input peak");
        QTemporaryDir files;require(files.isValid(),"table file temporary directory");
        const auto csvPath=files.filePath("road.csv"),txtPath=files.filePath("road.txt"),badPath=files.filePath("invalid.csv");
        QJsonArray importedX,importedY;
        require(writeNativeFunctionTable(csvPath,longX,longY,"N, \"quoted\"",error),error);
        require(readNativeFunctionTable(csvPath,importedX,importedY,error)&&importedX==longX&&importedY==longY,error);
        require(writeNativeFunctionTable(txtPath,longX,longY,"mm",error),error);
        require(readNativeFunctionTable(txtPath,importedX,importedY,error)&&importedX==longX&&importedY==longY,error);
        const auto writeFile=[&](const QString& path,const QByteArray& bytes){QFile file(path);require(file.open(QIODevice::WriteOnly|QIODevice::Truncate)&&file.write(bytes)==bytes.size(),"write import fixture");};
        writeFile(badPath,"Time_s,Value\n0,1\n1,2\n1,3\n");
        require(!readNativeFunctionTable(badPath,importedX,importedY,error)&&error.contains("4")&&importedX==longX&&importedY==longY,
            "Duplicate time import did not report its line and preserve the accepted table");
        writeFile(files.filePath("extra.csv"),"time_s,value\n0,1,2\n1,3\n");
        require(!readNativeFunctionTable(files.filePath("extra.csv"),importedX,importedY,error)&&error.contains("2"),"Malformed table row was silently dropped");
        writeFile(files.filePath("nonfinite.csv"),"time_s,value\n0,1\n1,nan\n");
        require(!readNativeFunctionTable(files.filePath("nonfinite.csv"),importedX,importedY,error)&&error.contains("3"),"Nonfinite CSV row accepted");
        writeFile(files.filePath("spaces.txt"),"  Time_s   Value  \n 0   3\n1   4  \n");
        require(readNativeFunctionTable(files.filePath("spaces.txt"),importedX,importedY,error)
            &&importedX==QJsonArray{0,1}&&importedY==QJsonArray{3,4},error);
        auto unicode=QTextCodec::codecForName("UTF-16LE")->fromUnicode(QStringLiteral("\"Time (s)\"\t\"Value\"\r\n\"0\"\t\"3\"\r\n\"1\"\t\"4\"\r\n"));
        if(!unicode.startsWith(QByteArray::fromHex("fffe")))unicode.prepend(QByteArray::fromHex("fffe"));
        writeFile(files.filePath("unicode.tsv"),unicode);
        require(readNativeFunctionTable(files.filePath("unicode.tsv"),importedX,importedY,error)
            &&importedX==QJsonArray{0,1}&&importedY==QJsonArray{3,4},error);
        const auto protectedPath=files.filePath("protected.csv");writeFile(protectedPath,"accepted-file");
        require(!writeNativeFunctionTable(protectedPath,QJsonArray{0,0},QJsonArray{1,2},"N",error),"Invalid table export accepted");
        QFile protectedFile(protectedPath);require(protectedFile.open(QIODevice::ReadOnly)&&protectedFile.readAll()=="accepted-file","Invalid export replaced an existing file");
        qInfo() << "CSV/TXT parser, 1025 table knots, encoding, invalid input and atomic export passed";

        // Exercise the modal editor's real controls. A formula selection must
        // clear an old table so the solver cannot silently keep using it.
        QString expression = "old_invalid("; QJsonArray tableX{0,1}, tableY{4,9};
        bool controlsFound = false;
        QTimer::singleShot(0, [&] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            auto* mode = dialog->findChild<QComboBox*>("nativeFunctionMode");
            auto* formula = dialog->findChild<QPlainTextEdit*>("nativeFunctionExpression");
            auto* buttons = dialog->findChild<QDialogButtonBox*>("nativeFunctionButtons");
            controlsFound = mode && formula && buttons;
            if (!controlsFound) { dialog->reject(); return; }
            mode->setCurrentIndex(1); formula->setPlainText("30*t*t");
            buttons->button(QDialogButtonBox::Ok)->click();
        });
        require(editNativeFunction(nullptr, expression, tableX, tableY, "mm", 1), "formula dialog application failed");
        require(controlsFound && expression == "30*t*t" && tableX.isEmpty() && tableY.isEmpty(), "formula/table precedence not updated by editor");

        expression = "12"; const auto prior = expression;
        bool invalidRejected = false;
        QTimer::singleShot(0, [&] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            dialog->findChild<QComboBox*>("nativeFunctionMode")->setCurrentIndex(1);
            dialog->findChild<QPlainTextEdit*>("nativeFunctionExpression")->setPlainText("1/(t-.5)");
            dialog->findChild<QDialogButtonBox*>("nativeFunctionButtons")->button(QDialogButtonBox::Ok)->click();
            invalidRejected = dialog->isVisible() && !dialog->findChild<QLabel*>("nativeFunctionStatus")->text().isEmpty();
            dialog->reject();
        });
        require(!editNativeFunction(nullptr, expression, tableX, tableY, "deg", 1), "invalid expression dialog accepted");
        require(invalidRejected && expression == prior && tableX.isEmpty() && tableY.isEmpty(), "cancel/invalid dialog mutated input");

        bool templateUsed = false;
        QTimer::singleShot(0, [&] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            dialog->findChild<QComboBox*>("nativeFunctionMode")->setCurrentIndex(2);
            dialog->findChild<QComboBox*>("nativeFunctionTemplate")->setCurrentIndex(0);
            dialog->findChild<QLineEdit*>("nativeFunctionParameter0")->setText("4");
            dialog->findChild<QLineEdit*>("nativeFunctionParameter1")->setText(".5");
            dialog->findChild<QLineEdit*>("nativeFunctionParameter2")->setText("0");
            dialog->findChild<QLineEdit*>("nativeFunctionParameter3")->setText("2");
            templateUsed = true;
            dialog->findChild<QDialogButtonBox*>("nativeFunctionButtons")->button(QDialogButtonBox::Ok)->click();
        });
        require(editNativeFunction(nullptr, expression, tableX, tableY, "mm", 1), "template application failed");
        require(templateUsed && sampleNativeFunction(expression, tableX, tableY, 0, 1, 3, samples, error), error);
        expectNear(samples.value[1], 6, 1e-12, "template frequency, phase, offset");
        // Force laws have no supported time-table slot. The dialog must not
        // expose a successful edit that the force adapter will silently ignore.
        expression="2*x+v+t";tableX={};tableY={};bool forceModeChecked=false;
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
            auto* mode=dialog->findChild<QComboBox*>("nativeFunctionMode");
            const auto flags=mode->model()->flags(mode->model()->index(3,0));
            forceModeChecked=!(flags&Qt::ItemIsEnabled);
            mode->setCurrentIndex(3);
            dialog->findChild<QDialogButtonBox*>("nativeFunctionButtons")->button(QDialogButtonBox::Ok)->click();
            forceModeChecked=forceModeChecked&&dialog->isVisible();
            mode->setCurrentIndex(1);
            dialog->findChild<QDialogButtonBox*>("nativeFunctionButtons")->button(QDialogButtonBox::Ok)->click();
        });
        require(editNativeFunction(nullptr,expression,tableX,tableY,"N",1,true,false),"State-dependent force formula application failed");
        require(forceModeChecked&&expression=="2*x+v+t"&&tableX.isEmpty()&&tableY.isEmpty(),"Unsupported force table mode was accepted");
        tableX={0,1};tableY={2,3};const auto existingX=tableX,existingY=tableY;
        require(!editNativeFunction(nullptr,expression,tableX,tableY,"N",1,true,false)
            &&tableX==existingX&&tableY==existingY,"Force editor discarded an unsupported supplied table");
        tableX={};tableY={};bool timeOnlyRejected=false;
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
            dialog->findChild<QComboBox*>("nativeFunctionMode")->setCurrentIndex(1);
            dialog->findChild<QPlainTextEdit*>("nativeFunctionExpression")->setPlainText("x+t");
            dialog->findChild<QDialogButtonBox*>("nativeFunctionButtons")->button(QDialogButtonBox::Ok)->click();
            timeOnlyRejected=dialog->isVisible();dialog->reject();
        });
        require(!editNativeFunction(nullptr,expression,tableX,tableY,"N",1,false,false)&&timeOnlyRejected,
            "Time-only vector force editor accepted a deformation state law");
        expression="0";tableX={};tableY={};bool fileButtonsUsed=false;
        const auto uiExport=files.filePath("ui-export.csv");
        QTimer::singleShot(0,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
            dialog->findChild<QComboBox*>("nativeFunctionMode")->setCurrentIndex(3);
            const auto chooseFile=[&](QPushButton* button,const QString& path){
                qInfo() << "Opening table file dialog" << button->objectName() << path;
                bool chosen=false,timedOut=false;QTimer chooser,watchdog;
                QObject::connect(&chooser,&QTimer::timeout,dialog,[&]{
                    auto* file=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());if(!file)return;
                    file->selectFile(path);
                    // On a reused Open dialog Qt may keep the prior directory
                    // selection. Enter the requested path just as the user does.
                    if(auto* name=file->findChild<QLineEdit*>("fileNameEdit"))name->setText(path);
                    chosen=true;QMetaObject::invokeMethod(file,"accept",Qt::DirectConnection);
                });
                QObject::connect(&watchdog,&QTimer::timeout,dialog,[&]{
                    timedOut=true;
                    auto* active=qobject_cast<QDialog*>(QApplication::activeModalWidget());
                    qCritical() << "Table file dialog timed out" << path << (active?active->windowTitle():QString{});
                    if(active&&active!=dialog)active->reject();
                });
                watchdog.setSingleShot(true);watchdog.start(5000);
                chooser.start(20);button->click();chooser.stop();watchdog.stop();return chosen&&!timedOut;
            };
            auto* importButton=dialog->findChild<QPushButton*>("nativeFunctionImportTable");
            auto* exportButton=dialog->findChild<QPushButton*>("nativeFunctionExportTable");
            fileButtonsUsed=importButton&&exportButton&&chooseFile(importButton,csvPath);
            auto* table=dialog->findChild<QTableWidget*>("nativeFunctionTable");
            fileButtonsUsed=fileButtonsUsed&&table->rowCount()==1025&&chooseFile(importButton,badPath)
                &&table->rowCount()==1025&&chooseFile(exportButton,uiExport);
            dialog->findChild<QDialogButtonBox*>("nativeFunctionButtons")->button(QDialogButtonBox::Ok)->click();
            if(dialog->isVisible()){
                qCritical() << "Table application failed" << dialog->findChild<QLabel*>("nativeFunctionStatus")->text();
                fileButtonsUsed=false;dialog->reject();
            }
        });
        require(editNativeFunction(nullptr,expression,tableX,tableY,"mm",1.024)&&fileButtonsUsed
            &&tableX==longX&&tableY==longY,"Real CSV/TXT dialog controls did not retain the full accepted road input");
        require(readNativeFunctionTable(uiExport,importedX,importedY,error)&&importedX==longX&&importedY==longY,error);
        qInfo() << "Native function editor: real engine values, table knots, errors, dialog application and cancel passed";
        return 0;
    } catch (const std::exception& failure) { qCritical() << failure.what(); return 1; }
}
