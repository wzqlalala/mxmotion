#include "mPostExportWord.h"

#include <QFileDialog>
#include <QFile>
#include <QIODevice>
#include <QFont>
#include <QDesktopServices>
#include <QFontMetrics>
#include <QRect>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <QDateTime>
#include <QProcess>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <algorithm>
#include <math.h>
#include <QBuffer>

#include "mCommonFunctions.h"
#include "mOtherFunction.h"
#include "mReportPictureDialog.h"
#include "mOutputMessage.h"
#include "mGlobalSignals.h"
#include "mDataPost1.h"
#include "mAppConfiguration.h"
#include "mIMxdbFile1.h"
#include "../IOFile/mIOFileManager.h"//前IOFile版（MechIOFile命名空间；与PostIOFile同名头区分）
#include "mPropertyBase.h"
#include "mPostAgaModelBuilder.h"
#include "AltairResultIO/aga_data.h"
#include "AltairResultIO/aga_writer.h"
#include "mMaterialManager.h"
#include "mMaterialBase.h"
//#include "pythonCpp.h"
#include "mCaseManager.h"
#include "mCaseBase.h"
#include "mBaseData.h"
#include "mContactBase.h"
#include "mConnectorBase.h"
#include "mConnectorManager.h"
//#include "mDataList.h"
#include "mOneFrameData1.h"
#include "PreViewportWidget.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/REBUILD_PostRend.h"
// S5 门面直接替换：会话头已删除（REBUILD_PostRenderSession 转私有宿主）——
// 值类型经 mxr/view/REBUILD_PostRender.h（内含 REBUILD_PostRenderTypes.h）带入
#include "mMechGlobalSignals.h"
#include "mPostInfoHub.h"

// B1b(M6/V-3)：录制驱动点帧就绪检查（R9 任务二清单项——渐进装载×录制竞态窗口
// 整改）。过渡期录制链保留外壳 getAnimationWindow（G7 record 阶段一 stub，禁迁
// 移），驱动点空判升级为就绪检查：范围内存在未就绪帧即拒绝录制（未就绪帧会截
// 到上一帧陈旧画面——R8 实证）。返回 true=就绪可录（无会话/无装载能力时放行，
// 维持 legacy 装载态现状）
static bool checkRecordFramesReady(REBUILD_PostRend* postRend, int firstFrame, int lastFrame)
{    // S5 门面直接替换：getSession()→getPostRender()（返回 shared_ptr 门面，恒在）；
    // FrameLoadState 改 mxr 作用域（REBUILD_PostRenderTypes.h）。录制链本体
    //（getAnimationWindow 保留路径）按裁定零改动
    auto session = postRend->getPostRender();
    for (int f = qMax(1, firstFrame); f <= lastFrame; ++f)
    {
        if (session->frameLoadState(f) != mxr::FrameLoadState::Ready)
        {            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                QString("帧 %1 数据未就绪（渐进装载中），请稍后重试录制").arg(f), true);
            return false;
        }
    }
    return true;
}
//#include "mOutputDatabase.h"
//#include "mPostDatabaseSingleton.h"
#include "mPostExportWordData.h"
#include "mPostModelData.h"
#include "mDataList.h"
#include "mPostReportContext.h"
#include "mPostReportRenderer.h"
#include "mPostExportAPIRegistrar.h"

using namespace MIOFile;
using namespace MAppConfiguration;
using namespace MechPreData;
using namespace MechPropertyData;
using namespace MechMaterialData;
using namespace MxFunctions;
using namespace MechCaseData;
using namespace MechConnectorData;
using namespace MechContactData;

namespace PostWindow
{
    mPostExportWordPanel::mPostExportWordPanel(QWidget* parent) : mPanelBase(parent)
    {
        setupUI();

        // 与云图面板统一：折叠区注册到基类统一管理
        m_foldWidgets.append(_contentFold);
        m_foldWidgets.append(_titleFold);
        m_foldWidgets.append(_executeFold);

        //截图窗口
        _pictureDialog = new mReportPictureDialog(this);

        //初始化界面：只清空列表，不删除图片目录，避免多节点图片互相清除
        clearListWidget(_listWidget);

        //连接信号与槽
        connect(_listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(slot_clickContentList(QListWidgetItem*)));
        connect(_checkBox_text, SIGNAL(toggled(bool)), this, SLOT(slot_textCheckBox(bool)));
        connect(_textEdit, SIGNAL(textChanged()), this, SLOT(slot_textChanged()));
        connect(_checkBox_screenshot, SIGNAL(toggled(bool)), this, SLOT(slot_pictureCheckBox(bool)));
        connect(_pushButton_saveView, SIGNAL(clicked()), this, SLOT(slot_savePictureButton()));
        connect(_pushButton_editScreenshot, SIGNAL(clicked()), this, SLOT(slot_editPictureButton()));
        connect(_checkBox_subheader, SIGNAL(toggled(bool)), this, SLOT(slot_titleCheckBox(bool)));
        connect(_checkBox_user, SIGNAL(toggled(bool)), this, SLOT(slot_personCheckBox(bool)));
        connect(_checkBox_company, SIGNAL(toggled(bool)), this, SLOT(slot_companyCheckBox(bool)));
        connect(_checkBox_addresa, SIGNAL(toggled(bool)), this, SLOT(slot_addressCheckBox(bool)));
        connect(_checkBox_phone, SIGNAL(toggled(bool)), this, SLOT(slot_phoneCheckBox(bool)));
        connect(_lineEdit_subHeader, SIGNAL(textChanged(QString)), this, SLOT(slot_titleTextChanged()));
        connect(_lineEdit_user, SIGNAL(textChanged(QString)), this, SLOT(slot_userTextChanged()));
        connect(_lineEdit_company, SIGNAL(textChanged(QString)), this, SLOT(slot_companyTextChanged()));
        connect(_lineEdit_address, SIGNAL(textChanged(QString)), this, SLOT(slot_addressTextChanged()));
        connect(_lineEdit_phone, SIGNAL(textChanged(QString)), this, SLOT(slot_phoneTextChanged()));
        connect(_pushButton_generator, SIGNAL(clicked()), this, SLOT(slot_generateButton()));
        connect(_pushButton_open, SIGNAL(clicked()), this, SLOT(slot_autoOpen()));
        connect(_comboBox_format, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_formatChanged(int)));
        connect(_pushButton_exportVideo, SIGNAL(clicked()), this, SLOT(slot_exportAnimation()));

        //初始化导出格式
        initFormatComboBox();

        //切换界面语言
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
        slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

        this->setMinimumWidth(300);

