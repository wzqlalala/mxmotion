/********************************************************************************
** Form generated from reading UI file 'mPostProbes.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MPOSTPROBES_H
#define UI_MPOSTPROBES_H
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "../mPanelBase.h"

//QT_BEGIN_NAMESPACE
using namespace MechMainWindow;

class Ui_mPostExportWord
{
public:
    QVBoxLayout* _mainLayout;

    FoldSection* _contentFold;                 // 报告内容
    FoldSection* _titleFold;                   // 标题信息
    FoldSection* _executeFold;                   // 执行

    // 报告内容分组
    //QLabel* label;
    QListWidget* listWidget;                    // 报告内容列表
    QCheckBox* checkBox_text;                   // 文本单选
    QCheckBox* checkBox_screenshot;             // 截图单选

    /*QHBoxLayout* horizontalLayout_2;
    QScrollArea* scrollArea;
    QWidget* scrollAreaWidgetContents;
    QGridLayout* gridLayout_2;
    QVBoxLayout* verticalLayout_3;
    QHBoxLayout* horizontalLayout_4;*/
    QHBoxLayout* horizontalLayout_5;

    //
    /*QVBoxLayout* verticalLayout_2;
    QHBoxLayout* horizontalLayout;
    QVBoxLayout* verticalLayout;
    QSpacerItem* verticalSpacer;*/
    QTextEdit* textEdit;                                // 文本编辑框
    //QHBoxLayout* horizontalLayout_3;
    QPushButton* pushButton_saveView;                   // 保存当前视图按钮
    QPushButton* pushButton_editScreenshot;             // 编辑截图按钮

    // 标题信息分组
    //QGroupBox* groupBox;
    //QGridLayout* gridLayout;
    QCheckBox* checkBox_subheader;                      // 副标题
    QLineEdit* lineEdit_subHeader;                      // 副标题
    QCheckBox* checkBox_user;                           // 分析员
    QLineEdit* lineEdit_user;                           // 分析员
    QCheckBox* checkBox_company;                        // 公司
    QLineEdit* lineEdit_company;                        // 公司
    QCheckBox* checkBox_addresa;                        // 地址
    QLineEdit* lineEdit_address;                        // 地址
    QCheckBox* checkBox_phone;                          // 电话
    QLineEdit* lineEdit_phone;                          // 电话
    /*QVBoxLayout* verticalLayout_4;
    QSpacerItem* verticalSpacer_2;*/

    // 执行
    QComboBox* comboBox_format;                   // 导出格式选择
    QCheckBox* pushButton_open;                   // 自动打开按钮
    QPushButton* pushButton_generator;              // 生成按钮
    QSpacerItem* horizontalSpacer_4;

    // 图像/动画输出格式
    QComboBox* comboBox_imageFormat;              // 截图保存格式(PNG/JPEG/BMP/TIFF)
    QComboBox* comboBox_videoFormat;              // 动画输出格式(AVI/GIF/MPEG/MP4)
    QPushButton* pushButton_exportVideo;            // 导出动画按钮

    void setupUi(QWidget *mPostProbes)
    {
        _mainLayout = new QVBoxLayout(mPostProbes);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        /********************视图********************/
        _contentFold = new FoldSection("报告内容", mPostProbes);

        // 视图名称列表
        listWidget = new QListWidget(mPostProbes);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));
        listWidget->setContextMenuPolicy(Qt::NoContextMenu);
        listWidget->setDragDropMode(QAbstractItemView::DragOnly);
        listWidget->setMaximumHeight(160);      // 设置最大高度
        listWidget->setMinimumHeight(160);      // 设置最小高度
        _contentFold->addRow(listWidget);
        listWidget->show();

        checkBox_text = new QCheckBox(mPostProbes);
        checkBox_text->setObjectName(QString::fromUtf8("checkBox_text"));
        checkBox_text->setChecked(false);
        _contentFold->addRow(checkBox_text);
        checkBox_text->show();

        // 文本
        textEdit = new QTextEdit(mPostProbes);
        textEdit->setObjectName(QString::fromUtf8("textEdit"));
        textEdit->setEnabled(false);
        _contentFold->addRow(textEdit);
        //textEdit->hide();
        textEdit->show();

        // 截图
        checkBox_screenshot = new QCheckBox(mPostProbes);
        checkBox_screenshot->setObjectName(QString::fromUtf8("checkBox_screenshot"));
        checkBox_screenshot->setChecked(false);
        pushButton_saveView = new QPushButton(mPostProbes);
        pushButton_saveView->setObjectName(QString::fromUtf8("pushButton_saveView"));
        pushButton_saveView->setEnabled(false);
        pushButton_saveView->setMinimumSize(QSize(80, 0));
        pushButton_editScreenshot = new QPushButton(mPostProbes);
        pushButton_editScreenshot->setObjectName(QString::fromUtf8("pushButton_editScreenshot"));
        pushButton_editScreenshot->setEnabled(false);
        pushButton_editScreenshot->setMinimumSize(QSize(80, 0));
        _contentFold->addRow(checkBox_screenshot, pushButton_saveView, pushButton_editScreenshot);
        checkBox_screenshot->show();
        pushButton_saveView->show();
        pushButton_editScreenshot->show();

        // 图像格式
        comboBox_imageFormat = new QComboBox(mPostProbes);
        comboBox_imageFormat->setObjectName(QString::fromUtf8("comboBox_imageFormat"));
        QStringList imageFormats;
        imageFormats << "PNG" << "JPEG" << "BMP" << "TIFF";
        comboBox_imageFormat->addItems(imageFormats);
        _contentFold->addRow(comboBox_imageFormat);
        comboBox_imageFormat->show();

        // 动画输出格式 + 导出动画
        comboBox_videoFormat = new QComboBox(mPostProbes);
        comboBox_videoFormat->setObjectName(QString::fromUtf8("comboBox_videoFormat"));
        QStringList videoFormats;
        videoFormats << "AVI" << "GIF" << "MPEG" << "MP4";
        comboBox_videoFormat->addItems(videoFormats);
        pushButton_exportVideo = new QPushButton(mPostProbes);
        pushButton_exportVideo->setObjectName(QString::fromUtf8("pushButton_exportVideo"));
        pushButton_exportVideo->setMinimumSize(QSize(80, 0));
        _contentFold->addRow(comboBox_videoFormat, pushButton_exportVideo);
        comboBox_videoFormat->show();
        pushButton_exportVideo->show();

        _contentFold->refreshLayout();
        //m_foldWidgets.append(_viewFold);
        _mainLayout->addWidget(_contentFold);

        /********************标题********************/
        _titleFold = new FoldSection("标题信息", mPostProbes);

        // 副标题
        checkBox_subheader = new QCheckBox(mPostProbes);
        checkBox_subheader->setObjectName(QString::fromUtf8("checkBox_subheader"));
        checkBox_subheader->setChecked(false);
        lineEdit_subHeader = new QLineEdit(mPostProbes);
        lineEdit_subHeader->setObjectName(QString::fromUtf8("lineEdit_subHeader"));
        lineEdit_subHeader->setEnabled(false);
        _titleFold->addRow(checkBox_subheader, lineEdit_subHeader);
        checkBox_subheader->show();
        lineEdit_subHeader->show();

        // 分析员
        checkBox_user = new QCheckBox(mPostProbes);
        checkBox_user->setObjectName(QString::fromUtf8("checkBox_user"));
        checkBox_user->setChecked(false);
        lineEdit_user = new QLineEdit(mPostProbes);
        lineEdit_user->setObjectName(QString::fromUtf8("lineEdit_user"));
        lineEdit_user->setEnabled(false);
        _titleFold->addRow(checkBox_user, lineEdit_user);
        checkBox_user->show();
        lineEdit_user->show();

        // 公司
        checkBox_company = new QCheckBox(mPostProbes);
        checkBox_company->setObjectName(QString::fromUtf8("checkBox_company"));
        checkBox_company->setChecked(false);
        lineEdit_company = new QLineEdit(mPostProbes);
        lineEdit_company->setObjectName(QString::fromUtf8("lineEdit_company"));
        lineEdit_company->setEnabled(false);
        _titleFold->addRow(checkBox_company, lineEdit_company);
        checkBox_company->show();
        lineEdit_company->show();

        // 地址
        checkBox_addresa = new QCheckBox(mPostProbes);
        checkBox_addresa->setObjectName(QString::fromUtf8("checkBox_addresa"));
        checkBox_addresa->setChecked(false);
        lineEdit_address = new QLineEdit(mPostProbes);
        lineEdit_address->setObjectName(QString::fromUtf8("lineEdit_address"));
        lineEdit_address->setEnabled(false);
        _titleFold->addRow(checkBox_addresa, lineEdit_address);
        checkBox_addresa->show();
        lineEdit_address->show();

        // 电话
        checkBox_phone = new QCheckBox(mPostProbes);
        checkBox_phone->setObjectName(QString::fromUtf8("checkBox_phone"));
        checkBox_phone->setChecked(false);
        lineEdit_phone = new QLineEdit(mPostProbes);
        lineEdit_phone->setObjectName(QString::fromUtf8("lineEdit_phone"));
        lineEdit_phone->setEnabled(false);
        _titleFold->addRow(checkBox_phone, lineEdit_phone);
        checkBox_phone->show();
        lineEdit_phone->show();

        _titleFold->refreshLayout();
        //m_foldWidgets.append(_viewFold);
        _mainLayout->addWidget(_titleFold);

        /********************执行********************/
        _executeFold = new FoldSection("执行", mPostProbes);
        /*horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);*/

        //horizontalLayout_5 = new QHBoxLayout();
        //horizontalLayout_5->setSpacing(6);
        //horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));

        comboBox_format = new QComboBox(mPostProbes);
        comboBox_format->setObjectName(QString::fromUtf8("comboBox_format"));
        comboBox_format->setMinimumSize(QSize(100, 0));
        _executeFold->addRow(comboBox_format);

        pushButton_open = new QCheckBox(mPostProbes);
        pushButton_open->setObjectName(QString::fromUtf8("pushButton_open"));
        pushButton_open->setMinimumSize(QSize(80, 0));
        //horizontalLayout_5->addWidget(pushButton_open);

        /*verticalSpacer_3 = new QSpacerItem(20, 118, QSizePolicy::Minimum, QSizePolicy::Expanding);
        horizontalLayout_5->addItem(verticalSpacer_3);*/

        pushButton_generator = new QPushButton(mPostProbes);
        pushButton_generator->setObjectName(QString::fromUtf8("pushButton_generator"));
        pushButton_generator->setMinimumSize(QSize(80, 0));
        pushButton_generator->setCheckable(false);
        _executeFold->addRow(pushButton_open, pushButton_generator);

        _executeFold->refreshLayout();
        //horizontalLayout_5->addWidget(pushButton_generator);

        _mainLayout->addWidget(_executeFold);

        /********************结束********************/
        _mainLayout->addStretch();

        retranslateUi(mPostProbes);

        QMetaObject::connectSlotsByName(mPostProbes);
    } // setupUi

    void retranslateUi(QWidget *mPostProbes)
    {
        //label->setText(QCoreApplication::translate("mPostWordReportPanel", "\346\212\245\345\221\212\345\206\205\345\256\271", nullptr));
        checkBox_text->setText(QCoreApplication::translate("mPostWordReportPanel", "\346\226\207\346\234\254", nullptr));
        checkBox_screenshot->setText(QCoreApplication::translate("mPostWordReportPanel", "\346\210\252\345\233\276", nullptr));
        pushButton_saveView->setText(QCoreApplication::translate("mPostWordReportPanel", "\344\277\235\345\255\230\345\275\223\345\211\215\350\247\206\345\233\276", nullptr));
        pushButton_editScreenshot->setText(QCoreApplication::translate("mPostWordReportPanel", "\347\274\226\350\276\221\346\210\252\345\233\276", nullptr));
        //groupBox->setTitle(QCoreApplication::translate("mPostWordReportPanel", "\346\240\207\351\242\230\344\277\241\346\201\257", nullptr));
        checkBox_subheader->setText(QCoreApplication::translate("mPostWordReportPanel", "\345\211\257\346\240\207\351\242\230", nullptr));
        checkBox_user->setText(QCoreApplication::translate("mPostWordReportPanel", "\345\210\206\346\236\220\345\221\230", nullptr));
        checkBox_company->setText(QCoreApplication::translate("mPostWordReportPanel", "\345\205\254\345\217\270", nullptr));
        checkBox_addresa->setText(QCoreApplication::translate("mPostWordReportPanel", "\345\234\260\345\235\200", nullptr));
        checkBox_phone->setText(QCoreApplication::translate("mPostWordReportPanel", "\347\224\265\350\257\235", nullptr));
        pushButton_open->setText(QCoreApplication::translate("mPostWordReportPanel", "\350\207\252\345\212\250\346\211\223\345\274\200", nullptr));
        pushButton_generator->setText(QCoreApplication::translate("mPostWordReportPanel", "\347\224\237\346\210\220", nullptr));
        pushButton_exportVideo->setText(QCoreApplication::translate("mPostWordReportPanel", "\345\257\274\345\207\272\345\212\250\347\224\273", nullptr));
    } // retranslateUi

    void changeLanguage(int language)
    {
        if (language == 0)
        {
            //label->setText("报告内容");
            checkBox_text->setText("文本");
            checkBox_screenshot->setText("截图");


            //groupBox->setTitle("标题信息");
            checkBox_subheader->setText("副标题");
            checkBox_user->setText("分析员");
            checkBox_company->setText("公司");
            checkBox_addresa->setText("地址");
            checkBox_phone->setText("电话");
            pushButton_generator->setText("生成");
            pushButton_saveView->setText("保存当前视图");
            pushButton_editScreenshot->setText("编辑截图");
            pushButton_open->setText("自动打开");
            pushButton_exportVideo->setText("导出动画");

            if (comboBox_format)
            {
                int idx = comboBox_format->findData("Word");
                if (idx >= 0) comboBox_format->setItemText(idx, "Word");
                idx = comboBox_format->findData("HTML");
                if (idx >= 0) comboBox_format->setItemText(idx, "HTML");
                idx = comboBox_format->findData("PowerPoint");
                if (idx >= 0) comboBox_format->setItemText(idx, "PowerPoint");
            }
        }
        else if (language == 1)
        {
            //label->setText("Report Content");
            checkBox_text->setText("Text");
            checkBox_screenshot->setText("ScreenShot");


            //groupBox->setTitle("Title");
            checkBox_subheader->setText("SubTitle");
            checkBox_user->setText("Analyst");
            checkBox_company->setText("Company");
            checkBox_addresa->setText("Address");
            checkBox_phone->setText("Phone");
            pushButton_generator->setText("Generate");
            pushButton_saveView->setText("Save View");
            pushButton_editScreenshot->setText("Edit View");
            pushButton_open->setText("Auto Open");
            pushButton_exportVideo->setText("Export Animation");

            if (comboBox_format)
            {
                int idx = comboBox_format->findData("Word");
                if (idx >= 0) comboBox_format->setItemText(idx, "Word");
                idx = comboBox_format->findData("HTML");
                if (idx >= 0) comboBox_format->setItemText(idx, "HTML");
                idx = comboBox_format->findData("PowerPoint");
                if (idx >= 0) comboBox_format->setItemText(idx, "PowerPoint");
            }
        }
    }
};

namespace Ui {
    class mPostExportWord : public Ui_mPostExportWord {};
} // namespace Ui

//QT_END_NAMESPACE

#endif // UI_MPOSTPROBES_H