        _grabInited = false;
    }

    mPostExportWordPanel::~mPostExportWordPanel()
    {
    }

    /*
    * 构建面板控件（结构由原 UI/UI_PostExportWord.h 内联）：
    * 折叠组"报告内容"（内容列表/文本/截图/图像格式/动画格式+导出动画）
    * + "标题信息"（副标题/分析员/公司/地址/电话）+ "执行"（导出格式/自动打开/生成）。
    */
    void mPostExportWordPanel::setupUI()
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        /********************视图********************/
        _contentFold = new FoldSection("报告内容", this);

        // 视图名称列表
        _listWidget = new QListWidget(this);
        _listWidget->setObjectName(QString::fromUtf8("listWidget"));
        _listWidget->setContextMenuPolicy(Qt::NoContextMenu);
        _listWidget->setDragDropMode(QAbstractItemView::DragOnly);
        _listWidget->setMaximumHeight(160);      // 设置最大高度
        _listWidget->setMinimumHeight(160);      // 设置最小高度
        _contentFold->addRow(_listWidget);
        _listWidget->show();

        _checkBox_text = new QCheckBox(this);
        _checkBox_text->setObjectName(QString::fromUtf8("checkBox_text"));
        _checkBox_text->setChecked(false);
        _checkBox_text->setText("文本");
        _contentFold->addRow(_checkBox_text);
        _checkBox_text->show();

        // 文本
        _textEdit = new QTextEdit(this);
        _textEdit->setObjectName(QString::fromUtf8("textEdit"));
        _textEdit->setEnabled(false);
        _contentFold->addRow(_textEdit);
        _textEdit->show();

        // 截图
        _checkBox_screenshot = new QCheckBox(this);
        _checkBox_screenshot->setObjectName(QString::fromUtf8("checkBox_screenshot"));
        _checkBox_screenshot->setChecked(false);
        _checkBox_screenshot->setText("截图");
        _pushButton_saveView = new QPushButton(this);
        _pushButton_saveView->setObjectName(QString::fromUtf8("pushButton_saveView"));
        _pushButton_saveView->setEnabled(false);
        _pushButton_saveView->setMinimumSize(QSize(80, 0));
        _pushButton_saveView->setText("保存当前视图");
        _pushButton_editScreenshot = new QPushButton(this);
        _pushButton_editScreenshot->setObjectName(QString::fromUtf8("pushButton_editScreenshot"));
        _pushButton_editScreenshot->setEnabled(false);
        _pushButton_editScreenshot->setMinimumSize(QSize(80, 0));
        _pushButton_editScreenshot->setText("编辑截图");
        _contentFold->addRow(_checkBox_screenshot, _pushButton_saveView, _pushButton_editScreenshot);
        _checkBox_screenshot->show();
        _pushButton_saveView->show();
        _pushButton_editScreenshot->show();

        // 图像格式
        _comboBox_imageFormat = new QComboBox(this);
        _comboBox_imageFormat->setObjectName(QString::fromUtf8("comboBox_imageFormat"));
        QStringList imageFormats;
        imageFormats << "PNG" << "JPEG" << "BMP" << "TIFF";
        _comboBox_imageFormat->addItems(imageFormats);
        _contentFold->addRow(_comboBox_imageFormat);
        _comboBox_imageFormat->show();

        // 动画输出格式 + 导出动画
        _comboBox_videoFormat = new QComboBox(this);
        _comboBox_videoFormat->setObjectName(QString::fromUtf8("comboBox_videoFormat"));
        QStringList videoFormats;
        videoFormats << "AVI" << "GIF" << "MPEG" << "MP4" << "H3D";
        _comboBox_videoFormat->addItems(videoFormats);
        _pushButton_exportVideo = new QPushButton(this);
        _pushButton_exportVideo->setObjectName(QString::fromUtf8("pushButton_exportVideo"));
        _pushButton_exportVideo->setMinimumSize(QSize(80, 0));
        _pushButton_exportVideo->setText("导出动画");
        _contentFold->addRow(_comboBox_videoFormat, _pushButton_exportVideo);
        _comboBox_videoFormat->show();
        _pushButton_exportVideo->show();

        _contentFold->refreshLayout();
        _mainLayout->addWidget(_contentFold);

        /********************标题********************/
        _titleFold = new FoldSection("标题信息", this);

        // 副标题
        _checkBox_subheader = new QCheckBox(this);
        _checkBox_subheader->setObjectName(QString::fromUtf8("checkBox_subheader"));
        _checkBox_subheader->setChecked(false);
        _checkBox_subheader->setText("副标题");
        _lineEdit_subHeader = new QLineEdit(this);
        _lineEdit_subHeader->setObjectName(QString::fromUtf8("lineEdit_subHeader"));
        _lineEdit_subHeader->setEnabled(false);
        _titleFold->addRow(_checkBox_subheader, _lineEdit_subHeader);
        _checkBox_subheader->show();
        _lineEdit_subHeader->show();

        // 分析员
        _checkBox_user = new QCheckBox(this);
        _checkBox_user->setObjectName(QString::fromUtf8("checkBox_user"));
        _checkBox_user->setChecked(false);
        _checkBox_user->setText("分析员");
        _lineEdit_user = new QLineEdit(this);
        _lineEdit_user->setObjectName(QString::fromUtf8("lineEdit_user"));
        _lineEdit_user->setEnabled(false);
        _titleFold->addRow(_checkBox_user, _lineEdit_user);
        _checkBox_user->show();
        _lineEdit_user->show();

        // 公司
        _checkBox_company = new QCheckBox(this);
        _checkBox_company->setObjectName(QString::fromUtf8("checkBox_company"));
        _checkBox_company->setChecked(false);
        _checkBox_company->setText("公司");
        _lineEdit_company = new QLineEdit(this);
        _lineEdit_company->setObjectName(QString::fromUtf8("lineEdit_company"));
        _lineEdit_company->setEnabled(false);
        _titleFold->addRow(_checkBox_company, _lineEdit_company);
        _checkBox_company->show();
        _lineEdit_company->show();

        // 地址
        _checkBox_addresa = new QCheckBox(this);
        _checkBox_addresa->setObjectName(QString::fromUtf8("checkBox_addresa"));
        _checkBox_addresa->setChecked(false);
        _checkBox_addresa->setText("地址");
        _lineEdit_address = new QLineEdit(this);
        _lineEdit_address->setObjectName(QString::fromUtf8("lineEdit_address"));
        _lineEdit_address->setEnabled(false);
        _titleFold->addRow(_checkBox_addresa, _lineEdit_address);
        _checkBox_addresa->show();
        _lineEdit_address->show();

        // 电话
        _checkBox_phone = new QCheckBox(this);
        _checkBox_phone->setObjectName(QString::fromUtf8("checkBox_phone"));
        _checkBox_phone->setChecked(false);
        _checkBox_phone->setText("电话");
        _lineEdit_phone = new QLineEdit(this);
        _lineEdit_phone->setObjectName(QString::fromUtf8("lineEdit_phone"));
        _lineEdit_phone->setEnabled(false);
        _titleFold->addRow(_checkBox_phone, _lineEdit_phone);
        _checkBox_phone->show();
        _lineEdit_phone->show();

        _titleFold->refreshLayout();
        _mainLayout->addWidget(_titleFold);

        /********************执行********************/
        _executeFold = new FoldSection("执行", this);

        _comboBox_format = new QComboBox(this);
        _comboBox_format->setObjectName(QString::fromUtf8("comboBox_format"));
        _comboBox_format->setMinimumSize(QSize(100, 0));
        _executeFold->addRow(_comboBox_format);

        _pushButton_open = new QCheckBox(this);
        _pushButton_open->setObjectName(QString::fromUtf8("pushButton_open"));
        _pushButton_open->setMinimumSize(QSize(80, 0));
        _pushButton_open->setText("自动打开");

        _pushButton_generator = new QPushButton(this);
        _pushButton_generator->setObjectName(QString::fromUtf8("pushButton_generator"));
        _pushButton_generator->setMinimumSize(QSize(80, 0));
        _pushButton_generator->setCheckable(false);
        _pushButton_generator->setText("生成");
        _executeFold->addRow(_pushButton_open, _pushButton_generator);

        _executeFold->refreshLayout();
        _mainLayout->addWidget(_executeFold);

        /********************结束********************/
        _mainLayout->addStretch();
    }

    void mPostExportWordPanel::editExportWordForViewport(int caseid)
    {
        //视口绑定打开（用户定案 2026-11-04）：报告不按次生成实例——数据键=当前激活
        //后处理视口名：同视口重复打开沿用既有配置，切换视口后打开自动跟随新视口。
        //条目登记按视口名幂等（ExportWordData 挂在条目上，无条目则 addExportWordData
        //静默无效，故首开该视口时登记并初始化数据）
        auto* curPriv = VMManager->getCurrentViewportPrivate();
        if (curPriv == nullptr || curPriv->getViewportType() != MxFunctions::ViewportPost3D)
        {
            return;
        }
        const QString viewName = curPriv->getViewportName();
        if (viewName.isEmpty())
        {
            return;
        }

        _curViewName = viewName;

        auto& mgr = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post);
        if (!mgr.hasItemByViewName(PostData::ExportWordItem, viewName))
        {
            int exportWordId = mgr.getItemId(PostData::ExportWordItem);
            PostData::ExportWordInfo info;
            info.caseId = caseid;
            info._ExportWordId = exportWordId;
            info.itemName = QString("报告_%1").arg(exportWordId);
            info.viewName = viewName;
            mgr.addItem(PostData::ExportWordItem, info);

            updateExportWordPanel(caseid);
        }

        updateExportWordGUI();

        QWidget* subwin = getCVM->activatePostViewport();
        if (subwin)
        {
            if (!_grabInited) {
                // 预热改为离屏抓取：QWidget::grab() 强制视口完成一次 OpenGL 渲染合成，
                // 使帧缓冲/DWM 后备存储有内容，避免后续屏拷截图首图发黑；
                // 不触碰屏幕、不调用 winId()（winId 会强制窗口本地化，导致界面闪黑）
                subwin->grab();
                _grabInited = true;
            }
        }
    }

    void mPostExportWordPanel::updateExportWordGUI()
    {
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;

        blockAllSignals(true);

        // 先重置右侧编辑器
        _checkBox_text->setChecked(false);
        _textEdit->clear();
        _textEdit->setEnabled(false);
        _checkBox_screenshot->setChecked(false);
        _pushButton_saveView->setEnabled(false);
        _pushButton_editScreenshot->setEnabled(false);
        _pictureDialog->updatePictureList(QStringList());

        //设置标题信息窗口状态
        _checkBox_subheader->setChecked(curveData->_reportShowSubTitle);
        _checkBox_user->setChecked(curveData->_reportShowUser);
        _checkBox_company->setChecked(curveData->_reportShowCompany);
        _checkBox_addresa->setChecked(curveData->_reportShowAddress);
        _checkBox_phone->setChecked(curveData->_reportShowPhone);

        _lineEdit_subHeader->setText(curveData->_reportSubTitle);
        _lineEdit_user->setText(curveData->_reportUser);
        _lineEdit_company->setText(curveData->_reportCompany);
        _lineEdit_address->setText(curveData->_reportAddress);
        _lineEdit_phone->setText(curveData->_reportPhone);

        _listWidget->clear();

        // 记录当前应选中的类型：优先恢复上一次保存的类型，否则默认第一个
        QString targetType = _currentContentType;
        QListWidgetItem* targetItem = nullptr;
        int row = 0;
        for (auto& content : curveData->_reportContent)
        {
            if (content.bShowInList)
            {
                QString _typeStr = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0 ? content.typeStr : content.typeStrEn;
                QListWidgetItem* item = new QListWidgetItem(_typeStr);
                item->setCheckState(content.bSelected ? Qt::Checked : Qt::Unchecked);
                _listWidget->addItem(item);

                if (targetType.isEmpty() || _typeStr == targetType)
                {
                    targetItem = item;
                    targetType = _typeStr;
                }
                ++row;
            }
        }

        if (targetItem)
        {
            _listWidget->setCurrentItem(targetItem);
            setCurrentContentType(targetType);
            refreshEditorByData(targetType);
        }
        else
        {
            setCurrentContentType(QString());
            _pictureDialog->setImageDir(getExportWordImageDir());
            _pictureDialog->updatePictureList(QStringList());
        }

        blockAllSignals(false);
    }

    void mPostExportWordPanel::updateExportWordPanel(int caseid)
    {
        QPointer<PostData::mPostExportWordData> curveData = new PostData::mPostExportWordData();
        if (!curveData)
            return;

        getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addExportWordData(_curViewName, curveData);

        QStringList contents;
        QStringList contents_en;
        //if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
        {
            contents = QStringList{ QString("说明"),QString("模型"),QString("材料"),QString("截面属性"),
                                QString("分析工况"),QString("载荷"),QString("边界"),QString("连接"),
                                QString("接触"),QString("结果") };
        }
        //if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
        {
            contents_en = QStringList{ QString("Specification"),QString("Model"),QString("Materials"),QString("Property"),
                    QString("Case"),QString("Load"),QString("Boundary"),QString("Connect"),QString("Contact"),QString("Result") };
        }

        curveData->_reportContent.clear();
        int contentCnt = contents.size();
        for (int inx = 0; inx < contentCnt; inx ++) {
            Type_WordContent _listcontent;
            _listcontent.typeStr = contents[inx];
            _listcontent.typeStrEn = contents_en[inx];
            _listcontent.bShowInList = true;
            _listcontent.bSelected = true;
            _listcontent.ContentChecked = false;
            _listcontent.pictureChecked = false;

            curveData->_reportContent.push_back(_listcontent);
        }

        curveData->_reportShowSubTitle = false;
        curveData->_reportShowUser = false;
        curveData->_reportShowCompany = false;
        curveData->_reportShowAddress = false;
        curveData->_reportShowPhone = false;
    }

    void mPostExportWordPanel::initReportPanel()
    {
        // 仅清空列表内容，不再删除图片目录。
        // 截图按 _curViewName 子目录隔离，避免多节点互相删除。
        clearListWidget(_listWidget);
    }

    PostData::mPostExportWordData* mPostExportWordPanel::getExportWordData() const
    {
        if (_curViewName.isEmpty())
            return nullptr;
        return getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getExportWordData(_curViewName);
    }

    QString mPostExportWordPanel::getExportWordImageDir() const
    {
        QString exePath = QCoreApplication::applicationDirPath();
        QString baseDir = exePath + "/Report/post-images";
        if (_curViewName.isEmpty())
            return baseDir;
        return baseDir + "/" + _curViewName;
    }

    PostData::Type_WordContent* mPostExportWordPanel::findWordContent(PostData::mPostExportWordData* data, const QString& type) const
    {
        if (data == nullptr || type.isEmpty())
            return nullptr;
        for (auto& content : data->_reportContent)
        {
            if (content.typeStr == type || content.typeStrEn == type)
                return const_cast<PostData::Type_WordContent*>(&content);
        }
        return nullptr;
    }

    void mPostExportWordPanel::blockAllSignals(bool block)
    {
        _listWidget->blockSignals(block);
        _checkBox_text->blockSignals(block);
        _textEdit->blockSignals(block);
        _checkBox_screenshot->blockSignals(block);
        _pushButton_saveView->blockSignals(block);
        _pushButton_editScreenshot->blockSignals(block);
        _checkBox_subheader->blockSignals(block);
        _checkBox_user->blockSignals(block);
        _checkBox_company->blockSignals(block);
        _checkBox_addresa->blockSignals(block);
        _checkBox_phone->blockSignals(block);
        _lineEdit_subHeader->blockSignals(block);
        _lineEdit_user->blockSignals(block);
        _lineEdit_company->blockSignals(block);
        _lineEdit_address->blockSignals(block);
        _lineEdit_phone->blockSignals(block);
    }

    QString mPostExportWordPanel::currentContentType() const
    {
        return _currentContentType;
    }

    void mPostExportWordPanel::setCurrentContentType(const QString& type)
    {
        _currentContentType = type;
    }

    void mPostExportWordPanel::refreshEditorByData(const QString& contentType)
    {
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;

        PostData::Type_WordContent* content = findWordContent(curveData, contentType);
        if (content == nullptr)
            return;

        _checkBox_text->setChecked(content->ContentChecked);
        _textEdit->setEnabled(content->ContentChecked);
        if (content->ContentChecked)
            _textEdit->setText(content->ContentStr);
        else
            _textEdit->clear();

        _checkBox_screenshot->setChecked(content->pictureChecked);
        _pushButton_saveView->setEnabled(content->pictureChecked);
        _pushButton_editScreenshot->setEnabled(content->pictureChecked);

        _pictureDialog->setImageDir(getExportWordImageDir());
        _pictureDialog->updatePictureList(content->pictureList);
    }

    void mPostExportWordPanel::clearAllContainer()
    {
        html_.clear();
        piclistModel_.clear();
        piclistMaterial_.clear();
        piclistProperty_.clear();
        piclistCase_.clear();
        piclistLoad_.clear();
        piclistBoundary_.clear();
        listPartInfo_.clear();
        listMaterialInfo_.clear();
        listPropertyInfo_.clear();
        listCaseInfo_.clear();
        listLoadInfo_.clear();
        listBoundaryInfo_.clear();
        piclistResult_.clear();

        _modelPics.clear();
        _materialPics.clear();
        _propertyPics.clear();
        _casePics.clear();
        _loadPics.clear();
        _boundaryPics.clear();
        _connectPics.clear();
        _contactPics.clear();
        _resultPics.clear();
    }

    void mPostExportWordPanel::copyPictureFolder(QString src, QString dst)
    {
        QDir source(src);
        QDir destination(dst);

        if (destination.exists())
        {
            destination.removeRecursively();
            destination.mkdir(dst);
        }
        else
            destination.mkdir(dst);

        // 遍历源目录中的所有条目  
        QFileInfoList entries = source.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
        for (const QFileInfo& entry : entries) {
            QString srcPath = entry.absoluteFilePath();
            QString dstPath = destination.absoluteFilePath(entry.fileName());

            if (entry.isDir())
            {
                // 如果是目录，则递归调用copyDir  
                copyPictureFolder(srcPath, dstPath);
            }
            else
            {
                // 如果是文件，则拷贝文件  
                QFile srcFile(srcPath);
                QFile dstFile(dstPath);
                srcFile.copy(dstPath);
            }
        }
    }

    void mPostExportWordPanel::getCurrentTime()
    {
        QDateTime currentDateTime = QDateTime::currentDateTime();
        testDate_ = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
    }

    QString mPostExportWordPanel::getmxFileName(QString dir)
    {
        QDir exePath(dir);
        QStringList mxFileNames;
        if (exePath.exists())
        {
            //获取文件夹中的所有文件
            QFileInfoList files = exePath.entryInfoList(QDir::Files);
            //遍历文件列表，筛选出txt文件
            for (const QFileInfo& fileinfo : files)
            {
                if (fileinfo.suffix() == "txt")
                {
                    mxFileNames.append(fileinfo.fileName());
                }
            }
        }
        if (mxFileNames.size() != 0)
        {
            return mxFileNames[0];
        }
        return QString();
    }

    void mPostExportWordPanel::getModelImageFiles(QDir reportDir)
    {
        QStringList filter;
        filter << "*.png";
        QString exePath = QCoreApplication::applicationDirPath();
        reportDir.setNameFilters(filter);
        QFileInfoList files = reportDir.entryInfoList(QDir::Files);
        QString dir = "images/";
        for (const QFileInfo& fileinfo : files)
        {
            QString file = fileinfo.fileName();
            if (file.startsWith("model_"))
            {
                //piclistModel_.append(dir + file);
                _modelPics.append(fileinfo.absoluteFilePath());
            }
            else if (file.startsWith("material_"))
            {
                //piclistMaterial_.append(dir + file);
                _materialPics.append(fileinfo.absoluteFilePath());
            }
            else if (file.startsWith("property_"))
            {
                //piclistProperty_.append(dir + file);
                _propertyPics.append(fileinfo.absoluteFilePath());
            }
            else if (file.startsWith("case_"))
            {
                //piclistCase_.append(dir + file);
                _casePics.append(fileinfo.absoluteFilePath());
            }
            else if (file.startsWith("load_"))
            {
                //piclistLoad_.append(dir + file);
                _loadPics.append(fileinfo.absoluteFilePath());
            }
            else if (file.startsWith("boundary_"))
            {
                //piclistBoundary_.append(dir + file);
                _boundaryPics.append(fileinfo.absoluteFilePath());
            }
            else if (file.startsWith("connect_"))
            {
                //piclistBoundary_.append(dir + file);
                _connectPics.append(fileinfo.absoluteFilePath());
            }
            else if (file.startsWith("contact_"))
            {
                //piclistBoundary_.append(dir + file);
                _contactPics.append(fileinfo.absoluteFilePath());
            }
            else if (file.startsWith("postResult_"))
            {
                //piclistResult_.append(dir + file);
                _resultPics.append(fileinfo.absoluteFilePath());
            }
        }
    }

    //void mPostExportWordPanel::getModelDescription(QString dir)
    //{
    //    MechModelData::mModelData* _modelData = getCVM->getModelData();
    //    // 整理部件数据  listPartInfo_
    //    QHash<int, mPartData*>& partList = _modelData->getMeshData()->GetAllParts();
    //    int PartCount = partList.size();
    //    QHash<int, mPartData*>::const_iterator partIter;
    //    for (partIter = partList.constBegin(); partIter != partList.constEnd(); partIter++) {
    //        PartInfo part;
    //        // 获取部件ID和名称
    //        int partId = partIter.key();
    //        mPartData* _partData = partIter.value();
    //        QString partName = _partData->getPartName();
    //        // 写ID和名称
    //        part.paraNames.append("部件名");
    //        part.paraContents.append(partName);

    //        if (1)
    //        {
    //            // 获取截面属性ID
    //            int sectionID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Part, partId }, Physical_Property);
    //            mPropertyBase* sectionBase = _modelData->getManager<mPropertyManager>(Physical_Property).getPropertyData(sectionID);
    //            if (sectionBase) {
    //                // 写部件截面属性
    //                part.paraNames.push_back("属性名");
    //                part.paraContents.append(sectionBase->getPropertyName());

    //                int matID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Property, sectionID }, Physical_Material);
    //                std::shared_ptr<mMaterialBase> _materialBase = _modelData->getManager<mMaterialManager>(Physical_Material).getMaterialData(matID);
    //                if (_materialBase) {
    //                    //写部件材料名
    //                    part.paraNames.push_back("材料名");
    //                    part.paraContents.append(_materialBase->getMaterialName());

    //                    // 写部件材料类型
    //                    part.paraNames.push_back("材料类型");
    //                    part.paraContents.append(_materialBase->getMaterialTypeStr());
    //                }
    //                else {
    //                    mGlobalSignals::getInstance()->outputMessageSig(QString("部件“%1”没有材料属性数据，无法输出材料名及材料类型!").arg(partName));
    //                }
    //            }
    //            else {
    //                mGlobalSignals::getInstance()->outputMessageSig(QString("部件“%1”没有截面属性数据，无法输出截面属性及材料属性!").arg(partName));
    //            }
    //        }
    //        else {
    //            mGlobalSignals::getInstance()->outputMessageSig(QString("部件“%1”没有部件属性数据，无法输出截面属性及材料属性!").arg(partName));
    //        }

    //        // 写部件节点数
    //        part.paraNames.push_back("节点数");
    //        part.paraContents.append(MxFunctions::mNumberToString(_partData->getTotalNodeNum()));

    //        // 写部件单元数
    //        part.paraNames.push_back("单元数");
    //        part.paraContents.append(MxFunctions::mNumberToString(_partData->getTotalElementNum()));

    //        listPartInfo_.append(part);
    //    }
    //    
    //    // 整理材质数据   listMaterialInfo_
    //    std::map<int, std::shared_ptr<mMaterialBase>>& _allMaterials = _modelData->getManager<mMaterialManager>(Physical_Material).GetAllMaterials();
    //    int materialNum = _allMaterials.size();
    //    for (auto& id2Material : _allMaterials) {
    //        PartInfo material;
    //        std::shared_ptr<mMaterialBase> tepmat = id2Material.second;
    //        if (tepmat != nullptr)
    //        {
    //            //材料名
    //            material.paraNames.push_back("材料");
    //            material.paraContents.push_back(tepmat->getMaterialName());

    //            //材料类型
    //            QString mattype = tepmat->getMaterialTypeStr();
    //            material.paraNames.push_back("材料类型");
    //            material.paraContents.push_back(mattype);

    //            //其他参数
    //            QHash<MxFunctions::MaterialType, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = tepmat->getAllProps();
    //            QHash<MxFunctions::MaterialPropType, QVariant> props = matProp.value(tepmat->getMaterialType());

    //            QHash<MxFunctions::MaterialPropType, QVariant>::const_iterator propIter;
    //            for (propIter = props.constBegin(); propIter != props.constEnd(); propIter++) {
    //                material.paraNames.push_back(MxFunctions::mOtherFunction::getMaterialPropStr(propIter.key()));
    //                const QVariant& _value = propIter.value();
    //                // if (_value.canConvert<QString>())
    //                if (_value.isValid()) {
    //                    switch (_value.type()) {
    //                    case QMetaType::Int:
    //                    {
    //                        material.paraContents.push_back(MxFunctions::mNumberToString((_value.toInt())));
    //                        break;
    //                    }
    //                    case QMetaType::QString:
    //                    {
    //                        material.paraContents.push_back(_value.toString());
    //                        break;
    //                    }
    //                    case QMetaType::Double:
    //                    {
    //                        material.paraContents.push_back(MxFunctions::mNumberToString((_value.toDouble())));
    //                        break;
    //                    }
    //                    default:
    //                    {
    //                        material.paraContents.push_back("");
    //                        mGlobalSignals::getInstance()->outputMessageSig(QString("未处理的材质属性数据类型（“%1”）!").arg(_value.type()));
    //                        break;
    //                    }
    //                    }
    //                }
    //                else {
    //                    material.paraContents.push_back("");
    //                }
    //            }
    //        }

    //        listMaterialInfo_.append(material);
    //    }

    //    // 整理属性数据  listPropertyInfo_
    //     std::vector<int> propIds = _modelData->getAllPhyID(Physical_Property);  //getManager<mPropertyManager>(Physical_Property).getPropertyData(sectionID);
    //     for (auto id : propIds) {
    //         mPropertyBase* sectionBase = _modelData->getManager<mPropertyManager>(Physical_Property).getPropertyData(id);
    //         if (sectionBase) {
    //             PartInfo property;
    //             //截面属性名
    //             property.paraNames.push_back("截面属性名");
    //             property.paraContents.push_back(sectionBase->getPropertyName());

    //             //类型
    //             property.paraNames.push_back("类型");
    //             property.paraContents.push_back(sectionBase->getPropertyTypeStr());

    //             // todo: 没有getParameterList函数统计属性，没法统计其他参数
    //             mDataList datalist;
    //             //其他参数
    //             sectionBase->getParameterList(datalist);

    //             //填充输出数据
    //             const int othercount = datalist.getDataCount();
    //             for (int j = 0; j < othercount; ++j)
    //             {
    //                 mBaseData* tepData = datalist.getData(j);
    //                 if (tepData == nullptr)
    //                 {
    //                     continue;
    //                 }
    //                 QString name = tepData->getDataName();
    //                 QString value = tepData->getDataVariant().toString();

    //                 property.paraNames.push_back(name);
    //                 property.paraContents.push_back(value);
    //             }

    //             listPropertyInfo_.append(property);
    //         }
    //     }

    //     // 整理工况数据  listCaseInfo_
    //     std::map<int, MechCaseData::mCaseBase*>& _allCases = _modelData->getManager<mCaseManager>(Physical_Case).GetAllCases();
    //     int caseNum = _allCases.size();
    //     for (auto& id2Case : _allCases) {
    //         PartInfo caseinfo;
    //         MechCaseData::mCaseBase* tempCase = id2Case.second;
    //         if (tempCase != nullptr)
    //         {
    //             //分析工况名
    //             caseinfo.paraNames.push_back("分析类型名");
    //             caseinfo.paraContents.push_back(tempCase->getCaseName());
    //             //分析工况类型
    //             QString casetype = tempCase->getCaseTypeStr();
    //             caseinfo.paraNames.push_back("分析类型");
    //             caseinfo.paraContents.push_back(casetype);
    //             //输出范围
    //            // QString caseOutPutSetName = tempCase->getCaseOutPutSetName();
    //             if (1)//(caseOutPutSetName.isEmpty())
    //             {
    //                 caseinfo.paraNames.push_back("输出范围");
    //                 caseinfo.paraContents.push_back("全部输出");
    //             }
    //             /*else
    //             {
    //                 caseinfo.paraNames.push_back("输出范围");
    //                 caseinfo.paraContents.push_back(caseOutPutSetName);
    //             }*/

    //             mDataList datalist;
    //             //其他参数
    //             tempCase->getParameterList(datalist);

    //             //填充输出数据
    //             const int othercount = datalist.getDataCount();
    //             for (int j = 0; j < othercount; ++j)
    //             {
    //                 mBaseData* tepData = datalist.getData(j);
    //                 if (tepData == nullptr)
    //                 {
    //                     continue;
    //                 }
    //                 QString name = tepData->getDataName();
    //                 QString value = tepData->getDataVariant().toString();

    //                 caseinfo.paraNames.push_back(name);
    //                 caseinfo.paraContents.push_back(value);
    //             }

    //             listCaseInfo_.append(caseinfo);
    //         }
    //     }


    //     // 整理载荷数据  listLoadInfo_
    //     for (auto& id2Case : _allCases) {
    //         PartInfo caseinfo;
    //         MechCaseData::mCaseBase* tempCase = id2Case.second;
    //         if (tempCase != nullptr)
    //         {
    //             std::unordered_map<int, std::shared_ptr<MechCaseData::mLoadBase>>& _allLoads = tempCase->GetAllLoads();
    //             for (auto& id2Load : _allLoads) {
    //                 std::shared_ptr<MechCaseData::mLoadBase> tepLoad = id2Load.second;
    //                 if (tepLoad != nullptr)
    //                 {
    //                     PartInfo load;

    //                     //载荷名
    //                     load.paraNames.push_back("载荷名");
    //                     load.paraContents.push_back(tepLoad->getLoadName());

    //                     //载荷类型
    //                     QString loadtype = tepLoad->getLoadTypeStr();
    //                     load.paraNames.push_back("载荷类型");
    //                     load.paraContents.push_back(loadtype);

    //                     //分析工况
    //                     QString loadcase = "";//mContainerOperation::transformQStringListToQString(tepLoad->_loadcase);
    //                     load.paraNames.push_back("分析工况");
    //                     load.paraContents.push_back(loadcase);

    //                     mDataList datalist;
    //                     //其他参数
    //                     tepLoad->getParameterList(datalist);
    //                     //填充输出数据
    //                     const int othercount = datalist.getDataCount();
    //                     for (int j = 0; j < othercount; ++j)
    //                     {
    //                         mBaseData* tepData = datalist.getData(j);
    //                         if (tepData == nullptr)
    //                         {
    //                             continue;
    //                         }
    //                         QString name = tepData->getDataName();
    //                         QString value = tepData->getDataVariant().toString();

    //                         load.paraNames.push_back(name);
    //                         load.paraContents.push_back(value);
    //                     }
    //                     listLoadInfo_.push_back(load);
    //                 }
    //             }
    //         }
    //     }

    //     // 整理边界数据    listBoundaryInfo_
    //     for (auto& id2Case : _allCases) {
    //         PartInfo caseinfo;
    //         MechCaseData::mCaseBase* tempCase = id2Case.second;
    //         if (tempCase != nullptr)
    //         {
    //             std::unordered_map<int, std::shared_ptr<MechCaseData::mBoundaryBase>>& _allLoads = tempCase->GetAllBoundarys();
    //             for (auto& id2Load : _allLoads) {
    //                 std::shared_ptr<MechCaseData::mBoundaryBase> tepLoad = id2Load.second;
    //                 if (tepLoad != nullptr)
    //                 {
    //                     PartInfo load;

    //                     //载荷名
    //                     load.paraNames.push_back("边界名");
    //                     load.paraContents.push_back(tepLoad->getBoundaryName());

    //                     //载荷类型
    //                     QString loadtype = tepLoad->getBoundaryTypeStr();
    //                     load.paraNames.push_back("边界类型");
    //                     load.paraContents.push_back(loadtype);

    //                     //分析工况
    //                     QString loadcase = "";//mContainerOperation::transformQStringListToQString(tepLoad->_loadcase);
    //                     load.paraNames.push_back("分析工况");
    //                     load.paraContents.push_back(loadcase);

    //                     mDataList datalist;
    //                     //其他参数
    //                     tepLoad->getParameterList(datalist);
    //                     //填充输出数据
    //                     const int othercount = datalist.getDataCount();
    //                     for (int j = 0; j < othercount; ++j)
    //                     {
    //                         mBaseData* tepData = datalist.getData(j);
    //                         if (tepData == nullptr)
    //                         {
    //                             continue;
    //                         }
    //                         QString name = tepData->getDataName();
    //                         QString value = tepData->getDataVariant().toString();

    //                         load.paraNames.push_back(name);
    //                         load.paraContents.push_back(value);
    //                     }
    //                     listBoundaryInfo_.push_back(load);
    //                 }
    //             }
    //         }
    //     }

    //     // 整理连接条件数据    listConnectInfo_
    //     std::map<int, std::shared_ptr<MechConnectorData::mConnectorBase>>& _allConnectors = _modelData->getManager<mConnectorManager>(Physical_Connector).GetAllConnectors();
    //     int ConnectorNum = _allConnectors.size();
    //     for (auto& id2Case : _allConnectors) {
    //         PartInfo connectinfo;
    //         std::shared_ptr < MechConnectorData::mConnectorBase> tempCase = id2Case.second;
    //         if (tempCase != nullptr)
    //         {
    //             //连接集合名
    //             connectinfo.paraNames.push_back("连接集合名");
    //             connectinfo.paraContents.push_back(tempCase->getConnectorName());

    //             //连接个数
    //             int ncnt = tempCase->getConnectCount();
    //             connectinfo.paraNames.push_back("连接个数");
    //             connectinfo.paraContents.push_back(MxFunctions::mNumberToString(ncnt));

    //             listConnectInfo_.push_back(connectinfo);
    //         }
    //     }

    //     // 整理接触条件数据    listContactInfo_
    //     std::map<int, MechContactData::mContactBase*>& _allContacts = _modelData->getManager<mContactManager>(Physical_Contact).GetAllContacts();
    //     int ContactNum = _allContacts.size();
    //     for (auto& id2Case : _allContacts) {
    //         PartInfo contact;
    //         MechContactData::mContactBase* tempContact = id2Case.second;
    //         if (tempContact != nullptr)
    //         {
    //             //名称
    //             contact.paraNames.push_back("名称");
    //             contact.paraContents.push_back(tempContact->getContactName());

    //             //类型
    //             QString contype = tempContact->getContactTypeStr();
    //             contact.paraNames.push_back("类型");
    //             contact.paraContents.push_back(contype);

    //             mDataList datalist;
    //             //其他参数
    //             tempContact->getParameterList(datalist);

    //             //填充输出数据
    //             const int othercount = datalist.getDataCount();
    //             for (int j = 0; j < othercount; ++j)
    //             {
    //                 mBaseData* tepData = datalist.getData(j);
    //                 if (tepData == nullptr)
    //                 {
    //                     continue;
    //                 }
    //                 QString name = tepData->getDataName();
    //                 QString value = tepData->getDataVariant().toString();

    //                 contact.paraNames.push_back(name);
    //                 contact.paraContents.push_back(value);
    //             }
    //             listContactInfo_.push_back(contact);
    //         }
    //     }


    //    //QDir exePath(dir);
    //    //QString mxtxt = getmxFileName(dir);

    //    //QFile file(dir + mxtxt);
    //    //if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    //    //{
    //    //    return;
    //    //}
    //    //QTextStream in(&file);
    //    //in.setCodec("UTF-8");
    //    //QString line;
    //    //while (!in.atEnd())
    //    //{
    //    //    line = in.readLine();
    //    //    if (line.startsWith("*ModelInfo"))
    //    //    {
    //    //        if (!in.atEnd())
    //    //        {
    //    //            int count = in.readLine().toInt();
    //    //            for (int i = 0; i < count; i++)
    //    //            {
    //    //                PartInfo part;

    //    //                //部件参数名称
    //    //                line = in.readLine();
    //    //                QStringList strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    part.paraNames.append(strlist.at(i));
    //    //                }
    //    //                //部件参数内容
    //    //                line = in.readLine();
    //    //                strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    part.paraContents.append(strlist.at(i));
    //    //                }

    //    //                listPartInfo_.append(part);
    //    //            }
    //    //        }
    //    //    }
    //    //    else if (line.startsWith("*MaterialInfo"))
    //    //    {
    //    //        if (!in.atEnd())
    //    //        {
    //    //            int count = in.readLine().toInt();
    //    //            for (int i = 0; i < count; i++)
    //    //            {
    //    //                PartInfo material;

    //    //                //部件参数名称
    //    //                line = in.readLine();
    //    //                QStringList strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    material.paraNames.append(strlist.at(i));
    //    //                }
    //    //                //部件参数内容
    //    //                line = in.readLine();
    //    //                strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    material.paraContents.append(strlist.at(i));
    //    //                }

    //    //                listMaterialInfo_.append(material);
    //    //            }
    //    //        }
    //    //    }
    //    //    else if (line.startsWith("*PropertyInfo"))
    //    //    {
    //    //        if (!in.atEnd())
    //    //        {
    //    //            int count = in.readLine().toInt();
    //    //            for (int i = 0; i < count; i++)
    //    //            {
    //    //                PartInfo property;

    //    //                //部件参数名称
    //    //                line = in.readLine();
    //    //                QStringList strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    property.paraNames.append(strlist.at(i));
    //    //                }
    //    //                //部件参数内容
    //    //                line = in.readLine();
    //    //                strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    property.paraContents.append(strlist.at(i));
    //    //                }

    //    //                listPropertyInfo_.append(property);
    //    //            }
    //    //        }
    //    //    }
    //    //    else if (line.startsWith("*CaseInfo"))
    //    //    {
    //    //        if (!in.atEnd())
    //    //        {
    //    //            int count = in.readLine().toInt();
    //    //            for (int i = 0; i < count; i++)
    //    //            {
    //    //                PartInfo caseinfo;

    //    //                //部件参数名称
    //    //                line = in.readLine();
    //    //                QStringList strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    caseinfo.paraNames.append(strlist.at(i));
    //    //                }
    //    //                //部件参数内容
    //    //                line = in.readLine();
    //    //                strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    caseinfo.paraContents.append(strlist.at(i));
    //    //                }

    //    //                listCaseInfo_.append(caseinfo);
    //    //            }
    //    //        }
    //    //    }
    //    //    else if (line.startsWith("*LoadInfo"))
    //    //    {
    //    //        if (!in.atEnd())
    //    //        {
    //    //            int count = in.readLine().toInt();
    //    //            for (int i = 0; i < count; i++)
    //    //            {
    //    //                PartInfo load;

    //    //                //部件参数名称
    //    //                line = in.readLine();
    //    //                QStringList strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    load.paraNames.append(strlist.at(i));
    //    //                }
    //    //                //部件参数内容
    //    //                line = in.readLine();
    //    //                strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    load.paraContents.append(strlist.at(i));
    //    //                }

    //    //                listLoadInfo_.append(load);
    //    //            }
    //    //        }
    //    //    }
    //    //    else if (line.startsWith("*BoundaryInfo"))
    //    //    {
    //    //        if (!in.atEnd())
    //    //        {
    //    //            int count = in.readLine().toInt();
    //    //            for (int i = 0; i < count; i++)
    //    //            {
    //    //                PartInfo boundary;

    //    //                //部件参数名称
    //    //                line = in.readLine();
    //    //                QStringList strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    boundary.paraNames.append(strlist.at(i));
    //    //                }
    //    //                //部件参数内容
    //    //                line = in.readLine();
    //    //                strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    boundary.paraContents.append(strlist.at(i));
    //    //                }

    //    //                listBoundaryInfo_.append(boundary);
    //    //            }
    //    //        }
    //    //    }
    //    //    else if (line.startsWith("*ConnectInfo"))
    //    //    {
    //    //        if (!in.atEnd())
    //    //        {
    //    //            int count = in.readLine().toInt();
    //    //            for (int i = 0; i < count; i++)
    //    //            {
    //    //                PartInfo connectinfo;

    //    //                //部件参数名称
    //    //                line = in.readLine();
    //    //                QStringList strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    connectinfo.paraNames.append(strlist.at(i));
    //    //                }
    //    //                //部件参数内容
    //    //                line = in.readLine();
    //    //                strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    connectinfo.paraContents.append(strlist.at(i));
    //    //                }

    //    //                listConnectInfo_.append(connectinfo);
    //    //            }
    //    //        }
    //    //    }
    //    //    else if (line.startsWith("*ContactInfo"))
    //    //    {
    //    //        if (!in.atEnd())
    //    //        {
    //    //            int count = in.readLine().toInt();
    //    //            for (int i = 0; i < count; i++)
    //    //            {
    //    //                PartInfo contact;

    //    //                //部件参数名称
    //    //                line = in.readLine();
    //    //                QStringList strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    contact.paraNames.append(strlist.at(i));
    //    //                }
    //    //                //部件参数内容
    //    //                line = in.readLine();
    //    //                strlist = line.split(";");
    //    //                for (int i = 0; i < strlist.size() - 1; i++)
    //    //                {
    //    //                    contact.paraContents.append(strlist.at(i));
    //    //                }

    //    //                listContactInfo_.append(contact);
    //    //            }
    //    //        }
    //    //    }
    //    //}
    //}

    void mPostExportWordPanel::getResultInfo()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        //if (postRend->getViewportName() != _curViewName)
        //    return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        QString postname = postRend->getPostDataName();

        QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
        if (postDB == nullptr)
            return;

        //获取GUI数据
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr)
            return;

        //当前帧
        int framei = guidata->_cloudFrame;
        resultInfo_.currentFrame = framei;

        //总帧数
        int totalframe = postDB->getTotalFrameCount();
        resultInfo_.frameCount = totalframe;

        //根据帧号获取对应的模型数据
        mPostModelData* modeldata = postDB->getModelData(framei);
        if (modeldata == nullptr)
            return;

        if (modeldata != nullptr)
        {
            //获取节点总数
            int nodeamount = modeldata->_nodedata.nodeamount;
            resultInfo_.nodeNum = nodeamount;

            //获取单元总数
            int eleamount = modeldata->getElementAmount();
            resultInfo_.elementNum = eleamount;
        }

        //离散阶数
        if (guidata->_cloudDiscrete)
        {
            resultInfo_.intDiscrete = guidata->_cloudDiscreteNum;
        }
        else
        {
            resultInfo_.strDiscrete = "连续";
        }

        //默认最大值和最小值
        resultInfo_.defaultMax = guidata->_cloudDefualtMax;
        resultInfo_.defaultMin = guidata->_cloudDefualtMin;

        ////模态数据
        //if (modeldata != nullptr)
        //{
        //    if (oneframe->isOneFrameModalValue())
        //    {
        //        resultInfo_.modalFrequency = oneframe->getOneFrameModalFrequency();
        //        resultInfo_.modalEigen = oneframe->getOneFrameModalEigen();
        //    }
        //}
    }

    void mPostExportWordPanel::setHeadline(QString header)
    {
        html_.append(QString("<h1 style='text-align:center; font-size:36px; color:rgb(59,87,125)'>"));
        html_.append(header);
        html_.append("</h1>");
    }

    void mPostExportWordPanel::setSubHeading(QString header)
    {
        html_.append(QString("<h2 style='text-align:center; font-size:24px; color:rgb(59,87,125)'>"));
        html_.append(header);
        html_.append("</h2><br/>");
    }

    void mPostExportWordPanel::setFirstLevelHeading(QString header)
    {
        html_.append(QString("<h2 style='text-align:left; font-size:24px; color:rgb(59,87,125)'>"));
        html_.append(header);
        html_.append("</h2>");
    }

    void mPostExportWordPanel::setTableHeading(QString header, int col)
    {
        QString Col = MxFunctions::mNumberToString(col);
        html_.append(QString("<tr>"));
        html_.append(QString("<td align='center' colspan='%1' style='vertical-align:middle;font-weight:bold; "
            "font-size:24px; border:1px solid rgb(122, 153, 192); background-color:rgb(165, 186, 213)' >").arg(Col));
        html_.append(header);
        html_.append(QString("</td>"));
        html_.append(QString("</tr>"));
    }

    void mPostExportWordPanel::setCellHeaderStyle(int col, QString scale, QString field)
    {
        QString Col = MxFunctions::mNumberToString(col);
        html_.append(QString("<td width=%1 colspan='%2' valign='center' style='vertical-align:middle;font-size:16x;"
            "border:1px solid rgb(122, 153, 192); background-color:rgb(209, 220, 234); text-align:right;'>").arg(scale).arg(Col));
        html_.append(field);
        html_.append(QString("</td>"));
    }

    void mPostExportWordPanel::setCellHeaderStyle(QString scale, QString field)
    {
        html_.append(QString("<td width=%1 valign='center' style='vertical-align:middle;font-size:16x;"
            "border:1px solid rgb(122, 153, 192); background-color:rgb(209, 220, 234); text-align:right;'>").arg(scale));
        html_.append(field);
        html_.append(QString("</td>"));
    }

    void mPostExportWordPanel::setCellContentStyle(int col, QString scale, QString value)
    {
        QString Col = MxFunctions::mNumberToString(col);
        html_.append(QString("<td width=%1 colspan='%2' valign='center' style='vertical-align:middle;"
            "font-size:16px; border: 1px solid rgb(122, 153, 192); text-align:center;'>").arg(scale).arg(Col));
        html_.append(value);
        html_.append(QString("</td>"));
    }

    void mPostExportWordPanel::setCellContentStyle(QString scale, QString value)
    {
        html_.append(QString("<td width=%1 valign='center' style='vertical-align:middle;"
            "font-size:16px; border: 1px solid rgb(122, 153, 192); text-align:center;'>").arg(scale));
        html_.append(value);
        html_.append(QString("</td>"));
    }

    void mPostExportWordPanel::insertTableImage(int col, QString path)
    {
        QString Col = MxFunctions::mNumberToString(col);
        html_.append(QString("<td colspan=%1 style ='text-align:center; border:1px solid rgb(122, 153, 192);'> "
            "<img src='%2'width = '550' height='350'>").arg(Col).arg(path));
        html_.append(QString("</td>"));
    }

    void mPostExportWordPanel::insertImage(QString path)
    {
        QByteArray imageData;
        QBuffer buffer(&imageData);
        buffer.open(QIODevice::WriteOnly);

        QImage image(path);
        image.save(&buffer, "PNG");
        QString base64Image = imageData.toBase64();

        html_.append(QString("<div style = 'text-align:center'> <img src = \"data:image/png;base64,%1\" width = '550' height = '300'vspace='20'> </div>").arg(base64Image));

        //html_.append(QString("<div style = 'text-align:center'> <img src = '%1'width = '550' height = '300'vspace='20'> </div>").arg(path));
    }

    void mPostExportWordPanel::insertImageTitle(QString imageTitle)
    {
        html_.append(QString("<p style ='text-align:center;font-size:16px;'>%1</p>").arg(imageTitle));
        html_.append(QString("<br>"));
    }

    void mPostExportWordPanel::insertText(QString text)
    {
        html_.append(QString("<p style='text-align:left; font-size:16px;'>&nbsp;&nbsp;&nbsp;&nbsp;"));
        html_.append(text);
        html_.append("</p>");
    }

    void mPostExportWordPanel::tableBegin(QString scale)
    {
        html_.append(QString("<table align='center' border='1' cellpadding='4' vspace='50'"
            "style='width:%1; border-collapse:collapse;' >").arg(scale));
    }

    void mPostExportWordPanel::tableEnd()
    {
        html_.append(QString("</table><br>"));
    }

    void mPostExportWordPanel::rowBegin()
    {
        html_.append(QString("<tr>"));
    }

    void mPostExportWordPanel::rowEnd()
    {
        html_.append(QString("</tr>"));
    }

    QStringList mPostExportWordPanel::getPictureListByType(const QString& type) const
    {
        QStringList result;
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return result;
        PostData::Type_WordContent* content = findWordContent(curveData, type);
        if (content == nullptr)
            return result;

        QString imageDir = getExportWordImageDir();
        for (QString picname : content->pictureList)
        {
            result.append(imageDir + "/" + picname);
        }
        return result;
    }

    void mPostExportWordPanel::writePictures(const QString& type)
    {
        //添加用户截图
        QStringList piclist = getPictureListByType(type);
        for (QString imagepath : piclist)
        {
            insertImage(imagepath);

            //写图例
            QFileInfo fi(imagepath);
            QString legend = _pictureDialog->getPictureLegendName(fi.fileName());
            insertImageTitle(legend);
        }
    }

    void mPostExportWordPanel::writeDescriptionContent()
    {
        //写章节标题
        QString header = QString("%1  说明").arg(_contentIndex);
        setFirstLevelHeading(header);

        //写内容
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("说明"));
            if (content != nullptr && !content->ContentStr.isEmpty())
                insertText(content->ContentStr);
        }

        //写用户截图
        this->writePictures(QString("说明"));
    }

    void mPostExportWordPanel::collectDescriptionContext(PostData::mPostReportContext& context)
    {
        QString sectionId = "description";
        QString title = QString("%1  说明").arg(_contentIndex);
        PostData::ReportSection section;
        section.id = sectionId;
        section.title = title;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("说明"));
            if (content != nullptr && !content->ContentStr.isEmpty())
            {
                PostData::ReportSectionItem item;
                item.type = PostData::ReportSectionItem::Text;
                item.text.content = content->ContentStr;
                section.items.append(item);
            }
        }

        for (const QString& pic : getPictureListByType(QString("说明")))
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = pic;
            item.image.caption = _pictureDialog->getPictureLegendName(QFileInfo(pic).fileName());
            section.items.append(item);
            if (!context.resourceImages.contains(pic))
                context.resourceImages.append(pic);
        }

        context.addSection(section);
    }

    void mPostExportWordPanel::writeModelContent()
    {
        //写章节标题
        QString header = QString("%1  模型").arg(_contentIndex);
        setFirstLevelHeading(header);

        //写内容
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("模型"));
            if (content != nullptr && !content->ContentStr.isEmpty())
                insertText(content->ContentStr);
        }

        //写文件图片
        //for (int i = 0; i < piclistModel_.size(); i++)
        //{
        //    insertImage(piclistModel_[i]);
        //    QString title = QString("%1.%2 导入几何/网格/计算/结果文件").arg(_contentIndex).arg(i + 1);
        //    insertImageTitle(title);
        //}
        for (int i = 0; i < _modelPics.size(); i++)
        {
            insertImage(_modelPics[i]);
            QString title = QString("%1.%2 导入几何/网格/计算/结果文件").arg(_contentIndex).arg(i + 1);
            insertImageTitle(title);
        }

        //写文件内容
        for (PartInfo part : listPartInfo_)
        {
            tableBegin("100%");
            setTableHeading("部件信息", 2);

            for (int i = 0; i < part.paraNames.size(); i++)
            {
                rowBegin();
                setCellHeaderStyle("50%", part.paraNames[i]);
                setCellContentStyle("50%", part.paraContents[i]);
                rowEnd();
            }

            tableEnd();
        }

        //写用户截图
        this->writePictures(QString("模型"));
    }

    void mPostExportWordPanel::collectModelContext(PostData::mPostReportContext& context)
    {
        QString sectionId = "model";
        QString title = QString("%1  模型").arg(_contentIndex);
        PostData::ReportSection section;
        section.id = sectionId;
        section.title = title;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("模型"));
            if (content != nullptr && !content->ContentStr.isEmpty())
            {
                PostData::ReportSectionItem item;
                item.type = PostData::ReportSectionItem::Text;
                item.text.content = content->ContentStr;
                section.items.append(item);
            }
        }

        for (int i = 0; i < _modelPics.size(); i++)
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = _modelPics[i];
            item.image.caption = QString("%1.%2 导入几何/网格/计算/结果文件").arg(_contentIndex).arg(i + 1);
            section.items.append(item);
            if (!context.resourceImages.contains(_modelPics[i]))
                context.resourceImages.append(_modelPics[i]);
        }

        for (const PartInfo& part : listPartInfo_)
        {
            PostData::ReportTable table;
            table.title = "部件信息";
            table.columnCount = 2;
            for (int i = 0; i < part.paraNames.size(); i++)
            {
                PostData::ReportRow row;
                PostData::ReportCell headerCell;
                headerCell.text = part.paraNames[i];
                headerCell.isHeader = true;
                headerCell.hAlign = "right";
                row.cells.append(headerCell);
                PostData::ReportCell valueCell;
                valueCell.text = part.paraContents[i];
                valueCell.hAlign = "center";
                row.cells.append(valueCell);
                table.rows.append(row);
            }
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Table;
            item.table = table;
            section.items.append(item);
        }

        for (const QString& pic : getPictureListByType(QString("模型")))
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = pic;
            item.image.caption = _pictureDialog->getPictureLegendName(QFileInfo(pic).fileName());
            section.items.append(item);
            if (!context.resourceImages.contains(pic))
                context.resourceImages.append(pic);
        }

        context.addSection(section);
    }

    void mPostExportWordPanel::writeMaterialContent()
    {
        //写章节标题
        QString header = QString("%1  材料").arg(_contentIndex);
        setFirstLevelHeading(header);

        //写内容
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("材料"));
            if (content != nullptr && !content->ContentStr.isEmpty())
                insertText(content->ContentStr);
        }

        //写文件图片
        //for (int i = 0; i < piclistMaterial_.size(); i++)
        //{
        //    insertImage(piclistMaterial_[i]);
        //    QString title = QString("%1.%2 设置材料属性").arg(_contentIndex).arg(i + 1);
        //    insertImageTitle(title);
        //}
        for (int i = 0; i < _materialPics.size(); i++)
        {
            insertImage(_materialPics[i]);
            QString title = QString("%1.%2 设置材料属性").arg(_contentIndex).arg(i + 1);
            insertImageTitle(title);
        }

        //写文件内容
        for (PartInfo material : listMaterialInfo_)
        {
            tableBegin("100%");
            setTableHeading("材料信息", 2);

            for (int i = 0; i < material.paraNames.size(); i++)
            {
                rowBegin();
                setCellHeaderStyle("50%", material.paraNames[i]);
                setCellContentStyle("50%", material.paraContents[i]);
                rowEnd();
            }

            tableEnd();
        }

        //写用户截图
        this->writePictures(QString("材料"));
    }

    void mPostExportWordPanel::collectMaterialContext(PostData::mPostReportContext& context)
    {
        QString sectionId = "material";
        QString title = QString("%1  材料").arg(_contentIndex);
        PostData::ReportSection section;
        section.id = sectionId;
        section.title = title;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("材料"));
            if (content != nullptr && !content->ContentStr.isEmpty())
            {
                PostData::ReportSectionItem item;
                item.type = PostData::ReportSectionItem::Text;
                item.text.content = content->ContentStr;
                section.items.append(item);
            }
        }

        for (int i = 0; i < _materialPics.size(); i++)
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = _materialPics[i];
            item.image.caption = QString("%1.%2 设置材料属性").arg(_contentIndex).arg(i + 1);
            section.items.append(item);
            if (!context.resourceImages.contains(_materialPics[i]))
                context.resourceImages.append(_materialPics[i]);
        }

        for (const PartInfo& material : listMaterialInfo_)
        {
            PostData::ReportTable table;
            table.title = "材料信息";
            table.columnCount = 2;
            for (int i = 0; i < material.paraNames.size(); i++)
            {
                PostData::ReportRow row;
                PostData::ReportCell headerCell;
                headerCell.text = material.paraNames[i];
                headerCell.isHeader = true;
                headerCell.hAlign = "right";
                row.cells.append(headerCell);
                PostData::ReportCell valueCell;
                valueCell.text = material.paraContents[i];
                valueCell.hAlign = "center";
                row.cells.append(valueCell);
                table.rows.append(row);
            }
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Table;
            item.table = table;
            section.items.append(item);
        }

        for (const QString& pic : getPictureListByType(QString("材料")))
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = pic;
            item.image.caption = _pictureDialog->getPictureLegendName(QFileInfo(pic).fileName());
            section.items.append(item);
            if (!context.resourceImages.contains(pic))
                context.resourceImages.append(pic);
        }

        context.addSection(section);
    }

    void mPostExportWordPanel::writePropertyContent()
    {
        //写章节标题
        QString header = QString("%1  截面属性").arg(_contentIndex);
        setFirstLevelHeading(header);

        //写内容
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("截面属性"));
            if (content != nullptr && !content->ContentStr.isEmpty())
                insertText(content->ContentStr);
        }

        //写文件图片
        //for (int i = 0; i < piclistProperty_.size(); i++)
        //{
        //    insertImage(piclistProperty_[i]);
        //    QString title = QString("%1.%2 设置截面属性").arg(_contentIndex).arg(i + 1);
        //    insertImageTitle(title);
        //}
        for (int i = 0; i < _propertyPics.size(); i++)
        {
            insertImage(_propertyPics[i]);
            QString title = QString("%1.%2 设置截面属性").arg(_contentIndex).arg(i + 1);
            insertImageTitle(title);
        }

        //写文件内容
        for (PartInfo property : listPropertyInfo_)
        {
            tableBegin("100%");
            setTableHeading("截面属性信息", 2);

            for (int i = 0; i < property.paraNames.size(); i++)
            {
                rowBegin();
                setCellHeaderStyle("50%", property.paraNames[i]);
                setCellContentStyle("50%", property.paraContents[i]);
                rowEnd();
            }

            tableEnd();
        }
        //写用户截图
        this->writePictures(QString("截面属性"));
    }

    void mPostExportWordPanel::collectPropertyContext(PostData::mPostReportContext& context)
    {
        QString sectionId = "property";
        QString title = QString("%1  截面属性").arg(_contentIndex);
        PostData::ReportSection section;
        section.id = sectionId;
        section.title = title;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("截面属性"));
            if (content != nullptr && !content->ContentStr.isEmpty())
            {
                PostData::ReportSectionItem item;
                item.type = PostData::ReportSectionItem::Text;
                item.text.content = content->ContentStr;
                section.items.append(item);
            }
        }

        for (int i = 0; i < _propertyPics.size(); i++)
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = _propertyPics[i];
            item.image.caption = QString("%1.%2 设置截面属性").arg(_contentIndex).arg(i + 1);
            section.items.append(item);
            if (!context.resourceImages.contains(_propertyPics[i]))
                context.resourceImages.append(_propertyPics[i]);
        }

        for (const PartInfo& property : listPropertyInfo_)
        {
            PostData::ReportTable table;
            table.title = "截面属性信息";
            table.columnCount = 2;
            for (int i = 0; i < property.paraNames.size(); i++)
            {
                PostData::ReportRow row;
                PostData::ReportCell headerCell;
                headerCell.text = property.paraNames[i];
                headerCell.isHeader = true;
                headerCell.hAlign = "right";
                row.cells.append(headerCell);
                PostData::ReportCell valueCell;
                valueCell.text = property.paraContents[i];
                valueCell.hAlign = "center";
                row.cells.append(valueCell);
                table.rows.append(row);
            }
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Table;
            item.table = table;
            section.items.append(item);
        }

        for (const QString& pic : getPictureListByType(QString("截面属性")))
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = pic;
            item.image.caption = _pictureDialog->getPictureLegendName(QFileInfo(pic).fileName());
            section.items.append(item);
            if (!context.resourceImages.contains(pic))
                context.resourceImages.append(pic);
        }

        context.addSection(section);
    }

    void mPostExportWordPanel::writeCaseContent()
    {
        //写章节标题
        QString header = QString("%1  分析工况").arg(_contentIndex);
        setFirstLevelHeading(header);

        //写内容
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("分析工况"));
            if (content != nullptr && !content->ContentStr.isEmpty())
                insertText(content->ContentStr);
        }

        //写文件图片
        //for (int i = 0; i < piclistCase_.size(); i++)
        //{
        //    insertImage(piclistCase_[i]);
        //    QString title = QString("%1.%2 工况描述").arg(_contentIndex).arg(i + 1);
        //    insertImageTitle(title);
        //}
        for (int i = 0; i < _casePics.size(); i++)
        {
            insertImage(_casePics[i]);
            QString title = QString("%1.%2 工况描述").arg(_contentIndex).arg(i + 1);
            insertImageTitle(title);
        }

        //写文件内容
        for (PartInfo caseinfo : listCaseInfo_)
        {
            tableBegin("100%");
            setTableHeading("工况信息", 2);

            for (int i = 0; i < caseinfo.paraNames.size(); i++)
            {
                rowBegin();
                setCellHeaderStyle("50%", caseinfo.paraNames[i]);
                setCellContentStyle("50%", caseinfo.paraContents[i]);
                rowEnd();
            }

            tableEnd();
        }
        //写用户截图
        this->writePictures(QString("分析工况"));
    }

    void mPostExportWordPanel::collectCaseContext(PostData::mPostReportContext& context)
    {
        QString sectionId = "case";
        QString title = QString("%1  分析工况").arg(_contentIndex);
        PostData::ReportSection section;
        section.id = sectionId;
        section.title = title;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("分析工况"));
            if (content != nullptr && !content->ContentStr.isEmpty())
            {
                PostData::ReportSectionItem item;
                item.type = PostData::ReportSectionItem::Text;
                item.text.content = content->ContentStr;
                section.items.append(item);
            }
        }

        for (int i = 0; i < _casePics.size(); i++)
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = _casePics[i];
            item.image.caption = QString("%1.%2 工况描述").arg(_contentIndex).arg(i + 1);
            section.items.append(item);
            if (!context.resourceImages.contains(_casePics[i]))
                context.resourceImages.append(_casePics[i]);
        }

        for (const PartInfo& caseinfo : listCaseInfo_)
        {
            PostData::ReportTable table;
            table.title = "工况信息";
            table.columnCount = 2;
            for (int i = 0; i < caseinfo.paraNames.size(); i++)
            {
                PostData::ReportRow row;
                PostData::ReportCell headerCell;
                headerCell.text = caseinfo.paraNames[i];
                headerCell.isHeader = true;
                headerCell.hAlign = "right";
                row.cells.append(headerCell);
                PostData::ReportCell valueCell;
                valueCell.text = caseinfo.paraContents[i];
                valueCell.hAlign = "center";
                row.cells.append(valueCell);
                table.rows.append(row);
            }
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Table;
            item.table = table;
            section.items.append(item);
        }

        for (const QString& pic : getPictureListByType(QString("分析工况")))
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = pic;
            item.image.caption = _pictureDialog->getPictureLegendName(QFileInfo(pic).fileName());
            section.items.append(item);
            if (!context.resourceImages.contains(pic))
                context.resourceImages.append(pic);
        }

        context.addSection(section);
    }

    void mPostExportWordPanel::writeLoadContent()
    {
        //写章节标题
        QString header = QString("%1  载荷").arg(_contentIndex);
        setFirstLevelHeading(header);

        //写内容
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("载荷"));
            if (content != nullptr && !content->ContentStr.isEmpty())
                insertText(content->ContentStr);
        }

        //写文件图片
        //for (int i = 0; i < piclistLoad_.size(); i++)
        //{
        //    insertImage(piclistLoad_[i]);
        //    QString title = QString("%1.%2 设置载荷条件").arg(_contentIndex).arg(i + 1);
        //    insertImageTitle(title);
        //}
        for (int i = 0; i < _loadPics.size(); i++)
        {
            insertImage(_loadPics[i]);
            QString title = QString("%1.%2 设置载荷条件").arg(_contentIndex).arg(i + 1);
            insertImageTitle(title);
        }

        //写文件内容
        for (PartInfo load : listLoadInfo_)
        {
            tableBegin("100%");
            setTableHeading("载荷信息", 2);

            for (int i = 0; i < load.paraNames.size(); i++)
            {
                rowBegin();
                setCellHeaderStyle("50%", load.paraNames[i]);
                setCellContentStyle("50%", load.paraContents[i]);
                rowEnd();
            }

            tableEnd();
        }

        //写用户截图
        this->writePictures(QString("载荷"));
    }

    void mPostExportWordPanel::collectLoadContext(PostData::mPostReportContext& context)
    {
        QString sectionId = "load";
        QString title = QString("%1  载荷").arg(_contentIndex);
        PostData::ReportSection section;
        section.id = sectionId;
        section.title = title;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("载荷"));
            if (content != nullptr && !content->ContentStr.isEmpty())
            {
                PostData::ReportSectionItem item;
                item.type = PostData::ReportSectionItem::Text;
                item.text.content = content->ContentStr;
                section.items.append(item);
            }
        }

        for (int i = 0; i < _loadPics.size(); i++)
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = _loadPics[i];
            item.image.caption = QString("%1.%2 设置载荷条件").arg(_contentIndex).arg(i + 1);
            section.items.append(item);
            if (!context.resourceImages.contains(_loadPics[i]))
                context.resourceImages.append(_loadPics[i]);
        }

        for (const PartInfo& load : listLoadInfo_)
        {
            PostData::ReportTable table;
            table.title = "载荷信息";
            table.columnCount = 2;
            for (int i = 0; i < load.paraNames.size(); i++)
            {
                PostData::ReportRow row;
                PostData::ReportCell headerCell;
                headerCell.text = load.paraNames[i];
                headerCell.isHeader = true;
                headerCell.hAlign = "right";
                row.cells.append(headerCell);
                PostData::ReportCell valueCell;
                valueCell.text = load.paraContents[i];
                valueCell.hAlign = "center";
                row.cells.append(valueCell);
                table.rows.append(row);
            }
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Table;
            item.table = table;
            section.items.append(item);
        }

        for (const QString& pic : getPictureListByType(QString("载荷")))
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = pic;
            item.image.caption = _pictureDialog->getPictureLegendName(QFileInfo(pic).fileName());
            section.items.append(item);
            if (!context.resourceImages.contains(pic))
                context.resourceImages.append(pic);
        }

        context.addSection(section);
    }

    void mPostExportWordPanel::writeBCContent()
    {
        //写章节标题
        QString header = QString("%1  边界").arg(_contentIndex);
        setFirstLevelHeading(header);

        //写内容
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("边界"));
            if (content != nullptr && !content->ContentStr.isEmpty())
                insertText(content->ContentStr);
        }

        //写文件图片
        //for (int i = 0; i < piclistBoundary_.size(); i++)
        //{
        //    insertImage(piclistBoundary_[i]);
        //    QString title = QString("%1.%2 设置边界").arg(_contentIndex).arg(i + 1);
        //    insertImageTitle(title);
        //}
        for (int i = 0; i < _boundaryPics.size(); i++)
        {
            insertImage(_boundaryPics[i]);
            QString title = QString("%1.%2 设置边界").arg(_contentIndex).arg(i + 1);
            insertImageTitle(title);
        }

        //写文件内容
        for (PartInfo boundary : listBoundaryInfo_)
        {
            tableBegin("100%");
            setTableHeading("边界信息", 2);

            for (int i = 0; i < boundary.paraNames.size(); i++)
            {
                rowBegin();
                setCellHeaderStyle("50%", boundary.paraNames[i]);
                setCellContentStyle("50%", boundary.paraContents[i]);
                rowEnd();
            }

            tableEnd();
        }

        //写用户截图
        this->writePictures(QString("边界"));
    }

    void mPostExportWordPanel::collectBCContext(PostData::mPostReportContext& context)
    {
        QString sectionId = "boundary";
        QString title = QString("%1  边界").arg(_contentIndex);
        PostData::ReportSection section;
        section.id = sectionId;
        section.title = title;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("边界"));
            if (content != nullptr && !content->ContentStr.isEmpty())
            {
                PostData::ReportSectionItem item;
                item.type = PostData::ReportSectionItem::Text;
                item.text.content = content->ContentStr;
                section.items.append(item);
            }
        }

        for (int i = 0; i < _boundaryPics.size(); i++)
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = _boundaryPics[i];
            item.image.caption = QString("%1.%2 设置边界").arg(_contentIndex).arg(i + 1);
            section.items.append(item);
            if (!context.resourceImages.contains(_boundaryPics[i]))
                context.resourceImages.append(_boundaryPics[i]);
        }

        for (const PartInfo& boundary : listBoundaryInfo_)
        {
            PostData::ReportTable table;
            table.title = "边界信息";
            table.columnCount = 2;
            for (int i = 0; i < boundary.paraNames.size(); i++)
            {
                PostData::ReportRow row;
                PostData::ReportCell headerCell;
                headerCell.text = boundary.paraNames[i];
                headerCell.isHeader = true;
                headerCell.hAlign = "right";
                row.cells.append(headerCell);
                PostData::ReportCell valueCell;
                valueCell.text = boundary.paraContents[i];
                valueCell.hAlign = "center";
                row.cells.append(valueCell);
                table.rows.append(row);
            }
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Table;
            item.table = table;
            section.items.append(item);
        }

        for (const QString& pic : getPictureListByType(QString("边界")))
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = pic;
            item.image.caption = _pictureDialog->getPictureLegendName(QFileInfo(pic).fileName());
            section.items.append(item);
            if (!context.resourceImages.contains(pic))
                context.resourceImages.append(pic);
        }

        context.addSection(section);
    }

    void mPostExportWordPanel::writeCNContent()
    {
        //写章节标题
        QString header = QString("%1  连接").arg(_contentIndex);
        setFirstLevelHeading(header);

        //写内容
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("连接"));
            if (content != nullptr && !content->ContentStr.isEmpty())
                insertText(content->ContentStr);
        }

        for (int i = 0; i < _connectPics.size(); i++)
        {
            insertImage(_connectPics[i]);
            QString title = QString("%1.%2 设置连接").arg(_contentIndex).arg(i + 1);
            insertImageTitle(title);
        }

        //写文件内容
        for (PartInfo connectinfo : listConnectInfo_)
        {
            tableBegin("100%");
            setTableHeading("连接信息", 2);

            for (int i = 0; i < connectinfo.paraNames.size(); i++)
            {
                rowBegin();
                setCellHeaderStyle("50%", connectinfo.paraNames[i]);
                setCellContentStyle("50%", connectinfo.paraContents[i]);
                rowEnd();
            }

            tableEnd();
        }

        //写用户截图
        this->writePictures(QString("连接"));
    }

    void mPostExportWordPanel::collectCNContext(PostData::mPostReportContext& context)
    {
        QString sectionId = "connector";
        QString title = QString("%1  连接").arg(_contentIndex);
        PostData::ReportSection section;
        section.id = sectionId;
        section.title = title;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("连接"));
            if (content != nullptr && !content->ContentStr.isEmpty())
            {
                PostData::ReportSectionItem item;
                item.type = PostData::ReportSectionItem::Text;
                item.text.content = content->ContentStr;
                section.items.append(item);
            }
        }

        for (int i = 0; i < _connectPics.size(); i++)
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = _connectPics[i];
            item.image.caption = QString("%1.%2 设置连接").arg(_contentIndex).arg(i + 1);
            section.items.append(item);
            if (!context.resourceImages.contains(_connectPics[i]))
                context.resourceImages.append(_connectPics[i]);
        }

        for (const PartInfo& connectinfo : listConnectInfo_)
        {
            PostData::ReportTable table;
            table.title = "连接信息";
            table.columnCount = 2;
            for (int i = 0; i < connectinfo.paraNames.size(); i++)
            {
                PostData::ReportRow row;
                PostData::ReportCell headerCell;
                headerCell.text = connectinfo.paraNames[i];
                headerCell.isHeader = true;
                headerCell.hAlign = "right";
                row.cells.append(headerCell);
                PostData::ReportCell valueCell;
                valueCell.text = connectinfo.paraContents[i];
                valueCell.hAlign = "center";
                row.cells.append(valueCell);
                table.rows.append(row);
            }
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Table;
            item.table = table;
            section.items.append(item);
        }

        for (const QString& pic : getPictureListByType(QString("连接")))
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = pic;
            item.image.caption = _pictureDialog->getPictureLegendName(QFileInfo(pic).fileName());
            section.items.append(item);
            if (!context.resourceImages.contains(pic))
                context.resourceImages.append(pic);
        }

        context.addSection(section);
    }

    void mPostExportWordPanel::writeCTContent()
    {
        //写章节标题
        QString header = QString("%1  接触").arg(_contentIndex);
        setFirstLevelHeading(header);

        //写内容
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("接触"));
            if (content != nullptr && !content->ContentStr.isEmpty())
                insertText(content->ContentStr);
        }

        for (int i = 0; i < _contactPics.size(); i++)
        {
            insertImage(_contactPics[i]);
            QString title = QString("%1.%2 设置接触").arg(_contentIndex).arg(i + 1);
            insertImageTitle(title);
        }

        //写文件内容
        for (PartInfo contact : listContactInfo_)
        {
            tableBegin("100%");
            setTableHeading("接触信息", 2);

            for (int i = 0; i < contact.paraNames.size(); i++)
            {
                rowBegin();
                setCellHeaderStyle("50%", contact.paraNames[i]);
                setCellContentStyle("50%", contact.paraContents[i]);
                rowEnd();
            }

            tableEnd();
        }

        //写用户截图
        this->writePictures(QString("接触"));
    }

    void mPostExportWordPanel::collectCTContext(PostData::mPostReportContext& context)
    {
        QString sectionId = "contact";
        QString title = QString("%1  接触").arg(_contentIndex);
        PostData::ReportSection section;
        section.id = sectionId;
        section.title = title;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("接触"));
            if (content != nullptr && !content->ContentStr.isEmpty())
            {
                PostData::ReportSectionItem item;
                item.type = PostData::ReportSectionItem::Text;
                item.text.content = content->ContentStr;
                section.items.append(item);
            }
        }

        for (int i = 0; i < _contactPics.size(); i++)
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = _contactPics[i];
            item.image.caption = QString("%1.%2 设置接触").arg(_contentIndex).arg(i + 1);
            section.items.append(item);
            if (!context.resourceImages.contains(_contactPics[i]))
                context.resourceImages.append(_contactPics[i]);
        }

        for (const PartInfo& contact : listContactInfo_)
        {
            PostData::ReportTable table;
            table.title = "接触信息";
            table.columnCount = 2;
            for (int i = 0; i < contact.paraNames.size(); i++)
            {
                PostData::ReportRow row;
                PostData::ReportCell headerCell;
                headerCell.text = contact.paraNames[i];
                headerCell.isHeader = true;
                headerCell.hAlign = "right";
                row.cells.append(headerCell);
                PostData::ReportCell valueCell;
                valueCell.text = contact.paraContents[i];
                valueCell.hAlign = "center";
                row.cells.append(valueCell);
                table.rows.append(row);
            }
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Table;
            item.table = table;
            section.items.append(item);
        }

        for (const QString& pic : getPictureListByType(QString("接触")))
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = pic;
            item.image.caption = _pictureDialog->getPictureLegendName(QFileInfo(pic).fileName());
            section.items.append(item);
            if (!context.resourceImages.contains(pic))
                context.resourceImages.append(pic);
        }

        context.addSection(section);
    }

    void mPostExportWordPanel::writeResultContent()
    {
        //写章节标题
        QString header = QString("%1  结果").arg(_contentIndex);
        setFirstLevelHeading(header);

        //写内容
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("结果"));
            if (content != nullptr && !content->ContentStr.isEmpty())
                insertText(content->ContentStr);
        }

        //写文件图片
        //for (int i = 0; i < piclistResult_.size(); i++)
        for (int i = 0; i < _resultPics.size(); i++)
        {
            //相对路径
            //insertImage(piclistResult_[i]);

            //base64编码
            insertImage(_resultPics[i]);
            QString title = QString("%1.%2 测试结果").arg(_contentIndex).arg(i + 1);
            insertImageTitle(title);
        }

        //写文件内容
        if (resultInfo_.currentFrame == 0)
            return;
        tableBegin("100%");
        setTableHeading("测试结果", 2);
        rowBegin();
        setCellHeaderStyle("50%", "当前帧");
        QString curframe = MxFunctions::mNumberToString(resultInfo_.currentFrame);
        setCellContentStyle("50%", curframe);
        rowEnd();
        setCellHeaderStyle("50%", "总帧数");
        QString totalframe = MxFunctions::mNumberToString(resultInfo_.frameCount);
        setCellContentStyle("50%", totalframe);
        rowBegin();
        setCellHeaderStyle("50%", "节点总数");
        QString node = MxFunctions::mNumberToString(resultInfo_.nodeNum);
        setCellContentStyle("50%", node);
        rowEnd();
        rowBegin();
        setCellHeaderStyle("50%", "单元总数");
        QString elelment = MxFunctions::mNumberToString(resultInfo_.elementNum);
        setCellContentStyle("50%", elelment);
        rowEnd();
        rowBegin();
        if (resultInfo_.strDiscrete.isEmpty())
        {
            setCellHeaderStyle("50%", "离散阶数");
            QString discrete = MxFunctions::mNumberToString(resultInfo_.intDiscrete);
            setCellContentStyle("50%", discrete);
        }
        else
        {
            setCellHeaderStyle("50%", "离散阶数");
            setCellContentStyle("50%", resultInfo_.strDiscrete);
        }
        rowEnd();
        rowBegin();
        setCellHeaderStyle("50%", "默认最大值");
        QString max = MxFunctions::mNumberToString(resultInfo_.defaultMax);
        setCellContentStyle("50%", max);
        rowEnd();
        rowBegin();
        setCellHeaderStyle("50%", "默认最小值");
        QString min = MxFunctions::mNumberToString(resultInfo_.defaultMin);
        setCellContentStyle("50%", min);
        rowEnd();
        if (fabs(resultInfo_.modalFrequency) > 1e-15)
        {
            rowBegin();
            setCellHeaderStyle("50%", "模态频率");
            QString min = MxFunctions::mNumberToString(resultInfo_.defaultMin);
            setCellContentStyle("50%", min);
            rowEnd();
        }
        if (fabs(resultInfo_.modalEigen) > 1e-15)
        {
            rowBegin();
            setCellHeaderStyle("50%", "模态特征值");
            QString min = MxFunctions::mNumberToString(resultInfo_.defaultMin);
            setCellContentStyle("50%", min);
            rowEnd();
        }
        tableEnd();

        //写用户截图
        this->writePictures(QString("结果"));
    }

    void mPostExportWordPanel::collectResultContext(PostData::mPostReportContext& context)
    {
        QString sectionId = "result";
        QString title = QString("%1  结果").arg(_contentIndex);
        PostData::ReportSection section;
        section.id = sectionId;
        section.title = title;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData != nullptr)
        {
            PostData::Type_WordContent* content = findWordContent(curveData, QString("结果"));
            if (content != nullptr && !content->ContentStr.isEmpty())
            {
                PostData::ReportSectionItem item;
                item.type = PostData::ReportSectionItem::Text;
                item.text.content = content->ContentStr;
                section.items.append(item);
            }
        }

        for (int i = 0; i < _resultPics.size(); i++)
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = _resultPics[i];
            item.image.caption = QString("%1.%2 测试结果").arg(_contentIndex).arg(i + 1);
            section.items.append(item);
            if (!context.resourceImages.contains(_resultPics[i]))
                context.resourceImages.append(_resultPics[i]);
        }

        if (resultInfo_.currentFrame != 0)
        {
            PostData::ReportTable table;
            table.title = "测试结果";
            table.columnCount = 2;
            auto addRow = [&](const QString& name, const QString& value) {
                PostData::ReportRow row;
                PostData::ReportCell headerCell;
                headerCell.text = name;
                headerCell.isHeader = true;
                headerCell.hAlign = "right";
                row.cells.append(headerCell);
                PostData::ReportCell valueCell;
                valueCell.text = value;
                valueCell.hAlign = "center";
                row.cells.append(valueCell);
                table.rows.append(row);
            };
            addRow("当前帧", QString::number(resultInfo_.currentFrame));
            addRow("总帧数", QString::number(resultInfo_.frameCount));
            addRow("节点总数", QString::number(resultInfo_.nodeNum));
            addRow("单元总数", QString::number(resultInfo_.elementNum));
            if (resultInfo_.strDiscrete.isEmpty())
                addRow("离散阶数", QString::number(resultInfo_.intDiscrete));
            else
                addRow("离散阶数", resultInfo_.strDiscrete);
            addRow("默认最大值", QString::number(resultInfo_.defaultMax));
            addRow("默认最小值", QString::number(resultInfo_.defaultMin));
            if (fabs(resultInfo_.modalFrequency) > 1e-15)
                addRow("模态频率", QString::number(resultInfo_.modalFrequency));
            if (fabs(resultInfo_.modalEigen) > 1e-15)
                addRow("模态特征值", QString::number(resultInfo_.modalEigen));

            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Table;
            item.table = table;
            section.items.append(item);
        }

        for (const QString& pic : getPictureListByType(QString("结果")))
        {
            PostData::ReportSectionItem item;
            item.type = PostData::ReportSectionItem::Image;
            item.image.imagePath = pic;
            item.image.caption = _pictureDialog->getPictureLegendName(QFileInfo(pic).fileName());
            section.items.append(item);
            if (!context.resourceImages.contains(pic))
                context.resourceImages.append(pic);
        }

        context.addSection(section);
    }

    void mPostExportWordPanel::updateUserImages(QString image)
    {
        QListWidgetItem* item = _listWidget->currentItem();
        if (!item)
            return;

        QString context = item->text();

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;

        PostData::Type_WordContent* content = findWordContent(curveData, context);
        if (content == nullptr)
            return;

        QStringList picnames = content->pictureList;
        picnames.removeOne(image);
        content->pictureList = picnames;
        _pictureDialog->updatePictureList(picnames);
    }

    void mPostExportWordPanel::setCurViewName(const QString& viewName)
    {
        _curViewName = viewName;
    }

    void mPostExportWordPanel::slot_clickContentList(QListWidgetItem* item)
    {
        if (item == nullptr)
            return;

        QString context = item->text();
        setCurrentContentType(context);
        refreshEditorByData(context);

        // 更新列表中选中状态
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData)
            curveData->SetContentListChecked(context, item->checkState() == Qt::Checked);
    }

    void mPostExportWordPanel::slot_textCheckBox(bool ischeck)
    {
        _textEdit->setEnabled(ischeck);

        QString type = currentContentType();
        if (type.isEmpty())
            return;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData)
            curveData->SetContentTextChecked(type, ischeck);
    }

    void mPostExportWordPanel::slot_textChanged()
    {
        QString type = currentContentType();
        if (type.isEmpty())
            return;

        QString text = _textEdit->toPlainText();

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData)
            curveData->SetContentText(type, text);
    }

    void mPostExportWordPanel::slot_pictureCheckBox(bool ischeck)
    {
        _pushButton_saveView->setEnabled(ischeck);
        _pushButton_editScreenshot->setEnabled(ischeck);

        QString type = currentContentType();
        if (type.isEmpty())
            return;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData)
            curveData->SetContentPictureChecked(type, ischeck);
    }

    void mPostExportWordPanel::slot_savePictureButton()
    {
        //获取当前激活窗口
        QWidget* subwin = getCVM->activatePostViewport();
        if (subwin == nullptr)
            return;

        QString type = currentContentType();
        if (type.isEmpty())
            return;

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;

        //截图：抓取桌面整屏后裁剪出视口区域。
        //裁剪基准=渲染控件本体：REBUILD_PostRend 继承 mBaseRend(QWidget)，其全局几何才是
        //可见 GL 视口；activatePostViewport 返回的容器在单视口重构后几何不代表可见视口
        //（2026-11-16 用户实测裁剪偏移实证，crop 尺寸恒 640×480）
        QWidget* grabHost = subwin;
        if (REBUILD_PostRend* rendHost = getCVM->getPostRenderWidget())
            grabHost = rendHost;
        //grabWindow(0) 拿到的是 DWM 合成后的最终画面（logo 水印等叠加层与运行时显示一致），
        //且不需要目标窗口的原生句柄，不会触发 winId() 本地化导致界面闪黑
        QScreen* screen = QGuiApplication::screenAt(grabHost->mapToGlobal(QPoint(0, 0)));
        if (screen == nullptr)
            screen = QGuiApplication::primaryScreen();
        QPixmap fullScreen = screen->grabWindow(0);
        QPoint topLeft = grabHost->mapToGlobal(QPoint(0, 0));
        qreal dpr = fullScreen.devicePixelRatio();
        QRect pixelRect(static_cast<int>(topLeft.x() * dpr), static_cast<int>(topLeft.y() * dpr),
                        static_cast<int>(grabHost->width() * dpr), static_cast<int>(grabHost->height() * dpr));
        QPixmap picture = fullScreen.copy(pixelRect);

        //图像输出格式(默认 PNG)
        QString imageFormat = "PNG";
        if (_comboBox_imageFormat != nullptr && _comboBox_imageFormat->currentText().size() > 0)
            imageFormat = _comboBox_imageFormat->currentText().toUpper();
        QString ext = imageFormat.toLower();
        if (imageFormat == "JPEG")
            ext = "jpg";

        //生成路径及文件名：按视图名称隔离目录
        QString imagesdir = getExportWordImageDir();
        QDir dir(imagesdir);
        if (!dir.exists())
            dir.mkpath(imagesdir);

        //统计所有支持格式的已有文件,避免跨格式重名
        QStringList namelist = dir.entryList(QStringList{ "*.png","*.PNG","*.jpg","*.jpeg","*.JPEG","*.bmp","*.BMP","*.tif","*.tiff","*.TIFF" });
        int namei = namelist.size() + 1;
        QString filename = QString("pic_%1.%2").arg(namei).arg(ext);
        while (namelist.contains(filename))
        {
            namei++;
            filename = QString("pic_%1.%2").arg(namei).arg(ext);
        }

        //保存
        bool issave = picture.save(imagesdir + "/" + filename, imageFormat.toLatin1().constData());
        if (issave)
        {
            mOutputMessage msg(NormalMessage, QString("保存成功！"));
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("保存成功！"), true);

            PostData::Type_WordContent* content = findWordContent(curveData, type);
            if (content)
            {
                content->pictureList.append(filename);
                _pictureDialog->setImageDir(getExportWordImageDir());
                _pictureDialog->updatePictureList(content->pictureList);
            }
        }
        else
        {
            mOutputMessage msg(ErrorMessage, QString("保存失败！"));
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("保存失败！"), true);
        }
    }

    void mPostExportWordPanel::slot_exportAnimation()
    {
        //H3D 走数据导出链路（Generic ASCII + HvTrans），不走屏幕录制
        if (_comboBox_videoFormat->currentText().compare("H3D", Qt::CaseInsensitive) == 0)
        {
            exportAnimationH3D();
            return;
        }

        //检查视频编码依赖
        //mInfoWindow::getInstance()->slot_aniExportVideo();
        //return;

        QString ffmpegDir = QCoreApplication::applicationDirPath() + "/ffmpeg";
        if (!QDir(ffmpegDir).exists())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("缺少视频依赖文件，请在安装目录下检查ffmpeg文件夹！"), true);
            return;
        }

        //当前报告视口的渲染器
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();//getCVM->getPostRenderByViewName(_curViewName);
        if (postRend == nullptr || postRend->getAnimationWindow() == nullptr)
            return;

        QWidget* msWindow = getCVM->activatePostViewport();//getCVM->getViewport(_curViewName);
        if (msWindow == nullptr)
            return;

        //录制结束提示
        disconnect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_animationFinished()));
        connect(postRend->getAnimationWindow(), SIGNAL(signal_EndAnimation()), this, SLOT(slot_animationFinished()));

        //分辨率宽高需为 2 的倍数；缓冲放大 2 倍提高清晰度（上限 8192）
        QSize size = postRend->size();
        int width = size.width();// -(size.width() % 2);
        int height = size.height();// -(size.height() % 2);
        if (width % 2 != 0)//确保值是2的倍数
        {
            width -= 1;
        }
        if (height % 2 != 0)//确保值是2的倍数
        {
            height -= 1;
        }
        float resScale = 1.0f;
        /*if (width * resScale > 8192 || height * resScale > 8192)
            resScale = 1.0f;*/
        postRend->getAnimationWindow()->setAnimationSize(width, height, resScale);
        postRend->getAnimationWindow()->setAnimationFrameRate(10);

        //离线缓冲即整个视口内容，裁剪区域取全帧；偏移量会导致画面错位/宽高比不对
        int x = msWindow->pos().x();
        int y = msWindow->pos().y();
        postRend->getAnimationWindow()->setAnimationRect(x, y, width, height);

        //帧范围取当前动画设置
        int fNo = mPostInfoHub::getInstance()->slot_aniStartFrame();
        int sNo = mPostInfoHub::getInstance()->slot_aniEndFrame();

        //录制前先把视口定位到起始帧：引擎首帧记录的是开始录制时视口正在显示的帧，
        //若动画停在其他位置（如上次播放结束处），序列起始位置会不对
        //postRend->getPostRender()->toAnimation(fNo);
        //postRend->update();

        //输出到工作目录,文件名带时间戳避免重名
        QString path = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QDir dir(path);
        if (!dir.exists())
            dir.mkpath(path);
        QString fileName = QString("报告动画%1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss"));

        //动画格式:当前视口正在播放的动画(云图/变形/模态/瞬态/多体动力学等)由引擎逐帧录制编码
        QString suffix = "." + _comboBox_videoFormat->currentText().toLower();
        // B1b(M6/V-3)：驱动点就绪检查（帧范围）
        if (!checkRecordFramesReady(postRend, fNo, sNo)) return;
        postRend->getAnimationWindow()->RecordAnimation(fNo, sNo,
            { 255.0f, 255.0f, 255.0f }, fileName.toStdString(), path.toStdString(), suffix.toStdString());

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("开始动画录制: %1%2").arg(fileName).arg(suffix), true);
        postRend->update();
    }

    void mPostExportWordPanel::slot_animationFinished()
    {
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("动画录制完成！"), true);
    }

    void mPostExportWordPanel::exportAnimationH3D()
    {
        // 1) 后处理数据库
        PostData::mPostDataBase* postDB = nullptr;
        if (getCVM->getModelData() != nullptr)
            postDB = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getPostDatabase();
        if (postDB == nullptr)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("后处理数据库为空，请先加载结果"), true);
            return;
        }

        // 2) 帧范围：动画面板当前起止帧 → 数据库帧号。
        // 注意：getFrameIds() 基于 QHash，键顺序不确定，必须按帧号升序排序后
        // 再用下标截取，否则选错帧且 H3D 帧顺序错乱
        QList<int> allFrameIds = postDB->getFrameIds();
        std::sort(allFrameIds.begin(), allFrameIds.end());
        const int fNo = mPostInfoHub::getInstance()->slot_aniStartFrame();
        const int sNo = mPostInfoHub::getInstance()->slot_aniEndFrame();
        QList<int> frameIds;
        for (int i = 0; i < allFrameIds.size(); ++i)
        {
            if (i >= fNo && i <= sNo)
                frameIds.append(allFrameIds[i]);
        }
        if (frameIds.isEmpty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("帧范围无效，请先在动画面板设置帧范围"), true);
            return;
        }

        // 3) 抽数据构建 Generic ASCII 数据模型
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("正在抽取网格与帧数据..."), true);
        altairresultio::AgaModel model;
        QString error;
        QString skipped;
        if (!MxPost::buildAgaModelFromPost(postDB, frameIds, model, error, &skipped))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("H3D 导出失败: %1").arg(error), true);
            return;
        }
        if (!skipped.isEmpty())
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("以下单元暂不支持导出，已跳过: %1").arg(skipped), true);

        // 4) 写出 Generic ASCII
        const QString workDir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QDir(workDir).mkpath(".");
        const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss");
        const QString asciiPath = QDir(workDir).absoluteFilePath(QString("报告动画%1.ascii").arg(stamp));
        altairresultio::AgaWriteOptions opt;
        std::string writeError;
        if (!altairresultio::AgaWriter::write(model, asciiPath.toStdString(), opt, writeError))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                QString("H3D 导出失败: %1").arg(QString::fromStdString(writeError)), true);
            return;
        }
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("Generic ASCII 已写出: %1").arg(asciiPath), true);

        // 5) HvTrans 转换（QProcess，同步等待）
        const QString hvtrans = MxPost::findHvTransExe();
        if (hvtrans.isEmpty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                QString("未找到 hvtrans.exe，请安装 HyperWorks 或设置 ALTAIR_HOME 环境变量。"
                        "Generic ASCII 文件已生成，可稍后手动转换: %1").arg(asciiPath), true);
            return;
        }

        const QString h3dPath = QDir(workDir).absoluteFilePath(QString("报告动画%1.h3d").arg(stamp));
        QProcess process;
        process.start(hvtrans, QStringList() << "-ascii" << asciiPath << "-h3d" << h3dPath);
        if (!process.waitForStarted(5000) || !process.waitForFinished(10 * 60 * 1000))
        {
            process.kill();
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("HvTrans 转换超时或启动失败"), true);
            return;
        }
        if (process.exitCode() != 0 || !QFile::exists(h3dPath))
        {
            const QString log = QString::fromLocal8Bit(process.readAllStandardError());
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                QString("HvTrans 转换失败(退出码 %1): %2").arg(process.exitCode()).arg(log.left(500)), true);
            return;
        }

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("H3D 导出完成: %1").arg(h3dPath), true);
    }

    QStringList mPostExportWordPanel::findAllAnimationVideos()
    {
        //在工作目录中查找所有已导出的动画文件(报告动画*.avi/mp4/mpeg/gif)，按修改时间倒序
        QStringList result;
        QString path = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QDir dir(path);
        if (!dir.exists())
            return result;

        const QStringList filters{ "报告动画*.avi", "报告动画*.mp4", "报告动画*.mpeg", "报告动画*.gif" };
        QFileInfoList files;
        for (const QString& filter : filters)
            files.append(dir.entryInfoList(QStringList{ filter }, QDir::Files));

        //按修改时间倒序：最新导出的排在报告最后生成的动画前
        std::sort(files.begin(), files.end(),
            [](const QFileInfo& a, const QFileInfo& b) { return a.lastModified() > b.lastModified(); });

        for (const QFileInfo& fi : files)
            result.append(fi.absoluteFilePath());
        return result;
    }

    void mPostExportWordPanel::slot_editPictureButton()
    {
        // 显示窗口前确保图片目录与当前视图一致
        _pictureDialog->setImageDir(getExportWordImageDir());
        _pictureDialog->show();
    }

    void mPostExportWordPanel::slot_titleCheckBox(bool ischeck)
    {
        _lineEdit_subHeader->setEnabled(ischeck);

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;
        curveData->_reportShowSubTitle = ischeck;
    }

    void mPostExportWordPanel::slot_personCheckBox(bool ischeck)
    {
        _lineEdit_user->setEnabled(ischeck);

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;
        curveData->_reportShowUser = ischeck;
    }

    void mPostExportWordPanel::slot_companyCheckBox(bool ischeck)
    {
        _lineEdit_company->setEnabled(ischeck);

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;
        curveData->_reportShowCompany = ischeck;
    }

    void mPostExportWordPanel::slot_addressCheckBox(bool ischeck)
    {
        _lineEdit_address->setEnabled(ischeck);

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;
        curveData->_reportShowAddress = ischeck;
    }

    void mPostExportWordPanel::slot_phoneCheckBox(bool ischeck)
    {
        _lineEdit_phone->setEnabled(ischeck);

        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;
        curveData->_reportShowPhone = ischeck;
    }

    void mPostExportWordPanel::slot_titleTextChanged()
    {
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;
        curveData->_reportSubTitle = _lineEdit_subHeader->text();
    }

    void mPostExportWordPanel::slot_userTextChanged()
    {
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;
        curveData->_reportUser = _lineEdit_user->text();
    }

    void mPostExportWordPanel::slot_companyTextChanged()
    {
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;
        curveData->_reportCompany = _lineEdit_company->text();
    }

    void mPostExportWordPanel::slot_addressTextChanged()
    {
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;
        curveData->_reportAddress = _lineEdit_address->text();
    }

    void mPostExportWordPanel::slot_phoneTextChanged()
    {
        PostData::mPostExportWordData* curveData = getExportWordData();
        if (curveData == nullptr)
            return;
        curveData->_reportPhone = _lineEdit_phone->text();
    }

    void mPostExportWordPanel::slot_generateButton()
    {
        QString exePath = QCoreApplication::applicationDirPath();
        /*QDir report(exePath + "/RePort");
        if (!report.exists())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有前处理信息,请检查..."), true);
            return;
        }*/

        clearAllContainer();
        getCurrentTime();

        //获取结果信息
        getResultInfo();

        //计算文件：优先取菜单栏导入的计算文件（如 ballDrop.k），无则回退旧版 RePort 目录机制
        mxFileName_ = MechIOFile::mIOFileManager::getInstance()->getReadSolveFileName();
        if (mxFileName_.isEmpty())
        {
            mxFileName_ = getmxFileName(exePath + "/RePort/");
            mxFileName_.replace(".txt", ".mx");
        }
        //仅保留文件名（导入的是完整路径）
        if (!mxFileName_.isEmpty())
            mxFileName_ = QFileInfo(mxFileName_).fileName();

        //获取文件用户图片信息
        getModelImageFiles(exePath + "/RePort/");

        // 收集报告上下文
        PostData::mPostReportContext context;
        QString collectError;
        if (!collectReportContext(context, collectError))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("导出报告失败: %1").arg(collectError), true);
            return;
        }

        // 追加已导出的动画文件到报告末尾（无动画文件则跳过）
        context.animationPaths = findAllAnimationVideos();

        // 选择渲染器：直接取下拉框当前项，避免缓存的 _selectedFormat 过期
        if (_comboBox_format != nullptr)
            _selectedFormat = _comboBox_format->currentData().toString();
        PostData::mPostReportRendererFactory* factory = PostData::mPostReportRendererFactory::instance();
        QSharedPointer<PostData::mPostReportRenderer> renderer = factory->createRenderer(_selectedFormat);
        if (renderer.isNull())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("不支持的导出格式: %1").arg(_selectedFormat), true);
            return;
        }

        // 获取保存路径
        QString fileFilter = renderer->fileFilter();
        QString defaultSuffix;
        if (fileFilter.contains("*.html"))
            defaultSuffix = ".html";
        else if (fileFilter.contains("*.docx"))
            defaultSuffix = ".docx";
        else if (fileFilter.contains("*.pptx"))
            defaultSuffix = ".pptx";

        QString savePath = QFileDialog::getSaveFileName(nullptr, QString("保存报告"),
            QString("MxSimReport%1").arg(defaultSuffix), fileFilter);
        if (savePath.isEmpty())
            return;
        if (QFileInfo(savePath).suffix().isEmpty())
            savePath.append(defaultSuffix);

        // 渲染并保存
        QString renderError;
        if (!renderer->render(context, savePath, renderError))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("导出报告失败: %1").arg(renderError), true);
            return;
        }

        //是否自动打开
        if (_pushButton_open->isChecked()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(savePath));
        }
    }

    void mPostExportWordPanel::slot_autoOpen()
    {
        QPushButton* btn = qobject_cast<QPushButton*>(sender());
        if (btn == nullptr) return;

        btn->setChecked(!btn->isChecked());
    }

    void mPostExportWordPanel::slot_changeLanguage(int language)
    {
        if (language == 0)
        {
            _contentFold->setTitle(QString::fromUtf8("报告内容"));
            _titleFold->setTitle(QString::fromUtf8("标题信息"));
            _executeFold->setTitle(QString::fromUtf8("执行"));

            _checkBox_text->setText("文本");
            _checkBox_screenshot->setText("截图");

            _checkBox_subheader->setText("副标题");
            _checkBox_user->setText("分析员");
            _checkBox_company->setText("公司");
            _checkBox_addresa->setText("地址");
            _checkBox_phone->setText("电话");
            _pushButton_generator->setText("生成");
            _pushButton_saveView->setText("保存当前视图");
            _pushButton_editScreenshot->setText("编辑截图");
            _pushButton_open->setText("自动打开");
            _pushButton_exportVideo->setText("导出动画");

            // 条目文本按 userData 重写（条目由 initFormatComboBox 按 data 填充）
            if (_comboBox_format)
            {
                int idx = _comboBox_format->findData("Word");
                if (idx >= 0) _comboBox_format->setItemText(idx, "Word");
                idx = _comboBox_format->findData("HTML");
                if (idx >= 0) _comboBox_format->setItemText(idx, "HTML");
                idx = _comboBox_format->findData("PowerPoint");
                if (idx >= 0) _comboBox_format->setItemText(idx, "PowerPoint");
            }
        }
        else if (language == 1)
        {
            _contentFold->setTitle(QString::fromUtf8("Report Content"));
            _titleFold->setTitle(QString::fromUtf8("Title Info"));
            _executeFold->setTitle(QString::fromUtf8("Execute"));

            _checkBox_text->setText("Text");
            _checkBox_screenshot->setText("ScreenShot");

            _checkBox_subheader->setText("SubTitle");
            _checkBox_user->setText("Analyst");
            _checkBox_company->setText("Company");
            _checkBox_addresa->setText("Address");
            _checkBox_phone->setText("Phone");
            _pushButton_generator->setText("Generate");
            _pushButton_saveView->setText("Save View");
            _pushButton_editScreenshot->setText("Edit View");
            _pushButton_open->setText("Auto Open");
            _pushButton_exportVideo->setText("Export Animation");

            if (_comboBox_format)
            {
                int idx = _comboBox_format->findData("Word");
                if (idx >= 0) _comboBox_format->setItemText(idx, "Word");
                idx = _comboBox_format->findData("HTML");
                if (idx >= 0) _comboBox_format->setItemText(idx, "HTML");
                idx = _comboBox_format->findData("PowerPoint");
                if (idx >= 0) _comboBox_format->setItemText(idx, "PowerPoint");
            }
        }
    }

    void mPostExportWordPanel::initFormatComboBox()
    {
        if (_comboBox_format == nullptr)
            return;

        _comboBox_format->clear();
        PostData::mPostReportRendererFactory* factory = PostData::mPostReportRendererFactory::instance();
        //本面板构造可能早于 mMenuWidget::registerAPI()，工厂为空时就地注册内置渲染器，
        //避免 comboBox_format 因初始化顺序而为空（registerRenderer 内部按 name 去重，重复调用安全）
        if (factory->availableNames().isEmpty())
            registerPostExportRenderers();
        QStringList names = factory->availableNames();
        for (const QString& name : names)
        {
            _comboBox_format->addItem(name, name);
        }

        if (_comboBox_format->count() > 0)
        {
            _comboBox_format->setCurrentIndex(0);
            _selectedFormat = _comboBox_format->currentData().toString();
        }
    }

    void mPostExportWordPanel::slot_formatChanged(int index)
    {
        if (_comboBox_format == nullptr)
            return;
        _selectedFormat = _comboBox_format->itemData(index).toString();
    }

    bool mPostExportWordPanel::collectReportContext(PostData::mPostReportContext& context, QString& error)
    {
        // 当前先保持兼容：把现有 HTML 生成逻辑中已收集的数据整理成上下文
        // 后续逐步迁移到 mPostReportDataCollector
        context.clear();

        // Meta
        context.meta.mainTitle = "MxSim仿真分析报告";
        if (_checkBox_subheader->isChecked())
        {
            context.meta.showSubTitle = true;
            context.meta.subTitle = _lineEdit_subHeader->text();
        }
        if (_checkBox_user->isChecked())
        {
            context.meta.showAuthor = true;
            context.meta.author = _lineEdit_user->text();
        }
        if (_checkBox_company->isChecked())
        {
            context.meta.showCompany = true;
            context.meta.company = _lineEdit_company->text();
        }
        if (_checkBox_addresa->isChecked())
        {
            context.meta.showAddress = true;
            context.meta.address = _lineEdit_address->text();
        }
        if (_checkBox_phone->isChecked())
        {
            context.meta.showPhone = true;
            context.meta.phone = _lineEdit_phone->text();
        }
        context.meta.dateTime = testDate_;
        context.meta.mxFileName = mxFileName_;

        // 概述表格
        PostData::ReportSection summarySec;
        summarySec.id = "summary";
        summarySec.title = "用例概述";
        PostData::ReportTable summaryTable;
        summaryTable.title = "用例概述";
        summaryTable.columnCount = 2;

        auto addSummaryRow = [&](const QString& name, const QString& value) {
            PostData::ReportRow row;
            PostData::ReportCell headerCell;
            headerCell.text = name;
            headerCell.isHeader = true;
            headerCell.hAlign = "right";
            row.cells.append(headerCell);
            PostData::ReportCell valueCell;
            valueCell.text = value;
            valueCell.hAlign = "center";
            row.cells.append(valueCell);
            summaryTable.rows.append(row);
        };
        addSummaryRow("计算文件", mxFileName_);
        addSummaryRow("测试时间", testDate_);
        if (context.meta.showAuthor)
            addSummaryRow("分析员", context.meta.author);
        if (context.meta.showCompany)
            addSummaryRow("公司", context.meta.company);
        if (context.meta.showAddress)
            addSummaryRow("地址", context.meta.address);
        if (context.meta.showPhone)
            addSummaryRow("电话", context.meta.phone);

        PostData::ReportSectionItem summaryItem;
        summaryItem.type = PostData::ReportSectionItem::Table;
        summaryItem.table = summaryTable;
        summarySec.items.append(summaryItem);
        context.addSection(summarySec);

        // 各内容章节
        _contentIndex = 0;
        QStringList contentlist = PostCommon::mCommonFunctions::getCheckedItemNames(_listWidget);
        for (QString contentstr : contentlist) {
            _contentIndex++;
            if (contentstr == QString("说明") || contentstr == "Specification") {
                collectDescriptionContext(context);
            }
            else if (contentstr == QString("模型") || contentstr == "Model") {
                collectModelContext(context);
            }
            else if (contentstr == QString("材料") || contentstr == "Material") {
                collectMaterialContext(context);
            }
            else if (contentstr == QString("截面属性") || contentstr == "Property") {
                collectPropertyContext(context);
            }
            else if (contentstr == QString("分析工况") || contentstr == "Case") {
                collectCaseContext(context);
            }
            else if (contentstr == QString("载荷") || contentstr == "Load") {
                collectLoadContext(context);
            }
            else if (contentstr == QString("边界") || contentstr == "Boundary") {
                collectBCContext(context);
            }
            else if (contentstr == QString("连接") || contentstr == "Connect") {
                collectCNContext(context);
            }
            else if (contentstr == QString("接触") || contentstr == "Contact") {
                collectCTContext(context);
            }
            else if (contentstr == QString("结果") || contentstr == "Result") {
                collectResultContext(context);
            }
        }

        return true;
    }

    void mPostExportWordPanel::updatePickFilterAndSymbol(bool flag)
    {
        PreViewportWidget* postRend = getCVM->getPreViewportWidget();
        if (postRend == nullptr) return;
        /*if (postRend->getViewportName() != _curViewName)
            return;*/
        auto  postRender = postRend->renderWidget();
        if (postRender == nullptr) return;

        postRender->setPickFilter(PickFilter::PickNothing);

    }
}