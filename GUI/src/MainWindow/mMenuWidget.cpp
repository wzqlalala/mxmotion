#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "mMenuWidget.h"
#include "mCaseImplicitModalWindow.h"
#include "mCaseImplicitStiffnessWindow.h"
#include "mCaseImplicitVTFWindow.h"
#include "mCaseImplicitSuperElementWindow.h"
#include "mCaseImplicitNTFWindow.h"
#include "mCaseImplicitFrequencyResponseWindow.h"
#include "mCaseImplicitTransientWindow.h"
#include "mCaseImplicitRandomVibrationWindow.h"
#include "mCaseTimeSeriesHeatWizard.h"
#include "mPostDataManager.h"   //动画视口判定经管理器关系（显示名接管后名字不可解析）
#include "mUiMenu.h"
#include "UiMetrics.h"
#include "UiPalette.h"
#include "ThemeManager.h"
#include "mFramelessDialog.h"
#include "mInfoWindow.h"
#include "mCurveWindow.h"
#include "mTableDataWindow.h"
#include "mBrowserWindow.h"
#include "mCenterMainWindow.h"
#include "mSectionWindow.h"
#include "mSelectInfoWindow.h"
#include "mOutputInfoWindow.h"
#include "mDockLayoutManager.h"
#include "mMotionLoadImportPanel.h"
#include "mSelectMenuFunction.h"
#include "mDisplayMenuFunction.h"
#include "mViewTreeFunction.h"
#include "mCalcLineEdit.h"
#include "mUnitIoCodec.h"
#include "mCaseWindow.h"

//Qt
#include <QVBoxLayout>
#include <QLabel>

//mpif 点到点映射示例（引路用）依赖的 FieldTransfer 库
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <FieldTransferCore/Model.hpp>
#include <FieldTransferCore/MpifReader.hpp>
#include <FieldTransferCore/DataBoundaries.hpp>
#include <FieldTransferCore/Diagnostic.hpp>
#include <FieldTransferMapping/Mapping.hpp>
#include <QMenu>
#include <QMenuBar>
#include <QScrollBar>
#include <QPainter>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QPainter>
#include <QStyleOptionButton>
#include <QMouseEvent>
#include <QCursor>
#include <QWidgetAction>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QDesktopWidget>
#include <QFileDialog>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QEventLoop>
#include <QRadioButton>
#include <QCollator>
#include <QSlider>
#include <QSplitter>
#include <QComboBox>
#include <QTextBrowser>
#include <QStandardPaths>
#include <QColorDialog>
#include <QFrame>
#include <QFontDatabase>
//QSql
#include <QSqlQuery>
#include <QSqlError>
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QDirIterator>
#include <QDebug>
#include <QString>
#include <QFontMetrics>

#include <cmath>
#include <functional>

//MxFunctions
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mPhysicalFieldEnum.h"

//panel
#include "mInfoWindow.h"
#include "mSectionWindow.h"
#include "mSetPanel.h"
//多体动力学（VectorLoadType/SpringType枚举）
#include "mMotionForceElement.h"
#include "mMotionSpringBase.h"
//直接建模集成
#include "mDirectModelingController.h"
#include "mTransFileInfoDialog.h"
#include "mBrowserWindow.h"
#include "mCenterMainWindow.h"
#include "mTreeView.h"
#include "mGeoPanelBase.h"


//AICAE

//IOFile
#include "mIOFileManager.h"
#include "mMxsesRestore.h"
//CustomTooltip
#include "mCustomTooltip.h"

#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/view/mBackGroundRender.h"
//PreData
#include "mViewportModel.h"
#include "mGeoManager.h"
#include "mDataStatusManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"

//UnitSystem
#include "mUnitSystemManager.h"

//Material
#include "mMaterialWindow.h"
#include "mMaterialProps.h"
#include "mMechGlobalSignals.h"
#include <QFontDatabase>
#include "mCustomMaterialWindow.h"
#include "mFrpCompMaterialWindow.h"

//MechSetData
#include "mSetBase.h"
#include "mNode.h"
#include "mElementSet.h"
#include "mElementSurface.h"
#include "mGeoSet.h"
#include "mPartSet.h"
#include "mModelData.h"
#include "mSolveTaskCenter.h"
#include "mSolveTaskDialog.h"

//MechCoordinateData
#include "mCoordinateBase.h"

//Post

//commond
#include "mCommondSystem.h"
#include "mCommonds.h"

//Mesh
//#include "MXMeshAPI.h"
#include "mMeshData.h"
#include "MXMesh.h"
#include "MXMeshParameters.h"
//#include "mMeshSolid1.h"
#include "mMeshTransform.h"
#include "mMeshEdit.h"

#include "mModelData.h"
//#include "mGeoCommonFunction.h" //【OCC摘除·S10/P0-1 终局2】死 include（零使用；该头携带 OCC 链）
#include "mPlotView.h"
#include "mPlotViewStyleWidget.h"
#include "mFunctionManager.h"
//#include "mGeoCommandSolidOfSketchOCC.h" //【OCC摘除·S10/P0-1】退役头死 include（零引用）
//#include "mGeoCommandFactory.h" //【OCC摘除·S10/P0-1】退役头死 include（零引用）

#include "mxr/view/HighlightController.h"
#include "PickCommonFunction.h"
#include "TimerWidget.h"
#include "mLicense.h"
#include "mCaseWindow.h"
#include "PreViewportWidget.h"

#include "InteractionCore/Tool/CreateSketchGeometryTool.h"
#include "InteractionCore/Tool/MoveGeometrySolidTool.h"

#ifdef HAS_CGM_INTEROP
#ifdef CGM_VERSION_WIN10
#include "CGMDataExchangeWin10.h"
#else
#include "CGMDataExchange.h"
#endif
#endif
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//MechCaseData
#include "mLoadBoundaryManager.h"
#include "mCaseManager.h"
#include "mCaseImplicitModal.h"
#include <QDirIterator>
#include <QDebug>
#include <QString>
#include "mVRMLExporter.h"

#include "mGeoSketchSignalManager.h"
#include "mMeshIO.h"
#include "GlobalMeshMessage.h"

#include "mShortcutManager.h"
#include "mShortcutActionList.h"
#include <mGeoPythonAPIRegistrar.h>
#include "mOtherFunction.h"
#include <mPostExportAPIRegistrar.h>  //【集成回补】dev 后处理导出 API 注册（PostIOFile 导出）
#include "mRibbonWidget.h"
#include "mOpenCurveDlg.h"  //【集成回补】PostCurve 曲线打开对话框（PchReader 公共头）  //【集成回补】mGroupWidget 完整类型（原经传递链可见，链断后显式自含）
#include<mMeshPythonAPIRegistrar.h>
#include <muParser.h>   // 解析曲线导出中的表达式求值（合并 dev_base 后 include 被清，补回）

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>  
#endif


using namespace CommonData;
using namespace MxFunctions;


using namespace MechIOFile;
using namespace MechPreData;
using namespace MechCommon;
using namespace Common;
using namespace MGeoCommand;
using namespace MechCaseData;
#define OUT_HEADER(ts, tp) \
    ts << "# FunctionType: " << (tp) << "\n" \
       << "# Variable: x\n" \
       << "# Range: " << getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->xMin() << " ~ " << getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->xMax() << "\n"



    /*
    * 左右键双功能按钮的事件过滤器：左键走按钮原有信号，右键触发onRightClick
    * 用于“步骤/旋转/缩放”等融合按钮（左键正向操作，右键反向操作）
    */
    class DualClickFilter : public QObject
    {
    public:
        explicit DualClickFilter(QObject* parent = nullptr) : QObject(parent) {}
        std::function<void()> onRightClick;
    protected:
        bool eventFilter(QObject* watched, QEvent* event) override
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::RightButton && onRightClick)
                {
                    onRightClick();
                    return true;
                }
            }
            return QObject::eventFilter(watched, event);
        }
    };


QStringList argvToQStringListUtf16()
{
    QStringList out;

#ifdef _WIN32
    int argc = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!wargv) return out;

    out.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        out.push_back(QString::fromWCharArray(wargv[i])); // UTF-16 -> QString
    }
    LocalFree(wargv);
#else
    // 非 Windows：Qt 6 通常 argv 即 UTF-8；这里给个通用兜底
    const QStringList args = QCoreApplication::arguments();
    out = args;
#endif

    return out;
}

namespace MechMainWindow
{

	mMenuWidget::mMenuWidget(QMenuBar* parent) : QMenuBar(parent)
	{
		// 启动时先恢复持久化的档位+主题，再初始化控件（控件构造即按当前档位/主题取初值）
		ThemeManager::instance()->loadSettings();
		this->resize(800, UiMetrics::menuBarTotalHeight());
		view = nullptr;
		viewAbout = nullptr;
		//缺 QtWebEngineProcess 时 initCaseLibrary 整体跳过不建弹窗，判空兜底用
		caseLibraryDialog = nullptr;

		initVariable();
		initDialog();
		initMenuBtn();
		initMpcomm();
#ifdef MX_AUTOSAVE_ENABLED
		//主窗口显示、事件循环启动后再检测自动保存文件，避免在初始化中途弹窗
		QTimer::singleShot(0, this, &mMenuWidget::checkAutoSaveRecovery);
#endif //MX_AUTOSAVE_ENABLED
		/*m_showFuntion = ShowFuntion(0);*/
		registerAPI();

		// 菜单栏初始化完成后统一下发一次尺寸+主题，保证所有控件（含后注册页）一致
		ThemeManager::instance()->applyAll(this);
	}

	/*	mMenuWidget	*/
	mMenuWidget::~mMenuWidget()
	{
        unregisterAPI();
        if (view)
            delete view;
        if (viewAbout)
            delete viewAbout;
        if (m_customMaterialWindow)
            delete m_customMaterialWindow;
        if (m_caseImplicitModalWindow)
            delete m_caseImplicitModalWindow;
        if (m_caseImplicitStiffnessWindow)
            delete m_caseImplicitStiffnessWindow;
        if (m_caseImplicitVTFWindow)
            delete m_caseImplicitVTFWindow;
        if (m_FrpCompMaterialWindow)
            delete m_FrpCompMaterialWindow;
	}

    void mMenuWidget::registerAPI()
    {
        registerPostExportAPIs();
        mGeoPythonAPIRegistrar::instance()->registerGeometryAPIs();
        mMeshPythonAPIRegistrar::instance()->registerMeshAPIs();
    }

    void mMenuWidget::unregisterAPI()
    {
        unregisterPostExportAPIs();
        mGeoPythonAPIRegistrar::instance()->unregisterGeometryAPIs();
        mMeshPythonAPIRegistrar::instance()->unregisterMeshAPIs();
    }

    void mMenuWidget::setCenterMainWindow(mCenterMainWindow* window)
    {
        m_centerMainWindow = window;
    }

    void mMenuWidget::setMaterialWindow(mMaterialWindow* window)
    {
        m_materialWindow = window;
    }

    void mMenuWidget::cmdExecute(QStringList cmdList)
    {
        cmdList = argvToQStringListUtf16();
        if (cmdList.size() == 0)
            return;

        int index = cmdList.indexOf("-i");
        if (index != -1 && cmdList.size() > index + 1)
        {
            importMxFile(cmdList[index + 1]);
            mIOFileManager::getInstance()->_cmdImportFilePath = QFileInfo(cmdList[index + 1]).absolutePath();
            connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::threadExecuteFinishSig, this, &mMenuWidget::slot_cmdExecute);
        }
    }

    void mMenuWidget::initDialog()
    {
        //最近打开弹窗
        initRecentOpen();
        //首选项Preferences弹窗
        initPreferences();
        //关于MxSim.Mechanical弹窗
        initAbout();
        //案例库弹窗
        initCaseLibrary();
        //许可证弹窗
        initLicense();
        //初始化自定义材料弹窗
        m_customMaterialWindow = new mCustomMaterialWindow(this);
    }
    void mMenuWidget::initRecentOpen()
    {
        recentlyOpenedDialog = new mFramelessDialog(this);
        recentlyOpenedDialog->setWindowTitle("最近打开 - [预览]");
        QHBoxLayout* h = new QHBoxLayout;
        QHBoxLayout* h1 = new QHBoxLayout;
        QHBoxLayout* h2 = new QHBoxLayout;
        QHBoxLayout* h3 = new QHBoxLayout();

        QWidget* vBox1 = new QWidget;
        QWidget* vBox2 = new QWidget;
        QVBoxLayout* v1 = new QVBoxLayout(vBox1);
        QVBoxLayout* v2 = new QVBoxLayout(vBox2);
        QLabel* label1 = new QLabel("过滤");
        QLineEdit* lineedit = new QLineEdit();

        recentTable = new QTableWidget(2, 2);
        recentTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        recentTable->setHorizontalHeaderLabels({ "名称", "路径" });
        QPushButton* btn = new QPushButton("打开");
        QPushButton* btn1 = new QPushButton("删除");

        QLabel* label2 = new QLabel("模型快照预览");
        QString dir = QDir::currentPath();
        QString name = dir + "/../../test.png";
        imageWidget = new AutoScaledPNG(name);

        h1->addWidget(label1);
        h1->addWidget(lineedit);
        h3->addWidget(recentTable);
        h2->addWidget(btn);
        h2->addWidget(btn1);
        v1->addLayout(h1);
        v1->addLayout(h3);
        v1->addLayout(h2);
        vBox2->setMinimumSize(imageWidget->width() + 30, imageWidget->height() + 30);
        recentlyOpenedDialog->setMinimumHeight(imageWidget->height() + 30);
        v2->addWidget(label2);
        v2->addWidget(imageWidget, 1);
        /*v2->addStretch(1);*/
        QSplitter* split = new QSplitter(Qt::Horizontal);
        split->addWidget(vBox1);
        split->addWidget(vBox2);
        h->addWidget(split);   // 把 splitter 放进主窗
        h->addLayout(v1);
        h->addLayout(v2);
        recentlyOpenedDialog->setExternalLayout(h);
        recentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        recentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        vBox1->setMinimumWidth(400);
        //单击
        connect(recentTable, &QTableWidget::cellClicked, this, [=](int row, int col) {
            QString fileName = recentTable->item(row, col)->text();
            QString id = recentTable->item(row, 0)->data(Qt::UserRole).toString();

            QDir exeDir(QCoreApplication::applicationDirPath());
            exeDir.cdUp();
            QString dir = exeDir.filePath("Setting/Recent");
            QString name = dir + "/" + QFileInfo(fileName).baseName() + id + ".png";

            if (!QFile::exists(name))
                return;

            QPixmap originalPix(name);

            imageWidget->setPixmap(originalPix);

            QSize initSize = originalPix.size() / 2;
            imageWidget->setMinimumSize(200, 150);
            imageWidget->resize(initSize);

            vBox2->setMinimumSize(initSize.width() + 40, initSize.height() + 40);
            recentlyOpenedDialog->setMinimumHeight(initSize.height() + 40);

            imageWidget->update();
            });
        //双击
        connect(recentTable, &QTableWidget::cellDoubleClicked, this, [=](int row, int col) {
            if (getCVM->isPostModel())
            {
                mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入该模型，请选择前处理模型！"));
                return;
            }
            QString fileName = recentTable->item(row, 1)->text();
            QString suffix = QFileInfo(fileName).suffix();
            QStringList meshSuffixs{ "obj","ply","off","stl" };
            if (!fileName.isEmpty()) {
                if (suffix == "mx")
                {
                    MechIOFile::mIOFileManager::getInstance()->slot_importSolveFile(fileName);
                }
                else if (meshSuffixs.contains(suffix)) {
                    MechIOFile::mIOFileManager::getInstance()->readMeshIOFile(fileName);
                }
                else
                {
                    auto modeldata = getCVM->getModelData();
                    auto geomanager = modeldata->getGeoManager();
                    // mpcomm：几何导入完成后回传 committed(geometry)
                    reportMpcommGeometryOnRenderFinished(fileName);
                    geomanager->slotImportGeoFiles(fileName);
                }
                //加入最近打开
                auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
                QSettings Settings(path, QSettings::IniFormat);
                // 添加一条“最近打开”
                QString file = fileName;
                QString key = QString("recent/%1").arg(QDateTime::currentMSecsSinceEpoch());//当前时间距离 1970-01-01 00:00:00 UTC 的毫秒数（int64）

                Settings.beginGroup("recent");
                for (auto k : Settings.childKeys())
                {
                    if (Settings.value(k) == fileName)
                        Settings.remove(QString("%1").arg(k));
                }
                Settings.endGroup();
                Settings.setValue(key, file);
                //模型快照
                QTimer::singleShot(2000, this, [=]() {
                    auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
                    QSettings Settings(path, QSettings::IniFormat);
                    Settings.beginGroup("recent");
                    QStringList keys = Settings.childKeys();


                    QString id = keys[Settings.childKeys().size() - 1];//时间戳
                    Settings.endGroup();

                    getCVM->modelFastCapture(fileName, id);
                    });
            }

            });
        //删除
        connect(btn1, &QPushButton::clicked, [=]() {
            imageWidget->setPixmap(QPixmap(""));
            // 1. 收集选中行（降序）
            QList<int> rows;
            for (QModelIndex idx : recentTable->selectionModel()->selectedRows())
                rows.prepend(idx.row());          // 降序，防止删后索引错位
            auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
            QSettings Settings(path, QSettings::IniFormat);
            Settings.beginGroup("recent");               // 进入 [recent]
            // 2. 逐行删除
            for (int row : rows)
            {
                /* 3. 取路径（第 0 列存的是完整路径） */
                QString path = recentTable->item(row, 0)->text();

                /* 4. 刷新表格（重新 SELECT） */

                QString str = recentTable->item(row, 0)->data(Qt::UserRole).toString();
                Settings.remove(recentTable->item(row, 0)->data(Qt::UserRole).toString());         // 删指定键（含路径值）
                recentTable->removeRow(row);   // 你已有的填充函数
            }
            Settings.endGroup();                         // 离开分组
            });
        //打开
        connect(btn, &QPushButton::clicked, [=]() {
            if (getCVM->isPostModel())
            {
                mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入该模型，请选择前处理模型！"));
                return;
            }
            int row = recentTable->currentRow();
            if (row < 0) return;

            /* 2. 取路径（第 1 列存的是完整路径） */
            QString fileName = recentTable->item(row, 1)->text();
            QString suffix = QFileInfo(fileName).suffix();
            QStringList meshSuffixs{ "obj","ply","off","stl" };
            if (!fileName.isEmpty()) {
                if (suffix == "mx")
                {
                    MechIOFile::mIOFileManager::getInstance()->slot_importSolveFile(fileName);
                }
                else if (meshSuffixs.contains(suffix)) {
                    MechIOFile::mIOFileManager::getInstance()->readMeshIOFile(fileName);
                }
                else
                {
                    auto modeldata = getCVM->getModelData();
                    auto geomanager = modeldata->getGeoManager();
                    // mpcomm：几何导入完成后回传 committed(geometry)
                    reportMpcommGeometryOnRenderFinished(fileName);
                    geomanager->slotImportGeoFiles(fileName);
                }
                //加入最近打开
                auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
                QSettings Settings(path, QSettings::IniFormat);
                // 添加一条“最近打开”
                QString file = fileName;
                QString key = QString("recent/%1").arg(QDateTime::currentMSecsSinceEpoch());//当前时间距离 1970-01-01 00:00:00 UTC 的毫秒数（int64）

                Settings.beginGroup("recent");
                for (auto k : Settings.childKeys())
                {
                    if (Settings.value(k) == fileName)
                        Settings.remove(QString("%1").arg(k));
                }
                Settings.endGroup();
                Settings.setValue(key, file);
                //模型快照
                QTimer::singleShot(2000, this, [=]() {
                    auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
                    QSettings Settings(path, QSettings::IniFormat);
                    Settings.beginGroup("recent");
                    QStringList keys = Settings.childKeys();


                    QString id = keys[Settings.childKeys().size() - 1];//时间戳
                    Settings.endGroup();

                    getCVM->modelFastCapture(fileName, id);
                    });
            }
            });
        // 动态过滤（只比对第 0 列 Name）
        connect(lineedit, &QLineEdit::textChanged, this, [=](const QString& text) {
            for (int row = 0; row < recentTable->rowCount(); ++row) {
                bool match = recentTable->item(row, 0)->text()
                    .contains(text, Qt::CaseInsensitive);
                recentTable->setRowHidden(row, !match);   // 隐藏不匹配行
            }
            });
    }
    void mMenuWidget::initPreferences()
    {
        // 自绘标题栏对话框：边框/阴影/拖拽/最大化动画由 mFramelessDialog 基类
        // 与 mFramelessWindowHelper 统一提供（对齐原生标题栏行为），
        // 内容部件经 setContentWidget 装入标题栏下方
        auto* framelessDlg = new mFramelessDialog(this);
        framelessDlg->setWindowTitle("首选项 - [预览]");
        framelessDlg->setFixedSize(800, 600);
        preferencesDialog = framelessDlg;

        QWidget* pre_content = new QWidget;
        QHBoxLayout* pre_h = new QHBoxLayout(pre_content);
        QHBoxLayout* pre_h1 = new QHBoxLayout;
        QHBoxLayout* pre_h2 = new QHBoxLayout;
        QHBoxLayout* pre_h3 = new QHBoxLayout();

        QWidget* pre_vBox1 = new QWidget;
        QWidget* pre_vBox2 = new QWidget;
        QVBoxLayout* pre_v1 = new QVBoxLayout(pre_vBox1);
        QVBoxLayout* pre_v2 = new QVBoxLayout(pre_vBox2);
        QLabel* pre_label1 = new QLabel("搜索");
        QLineEdit* pre_lineedit = new QLineEdit();
        QTreeWidget* pre_tree = new QTreeWidget;
        QTreeWidgetItem* root = new QTreeWidgetItem(pre_tree);
        QTreeWidgetItem* root1 = new QTreeWidgetItem(pre_tree);
        QTreeWidgetItem* root2 = new QTreeWidgetItem(pre_tree);
        root->setText(0, "文件");
        root1->setText(0, "颜色");
        root2->setText(0, "外观");
        QTreeWidgetItem* child = new QTreeWidgetItem(root);
        child->setText(0, "最近打开");
        QTreeWidgetItem* child1 = new QTreeWidgetItem(root1);
        child1->setText(0, "视口");
        QTreeWidgetItem* child2 = new QTreeWidgetItem(root2);
        child2->setText(0, "尺寸与主题");
        pre_tree->setHeaderHidden(true);
        pre_tree->expandAll();
        QPushButton* pre_btn = new QPushButton("全部置为默认");
        QPushButton* pre_btn1 = new QPushButton("全部展开");
        QPushButton* pre_btn2 = new QPushButton("全部折叠");


        QStackedWidget* pre_stack = new QStackedWidget();
        QWidget* pre_page1 = new QWidget();
        pre_stack->addWidget(pre_page1);
        QVBoxLayout* pre_page1v = new QVBoxLayout;
        QWidget* pre_page2 = new QWidget();
        pre_stack->addWidget(pre_page2);
        QGridLayout* pre_page2v = new QGridLayout;
        //第一页:最近打开
        QLabel* pre_label2 = new QLabel("最近打开");
        QLabel* pre_way = new QLabel("方式");
        QComboBox* pre_waybox = new QComboBox(); pre_waybox->addItems({ "根据时间", "根据数量" });
        pre_waybox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QHBoxLayout* pre_wayBox = new QHBoxLayout();
        pre_wayBox->addWidget(pre_way);
        pre_wayBox->addWidget(pre_waybox);
        QLabel* pre_time = new QLabel("时间");
        mCalcLineEdit* pre_timeEdit = new mCalcLineEdit("30", mCalcLineEdit::CalcMode::StrictPositive, this);
        QLabel* pre_day = new QLabel("天");
        QHBoxLayout* pre_timeBox = new QHBoxLayout();
        pre_timeBox->addWidget(pre_time);
        pre_timeBox->addWidget(pre_timeEdit);
        pre_timeBox->addWidget(pre_day);
        QLabel* pre_num = new QLabel("数量");
        mCalcLineEdit* pre_numEdit = new mCalcLineEdit("10", mCalcLineEdit::CalcMode::StrictPositive, this);
        QLabel* pre_numPiece = new QLabel("条");
        QHBoxLayout* pre_numBox = new QHBoxLayout();
        pre_numBox->addWidget(pre_num);
        pre_numBox->addWidget(pre_numEdit);
        pre_numBox->addWidget(pre_numPiece);
        pre_page1v->addWidget(pre_label2);
        pre_page1v->addLayout(pre_wayBox);
        pre_page1v->addLayout(pre_timeBox);
        pre_page1v->addLayout(pre_numBox);
        pre_page1v->addStretch();
        pre_page1->setLayout(pre_page1v);
        //第二页：颜色
        QLabel* pre_page2label = new QLabel("视口");
        pre_page2v->addWidget(pre_page2label, 0, 0);

        QLabel* pre_mode = new QLabel("模式：");
        QComboBox* pre_modebox = new QComboBox(); pre_modebox->addItems({ "单一颜色", "上下渐变" ,"左右渐变" });
        pre_modebox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        pre_page2v->addWidget(pre_mode, 1, 0);
        pre_page2v->addWidget(pre_modebox, 1, 1, 1, 2);

        QLabel* pre_color = new QLabel("颜色：");
        QPushButton* pre_colorBtn = new QPushButton();
        QToolButton* pre_colorRefresh = new QToolButton(); pre_colorRefresh->setIcon(QIcon(":/mMainWidgets/Icon/shuaxin3x.png"));
        connect(pre_colorRefresh, &QToolButton::clicked, [=]() {
            curColor0 = QVector3D(225, 238, 250); pre_colorBtn->setText(MxFunctions::mNumberToString(curColor0.x()) + "," + MxFunctions::mNumberToString(curColor0.y()) + "," + MxFunctions::mNumberToString(curColor0.z()));
            });
        pre_page2v->addWidget(pre_color, 2, 0);
        pre_page2v->addWidget(pre_colorBtn, 2, 1);
        pre_page2v->addWidget(pre_colorRefresh, 2, 2);

        QLabel* pre_upLeft = new QLabel("上/左：");
        QPushButton* pre_upLeftBtn = new QPushButton();
        QToolButton* pre_upLeftRefresh = new QToolButton(); pre_upLeftRefresh->setIcon(QIcon(":/mMainWidgets/Icon/shuaxin3x.png"));
        connect(pre_upLeftRefresh, &QToolButton::clicked, [=]() {
            curColor1 = QVector3D(150, 199, 237); pre_upLeftBtn->setText(MxFunctions::mNumberToString(curColor1.x()) + "," + MxFunctions::mNumberToString(curColor1.y()) + "," + MxFunctions::mNumberToString(curColor1.z()));
            });
        pre_page2v->addWidget(pre_upLeft, 3, 0);
        pre_page2v->addWidget(pre_upLeftBtn, 3, 1);
        pre_page2v->addWidget(pre_upLeftRefresh, 3, 2);

        QLabel* pre_downRight = new QLabel("下/右：");
        QPushButton* pre_downRightBtn = new QPushButton();
        QToolButton* pre_downRightRefresh = new QToolButton(); pre_downRightRefresh->setIcon(QIcon(":/mMainWidgets/Icon/shuaxin3x.png"));
        connect(pre_downRightRefresh, &QToolButton::clicked, [=]() {
            curColor2 = QVector3D(225, 238, 250); pre_downRightBtn->setText(MxFunctions::mNumberToString(curColor2.x()) + "," + MxFunctions::mNumberToString(curColor2.y()) + "," + MxFunctions::mNumberToString(curColor2.z()));
            });
        pre_page2v->addWidget(pre_downRight, 4, 0);
        pre_page2v->addWidget(pre_downRightBtn, 4, 1);
        pre_page2v->addWidget(pre_downRightRefresh, 4, 2);
        pre_page2v->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding),
            5, 0, 1, -1);

        pre_page2v->setHorizontalSpacing(6);
        pre_page2v->setVerticalSpacing(6);

        // 2) 三列的伸缩策略：label列固定、内容列吃空间、图标列固定
        pre_page2v->setColumnStretch(0, 0);
        pre_page2v->setColumnStretch(1, 1);
        pre_page2v->setColumnStretch(2, 0);

        pre_page2->setLayout(pre_page2v);

        //第三页：外观（尺寸档位 + 主题）
        QWidget* pre_page3 = new QWidget();
        pre_stack->addWidget(pre_page3);
        QGridLayout* pre_page3v = new QGridLayout;
        QLabel* pre_page3label = new QLabel("尺寸与主题");
        pre_page3v->addWidget(pre_page3label, 0, 0);

        QLabel* pre_sizeLabel = new QLabel("尺寸：");
        QComboBox* pre_sizeBox = new QComboBox();
        const QStringList sizeNames = { "特小", "小号", "标准", "大号", "特大" };
        for (int i = 0; i < sizeNames.size(); ++i)
            pre_sizeBox->addItem(sizeNames.at(i), i + 1);   // data = 档位 1-5
        pre_sizeBox->setCurrentIndex(UiMetrics::level() - 1);
        pre_sizeBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        pre_page3v->addWidget(pre_sizeLabel, 1, 0);
        pre_page3v->addWidget(pre_sizeBox, 1, 1, 1, 2);

        QLabel* pre_themeLabel = new QLabel("主题：");
        QComboBox* pre_themeBox = new QComboBox();
        const QStringList themeNames = { "浅色", "深色", "护眼" };
        for (int i = 0; i < themeNames.size(); ++i)
            pre_themeBox->addItem(themeNames.at(i), i);
        pre_themeBox->setCurrentIndex(static_cast<int>(UiPalette::theme()));
        pre_themeBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        pre_page3v->addWidget(pre_themeLabel, 2, 0);
        pre_page3v->addWidget(pre_themeBox, 2, 1, 1, 2);

        pre_page3v->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding),
            3, 0, 1, -1);
        pre_page3v->setHorizontalSpacing(6);
        pre_page3v->setVerticalSpacing(6);
        pre_page3v->setColumnStretch(0, 0);
        pre_page3v->setColumnStretch(1, 1);
        pre_page3v->setColumnStretch(2, 0);
        pre_page3->setLayout(pre_page3v);

        QPushButton* btn_sure = new QPushButton("确定");
        QPushButton* btn_apply = new QPushButton("应用");
        QPushButton* btn_default = new QPushButton("置为默认");
        QHBoxLayout* pre_buttonBox = new QHBoxLayout;
        pre_buttonBox->addWidget(btn_sure);
        pre_buttonBox->addWidget(btn_apply);
        pre_buttonBox->addWidget(btn_default);

        pre_h1->addWidget(pre_label1);
        pre_h1->addWidget(pre_lineedit);
        pre_h3->addWidget(pre_tree);
        pre_h2->addWidget(pre_btn);
        pre_h2->addWidget(pre_btn1);
        pre_h2->addWidget(pre_btn2);

        pre_v1->addLayout(pre_h1);
        pre_v1->addLayout(pre_h3);
        pre_v1->addLayout(pre_h2);

        pre_v2->addWidget(pre_stack);
        pre_v2->addStretch();
        pre_v2->addLayout(pre_buttonBox);

        QSplitter* pre_split = new QSplitter(Qt::Horizontal);
        pre_split->addWidget(pre_vBox1);
        pre_split->addWidget(pre_vBox2);
        pre_h->addWidget(pre_split);   // 把 splitter 放进主窗
        pre_h->addLayout(pre_v1);
        pre_h->addLayout(pre_v2);
        framelessDlg->setContentWidget(pre_content);
        pre_num->hide(); pre_day->show();
        pre_numEdit->hide(); pre_timeEdit->show();
        pre_numPiece->hide(); pre_time->show();

        // 首选项各控件按当前主题色板上色（背景/文字/悬浮/选中），切主题时
        // 由 ThemeManager::applyAll 遍历顶层 QDialog 重发，这里负责首次创建。
        // 注意：内容部件已装入 mFramelessDialog，主题要打在内容根上（对话框
        // 自身的标题栏/边框由 mWindowTitleBar/FramelessWindowHelper 接管）
        ThemeManager::instance()->applyDialogTheme(pre_content);
        // 标题栏与窗口边框按当前主题（自绘，非 DWM）
        ThemeManager::applyTitleBarTheme(framelessDlg);

        //一键关闭所有按钮的默认回车行为
        QList<QPushButton*> allButtons = preferencesDialog->findChildren<QPushButton*>();
        for (QPushButton* btn : allButtons)
        {
            btn->setAutoDefault(false);
            btn->setDefault(false);
        }

        connect(pre_lineedit, &QLineEdit::textChanged, [=]() {this->onFilterTextChanged(pre_lineedit->text(), pre_tree); });
        connect(pre_waybox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index)
            {
                if (index == 0)
                {
                    pre_num->hide(); pre_day->show();
                    pre_numEdit->hide(); pre_timeEdit->show();
                    pre_numPiece->hide(); pre_time->show();
                }
                else
                {
                    pre_num->show(); pre_day->hide();
                    pre_numEdit->show(); pre_timeEdit->hide();
                    pre_numPiece->show(); pre_time->hide();
                }
            });
        connect(pre_tree, &QTreeWidget::itemClicked,
            this, [=](QTreeWidgetItem* item, int col) {
                qDebug() << "单击了" << item->text(col) << "第" << col << "列";
                if (item->text(col) == "最近打开") pre_stack->setCurrentIndex(0);
                if (item->text(col) == "视口")
                {
                    auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
                    QSettings Settings(path, QSettings::IniFormat);
                    Settings.beginGroup("color");
                    auto map = Settings.childKeys();
                    QStringList keys;
                    QStringList values;
                    for (const QString& k : Settings.childKeys()) {
                        keys.push_back(k); // 时间戳升序 → 倒序
                        values.push_back(Settings.value(k).toString());
                        if (k == "PureColor")
                            pre_colorBtn->setText(Settings.value(k).toString());
                        else if (k == "GradientT_L")
                            pre_upLeftBtn->setText(Settings.value(k).toString());
                        else if (k == "GradientB_R")
                            pre_downRightBtn->setText(Settings.value(k).toString());
                    }

                    Settings.endGroup();
                    pre_stack->setCurrentIndex(1);

                }
                if (item->text(col) == "尺寸与主题")
                {
                    // 打开时回显当前生效的档位/主题，避免显示未应用的旧选择
                    pre_sizeBox->setCurrentIndex(UiMetrics::level() - 1);
                    pre_themeBox->setCurrentIndex(static_cast<int>(UiPalette::theme()));
                    pre_stack->setCurrentIndex(2);
                }
            });
        connect(pre_btn1, &QPushButton::clicked, [=]() {pre_tree->expandAll(); });
        connect(pre_btn2, &QPushButton::clicked, [=]() {pre_tree->collapseAll(); });

        pre_downRight->hide(); pre_downRightBtn->hide(); pre_downRightRefresh->hide();
        pre_upLeft->hide(); pre_upLeftBtn->hide(); pre_upLeftRefresh->hide();
        connect(pre_modebox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index) {
            if (pre_modebox->currentIndex() == 0)//单一颜色
            {
                pre_color->show(); pre_colorBtn->show(); pre_colorRefresh->show();
                pre_downRight->hide(); pre_downRightBtn->hide(); pre_downRightRefresh->hide();
                pre_upLeft->hide(); pre_upLeftBtn->hide(); pre_upLeftRefresh->hide();
            }
            else
            {
                pre_color->hide(); pre_colorBtn->hide(); pre_colorRefresh->hide();
                pre_downRight->show(); pre_downRightBtn->show(); pre_downRightRefresh->show();
                pre_upLeft->show(); pre_upLeftBtn->show(); pre_upLeftRefresh->show();
            }
            });
        connect(pre_colorBtn, &QPushButton::clicked, [=]() {
            QColor _color = QColorDialog::getColor(QColor(curColor0.x(), curColor0.y(), curColor0.z()), nullptr, "获取颜色");
            if (_color.isValid())
            {
                pre_colorBtn->setText(MxFunctions::mNumberToString(_color.red()) + "," + MxFunctions::mNumberToString(_color.green()) + "," + MxFunctions::mNumberToString(_color.blue()));
                curColor0 = QVector3D(_color.red(), _color.green(), _color.blue());
            }
            });
        connect(pre_upLeftBtn, &QPushButton::clicked, [=]() {
            QColor _color = QColorDialog::getColor(QColor(curColor1.x(), curColor1.y(), curColor1.z()), nullptr, "获取颜色");
            if (_color.isValid())
            {
                pre_upLeftBtn->setText(MxFunctions::mNumberToString(_color.red()) + "," + MxFunctions::mNumberToString(_color.green()) + "," + MxFunctions::mNumberToString(_color.blue()));
                curColor1 = QVector3D(_color.red(), _color.green(), _color.blue());
            }
            });
        connect(pre_downRightBtn, &QPushButton::clicked, [=]() {
            QColor _color = QColorDialog::getColor(QColor(curColor2.x(), curColor2.y(), curColor2.z()), nullptr, "获取颜色");
            if (_color.isValid())
            {
                pre_downRightBtn->setText(MxFunctions::mNumberToString(_color.red()) + "," + MxFunctions::mNumberToString(_color.green()) + "," + MxFunctions::mNumberToString(_color.blue()));
                curColor2 = QVector3D(_color.red(), _color.green(), _color.blue());
            }
            });
        connect(btn_sure, &QPushButton::clicked, [=]() {
            btn_apply->click();
            preferencesDialog->close();
            });
        connect(btn_apply, &QPushButton::clicked, [=]() {
            int page = pre_stack->currentIndex();
            if (page == 0)//最近打开
            {
                if (pre_waybox->currentIndex() == 0)
                {
                    recentlyOpenedDialog->setWindowTitle("最近打开(根据时间) - [预览]");
                    recentDay = pre_timeEdit->text().toInt();
                    recentflag = 1;
                }
                else
                {
                    recentlyOpenedDialog->setWindowTitle("最近打开(根据数量) - [预览]");
                    recentNum = pre_numEdit->text().toInt();
                    recentflag = 0;
                }
            }
            else if (page == 1)//视口
            {
                auto colorRender = getCVM->getPreRenderWidget()->getBackGroundRender();
                int index = pre_modebox->currentIndex();
                if (index == 0)
                {
                    colorRender->setBackGroundMode(BackGroundMode::PureColor);
                    colorRender->setPureColor(curColor0);
                }
                else if (index == 1)
                {
                    colorRender->setBackGroundMode(BackGroundMode::GradientT_B);
                    colorRender->setGradientColor(curColor1, curColor2);
                }
                else if (index == 2)
                {
                    colorRender->setBackGroundMode(BackGroundMode::GradientL_R);
                    colorRender->setGradientColor(curColor1, curColor2);
                }

                //加入最近打开
                auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
                QSettings Settings(path, QSettings::IniFormat);
                // 添加一条“最近打开”
                QString value1 = MxFunctions::mNumberToString(curColor0.x()) + "," + MxFunctions::mNumberToString(curColor0.y()) + "," + MxFunctions::mNumberToString(curColor0.z());
                QString key1 = QString("color/%1").arg("PureColor");
                Settings.setValue(key1, value1);
                QString value2 = MxFunctions::mNumberToString(curColor1.x()) + "," + MxFunctions::mNumberToString(curColor1.y()) + "," + MxFunctions::mNumberToString(curColor1.z());
                QString key2 = QString("color/%1").arg("GradientT_L");
                Settings.setValue(key2, value2);
                QString value3 = MxFunctions::mNumberToString(curColor2.x()) + "," + MxFunctions::mNumberToString(curColor2.y()) + "," + MxFunctions::mNumberToString(curColor2.z());
                QString key3 = QString("color/%1").arg("GradientB_R");
                Settings.setValue(key3, value3);
                getCVM->getPreRenderWidget()->activateWindow();
                preferencesDialog->activateWindow();
            }
            else if (page == 2)//尺寸与主题
            {
                // 档位+主题合并为一轮全量下发（分别 applyLevel/applyTheme 会重发两遍）
                ThemeManager::instance()->applyAppearance(
                    pre_sizeBox->currentData().toInt(), pre_themeBox->currentData().toInt(), this);
            }
            });
        //置为默认
        connect(btn_default, &QPushButton::clicked, [=]() {
            int page = pre_stack->currentIndex();
            if (page == 0)//最近打开
            {
                pre_waybox->setCurrentIndex(0);
                pre_timeEdit->setText("30");
            }
            else if (page == 1)
            {
                pre_modebox->setCurrentIndex(0);
                pre_colorRefresh->click();
            }
            else if (page == 2)//尺寸与主题：标准档 + 浅色主题
            {
                pre_sizeBox->setCurrentIndex(2);
                pre_themeBox->setCurrentIndex(0);
            }
            });
        //全部置为默认
        connect(pre_btn, &QPushButton::clicked, [=]() {
            //pre_waybox->setCurrentIndex(0);
            pre_timeEdit->setText("30");
            pre_numEdit->setText("10");
            //pre_modebox->setCurrentIndex(0);
            pre_colorRefresh->click();
            pre_downRightRefresh->click();
            pre_upLeftRefresh->click();
            //尺寸与主题：标准档 + 浅色主题（与"置为默认"单页分支一致）
            pre_sizeBox->setCurrentIndex(2);
            pre_themeBox->setCurrentIndex(0);
            });

    }
    void mMenuWidget::initAbout()
    {
        aboutMechanicalDialog = new mFramelessDialog(this);
        //多体形态：关于弹窗为多体品牌（产品名/版本为占位，定稿后替换）
        const bool aboutMotionForm = (MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion);
        aboutMechanicalDialog->setWindowTitle(aboutMotionForm ? QString::fromUtf8("关于MxSim.Motion") : QString::fromUtf8("关于MxSim.Mechanical"));
        aboutMechanicalDialog->setFixedSize(600, 507);
        QVBoxLayout* about_vLayout = new QVBoxLayout;
        about_vLayout->setContentsMargins(0, 0, 0, 0);
        //about_vLayout->setSpacing(4);
        QLabel* aboutPicLabel = new QLabel();
        if (aboutMotionForm)
        {
            //多体品牌横幅（600x179，由启动图内容密度最高横带离线裁切生成，见 scripts/工具）
            QPixmap motionAboutPic(":/mMainWidgets/Icon/关于_Motion.png");
            aboutPicLabel->setPixmap(motionAboutPic);
        }
        else
        {
            QPixmap aboutPic(":/mMainWidgets/Icon/关于.png");
            aboutPicLabel->setPixmap(aboutPic);
        }
        //aboutPicLabel->setAlignment(Qt::AlignCenter);
        aboutPicLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QWidget* contentwidget = new QWidget();
        QVBoxLayout* contentLayout = new QVBoxLayout(contentwidget);
        contentLayout->setContentsMargins(8, 8, 8, 8);
        contentwidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        //contentLayout->setAlignment(Qt::AlignCenter);
        //contentLayout->setSpacing(2);//控件间距

        QList<QPair<QString, bool>> aboutTexts;
        if (aboutMotionForm)
        {
            aboutTexts = {
            {QString::fromUtf8("产品名称:MxSim.Motion多体动力学仿真软件\n版本号:V1.0\n发布时间：2026年01月01日\n版权所有：湖南迈曦软件有限责任公司\n公司官网：https://mxcae.com/"),
             false},
            {QString::fromUtf8("　　迈曦软件成立于2020年8月10日，是由湖南大学通过科技成果转化的一家致力于工业CAE软件开发和技术服务的国家高新技术企业、国家科技型中小企业、湖南省专精特新中小企业。公司获得中车、三一、深创投等行业龙头企业战略投资。公司推出了多款面向复杂产品分析和设计的CAE工业软件MxSim、MxDesign、MxCloud、MxEdu，形成了“仿真+设计+云服务+教育”的多层次产品体系，为我国民用和军用复杂装备产品设计提供工程计算软件支撑。"),
             true} };
        }
        else
        {
            aboutTexts = {
            {"产品名称:MxSim.Mechanical通用结构仿真软件\n版本号:V3.0.1\n发布时间：2026年01月01日\n版权所有：湖南迈曦软件有限责任公司\n公司官网：https://mxcae.com/",false},
            { "\u3000\u3000迈曦软件成立于2020年8月10日，是由湖南大学通过科技成果转化的一家致力于工业CAE软件开发和技术服务的国家高新技术企业、国家科技型中小企业、湖南省专精特新中小企业。公司获得中车、三一、深创投等行业龙头企业战略投资。公司推出了多款面向复杂产品分析和设计的CAE工业软件MxSim、MxDesign、MxCloud、MxEdu，形成了“仿真+设计+云服务+教育”的多层次产品体系，为我国民用和军用复杂装备产品设计提供工程计算软件支撑。\n\u3000\u3000MxSim.Mechanical通用结构仿真软件面向机械装备、消费电子、汽车工业、航天航空等行业复杂的结构仿真需求，提供从线性到非线性、从静态到动态、从仿真到优化的全方位解决方案。软件以高性能单元理论算法为核心，具备全面的材料本构模型、丰富的单元类型与连接方式、多类载荷边界条件、多种目标的结构优化功能。能够有效应对多部件、多连接结构在静力、动力和非线性等多种工况下的仿真分析与结构优化挑战。",true }, };
        }
        QFont aboutFont(UiMetrics::uiFontFamily(), 12);
        QList<QLabel*> aboutcontentLabels;
        for (const QPair<QString, bool>& pair : aboutTexts) {
            QLabel* label = new QLabel(pair.first);
            label->setFont(aboutFont);
            label->setWordWrap(true);
            label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            label->setStyleSheet("QLabel {line-height:1.5em;margin:0;padding:0;}");

            if (!pair.second)
            {
                label->setAlignment(Qt::AlignCenter);
                label->setStyleSheet("QLabel { text-align:center;}");
            }
            aboutcontentLabels.append(label);
            contentLayout->addWidget(label);
        }


        QToolButton* thanksInfoBtn = new QToolButton;
        thanksInfoBtn->setText("致谢信息 ▼");
        thanksInfoBtn->setFixedSize(85, 30);

        //多体形态不建 QWebEngineView：致谢 PDF 是 Mechanical 声明文件（多体无此内容），
        //且 WebEngine 子窗口与零非客户区无边框窗口叠加会触发 DWM 黑框压住标题栏
        const bool aboutMotionFormWeb = (MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion);
        if (!aboutMotionFormWeb && mCaseWindow::checkQtWebEngineProcessExists())
        {
            viewAbout = new QWebEngineView;
            QWebEngineSettings* settingsAbout = viewAbout->page()->settings();
            settingsAbout->setAttribute(QWebEngineSettings::PluginsEnabled, true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
            settingsAbout->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
#endif
            QString fileAbout = QStringLiteral("MxSim.Mechanical前后处理引用开源代码及开源库声明.pdf");
            QString pdfAbout = QApplication::applicationDirPath() + "/../About";
            QDir().mkpath(pdfAbout);   // 返回 true 表示创建成功或已存在
            QString pdfDirAbout = QApplication::applicationDirPath() + "/../About/MxSim.Mechanical前后处理引用开源代码及开源库声明.pdf";
            viewAbout->hide();

            viewAbout->load(QUrl(pdfDirAbout));
            //致谢信息
            connect(thanksInfoBtn, &QToolButton::clicked, this, [=]() {
                bool isVisible = viewAbout->isVisible();
                if (isVisible) {
                    viewAbout->hide();
                    aboutMechanicalDialog->setFixedSize(600, 507);
                    thanksInfoBtn->setText("致谢信息 ▼");
                }
                else
                {
                    viewAbout->show();
                    aboutMechanicalDialog->setFixedSize(600, 750);
                    thanksInfoBtn->setText("致谢信息 ▲");
                }
                });
        }
        else
        {
            //多体形态有意跳过 / 部署缺 QtWebEngineProcess：致谢 PDF 视图不建
            //（后续 if(viewAbout) 守卫），启动期静默降级不报错
        }

        QHBoxLayout* aboutBtnLayout = new QHBoxLayout();
        aboutBtnLayout->setContentsMargins(0, 0, 10, 10);
        aboutBtnLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
        aboutBtnLayout->addWidget(thanksInfoBtn);
        thanksInfoBtn->setVisible(!aboutMotionFormWeb);
        about_vLayout->addWidget(aboutPicLabel);
        about_vLayout->addWidget(contentwidget);
        about_vLayout->addLayout(aboutBtnLayout);
        if (viewAbout)
        {
            about_vLayout->addWidget(viewAbout, 1);
        }
        about_vLayout->setAlignment(aboutBtnLayout, Qt::AlignBottom);
        aboutMechanicalDialog->setExternalLayout(about_vLayout);
    }
    void mMenuWidget::initCaseLibrary()
    {
        //缺 QtWebEngineProcess（部署未带 WebEngine）：案例简介 PDF 视图不可用，
        //整个案例库弹窗不建（文件菜单入口同步隐藏），启动期静默降级不报错
        if (!mCaseWindow::checkQtWebEngineProcessExists())
            return;

        caseLibraryDialog = new mFramelessDialog(this);
        caseLibraryDialog->setWindowTitle("案例库 - [预览]");
        caseLibraryDialog->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
        QHBoxLayout* case_h = new QHBoxLayout;
        QHBoxLayout* case_h1 = new QHBoxLayout;
        QHBoxLayout* case_h2 = new QHBoxLayout;
        QHBoxLayout* case_h3 = new QHBoxLayout();

        QWidget* case_vBox1 = new QWidget;
        QWidget* case_vBox2 = new QWidget;
        QVBoxLayout* case_v1 = new QVBoxLayout(case_vBox1);
        QVBoxLayout* case_v2 = new QVBoxLayout(case_vBox2);

        QComboBox* case_box = new QComboBox; case_box->addItems({ "工程案例", "基础案例" });
        QLabel* case_label1 = new QLabel("过滤");
        QLineEdit* case_lineedit = new QLineEdit();
        QStackedWidget* case_stacktree = new QStackedWidget;
        case_tree = new QTreeWidget;
        QTreeWidget* case_tree1 = new QTreeWidget;
        case_stacktree->addWidget(case_tree);
        case_stacktree->addWidget(case_tree1);

        QString folder = QCoreApplication::applicationDirPath() + "/../Examples/工程案例";          // 要扫描的根目录
        QDir().mkpath(folder);   // 返回 true 表示创建成功
        QDir dir1(folder);
        /* 只要子目录，不要 . 和 .. */
        QStringList subDirs = dir1.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);   // 可按需排序
        for (int i = 0; i < subDirs.size(); i++)
        {
            QTreeWidgetItem* case_root = new QTreeWidgetItem(case_tree);
            case_root->setText(0, subDirs[i]);
            QString subfolder = subDirs[i];
            QDir dir1(folder + "/" + subfolder);
            /* 只要子目录，不要 . 和 .. */
            QStringList subDirs = dir1.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);   // 可按需排序
            for (auto name : subDirs)
            {
                QTreeWidgetItem* case_recent = new QTreeWidgetItem(case_root);
                case_recent->setText(0, name);
            }
        }
        case_tree->setHeaderHidden(true);
        case_tree1->setHeaderHidden(true);
        QTreeWidgetItem* case_root1 = new QTreeWidgetItem(case_tree1);
        case_root1->setText(0, "静力学");
        QTreeWidgetItem* case_child = new QTreeWidgetItem(case_root1);
        case_child->setText(0, "车门抗凹分析");

        QPushButton* case_open = new QPushButton("打开");
        QPushButton* case_check = new QPushButton("查看案例简介");
        QLabel* stack_label = new QLabel("案例简介");

        QWidget* image = new QWidget;
        QWidget* txt = new QWidget;


        QHBoxLayout* stackLayout = new QHBoxLayout;
        stackLayout->addWidget(stack_label);
        stackLayout->addStretch();

        if (mCaseWindow::checkQtWebEngineProcessExists())
        {

            view = new QWebEngineView(caseLibraryDialog);
            QWebEngineSettings* settings = view->page()->settings();
            settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
            settings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
#endif
            view->resize(800, 600);
            view->setWindowState(Qt::WindowMaximized);

            QString file = QStringLiteral("案例操作手册.pdf");
            QString pdf = QApplication::applicationDirPath() + "/../案例操作手册.pdf";
            QString pdfDir = QCoreApplication::applicationDirPath() + "/..";
            QDir().mkpath(pdfDir);                 // 确保目录存在
            //view->show();
            view->load(QUrl(pdf));
        }

        QWidget* secondPage = new QWidget;
        QLabel* case_jieshao = new QLabel("案例介绍");
        QLabel* case_jietu = new QLabel("案例截图");
        QLabel* case_jietuwidget = new QLabel();
        case_jietuwidget->setScaledContents(true);   // 自动缩放
        case_jietuwidget->setFixedSize(640, 480);
        //case_jietuwidget->show();
        mChineseTextEdit* case_jieshaowidget = new mChineseTextEdit();
        QVBoxLayout* case_jianjie = new QVBoxLayout;
        case_jianjie->addWidget(case_jietu);
        case_jianjie->addWidget(case_jietuwidget);
        case_jianjie->addWidget(case_jieshao);
        case_jianjie->addWidget(case_jieshaowidget);
        secondPage->setLayout(case_jianjie);

        QStackedWidget* case_stack = new QStackedWidget;
        if (view)
            case_stack->addWidget(view);
        case_stack->addWidget(secondPage);
        case_stack->setCurrentIndex(1);

        case_h1->addWidget(case_label1);
        case_h1->addWidget(case_lineedit);
        case_h3->addWidget(case_stacktree);
        case_h2->addWidget(case_open);
        case_h2->addWidget(case_check);
        case_v1->addWidget(case_box);
        case_v1->addLayout(case_h1);
        case_v1->addLayout(case_h3);
        case_v1->addLayout(case_h2);
        case_v2->addLayout(stackLayout);
        case_v2->addWidget(case_stack);


        QSplitter* case_split = new QSplitter(Qt::Horizontal);
        case_split->addWidget(case_vBox1);
        case_split->addWidget(case_vBox2);
        case_h->addWidget(case_split);   // 把 splitter 放进主窗
        case_h->addLayout(case_v1);
        case_h->addLayout(case_v2);
        caseLibraryDialog->setExternalLayout(case_h);
        //动态过滤
        connect(case_lineedit, &QLineEdit::textChanged, [=]() {this->onFilterTextChanged(case_lineedit->text(), case_tree); });
        connect(case_box, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=]() {
            if (case_box->currentIndex() == 0)
                case_stacktree->setCurrentIndex(0);
            else
                case_stacktree->setCurrentIndex(1);
            });
        connect(case_tree, &QTreeWidget::itemClicked, [=]()
            {
                mDockLayoutManager::getInstance()->activateDock("Case");
                //案例简介
                QTreeWidgetItem* item = case_tree->currentItem();
                if (!item->parent())
                {
                    return;
                }
                QTreeWidgetItem* parent = item->parent();     // 父节点
                QString parentName = item->parent()->text(0);
                QString name = item->text(0);
                QString Path = folder + "/" + parentName + "/" + name;
                // 1. 读文件
                QFile file(Path + "/案例介绍.txt");
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
                QPixmap pix(Path + "/截图.png");
                if (pix.isNull()) return;                 // 文件不存在/格式错误
                // 3. 放进 QLabel
                case_jietuwidget->setPixmap(pix);
                case_jietuwidget->setScaledContents(true);       // 自动缩放填满 label
                QTextStream in(&file);
                QString content = in.readAll();
                file.close();
                // 2. 创建显示控件
                case_jieshaowidget->setPlainText(content);
                case_jieshaowidget->setReadOnly(true);        // 只读
                case_jieshaowidget->setWordWrapMode(QTextOption::WrapAnywhere);
                //案例文档
                QString _file = ("/工程案例/" + parentName + "/" + name + "/案例操作手册.pdf");
                QString _pdf = QUrl(QApplication::applicationDirPath() + "/../Examples" + _file, QUrl::StrictMode).toString(QUrl::FullyEncoded);
                if (view)
                    view->load(QUrl(_pdf));

                auto caseView = mCaseWindow::getInstance()->getView();
                if (caseView)
                {
                    caseView->load(QUrl(_pdf));
                    mCaseWindow::getInstance()->setFixedWidth(600);
                }
            });
        connect(case_open, &QPushButton::clicked, [=]() {
            QTreeWidgetItem* item = case_tree->currentItem();
            if (item == nullptr || !item->parent())
            {
                return;
            }
            QTreeWidgetItem* parent = item->parent();     // 父节点
            QString parentName = item->parent()->text(0);
            QString name = item->text(0);
            QString Path = folder + "/" + parentName + "/" + name;
            QDir dir(Path);
            if (!dir.exists()) return;                              // 目录不存在
            // 过滤 *.mx
            QStringList mx = dir.entryList({ "*.mx" }, QDir::Files, QDir::Name);
            if (mx.isEmpty())
                return;
            QString mxpath = dir.absoluteFilePath(mx.first());
            // 1. 导入计算模型
            if (!mxpath.isEmpty()) {
                QFileInfo fileinfo(mxpath);
                QString suffix = fileinfo.suffix();
                if (suffix == "mx")
                {
                    MechIOFile::mIOFileManager::getInstance()->slot_importSolveFile(mxpath);
                }
                else
                {
                    m_transFileInfoDialog->transMxFile(mxpath);
                }
            }
            });
        connect(case_check, &QPushButton::clicked, [=]() {
            if (stack_label->text() == "案例文档")
            {
                stack_label->setText("案例简介");
                case_check->setText("查看案例简介");
                case_stack->setCurrentIndex(1);

            }
            else if (stack_label->text() == "案例简介")
            {
                stack_label->setText("案例文档");
                case_check->setText("查看案例文档");
                case_stack->setCurrentIndex(0);
            }

            });
    }
    void mMenuWidget::initLicense()
    {
        licenseDialog = new mFramelessDialog(this);
        licenseDialog->setWindowTitle("许可证 - [预览]");
        QHBoxLayout* lic_h = new QHBoxLayout;
        QHBoxLayout* lic_h1 = new QHBoxLayout;
        QHBoxLayout* lic_h2 = new QHBoxLayout;
        QHBoxLayout* lic_h3 = new QHBoxLayout;
        QHBoxLayout* lic_h4 = new QHBoxLayout;
        QVBoxLayout* lic_v1 = new QVBoxLayout();

        QLabel* lic_label1 = new QLabel("许可剩余天数[天]");
        TimerWidget* w = new TimerWidget;
        //w->show();
        lic_h4->addStretch();
        lic_h4->addWidget(lic_label1);
        lic_h4->addStretch();
        lic_h3->addWidget(w);


        QLabel* lic_type = new QLabel("许可证类型：单机许可");
        QLabel* lic_reactive = new QLabel("重新激活");
        QLineEdit* lic_reactiveEdit = new QLineEdit();
        lic_reactiveEdit->setPlaceholderText("导入许可证文件");
        QToolButton* lic_reactiveBtn = new QToolButton();
        lic_reactiveBtn->setIcon(QIcon(":/mMainWidgets/Icon/homepage/dakai.png"));
        QPushButton* lic_btn = new QPushButton("确定");
        QPushButton* lic_btn1 = new QPushButton("应用");


        lic_h1->addWidget(lic_reactive);
        lic_h1->addWidget(lic_reactiveEdit);
        lic_h1->addWidget(lic_reactiveBtn);

        lic_h2->addStretch();
        lic_h2->addWidget(lic_btn);
        lic_h2->addWidget(lic_btn1);

        lic_v1->addLayout(lic_h4);
        lic_v1->addLayout(lic_h3);
        lic_v1->addWidget(lic_type);
        lic_v1->addLayout(lic_h1);
        lic_v1->addLayout(lic_h2);


        lic_h->addLayout(lic_v1);
        licenseDialog->setExternalLayout(lic_h);
        licenseDialog->setFixedWidth(290);
        //导入许可证
        connect(lic_reactiveBtn, &QToolButton::clicked, [=]() {
            QString filePath = QFileDialog::getOpenFileName(
                this,                                    // 父窗口
                "打开文件",                               // 标题
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), // 起始目录
                "所有文件 (*);;文本文件 (*.txt)" // 过滤器
            );
            if (filePath.isEmpty())
                return;
            QDir dstDir("D:/RebuildMech_2/Software/Common/RebuildMech/bin/License");
            if (!dstDir.exists()) dstDir.mkpath(".");  // 自动建目录

            QString dst = dstDir.absoluteFilePath(QFileInfo(filePath).fileName());
            if (QFile::exists(dst)) QFile::remove(dst);   // 存在就删除
            if (QFile::copy(filePath, dst))
            {
                lic_reactiveEdit->setText(dst);
            }
            });
        //应用
        connect(lic_btn1, &QPushButton::clicked, [=]()
            {
                auto a = lic_reactiveEdit->text();
                if (lic_reactiveEdit->text() != "D:/RebuildMech_2/Software/Common/RebuildMech/bin/License/MxLicense.lic")
                {
                    MechMainWindow::mFramelessMessageBox::warning(this, "失败", "lic文件路径错误");
                    return;
                }
                Common::mLicense* license = new Common::mLicense(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir);
                //0正确  1配置文件失败  2文件不存在 3不正确 4许可证过期
                //state代表许可证的状态，自己开发时刻设为0 
                int state = license->checkLicense();
                int Day = license->getRemainDay();
                if (Day == -1)
                    Day = 88;
                w->setTimer(Day);
            });
        //确定
        connect(lic_btn, &QPushButton::clicked, [=]()
            {
                auto a = lic_reactiveEdit->text();
                if (lic_reactiveEdit->text() != "D:/RebuildMech_2/Software/Common/RebuildMech/bin/License/MxLicense.lic")
                {
                    MechMainWindow::mFramelessMessageBox::warning(this, "失败", "lic文件路径错误");
                    return;
                }
                Common::mLicense* license = new Common::mLicense(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir);
                //0正确  1配置文件失败  2文件不存在 3不正确 4许可证过期
                //state代表许可证的状态，自己开发时刻设为0 
                int state = license->checkLicense();
                int Day = license->getRemainDay();
                if (Day == -1)
                    Day = 0;
                w->setTimer(Day);
                licenseDialog->close();
            });

    }
    int countRecentTimestamps(const QStringList& timestamps, int days, bool isMillisecond = true) {
        if (timestamps.isEmpty()) return 0;

        // 获取当前时间戳（毫秒）
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        qint64 threshold;

        if (isMillisecond) {
            threshold = now - days * 24 * 3600 * 1000LL;
        }
        else {
            // 秒级
            qint64 nowSec = now / 1000;
            threshold = nowSec - days * 24 * 3600;
            // 但为了统一比较，如果输入是秒，我们也可以先转成毫秒再算，或者保持秒比较
            // 这里假设 timestamps 是秒，threshold 也是秒
            // 我们这里按毫秒写，如果是秒，需要调整
            // 为了通用，我们直接在比较时处理单位差异
        }

        // 由于列表是升序，我们找到第一个 >= threshold 的位置
        int low = 0, high = timestamps.size() - 1;
        int firstIndex = timestamps.size(); // 默认没有找到则全不满足

        while (low <= high) {
            int mid = low + (high - low) / 2;
            qint64 ts = timestamps[mid].toLongLong();
            // 如果单位是秒，而threshold是毫秒，需要调整，这里假设单位一致
            if (ts >= threshold) {
                firstIndex = mid;
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }

        return timestamps.size() - firstIndex;
    }
    void mMenuWidget::initMenuBtn()
    {

		/*
		menu  ： 居左文件 | 居中Menus | 居右 查找功能
		*/
		// 文件按钮：全自绘 mRibbonButton（文字+小图标，图标在左），彻底脱离 Qt 底座
		// 样式——底座样式的 menu-indicator 槽会把下拉箭头钉到控件右下/右上角
		//（与文字顶部对齐的观感），自绘路径箭头与文字共用基线
		mRibbonButton* fileButton_ = new mRibbonButton(mRibbonButton::Mode::Small);
		fileButton_->setObjectName("FileButton");
		fileButton_->setText("文件");
		fileButton_->setIcon(QIcon(":/mMainWidgets/Icon/file.png"));
		fileButton_->setIconSize(QSize(12, 12));
		// 入口按钮不显示下拉小三角：mxNoArrow 标记让自绘/宽度公式跳过箭头占宽
		fileButton_->setProperty("mxNoArrow", true);

		QMenu* menu = new mUiMenu(this);
		// 菜单圆角由 mUiStyle 的 PE_PanelMenu 自绘统一保证（规则 1.0 radius=4），
		// 不再单独设 FramelessWindowHint/WA_TranslucentBackground（系统窗口在圆角外会露直角）

		// 菜单一站式装配（规则 1.0）：统一样式表 + mUiStyle 自绘行内容；
		// 子菜单创建后也立即应用（ThemeManager::applyAll 在构造末尾才统一执行，
		// 但子菜单在弹出时才创建/填充，需要即时设置）
		auto applyMenuTheme = [](QMenu* m) {
			ThemeManager::applyMenuStyle(m);
		};
		applyMenuTheme(menu);
		menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/xinjian.png"), "新建", this, &mMenuWidget::slot_New);
		menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/dakai.png"), "打开", this, &mMenuWidget::slot_openFile);
		//menu->addAction("最近打开", this, [=]() {});
		subMenu1 = new mUiMenu("最近打开", menu);
		applyMenuTheme(subMenu1);
		QString dir = QDir::currentPath();
		QString name = "D:/lry/mech/first.stp";
		//读取最近 5 条（按时间倒序）
		auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
		connect(subMenu1, &QMenu::aboutToShow, [=]()
			{
				subMenu1->clear();
				QSettings Settings(path, QSettings::IniFormat);
				Settings.beginGroup("recent");
				QStringList files;
				for (const QString& k : Settings.childKeys()) {
					files.prepend(Settings.value(k).toString()); // 时间戳升序 → 倒序
				}
				QStringList files_recentNum = files.mid(0, 5);
				Settings.endGroup();
				for (auto str : files_recentNum)
				{
					QString path = str;
					QAction* act = subMenu1->addAction(path);
					connect(act, &QAction::triggered, [=]() {
						if (getCVM->isPostModel())
						{
							mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入该模型，请选择前处理模型！"));
							return;
						}
						QString fileName = path;
						//若为计算文件
						QString suffix = QFileInfo(fileName).suffix();
                        QStringList meshSuffixs{ "obj","ply","off","stl" };
						if (!fileName.isEmpty()) {
							if (suffix == "mx")
							{
								MechIOFile::mIOFileManager::getInstance()->slot_importSolveFile(fileName);
                            }
                            else if (meshSuffixs.contains(suffix)) {
                                MechIOFile::mIOFileManager::getInstance()->readMeshIOFile(fileName);
                            }
							else
							{
								auto modeldata = getCVM->getModelData();
								auto geomanager = modeldata->getGeoManager();
								// mpcomm：几何导入完成后回传 committed(geometry)
								reportMpcommGeometryOnRenderFinished(fileName);
								geomanager->slotImportGeoFiles(fileName);
							}

                            //加入最近打开
                            auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
                            QSettings Settings(path, QSettings::IniFormat);
                            // 添加一条“最近打开”
                            QString file = fileName;
                            QString key = QString("recent/%1").arg(QDateTime::currentMSecsSinceEpoch());//当前时间距离 1970-01-01 00:00:00 UTC 的毫秒数（int64）

                            Settings.beginGroup("recent");
                            for (auto k : Settings.childKeys())
                            {
                                if (Settings.value(k) == fileName)
                                    Settings.remove(QString("%1").arg(k));
                            }
                            Settings.endGroup();
                            Settings.setValue(key, file);
                            //模型快照
                            QTimer::singleShot(2000, this, [=]() {
                                auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
                                QSettings Settings(path, QSettings::IniFormat);
                                Settings.beginGroup("recent");
                                QStringList keys = Settings.childKeys();

                                QString id = keys[Settings.childKeys().size() - 1];//时间戳
                                Settings.endGroup();

                                getCVM->modelFastCapture(fileName, id);
                                });
                        }
                        });
                }
                subMenu1->addAction("查看所有最近打开的模型", this, [=]() {
                    QSettings Settings(path, QSettings::IniFormat);
                    Settings.beginGroup("recent");
                    QStringList files_all;
                    QStringList keys_all = Settings.childKeys();
                    for (const QString& k : Settings.childKeys()) {
                        files_all.prepend(Settings.value(k).toString()); // 时间戳升序 → 倒序
                    }
                    QStringList files_recentNum;
                    if (recentflag == 0)
                        files_recentNum = files_all.mid(0, recentNum);
                    else
                    {
                        int count = countRecentTimestamps(keys_all, recentDay, true); // true 表示毫秒
                        files_recentNum = files_all.mid(0, count);
                    }
                    recentTable->setRowCount(files_recentNum.size());
                    for (int r = 0; r < files_recentNum.size(); r++)
                    {
                        QString filePath = files_recentNum[r];
                        QString name = QFileInfo(filePath).fileName();
                        recentTable->setItem(r, 1, new QTableWidgetItem(filePath));
                        recentTable->setItem(r, 0, new QTableWidgetItem(name));
                        recentTable->item(r, 0)->setData(Qt::UserRole, keys_all[keys_all.size() - 1 - r]);//时间戳
                    }

                    if (recentTable->rowCount() > 0)
                    {
                        recentTable->selectRow(0);
                        emit recentTable->cellClicked(0, 0);
                    }

                    if (recentlyOpenedDialog)
                        recentlyOpenedDialog->exec();
                    });
            });


		subMenu1->setIcon(QIcon(":/mMainWidgets/Icon/homepage/zuijindakai.png"));
		menu->addMenu(subMenu1);

        //后处理结果文件入口（执行方为mPostBrowserWindow，经全局信号解耦，自slot_openPostFile/slot_openPostNVHFile迁出）
        //多体形态不提供（多体结果经求解方案的动画/运动曲线查看）
        if (MechCommon::resolveRibbonForm() != MechCommon::MenuRibbonForm::Motion)
        {
            QMenu* postMenu = new mUiMenu("后处理", menu);
            postMenu->setIcon(QIcon(":/mMainWidgets/Icon/homepage/zuijindakai.png"));
            auto* postBus = MechCommon::mMechGlobalSignals::getInstance();
            QObject::connect(postMenu->addAction("云图结果"), &QAction::triggered, postMenu, [postBus]() {
                emit postBus->clickOpenPostCloudFileSig();
                });
            QObject::connect(postMenu->addAction("曲线结果"), &QAction::triggered, postMenu, [postBus]() {
                emit postBus->clickOpenPostNVHFileSig();
                });
            menu->addMenu(postMenu);
            applyMenuTheme(postMenu);
        }

        //menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/dakai.png"), "后处理", this, &mMenuWidget::slot_openPostFile);
		menu->addSeparator();
		menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/baocun.png"), "保存", this, &mMenuWidget::slot_saveFile);
		menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/lincunwei.png"), "另存为", this, &mMenuWidget::slot_saveAsFile);
		menu->addSeparator();
		menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/dakai.png"), "打开会话(.mxses)", this, &mMenuWidget::slot_openMxsesFile);
		menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/baocun.png"), "保存会话(.mxses)", this, &mMenuWidget::slot_saveMxsesFile);
		menu->addSeparator();
        //menu->addAction("更新产品及组件", this, [=]() {});
        QMenu* importMenu = new mUiMenu("导入", menu);   // 子菜单标题
        importMenu->setIcon(QIcon(":/mMainWidgets/Icon/homepage/daoru3x.png"));
        //多体形态：网格/计算模型为隐式流程文件，隐藏（保留几何模型与运动数据）
        const bool motionFormFile = (MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion);
        importMenu->addAction("几何模型", this, &mMenuWidget::slot_importGeoFile);
        if (!motionFormFile) importMenu->addAction("网格模型", this, &mMenuWidget::slot_importMeshFile);
        if (!motionFormFile) importMenu->addAction("计算模型", this, &mMenuWidget::slot_importMxFile);
        importMenu->addAction("运动数据", this, &mMenuWidget::slot_importMotionJsonFile);
        menu->addMenu(importMenu);
        applyMenuTheme(importMenu);
        QMenu* exportMenu = new mUiMenu("导出", menu);   // 子菜单标题
        exportMenu->setIcon(QIcon(":/mMainWidgets/Icon/homepage/daochu3x.png"));
        exportMenu->addAction("几何模型", this, &mMenuWidget::slot_exportGeoFile);
        if (!motionFormFile) exportMenu->addAction("网格模型", this, &mMenuWidget::slot_exportMeshFile);
        if (!motionFormFile) exportMenu->addAction("计算模型", this, &mMenuWidget::slot_exportMxFile);
        exportMenu->addAction("运动数据", this, &mMenuWidget::slot_exportMotionJsonFile);
        menu->addMenu(exportMenu);
        applyMenuTheme(exportMenu);
        menu->addSeparator();
        menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/dakai.png"), "工作目录", this, [=]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createWorkingDir(); });
        menu->addSeparator();
        //案例库入口随弹窗可用性：缺 QtWebEngineProcess 时 initCaseLibrary 已整体跳过，
        //入口隐藏（exec 前判空兜底），不再出现"点了没反应/启动报错"
        QAction* caseLibAction = menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/anliku.png"), "案例库", this, [=]() {
            if (caseLibraryDialog) caseLibraryDialog->exec();
            });
        caseLibAction->setVisible(caseLibraryDialog != nullptr);
        menu->addSeparator();
        menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/shouxuanxiang.png"), "首选项", this, [=]() {preferencesDialog->show(); });
        menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/xukezheng.png"), "许可证", this, [=]() {licenseDialog->exec(); });
        menu->addSeparator();
        menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/bangzhuwendang.png"), "帮助文档", this, [=]() {});
        //menu->addAction("更新产品及组件", this, [=]() {});
        QMenu* updateMenu = new mUiMenu("更新产品及组件", menu);   // 子菜单标题
        updateMenu->setIcon(QIcon(":/mMainWidgets/Icon/homepage/gengxin.png"));
        updateMenu->addAction("更新产品");
        updateMenu->addAction("更新案例库");
        updateMenu->addAction("更新材料库");
        menu->addMenu(updateMenu);
        applyMenuTheme(updateMenu);
        menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/guanyu.png"),
            MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion
                ? QString::fromUtf8("关于MxSim.Motion") : QString::fromUtf8("关于MxSim.Mechanical"),
            this, [=]() {
            aboutMechanicalDialog->exec();
            });
        menu->addSeparator();
        menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/tuichu.png"), "退出", this, [=]() {QApplication::quit();  });
        // 测试代码
#ifdef _DEBUG
        menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/ceshi.png"), "测试", this, [=]() {

            });
#endif // _DEBUG

        fileButton_->setMenu(menu);
		// popupMode 仅作"带菜单"标记：菜单展开由 mRibbonButton::showMenu 统一按
		// 规则 1.0 定位，不走 Qt 内建 InstantPopup（底座样式弹位与自绘脱节）；
		// 入口不画下拉箭头，DelayedPopup 不作为箭头判定条件
		fileButton_->setPopupMode(QToolButton::DelayedPopup);

		// “视图”下拉按钮与快捷图标按钮（新建/保存/打开）
		initViewMenu();

		// 文件按钮尺寸由 ThemeManager::applyAll（layoutFileButton）统一下发；
		// 自绘按钮不挂任何 QSS（挂 QSS 会引入 QStyleSheetStyle 绕过自绘）

		// 页签栏高度/字号由 ThemeManager::applyAll 统一下发
        m_menuTabBar_ = new MyTabBar;
        m_menuTabBar_->setObjectName("MenuTabBar");
        m_menuTabBar_->setFixedHeight(UiMetrics::tabBarHeight());
        //文档模式去掉页签栏在首个页签前的内边距，使“主页”与左侧分割线的间距与其他间距一致
        m_menuTabBar_->setDocumentMode(true);
        // 页签文字随档位缩放，绘制走 mUiStyle 自绘；告诉布局页签可以扩展以按 tabSizeHint 撑开
        m_menuTabBar_->setExpanding(false);
        m_menuTabBar_->setUsesScrollButtons(true);
        // 页签栏自身的 WA_StyledBackground 不打开，避免 QSS 背景盖住自绘
        m_menuTabBar_->setAttribute(Qt::WA_StyledBackground, false);
        //页签字体（微软雅黑，像素值随档位），代码级设置避免QSS子控件字体解析差异
        QFont tabFont(UiMetrics::uiFontFamily());
        tabFont.setPixelSize(UiMetrics::tabFontSize());
        m_menuTabBar_->setFont(tabFont);

        // 布局：紧凑排列，文件/视图/快捷图标组之间以竖分割线分隔
        // 规则 1.1：快捷区与页签之间、分隔线两侧留白均取 windowInset=G+2（标准 8）；
        // 分隔线高 = icon = max(H/2+3,G+10)（标准 16，规则 1.2 页签分隔线同规格）
        const int kTopInset = UiMetrics::windowInset();
        const int kSepLineH = UiMetrics::separatorLineHeight();
        QWidget* hwidget = new QWidget;
        QHBoxLayout* hLayOut = new QHBoxLayout;
        hLayOut->setSpacing(0);
        hLayOut->setContentsMargins(kTopInset, 0, kTopInset, 0);
        auto makeSeparator = [kSepLineH]() {
            QFrame* sep = new QFrame;
            sep->setObjectName("menuBarSeparator");
            sep->setFrameShape(QFrame::VLine);
            sep->setFixedHeight(kSepLineH);
            sep->setStyleSheet(QString("color:%1;border:none;border-left:1px solid %1;").arg(UiPalette::borderHex()));
            return sep;
            };
        hLayOut->addWidget(fileButton_);
        hLayOut->addSpacing(kTopInset);
        hLayOut->addWidget(makeSeparator());
        hLayOut->addSpacing(kTopInset);
		// 视图与新建/保存/打开属于同一快捷簇，簇间距仍由顶层 windowInset 管理
		QHBoxLayout* quickLayout = new QHBoxLayout;
		quickLayout->setObjectName("quickButtonLayout");
		quickLayout->setContentsMargins(0, 0, 0, 0);
		quickLayout->setSpacing(UiMetrics::quickGap());
		quickLayout->addWidget(m_viewButton_);
		quickLayout->addWidget(m_quickNewBtn_);
		quickLayout->addWidget(m_quickSaveBtn_);
		quickLayout->addWidget(m_quickOpenBtn_);
		hLayOut->addLayout(quickLayout);
        hLayOut->addSpacing(kTopInset);
        hLayOut->addWidget(makeSeparator());
        hLayOut->addSpacing(kTopInset);

		//页签栏紧随其后左对齐，剩余空间由尾部弹簧占位
		hLayOut->addWidget(m_menuTabBar_);
		hLayOut->addStretch(0);
		hwidget->setLayout(hLayOut);
		/*         ribbon toolBar        */
		// 切换menu对应功能模块的工具栏容器
		m_toolBarStackedWid_ = new QStackedWidget();
		m_toolBarStackedWid_->setObjectName("RibbonMenuStackeWidget");
		m_toolBarStackedWid_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
		// 顶部菜单栏和工具栏容器垂直布局
		QVBoxLayout* vLayOut = new QVBoxLayout;
		// 去掉多余拉伸：菜单栏高度由 setMinimumHeight 精确控制，
		// 避免 stretch 把顶部行顶到上沿导致遮挡
		vLayOut->addWidget(hwidget);
		vLayOut->addWidget(m_toolBarStackedWid_, 1);
        vLayOut->setSpacing(0);

        vLayOut->setContentsMargins(0, 0, 0, 0);
        this->setLayout(vLayOut);

        // 菜单栏中点击不同menu项时切换堆叠ribbon窗口
        connect(m_menuTabBar_, &QTabBar::currentChanged, this, &mMenuWidget::slot_CurrentTabChanged);

		// 菜单栏固定页签（常驻菜单，可由“视图-前处理常驻菜单栏”勾选控制显隐），
		// 名单之外的菜单（草图/截面属性/后处理各页等）显示在最右侧动态页签位。
		// 页签序列按形态取名单：多体形态为 主页/几何/五个多体功能区/求解/显示/选择；
		// 隐式页仍由各 initXXXMenu 注册页面（不建页签），多体形态下被彻底隐藏
		for (const QString& name : _residentMenuNames)
		{
			this->addMenu(name);
		}
        //“显示/选择”页签组左侧分隔线 + 常驻页签区与动态页签区（草图/截面属性/后处理等）分界线
        m_menuTabBar_->setSeparatorGroup({ "显示", "选择" });
        m_menuTabBar_->setResidentTabNames(_residentMenuNames);

        initHomeMenu();
        initModelMenu();
        initDisplayMenu();
        initSelectMenu();
        initNVHAnalysisMenu();
        initMaterialMenu();
        initPropertyMenu();
        initSetMenu();
        initField();
        initCoordinateMenu();
        initGeometry();
        initGeometrySketch();
        initMeshMenu();
        initWorkingConditionMenu();
        initAnalysisCaseMenu();
        initSectionShapeMenu();
        initInteractionMenu();
        initGlobalParamMenu();
        initStructuralMenu();
        initRunnerStructuralMenu();
        initFunctionMenu();
        initMotionMenu();
        //后处理菜单页由PostWindow模块的mPostMenuContributor注册（注册反转至组合根
        //mAppInitializer，经getRibbonHost()取得本类IRibbonHost接口后调用registerMenus）

		//恢复常驻页签显隐设置（需在页面全部创建后执行，
		//移除页签可能改变当前页签，随后显式同步一次页面显示）
		loadMenuBarSettings();

		/* ===== 截面形状页签动态插入支持 ===== */
	// 注意：createBtn 是在 initPropertyMenu() 里 new 的，这里只负责连信号
		QToolButton* createBtn = findChild<QToolButton*>("createBtn"); // 保证你之前 setObjectName("createBtn")
		if (createBtn)
			connect(createBtn, &QToolButton::clicked,
				this, &mMenuWidget::slotShowSectionShape);
    }

    void mMenuWidget::initRecentFile()
    {
        subMenu1->clear();
        subMenu1->addAction("1");
    }

    void mMenuWidget::initMpcomm()
    {
        // mpcomm 平台触发：导入指定几何/计算文件
        connect(MechCommon::mMechGlobalSignals::getInstance(),
            &MechCommon::mMechGlobalSignals::mpcommImportGeometryRequested,
            this, &mMenuWidget::slot_mpcommImportGeometry);
        connect(MechCommon::mMechGlobalSignals::getInstance(),
            &MechCommon::mMechGlobalSignals::mpcommImportCalcFileRequested,
            this, &mMenuWidget::slot_mpcommImportCalcFile);
        // mpcomm 平台触发：接收到上游场数据（mpif）
        connect(MechCommon::mMechGlobalSignals::getInstance(),
            &MechCommon::mMechGlobalSignals::mpcommImportMpifRequested,
            this, &mMenuWidget::slot_mpcommImportMpif);

        // mpcomm 平台触发：EXECUTE_ACTION solve → 复用"提交计算"入口
        // （该入口全程无弹窗；求解状态由求解任务中心监控回报）
        connect(MechCommon::mMechGlobalSignals::getInstance(),
            &MechCommon::mMechGlobalSignals::mpcommStartSolveRequested,
            this, &mMenuWidget::slot_startSolver);

        // OPEN_NODE 可能先于界面构造（桥接把待导数据/分析类型挂在 qApp 属性上，
        // 信号发出时无人接收），也可能晚于构造（OPEN_NODE 经队列投递，只有事件
        // 循环启动后才处理，必然在界面构造之后）——两种时序都要覆盖：
        // 构造时主动取一次 + 桥接每次挂属性后发通知信号再取一次。
        consumePendingMpcommContext();
        connect(MechCommon::mMechGlobalSignals::getInstance(),
            &MechCommon::mMechGlobalSignals::mpcommPendingContextChanged,
            this, &mMenuWidget::consumePendingMpcommContext);

        // OPEN_NODE 带入的后处理结果：等几何/计算文件导入完成后导入。
        // （结果导入依赖工况已建好；挂在导入完成信号上，两者谁先完成都可能触发，
        //  由槽内判空与延时兜底。结果经全局信号交 mPostBrowserWindow 打开：
        //  建 post 模型树/视口+读文件+登记跳转一条龙，MainWindow 不直连 PostWindow）
        {
            auto tryImportPendingResults = [this]() {
                const QStringList resultPaths =
                    qApp->property("mpcomm_pending_results").toStringList();
                if (resultPaths.isEmpty())
                    return;
                if (getCVM == nullptr || getCVM->getModelData() == nullptr)
                    return;
                vector<int> caseIDs = getCVM->getModelData()->getAllPhyID(Physical_Case);
                if (caseIDs.empty())
                    return;  // 工况未就绪，等下一次导入完成再试
                qApp->setProperty("mpcomm_pending_results", QString());
                QTimer::singleShot(500, this, [this, resultPaths]() {
                    emit MechCommon::mMechGlobalSignals::getInstance()
                        ->mpcommImportPostResultsSig(resultPaths);
                    });
            };
            connect(MechIOFile::mIOFileManager::getInstance(),
                &MechIOFile::mIOFileManager::readSolveFinishedSig,
                this, tryImportPendingResults);
            //菜单构造时视口模型可能尚未建立（新架构允许），空指针时不挂接；
            //待导结果的主触发路径是上面的计算文件导入完成信号
            if (getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr)
            {
                connect(getCVM->getPreRenderWidget(),
                    &CCSMRendWidget::importGeoModelRenderFinished,
                    this, tryImportPendingResults);
            }
        }
    }

    // 取回桥接挂起的 OPEN_NODE 待导数据（延时确保模型/几何管理器就绪；
    // 取走即清属性，幂等——重复调用无待导数据时为空操作）
    void mMenuWidget::consumePendingMpcommContext()
    {
        const QString pendingTemplateId =
            qApp->property("mpcomm_pending_templateId").toString();
        if (!pendingTemplateId.isEmpty()) {
            qApp->setProperty("mpcomm_pending_templateId", QString());
            QTimer::singleShot(500, this, [this, pendingTemplateId]() {
                slot_mpcommCreateCaseByTemplateId(pendingTemplateId);
                });
        }

        const QString pendingGeo =
            qApp->property("mpcomm_pending_geometry").toString();
        if (!pendingGeo.isEmpty()) {
            qApp->setProperty("mpcomm_pending_geometry", QString());
            QTimer::singleShot(500, this, [this, pendingGeo]() {
                slot_mpcommImportGeometry(pendingGeo);
                });
        }

        const QString pendingCalc =
            qApp->property("mpcomm_pending_calcfile").toString();
        if (!pendingCalc.isEmpty()) {
            qApp->setProperty("mpcomm_pending_calcfile", QString());
            QTimer::singleShot(500, this, [this, pendingCalc]() {
                slot_mpcommImportCalcFile(pendingCalc);
                });
        }

        const QStringList pendingMpif =
            qApp->property("mpcomm_pending_mpif").toStringList();
        if (!pendingMpif.isEmpty()) {
            qApp->setProperty("mpcomm_pending_mpif", QStringList());
            for (const QString& mpifPath : pendingMpif) {
                QTimer::singleShot(500, this, [this, mpifPath]() {
                    slot_mpcommImportMpif(mpifPath);
                    });
            }
        }
    }

    void mMenuWidget::initVariable()
    {
        m_transFileInfoDialog = new mTransFileInfoDialog(this);
        //读写文件信号与槽
        connect(MechIOFile::mIOFileManager::getInstance(), SIGNAL(ioFileErrorManagerSig(MechCommon::IOFileErrorType)), this, SLOT(slot_ioFileError(MechCommon::IOFileErrorType)));
        connect(MechIOFile::mIOFileManager::getInstance(), SIGNAL(readSolveFinishedSig()), this, SLOT(slot_readSolveFinished()));
        connect(MechIOFile::mIOFileManager::getInstance(), SIGNAL(writeSolveFinishedSig()), this, SLOT(slot_writeSolveFinished()));
        connect(MechIOFile::mIOFileManager::getInstance(), SIGNAL(readMeshFinishedSig()), this, SLOT(slot_readMeshFinished()));
        connect(MechIOFile::mIOFileManager::getInstance(), SIGNAL(readMxcaeFinishedSig()), this, SLOT(slot_readMxCaeFinished()));
        connect(MechIOFile::mIOFileManager::getInstance(), SIGNAL(writeMxcaeFinishedSig()), this, SLOT(slot_writeMxCaeFinished()));
        connect(MechIOFile::mIOFileManager::getInstance(), &MechIOFile::mIOFileManager::readMxsesFinishedSig, this, []() { mMxsesRestore::slot_readMxsesFinished(); });
        connect(MechIOFile::mIOFileManager::getInstance(), &MechIOFile::mIOFileManager::writeMxsesFinishedSig, this, []() { mMxsesRestore::slot_writeMxsesFinished(); });
        connect(MechIOFile::mIOFileManager::getInstance(), SIGNAL(readMfFinishedSig()), this, SLOT(slot_readMfFinished()));
        connect(MechIOFile::mIOFileManager::getInstance(), SIGNAL(writeMfFinishedSig()), this, SLOT(slot_writeMfFinished()));
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::switchMenuBarWindow, this, &mMenuWidget::switchMenuBarWindow);
        connect(mGlobalSignals::getInstance(), &mGlobalSignals::deleteViewportModel, this, &mMenuWidget::deleteViewportModel);
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::browsertreeSelectedChanged, this, &mMenuWidget::slot_browsertreeSelectedChanged);
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::caseRotorSettingChangedSig, this, &mMenuWidget::slot_updateRotorButtons);
        connect(mViewportModelManager::getInstance(), &mViewportModelManager::currentViewportChanged, this, &mMenuWidget::slot_subWindowActivated);
        //FR-01/FR-05 单视口形态：动画在云图视口就地生成/播放，全程无视口切换——
        //"动画"页启停重算（refreshPostMenuEnabledState）改由生成/删除收尾显式发射
        //refreshPostMenuEnableSig 触发（原仅靠 currentViewportChanged 链）
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::refreshPostMenuEnableSig, this, &mMenuWidget::refreshPostMenuEnabledState);
        //退出草图模式时移除"草图"动态页签：所有退出路径统一经 sketchExited 通知，
        //替代此前散落（且部分被注释禁用）的各点自行移除
        connect(mGeoSketchSignalManager::instance(), &mGeoSketchSignalManager::sketchExited,
            this, &mMenuWidget::slot_sketchExited);
        qApp->installEventFilter(this);
    }

    void mMenuWidget::initViewMenu()
    {
        //菜单栏常驻形态：统一走共享解析入口 MechCommon::resolveRibbonForm（编译宏定产品
        //默认 + Settings.ini menubar/ribbonForm 覆盖）；mTreeView 的树形态（多体形态隐藏
        //隐式模型根、部件挂多体"部件"固定组）用同一入口，两侧形态永远一致
        _ribbonForm = MechCommon::resolveRibbonForm();

        //两种形态的常驻名单：结构隐式（dev_Mech 默认，14页原样）；多体（10页，隐式功能页彻底隐藏）
        _mechFormResidentMenus = QStringList{ "主页", "模型", "几何", "网格", "材料", "属性", "相互作用", "分析工况", "载荷" , "边界" , "NVH" , "优化" , "显示" , "选择" };
        _motionFormResidentMenus = QStringList{ "主页", "几何", "体定义", "运动副与约束", "驱动", "连接与力元", "传感与控制", "求解", "显示", "选择" };
        //多体形态下彻底隐藏的隐式页 = 隐式名单 - 两形态共享页（主页/几何/显示/选择）
        _motionFormHiddenMenus = _mechFormResidentMenus;
        for (const QString& shared : { QString::fromUtf8("主页"), QString::fromUtf8("几何"),
            QString::fromUtf8("显示"), QString::fromUtf8("选择") })
        {
            _motionFormHiddenMenus.removeAll(shared);
        }
        _residentMenuNames = (_ribbonForm == MechCommon::MenuRibbonForm::Motion)
            ? _motionFormResidentMenus : _mechFormResidentMenus;

        //"显示/选择"页签文本与前处理同名，实际页为PostWindow注册的后处理独立页
        //（页键"显示#Post"/"选择#Post"，见pageForTabText），排最后与页签组分隔线呼应
        _postModeTabs = QStringList{ "主页", "模型", "结果", "云图", "显示", "选择" };
        //曲线模式页签：3D曲线在最右（PostWindow::build3DCurvePage 注册同名页；
        //页签文本与页键一致，无同名冲突不需要 #后缀）
        _curveModeTabs = QStringList{ "主页", "曲线", "报告", "3D曲线" };
        _postMenuCandidates = QStringList{ "主页", "模型", "结果", "云图", "动画", "路径", "流线图",
            "切面图", "等值面图", "矢量图", "分区显示", "后处理-集合", "线性叠加", "分析报告", "显示", "选择" };
        _curveMenuCandidates = QStringList{ "主页", "曲线", "报告", "3D曲线" };

        // “视图”下拉按钮：纯图标（与“新建”等快捷按钮一致），点击弹出下拉菜单。
        // 全自绘 mRibbonButton 纯图标模式：Qt 底座会为带菜单按钮在右下角叠加
        // menu-indicator 箭头（贴顶/贴底的病灶），自绘路径纯图标不画箭头
        //该图标字形占画布75%（新建约67%），按可见字形对齐：20px显示时字形≈新建22px的字形大小
        mRibbonButton* viewBtn = new mRibbonButton(mRibbonButton::Mode::Small);
        viewBtn->setObjectName("ViewButton");
        viewBtn->setToolTip("视图");
        viewBtn->setIcon(QIcon(":/mMainWidgets/Icon/菜单栏设置.svg"));
        viewBtn->setIconSize(UiMetrics::quickIconSize());
        viewBtn->setFixedSize(UiMetrics::quickButtonSize(), UiMetrics::quickButtonSize());
        viewBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_viewButton_ = viewBtn;

        m_viewMenu_ = new mUiMenu(this);
        // 菜单一站式装配（规则 1.0）；子菜单创建后也立即应用
        auto applySubMenuTheme = [](QMenu* m) {
            ThemeManager::applyMenuStyle(m);
        };
        applySubMenuTheme(m_viewMenu_);

        // 常驻菜单栏子菜单（前处理/后处理/曲线）：mUiMenu 原生可勾行——行背景/悬停/右侧勾选框
        // 全部由 mUiMenu 自绘，点击勾选不关闭菜单（mUiMenu::mouseReleaseEvent 对非互斥可勾
        // QAction 走持久激活路径），可连续勾选；勾选控制对应模式的固定页签显隐，
        // 非当前模式时仅记录意图，切到该模式时按意图重建
        auto buildResidentSubMenu = [this, applySubMenuTheme](const QString& title, MenuWorkMode mode,
            const QStringList& names, QMap<QString, QAction*>& menuActions, const QStringList& defaultChecked) {
            QMenu* subMenu = new mUiMenu(title, m_viewMenu_);
            applySubMenuTheme(subMenu);
            for (const QString& name : names)
            {
                QAction* act = subMenu->addAction(name);
                act->setCheckable(true);
                act->setChecked(defaultChecked.contains(name));
                menuActions.insert(name, act);
                connect(act, &QAction::toggled, this, [this, mode, name](bool checked) {
                    setModeMenuVisible(mode, name, checked);
                    });
            }
            m_viewMenu_->addMenu(subMenu);
        };
        buildResidentSubMenu("前处理常驻菜单栏", MenuWorkMode::Pre, _residentMenuNames, _residentMenuActions, _residentMenuNames);
        buildResidentSubMenu("后处理常驻菜单栏", MenuWorkMode::Post, _postMenuCandidates, _postMenuActions, _postModeTabs);
        buildResidentSubMenu("曲线常驻菜单栏", MenuWorkMode::Curve, _curveMenuCandidates, _curveMenuActions, _curveModeTabs);
        m_viewMenu_->addSeparator();

        // 树菜单栏联动子菜单：三个互斥单选项（QActionGroup 互斥，mUiMenu 原生绘制选中底色+
        // 右侧对勾），点击即选中并关闭菜单；选项顺序与TreeMenuLinkMode枚举值一一对应
        QMenu* treeLinkMenu = new mUiMenu("树菜单栏联动", m_viewMenu_);
        applySubMenuTheme(treeLinkMenu);
        auto* modeGroup = new QActionGroup(treeLinkMenu);
        modeGroup->setExclusive(true);
        const QStringList treeLinkModes{ "切换至关联菜单", "仅切换动态菜单", "不自动切换" };
        for (int i = 0; i < treeLinkModes.size(); ++i)
        {
            QAction* act = treeLinkMenu->addAction(treeLinkModes.at(i));
            act->setCheckable(true);
            modeGroup->addAction(act);
            _treeLinkModeItems << act;
            //选中即更新联动模式数据并写配置文件（互斥选中由 QActionGroup 自动完成）
            connect(act, &QAction::triggered, this, [this, i]() {
                mGlobalSignals::getInstance()->setTreeMenuLinkMode(static_cast<TreeMenuLinkMode>(i));
                saveMenuBarSettings();
                });
        }
        m_viewMenu_->addMenu(treeLinkMenu);

        m_viewButton_->setMenu(m_viewMenu_);
        m_viewButton_->setPopupMode(QToolButton::InstantPopup);
        // 自绘按钮不挂任何 QSS：Qt 的 menu-indicator 不参与绘制，菜单由
        // mRibbonButton::showMenu 统一按规则 1.0 定位展开

        // 快捷图标按钮：新建/保存/打开（同视图按钮，全自绘纯图标模式）
        auto makeQuickBtn = [this](QToolButton*& btn, const QString& objName,
            const QString& name, const QString& shortcutId, const QString& icon) {
            mRibbonButton* rb = new mRibbonButton(mRibbonButton::Mode::Small);
            rb->setObjectName(objName);
            rb->setIcon(QIcon(icon));
            rb->setToolButtonStyle(Qt::ToolButtonIconOnly);
            const auto syncToolTip = [rb, name, shortcutId]() {
                const QKeySequence key = ShortcutManager::instance()->shortcut(
                    ShortcutModule::Global, shortcutId);
                rb->setToolTip(key.isEmpty() ? name
                    : QStringLiteral("%1 (%2)").arg(name, key.toString(QKeySequence::NativeText)));
            };
            syncToolTip();
            QTimer::singleShot(0, rb, syncToolTip);
            btn = rb;
            };
        makeQuickBtn(m_quickNewBtn_, "QuickNewBtn", "新建", "new", ":/mMainWidgets/Icon/homepage/xinjian.png");
        makeQuickBtn(m_quickSaveBtn_, "QuickSaveBtn", "保存", "save", ":/mMainWidgets/Icon/homepage/baocun.png");
        makeQuickBtn(m_quickOpenBtn_, "QuickOpenBtn", "打开", "open", ":/mMainWidgets/Icon/homepage/dakai.png");
        connect(m_quickNewBtn_, &QToolButton::clicked, this, &mMenuWidget::slot_New);
        connect(m_quickSaveBtn_, &QToolButton::clicked, this, &mMenuWidget::slot_saveFile);
        connect(m_quickOpenBtn_, &QToolButton::clicked, this, &mMenuWidget::slot_openFile);

        // 尺寸/主题调整已移至 文件->首选项->尺寸与主题
    }

	int mMenuWidget::findTabByText(const QString& name) const
	{
		for (int i = 0; i < m_menuTabBar_->count(); ++i)
		{
			if (m_menuTabBar_->tabText(i) == name)
				return i;
		}
		return -1;
	}

	void mMenuWidget::setModeMenuVisible(MenuWorkMode mode, const QString& name, bool visible)
	{
		const QStringList order = candidateNamesForMode(mode);
		if (!order.contains(name))
			return;

		//当前模式：立即按所属名单顺序插入/移除页签；非当前模式：页签不在TabBar上，
		//仅记录复选框意图（写入配置），切到该模式时由setWorkMode按意图重建
		if (mode == _workMode)
		{
			if (visible)
			{
				if (findTabByText(name) < 0)
				{
					//按所属名单顺序插回原有位置（Pre模式下动态页签始终排在所有常驻页签之后）
					int pos = 0;
					for (const QString& tn : order)
					{
						if (tn == name) break;
						if (findTabByText(tn) >= 0) ++pos;
					}
					m_menuTabBar_->insertTab(pos, name);
				}
			}
			else
			{
				int idx = findTabByText(name);
				if (idx >= 0)
					m_menuTabBar_->removeTab(idx);
			}

			//插入的新页签默认enabled，重应用“结果”/“云图”页启停状态
			refreshPostMenuEnabledState();
		}
		saveMenuBarSettings();
	}

	QStringList mMenuWidget::candidateNamesForMode(MenuWorkMode mode) const
	{
		switch (mode)
		{
		case MenuWorkMode::Post:  return _postMenuCandidates;
		case MenuWorkMode::Curve: return _curveMenuCandidates;
		default:                  return _residentMenuNames;
		}
	}

    QStringList mMenuWidget::visibleTabsForMode(MenuWorkMode mode) const
    {
        const QMap<QString, QAction*>& boxes = (mode == MenuWorkMode::Post) ? _postMenuActions
            : (mode == MenuWorkMode::Curve) ? _curveMenuActions : _residentMenuActions;
        QStringList tabs;
        const QStringList order = candidateNamesForMode(mode);
        for (const QString& name : order)
        {
            auto it = boxes.find(name);
            if (it == boxes.end() || it.value()->isChecked())
                tabs << name;
        }
        return tabs;
    }

	void mMenuWidget::saveMenuBarSettings()
	{
		const auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
		QSettings settings(path, QSettings::IniFormat);

        //按菜单动作（用户意图）计算隐藏名单而非TabBar实况：非当前模式的页签整体不在TabBar上，
        //若按实况计算会把整组菜单误写为“隐藏”污染配置
        auto hiddenNames = [](const QStringList& order, const QMap<QString, QAction*>& boxes) {
            QStringList hidden;
            for (const QString& name : order)
            {
                auto it = boxes.find(name);
                if (it != boxes.end() && !it.value()->isChecked())
                    hidden << name;
            }
            return hidden;
            };
        settings.setValue("menubar/hiddenResidentMenus", hiddenNames(_residentMenuNames, _residentMenuActions));
        settings.setValue("menubar/hiddenPostMenus", hiddenNames(_postMenuCandidates, _postMenuActions));
        settings.setValue("menubar/hiddenCurveMenus", hiddenNames(_curveMenuCandidates, _curveMenuActions));
        settings.setValue("menubar/treeMenuLinkMode",
            static_cast<int>(mGlobalSignals::getInstance()->getTreeMenuLinkMode()));
    }

	/*
	* 单独持久化工作模式：不随setWorkMode即时写盘，仅由mMechMainWindow::closeEvent与
	* saveLayout（DockingState）同时写入——两键同源同刻，异常退出也只会恢复出上一次
	* 正常关闭时自洽的“模式+树布局”组合，不再出现“新workMode+旧树布局”的矛盾恢复
	* （恢复后另有syncTreeWindowsForMode兜底矫正树与模式一致）
	*/
	void mMenuWidget::saveWorkModeSetting()
	{
		const auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
		QSettings settings(path, QSettings::IniFormat);
		settings.setValue("menubar/workMode", static_cast<int>(_workMode));
	}

    namespace
    {
        /*
        * 视口类型->菜单工作模式：后处理云图视口->Post；曲线/曲线图视口->Curve；
        * 其余（前处理）->Pre。激活联动（slot_subWindowActivated）、启动恢复
        * （loadMenuBarSettings）与用户聚焦跟随（slot_viewportUserFocused）
        * 共用同一映射，保证各入口推导一致
        */
        MenuWorkMode workModeForViewportType(ViewportType vpType)
        {
            return (vpType == ViewportPost3D) ? MenuWorkMode::Post
                : (vpType == ViewportPost2D || vpType == ViewportCurve) ? MenuWorkMode::Curve
                : MenuWorkMode::Pre;
        }
    }

    void mMenuWidget::loadMenuBarSettings()
    {
        const auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
        QSettings settings(path, QSettings::IniFormat);

		//常驻页签显隐
		const QStringList hidden = settings.value("menubar/hiddenResidentMenus").toStringList();
		for (const QString& rn : hidden)
		{
			int idx = findTabByText(rn);
			if (idx >= 0)
				m_menuTabBar_->removeTab(idx);
		}

        //树菜单栏联动模式（选项顺序与TreeMenuLinkMode枚举值一一对应）
        int linkMode = settings.value("menubar/treeMenuLinkMode", 0).toInt();
        if (linkMode < 0 || linkMode > static_cast<int>(TreeMenuLinkMode::None))
            linkMode = 0;
        mGlobalSignals::getInstance()->setTreeMenuLinkMode(static_cast<TreeMenuLinkMode>(linkMode));
        for (int i = 0; i < _treeLinkModeItems.size(); ++i)
            _treeLinkModeItems.at(i)->setChecked(i == linkMode);

        //同步“视图”菜单中各项勾选状态（动作阻断信号，避免触发显隐逻辑重复执行）
        for (auto it = _residentMenuActions.begin(); it != _residentMenuActions.end(); ++it)
        {
            it.value()->blockSignals(true);
            it.value()->setChecked(!hidden.contains(it.key()));
            it.value()->blockSignals(false);
        }

        //后处理/曲线常驻菜单勾选（配置存在才恢复；首次运行无配置时保持 initViewMenu 中
        //_postModeTabs/_curveModeTabs 名单的默认勾选）
        auto restoreChecks = [](const QMap<QString, QAction*>& boxes, const QStringList& hiddenNames) {
            for (auto it = boxes.begin(); it != boxes.end(); ++it)
            {
                it.value()->blockSignals(true);
                it.value()->setChecked(!hiddenNames.contains(it.key()));
                it.value()->blockSignals(false);
            }
            };
        if (settings.contains("menubar/hiddenPostMenus"))
            restoreChecks(_postMenuActions, settings.value("menubar/hiddenPostMenus").toStringList());
        if (settings.contains("menubar/hiddenCurveMenus"))
            restoreChecks(_curveMenuActions, settings.value("menubar/hiddenCurveMenus").toStringList());

        //"云图"常驻改临时（一次性迁移）：存量配置按旧默认勾选落盘、hiddenPostMenus
        //不含"云图"，上方restore会把它恢复成常驻——强制取消勾选并把"云图"并入
        //隐藏名单落盘，此后以用户意图为准（视图菜单重新勾选即恢复常驻）；
        //无配置的新装环境复选框本就未勾选，不创建hiddenPostMenus键（创建会使
        //restoreChecks把"动画"等其余候选全部置勾）
        if (!settings.value("menubar/cloudTabTempMigrated").toBool())
        {
            if (QAction* cloudAct = _postMenuActions.value("云图", nullptr))
            {
                cloudAct->blockSignals(true);
                cloudAct->setChecked(false);
                cloudAct->blockSignals(false);
            }
            if (settings.contains("menubar/hiddenPostMenus"))
            {
                QStringList hiddenPost = settings.value("menubar/hiddenPostMenus").toStringList();
                if (!hiddenPost.contains("云图"))
                {
                    hiddenPost << "云图";
                    settings.setValue("menubar/hiddenPostMenus", hiddenPost);
                }
            }
            settings.setValue("menubar/cloudTabTempMigrated", true);
        }

        //启动后的工作模式：按实际当前视口推导，不沿用关盘保存的模式值——重启后
        //模型/视口不随配置恢复，全新启动仅有前处理视口，盲目恢复Curve/Post会得到
        //"菜单在曲线/后处理、视口却是前处理"的错位（该视口已是激活态，点击它又被
        //同ID去重早退，无激活信号可纠偏，菜单停留在错误模式）。恢复会话(.mxses)
        //重建视口的场景经currentViewportChanged正常联动，不受影响。
        //延迟到事件循环首次空闲执行：此刻PostWindow等模块的Ribbon页已注册完毕，
        //过早切换会落向尚未注册的页签（m_StaWidMap查无该页）
        QTimer::singleShot(0, this, [this]()
            {
                auto* priv = VMManager ? VMManager->getCurrentViewportPrivate() : nullptr;
                const auto mode = workModeForViewportType(
                    priv ? priv->getViewportType() : Viewport3D);
                setWorkMode(mode);
                //同模式幂等早退时setWorkMode内部不执行树同步，恢复场景显式补齐
                syncTreeWindowsForMode(mode);
            });

		//移除页签可能已改变当前页签（currentChanged未触发时页面不会跟随切换），显式同步一次
		if (m_menuTabBar_->currentIndex() >= 0)
			slot_CurrentTabChanged(m_menuTabBar_->currentIndex());
	}

    void mMenuWidget::alignModeToViewport(int viewportID)
    {
        //视口类型->模式映射见workModeForViewportType；未知视口按前处理兜底
        auto* priv = VMManager ? VMManager->getViewportPrivateByID(viewportID) : nullptr;
        const auto mode = workModeForViewportType(priv ? priv->getViewportType() : Viewport3D);
        setWorkMode(mode);
        //同模式幂等早退时setWorkMode内部不执行树同步，显式补齐（同loadMenuBarSettings）
        syncTreeWindowsForMode(mode);
    }

    /*
    * 切换菜单栏工作模式：按模式重建TabBar页签集合并落到默认页。
    * Post->主页/模型/结果/显示/选择（落"结果"）；Curve->主页/曲线（落"曲线"）；
    * Pre->常驻页签（按复选框意图恢复）+前处理动态页（落"主页"）。
    * 页widget始终常驻QStackedWidget，此处仅控制页签显隐；同模式幂等直接返回，
    * 保证后处理/前处理上下文内部切换视口时不打扰用户当前页签。
    */
    void mMenuWidget::setWorkMode(MenuWorkMode mode)
    {
        if (_workMode == mode)
            return;
        _workMode = mode;

		//目标页签集合（有序）：按各模式复选框勾选意图过滤候选名单
		const QStringList targetTabs = visibleTabsForMode(mode);

		//离开前处理前若处于草图编辑，先退出（“草图”动态页签即将被移除）
		if (mode != MenuWorkMode::Pre && mGeoSketchSignalManager::instance()->currentStatus())
			mGeoSketchSignalManager::instance()->exitSketch();

		//移除不属于目标集合的页签（从后往前遍历，避免索引位移）
		for (int i = m_menuTabBar_->count() - 1; i >= 0; --i)
		{
			if (!targetTabs.contains(m_menuTabBar_->tabText(i)))
				m_menuTabBar_->removeTab(i);
		}

		//按名单顺序回插缺失页签（与setResidentMenuVisible相同的按序定位法）
		for (const QString& name : targetTabs)
		{
			if (findTabByText(name) < 0)
			{
				int pos = 0;
				for (const QString& tn : targetTabs)
				{
					if (tn == name) break;
					if (findTabByText(tn) >= 0) ++pos;
				}
				m_menuTabBar_->insertTab(pos, name);
			}
		}

		//分界线：Pre恢复常驻名单；Post/Curve页签均为固定页签，传全集即不画“常驻区|动态区”分界线
		m_menuTabBar_->setResidentTabNames(mode == MenuWorkMode::Pre ? _residentMenuNames : targetTabs);

        //落到模式默认页（默认页被取消勾选时落到该模式第一个可见页签）
        QString landing = (mode == MenuWorkMode::Post) ? "结果"
            : (mode == MenuWorkMode::Curve) ? "曲线" : "主页";
        if (!targetTabs.contains(landing) && !targetTabs.isEmpty())
            landing = targetTabs.first();
        const int idx = findTabByText(landing);
        if (idx >= 0 && m_menuTabBar_->currentIndex() != idx)
            m_menuTabBar_->setCurrentIndex(idx);

		//持久化菜单设置（页签显隐/联动模式；工作模式改由closeEvent与布局同刻写盘，见saveWorkModeSetting）
		saveMenuBarSettings();

		//页签重建后新插入的页签默认enabled，重应用按激活视口计算的启停状态
		refreshPostMenuEnabledState();

		//menu→tree方向同步：左侧树随工作模式切换，与渲染视口激活时mTreeView的树同步
		//（updateTreeSectionsForViewport）走同一入口，消除仅切菜单路径（启动恢复、
		//switchMenuBarWindow跨模式页请求、曲线树节点选中）造成的菜单/树错位
		syncTreeWindowsForMode(mode);
	}

	/*
	* 按工作模式同步左侧树：Post→云图树、Curve→曲线树、Pre→模型树（三棵树互斥，
	* 视图树常显）。updateTreeWindows只切树dock显隐、不做置前，不打扰用户当前焦点
	*/
	void mMenuWidget::syncTreeWindowsForMode(MenuWorkMode mode)
	{
		const ViewportType type = (mode == MenuWorkMode::Post) ? ViewportPost3D
			: (mode == MenuWorkMode::Curve) ? ViewportCurve : Viewport3D;
		mDockLayoutManager::getInstance()->updateTreeWindows(type);
	}

    /*
    * 按激活视口类型启停后处理“结果”/“云图”页：仅云图视口（后处理模型视口ViewportPost3D）
    * 激活时可用；无激活视口（视口全删，currentViewportID=-1）视为不可用。动画页仅动画
    * 视口激活时可用——FR-01/FR-05 单视口形态下动画视口=云图视口（AnimationInfo.viewName
    * 即云图视口名），判定走管理器关系命中；正则兜底保留兼容存量会话的旧派生名
    * （<云图视口名>_Animation<N>）。触发时机：视口激活链 + refreshPostMenuEnableSig
    * （动画生成/删除收尾显式发射，补偿单视口形态全程无视口切换）。
    * 仅置灰页内功能（禁用沿RibbonPage→mGroupWidget父子链传播整页按钮），
    * 页签保持可点击：随时可切换浏览菜单内容，符合主流Ribbon“页签可切、命令按上下文置灰”惯例
    */
    void mMenuWidget::refreshPostMenuEnabledState()
    {
        auto* viewportPrivate = VMManager->getCurrentViewportPrivate();
        const bool enablePost = viewportPrivate && viewportPrivate->getViewportType() == ViewportPost3D;
        //动画页启停（2026-11-05 视口绑定收敛后重写）：旧规则依赖"_Animation\d+"派生视口名
        //或 AnimationItem 管理器关系（2026-10-13 曾因正则坏转义恒禁用，见彼时修复），两者
        //已随派生视口/条目去除断源，动画页再次恒禁用——页面级禁用覆盖按钮级使能，一键
        //生成后"暂停"灰显、动画无法停止。动画数据现挂在源云图视口 guidata 上，页启停与
        //"云图/结果"同规则（Post3D 视口激活即启用）；页内五钮/速率等控件启停仍由
        //setAnimationMenuStatus/updateAnimationMenuGUI 按钮级管理（未生成动画时全禁）
        const bool enableAnimation = enablePost;


        if (RibbonPage* page = m_StaWidMap.value("云图", nullptr))
            page->setEnabled(enablePost);
        if (RibbonPage* page = m_StaWidMap.value("结果", nullptr))
            page->setEnabled(enablePost);
        if (RibbonPage* page = m_StaWidMap.value("动画", nullptr))
            page->setEnabled(enableAnimation);
        //后处理"显示/选择"独立页（页键带#Post后缀，PostWindow注册）：同"结果/云图"规则，
        //仅云图视口激活时启用；前处理同名页页键不同，不受此启停影响
        if (RibbonPage* page = m_StaWidMap.value("显示#Post", nullptr))
            page->setEnabled(enablePost);
        if (RibbonPage* page = m_StaWidMap.value("选择#Post", nullptr))
            page->setEnabled(enablePost);
        //"模型"独立页（页键"模型#Post"，集合/坐标系创建均依赖后处理模型）：同规则置灰
        if (RibbonPage* page = m_StaWidMap.value("模型#Post", nullptr))
            page->setEnabled(enablePost);
    }


    void mMenuWidget::initHomeMenu()
    {
        // 下方Ribbon工具条上添加相应的显示widget作为Group的载体
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("主页", tempWid);

        //内存泄漏
        // 为menu添加Group分组

		auto tGW =  std::shared_ptr<mGroupWidget>(this->addGroup("主页", "文件"));
        auto tGW2 = std::shared_ptr<mGroupWidget>(this->addGroup("主页", "创建"));
        auto tGW3 = std::shared_ptr<mGroupWidget>(this->addGroup("主页", "布局"));
        auto tGW5 = std::shared_ptr<mGroupWidget>(this->addGroup("主页", "工具"));
        _homeGW.append(tGW);
        _homeGW.append(tGW2);
        _homeGW.append(tGW3);
        _homeGW.append(tGW5);
//#ifdef _DEBUG
//        _homeGW.append(tGW6);
//#endif
        // 在这里定义样式表 groupTitlStyle对应的是 “文件” “创建” “布局” “视口”这些，toolButtonStyle则是上面那些按钮		

        //文件选项组
        if (tGW)
        {

            // 新建、打开、后处理 - 大按钮（图标在上、文字在下）
            QToolButton* newBtn = tGW->addToolBtn("新建", ":/mMainWidgets/Icon/homepage/xinjian3x.png", 0, 1, 2, 1, QSize(30, 30));
            newBtn->setObjectName("narrowButton");
            setQToolButton(newBtn, "新建", ":/mMainWidgets/Icon/homepage/xinjian3x.png", "新建文件");
            connect(newBtn, &QToolButton::clicked, this, &mMenuWidget::slot_New);


            QToolButton* openBtn = tGW->addToolBtn("打开", ":/mMainWidgets/Icon/homepage/dakai3x.png", 0, 2, 2, 1, QSize(30, 30));
            openBtn->setObjectName("narrowButton");
            setQToolButton(openBtn, "打开", ":/mMainWidgets/Icon/homepage/dakai3x.png", "打开文件");
            connect(openBtn, &QToolButton::clicked, this, &mMenuWidget::slot_openFile);

            QToolButton* openPostFileBtn = tGW->addToolBtn("后处理", ":/mMainWidgets/Icon/homepage/houchuli", 0, 3, 2, 1, QSize(30, 30));
            openPostFileBtn->setObjectName("narrowButton");
            setQToolButton(openPostFileBtn, "后处理", ":/mMainWidgets/Icon/homepage/houchuli.png", "打开结果文件");
            //多体形态：后处理结果文件为隐式流程产物，隐藏（多体结果经求解方案的动画/运动曲线查看）
            openPostFileBtn->setVisible(MechCommon::resolveRibbonForm() != MechCommon::MenuRibbonForm::Motion);
            QObject::connect(openPostFileBtn, &QToolButton::clicked, openPostFileBtn, [postBus = MechCommon::mMechGlobalSignals::getInstance()]() {
                emit postBus->clickOpenPostCloudFileSig();
                });

            // 其余六个按钮 - 宽样式
            QToolButton* saveBtn = tGW->addToolBtn("保存", ":/mMainWidgets/Icon/homepage/baocun3x.png", 0, 4, 1, 1, QSize(20, 20));
            saveBtn->setObjectName("wideButton");
            setQToolButton(saveBtn, "保存", ":/mMainWidgets/Icon/homepage/baocun3x.png", "保存文件", Qt::ToolButtonTextBesideIcon);
            connect(saveBtn, &QToolButton::clicked, this, &mMenuWidget::slot_saveFile);


            QToolButton* saveASBtn = tGW->addToolBtn("另存为", ":/mMainWidgets/Icon/homepage/lingcunwei3x.png", 1, 4, 1, 1, QSize(20, 20));
            saveASBtn->setObjectName("wideButton");
            setQToolButton(saveASBtn, "另存为", ":/mMainWidgets/Icon/homepage/lingcunwei3x.png", "将文件另存为", Qt::ToolButtonTextBesideIcon);
            connect(saveASBtn, &QToolButton::clicked, this, &mMenuWidget::slot_saveAsFile);

#ifdef MX_AUTOSAVE_ENABLED
            //自动保存开关（暂不上线功能，屏蔽方式见 mAutoSaveManager.h 顶部说明）
            addAutoSaveBtn(tGW.get());
#endif //MX_AUTOSAVE_ENABLED


            QToolButton* inGeoFileBtn = tGW->addToolBtn("导入几何模型", ":/mMainWidgets/Icon/homepage/daoru3x.png", 0, 5, 1, 1, QSize(20, 20));
            inGeoFileBtn->setObjectName("wideButton");
            //inGeoFileBtn->setProperty("mxHasMenu", true); // 声明带下拉菜单，宽度公式预留箭头占宽
            setQToolButton(inGeoFileBtn, "导入几何模型", ":/mMainWidgets/Icon/homepage/daoru3x.png", "导入几何模型文件", Qt::ToolButtonTextBesideIcon);
            // 分动按钮（split）：点击左区直接执行导入，点击箭头槽（文字右侧
            // arrowReserveWidth 区域）弹出菜单。箭头与文字共用基线由自绘保证，
            // 菜单定位走 mRibbonButton::showMenu，不经 Qt 底座 menu-indicator 槽
            //inGeoFileBtn->setPopupMode(QToolButton::MenuButtonPopup);
            //if (auto* rb = qobject_cast<mRibbonButton*>(inGeoFileBtn))
            //    connect(rb, &mRibbonButton::dropClicked, rb, &mRibbonButton::showMenu);
            //QMenu* inGeoFileMenu = new mUiMenu(inGeoFileBtn);
            //ThemeManager::applyMenuStyle(inGeoFileMenu);
            //QAction* importGeoAction = inGeoFileMenu->addAction("导入外部模型继承");
            //importGeoAction->setIcon(QIcon(":/mMainWidgets/Icon/homepage/daoru3x.png"));
            //connect(importGeoAction, &QAction::triggered, this, &mMenuWidget::slot_importInheritanceGeoFile);
            //inGeoFileBtn->setMenu(inGeoFileMenu);
            connect(inGeoFileBtn, &QToolButton::clicked, this, &mMenuWidget::slot_importGeoFile);

			QToolButton* inSovleFileBtn = tGW->addToolBtn("导入计算模型", ":/mMainWidgets/Icon/homepage/daoru3x.png", 0, 6, 1, 1, QSize(20, 20));
            setQToolButton(inSovleFileBtn, "导入计算模型", ":/mMainWidgets/Icon/homepage/daoru3x.png", "导入计算模型文件", Qt::ToolButtonTextBesideIcon);
            inSovleFileBtn->setObjectName("wideButton");
            connect(inSovleFileBtn, &QToolButton::clicked, this, &mMenuWidget::slot_importMxFile);
            //多体形态：计算模型(.mx)为隐式求解输入/输出，多体走求解页，隐藏导入入口
            inSovleFileBtn->setVisible(MechCommon::resolveRibbonForm() != MechCommon::MenuRibbonForm::Motion);

            QToolButton* outGeoFileBtn = tGW->addToolBtn("导出几何模型", ":/mMainWidgets/Icon/homepage/daochu3x", 1, 5, 1, 1, QSize(20, 20));
            setQToolButton(outGeoFileBtn, "导出几何模型", ":/mMainWidgets/Icon/homepage/daochu3x.png", "导出几何模型文件", Qt::ToolButtonTextBesideIcon);
            outGeoFileBtn->setObjectName("wideButton");
            connect(outGeoFileBtn, &QToolButton::clicked, this, &mMenuWidget::slot_exportGeoFile);

            QToolButton* outSolveFileBtn = tGW->addToolBtn("导出计算模型", ":/mMainWidgets/Icon/homepage/daochu3x", 1, 6, 1, 1, QSize(20, 20));
            setQToolButton(outSolveFileBtn, "导出计算模型", ":/mMainWidgets/Icon/homepage/daochu3x.png", "导出计算模型文件", Qt::ToolButtonTextBesideIcon);
            outSolveFileBtn->setObjectName("wideButton");
            connect(outSolveFileBtn, &QToolButton::clicked, this, &mMenuWidget::slot_exportMxFile);
            //多体形态：隐藏导出计算模型入口（同导入）
            outSolveFileBtn->setVisible(MechCommon::resolveRibbonForm() != MechCommon::MenuRibbonForm::Motion);

            QToolButton* inMeshFileBtn = tGW->addToolBtn("导入网格模型", ":/mMainWidgets/Icon/homepage/daoru3x", 0, 7, 1, 1, QSize(20, 20));
            setQToolButton(inMeshFileBtn, "导入网格模型", ":/mMainWidgets/Icon/homepage/daoru3x.png", "导入网格模型文件", Qt::ToolButtonTextBesideIcon);
            inMeshFileBtn->setObjectName("wideButton");
            connect(inMeshFileBtn, &QToolButton::clicked, this, &mMenuWidget::slot_importMeshFile);
            //多体形态：网格导入为隐式流程（自动柔性体内部剖分），与文件下拉同口径隐藏
            inMeshFileBtn->setVisible(MechCommon::resolveRibbonForm() != MechCommon::MenuRibbonForm::Motion);

            QToolButton* outMeshFileBtn = tGW->addToolBtn("导出网格模型", ":/mMainWidgets/Icon/homepage/daochu3x", 1, 7, 1, 1, QSize(20, 20));
            setQToolButton(outMeshFileBtn, "导出网格模型", ":/mMainWidgets/Icon/homepage/daochu3x.png", "导出网格模型文件", Qt::ToolButtonTextBesideIcon);
            outMeshFileBtn->setObjectName("wideButton");
            connect(outMeshFileBtn, &QToolButton::clicked, this, &mMenuWidget::slot_exportMeshFile);
            outMeshFileBtn->setVisible(MechCommon::resolveRibbonForm() != MechCommon::MenuRibbonForm::Motion);

#ifdef _DEBUG
            // 测试接口
            QToolButton* testBtn = tGW->addToolBtn("测试按钮", ":/mMainWidgets/Icon/homepage/daochu3x", 1, 8, 1, 1, QSize(20, 20));
            setQToolButton(testBtn, "测试按钮", ":/mMainWidgets/Icon/homepage/daochu3x.png", "打开结果文件", Qt::ToolButtonTextBesideIcon);
            testBtn->setObjectName("wideButton");
            connect(testBtn, &QToolButton::clicked, this, &mMenuWidget::slot_testButton);
#endif // _DEBUG


        }
        //创建选项组
        if (tGW2)
        {
            // 直接应用之前定义的样式表


            // 创建分析工况
            QToolButton* analysisBtn = tGW2->addToolBtn("分析", ":/mMainWidgets/Icon/homepage/fenxi3x.png", 0, 1, 2, 1, QSize(30, 30));
            setQToolButton(analysisBtn, "分析", ":/mMainWidgets/Icon/homepage/fenxi3x.png", "显示下拉菜单,选择要使\n用的仿真分析。");
            analysisBtn->setMenu(getCaseMenu());
            analysisBtn->setPopupMode(QToolButton::InstantPopup);
            //多体形态：分析工况为隐式流程入口，隐藏（多体求解经求解页）
            analysisBtn->setVisible(MechCommon::resolveRibbonForm() != MechCommon::MenuRibbonForm::Motion);
            //analysisBtn->setIcon(QIcon(":/mMainWidgets/Icon/Analysis.png"));


            QToolButton* systemUnitBtn = tGW2->addToolBtn("单位", ":/mMainWidgets/Icon/homepage/danwei3x.png", 0, 2, 2, 1, QSize(30, 30));
            systemUnitBtn->setProperty("mxHasMenu", true);
            setQToolButton(systemUnitBtn, "单位", ":/mMainWidgets/Icon/homepage/danwei3x.png", "显示下拉菜单,选择要使\n用的单位系统。");
            systemUnitBtn->setMenu(getUnitsMenu());
            systemUnitBtn->setPopupMode(QToolButton::InstantPopup);


            // 剖面
            QToolButton* crossSectionBtn = tGW2->addToolBtn("剖面",
                ":/mMainWidgets/Icon/homepage/poumian3x.png",
                0, 3, 2, 1, QSize(30, 30));
            setQToolButton(crossSectionBtn, "剖面", ":/mMainWidgets/Icon/homepage/poumian3x.png", "创建剖面对象。");
            crossSectionBtn->setProperty("mxHasMenu", true);
            crossSectionBtn->setPopupMode(QToolButton::InstantPopup);

            QMenu* sectionMenu = new mUiMenu(this);
            ThemeManager::applyMenuStyle(sectionMenu);
            crossSectionBtn->setMenu(sectionMenu);

            /* === 简洁写法：图标 + 文字 === */
            const QSize iconSize(20, 20);

            sectionMenu->addAction(QIcon(":/mMainWidgets/Icon/homepage/pouqiemian3x.png"), "剖切面", this, [this]() {
                slot_createSection(1);
                });

            sectionMenu->addAction(QIcon(":/mMainWidgets/Icon/homepage/zuobiaoxipoumian3x.png"), "坐标系平面", this, [this]() {
                slot_createSection(2);
                });

            sectionMenu->addAction(QIcon(":/mMainWidgets/Icon/homepage/jihepingmian3x.png"), "几何平面", this, [this]() {
                slot_createSection(3);
                });
        }

        //布局选项组
        if (tGW3)
        {
            QToolButton* manageBtn = tGW3->addToolBtn("管理布局", ":/mMainWidgets/Icon/homepage/guanlibuju3x.png", 0, 1, 1, 1, QSize(30, 30));
            manageBtn->setProperty("mxHasMenu", true);
            setQToolButton(manageBtn, "管理布局", ":/mMainWidgets/Icon/homepage/guanlibuju3x.png", "打开界面显示选项的下拉菜单。");
            auto menu = mDockLayoutManager::getInstance()->createLayoutMenuButton();
            ThemeManager::applyMenuStyle(menu);
            //创建视图工具栏的显隐管理
            QAction* action = new QAction("视图工具栏");
            action->setCheckable(true);
            connect(action, &QAction::toggled, this, [this, action](bool checked) {
                if (m_centerMainWindow) {
                    m_centerMainWindow->setViewToolBarVisibility(checked, action);
                }
                else {
                    // 延迟检查，直到 m_centerMainWindow 被设置
                    QTimer* timer = new QTimer(this);
                    timer->setInterval(100);  // 每 100ms 检查一次
                    connect(timer, &QTimer::timeout, this, [this, action, checked, timer]() {
                        if (m_centerMainWindow) {
                            m_centerMainWindow->setViewToolBarVisibility(checked, action);
                            timer->stop();
                            timer->deleteLater();
                        }
                        });
                    timer->start();
                }
                });
            action->setChecked(true);
            menu->addAction(action);
            manageBtn->setMenu(menu);
            manageBtn->setPopupMode(QToolButton::InstantPopup); // 点击就下拉


            QToolButton* resetLayoutBtn = tGW3->addToolBtn("重置布局", ":/mMainWidgets/Icon/homepage/chongzhibuju3x.png", 0, 2, 1, 1, QSize(30, 30));
            resetLayoutBtn->setProperty("mxHasMenu", true);
            setQToolButton(resetLayoutBtn, "重置布局", ":/mMainWidgets/Icon/homepage/chongzhibuju3x.png", "将界面布局恢复到默认状态。");
            resetLayoutBtn->setPopupMode(QToolButton::InstantPopup); // 点击就下拉
            QMenu* resetMenu = new mUiMenu(this);
            ThemeManager::applyMenuStyle(resetMenu);

            // 布局1
            QAction* resetAction = resetMenu->addAction(QIcon(":/mMainWidgets/Icon/homepage/bujuyi3x.png"), "布局一");
            connect(resetAction, &QAction::triggered, this, [this]() {
                mDockLayoutManager::getInstance()->applyPreset1();
                m_centerMainWindow->setViewToolBarArea(Qt::TopToolBarArea);
                });

            // 布局2
            resetAction = resetMenu->addAction(QIcon(":/mMainWidgets/Icon/homepage/bujuer3x.png"), "布局二");
            connect(resetAction, &QAction::triggered, this, [this]() {
                mDockLayoutManager::getInstance()->applyPreset2();
                m_centerMainWindow->setViewToolBarArea(Qt::TopToolBarArea);
                });
            resetLayoutBtn->setMenu(resetMenu);

        }

        //工具
        if (tGW5)
        {
            //快速截图
            QToolButton* quickShotBtn = tGW5->addToolBtn("快速截屏", ":/mMainWidgets/Icon/post/kuaisujieping.png", 0, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(quickShotBtn, "快速截屏", ":/mMainWidgets/Icon/post/kuaisujieping.png", "对当前视口进行截屏。", Qt::ToolButtonTextBesideIcon);

            //自定义截屏
            QToolButton* customShotBtn = tGW5->addToolBtn("自定义截屏", ":/mMainWidgets/Icon/post/zidingyijieping", 1, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(customShotBtn, "自定义截屏", ":/mMainWidgets/Icon/post/zidingyijieping.png", "自定义视口截屏范围。", Qt::ToolButtonTextBesideIcon);

            connect(quickShotBtn, &QToolButton::clicked, this, [this]() {
                //截屏执行体已随动画族迁PostWindow::mPostInfoHub，经总线解耦
                emit MechCommon::mMechGlobalSignals::getInstance()->clickAniQuickScreenshotSig();
                });

            connect(customShotBtn, &QToolButton::clicked, this, [this]() {
                emit MechCommon::mMechGlobalSignals::getInstance()->clickCustomScreenshotSig();
                });


        }

        //AI助手（仅调试版本显示“优化”入口，随分组一并隐藏/显示）
//#ifdef _DEBUG
//        if (tGW6)
//        {
//            QToolButton* myPanelBtn = tGW6->addToolBtn("优化", ":/mMainWidgets/Icon/BrowserTree/jiegouyouhua.png", 0, 1, 1, 1, QSize(30, 30), 10);
//            myPanelBtn->setMinimumHeight(50);
//            myPanelBtn->setMinimumWidth(40);
//            connect(myPanelBtn, &QToolButton::clicked, this, &mMenuWidget::slot_createMyPanel);
//        }
//#endif
    }

    void mMenuWidget::initModelMenu()
    {
        // 下方Ribbon工具条上添加相应的显示widget作为Group的载体
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("模型", tempWid);

        //内存泄漏
        // 为menu添加Group分组
        mGroupWidget* tGW2 = this->addGroup("模型", "定义");
        mGroupWidget* tGWCtrl = this->addGroup("模型", "控制");
        mGroupWidget* tGW3 = this->addGroup("模型", "优化");
        mGroupWidget* tGW4 = this->addGroup("模型", "求解");

        //定义选项组
        if (tGW2)
        {
            //集合：下拉选择要创建的集合类型
            QToolButton* setBtn = tGW2->addToolBtn("集合", ":/mMainWidgets/Icon/homepage/jihe3x.png", 0, 1, 1, 1, QSize(30, 30), 10);
            setQToolButton(setBtn, "集合", ":/mMainWidgets/Icon/homepage/jihe3x.png", "显示下拉菜单,选择要创\n建的集合类型。");
            setBtn->setMenu(getSetMenu());
            setBtn->setPopupMode(QToolButton::InstantPopup);

            //函数
            QToolButton* functionBtn = tGW2->addToolBtn("函数", ":/mMainWidgets/Icon/homepage/hanshu3x.png", 0, 2, 1, 1, QSize(30, 30), 10);
            setQToolButton(functionBtn, "函数", ":/mMainWidgets/Icon/homepage/hanshu3x.png", "创建函数。");
            connect(functionBtn, &QToolButton::clicked, this, &mMenuWidget::slot_createFunction);

            //坐标系
            QToolButton* coorSysBtn = tGW2->addToolBtn("坐标系", ":/mMainWidgets/Icon/homepage/zuobiaoxi3x.png", 0, 3, 1, 1, QSize(30, 30), 10);
            setQToolButton(coorSysBtn, "坐标系", ":/mMainWidgets/Icon/homepage/zuobiaoxi3x.png", "插入坐标对象\n创建坐标系。");
            connect(coorSysBtn, &QToolButton::clicked, this, &mMenuWidget::slot_createCoordinate);

            //离散场
            QToolButton* discreteBtn = tGW2->addToolBtn("离散场", ":/mMainWidgets/Icon/DiscreteField/discreteField@3x.png", 0, 4, 1, 1, QSize(30, 30), 10);
            setQToolButton(discreteBtn, "离散场", ":/mMainWidgets/Icon/DiscreteField/discreteField@3x.png", "创建离散场。");
            connect(discreteBtn, &QToolButton::clicked, this, &mMenuWidget::slot_createDiscreteFieldNoType);

            //非结构质量
            QToolButton* nsmBtn = tGW2->addToolBtn("非结构质量", ":/mMainWidgets/Icon/Case/nsm@3x.png", 0, 5, 1, 1, QSize(30, 30), 10);
            setQToolButton(nsmBtn, "非结构质量", ":/mMainWidgets/Icon/Case/nsm@3x.png", "创建非结构质量。");
            connect(nsmBtn, &QToolButton::clicked, this, &mMenuWidget::slot_createNonStructuralQuality);

            //采样频率点
            QToolButton* freqPointBtn = tGW2->addToolBtn("采样频率点", ":/mMainWidgets/Icon/homepage/caiyangpinlvdian.png", 0, 6, 1, 1, QSize(30, 30), 10);
            setQToolButton(freqPointBtn, "采样频率点", ":/mMainWidgets/Icon/homepage/caiyangpinlvdian.png", "创建采样频率点。");
            connect(freqPointBtn, &QToolButton::clicked, this, &mMenuWidget::slot_createFREQ);
        }

        //控制选项组
        if (tGWCtrl)
        {
            QToolButton* globalParamBtn = tGWCtrl->addToolBtn("全局参数", ":/mMainWidgets/Icon/BrowserTree/globalParam.png", 0, 1, 1, 1, QSize(30, 30), 10);
            setQToolButton(globalParamBtn, "全局参数", ":/mMainWidgets/Icon/BrowserTree/globalParam.png", "跳转到全局参数设置菜单栏。");
            connect(globalParamBtn, &QToolButton::clicked, this, [this]() {
                mMechGlobalSignals::getInstance()->switchMenuBarWindow("全局参数");
                });
        }

        //结构优化
        if (tGW3)
        {
            QToolButton* structuralbtn = tGW3->addToolBtn("优化", ":/mMainWidgets/Icon/TopologyOptimization/tuopuyouhua.png", 0, 1, 1, 1, QSize(30, 30), 10);
            setQToolButton(structuralbtn, "优化", ":/mMainWidgets/Icon/TopologyOptimization/tuopuyouhua.png", "创建结构优化。");
            connect(structuralbtn, &QToolButton::clicked, this, &mMenuWidget::slot_createStrual);
            //多体形态：结构优化为隐式功能，隐藏入口
            structuralbtn->setVisible(MechCommon::resolveRibbonForm() != MechCommon::MenuRibbonForm::Motion);

            //QToolButton* runnerStructuralbtn = tGW3->addToolBtn("流道优化", ":/mMainWidgets/Icon/TopologyOptimization/runner.png", 0, 2, 1, 1, QSize(30, 30), 10);
            //setQToolButton(runnerStructuralbtn, "流道优化", ":/mMainWidgets/Icon/TopologyOptimization/runner.png", "创建流道优化。");
            //connect(runnerStructuralbtn, &QToolButton::clicked, this, &mMenuWidget::slot_createRunnerStrual);
        }

        //求解选项组
        if (tGW4)
        {

            QToolButton* createbtn = tGW4->addToolBtn("一键剖分", ":/mMainWidgets/Icon/Mesh/shengcheng.png", 0, 1, 1, 1, QSize(30, 30), 10);
            setQToolButton(createbtn, "一键剖分", ":/mMainWidgets/Icon/Mesh/shengcheng.png", "对当前模型中的全部部件一键生成网格。");
            connect(createbtn, &QToolButton::clicked, this, [=]() {
                if (getCVM->isPostModel())
                {
                    return;
                }
                mInfoWindow::getInstance()->createMeshAuto();
                });

            QToolButton* checktn = tGW4->addToolBtn("模型检查", ":/mMainWidgets/Icon/homepage/moxingjiancha.png", 0, 2, 1, 1, QSize(30, 30), 10);
            setQToolButton(checktn, "模型检查", ":/mMainWidgets/Icon/homepage/moxingjiancha.png", "检查是否具备计算的基本条件，\n各必须模块是否已定义，是否缺失参数。");
            connect(checktn, &QToolButton::clicked, this, [=]() {
                if (getCVM->isPostModel())
                {
                    return;
                }
                if (!getCVM->getModelData()->checkModelData() || !getCVM->getDataStatusManager()->getDynamicInfoData().isEmpty())
                {
                    mDockLayoutManager::getInstance()->activateDock("Output");
                    mOutputInfoWindow::getInstance()->showTableInfo();
                }
                else
                {
                    mGlobalSignals::getInstance()->outputMessageSig("模型具备计算基本条件！");
                }
                });

            QToolButton* solveTaskBtn = tGW4->addToolBtn("求解任务", ":/mMainWidgets/Icon/homepage/tijiaojisuan.png", 0, 3, 1, 1, QSize(30, 30), 10);
            setQToolButton(solveTaskBtn, "求解任务", ":/mMainWidgets/Icon/homepage/tijiaojisuan.png", "打开当前模型的求解任务配置弹窗：\n创建任务、提交求解、查看队列与记录。");
            connect(solveTaskBtn, &QToolButton::clicked, this, [=]() {
                if (getCVM == nullptr || getCVM->isPostModel())
                {
                    return;
                }
                mSolveTaskDialog::openDialog(getCVM->getModelData()->getModelDataID(), this);
                });
        }

        // 多体载荷导入选项组（独立一组，排在求解组之后；主动唤出面板，不自动触发）
        mGroupWidget* tGW5 = this->addGroup("模型", "多体载荷");
        if (tGW5)
        {
            QToolButton* motionLoadBtn = tGW5->addToolBtn("多体载荷导入", ":/mMainWidgets/Icon/Case/li", 0, 1, 1, 1, QSize(30, 30), 10);
            setQToolButton(motionLoadBtn, "多体载荷导入", ":/mMainWidgets/Icon/Case/li", "导入多体特征工况载荷（JSON）。");
            connect(motionLoadBtn, &QToolButton::clicked, this, [this]() {
                slot_openMotionLoadImport();
                });
        }
    }

    //------------------------------结构优化--------------------------
    void mMenuWidget::initStructuralMenu()
    {

        // 下方Ribbon工具条上添加相应的显示widget作为Group的载体
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("优化", tempWid);

        mGroupWidget* mGW1 = this->addGroup("优化", "设计域");
        mGroupWidget* mGW2 = this->addGroup("优化", "优化目标");
        mGroupWidget* mGW3 = this->addGroup("优化", "优化约束");

        if (mGW1)
        {
            QToolButton* createbtn1 = mGW1->addToolBtn("实体", ":/mMainWidgets/Icon/TopologyOptimization/shejiyushiti.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(createbtn1, "实体", ":/mMainWidgets/Icon/TopologyOptimization/shejiyushiti.png", "实体。");
            createbtn1->setIconSize(QSize(30, 30));

            connect(createbtn1, &QToolButton::clicked, this, [this, createbtn1]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createOptimizationDesign(createbtn1->text());
                });

            QToolButton* createbtn2 = mGW1->addToolBtn("壳", ":/mMainWidgets/Icon/TopologyOptimization/qiaoti.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(createbtn2, "壳", ":/mMainWidgets/Icon/TopologyOptimization/qiaoti.png", "壳。");
            createbtn2->setIconSize(QSize(30, 30));

            connect(createbtn2, &QToolButton::clicked, this, [this, createbtn2]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createOptimizationDesign(createbtn2->text());
                });

            //QToolButton* createbtn3 = mGW1->addToolBtn("形状", ":/mMainWidgets/Icon/TopologyOptimization/xingzhuang.png", 0, 2, 1, 1, QSize(30, 30));
            //setQToolButton(createbtn3, "形状", ":/mMainWidgets/Icon/TopologyOptimization/xingzhuang.png", "形状。");
            //createbtn3->setIconSize(QSize(30, 30));

            //connect(createbtn3, &QToolButton::clicked, this, [this, createbtn3]() {
            //    mInfoWindow::getInstance()->unEditInfoWindow();
            //    mInfoWindow::getInstance()->createOptimizationDesign(createbtn3->text());
            //    });
        }
        if (mGW2)
        {
            QToolButton* checktn1 = mGW2->addToolBtn("柔度", ":/mMainWidgets/Icon/TopologyOptimization/roudu.png", 0, 1, 1, 1, QSize(30, 30), 10);
            setQToolButton(checktn1, "柔度", ":/mMainWidgets/Icon/TopologyOptimization/roudu.png", "柔度");
            connect(checktn1, &QToolButton::clicked, this, [this, checktn1]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->editOptimizationGoal(checktn1->text());
                });

            QToolButton* checktn2 = mGW2->addToolBtn("频率", ":/mMainWidgets/Icon/TopologyOptimization/pinglu.png", 0, 2, 1, 1, QSize(30, 30), 10);
            setQToolButton(checktn2, "频率", ":/mMainWidgets/Icon/TopologyOptimization/pinglu.png", "频率");
            connect(checktn2, &QToolButton::clicked, this, [this, checktn2]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->editOptimizationGoal(checktn2->text());
                });

            QToolButton* checktn3 = mGW2->addToolBtn("位移", ":/mMainWidgets/Icon/TopologyOptimization/weiyi.png", 0, 3, 1, 1, QSize(30, 30), 10);
            setQToolButton(checktn3, "位移", ":/mMainWidgets/Icon/TopologyOptimization/weiyi.png", "位移");
            connect(checktn3, &QToolButton::clicked, this, [this, checktn3]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->editOptimizationGoal(checktn3->text());
                });

            QToolButton* checktn4 = mGW2->addToolBtn("综合柔度", ":/mMainWidgets/Icon/TopologyOptimization/zongheroudu.png", 0, 4, 1, 1, QSize(30, 30), 10);
            setQToolButton(checktn4, "综合柔度", ":/mMainWidgets/Icon/TopologyOptimization/zongheroudu.png", "综合柔度");
            connect(checktn4, &QToolButton::clicked, this, [this, checktn4]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->editOptimizationGoal(checktn4->text());
                });

            QToolButton* checktn5 = mGW2->addToolBtn("其他", ":/mMainWidgets/Icon/TopologyOptimization/qita.png", 0, 5, 1, 1, QSize(30, 30), 10);
            setQToolButton(checktn5, "其他", ":/mMainWidgets/Icon/TopologyOptimization/qita.png", "其他");


            auto addOptAction = [&](QMenu* menu, const QString& icon, const QString& text)
                {
                    auto action = menu->addAction(QIcon(icon), text);
                    connect(action, &QAction::triggered, this, [action, text]()
                        {
                            mInfoWindow::getInstance()->unEditInfoWindow();
                            mInfoWindow::getInstance()->editOptimizationGoal(text);
                        });
                };

            QMenu* loadMenu = new mUiMenu(this);
            ThemeManager::applyMenuStyle(loadMenu);
            checktn5->setMenu(loadMenu);
            checktn5->setPopupMode(QToolButton::InstantPopup);
            addOptAction(loadMenu, ":/mMainWidgets/Icon/TopologyOptimization/zonghepingyu.png", "综合频率");
            addOptAction(loadMenu, ":/mMainWidgets/Icon/TopologyOptimization/rouduzhishu.png", "柔度指数");
            addOptAction(loadMenu, ":/mMainWidgets/Icon/TopologyOptimization/reroudu.png", "热柔度");
            addOptAction(loadMenu, ":/mMainWidgets/Icon/TopologyOptimization/yingli.png", "应力");
            addOptAction(loadMenu, ":/mMainWidgets/Icon/TopologyOptimization/jiedianwendu .png", "节点温度");
        }

        if (mGW3)
        {
            QToolButton* createbtn17 = mGW3->addToolBtn("体积", ":/mMainWidgets/Icon/TopologyOptimization/tiji.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(createbtn17, "体积", ":/mMainWidgets/Icon/TopologyOptimization/tiji.png", "体积。");
            createbtn17->setIconSize(QSize(30, 30));

            connect(createbtn17, &QToolButton::clicked, this, [this, createbtn17]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createOptimizationConstraint(createbtn17->text());
                });

            QToolButton* createbtn19 = mGW3->addToolBtn("质量", ":/mMainWidgets/Icon/TopologyOptimization/zhiliang.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(createbtn19, "质量", ":/mMainWidgets/Icon/TopologyOptimization/zhiliang.png", "质量。");
            createbtn19->setIconSize(QSize(30, 30));

            connect(createbtn19, &QToolButton::clicked, this, [this, createbtn19]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createOptimizationConstraint(createbtn19->text());
                });


            QToolButton* createbtn20 = mGW3->addToolBtn("体积分数", ":/mMainWidgets/Icon/TopologyOptimization//tijifenshu.png", 0, 2, 1, 1, QSize(30, 30));
            setQToolButton(createbtn20, "体积分数", ":/mMainWidgets/Icon/TopologyOptimization//tijifenshu.png", "体积分数。");
            createbtn20->setIconSize(QSize(30, 30));

            connect(createbtn20, &QToolButton::clicked, this, [this, createbtn20]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createOptimizationConstraint(createbtn20->text());
                });


            QToolButton* createbtn18 = mGW3->addToolBtn("质量分数", ":/mMainWidgets/Icon/TopologyOptimization/zhiliangfenshu.png", 0, 3, 1, 1, QSize(30, 30));
            setQToolButton(createbtn18, "质量分数", ":/mMainWidgets/Icon/TopologyOptimization/zhiliangfenshu.png", "质量分数。");
            createbtn18->setIconSize(QSize(30, 30));

            connect(createbtn18, &QToolButton::clicked, this, [this, createbtn18]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createOptimizationConstraint(createbtn18->text());
                });

        }
    }

    void mMenuWidget::initRunnerStructuralMenu()
    {
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("流道优化", tempWid);

        mGroupWidget* mGW1 = this->addGroup("流道优化", "设计域");
        mGroupWidget* mGW2 = this->addGroup("流道优化", "优化目标");
        mGroupWidget* mGW3 = this->addGroup("流道优化", "优化约束");

        if (mGW1)
        {
            QToolButton* createbtn1 = mGW1->addToolBtn("实体", ":/mMainWidgets/Icon/TopologyOptimization/shejiyushiti.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(createbtn1, "实体", ":/mMainWidgets/Icon/TopologyOptimization/shejiyushiti.png", "实体。");
            createbtn1->setIconSize(QSize(30, 30));

            connect(createbtn1, &QToolButton::clicked, this, [this, createbtn1]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createRunnerOptimizationDesign(createbtn1->text());
                });
        }
        if (mGW2)
        {
            QToolButton* checktn1 = mGW2->addToolBtn("平均压强", ":/mMainWidgets/Icon/TopologyOptimization/averagePressure.png", 0, 1, 1, 1, QSize(30, 30), 10);
            setQToolButton(checktn1, "平均压强", ":/mMainWidgets/Icon/TopologyOptimization/averagePressure.png", "平均压强");
            connect(checktn1, &QToolButton::clicked, this, [this, checktn1]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->editRunnerOptimizationGoal(checktn1->text());
                });

            QToolButton* checktn2 = mGW2->addToolBtn("流量均匀性", ":/mMainWidgets/Icon/TopologyOptimization/dischargeUniformity.png", 0, 2, 1, 1, QSize(30, 30), 10);
            setQToolButton(checktn2, "流量均匀性", ":/mMainWidgets/Icon/TopologyOptimization/dischargeUniformity.png", "流量均匀性");
            connect(checktn2, &QToolButton::clicked, this, [this, checktn2]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->editRunnerOptimizationGoal(checktn2->text());
                });
        }

        if (mGW3)
        {
            QToolButton* createbtn17 = mGW3->addToolBtn("体积", ":/mMainWidgets/Icon/TopologyOptimization/tiji.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(createbtn17, "体积", ":/mMainWidgets/Icon/TopologyOptimization/tiji.png", "体积。");
            createbtn17->setIconSize(QSize(30, 30));

            connect(createbtn17, &QToolButton::clicked, this, [this, createbtn17]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createRunnerOptimizationConstraint(createbtn17->text());
                });

            QToolButton* createbtn19 = mGW3->addToolBtn("质量", ":/mMainWidgets/Icon/TopologyOptimization/zhiliang.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(createbtn19, "质量", ":/mMainWidgets/Icon/TopologyOptimization/zhiliang.png", "质量。");
            createbtn19->setIconSize(QSize(30, 30));

            connect(createbtn19, &QToolButton::clicked, this, [this, createbtn19]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createRunnerOptimizationConstraint(createbtn19->text());
                });


            QToolButton* createbtn20 = mGW3->addToolBtn("体积分数", ":/mMainWidgets/Icon/TopologyOptimization//tijifenshu.png", 0, 2, 1, 1, QSize(30, 30));
            setQToolButton(createbtn20, "体积分数", ":/mMainWidgets/Icon/TopologyOptimization//tijifenshu.png", "体积分数。");
            createbtn20->setIconSize(QSize(30, 30));

            connect(createbtn20, &QToolButton::clicked, this, [this, createbtn20]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createRunnerOptimizationConstraint(createbtn20->text());
                });


            QToolButton* createbtn18 = mGW3->addToolBtn("质量分数", ":/mMainWidgets/Icon/TopologyOptimization/zhiliangfenshu.png", 0, 3, 1, 1, QSize(30, 30));
            setQToolButton(createbtn18, "质量分数", ":/mMainWidgets/Icon/TopologyOptimization/zhiliangfenshu.png", "质量分数。");
            createbtn18->setIconSize(QSize(30, 30));

            connect(createbtn18, &QToolButton::clicked, this, [this, createbtn18]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createRunnerOptimizationConstraint(createbtn18->text());
                });

        }

    }

    void mMenuWidget::initGlobalParamMenu()
    {
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("全局参数", tempWid);

        mGroupWidget* mGW1 = this->addGroup("全局参数", "超单元");
        mGroupWidget* mGW2 = this->addGroup("全局参数", "诊断优化");
        mGroupWidget* mGW3 = this->addGroup("全局参数", "求解控制");

        if (mGW1)
        {
            auto createbtn2 = mGW1->addToolBtn("超单元缩聚方法", ":/mMainWidgets/Icon/GlobalParameters/genericParam.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(createbtn2, "超单元缩聚方法", ":/mMainWidgets/Icon/GlobalParameters/genericParam.png", "超单元缩聚方法。");
            createbtn2->setIconSize(QSize(30, 30));
            connect(createbtn2, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createGenericParam();
                });

            QToolButton* createbtn3 = mGW1->addToolBtn("超单元调用", ":/mMainWidgets/Icon/GlobalParameters/genericAssign.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(createbtn3, "超单元调用", ":/mMainWidgets/Icon/GlobalParameters/genericAssign.png", "超单元调用。");
            createbtn3->setIconSize(QSize(30, 30));
            connect(createbtn3, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createGenericAssign();
                });

            QToolButton* createbtn4 = mGW1->addToolBtn("超单元矩阵", ":/mMainWidgets/Icon/GlobalParameters/genericMatrix.png", 0, 2, 1, 1, QSize(30, 30));
            setQToolButton(createbtn4, "超单元矩阵", ":/mMainWidgets/Icon/GlobalParameters/genericMatrix.png", "超单元矩阵。");
            createbtn4->setIconSize(QSize(30, 30));
            connect(createbtn4, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createGenericMatrix();
                });

            QToolButton* createbtn5 = mGW1->addToolBtn("超单元部件", ":/mMainWidgets/Icon/GlobalParam/genericPart.png", 0, 3, 1, 1, QSize(30, 30));
            setQToolButton(createbtn5, "超单元部件", ":/mMainWidgets/Icon/BrowserTree/genericPart.png", "超单元部件。");
            createbtn5->setIconSize(QSize(30, 30));
            connect(createbtn5, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createGenericPart();
                });

            QToolButton* createbtn6 = mGW1->addToolBtn("声固耦合对", ":/mMainWidgets/Icon/GlobalParam/ascouplingSearch.png", 0, 4, 1, 1, QSize(30, 30));
            setQToolButton(createbtn6, "声固耦合对搜索", ":/mMainWidgets/Icon/BrowserTree/ascouplingSearch.png", "声固耦合对搜索。");
            createbtn6->setIconSize(QSize(30, 30));
            connect(createbtn6, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createASCouplingSearch();
                });
        }

        if (mGW2)
        {
            QToolButton* modalContriBtn = mGW2->addToolBtn("模态贡献量", ":/mMainWidgets/Icon/GlobalParameters/motaigongxianliang.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(modalContriBtn, "模态贡献量", ":/mMainWidgets/Icon/GlobalParameters/motaigongxianliang.png", "创建模态贡献量。");
            modalContriBtn->setIconSize(QSize(30, 30));
            connect(modalContriBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createModalContri(0);
                });

            QToolButton* nodeContriBtn = mGW2->addToolBtn("节点贡献量", ":/mMainWidgets/Icon/GlobalParameters/jiediangongxianliang.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(nodeContriBtn, "节点贡献量", ":/mMainWidgets/Icon/GlobalParameters/jiediangongxianliang.png", "创建节点贡献量。");
            nodeContriBtn->setIconSize(QSize(30, 30));
            connect(nodeContriBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createNodeContri(0);
                });

            QToolButton* panelContriBtn = mGW2->addToolBtn("面板贡献量", ":/mMainWidgets/Icon/GlobalParameters/mianbangongxianliang.png", 0, 2, 1, 1, QSize(30, 30));
            setQToolButton(panelContriBtn, "面板贡献量", ":/mMainWidgets/Icon/GlobalParameters/mianbangongxianliang.png", "创建面板贡献量。");
            panelContriBtn->setIconSize(QSize(30, 30));
            connect(panelContriBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createPanelContri(0);
                });

            QToolButton* tpContriBtn = mGW2->addToolBtn("传递路径贡献量", ":/mMainWidgets/Icon/GlobalParameters/chuandilujinggongxianliang.png", 0, 3, 1, 1, QSize(30, 30));
            setQToolButton(tpContriBtn, "传递路径贡献量", ":/mMainWidgets/Icon/GlobalParameters/chuandilujinggongxianliang.png", "创建传递路径贡献量。");
            tpContriBtn->setIconSize(QSize(30, 30));
            connect(tpContriBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createTPContri(0);
                });

            QToolButton* erpBtn = mGW2->addToolBtn("等效辐射功率", ":/mMainWidgets/Icon/GlobalParameters/dengxiaofushegonglv.png", 0, 4, 1, 1, QSize(30, 30));
            setQToolButton(erpBtn, "等效辐射功率", ":/mMainWidgets/Icon/GlobalParameters/dengxiaofushegonglv.png", "创建等效辐射功率。");
            erpBtn->setIconSize(QSize(30, 30));
            connect(erpBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createERP(0);
                });

            QToolButton* peakFilterBtn = mGW2->addToolBtn("峰值筛选", ":/mMainWidgets/Icon/GlobalParameters/peakfilter.png", 0, 5, 1, 1, QSize(30, 30));
            setQToolButton(peakFilterBtn, "峰值筛选", ":/mMainWidgets/Icon/GlobalParameters/peakfilter.png", "创建峰值筛选。");
            peakFilterBtn->setIconSize(QSize(30, 30));
            connect(peakFilterBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createPeakFilter();
                });
        }

        if (mGW3)
        {
            QToolButton* outputControlBtn = mGW3->addToolBtn("全局输出控制", ":/mMainWidgets/Icon/GlobalParam/quanjushuchu.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(outputControlBtn, "全局输出控制", ":/mMainWidgets/Icon/BrowserTree/solveControl.png", "创建全局输出控制。");
            outputControlBtn->setIconSize(QSize(30, 30));
            connect(outputControlBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createGlobalOutputControl();
                });
            QToolButton* solveControlBtn = mGW3->addToolBtn("求解控制", ":/mMainWidgets/Icon/GlobalParam/qiujiekongzhi.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(solveControlBtn, "求解控制", ":/mMainWidgets/Icon/BrowserTree/solveControl.png", "创建求解控制。");
            solveControlBtn->setIconSize(QSize(30, 30));
            connect(solveControlBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createSolveControl();
                });
        }

    }

    void mMenuWidget::initInteractionMenu()
    {
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("相互作用", tempWid);

        mGroupWidget* mGW1 = this->addGroup("相互作用", "连接");
        mGroupWidget* mGW2 = this->addGroup("相互作用", "连接副");
        mGroupWidget* mGW3 = this->addGroup("相互作用", "接触");
        mGroupWidget* mGW5 = this->addGroup("相互作用", "模型修改");

        if (mGW1)
        {
            QToolButton* createbtn2 = mGW1->addToolBtn("绑定", ":/mMainWidgets/Icon/Connector/bangding.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(createbtn2, "绑定", ":/mMainWidgets/Icon/Connector/bangding.png", "绑定。");
            createbtn2->setIconSize(QSize(30, 30));
            connect(createbtn2, &QToolButton::clicked, this, [this]() {
                slot_createConnector(MxFunctions::ConnectorType::Connector_Bind);
                });

            QToolButton* createbtn3 = mGW1->addToolBtn("刚性连接", ":/mMainWidgets/Icon/Connector/gangxinglianjie.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(createbtn3, "刚性连接", ":/mMainWidgets/Icon/Connector/gangxinglianjie.png", "刚性连接。");
            createbtn3->setIconSize(QSize(30, 30));
            connect(createbtn3, &QToolButton::clicked, this, [this]() {
                slot_createConnector(MxFunctions::ConnectorType::Connector_Rigid);
                });

            QToolButton* createbtn4 = mGW1->addToolBtn("柔性连接", ":/mMainWidgets/Icon/Connector/rouxinglianjie.png", 0, 2, 1, 1, QSize(30, 30));
            setQToolButton(createbtn4, "柔性连接", ":/mMainWidgets/Icon/Connector/rouxinglianjie.png", "柔性连接。");
            createbtn4->setIconSize(QSize(30, 30));
            connect(createbtn4, &QToolButton::clicked, this, [this]() {
                slot_createConnector(MxFunctions::ConnectorType::Connector_Flexible);
                });

            QToolButton* createbtn5 = mGW1->addToolBtn("MPC约束", ":/mMainWidgets/Icon/Connector/duodianyueshu.png", 0, 3, 1, 1, QSize(30, 30));
            setQToolButton(createbtn5, "MPC约束", ":/mMainWidgets/Icon/Connector/duodianyueshu.png", "MPC约束。");
            createbtn5->setIconSize(QSize(30, 30));
            connect(createbtn5, &QToolButton::clicked, this, [this]() {
                slot_createConnector(MxFunctions::ConnectorType::Connector_MPC);
                });

            QToolButton* createbtn7 = mGW1->addToolBtn("离散刚体", ":/mMainWidgets/Icon/Connector/lisangangti.png", 0, 4, 1, 1, QSize(30, 30));
            setQToolButton(createbtn7, "离散刚体", ":/mMainWidgets/Icon/Connector/lisangangti.png", "离散刚体。");
            createbtn7->setIconSize(QSize(30, 30));
            connect(createbtn7, &QToolButton::clicked, this, [this]() {
                slot_createConnector(MxFunctions::ConnectorType::Connector_Disperse);
                });

            QToolButton* createbtn8 = mGW1->addToolBtn("螺栓约束", ":/mMainWidgets/Icon/Connector/luoshuanyueshu.png", 0, 5, 1, 1, QSize(30, 30));
            setQToolButton(createbtn8, "螺栓约束", ":/mMainWidgets/Icon/Connector/luoshuanyueshu.png", "螺栓约束。");
            createbtn8->setIconSize(QSize(30, 30));
            connect(createbtn8, &QToolButton::clicked, this, [this]() {
                slot_createConnector(MxFunctions::ConnectorType::Connector_Bolt);
                });

            QToolButton* createbtn11 = mGW1->addToolBtn("解析刚体", ":/mMainWidgets/Icon/Connector/jiexigangti.png", 0, 6, 1, 1, QSize(30, 30));
            setQToolButton(createbtn11, "解析刚体", ":/mMainWidgets/Icon/Connector/jiexigangti.png", "解析刚体。");
            createbtn11->setIconSize(QSize(30, 30));
            connect(createbtn11, &QToolButton::clicked, this, [this]() {
                slot_createConnector(MxFunctions::ConnectorType::Connector_Rigidbody);
                });

            QToolButton* createbtn9 = mGW1->addToolBtn("分布式耦合", ":/mMainWidgets/Icon/Connector/fenbushiouhelianjie.png", 0, 7, 1, 1, QSize(30, 30));
            setQToolButton(createbtn9, "分布式耦合", ":/mMainWidgets/Icon/Connector/fenbushiouhelianjie.png", "分布式耦合连接。");
            createbtn9->setIconSize(QSize(30, 30));
            connect(createbtn9, &QToolButton::clicked, this, [this]() {
                slot_createConnector(MxFunctions::ConnectorType::Connector_DistributedCoupling);
                });

            QToolButton* createbtn10 = mGW1->addToolBtn("轴承单元", ":/mMainWidgets/Icon/Connector/zhouchengdanyuan.png", 0, 8, 1, 1, QSize(30, 30));
            createbtn10->setIconSize(QSize(30, 30));
            setQToolButton(createbtn10, "轴承单元", ":/mMainWidgets/Icon/Connector/zhouchengdanyuan.png", "轴承单元。");

            connect(createbtn10, &QToolButton::clicked, this, [this]() {
                slot_createConnector(MxFunctions::ConnectorType::Connector_BearingUnits);
                });

            QToolButton* createbtn6 = mGW1->addToolBtn("连接搜索", ":/mMainWidgets/Icon/Connector/lianjiesousuo.png", 0, 9, 1, 1, QSize(30, 30));
            createbtn6->setIconSize(QSize(30, 30));
            createbtn6->installEventFilter(new TooltipInstaller("连接搜索",
                QPixmap(":/mMainWidgets/Icon/Connector/lianjiesousuo.png"),
                "", 200, createbtn6));
            connect(createbtn6, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->searchConnector();
                });
        }

        if (mGW2)
        {
            QToolButton* createbtn = mGW2->addToolBtn("连接器", ":/mMainWidgets/Icon/Connector/lianjieqi.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(createbtn, "连接器", ":/mMainWidgets/Icon/Connector/lianjieqi.png", "连接器。");
            createbtn->setIconSize(QSize(30, 30));
            connect(createbtn, &QToolButton::clicked, this, [this]() {
                slot_createDeputy(MxFunctions::DeputyType::Deputy_Connector);
                });

            QToolButton* createbtn1 = mGW2->addToolBtn("fastener", ":/mMainWidgets/Icon/Connector/fastener.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(createbtn1, "fastener", ":/mMainWidgets/Icon/Connector/fastener.png", "fastener。");
            createbtn1->setIconSize(QSize(30, 30));
            connect(createbtn1, &QToolButton::clicked, this, [this]() {
                slot_createDeputy(MxFunctions::DeputyType::Deputy_Fastener);
                });
        }

        if (mGW3)
        {
            auto makeVLine = []() {
                QFrame* line = new QFrame();
                line->setFrameShape(QFrame::VLine);
                line->setFrameShadow(QFrame::Plain);
                line->setStyleSheet("color: #000000; background-color: #000000; width: 0.5px;");
                return line;
                };

            QToolButton* createbtn = mGW3->addToolBtn("面面接触", ":/mMainWidgets/Icon/Connector/mianmianjiechu.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(createbtn, "面面接触", ":/mMainWidgets/Icon/Connector/mianmianjiechu.png", "面面接触。");
            createbtn->setIconSize(QSize(30, 30));
            connect(createbtn, &QToolButton::clicked, this, [this]() {
                slot_createContact(SurfaceToSurfaceContact);
                });

            QToolButton* createbtn2D = mGW3->addToolBtn("二维接触", ":/mMainWidgets/Icon/Connector/mianmianjiechu.png", 0, 6, 1, 1, QSize(30, 30));
            setQToolButton(createbtn2D, "二维接触", ":/mMainWidgets/Icon/Connector/mianmianjiechu.png", "二维接触。");
            createbtn2D->setIconSize(QSize(30, 30));
            connect(createbtn2D, &QToolButton::clicked, this, [this]() {
                slot_createContact(TwoDimensionalContact);
                });

            QToolButton* createbtn2 = mGW3->addToolBtn("点面接触", ":/mMainWidgets/Icon/Connector/dianmianjiechu.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(createbtn2, "点面接触", ":/mMainWidgets/Icon/Connector/dianmianjiechu.png", "点面接触。");
            createbtn2->setIconSize(QSize(30, 30));
            connect(createbtn2, &QToolButton::clicked, this, [this]() {
                slot_createContact(NodeToSurfaceContact);
                });

            QToolButton* createbtn3 = mGW3->addToolBtn("通用接触", ":/mMainWidgets/Icon/Connector/tongyongjiechu.png", 0, 3, 1, 1, QSize(30, 30));
            setQToolButton(createbtn3, "通用接触", ":/mMainWidgets/Icon/Connector/tongyongjiechu.png", "通用接触。");
            createbtn3->setIconSize(QSize(30, 30));
            connect(createbtn3, &QToolButton::clicked, this, [this]() {
                slot_createContact(CommonContact);
                });

            QToolButton* createbtn4 = mGW3->addToolBtn("自接触", ":/mMainWidgets/Icon/Connector/zijiechu.png", 0, 4, 1, 1, QSize(30, 30));
            setQToolButton(createbtn4, "自接触", ":/mMainWidgets/Icon/Connector/zijiechu.png", "自接触。");
            createbtn4->setIconSize(QSize(30, 30));
            connect(createbtn4, &QToolButton::clicked, this, [this]() {
                slot_createContact(SelfContact);
                });

            QToolButton* createbtn6 = mGW3->addToolBtn("过盈接触", ":/mMainWidgets/Icon/Connector/guoyingjiechu.png", 0, 5, 1, 1, QSize(30, 30));
            setQToolButton(createbtn6, "过盈接触", ":/mMainWidgets/Icon/Connector/guoyingjiechu.png", "过盈接触。");
            createbtn6->setIconSize(QSize(30, 30));
            connect(createbtn6, &QToolButton::clicked, this, [this]() {
                slot_createContact(InterferenceContact);
                });

            QToolButton* createbtnLine = mGW3->addToolBtn("线面接触", ":/mMainWidgets/Icon/Connector/dianmianjiechu.png", 0, 2, 1, 1, QSize(30, 30));
            setQToolButton(createbtnLine, "线面接触", ":/mMainWidgets/Icon/Connector/dianmianjiechu.png", "线面接触。");
            createbtnLine->setIconSize(QSize(30, 30));
            connect(createbtnLine, &QToolButton::clicked, this, [this]() {
                slot_createContact(LineToSurfaceContact);
                });

            //mGW3->addWidget(makeVLine(), 0, 3, 2, 1);

            QToolButton* createbtn5 = mGW3->addToolBtn("接触对搜索", ":/mMainWidgets/Icon/Connector/jiechuduisousuo.png", 0, 7, 1, 1, QSize(30, 30));
            setQToolButton(createbtn5, "接触对搜索", ":/mMainWidgets/Icon/Connector/jiechuduisousuo.png", "接触对搜索。");
            createbtn5->setIconSize(QSize(30, 30));
            connect(createbtn5, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->editContactPairPanel();
                });
        }

        if (mGW5)
        {
            QToolButton* createbtn = mGW5->addToolBtn("单元生死", ":/mMainWidgets/Icon/load/单元生死_大@3x.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(createbtn, "单元生死", ":/mMainWidgets/Icon/load/单元生死_大@3x.png", "单元生死。");
            createbtn->setIconSize(QSize(30, 30));
            connect(createbtn, &QToolButton::clicked, this, [this]() {
                slot_createElementSetCtrl();
                });
        }

        mGroupWidget* mGW4 = this->addGroup("相互作用", "周期性边界");
        if (mGW4)
        {
            QToolButton* createbtnPB1 = mGW4->addToolBtn("平移边界", ":/mMainWidgets/Icon/Connector/pingyibianjie.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(createbtnPB1, "平移边界", ":/mMainWidgets/Icon/Connector/pingyibianjie.png", "创建平移周期性边界。");
            createbtnPB1->setIconSize(QSize(30, 30));
            connect(createbtnPB1, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createPeriodicBoundary(MxFunctions::PeriodicBoundary_Translation);
                });

            QToolButton* createbtnPB2 = mGW4->addToolBtn("旋转边界", ":/mMainWidgets/Icon/Connector/xuanzhuanbianjie.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(createbtnPB2, "旋转边界", ":/mMainWidgets/Icon/Connector/xuanzhuanbianjie.png", "创建旋转周期性边界。");
            createbtnPB2->setIconSize(QSize(30, 30));
            connect(createbtnPB2, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createPeriodicBoundary(MxFunctions::PeriodicBoundary_Rotation);
                });
        }
    }

    void mMenuWidget::initMaterialMenu()
    {
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("材料", tempWid);

        mGroupWidget* mGW1 = this->addGroup("材料", "线弹性");
        mGroupWidget* mGW2 = this->addGroup("材料", "弹塑性");
        mGroupWidget* mGW3 = this->addGroup("材料", "超弹性");
        mGroupWidget* mGW4 = this->addGroup("材料", "粘性");
        mGroupWidget* mGW5 = this->addGroup("材料", "热学");
        mGroupWidget* mGW6 = this->addGroup("材料", "声学");
        mGroupWidget* mGW7 = this->addGroup("材料", "刚体");
        mGroupWidget* mGW8 = this->addGroup("材料", "其他");
        mGroupWidget* mGW9 = this->addGroup("材料", "复合材料");
        mGroupWidget* mGW = this->addGroup("材料", "材料库");

        if (mGW)
        {

            QToolButton* meterialbtn = mGW->addToolBtn("工程材料库", ":/mMainWidgets/Icon/MaterialWindow/cailiaoku3x", 0, 0, 1, 1, QSize(16, 16));
            meterialbtn->setFixedSize(70, 60);
            meterialbtn->setIconSize(QSize(32, 32));
            meterialbtn->installEventFilter(new TooltipInstaller("工程材料库",
                QPixmap(":/mMainWidgets/Icon/MaterialWindow/cailiaoku3x"),
                "调出工程材料库窗口，选择\n合适的材料。", 200, meterialbtn));
            connect(meterialbtn, &QToolButton::clicked, this, &mMenuWidget::slot_openMaterial);

            QToolButton* customMaterialBtn = mGW->addToolBtn("自定义材料库", ":/mMainWidgets/Icon/MaterialWindow/zidingyicailiaoku@3x.png", 0, 1, 1, 1, QSize(16, 16));
            customMaterialBtn->setFixedSize(90, 60);
            customMaterialBtn->setIconSize(QSize(32, 32));
            customMaterialBtn->installEventFilter(new TooltipInstaller("自定义材料库",
                QPixmap(":/mMainWidgets/Icon/MaterialWindow/cailiaoku3x"),
                "调出自定义材料库窗口。", 200, customMaterialBtn));
            connect(customMaterialBtn, &QToolButton::clicked, this, &mMenuWidget::slot_openCustomMaterial);
        }

        if (mGW1)
        {
            QToolButton* btn1 = mGW1->addToolBtn("各向同性", ":/mMainWidgets/Icon/mat/各向同性@3x.png", 0, 1, 2, 1, QSize(30, 30));
            setQToolButton(btn1, "各向同性", ":/mMainWidgets/Icon/mat/各向同性@3x.png", "创建各向同性线弹性材料。");
            connect(btn1, &QToolButton::clicked, this, [this]() {
                createMaterial(LinearElastic_Isotropic);
                });


            QToolButton* btn2 = mGW1->addToolBtn("正交异性", "", 0, 2, 1, 1, QSize(20, 20));
            setQToolButton(btn2, "正交异性", "", "创建正交各向异性线弹性材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn2, &QToolButton::clicked, this, [this]() {
                createMaterial(LinearElastic_Orthotropic);
                });

            QToolButton* btn3 = mGW1->addToolBtn("各向异性", "", 1, 2, 1, 1, QSize(20, 20));
            setQToolButton(btn3, "各向异性", "", "创建各向异性线弹性材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn3, &QToolButton::clicked, this, [this]() {
                createMaterial(LinearElastic_Anisotropic);
                });
        }

        if (mGW2)
        {
            QToolButton* btn1 = mGW2->addToolBtn("常规塑性", ":/mMainWidgets/Icon/mat/常规塑性@3x.png", 0, 1, 2, 1, QSize(30, 30));
            setQToolButton(btn1, "常规塑性", ":/mMainWidgets/Icon/mat/常规塑性@3x.png", "创建常规塑性材料。");
            connect(btn1, &QToolButton::clicked, this, [this]() {
                createMaterial(Plastic_Normal);
                });


            QToolButton* btn2 = mGW2->addToolBtn("循环塑性", "", 0, 2, 1, 1, QSize(20, 20));
            setQToolButton(btn2, "循环塑性", "", "创建循环塑性材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn2, &QToolButton::clicked, this, [this]() {
                createMaterial(Plastic_Cyclic);
                });

            QToolButton* btn3 = mGW2->addToolBtn("变形塑性", "", 1, 2, 1, 1, QSize(20, 20));
            setQToolButton(btn3, "变形塑性", "", "创建变形塑性材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn3, &QToolButton::clicked, this, [this]() {
                createMaterial(Plastic_Damage);
                });
        }

        if (mGW3)
        {
            QToolButton* btn1 = mGW3->addToolBtn("Mooney-Rivlin", ":/mMainWidgets/Icon/mat/Mooney-Rivlin@3x.png", 0, 1, 2, 1, QSize(30, 30));
            setQToolButton(btn1, "Mooney-Rivlin", ":/mMainWidgets/Icon/mat/Mooney-Rivlin@3x.png", "创建Mooney-Rivlin超弹性材料。");
            connect(btn1, &QToolButton::clicked, this, [this]() {
                createMaterial(HyperElastic_MooneyRivlin);
                });

            QToolButton* btn2 = mGW3->addToolBtn("Arruda-Boyce", "", 0, 2, 1, 1, QSize(20, 20));
            setQToolButton(btn2, "Arruda-Boyce", "", "创建Arruda-Boyce超弹性材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn2, &QToolButton::clicked, this, [this]() {
                createMaterial(HyperElastic_ArrudaBoyce);
                });

            QToolButton* btn3 = mGW3->addToolBtn("Neo-Hookean", "", 1, 2, 1, 1, QSize(20, 20));
            setQToolButton(btn3, "Neo-Hookean", "", "创建Neo-Hookean超弹性材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn3, &QToolButton::clicked, this, [this]() {
                createMaterial(HyperElastic_NeoHookean);
                });


            QToolButton* btn4 = mGW3->addToolBtn("Ogden", "", 0, 3, 1, 1, QSize(20, 20));
            setQToolButton(btn4, "Ogden", "", "创建Ogden超弹性材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn4, &QToolButton::clicked, this, [this]() {
                createMaterial(HyperElastic_Ogden);
                });

            QToolButton* btn5 = mGW3->addToolBtn("Yeoh", "", 1, 3, 1, 1, QSize(20, 20));
            setQToolButton(btn5, "Yeoh", "", "创建Yeoh超弹性材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn5, &QToolButton::clicked, this, [this]() {
                createMaterial(HyperElastic_Yeoh);
                });
        }

        if (mGW4)
        {
            QToolButton* btn1 = mGW4->addToolBtn("粘弹性", "", 0, 1, 1, 1, QSize(20, 20));
            setQToolButton(btn1, "粘弹性", "", "创建粘弹性材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn1, &QToolButton::clicked, this, [this]() {
                createMaterial(Viscous_Viscoelastic);
                });

            QToolButton* btn2 = mGW4->addToolBtn("粘塑性", "", 1, 1, 1, 1, QSize(20, 20));
            setQToolButton(btn2, "粘塑性", "", "创建粘塑性材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn2, &QToolButton::clicked, this, [this]() {
                createMaterial(Viscous_Viscoplastic);
                });
        }

        if (mGW5)
        {
            QToolButton* btn1 = mGW5->addToolBtn("热学材料", ":/mMainWidgets/Icon/mat/热学材料@3x.png", 0, 1, 2, 1, QSize(30, 30));
            setQToolButton(btn1, "热学材料", ":/mMainWidgets/Icon/mat/热学材料@3x.png", "创建热学材料。");
            connect(btn1, &QToolButton::clicked, this, [this]() {
                createMaterial(Thermal_Base);
                });
        }

        if (mGW6)
        {
            QToolButton* btn1 = mGW6->addToolBtn("声学材料", ":/mMainWidgets/Icon/mat/声学材料@3x.png", 0, 1, 2, 1, QSize(30, 30));
            setQToolButton(btn1, "声学材料", ":/mMainWidgets/Icon/mat/声学材料@3x.png", "创建声学材料。");
            connect(btn1, &QToolButton::clicked, this, [this]() {
                createMaterial(AcousticMaterial_Base);
                });
        }

        if (mGW7)
        {
            QToolButton* btn1 = mGW7->addToolBtn("刚体材料", ":/mMainWidgets/Icon/mat/刚体材料@3x.png", 0, 1, 2, 1, QSize(30, 30));
            setQToolButton(btn1, "刚体材料", ":/mMainWidgets/Icon/mat/刚体材料@3x.png", "创建刚体材料。");
            connect(btn1, &QToolButton::clicked, this, [this]() {
                createMaterial(OtherMaterial_Rigid);
                });
            btn1->setEnabled(false);
        }

        if (mGW8)
        {
            QToolButton* btn1 = mGW8->addToolBtn("脆性材料", "", 0, 1, 1, 1, QSize(20, 20));
            setQToolButton(btn1, "脆性材料", "", "创建脆性材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn1, &QToolButton::clicked, this, [this]() {
                createMaterial(OtherMaterial_Brittle);
                });

            QToolButton* btn2 = mGW8->addToolBtn("超弹泡沫", "", 1, 1, 1, 1, QSize(20, 20));
            setQToolButton(btn2, "超弹泡沫", "", "创建超弹泡沫材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn2, &QToolButton::clicked, this, [this]() {
                createMaterial(OtherMaterial_Foam);
                });

            QToolButton* btn3 = mGW8->addToolBtn("垫片材料", "", 0, 2, 1, 1, QSize(20, 20));
            setQToolButton(btn3, "垫片材料", "", "创建垫片材料。", Qt::ToolButtonTextBesideIcon);
            connect(btn3, &QToolButton::clicked, this, [this]() {
                createMaterial(OtherMaterial_Shim);
                });

            QToolButton* btn4 = mGW8->addToolBtn("其他", "", 1, 2, 1, 1, QSize(20, 20));
            setQToolButton(btn4, "其他", "", "", Qt::ToolButtonTextBesideIcon);
            connect(btn4, &QToolButton::clicked, this, [this]() {
                createMaterial(OtherMaterial_Break);
                });
        }

        if (mGW9)
        {
            QToolButton* customMaterialBtn = mGW9->addToolBtn("纤维增强复合材料", ":/mMainWidgets/Icon/mat/xianweizengqiang.png", 0, 1, 1, 1, QSize(16, 16));
            customMaterialBtn->setFixedSize(130, 60);
            customMaterialBtn->setIconSize(QSize(32, 32));
            customMaterialBtn->installEventFilter(new TooltipInstaller("纤维增强复合材料",
                QPixmap(":/mMainWidgets/Icon/mat/xianweizengqiang.png"),
                "调出纤维增强复合材料窗口。", 200, customMaterialBtn));
            connect(customMaterialBtn, &QToolButton::clicked, this, &mMenuWidget::slot_FrpCompMaterial);
        }
    }


    void mMenuWidget::createMaterial(MxFunctions::MaterialType matType)
    {
        mInfoWindow::getInstance()->createMaterial(matType);
    }

    void mMenuWidget::initPropertyMenu()
    {
        // 下方Ribbon工具条上添加相应的显示widget作为Group的载体
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("属性", tempWid);

        std::shared_ptr<mGroupWidget> predefinedFeatureGW;
        std::shared_ptr<mGroupWidget> oneDimGW;
        std::shared_ptr<mGroupWidget> twoDimGW;
        std::shared_ptr<mGroupWidget> threeDimGW;
        std::shared_ptr<mGroupWidget> compositeGW;
        std::shared_ptr<mGroupWidget> eleGW;
        std::shared_ptr<mGroupWidget> checkGW;

        eleGW = std::shared_ptr<mGroupWidget>(this->addGroup("属性", "单元类型"));
        predefinedFeatureGW = std::shared_ptr<mGroupWidget>(this->addGroup("属性", "预定义特征"));
        oneDimGW = std::shared_ptr<mGroupWidget>(this->addGroup("属性", "一维截面"));
        twoDimGW = std::shared_ptr<mGroupWidget>(this->addGroup("属性", "二维截面"));
        threeDimGW = std::shared_ptr<mGroupWidget>(this->addGroup("属性", "三维截面"));
        compositeGW = std::shared_ptr<mGroupWidget>(this->addGroup("属性", "复材截面"));
        checkGW = std::shared_ptr<mGroupWidget>(this->addGroup("属性", "检查"));

        _propertyGW.append(predefinedFeatureGW);
        _propertyGW.append(oneDimGW);
        _propertyGW.append(twoDimGW);
        _propertyGW.append(threeDimGW);
        _propertyGW.append(compositeGW);
        _propertyGW.append(eleGW);
        _propertyGW.append(checkGW);

        // 按钮尺寸由主题档位统一计算，同列小按钮由功能组按最大内容宽度对齐

        if (predefinedFeatureGW)
        {
            auto beamSectionBtn(predefinedFeatureGW->addToolBtn("梁截面", ":/mMainWidgets/Icon/Property/jiemianxinzhuang.png", 0, 0, 1, 1, QSize(30, 30)));
            setQToolButton(beamSectionBtn, "梁截面", ":/mMainWidgets/Icon/Property/jiemianxinzhuang.png", "创建梁截面形状。\n", Qt::ToolButtonTextUnderIcon);
            beamSectionBtn->setObjectName("createBtn");
            connect(beamSectionBtn, &QToolButton::pressed, this, [this, beamSectionBtn]() {
                m_menuTabBar_->setTabText(1, beamSectionBtn->text());
                slot_CurrentTabChanged(1);
                });

            auto singleLayerBtn(predefinedFeatureGW->addToolBtn("单层板", ":/mMainWidgets/Icon/BrowserTree/dancengban.png", 0, 1, 1, 1, QSize(30, 30)));
            setQToolButton(singleLayerBtn, "单层板", ":/mMainWidgets/Icon/BrowserTree/dancengban.png", "创建单层板。\n", Qt::ToolButtonTextUnderIcon);
            connect(singleLayerBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->creatLaminate(SglLayerPlate);
                });

            auto globalSingleLayerBtn(predefinedFeatureGW->addToolBtn("全局单层板", ":/mMainWidgets/Icon/BrowserTree/quanjudancengban.png", 0, 2, 1, 1, QSize(30, 30)));
            setQToolButton(globalSingleLayerBtn, "全局单层板", ":/mMainWidgets/Icon/BrowserTree/quanjudancengban.png", "创建全局单层板。\n", Qt::ToolButtonTextUnderIcon);
            connect(globalSingleLayerBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->creatLaminate(GlobalSglLam);
                });

            auto laminateBtn(predefinedFeatureGW->addToolBtn("层合板", ":/mMainWidgets/Icon/BrowserTree/cengheban.png", 0, 3, 1, 1, QSize(30, 30)));
            setQToolButton(laminateBtn, "层合板", ":/mMainWidgets/Icon/BrowserTree/cengheban.png", "创建层合板。\n", Qt::ToolButtonTextUnderIcon);
            connect(laminateBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->creatLaminate(LamPlate);
                });
        }

        if (oneDimGW)
        {
            auto rodBtn(oneDimGW->addToolBtn("杆", ":/mMainWidgets/Icon/Property/gan.png", 0, 0, 1, 1, QSize(30, 30)));
            setQToolButton(rodBtn, "杆", ":/mMainWidgets/Icon/Property/gan.png", "创建杆截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(rodBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(RodProperty);
                });

            auto pipeBtn(oneDimGW->addToolBtn("管", ":/mMainWidgets/Icon/Property/guan.png", 0, 1, 1, 1, QSize(30, 30)));
            setQToolButton(pipeBtn, "管", ":/mMainWidgets/Icon/Property/guan.png", "创建管截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(pipeBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(TubeProperty);
                });

            auto beamBtn(oneDimGW->addToolBtn("梁", ":/mMainWidgets/Icon/Property/changjiemianliang.png", 0, 2, 1, 1, QSize(30, 30)));
            setQToolButton(beamBtn, "梁", ":/mMainWidgets/Icon/Property/changjiemianliang.png", "创建梁截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(beamBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(ResultantProperty);
                });

            auto springBtn(oneDimGW->addToolBtn("弹簧", ":/mMainWidgets/Icon/Property/tanhuang.png", 0, 3, 1, 1, QSize(30, 30)));
            setQToolButton(springBtn, "弹簧", ":/mMainWidgets/Icon/Property/tanhuang.png", "创建弹簧截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(springBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(Elas1Property);
                });

            auto damperBtn(oneDimGW->addToolBtn("阻尼器", ":/mMainWidgets/Icon/Property/zuni.png", 0, 4, 1, 1, QSize(30, 30)));
            setQToolButton(damperBtn, "阻尼器", ":/mMainWidgets/Icon/Property/zuni.png", "创建阻尼器截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(damperBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(ViscousDampProperty);
                });

            auto springDamperBtn(oneDimGW->addToolBtn("弹簧阻尼", ":/mMainWidgets/Icon/Property/tanhaungzuni.png", 0, 5, 1, 1, QSize(30, 30)));
            setQToolButton(springDamperBtn, "弹簧阻尼", ":/mMainWidgets/Icon/Property/tanhaungzuni.png", "创建弹簧阻尼截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(springDamperBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(CbushProperty);
                });

            auto gapuniBtn(oneDimGW->addToolBtn("间隙", ":/mMainWidgets/Icon/Property/jianxi.png", 0, 6, 1, 1, QSize(30, 30)));
            setQToolButton(gapuniBtn, "间隙", ":/mMainWidgets/Icon/Property/jianxi.png", "创建点对点间隙截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(gapuniBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(GapuniProperty);
                });

            //oneDimGW->updateLayout();
        }

        if (twoDimGW)
        {
            auto planeBtn(twoDimGW->addToolBtn("平面应力", ":/mMainWidgets/Icon/Property/pinmianyingli.png", 0, 0, 1, 1, QSize(30, 30)));
            setQToolButton(planeBtn, "平面应力", ":/mMainWidgets/Icon/Property/pinmianyingli.png", "创建平面应力截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(planeBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(PlaneStressProperty);
                });

            auto planeStrainBtn(twoDimGW->addToolBtn("平面应变", ":/mMainWidgets/Icon/Property/pinmianyingbian.png", 0, 1, 1, 1, QSize(30, 30)));
            setQToolButton(planeStrainBtn, "平面应变", ":/mMainWidgets/Icon/Property/pinmianyingbian.png", "创建平面应变截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(planeStrainBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(PlaneStrainProperty);
                });

            //auto membraneBtn(twoDimGW->addToolBtn("膜", ":/mMainWidgets/Icon/Property/mo.png", 0, 2, 1, 1, QSize(30, 30)));
            //setQToolButton(membraneBtn, "膜", ":/mMainWidgets/Icon/Property/mo.png", "创建膜截面属性。\n", Qt::ToolButtonTextUnderIcon);
            //connect(membraneBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(MembraneProperty);
            //	});

            auto plateBtn(twoDimGW->addToolBtn("板", ":/mMainWidgets/Icon/Property/jianqieban.png", 0, 2, 1, 1, QSize(30, 30)));
            setQToolButton(plateBtn, "板", ":/mMainWidgets/Icon/Property/jianqieban.png", "创建板截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(plateBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(ShearPlateProperty);
                });

            auto shellBtn(twoDimGW->addToolBtn("壳", ":/mMainWidgets/Icon/Property/ke.png", 0, 3, 1, 1, QSize(30, 30)));
            setQToolButton(shellBtn, "壳", ":/mMainWidgets/Icon/Property/ke.png", "创建壳截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(shellBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(ShellProperty);
                });

            //twoDimGW->updateLayout();
        }

        if (threeDimGW)
        {
            auto solidBtn(threeDimGW->addToolBtn("实体", ":/mMainWidgets/Icon/Property/3Dshiti.png", 0, 0, 1, 1, QSize(30, 30)));
            setQToolButton(solidBtn, "实体", ":/mMainWidgets/Icon/Property/3Dshiti.png", "创建实体截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(solidBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(SolidProperty);
                });

            auto solidShellBtn(threeDimGW->addToolBtn("三维实体壳", ":/mMainWidgets/Icon/Property/3Dshitiqiao.png", 0, 1, 1, 1, QSize(30, 30)));
            setQToolButton(solidShellBtn, "三维实体壳", ":/mMainWidgets/Icon/Property/3Dshitiqiao.png", "创建三维实体壳截面属性。\n", Qt::ToolButtonTextUnderIcon);
            solidShellBtn->setVisible(false);
            connect(solidShellBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(SolidShellProperty);
                });

            auto gasketBtn(threeDimGW->addToolBtn("垫片", ":/mMainWidgets/Icon/Property/dianpian.png", 0, 2, 1, 1, QSize(30, 30)));
            setQToolButton(gasketBtn, "垫片", ":/mMainWidgets/Icon/Property/dianpian.png", "创建垫片截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(gasketBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(GasketProperty);
                });

            //threeDimGW->updateLayout();
        }


        if (compositeGW)
        {
            auto layerBtn(compositeGW->addToolBtn("铺层复材", ":/mMainWidgets/Icon/Property/pucen.png", 0, 0, 1, 1, QSize(30, 30)));
            setQToolButton(layerBtn, "铺层复材", ":/mMainWidgets/Icon/Property/pucen.png", "创建铺层复材截面属性。\n", Qt::ToolButtonTextUnderIcon);
            connect(layerBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createProperty(PlyLaminateProperty);
                });
        }

        if (eleGW)
        {
            auto zeroDimEleBtn(eleGW->addToolBtn("点单元", ":/mMainWidgets/Icon/Property/dian.png", 0, 0, 1, 1, QSize(30, 30)));
            setQToolButton(zeroDimEleBtn, "点单元", ":/mMainWidgets/Icon/Property/dian.png", "点单元的类型设置。\n");
            connect(zeroDimEleBtn, &QToolButton::pressed, this, [this, zeroDimEleBtn]() {mInfoWindow::getInstance()->editElement(zeroDimEleBtn->text());
                });

            auto oneDimEleBtn(eleGW->addToolBtn("线单元", ":/mMainWidgets/Icon/Property/xian.png", 0, 1, 1, 1, QSize(30, 30)));
            setQToolButton(oneDimEleBtn, "线单元", ":/mMainWidgets/Icon/Property/xian.png", "线单元的类型设置。\n");
            connect(oneDimEleBtn, &QToolButton::pressed, this, [this, oneDimEleBtn]() {mInfoWindow::getInstance()->editElement(oneDimEleBtn->text());
                });

            auto twoDimEleBtn(eleGW->addToolBtn("面单元", ":/mMainWidgets/Icon/Property/mian.png", 0, 2, 1, 1, QSize(30, 30)));
            setQToolButton(twoDimEleBtn, "面单元", ":/mMainWidgets/Icon/Property/mian.png", "面单元的类型设置。\n");
            connect(twoDimEleBtn, &QToolButton::pressed, this, [this, twoDimEleBtn]() {mInfoWindow::getInstance()->editElement(twoDimEleBtn->text());
                });

            auto pipeBtn(eleGW->addToolBtn("体单元", ":/mMainWidgets/Icon/Property/ti.png", 0, 3, 1, 1, QSize(30, 30)));
            setQToolButton(pipeBtn, "体单元", ":/mMainWidgets/Icon/Property/ti.png", "体单元的类型设置。\n");
            connect(pipeBtn, &QToolButton::pressed, this, [this, pipeBtn]() {mInfoWindow::getInstance()->editElement(pipeBtn->text());
                });

            auto specialEleBtn(eleGW->addToolBtn("特殊单元", ":/mMainWidgets/Icon/Property/sunshangdanyuan.png", 0, 4, 1, 1, QSize(30, 30)));
            setQToolButton(specialEleBtn, "特殊单元", ":/mMainWidgets/Icon/Property/sunshangdanyuan.png", "特殊单元的类型设置。\n");
            connect(specialEleBtn, &QToolButton::pressed, this, [this, specialEleBtn]() {mInfoWindow::getInstance()->editElement(specialEleBtn->text());
                });

            auto btn(eleGW->addToolBtn("低阶高精度", ":/mMainWidgets/Icon/Property/dijiegaojingdu.png", 0, 5, 1, 1, QSize(30, 30)));
            setQToolButton(btn, "低阶高精度", ":/mMainWidgets/Icon/Property/dijiegaojingdu.png", "低阶高精度单元的类型设置。\n");
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mInfoWindow::getInstance()->editElement(btn->text());
                });
        }
        /*
        if (oneDimEleGW)
        {
            auto rodBtn(oneDimEleGW->addToolBtn("杆", ":/mMainWidgets/Icon/Property/gan.png", 0, 0, 1, 1, QSize(20, 20)));
            setQToolButton(rodBtn, "杆", ":/mMainWidgets/Icon/Property/gan.png", "杆单元赋予。\n", Qt::ToolButtonTextBesideIcon);
            connect(rodBtn, &QToolButton::pressed, this, [this, rodBtn]() {mInfoWindow::getInstance()->editElement(rodBtn->text());
                });

            auto beamBtn(oneDimEleGW->addToolBtn("梁", ":/mMainWidgets/Icon/Property/changjiemianliang.png", 1, 0, 1, 1, QSize(20, 20)));
            setQToolButton(beamBtn, "梁", ":/mMainWidgets/Icon/Property/changjiemianliang.png", "梁单元赋予。\n", Qt::ToolButtonTextBesideIcon);
            connect(beamBtn, &QToolButton::pressed, this, [this, beamBtn]() {mInfoWindow::getInstance()->editElement(beamBtn->text());
                });

            auto pipeBtn(oneDimEleGW->addToolBtn("管", ":/mMainWidgets/Icon/Property/guan.png", 0, 1, 2, 1, QSize(30, 30)));
            setQToolButton(pipeBtn, "管", ":/mMainWidgets/Icon/Property/guan.png", "管单元赋予。\n");
            connect(pipeBtn, &QToolButton::pressed, this, [this, pipeBtn]() {mInfoWindow::getInstance()->editElement(pipeBtn->text());
                });
        }


        if (twoDimEleGW)
        {
            auto planeStressBtn(twoDimEleGW->addToolBtn("平面应力", ":/mMainWidgets/Icon/Property/pinmianyingli.png", 0, 0, 1, 1, QSize(20, 20)));
            setQToolButton(planeStressBtn, "平面应力", ":/mMainWidgets/Icon/Property/pinmianyingli.png", "平面应力单元赋予。\n", Qt::ToolButtonTextBesideIcon);
            connect(planeStressBtn, &QToolButton::pressed, this, [this, planeStressBtn]() {mInfoWindow::getInstance()->editElement(planeStressBtn->text());
                });

            auto planeStrainBtn(twoDimEleGW->addToolBtn("平面应变", ":/mMainWidgets/Icon/Property/pinmianyingbian.png", 1, 0, 1, 1, QSize(20, 20)));
            setQToolButton(planeStrainBtn, "平面应变", ":/mMainWidgets/Icon/Property/pinmianyingbian.png", "平面应变单元赋予。\n", Qt::ToolButtonTextBesideIcon);
            connect(planeStrainBtn, &QToolButton::pressed, this, [this, planeStrainBtn]() {mInfoWindow::getInstance()->editElement(planeStrainBtn->text());
                });

            auto shellBtn(twoDimEleGW->addToolBtn("壳", ":/mMainWidgets/Icon/Property/ke.png", 0, 1, 1, 1, QSize(20, 20)));
            setQToolButton(shellBtn, "壳", ":/mMainWidgets/Icon/Property/ke.png", "壳单元赋予。\n", Qt::ToolButtonTextBesideIcon);
            connect(shellBtn, &QToolButton::pressed, this, [this, shellBtn]() {mInfoWindow::getInstance()->editElement(shellBtn->text());
                });

            auto membraneBtn(twoDimEleGW->addToolBtn("膜", ":/mMainWidgets/Icon/Property/mo.png", 1, 1, 1, 1, QSize(20, 20)));
            setQToolButton(membraneBtn, "膜", ":/mMainWidgets/Icon/Property/mo.png", "膜单元赋予。\n", Qt::ToolButtonTextBesideIcon);
            membraneBtn->setEnabled(false);

            auto shearPlateBtn(twoDimEleGW->addToolBtn("剪切板", ":/mMainWidgets/Icon/Property/jianqieban.png", 0, 2, 2, 1, QSize(30, 30)));
            setQToolButton(shearPlateBtn, "剪切板", ":/mMainWidgets/Icon/Property/jianqieban.png", "剪切板单元赋予。\n");
            connect(shearPlateBtn, &QToolButton::pressed, this, [this, shearPlateBtn]() {mInfoWindow::getInstance()->editElement(shearPlateBtn->text());
                });

            //twoDimEleGW->updateLayout();
        }

        if (threeDimEleGW)
        {
            auto solidBtn(threeDimEleGW->addToolBtn("三维实体", ":/mMainWidgets/Icon/Property/3Dshiti.png", 0, 0, 1, 1, QSize(20, 20)));
            setQToolButton(solidBtn, "三维实体", ":/mMainWidgets/Icon/Property/3Dshiti.png", "三维实体单元赋予。\n", Qt::ToolButtonTextBesideIcon);
            connect(solidBtn, &QToolButton::pressed, this, [this, solidBtn]() {mInfoWindow::getInstance()->editElement(solidBtn->text());
                });

            auto solidShellBtn(threeDimEleGW->addToolBtn("三维实体壳", ":/mMainWidgets/Icon/Property/3Dshitiqiao.png", 1, 0, 1, 1, QSize(20, 20)));
            setQToolButton(solidShellBtn, "三维实体壳", ":/mMainWidgets/Icon/Property/3Dshitiqiao.png", "三维实体壳单元赋予。\n", Qt::ToolButtonTextBesideIcon);
            connect(solidShellBtn, &QToolButton::pressed, this, [this, solidShellBtn]() {mInfoWindow::getInstance()->editElement(solidShellBtn->text());
                });
        }

        if (specialEleGW)
        {
            auto btn(specialEleGW->addToolBtn("低阶高精度", ":/mMainWidgets/Icon/Property/dijiegaojingdu.png", 0, 0, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "低阶高精度", ":/mMainWidgets/Icon/Property/dijiegaojingdu.png", "低阶高精度单元赋予。\n", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mInfoWindow::getInstance()->editElement(btn->text());
                });
            btn = (specialEleGW->addToolBtn("轴对称", ":/mMainWidgets/Icon/Property/zhouduichengdanyuan.png", 1, 0, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "轴对称", ":/mMainWidgets/Icon/Property/zhouduichengdanyuan.png", "轴对称单元赋予。\n", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mInfoWindow::getInstance()->editElement(btn->text());
                });

            btn = (specialEleGW->addToolBtn("损伤单元", ":/mMainWidgets/Icon/Property/sunshangdanyuan.png", 0, 1, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "损伤单元", ":/mMainWidgets/Icon/Property/sunshangdanyuan.png", "损伤单元赋予。\n");
            btn->setMinimumWidth(60);  // 设置最小宽度
            btn->setMinimumHeight(50);  // 设置最小高度
            btn->installEventFilter(new TooltipInstaller("损伤单元",
                QPixmap(":/mMainWidgets/Icon/Property/sunshangdanyuan.png"),
                "损伤单元赋予",
                200, btn));
            btn->setEnabled(false);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mInfoWindow::getInstance()->editElement(btn->text());
                });

            //twoDimEleGW->updateLayout();
        }
        */
        if (checkGW)
        {
            auto propertyBtn(checkGW->addToolBtn("截面属性", ":/mMainWidgets/Icon/Property/jiancha.png", 0, 0, 1, 1, QSize(20, 20)));
            setQToolButton(propertyBtn, "截面属性", ":/mMainWidgets/Icon/Property/jiancha.png", "检查截面属性。\n", Qt::ToolButtonTextBesideIcon);
            connect(propertyBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->checkProperty(0);
                });

            auto eleBtn(checkGW->addToolBtn("单元", ":/mMainWidgets/Icon/Property/jianchadanyuan.png", 1, 0, 1, 1, QSize(20, 20)));
            setQToolButton(eleBtn, "单元", ":/mMainWidgets/Icon/Property/jianchadanyuan.png", "检查单元。\n", Qt::ToolButtonTextBesideIcon);
            connect(eleBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->checkProperty(1);
                });

        }
    }

    void mMenuWidget::initSetMenu()
    {
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("集合", tempWid);

        std::shared_ptr<mGroupWidget> createGW;
        std::shared_ptr<mGroupWidget> convertGW;

        createGW = std::shared_ptr<mGroupWidget>(this->addGroup("集合", "创建"));
        convertGW = std::shared_ptr<mGroupWidget>(this->addGroup("集合", "转换"));

        _setGW.append(createGW);
        _setGW.append(convertGW);


        if (createGW)
        {
            auto btn(createGW->addToolBtn("几何点集", ":/mMainWidgets/Icon/choice/dingdian3x.png", 0, 0, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "几何点集", ":/mMainWidgets/Icon/choice/dingdian3x.png", "创建几何点集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                mMechGlobalSignals::getInstance()->activePickSig(1);
                this->slot_createSet();
                });

            btn = (createGW->addToolBtn("几何线集", ":/mMainWidgets/Icon/choice/bian3x.png", 0, 1, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "几何线集", ":/mMainWidgets/Icon/choice/bian3x.png", "创建几何线集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                mMechGlobalSignals::getInstance()->activePickSig(2);
                this->slot_createSet();
                });

            btn = (createGW->addToolBtn("几何面集", ":/mMainWidgets/Icon/choice/mian3x.png", 0, 2, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "几何面集", ":/mMainWidgets/Icon/choice/mian3x.png", "创建几何面集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                mMechGlobalSignals::getInstance()->activePickSig(3);
                this->slot_createSet();
                });

            btn = (createGW->addToolBtn("几何体集", ":/mMainWidgets/Icon/choice/jiheti3x.png", 0, 3, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "几何体集", ":/mMainWidgets/Icon/choice/jiheti3x.png", "创建几何体集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                mMechGlobalSignals::getInstance()->activePickSig(4);
                this->slot_createSet();
                });

            btn = (createGW->addToolBtn("节点集", ":/mMainWidgets/Icon/choice/jiedian3x.png", 0, 4, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "节点集", ":/mMainWidgets/Icon/choice/jiedian3x.png", "创建节点集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                mMechGlobalSignals::getInstance()->activePickSig(5);
                this->slot_createSet();
                });

            btn = (createGW->addToolBtn("单元边集", ":/mMainWidgets/Icon/choice/danyuanbian3x.png", 0, 5, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "单元边集", ":/mMainWidgets/Icon/choice/danyuanbian3x.png", "创建单元边集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                mMechGlobalSignals::getInstance()->activePickSig(6);
                this->slot_createSet();
                });

            btn = (createGW->addToolBtn("解析表面集", ":/mMainWidgets/Icon/choice/jiexibiaomian3x.png", 0, 6, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "解析表面集", ":/mMainWidgets/Icon/choice/jiexibiaomian3x.png", "创建解析表面集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                mMechGlobalSignals::getInstance()->activePickSig(6);
                this->slot_createSet_withName("解析表面集");
                });

            btn = (createGW->addToolBtn("单元面集", ":/mMainWidgets/Icon/choice/danyuanmian3x.png", 0, 7, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "单元面集", ":/mMainWidgets/Icon/choice/danyuanmian3x.png", "创建单元面集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                mMechGlobalSignals::getInstance()->activePickSig(7);
                this->slot_createSet();
                });

            btn = (createGW->addToolBtn("单元集", ":/mMainWidgets/Icon/choice/danyuan3x.png", 0, 8, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "单元集", ":/mMainWidgets/Icon/choice/danyuan3x.png", "创建单元面集合。");
            btn->setFixedWidth(60);  // 设置最小宽度
            btn->setFixedHeight(50);  // 设置最小高度
            btn->installEventFilter(new TooltipInstaller("单元集",
                QPixmap(":/mMainWidgets/Icon/choice/danyuan3x.png"), "创建单元集合",
                200, btn));
            connect(btn, &QToolButton::pressed, this, [this]() {
                mMechGlobalSignals::getInstance()->activePickSig(8);
                this->slot_createSet();
                });

            btn = (createGW->addToolBtn("部件集", ":/mMainWidgets/Icon/choice/bujian3x.png", 0, 9, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "部件集", ":/mMainWidgets/Icon/choice/bujian3x.png", "创建部件集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                mMechGlobalSignals::getInstance()->activePickSig(9);
                this->slot_createSet();
                });

            btn = (createGW->addToolBtn("频率点集", ":/mMainWidgets/Icon/choice/freqPoint@3x.png", 0, 10, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "频率点集", ":/mMainWidgets/Icon/choice/freqPoint@3x.png", "创建频率点集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                //添加标志位，传递频率点集合名称
                this->slot_createSet_withName("频率点集");
                });

            btn = (createGW->addToolBtn("自由度节点集", ":/mMainWidgets/Icon/choice/dofNode@3x.png", 0, 11, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "自由度节点集", ":/mMainWidgets/Icon/choice/dofNode@3x.png", "创建自由度节点集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                mMechGlobalSignals::getInstance()->activePickSig(6);
                this->slot_createSet_withName("自由度节点集");
                });

            btn = (createGW->addToolBtn("面板集", ":/mMainWidgets/Icon/choice/panel@3x.png", 0, 12, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "面板集", ":/mMainWidgets/Icon/choice/panel@3x.png", "创建面板集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                this->slot_createSet_withName("面板集");
                });

            btn = (createGW->addToolBtn("节点面集", ":/mMainWidgets/Icon/choice/jiedianmian.png", 0, 13, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "节点面集", ":/mMainWidgets/Icon/choice/jiedianmian.png", "创建节点面集合。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                this->slot_createSet_withName("节点面集");
                });
        }

        auto convertFunc = [&](QString targetSetTypestr)
            {
                //转换流程会程序化操纵选择容器与拾取模式（clearPick/映射/装载集合数据），
                //先失活集合面板的通用拾取控件并断开其监听，避免激活的控件把程序化
                //清空视口当作拾取变化提交：误清当前编辑集合的数据、误建空集合
                mInfoWindow::getInstance()->uneditSet();

                auto items = VMManager->getCurrentViewportPrivate()->getTreeSelectItems();
                for (int i = 0; i < items.size(); i++)
                {
                    auto item = items.at(i);
                    int setID = item._phyID;
                    auto setData = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setID);
                    if (setData == nullptr)
                    {
                        return;
                    }
                    //根据集合类型和数据更新面板
                    auto type = setData->getSetType();
                    //auto typetr = setData->getSetTypeStr();
                    QString name;
                    if (setData->getIsUserRename())//用户重命名过
                    {
                        name = setData->getSetName();
                    }
                    else//默认名称，类型更改后自动修改名称
                    {
                        QString EnName = mSetPanel::getEnglishNameByChineseName(targetSetTypestr);
                        QStringList list = setData->getSetName().split("_");
                        if (list.size() == 2)
                        {
                            name = QString("%1_%2").arg(EnName).arg(list.at(1));
                        }
                        else
                        {
                            name = QString("%1_%2").arg(EnName).arg(setData->getSetId());
                        }
                        if (getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).ExistSetName(name))
                        {
                            name = getCVM->getModelData()->createNameByType(Physical_Set, EnName);
                        }
                    }
                    switch (type)
                    {
                    case MxFunctions::GeoPoint:
                    case MxFunctions::GeoEdge:
                    case MxFunctions::GeoFace:
                    case MxFunctions::GeoSolid:
                    {
                        auto data = dynamic_cast<MechSetData::mGeoSet*>(setData);
                        auto ids = data->getGeoSetData();
                        getCVM->clearPick();
                        switch (type)
                        {
                        case MxFunctions::GeoPoint:
                            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPoint, ids); getCVM->getPreRenderWidget()->setPickFilter(PickFilter::PickGeoPoint); break;
                        case MxFunctions::GeoEdge:
                            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, ids); getCVM->getPreRenderWidget()->setPickFilter(PickFilter::PickGeoLine); break;
                        case MxFunctions::GeoFace:
                            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, ids); getCVM->getPreRenderWidget()->setPickFilter(PickFilter::PickGeoFace); break;
                        case MxFunctions::GeoSolid:
                            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoSolid, ids); getCVM->getPreRenderWidget()->setPickFilter(PickFilter::PickGeoSolid); break;
                        }
                        break;
                    }
                    break;
                    case MxFunctions::Element:
                    {
                        auto data = dynamic_cast<MechSetData::mElementSet*>(setData);
                        auto ids = data->getElementSetData();
                        std::vector<MeshSelectInfoData> elesurfaces;
                        for (auto id : ids)
                        {
                            MeshSelectInfoData n;
                            n.data_id.element_id = id.result_id;
                            n.part_id = id.part_id;
                            elesurfaces.push_back(n);
                        }
                        getCVM->clearPick();
                        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, elesurfaces);
                        getCVM->getPreRenderWidget()->setPickFilter(PickFilter::PickMeshElement); break;
                    }
                    break;
                    case MxFunctions::Node:
                    {
                        auto data = dynamic_cast<MechSetData::mNode*>(setData);
                        auto ids = data->getNodeData();
                        std::vector<MeshSelectInfoData> elesurfaces;
                        for (auto id : ids)
                        {
                            MeshSelectInfoData n;
                            n.data_id.element_id = id.result_id;
                            n.part_id = id.part_id;
                            elesurfaces.push_back(n);
                        }
                        getCVM->clearPick();
                        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshNode, elesurfaces);
                        getCVM->getPreRenderWidget()->setPickFilter(PickFilter::PickMeshNode); break;
                    }
                    break;
                    case MxFunctions::Part:
                    {
                        auto data = dynamic_cast<MechSetData::mPartSet*>(setData);
                        auto ids = data->getPartSetData();
                        getCVM->clearPick();
                        MechPreData::addPreWidgetSelectPartID(getCVM->getPreRenderWidget(), ids);
                        getCVM->getPreRenderWidget()->setPickFilter(PickFilter::PickMeshPart | PickFilter::PickGeoPart); break;
                        break;
                    }
                    case MxFunctions::ElementSurface:
                    {
                        auto data = dynamic_cast<MechSetData::mElementSurface*>(setData);
                        auto ids = data->getElementSurfaceData();
                        std::vector<MeshSelectInfoData> elesurfaces;
                        for (auto id : ids)
                        {
                            MeshSelectInfoData n;
                            n.data_id.element_id = id.result_id;
                            n.part_id = id.part_id;
                            elesurfaces.push_back(n);
                        }
                        getCVM->clearPick();
                        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, elesurfaces);
                        getCVM->getPreRenderWidget()->setPickFilter(PickFilter::PickMeshFace); break;
                    }
                    break;
                    case MxFunctions::ElementEdge:
                    {
                        auto data = dynamic_cast<MechSetData::mElementSurface*>(setData);
                        auto ids = data->getElementSurfaceData();
                        std::vector<MeshSelectInfoData> elesurfaces;
                        for (auto id : ids)
                        {
                            MeshSelectInfoData n;
                            n.data_id.element_id = id.result_id;
                            n.part_id = id.part_id;
                            elesurfaces.push_back(n);
                        }
                        getCVM->clearPick();
                        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshLine, elesurfaces);
                        getCVM->getPreRenderWidget()->setPickFilter(PickFilter::PickMeshLine); break;
                    }
                    break;
                    default:
                        break;
                    }

                    getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).deleteSet(setID);

                    if (targetSetTypestr == QString("部件集"))
                    {
                        mSelectMenuFunction::getInstance()->slot_mappingPart(false);
                        auto resids = MechPreData::getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
                        auto eleset = new MechSetData::mPartSet(setID, MxFunctions::SetType::Part, name);
                        eleset->setPartSetData(resids);
                        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                    }
                    else if (targetSetTypestr == QString("节点集"))
                    {
                        mSelectMenuFunction::getInstance()->slot_mappingNode(false);
                        auto resids = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
                        auto eleset = new MechSetData::mNode(setID, MxFunctions::SetType::Node, name);
                        std::vector<RenderPickResultData> elesurfaces;
                        for (auto elesurface : resids)
                        {
                            RenderPickResultData n;
                            n.result_id = elesurface.data_id.element_id;
                            n.result_id2 = elesurface.data_index.face_index;
                            n.part_id = elesurface.part_id;
                            elesurfaces.push_back(n);
                        }
                        eleset->setNodeData(elesurfaces);
                        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                    }
                    else if (targetSetTypestr == QString("单元集"))
                    {
                        mSelectMenuFunction::getInstance()->slot_mappingEle(false);
                        auto resids = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
                        auto eleset = new MechSetData::mElementSet(setID, MxFunctions::SetType::Element, name);
                        std::vector<RenderPickResultData> elesurfaces;
                        for (auto elesurface : resids)
                        {
                            RenderPickResultData n;
                            n.result_id = elesurface.data_id.element_id;
                            n.result_id2 = elesurface.data_index.face_index;
                            n.part_id = elesurface.part_id;
                            elesurfaces.push_back(n);
                        }
                        eleset->setElementSetData(elesurfaces);
                        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                    }
                    else if (targetSetTypestr == QString("单元面集"))
                    {
                        mSelectMenuFunction::getInstance()->slot_mappingMeshFace(false);
                        auto resids = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
                        auto eleset = new MechSetData::mElementSurface(setID, MxFunctions::SetType::ElementSurface, name);
                        std::vector<RenderPickResultData> elesurfaces;
                        for (auto elesurface : resids)
                        {
                            RenderPickResultData n;
                            n.result_id = elesurface.data_id.element_id;
                            n.result_id2 = elesurface.data_index.face_index;
                            n.part_id = elesurface.part_id;
                            elesurfaces.push_back(n);
                        }
                        eleset->setElementSurfaceData(elesurfaces);
                        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                    }
                    else if (targetSetTypestr == QString("单元边集"))
                    {
                        mSelectMenuFunction::getInstance()->slot_mappingMeshLine(false);
                        auto resids = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshLine);
                        auto eleset = new MechSetData::mElementSurface(setID, MxFunctions::SetType::ElementEdge, name);
                        std::vector<RenderPickResultData> elesurfaces;
                        for (auto elesurface : resids)
                        {
                            RenderPickResultData n;
                            n.result_id = elesurface.data_id.element_id;
                            n.result_id2 = elesurface.data_index.line_index;
                            n.part_id = elesurface.part_id;
                            elesurfaces.push_back(n);
                        }
                        eleset->setElementSurfaceData(elesurfaces);
                        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                    }
                    else if (targetSetTypestr.contains("几何"))
                    {
                        auto setType = SetType::GeoPoint;
                        auto geoType = SelectionInfoType::GeoPoint;
                        if (targetSetTypestr == "几何点集")
                        {
                            mSelectMenuFunction::getInstance()->slot_mappingGeoPoint(false);
                        }
                        if (targetSetTypestr == "几何线集")
                        {
                            setType = SetType::GeoEdge; geoType = SelectionInfoType::GeoEdge; mSelectMenuFunction::getInstance()->slot_mappingGeoEdge(false);
                        }
                        else if (targetSetTypestr == "几何面集")
                        {
                            setType = SetType::GeoFace; geoType = SelectionInfoType::GeoFace; mSelectMenuFunction::getInstance()->slot_mappingGeoFace(false);
                        }
                        else if (targetSetTypestr == "几何体集")
                        {
                            setType = SetType::GeoSolid; geoType = SelectionInfoType::GeoSolid; mSelectMenuFunction::getInstance()->slot_mappingGeoSolid(false);
                        }
                        auto resids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(geoType);
                        auto eleset = new MechSetData::mGeoSet(setID, setType, name);
                        eleset->setGeoSetData(resids);
                        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
                    }
                    getCVM->clearPick();

                    mGlobalSignals::getInstance()->outputMessageSig(QString("成功转换集合 %1").arg(name), true);
                    emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSetSig(false, setID);

                    if (items.size() == 1)
                    {
                        mInfoWindow::getInstance()->editSet(setID);
                    }
                }
                auto buttonList = _setGW.at(1)->getButtons();
                _setGW.at(1)->setEnabled(true);
                for (int i = 0; i < 9; i++)
                {
                    buttonList.at(i)->setEnabled(true);
                }
                if (targetSetTypestr == "几何点集")
                    buttonList.at(0)->setEnabled(false);
                else if (targetSetTypestr == "几何线集")
                    buttonList.at(1)->setEnabled(false);
                else if (targetSetTypestr == "几何面集")
                    buttonList.at(2)->setEnabled(false);
                else if (targetSetTypestr == "几何体集")
                    buttonList.at(3)->setEnabled(false);
                else if (targetSetTypestr == "节点集")
                    buttonList.at(4)->setEnabled(false);
                else if (targetSetTypestr == "单元边集")
                    buttonList.at(5)->setEnabled(false);
                else if (targetSetTypestr == "单元面集")
                    buttonList.at(6)->setEnabled(false);
                else if (targetSetTypestr == "单元集")
                    buttonList.at(7)->setEnabled(false);
                else if (targetSetTypestr == "部件集")
                    buttonList.at(8)->setEnabled(false);

            };

        if (convertGW)
        {
            auto btn(convertGW->addToolBtn("几何点集", ":/mMainWidgets/Icon/choice/zhuan_jihedian.png", 0, 0, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "几何点集", ":/mMainWidgets/Icon/choice/zhuan_jihedian.png", "转换为几何点集合。");
            connect(btn, &QToolButton::pressed, this, [this, convertFunc, btn]() {
                convertFunc(btn->text());
                });

            btn = (convertGW->addToolBtn("几何线集", ":/mMainWidgets/Icon/choice/zhuan_jihexian.png", 0, 1, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "几何线集", ":/mMainWidgets/Icon/choice/zhuan_jihexian.png", "转换为几何线集合。");
            connect(btn, &QToolButton::pressed, this, [this, convertFunc, btn]() {
                convertFunc(btn->text());
                });

            btn = (convertGW->addToolBtn("几何面集", ":/mMainWidgets/Icon/choice/zhuan_jihemian.png", 0, 2, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "几何面集", ":/mMainWidgets/Icon/choice/zhuan_jihemian.png", "转换为几何面集合。");
            connect(btn, &QToolButton::pressed, this, [this, convertFunc, btn]() {
                convertFunc(btn->text());
                });

            btn = (convertGW->addToolBtn("几何体集", ":/mMainWidgets/Icon/choice/zhuan_jiheti.png", 0, 3, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "几何体集", ":/mMainWidgets/Icon/choice/zhuan_jiheti.png", "转换为几何体集合。");
            connect(btn, &QToolButton::pressed, this, [this, convertFunc, btn]() {
                convertFunc(btn->text());
                });

            btn = (convertGW->addToolBtn("节点集", ":/mMainWidgets/Icon/choice/zhuan_jiedian.png", 0, 4, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "节点集", ":/mMainWidgets/Icon/choice/zhuan_jiedian.png", "转换为节点集合。");
            connect(btn, &QToolButton::pressed, this, [this, convertFunc, btn]() {
                convertFunc(btn->text());
                });

            btn = (convertGW->addToolBtn("单元边集", ":/mMainWidgets/Icon/choice/zhuan_danyuanbian.png", 0, 5, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "单元边集", ":/mMainWidgets/Icon/choice/zhuan_danyuanbian.png", "转换为单元边集合。");
            connect(btn, &QToolButton::pressed, this, [this, convertFunc, btn]() {
                convertFunc(btn->text());
                });

            btn = (convertGW->addToolBtn("单元面集", ":/mMainWidgets/Icon/choice/zhuan_danyuanmian.png", 0, 6, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "单元面集", ":/mMainWidgets/Icon/choice/zhuan_danyuanmian.png", "转换为单元面集合。");
            connect(btn, &QToolButton::pressed, this, [this, convertFunc, btn]() {
                convertFunc(btn->text());
                });

            btn = (convertGW->addToolBtn("单元集", ":/mMainWidgets/Icon/choice/zhuan_danyuan.png", 0, 7, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "单元集", ":/mMainWidgets/Icon/choice/zhuan_danyuan.png", "转换为单元集合。");
            connect(btn, &QToolButton::pressed, this, [this, convertFunc, btn]() {
                convertFunc(btn->text());
                });

            btn = (convertGW->addToolBtn("部件集", ":/mMainWidgets/Icon/choice/zhuan_bujian.png", 0, 8, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "部件集", ":/mMainWidgets/Icon/choice/zhuan_bujian.png", "转换为部件集合。");
            connect(btn, &QToolButton::pressed, this, [this, convertFunc, btn]() {
                convertFunc(btn->text());
                });

        }
    }

    void mMenuWidget::initField()
    {
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("场", tempWid);

        std::shared_ptr<mGroupWidget> standardGW;
        std::shared_ptr<mGroupWidget> vecGW;
        std::shared_ptr<mGroupWidget> analysisGW;
        std::shared_ptr<mGroupWidget> spaceGW;

        standardGW = std::shared_ptr<mGroupWidget>(this->addGroup("场", "标量场"));
        vecGW = std::shared_ptr<mGroupWidget>(this->addGroup("场", "方向场"));
        analysisGW = std::shared_ptr<mGroupWidget>(this->addGroup("场", "分析场"));
        spaceGW = std::shared_ptr<mGroupWidget>(this->addGroup("场", "场数据"));

        _fieldGW.append(standardGW);
        _fieldGW.append(vecGW);
        _fieldGW.append(analysisGW);
        _fieldGW.append(spaceGW);

        if (standardGW)
        {
            auto btn(standardGW->addToolBtn("厚度场", ":/mMainWidgets/Icon/DiscreteField/thickSpace@3x.png",
                0, 0, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "厚度场", ":/mMainWidgets/Icon/DiscreteField/thickSpace@3x.png",
                "创建厚度场。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                //TODO 传场类型参数
                this->slot_createDiscreteField(MxFunctions::DiscreteSpace::ThicknessSpace);
                });

            btn = (standardGW->addToolBtn("角度场", ":/mMainWidgets/Icon/DiscreteField/angelSpace@3x.png",
                0, 1, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "角度场", ":/mMainWidgets/Icon/DiscreteField/angelSpace@3x.png",
                "创建角度场。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                //TODO 传场类型参数
                this->slot_createDiscreteField(MxFunctions::DiscreteSpace::AngelSpace);
                });
        }

        if (vecGW)
        {
            auto btn(vecGW->addToolBtn("坐标系场", ":/mMainWidgets/Icon/DiscreteField/coorSpace@3x.png",
                0, 0, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "坐标系场", ":/mMainWidgets/Icon/DiscreteField/coorSpace@3x.png",
                "创建坐标系场。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                //TODO 传场类型参数
                this->slot_createDiscreteField(MxFunctions::DiscreteSpace::CoorSpace);
                });
        }

        if (analysisGW)
        {
            //分析场（TODO 图标暂复用离散场占位图标，待替换为分析场专用图标）
            auto btn(analysisGW->addToolBtn("分析场", ":/mMainWidgets/Icon/DiscreteField/discreteField@3x.png",
                0, 0, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "分析场", ":/mMainWidgets/Icon/DiscreteField/discreteField@3x.png",
                "创建分析场。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                this->slot_createAnalysisField();
                });
        }

        if (spaceGW)
        {
            auto btn(spaceGW->addToolBtn("导出数据", ":/mMainWidgets/Icon/function/daochu.png",
                0, 0, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "导出数据", ":/mMainWidgets/Icon/function/daochu.png",
                "导出数据。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                //TODO 先不做

                });
            btn->setEnabled(false);

            btn = (spaceGW->addToolBtn("查看数据", ":/mMainWidgets/Icon/function/chakan.png",
                0, 1, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "查看数据", ":/mMainWidgets/Icon/function/chakan.png",
                "查看数据。");
            connect(btn, &QToolButton::pressed, this, [this]() {
                //TODO 先不做

                });
            btn->setEnabled(false);
        }
    }

    void mMenuWidget::initCoordinateMenu()
    {
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("坐标系", tempWid);

        std::shared_ptr<mGroupWidget> createGW;
        std::shared_ptr<mGroupWidget> convertGW;
        std::shared_ptr<mGroupWidget> nodeCoordinateGW;

        createGW = std::shared_ptr<mGroupWidget>(this->addGroup("坐标系", "创建"));
        convertGW = std::shared_ptr<mGroupWidget>(this->addGroup("坐标系", "变换"));
        nodeCoordinateGW = std::shared_ptr<mGroupWidget>(this->addGroup("坐标系", "节点坐标系"));

        _coordinateGW.append(createGW);
        _coordinateGW.append(convertGW);
        _coordinateGW.append(nodeCoordinateGW);


        if (createGW)
        {
            auto Btn(createGW->addToolBtn("直角坐标系", ":/mMainWidgets/Icon/coordinate/zhijiao.png", 0, 0, 2, 1, QSize(30, 30)));
            setQToolButton(Btn, "直角坐标系", ":/mMainWidgets/Icon/coordinate/zhijiao.png", "创建直角坐标系。");
            connect(Btn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createCoordinate(MxFunctions::Cartesian);
                });

            Btn = (createGW->addToolBtn("圆柱坐标系", ":/mMainWidgets/Icon/coordinate/yuanzhu.png", 0, 1, 2, 1, QSize(30, 30)));
            setQToolButton(Btn, "圆柱坐标系", ":/mMainWidgets/Icon/coordinate/yuanzhu.png", "创建圆柱坐标系。");
            connect(Btn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createCoordinate(MxFunctions::Cylindrical);
                });

            Btn = (createGW->addToolBtn("球坐标系", ":/mMainWidgets/Icon/coordinate/qiu.png", 0, 2, 2, 1, QSize(30, 30)));
            setQToolButton(Btn, "球坐标系", ":/mMainWidgets/Icon/coordinate/qiu.png", "创建球坐标系。");
            connect(Btn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createCoordinate(MxFunctions::Spherical);
                });

            Btn = (createGW->addToolBtn("质心坐标系", ":/mMainWidgets/Icon/coordinate/zhixing.png", 0, 3, 2, 1, QSize(30, 30)));
            setQToolButton(Btn, "质心坐标系", ":/mMainWidgets/Icon/coordinate/zhixing.png", "创建质心坐标系。");
            Btn->setFixedWidth(90);  // 设置最小宽度
            Btn->setFixedHeight(50);  // 设置最小高度
            connect(Btn, &QToolButton::pressed, this, [this]() {
                auto meshData = getCVM->getModelData()->getMeshData();
                auto geoManager = getCVM->getModelData()->getGeoManager();
                auto geoDatabase = getCVM->getModelData()->getGeoData();
                auto res = meshData->getAllPartIds();
                if (res.empty())
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString(u8"当前模型不存在任何部件，无法创建质心坐标系。"));
                    return;
                }
                QVector3D centroid;
                double volume = 0.0;
                for (auto partId : res)
                {
                    auto partData = meshData->getPartData(partId);
                    if (partData == nullptr)
                    {
                        continue;
                    }
                    QVector3D tempCentroid;
                    double tempVolume = 0.0;
                    int partType = partData->getPartType();
                    if (partType == 2)//网格部件
                    {
                        meshData->getPartWeight(partId, tempVolume, tempCentroid);
                        centroid += tempCentroid * tempVolume;
                        volume += tempVolume;
                    }
                    else
                    {
                        auto solids = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 3);
                        auto solidsRes = getGEOServer.geoCalculate().CalculateSolidAttr(solids);
                        for (auto solid : solidsRes)
                        {
                            tempVolume += solid.m_volume;
                            tempCentroid += solid.m_coor * solid.m_volume;
                        }
                        if (tempVolume > 0.0)
                        {
                            tempCentroid /= tempVolume;
                            centroid += tempCentroid * tempVolume;
                            volume += tempVolume;
                        }
                    }
                }
                if (volume <= 0.0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString(u8"当前模型部件体积异常（体积为零），无法计算质心坐标系。"));
                    return;
                }
                centroid /= volume;
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createCoordinate(MxFunctions::Cartesian, centroid);
                });

            Btn = (createGW->addToolBtn("离散坐标系", ":/mMainWidgets/Icon/coordinate/lisan.png", 0, 4, 2, 1, QSize(30, 30)));
            setQToolButton(Btn, "离散坐标系", ":/mMainWidgets/Icon/coordinate/lisan.png", "创建离散坐标系。");
            connect(Btn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createCoordinate(MxFunctions::DiscreteCoordinate);
                });
        }

        if (convertGW)
        {
            _transformList = new QListWidget(this);
            _transformList->setVisible(false);
            _transformList->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
            _transformList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            _transformList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            _transformList->setDragDropMode(QAbstractItemView::DragDrop);
            _transformList->setSelectionMode(QAbstractItemView::NoSelection);
            _transformList->setAttribute(Qt::WA_ShowWithoutActivating);
            _transformList->setStyleSheet(R"(
				QListWidget {
					background-color: #f8f8f8;
					border: 1px solid #cccccc;
					border-radius: 6px;
					padding: 4px;
				}
				QListWidget::item {
					margin: 2px;
					border-radius: 3px;
				}
				QListWidget::item:selected {
					background-color: #cce4ff;
				}
			)");

            auto makeVLine = []() {
                QFrame* line = new QFrame();
                line->setFrameShape(QFrame::VLine);
                line->setFrameShadow(QFrame::Plain);
                line->setStyleSheet("color: #000000; background-color: #000000; width: 0.5px;");
                return line;
                };

            auto addTransformButton = [&](const QString& text, const QString& icon, int row, int col, const std::function<void()>& lambda) {
                auto Btn = convertGW->addToolBtn(text, icon, row, col, 1, 1, QSize(20, 20));
                Btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                Btn->setFixedSize(70, 26);
                Btn->installEventFilter(new TooltipInstaller(text, QPixmap(icon), "", 200, Btn));
                connect(Btn, &QToolButton::pressed, this, lambda);
                return Btn;
                };


            auto onToggleTransformQueue = [this](QToolButton* btnQueue)
                {
                    _transformList->setVisible(true);

                    // 获取按钮在全局坐标中的位置
                    QPoint btnGlobalPos = btnQueue->mapToGlobal(QPoint(0, btnQueue->height()));

                    _transformList->move(btnGlobalPos);

                    // 统一样式：无边框悬浮列表
                    _transformList->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
                    //_transformList->setAttribute(Qt::WA_ShowWithoutActivating);
                    _transformList->setFixedSize(300, 200);
                    _transformList->show();
                    _transformList->raise();

                };
            auto lineEdit1 = new QDoubleSpinBox(convertGW.get()); lineEdit1->setButtonSymbols(QAbstractSpinBox::NoButtons); lineEdit1->setRange(-100000, 100000); lineEdit1->setDecimals(1); lineEdit1->setValue(1);
            auto lineEdit2 = new QDoubleSpinBox(convertGW.get()); lineEdit2->setButtonSymbols(QAbstractSpinBox::NoButtons); lineEdit2->setRange(0.1, 359); lineEdit2->setDecimals(1); lineEdit2->setValue(90);
            auto comboBox = new QComboBox(convertGW.get());
            comboBox->addItems({ QString("+X"),QString("-X"),QString("+Y"),QString("-Y"),QString("+Z"),QString("-Z") });

            // 偏移按钮
            auto btnOffsetX = addTransformButton("偏移X", ":/mMainWidgets/Icon/coordinate/pianyiX.png", 0, 0, [this, lineEdit1]() { addCoordinateTransform("偏移X", lineEdit1->text()); });
            auto btnOffsetY = addTransformButton("偏移Y", ":/mMainWidgets/Icon/coordinate/pianyiY.png", 0, 1, [this, lineEdit1]() { addCoordinateTransform("偏移Y", lineEdit1->text()); });
            auto btnOffsetZ = addTransformButton("偏移Z", ":/mMainWidgets/Icon/coordinate/pianyiZ.png", 0, 2, [this, lineEdit1]() { addCoordinateTransform("偏移Z", lineEdit1->text()); });

            // === 旋转 ===
            auto btnRotX = addTransformButton("旋转X", ":/mMainWidgets/Icon/coordinate/xuanzhuanX.png", 0, 4, [this, lineEdit2]() { addCoordinateTransform("旋转X", lineEdit2->text()); });
            auto btnRotY = addTransformButton("旋转Y", ":/mMainWidgets/Icon/coordinate/xuanzhuanY.png", 0, 5, [this, lineEdit2]() { addCoordinateTransform("旋转Y", lineEdit2->text()); });
            auto btnRotZ = addTransformButton("旋转Z", ":/mMainWidgets/Icon/coordinate/xuanzhuanZ.png", 0, 6, [this, lineEdit2]() { addCoordinateTransform("旋转Z", lineEdit2->text()); });

            // === 翻转 ===
            auto btnFlipX = addTransformButton("翻转X", ":/mMainWidgets/Icon/coordinate/fanzhuanX.png", 0, 8, [this, comboBox]() { addCoordinateTransform("翻转X", comboBox->currentText()); });
            auto btnFlipY = addTransformButton("翻转Y", ":/mMainWidgets/Icon/coordinate/fanzhuanY.png", 0, 9, [this, comboBox]() { addCoordinateTransform("翻转Y", comboBox->currentText()); });
            auto btnFlipZ = addTransformButton("翻转Z", ":/mMainWidgets/Icon/coordinate/fanzhuanZ.png", 0, 10, [this, comboBox]() { addCoordinateTransform("翻转Z", comboBox->currentText()); });

            //convertGW->addWidget(makeVLine(), 0, 3, 2, 1);   // 偏移 / 旋转 分隔
            //convertGW->addWidget(makeVLine(), 0, 7, 2, 1);   // 旋转 / 翻转 分隔
            //convertGW->addWidget(makeVLine(), 0, 11, 2, 1);  // 翻转 / 队列 分隔

            // === 队列按钮 ===
            auto btnQueue = addTransformButton("变换队列", ":/mMainWidgets/Icon/Property/gan.png", 0, 12, [this, onToggleTransformQueue]() {
                });
            btnQueue->setFixedWidth(100);
            btnQueue->setIcon(QIcon(":/mMainWidgets/Icon/homepage/arrow-down3x.png"));
            connect(btnQueue, &QToolButton::pressed, [this, onToggleTransformQueue, btnQueue]() {
                onToggleTransformQueue(btnQueue);
                });

            // 布局
            QHBoxLayout* vlayout = new QHBoxLayout;
            vlayout->setSpacing(0);
            vlayout->setMargin(0);
            vlayout->addWidget(lineEdit1, 1);
            vlayout->addWidget(new QLabel("m"), 0);
            vlayout->addSpacing(20);
            convertGW->addBoxLayout(vlayout, 1, 0, 1, 3);

            lineEdit1->setFixedHeight(26);
            //lineEdit->setFixedWidth(40);


            // 布局
            vlayout = new QHBoxLayout;
            vlayout->setSpacing(0);
            vlayout->setMargin(0);
            vlayout->addWidget(lineEdit2, 1);
            vlayout->addWidget(new QLabel("°"), 0);
            vlayout->addSpacing(20);
            convertGW->addBoxLayout(vlayout, 1, 4, 1, 3);

            lineEdit2->setFixedHeight(26);
            //lineEdit->setFixedWidth(40);


            vlayout = new QHBoxLayout;
            vlayout->setSpacing(0);
            vlayout->setMargin(0);
            vlayout->addWidget(comboBox, 1);
            vlayout->addSpacing(20);
            convertGW->addBoxLayout(vlayout, 1, 8, 1, 3);

        }

        if (nodeCoordinateGW)
        {
            auto Btn(nodeCoordinateGW->addToolBtn("节点坐标系", ":/mMainWidgets/Icon/coordinate/jiedian.png", 0, 0, 1, 1, QSize(30, 30)));
            setQToolButton(Btn, "节点坐标系", ":/mMainWidgets/Icon/coordinate/jiedian.png", "节点坐标系。");
            connect(Btn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createNodeCoordinate();
                });
        }
    }

	mComboButton::mComboButton(QWidget* parent)
		: mRibbonButton(mRibbonButton::Mode::Split, parent)
	{
		initUi();
	}

	mComboButton::~mComboButton()
	{
	}

	void mComboButton::initUi()
	{
		// 尺寸/绘制全部交给 mRibbonButton（Mode::Split）与规则公式，本类只做业务编排。
		// 打标 uiBigButton=true：ThemeManager 按普通全高按钮同一分支统一重算
		// （高=bigButtonHeight、图标=menuLargeIconSize、宽=ribbonButtonMinWidth、主题重发），
		// 与其他全高按钮严格同口径
		setObjectName("comboButton");
		setProperty("uiBigButton", true);
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		setIconSize(QSize(UiMetrics::menuLargeIconSize(), UiMetrics::menuLargeIconSize()));  // 大图标 menuLargeIcon=H+4（标准 30）
		setFixedHeight(UiMetrics::bigButtonHeight());
		setFixedWidth(ruleWidth());

		// 选择菜单（内容由setupToggleItems/setActionItems填充）；
		// 挂到按钮上让 Split 模式的 showMenu() 统一负责级联定位（规则 1.0）。
		// 按钮全程不挂 QSS，Qt 的 menu-indicator 槽不参与绘制，不会叠加箭头
		m_menu = new mUiMenu(this);
		ThemeManager::applyMenuStyle(m_menu);
		setMenu(m_menu);

		// 菜单为模态弹窗并抓取鼠标，期间 hover 状态不可靠：
		// 打开/关闭时维持"入口展开激活态"由 setMenuActive 表达，关闭后由真实鼠标事件重新驱动
		connect(m_menu, &QMenu::aboutToShow, this, [this]() {
			setMenuActive(true);
			});
		connect(m_menu, &QMenu::aboutToHide, this, [this]() {
			setMenuActive(false);
			});

		// 文字+箭头区激活：弹出选择菜单，菜单关闭后同步选中状态（开关模式）
		connect(this, &mRibbonButton::dropClicked, this, &mComboButton::slot_dropClicked);

		// 图标区点击：开关模式由 toggled 通知外部；动作模式执行当前功能项
		connect(this, &mRibbonButton::toggled, this, [this](bool checked) {
			if (m_checkMode)
				emit toggled(checked, currentText());
			});
		connect(this, &mRibbonButton::clicked, this, [this](bool) {
			if (m_checkMode)
				return;  // 开关模式走 toggled
			if (m_currentIndex >= 0 && m_currentIndex < m_actionItems.size())
			{
				const Item& item = m_actionItems.at(m_currentIndex);
				if (item.callback)
					item.callback();
			}
			});
	}

	void mComboButton::setupToggleItems(const QString& dropTextPrefix, const QString& mainIconPath, const QString& mainTooltip, const QStringList& items)
	{
		m_checkMode = true;
		m_dropTextPrefix = dropTextPrefix;

		setIcon(QIcon(mainIconPath));
		installEventFilter(new TooltipInstaller(QString(),
			QPixmap(mainIconPath),
			mainTooltip,
			200, this));
		setCheckable(true);
		setDropText(m_dropTextPrefix + (items.isEmpty() ? QString() : items.first()));

		for (const QString& item : items)
		{
			QAction* action = m_menu->addAction(item);
			connect(action, &QAction::triggered, this, [this, action]() {
				slot_menuActionTriggered(action);
				});
		}
		syncButtonWidth();
	}

	void mComboButton::setActionItems(const QList<Item>& items)
	{
		m_checkMode = false;
		m_actionItems = items;
		m_currentIndex = items.isEmpty() ? -1 : 0;

		// 动作模式无选中态
		setCheckable(false);

		for (int i = 0; i < items.size(); ++i)
		{
			QAction* action = m_menu->addAction(QIcon(items.at(i).iconPath), items.at(i).text);
			action->setData(i);
			connect(action, &QAction::triggered, this, [this, action]() {
				slot_menuActionTriggered(action);
				});
		}
		applyCurrentItem();
		syncButtonWidth();
	}

	int mComboButton::ruleWidth() const
	{
		// 规则 1.3 全高按钮宽度公式：max(共用最小宽, max(大图标menuLargeIconSize, 文字宽+箭头占宽) + 2×menuInset)
		QFont f(UiMetrics::uiFontFamily());
		f.setPixelSize(UiMetrics::buttonFontSize());
		const QFontMetrics fm(f);
		const int textW = qMax(fm.horizontalAdvance(text()), fm.horizontalAdvance(dropText()));
		return qMax(UiMetrics::menuFullMinWidth(),
			qMax(UiMetrics::menuLargeIconSize(), textW + UiMetrics::arrowReserveWidth()) + 2 * UiMetrics::menuInset());
	}

	void mComboButton::syncButtonWidth()
	{
		setFixedHeight(UiMetrics::bigButtonHeight());
		setIconSize(QSize(UiMetrics::menuLargeIconSize(), UiMetrics::menuLargeIconSize()));
		setButtonWidth(ruleWidth());
	}

	void mComboButton::setButtonWidth(int width)
	{
		if (width <= 0)
		{
			return;
		}
		// 宽度下限走规则 1.3 公式：文字+箭头+留白装不下时不能压文字，
		// 调用方给的值只作建议、公式值兜底
		setFixedWidth(qMax(width, ruleWidth()));
	}

	void mComboButton::setButtonSize(int width, int mainHeight, int dropHeight)
	{
		// 旧双控件接口兼容：高度由规则公式统一（bigButtonHeight），只保留宽度语义
		Q_UNUSED(mainHeight);
		Q_UNUSED(dropHeight);
		setButtonWidth(width);
	}

	void mComboButton::setMainButtonHeight(int height)
	{
		// 旧双控件接口兼容：单控件后高度由 bigButtonHeight 统一
		Q_UNUSED(height);
	}

	void mComboButton::setDropDownHeight(int height)
	{
		Q_UNUSED(height);
	}

	void mComboButton::setMainButtonIconSize(const QSize& size)
	{
		if (size.width() <= 0 || size.height() <= 0)
		{
			return;
		}
		setIconSize(size);
	}

	bool mComboButton::isChecked() const
	{
		return mRibbonButton::isChecked();
	}

	QString mComboButton::currentText() const
	{
		return dropText();
	}

	void mComboButton::slot_dropClicked()
	{
		showMenu();
		if (!m_checkMode)
		{
			return;
		}
	}

	void mComboButton::slot_menuActionTriggered(QAction* action)
	{
		if (m_checkMode)
		{
			// 开关模式：更新参数项文字并保持开启；setChecked 在已开启时不发 toggled，
			// 这里统一补发一次（语义：参数切换也立即生效，与旧双控件实现一致）
			const bool wasChecked = isChecked();
			setDropText(m_dropTextPrefix + action->text());
			setChecked(true);
			if (wasChecked)
				emit toggled(true, currentText());
			syncButtonWidth();
		}
		else
		{
			// 动作模式：切换当前功能项并立即执行
			int index = action->data().toInt();
			if (index >= 0 && index < m_actionItems.size())
			{
				m_currentIndex = index;
				applyCurrentItem();
				syncButtonWidth();
				const Item& item = m_actionItems.at(index);
				if (item.callback)
				{
					item.callback();
				}
			}
		}
	}

	void mComboButton::applyCurrentItem()
	{
		if (m_currentIndex < 0 || m_currentIndex >= m_actionItems.size())
		{
			return;
		}
		const Item& item = m_actionItems.at(m_currentIndex);
		setIcon(QIcon(item.iconPath));
		setMainToolTip(item.tooltip);
		setToolTip(item.tooltip);
		setDropText(item.text);
	}

	void mMenuWidget::initDisplayMenu()
	{
		// 下方Ribbon工具条上添加相应的显示widget作为Group的载体
		RibbonPage* tempWid = new RibbonPage();
		m_toolBarStackedWid_->addWidget(tempWid);
		m_StaWidMap.insert("显示", tempWid);

        std::shared_ptr<mGroupWidget> orientGW;
        std::shared_ptr<mGroupWidget> entityGW;
        std::shared_ptr<mGroupWidget> IDGW;
        std::shared_ptr<mGroupWidget> freeNodeGW;
        std::shared_ptr<mGroupWidget> modelGW;

        orientGW = std::shared_ptr<mGroupWidget>(this->addGroup("显示", "视图"));
        entityGW = std::shared_ptr<mGroupWidget>(this->addGroup("显示", "实体化"));
        IDGW = std::shared_ptr<mGroupWidget>(this->addGroup("显示", "编号"));
        freeNodeGW = std::shared_ptr<mGroupWidget>(this->addGroup("显示", "自由节点"));
        modelGW = std::shared_ptr<mGroupWidget>(this->addGroup("显示", "模型"));

        addSpacerItem("显示");

        _displayGW.append(orientGW);
        _displayGW.append(entityGW);
        //多体形态：实体化（壳/梁/弹簧/阻尼显示实体化）为隐式 FEM 显示功能，整组隐藏
        if (entityGW && MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion)
        {
            entityGW->hide();
        }
        _displayGW.append(IDGW);
        _displayGW.append(freeNodeGW);
        _displayGW.append(modelGW);


        if (orientGW)
        {
            // ===== 大按钮区（图标上、文字下）：步骤 / 旋转Sx / 旋转Sy / 旋转Sz，左键正向、右键反向 =====

            // 步骤：左键上一个视角，右键下一个视角
            auto btn(orientGW->addToolBtn("步骤", ":/mMainWidgets/Icon/display/buzhou.png", 0, 0, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "步骤", ":/mMainWidgets/Icon/display/buzhou.png", "左键：上一个视角\n右键：下一个视角");
            auto* stepFilter = new DualClickFilter(btn);
            stepFilter->onRightClick = []() {
                mDisplayMenuFunction::getInstance()->slot_changeViewList(true);
                };
            btn->installEventFilter(stepFilter);
            connect(btn, &QToolButton::pressed, this, []() {
                mDisplayMenuFunction::getInstance()->slot_changeViewList(false);
                });

            // 度数输入框（旋转角度）
            auto angleEdit = new QDoubleSpinBox(orientGW.get());
            angleEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
            angleEdit->setRange(0.1, 359);
            angleEdit->setDecimals(1);
            angleEdit->setValue(30);
            angleEdit->setMinimumWidth(46);  // 保留增长余地，避免高度随档变化后数值被裁
            angleEdit->setFixedHeight(UiMetrics::menuItemHeight());

            // 旋转Sx/Sy/Sz：左键正向旋转，右键反向旋转（slot_angleRotation按+Sx/-Sx解析方向）
            const QString rotNames[3] = { "旋转Sx", "旋转Sy", "旋转Sz" };
            const QString rotIcons[3] = { ":/mMainWidgets/Icon/display/xuanzhuanSx.png",
                ":/mMainWidgets/Icon/display/xuanzhuanSy.png",
                ":/mMainWidgets/Icon/display/xuanzhuanSz.png" };
            for (int i = 0; i < 3; ++i)
            {
                const QString name = rotNames[i];
                const QString icon = rotIcons[i];
                const QString axisPos = QString("旋转+S%1").arg(QChar('x' + i));
                const QString axisNeg = QString("旋转-S%1").arg(QChar('x' + i));
                const QString tip = QString("左键：绕屏幕%1轴正向旋转\n右键：绕屏幕%1轴反向旋转").arg(QChar('x' + i));

                btn = (orientGW->addToolBtn(name, icon, 0, 1 + i, 2, 1, QSize(30, 30)));
                setQToolButton(btn, name, icon, tip);
                auto* rotFilter = new DualClickFilter(btn);
                rotFilter->onRightClick = [axisNeg, angleEdit]() {
                    mDisplayMenuFunction::getInstance()->slot_angleRotation(axisNeg, angleEdit->text().toDouble());
                    };
                btn->installEventFilter(rotFilter);
                connect(btn, &QToolButton::pressed, this, [axisPos, angleEdit]() {
                    mDisplayMenuFunction::getInstance()->slot_angleRotation(axisPos, angleEdit->text().toDouble());
                    });
            }

            // 度数框+“度”标签，垂直居中于两行
            {
                auto* hbox = new QHBoxLayout();
                hbox->setContentsMargins(0, 0, 0, 0);
                hbox->addWidget(angleEdit);
                auto* label = new QLabel("度");
                label->setFixedWidth(15);
                hbox->addWidget(label);
                auto* vbox = new QVBoxLayout();
                vbox->setContentsMargins(0, 0, 0, 0);
                vbox->addStretch();
                vbox->addLayout(hbox);
                orientGW->addBoxLayout(vbox, 0, 4, 2, 1);
            }

            // ===== 方向键小图标（键盘方向键T字形布局）+ 平移百分比输入框 =====
            auto panEdit = new QDoubleSpinBox(orientGW.get());
            panEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
            panEdit->setRange(1, 100);
            panEdit->setDecimals(0);
            panEdit->setValue(10);
            panEdit->setMinimumWidth(40);  // 1-100 三位数 + 内边距
            panEdit->setFixedHeight(UiMetrics::menuItemHeight());

            btn = (orientGW->addToolBtn("向上", ":/mMainWidgets/Icon/display/xiangshang.png", 0, 6, 1, 1, QSize(16, 16)));
            setQToolButton(btn, "向上", ":/mMainWidgets/Icon/display/xiangshang.png", "向上平移", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [panEdit]() {
                mDisplayMenuFunction::getInstance()->slot_directionPan("up", panEdit->text().toDouble());
                });

            btn = (orientGW->addToolBtn("向下", ":/mMainWidgets/Icon/display/xiangxia.png", 1, 6, 1, 1, QSize(16, 16)));
            setQToolButton(btn, "向下", ":/mMainWidgets/Icon/display/xiangxia.png", "向下平移", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [panEdit]() {
                mDisplayMenuFunction::getInstance()->slot_directionPan("down", panEdit->text().toDouble());
                });

            btn = (orientGW->addToolBtn("向左", ":/mMainWidgets/Icon/display/xiangzuo.png", 1, 5, 1, 1, QSize(16, 16)));
            setQToolButton(btn, "向左", ":/mMainWidgets/Icon/display/xiangzuo.png", "向左平移", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [panEdit]() {
                mDisplayMenuFunction::getInstance()->slot_directionPan("left", panEdit->text().toDouble());
                });

            btn = (orientGW->addToolBtn("向右", ":/mMainWidgets/Icon/display/xiangyou.png", 1, 7, 1, 1, QSize(16, 16)));
            setQToolButton(btn, "向右", ":/mMainWidgets/Icon/display/xiangyou.png", "向右平移", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [panEdit]() {
                mDisplayMenuFunction::getInstance()->slot_directionPan("right", panEdit->text().toDouble());
                });

            // 百分比输入框+“%”标签，垂直居中于两行
            {
                auto* hbox = new QHBoxLayout();
                hbox->setContentsMargins(0, 0, 0, 0);
                hbox->addWidget(panEdit);
                auto* label = new QLabel("%");
                label->setFixedWidth(15);
                hbox->addWidget(label);
                auto* vbox = new QVBoxLayout();
                vbox->setContentsMargins(0, 0, 0, 0);
                vbox->addStretch();
                vbox->addLayout(hbox);
                orientGW->addBoxLayout(vbox, 0, 8, 2, 1);
            }

            // ===== 缩放（左键放大、右键缩小）+ 倍数输入框 =====
            auto zoomEdit = new QDoubleSpinBox(orientGW.get());
            zoomEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
            zoomEdit->setRange(0.1, 100);
            zoomEdit->setDecimals(1);
            zoomEdit->setValue(2);
            zoomEdit->setMinimumWidth(42);  // 一位小数 + 内边距
            zoomEdit->setFixedHeight(UiMetrics::menuItemHeight());

            btn = (orientGW->addToolBtn("缩放", ":/mMainWidgets/Icon/display/suofang.png", 0, 9, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "缩放", ":/mMainWidgets/Icon/display/suofang.png", "左键：放大\n右键：缩小");
            auto* zoomFilter = new DualClickFilter(btn);
            zoomFilter->onRightClick = [zoomEdit]() {
                mDisplayMenuFunction::getInstance()->slot_valueZoom(false, zoomEdit->text().toDouble());
                };
            btn->installEventFilter(zoomFilter);
            connect(btn, &QToolButton::pressed, this, [zoomEdit]() {
                mDisplayMenuFunction::getInstance()->slot_valueZoom(true, zoomEdit->text().toDouble());
                });

            // 倍数输入框+“倍”标签，垂直居中于两行
            {
                auto* hbox = new QHBoxLayout();
                hbox->setContentsMargins(0, 0, 0, 0);
                hbox->addWidget(zoomEdit);
                auto* label = new QLabel("倍");
                label->setFixedWidth(15);
                hbox->addWidget(label);
                auto* vbox = new QVBoxLayout();
                vbox->setContentsMargins(0, 0, 0, 0);
                vbox->addStretch();
                vbox->addLayout(hbox);
                orientGW->addBoxLayout(vbox, 0, 10, 2, 1);
            }

            // 自动旋转组合按钮（开关模式）：上方图标按钮开启/停止旋转，下方下拉按钮选择旋转轴
            //（src 侧新增能力，合流保留；实现见 mComboButton 类）
            {
                mComboButton* autoRotateWidget = new mComboButton(this);
                autoRotateWidget->setupToggleItems(QString::fromUtf8("自动旋转"),
                    ":/mMainWidgets/Icon/display/zidongxuanzhuang.png",
                    QString::fromUtf8("绕选定轴自动旋转"),
                    { "+Sx", "-Sx", "+Sy", "-Sy", "+Sz", "-Sz" });
                autoRotateWidget->setMainButtonHeight(30);
                autoRotateWidget->setMainButtonIconSize(QSize(28, 28));
                autoRotateWidget->setDropDownHeight(22);
                connect(autoRotateWidget, &mComboButton::toggled, this, [](bool checked, const QString& text) {
                    mDisplayMenuFunction::getInstance()->slot_autoRotation(text, checked);
                    });

                QVBoxLayout* vlayout = new QVBoxLayout;
                vlayout->setSpacing(0);
                vlayout->setContentsMargins(0, 0, 0, 0);
                vlayout->addWidget(autoRotateWidget);
                orientGW->addBoxLayout(vlayout, 0, 11, 2, 1);
            }
        }

        if (entityGW)
        {
            // 壳/梁/弹簧/阻尼切换大按钮：按下显示实体化，弹起隐藏
            auto btn = (entityGW->addToolBtn("壳", ":/mMainWidgets/Icon/display/xianshiquanbuke.png", 0, 0, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "壳", ":/mMainWidgets/Icon/display/xianshiquanbuke.png", "按下：显示全部壳实体化\n弹起：隐藏全部壳实体化");
            btn->setCheckable(true);
            connect(btn, &QToolButton::toggled, this, [](bool checked) {
                mDisplayMenuFunction::getInstance()->slot_setAllShellVisble(checked);
                });

            btn = (entityGW->addToolBtn("梁", ":/mMainWidgets/Icon/display/xianshiquanbuliang.png", 0, 1, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "梁", ":/mMainWidgets/Icon/display/xianshiquanbuliang.png", "按下：显示全部梁实体化\n弹起：隐藏全部梁实体化");
            btn->setCheckable(true);
            connect(btn, &QToolButton::toggled, this, [](bool checked) {
                if (!getCVM)
                {
                    return;
                }
                if (getCVM->getPreRenderWidget())
                {
                    mDisplayMenuFunction::getInstance()->slot_setAllRodBeamVisble(checked, ResultantProperty);
                }
                if (getCVM->getPostRenderWidget())
                {
                    mDisplayMenuFunction::getInstance()->slot_setPostAllBeamVisble(checked);
                }
            });

            btn = (entityGW->addToolBtn("弹簧", ":/mMainWidgets/Icon/display/xianshiquanbutanhuang.png", 0, 2, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "弹簧", ":/mMainWidgets/Icon/display/xianshiquanbutanhuang.png", "按下：显示全部弹簧实体化\n弹起：隐藏全部弹簧实体化");
            btn->setCheckable(true);
            connect(btn, &QToolButton::toggled, this, [](bool checked) {
                mDisplayMenuFunction::getInstance()->slot_setAllSpringVisble(checked);
                });

            btn = (entityGW->addToolBtn("阻尼", ":/mMainWidgets/Icon/display/quanbuzuni.png", 0, 3, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "阻尼", ":/mMainWidgets/Icon/display/quanbuzuni.png", "按下：显示全部阻尼实体化\n弹起：隐藏全部阻尼实体化");
            btn->setCheckable(true);
            connect(btn, &QToolButton::toggled, this, [](bool checked) {
                mDisplayMenuFunction::getInstance()->slot_setAllSpringVisble(checked, ViscousDampProperty);
                });
        }

        if (IDGW)
        {
            auto btn = (IDGW->addToolBtn("显示编号", ":/mMainWidgets/Icon/display/xianshiID.png", 0, 1, 2, 1, QSize(30, 30)));
            setQToolButton(btn, "显示编号", ":/mMainWidgets/Icon/display/xianshiID.png", "可根据拾取内容显示节点、网格编号");
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_showNodeEleLabel();
                });

            btn = (IDGW->addToolBtn("隐藏编号", ":/mMainWidgets/Icon/display/yincangID.png", 0, 2, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "隐藏编号", ":/mMainWidgets/Icon/display/yincangID.png", "可根据拾取内容隐藏节点、网格编号", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_hideNodeEleLabel();
                });

            btn = (IDGW->addToolBtn("隐藏全部编号", ":/mMainWidgets/Icon/display/yincangquanbu.png", 1, 2, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "隐藏全部编号", ":/mMainWidgets/Icon/display/yincangquanbu.png", "隐藏全部的编号", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_hideAllNodeEleLabel();
                });
        }
        if (freeNodeGW)
        {
            auto btn = (freeNodeGW->addToolBtn("显示", ":/mMainWidgets/Icon/display/xianshiziyoujiedian.png", 0, 0, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "显示自由节点", ":/mMainWidgets/Icon/display/xianshiziyoujiedian.png", "显示选中自由节点", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_showFreeNode();
                });

            btn = (freeNodeGW->addToolBtn("隐藏", ":/mMainWidgets/Icon/display/yincangziyoujiedian.png", 1, 0, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "隐藏自由节点", ":/mMainWidgets/Icon/display/yincangziyoujiedian.png", "隐藏选中的自由节点", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_hideFreeNode();
                });

            btn = (freeNodeGW->addToolBtn("显示全部", ":/mMainWidgets/Icon/display/quanbuziyoujiedian.png", 0, 1, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "显示全部自由节点", ":/mMainWidgets/Icon/display/quanbuziyoujiedian.png", "显示全部自由节点", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_showAllFreeNode();
                });

            btn = (freeNodeGW->addToolBtn("隐藏全部", ":/mMainWidgets/Icon/display/yincangquanbuziyoujiedian.png", 1, 1, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "隐藏全部自由节点", ":/mMainWidgets/Icon/display/yincangquanbuziyoujiedian.png", "隐藏全部自由节点", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_hideAllFreeNode();
                });

            btn = (freeNodeGW->addToolBtn("删除全部", ":/mMainWidgets/Icon/display/yincangquanbuziyoujiedian.png", 0, 2, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "删除全部自由节点", ":/mMainWidgets/Icon/display/yincangquanbuziyoujiedian.png", "删除全部自由节点", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_deleteAllFreeNode();
                });

            btn = (freeNodeGW->addToolBtn("删除选中", ":/mMainWidgets/Icon/display/yincangziyoujiedian.png", 1, 2, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "删除自由节点", ":/mMainWidgets/Icon/display/yincangziyoujiedian.png", "删除选中的自由节点", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_deleteFreeNode();
                });

        }

        if (modelGW)
        {
            auto btn(modelGW->addToolBtn("隐藏当前", ":/mMainWidgets/Icon/display/yincangdangqian.png", 0, 1, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "隐藏当前", ":/mMainWidgets/Icon/display/yincangdangqian.png", "隐藏当前", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_maskModel();
                });

            btn = (modelGW->addToolBtn("隐藏其他", ":/mMainWidgets/Icon/display/yincangqita.png", 1, 1, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "隐藏其他", ":/mMainWidgets/Icon/display/yincangqita.png", "隐藏其他", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_maskOtherModel();
                });

            btn = (modelGW->addToolBtn("反转显示", ":/mMainWidgets/Icon/display/fanzhuanxianshi.png", 0, 2, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "反转显示", ":/mMainWidgets/Icon/display/fanzhuanxianshi.png", "反转显示", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_reverseMaskModel();
                });

            btn = (modelGW->addToolBtn("显示全部", ":/mMainWidgets/Icon/display/xianshiquanbu.png", 1, 2, 1, 1, QSize(20, 20)));
            setQToolButton(btn, "显示全部", ":/mMainWidgets/Icon/display/xianshiquanbu.png", "显示全部", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::pressed, this, [this, btn]() {mDisplayMenuFunction::getInstance()->slot_showAllModel();
                });

        }
    }

    void mMenuWidget::initSelectMenu()
    {
        // 下方Ribbon工具条上添加相应的显示widget作为Group的载体
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("选择", tempWid);

        std::shared_ptr<mGroupWidget> extendGW;
        std::shared_ptr<mGroupWidget> featureGW;
        std::shared_ptr<mGroupWidget> locateGW;
        std::shared_ptr<mGroupWidget> griddingGW;
        std::shared_ptr<mGroupWidget> exchangeGW;
        std::shared_ptr<mGroupWidget> batchGW;

        extendGW = std::shared_ptr<mGroupWidget>(this->addGroup("选择", "扩展"));
        featureGW = std::shared_ptr<mGroupWidget>(this->addGroup("选择", "特征"));
        locateGW = std::shared_ptr<mGroupWidget>(this->addGroup("选择", "位置尺寸"));
        griddingGW = std::shared_ptr<mGroupWidget>(this->addGroup("选择", "网格"));
        exchangeGW = std::shared_ptr<mGroupWidget>(this->addGroup("选择", "转换"));
        batchGW = std::shared_ptr<mGroupWidget>(this->addGroup("选择", "批量操作"));

        _selectGW.append(extendGW);
        _selectGW.append(featureGW);
        _selectGW.append(locateGW);
        _selectGW.append(griddingGW);
        _selectGW.append(exchangeGW);
        _selectGW.append(batchGW);


        /*
         *扩展
         */
        if (extendGW)
        {

            auto adjacentBtn(extendGW->addToolBtn("相邻", ":/mMainWidgets/Icon/choice/xianglin3x.png", -1, -1, 1, 1, QSize(30, 30)));
            setQToolButton(adjacentBtn, "相邻", ":/mMainWidgets/Icon/choice/xianglin3x.png", "搜索与当前所选面相邻且沿其共用边的角度\n"
                "偏差在容差范围内的面或与当前所选边相\n"
                "邻且在共享顶点处满足角度公差的边。\n");
            connect(adjacentBtn, &QToolButton::pressed, this, [this]() {mSelectMenuFunction::getInstance()->slot_adjacentFaceLine(false);
                }
            );

            auto limitsBtn(extendGW->addToolBtn("限值", ":/mMainWidgets/Icon/choice/xianzhi3x.png", 0, 0, 1, 1, QSize(20, 20)));
            setQToolButton(limitsBtn, "限值", ":/mMainWidgets/Icon/choice/xianzhi3x.png", "搜索与当前选择相切的面以及与部件内每个附\n"
                "加选择相切的所有面或与当前选择相切的边以\n"
				"及与部件内每个附加选择相切的所有边。\n",Qt::ToolButtonTextBesideIcon);
            connect(limitsBtn, &QToolButton::pressed, this, [this]() {mSelectMenuFunction::getInstance()->slot_adjacentFaceLine(true);
                }
            );

            auto connectionBtn(extendGW->addToolBtn("连接", ":/mMainWidgets/Icon/choice/lianjie3x.png", 1, 0, 1, 1, QSize(20, 20)));
            setQToolButton(connectionBtn, "连接", ":/mMainWidgets/Icon/choice/lianjie3x.png", "选择扩展到所有相邻的面，包括由连接(接触、连接等)促成\n"
                "的新选择面，该连接也位于所选部件上。如果进行了包含多\n"
                "个几何体的初始选择，则此选项会跨越多个几何体。\n", Qt::ToolButtonTextBesideIcon);
            connect(connectionBtn, &QToolButton::pressed, mSelectMenuFunction::getInstance(), &mSelectMenuFunction::slot_extendConnectFace);
        }
        /*
        * 特征
        */
        if (featureGW)
        {
            auto commonEdgesBtn(featureGW->addToolBtn("公共边", ":/mMainWidgets/Icon/choice/gonggongbian3x.png", 0, 0, 1, 1, QSize(20, 20)));
            setQToolButton(commonEdgesBtn, "公共边", ":/mMainWidgets/Icon/choice/gonggongbian3x.png", "仅对于几何，选中已经选\n"
                "中的面的公共边。", Qt::ToolButtonTextBesideIcon);
            connect(commonEdgesBtn, &QToolButton::pressed, mSelectMenuFunction::getInstance(), &mSelectMenuFunction::slot_faceCommonEdge);

            auto cylinderFacesBtn(featureGW->addToolBtn("圆柱面", ":/mMainWidgets/Icon/choice/yuanzhumian3x.png", 1, 0, 1, 1, QSize(20, 20)));
            setQToolButton(cylinderFacesBtn, "圆柱面", ":/mMainWidgets/Icon/choice/yuanzhumian3x.png", "选中模型上所有是圆柱形的表面\n"
                "(它们不一定是完整的圆柱形)。", Qt::ToolButtonTextBesideIcon);
            connect(cylinderFacesBtn, &QToolButton::pressed, mSelectMenuFunction::getInstance(), &mSelectMenuFunction::slot_cylinderFaceSelect);


            auto sunkenBtn(featureGW->addToolBtn("凹陷", ":/mMainWidgets/Icon/choice/aoxian.png", 0, 1, 1, 1, QSize(20, 20)));
            setQToolButton(sunkenBtn, "凹陷", ":/mMainWidgets/Icon/choice/aoxian.png", "仅对于几何，选中凹陷的\n"
                "几何面。", Qt::ToolButtonTextBesideIcon);
            connect(sunkenBtn, &QToolButton::pressed, this, []() {
                mSelectMenuFunction::getInstance()->slot_sunkenRaisedFaceSelect(true);
                });

            auto raisedBtn(featureGW->addToolBtn("凸起", ":/mMainWidgets/Icon/choice/tuqi.png", 1, 1, 1, 1, QSize(20, 20)));
            setQToolButton(raisedBtn, "凸起", ":/mMainWidgets/Icon/choice/tuqi.png", "仅对于几何，选中凸起的\n"
                "几何面", Qt::ToolButtonTextBesideIcon);
            connect(raisedBtn, &QToolButton::pressed, this, []() {
                mSelectMenuFunction::getInstance()->slot_sunkenRaisedFaceSelect(false);
                });


        }
        /*
        * 位置尺寸
        */
        if (locateGW)
        {
            auto locateBtn(locateGW->addToolBtn("位置", ":/mMainWidgets/Icon/choice/weizhi3x.png", -1, -1, 1, 1, QSize(30, 30)));
            locateBtn->setProperty("mxHasMenu", true);
            setQToolButton(locateBtn, "位置", ":/mMainWidgets/Icon/choice/weizhi3x.png", "显示位置下拉菜单。\n");

            auto locateMenu = new mUiMenu(this);
            ThemeManager::applyMenuStyle(locateMenu);
            locateBtn->setMenu(locateMenu);
            locateBtn->setPopupMode(QToolButton::InstantPopup);
            auto entity_x = locateMenu->addAction(QIcon(":/mMainWidgets/Icon/choice/Xshiti3x.png"), "选择X位置相同的所有实体");
            connect(entity_x, &QAction::triggered, this, [this]() {mSelectMenuFunction::getInstance()->slot_sameLocationGeoSelect(0);
                }
            );
            auto entity_y = locateMenu->addAction(QIcon(":/mMainWidgets/Icon/choice/Yshiti3x.png"), "选择Y位置相同的所有实体");
            connect(entity_y, &QAction::triggered, this, [this]() {mSelectMenuFunction::getInstance()->slot_sameLocationGeoSelect(1);
                }
            );
            auto entity_z = locateMenu->addAction(QIcon(":/mMainWidgets/Icon/choice/Zshiti3x.png"), "选择Z位置相同的所有实体");
            connect(entity_y, &QAction::triggered, this, [this]() {mSelectMenuFunction::getInstance()->slot_sameLocationGeoSelect(2);
                }
            );
            auto common_x = locateMenu->addAction(QIcon(":/mMainWidgets/Icon/choice/XGMshiti3x.png"), "选择具有相同X位置的所有共面实体");
            connect(entity_x, &QAction::triggered, this, [this]() {mSelectMenuFunction::getInstance()->slot_sameLocationFaceGeoSelect(0);
                }
            );
            auto common_y = locateMenu->addAction(QIcon(":/mMainWidgets/Icon/choice/YGMshiti3x.png"), "选择具有相同Y位置的所有共面实体");
            connect(entity_y, &QAction::triggered, this, [this]() {mSelectMenuFunction::getInstance()->slot_sameLocationFaceGeoSelect(1);
                }
            );
            auto common_z = locateMenu->addAction(QIcon(":/mMainWidgets/Icon/choice/ZGMshiti3x.png"), "选择具有相同Z位置的所有共面实体");
            connect(entity_y, &QAction::triggered, this, [this]() {mSelectMenuFunction::getInstance()->slot_sameLocationFaceGeoSelect(2);
                }
            );


            auto selectEntitiesBtn(locateGW->addToolBtn("基于已选选择实体", ":/mMainWidgets/Icon/choice/JYYXZST3x.png", 0, 0, 1, 1, QSize(20, 20)));//大于 等于 小于    JYYXZST3x.png
            setQToolButton(selectEntitiesBtn, "基于已选选择实体", ":/mMainWidgets/Icon/choice/JYYXZST3x.png", "选择模型中大于、等于\n"
                "或小于当前选择的\n"
                "所有几何实体。", Qt::ToolButtonTextBesideIcon);


            auto comboBox = locateGW->addCombobox("select1", 0, 1);
            comboBox->addItems({ "大于", "等于", "小于" });
            comboBox->setMinimumWidth(64);  // 18px 下拉箭头 + 两文字 + 内边距
            comboBox->setFixedHeight(UiMetrics::menuItemHeight());
            comboBox->setCurrentIndex(1);

            connect(selectEntitiesBtn, &QToolButton::pressed, this, [this, comboBox]() {
                mSelectMenuFunction::getInstance()->slot_sizeGeoSelect(comboBox->currentIndex());
                }
            );


            auto dimensionBtn(locateGW->addToolBtn("基于尺寸选择实体", ":/mMainWidgets/Icon/choice/JYCCXZST3x.png", 1, 0, 1, 1, QSize(20, 20)));//大于 等于 小于 xx尺寸  JYCCXZST3x.png
            setQToolButton(dimensionBtn, "基于尺寸选择实体", ":/mMainWidgets/Icon/choice/JYCCXZST3x.png", "选择模型中大于、等于\n"
                "或小于当前设定尺寸的\n"
                "所有几何实体。", Qt::ToolButtonTextBesideIcon);


            comboBox = locateGW->addCombobox("select2", 1, 1);
            comboBox->addItems({ "几何线", "几何面", "几何体", });
            comboBox->setMinimumWidth(76);  // 18px 下拉箭头 + 三文字 + 内边距
            comboBox->setFixedHeight(UiMetrics::menuItemHeight());

            connect(comboBox, &MxFunctions::mCheckComboBox::currentTextChanged, this, [this](const QString& text) {
                if (text == "几何线")
                {
                    mMechGlobalSignals::getInstance()->activePickSig(2);
                }
                else if (text == "几何面")
                {
                    mMechGlobalSignals::getInstance()->activePickSig(3);
                }
                else
                {
                    mMechGlobalSignals::getInstance()->activePickSig(4);
                }
                }
            );

            auto lineEdit1 = new mCalcLineEdit();
            locateGW->addLineEdit(lineEdit1, 1, 2);
            lineEdit1->setMinimumWidth(56);  // 数值输入最小宽度
            lineEdit1->setFixedHeight(UiMetrics::menuItemHeight());

            auto comboBox1 = locateGW->addCombobox("select3", 1, 3);
            comboBox1->addItems({ "大于", "等于", "小于", "之间" });
            comboBox1->setCurrentIndex(3);
            comboBox1->setMinimumWidth(64);  // 18px 下拉箭头 + 两文字 + 内边距
            comboBox1->setFixedHeight(UiMetrics::menuItemHeight());

            connect(comboBox1, &MxFunctions::mCheckComboBox::currentTextChanged, this, [this, comboBox1, lineEdit1](const QString& text) {
                if (text == "之间")
                {
                    lineEdit1->setEnabled(true);
                }
                else
                {
                    lineEdit1->setEnabled(false);
                }
                }
            );

            auto lineEdit2 = new mCalcLineEdit();
            locateGW->addLineEdit(lineEdit2, 1, 4);
            lineEdit2->setMinimumWidth(56);  // 数值输入最小宽度
            lineEdit2->setFixedHeight(UiMetrics::menuItemHeight());

            connect(dimensionBtn, &QToolButton::pressed, this, [this, lineEdit1, lineEdit2, comboBox, comboBox1]() {
                mSelectMenuFunction::getInstance()->slot_sizeGeoSelect(comboBox->currentText(), comboBox1->currentIndex(), lineEdit1->text().toDouble(), lineEdit2->text().toDouble());
                }
            );

            //exchangeGW->updateLayout();
        }

        /*
        *网格
        */
        if (griddingGW)
        {

            auto growElesBtn(griddingGW->addToolBtn("增长单元", ":/mMainWidgets/Icon/choice/zengzhangdanyuan3x.png", -1, -1, 1, 1, QSize(30, 30)));
            setQToolButton(growElesBtn, "增长单元", ":/mMainWidgets/Icon/choice/zengzhangdanyuan3x.png", "选择与当前单元选择相邻的所有单元,该\n"
                "选项将单元选择增加了一层相邻的单元。");
            connect(growElesBtn, &QToolButton::pressed, mSelectMenuFunction::getInstance(), &mSelectMenuFunction::slot_growEles);

            auto nodeIdBtn(griddingGW->addToolBtn("按照Id选择节点", ":/mMainWidgets/Icon/choice/IdChoicePoint3x.png", 0, 0, 1, 1, QSize(20, 20)));
            setQToolButton(nodeIdBtn, "按照Id选择节点", ":/mMainWidgets/Icon/choice/IdChoicePoint3x.png", "通过节点ID选中节点。", Qt::ToolButtonTextBesideIcon);
            connect(nodeIdBtn, &QToolButton::pressed, mSelectMenuFunction::getInstance(), &mSelectMenuFunction::slot_nodeIDSelect);

            mSelectMenuFunction::getInstance()->getNodeInputLineEdit()->setParent(griddingGW.get());
            griddingGW->addLineEdit(mSelectMenuFunction::getInstance()->getNodeInputLineEdit(), 0, 1);
            mSelectMenuFunction::getInstance()->getNodeInputLineEdit()->setMinimumWidth(110);  // 设置最小宽度
            mSelectMenuFunction::getInstance()->getNodeInputLineEdit()->setFixedHeight(UiMetrics::menuItemHeight());

            auto eleIdBtn(griddingGW->addToolBtn("按照Id选择单元", ":/mMainWidgets/Icon/choice/IdChoiceUnit3x.png", 1, 0, 1, 1, QSize(20, 20)));
            setQToolButton(eleIdBtn, "按照Id选择单元", ":/mMainWidgets/Icon/choice/IdChoiceUnit3x.png", "通过单元ID选中单元。", Qt::ToolButtonTextBesideIcon);
            connect(eleIdBtn, &QToolButton::pressed, mSelectMenuFunction::getInstance(), &mSelectMenuFunction::slot_eleIDSelect);

            mSelectMenuFunction::getInstance()->getEleInputLineEdit()->setParent(griddingGW.get());
            griddingGW->addLineEdit(mSelectMenuFunction::getInstance()->getEleInputLineEdit(), 1, 1);
            mSelectMenuFunction::getInstance()->getEleInputLineEdit()->setMinimumWidth(110);  // 设置最小宽度
            mSelectMenuFunction::getInstance()->getEleInputLineEdit()->setFixedHeight(UiMetrics::menuItemHeight());

            // 通过边角度
            auto bySidelineEdit = griddingGW->addDoubleSpinBox("通过边角度", 0, 3, 1, 1);
            bySidelineEdit->setValue(60);
            bySidelineEdit->setRange(0, 180);
            bySidelineEdit->setMinimumWidth(80);  // 设置最小宽度，与前面一致
            bySidelineEdit->setFixedHeight(UiMetrics::menuItemHeight());
            connect(bySidelineEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [](double value) {
                mSelectMenuFunction::getInstance()->setPickAssistLineAngle(value);
                });

            auto bySideBtn(griddingGW->addToolBtn("通过边角度", ":/mMainWidgets/Icon/choice/bianjiaodu.png", 0, 2, 1, 1, QSize(20, 20)));
            setQToolButton(bySideBtn, "通过边角度", ":/mMainWidgets/Icon/choice/bianjiaodu.png", "通过边角度选中。", Qt::ToolButtonTextBesideIcon);
            bySideBtn->setCheckable(true);

            // 通过面角度
            auto byFacelineEdit = griddingGW->addDoubleSpinBox("通过面角度", 1, 3, 1, 1);
            byFacelineEdit->setValue(60);
            byFacelineEdit->setRange(0, 180);
            byFacelineEdit->setMinimumWidth(80);
            byFacelineEdit->setFixedHeight(UiMetrics::menuItemHeight());
            connect(byFacelineEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [](double value) {
                mSelectMenuFunction::getInstance()->setPickAssistFaceAngle(value);
                });

            auto byFaceBtn(griddingGW->addToolBtn("通过面角度", ":/mMainWidgets/Icon/choice/mianjiaodu.png", 1, 2, 1, 1, QSize(20, 20)));
            setQToolButton(byFaceBtn, "通过面角度", ":/mMainWidgets/Icon/choice/mianjiaodu.png", "通过面角度选中。", Qt::ToolButtonTextBesideIcon);
            byFaceBtn->setCheckable(true);


            /* auto pathBtn(griddingGW->addToolBtn("路径", ":/mMainWidgets/Icon/choice/lujing.png", 0, 4, 1, 1, QSize(30, 30)));
             setQToolButton(pathBtn, "路径", ":/mMainWidgets/Icon/choice/lujing.png", "选择路径。", Qt::ToolButtonTextUnderIcon);
             pathBtn->setStyleSheet("QToolButton { margin-top: 5px; }");
             connect(pathBtn, &QToolButton::pressed, mSelectMenuFunction::getInstance(), &mSelectMenuFunction::slot_pathSelect);*/
            auto pathBtn(griddingGW->addToolBtn("路径", ":/mMainWidgets/Icon/choice/lujing.png", 0, 4, 2, 1, QSize(30, 30)));
            setQToolButton(pathBtn, "路径", ":/mMainWidgets/Icon/choice/lujing.png", "选择路径。", Qt::ToolButtonTextUnderIcon);
            // 全自绘按钮不挂 QSS：颜色/边距由 mUiStyle 自绘与 ThemeManager 统一收口，
            // 此处任何 setStyleSheet 都会在首次 applyAll 被 applyRibbonButtonTheme 清空
            if (auto* gridLayout = qobject_cast<QGridLayout*>(griddingGW->layout()))
                gridLayout->setAlignment(pathBtn, Qt::AlignCenter);
            pathBtn->setCheckable(true);

            //三个按钮可选中（互斥）：选中后每次视口拾取完成自动按对应逻辑拾取；再次点击取消选中恢复普通拾取
            connect(bySideBtn, &QToolButton::toggled, this, [bySideBtn, byFaceBtn, pathBtn, bySidelineEdit](bool on) {
                auto func = mSelectMenuFunction::getInstance();
                if (on)
                {
                    byFaceBtn->setChecked(false);
                    pathBtn->setChecked(false);
                    func->setPickAssistLineAngle(bySidelineEdit->value());
                    func->setPickAssistMode(mSelectMenuFunction::PickAssistMode::LineAngle);
                    func->applyPickAssist();//已有拾取时立即执行一次，保持原有点击行为
                }
                else if (func->getPickAssistMode() == mSelectMenuFunction::PickAssistMode::LineAngle)
                {
                    func->setPickAssistMode(mSelectMenuFunction::PickAssistMode::None);
                }
                });

            connect(byFaceBtn, &QToolButton::toggled, this, [bySideBtn, byFaceBtn, pathBtn, byFacelineEdit](bool on) {
                auto func = mSelectMenuFunction::getInstance();
                if (on)
                {
                    bySideBtn->setChecked(false);
                    pathBtn->setChecked(false);
                    func->setPickAssistFaceAngle(byFacelineEdit->value());
                    func->setPickAssistMode(mSelectMenuFunction::PickAssistMode::FaceAngle);
                    func->applyPickAssist();
                }
                else if (func->getPickAssistMode() == mSelectMenuFunction::PickAssistMode::FaceAngle)
                {
                    func->setPickAssistMode(mSelectMenuFunction::PickAssistMode::None);
                }
                });

            connect(pathBtn, &QToolButton::toggled, this, [bySideBtn, byFaceBtn, pathBtn](bool on) {
                auto func = mSelectMenuFunction::getInstance();
                if (on)
                {
                    bySideBtn->setChecked(false);
                    byFaceBtn->setChecked(false);
                    func->setPickAssistMode(mSelectMenuFunction::PickAssistMode::Path);
                    func->applyPickAssist();
                }
                else if (func->getPickAssistMode() == mSelectMenuFunction::PickAssistMode::Path)
                {
                    func->setPickAssistMode(mSelectMenuFunction::PickAssistMode::None);
                }
                });

            //locateGW->updateLayout();
        }
        /*
        * 转换
        */
        if (exchangeGW)
        {
            QList<QToolButton*> toolbuttons;

            auto pointsBtn(exchangeGW->addToolBtn("顶点", ":/mMainWidgets/Icon/choice/zhuan_jihedian.png", 1, 1, 1, 1, QSize(20, 20)));
            setQToolButton(pointsBtn, "顶点", ":/mMainWidgets/Icon/choice/zhuan_jihedian.png", "选择与当前选定的体、面、线、单元、单\n"
                "元线、单元面、节点相关联的所有几何点\n"
                "。选择模式自动更改为几何点选择。", Qt::ToolButtonTextBesideIcon);
            toolbuttons.append(pointsBtn);

            connect(pointsBtn, &QToolButton::pressed, this, []() {
                mSelectMenuFunction::getInstance()->slot_mappingGeoPoint();
                }
            );
            auto edgesBtn(exchangeGW->addToolBtn("边", ":/mMainWidgets/Icon/choice/zhuan_jihexian.png", 0, 1, 1, 1, QSize(20, 20)));
            setQToolButton(edgesBtn, "边", ":/mMainWidgets/Icon/choice/zhuan_jihexian.png", "选择与当前选定的体、面、顶点、单元、\n"
                "单元线、单元面、节点相关联的所有几何\n"
                "线。选择模式自动更改为几何线选择。", Qt::ToolButtonTextBesideIcon);
            toolbuttons.append(edgesBtn);

            connect(edgesBtn, &QToolButton::pressed, this, []() {
                mSelectMenuFunction::getInstance()->slot_mappingGeoEdge();
                }
            );

            auto facesBtn(exchangeGW->addToolBtn("面", ":/mMainWidgets/Icon/choice/zhuan_jihemian.png", 1, 0, 1, 1, QSize(20, 20)));
            setQToolButton(facesBtn, "面", ":/mMainWidgets/Icon/choice/zhuan_jihemian.png", "选择与当前选定的体、线、顶点、单元、\n"
                "单元线、单元面、节点相关联的所有几何\n"
                "面。选择模式自动更改为几何面选择。", Qt::ToolButtonTextBesideIcon);
            toolbuttons.append(facesBtn);

            connect(facesBtn, &QToolButton::pressed, this, []() {
                mSelectMenuFunction::getInstance()->slot_mappingGeoFace();
                }
            );

            auto bodiesBtn(exchangeGW->addToolBtn("几何体", ":/mMainWidgets/Icon/choice/zhuan_jiheti.png", 0, 0, 1, 1, QSize(20, 20)));
            setQToolButton(bodiesBtn, "几何体", ":/mMainWidgets/Icon/choice/zhuan_jiheti.png", "选择与当前选定的面、线、顶点、单元、\n"
                "单元线、单元面、节点相关联的所有几何\n"
                "体。选择模式自动更改为几何体选择。", Qt::ToolButtonTextBesideIcon);
            toolbuttons.append(bodiesBtn);

            connect(bodiesBtn, &QToolButton::pressed, this, []() {
                mSelectMenuFunction::getInstance()->slot_mappingGeoSolid();
                }
            );

            auto nodesBtn(exchangeGW->addToolBtn("节点", ":/mMainWidgets/Icon/choice/zhuan_jiedian.png", 1, 3, 1, 1, QSize(20, 20)));
            setQToolButton(nodesBtn, "节点", ":/mMainWidgets/Icon/choice/zhuan_jiedian.png", "选择与当前选定的体、面、线、顶点、单\n"
                "元、单元线、单元面相关联的所有节点。\n"
                "选择模式自动更改为节点选择。", Qt::ToolButtonTextBesideIcon);
            toolbuttons.append(nodesBtn);

            connect(nodesBtn, &QToolButton::pressed, this, []() {
                mSelectMenuFunction::getInstance()->slot_mappingNode();
                }
            );

            auto eleEdgesBtn(exchangeGW->addToolBtn("单元边", ":/mMainWidgets/Icon/choice/zhuan_danyuanbian.png", 0, 3, 1, 1, QSize(20, 20)));
            setQToolButton(eleEdgesBtn, "单元边", ":/mMainWidgets/Icon/choice/zhuan_danyuanbian.png", "选择与当前选定的体、面、线、顶点、单\n"
                "元、单元面、节点相关联的所有单元线。\n"
                "选择模式自动更改为单元线选择。", Qt::ToolButtonTextBesideIcon);
            toolbuttons.append(eleEdgesBtn);

            connect(eleEdgesBtn, &QToolButton::pressed, this, []() {
                mSelectMenuFunction::getInstance()->slot_mappingMeshLine();
                }
            );

            auto eleFacesBtn(exchangeGW->addToolBtn("单元面", ":/mMainWidgets/Icon/choice/zhuan_danyuanmian.png", 1, 2, 1, 1, QSize(20, 20)));
            setQToolButton(eleFacesBtn, "单元面", ":/mMainWidgets/Icon/choice/zhuan_danyuanmian.png", "选择与当前选定的体、面、线、顶点、单\n"
                "元线、单元、节点相关联的所有单元面。\n"
                "选择模式自动更改为单元面选择。", Qt::ToolButtonTextBesideIcon);
            toolbuttons.append(eleFacesBtn);

            connect(eleFacesBtn, &QToolButton::pressed, []() {
                mSelectMenuFunction::getInstance()->slot_mappingMeshFace();
                });

            auto elesBtn(exchangeGW->addToolBtn("单元", ":/mMainWidgets/Icon/choice/zhuan_danyuan.png", 0, 2, 1, 1, QSize(20, 20)));
            setQToolButton(elesBtn, "单元", ":/mMainWidgets/Icon/choice/zhuan_danyuan.png", "选择与当前选定的体、面、线、顶点、单\n"
                "元线、单元面、节点相关联的所有单元。\n"
                "选择模式自动更改为单元选择。", Qt::ToolButtonTextBesideIcon);
            toolbuttons.append(elesBtn);

            connect(elesBtn, &QToolButton::pressed, []() {
                mSelectMenuFunction::getInstance()->slot_mappingEle();
                });

            auto partsBtn(exchangeGW->addToolBtn("部件", ":/mMainWidgets/Icon/choice/zhuan_bujian.png", 0, 4, 1, 1, QSize(20, 20)));
            setQToolButton(partsBtn, "部件", ":/mMainWidgets/Icon/choice/zhuan_bujian.png", "选择与当前选定的体、面、线、顶点、单元、单\n"
                "元线、单元面、节点相关联的所有部件。\n"
                "选择模式自动更改为部件选择。", Qt::ToolButtonTextBesideIcon);
            toolbuttons.append(partsBtn);

            connect(partsBtn, &QToolButton::pressed, this, []() {
                mSelectMenuFunction::getInstance()->slot_mappingPart();
                }
            );

            //dimensionGW->updateLayout();
        }
        /*
        * 批量操作
        */
        if (batchGW)
        {

            //auto allBtn(batchGW->addToolBtn("全部", ":/mMainWidgets/Icon/choice/fanxiang3x.png", 0, 1, 2, 1, QSize(30, 30)));
            //setQToolButton(allBtn, "全部", ":/mMainWidgets/Icon/choice/fanxiang3x.png", "选中当前未选定的相同类\n"
            //    "型的实体(例如面、边等)。");


            auto invertBtn(batchGW->addToolBtn("反向", ":/mMainWidgets/Icon/choice/fanxiang3x.png", 0, 0, 2, 1, QSize(30, 30)));
            setQToolButton(invertBtn, "反向", ":/mMainWidgets/Icon/choice/fanxiang3x.png", "选中当前未选定的相同类\n"
                "型的实体(例如面、边等)。");
            connect(invertBtn, &QToolButton::pressed, mSelectMenuFunction::getInstance(), &mSelectMenuFunction::slot_reverse);


            auto clearBtn(batchGW->addToolBtn("清空", ":/mMainWidgets/Icon/choice/clear.png", 0, 1, 2, 1, QSize(30, 30)));
            setQToolButton(clearBtn, "清空", ":/mMainWidgets/Icon/choice/clear.png", "清空所有拾取对象。");

            connect(clearBtn, &QToolButton::pressed, this, []() {
                if (!getCVM)
                {
                    return;
                }
                getCVM->clearPick();
                }
            );
        }

        //connect(, &QToolButton::pressed, this, [this, comboBox]() {
        //	mSelectMenuFunction::getInstance()->slot_sizeGeoSelect(comboBox->currentIndex());
        //	}
        //);
    }
    QMenu* mMenuWidget::getCaseMenu()
    {
        // 分组标题使用独立的禁用动作（与单位制菜单一致，mUiMenu 原生自绘标题行，
        // 避免 QWidgetAction 宿主的行命中/样式刷新问题）
        auto addTitle = [](QMenu* menu, const QString& text) {
            QAction* title = new QAction(text, menu);
            title->setEnabled(false);
            title->setProperty("mxMenuSectionTitle", true);
            menu->addAction(title);
            };

        QMenu* caseMenu = new mUiMenu(this);
        ThemeManager::applyMenuStyle(caseMenu);

        //添加工况菜单项（图标与分析工况页各工况按钮一致）
        auto addCaseAction = [caseMenu](QActionGroup* group, const QString& text, const QString& icon, CaseType type) {
            QAction* act = caseMenu->addAction(QIcon(icon), text);
            act->setData((int)type);
            group->addAction(act);
            };

        addTitle(caseMenu, "静态分析");
        QActionGroup* staticGroup = new QActionGroup(caseMenu);
        staticGroup->setExclusive(false);
        addCaseAction(staticGroup, "线性静力学分析", ":/mMainWidgets/Icon/CaseType/linearStatic.svg", ImplicitLinearStatic);
        addCaseAction(staticGroup, "非线性静力学分析", ":/mMainWidgets/Icon/CaseType/nonlinearStatic.svg", ImplicitNonLinearStatic);

        addTitle(caseMenu, "隐式动力学");
        QActionGroup* dynamicGroup = new QActionGroup(caseMenu);
        dynamicGroup->setExclusive(false);
        addCaseAction(dynamicGroup, "非线性瞬态分析", ":/mMainWidgets/Icon/CaseType/nonlinearTransient.svg", ImplicitNonLinearDynamic);

        addTitle(caseMenu, "稳定性分析");
        QActionGroup* stableGroup = new QActionGroup(caseMenu);
        stableGroup->setExclusive(false);
        addCaseAction(stableGroup, "线性屈曲分析", ":/mMainWidgets/Icon/CaseType/linearBuckling.svg", ImplicitLinearBuckling);
        addCaseAction(stableGroup, "后屈曲分析", ":/mMainWidgets/Icon/CaseType/postBuckling.svg", ImplicitPostBuckling);

        addTitle(caseMenu, "传热分析");
        QActionGroup* heatGroup = new QActionGroup(caseMenu);
        heatGroup->setExclusive(false);
        //0630版本屏蔽：结构耦合传热分析（ImplicitStructuralCouplingHeat）
        addCaseAction(heatGroup, "稳态传热分析", ":/mMainWidgets/Icon/CaseType/steadyHeat.svg", ImplicitSteadyHeat);
        addCaseAction(heatGroup, "瞬态传热分析", ":/mMainWidgets/Icon/CaseType/transientHeat.svg", ImplicitTransientHeat);

        addTitle(caseMenu, "线性动态分析");
        QActionGroup* linearDynamicGroup = new QActionGroup(caseMenu);
        linearDynamicGroup->setExclusive(false);
        addCaseAction(linearDynamicGroup, "模态分析", ":/mMainWidgets/Icon/CaseType/modal.svg", ImplicitModal);
        addCaseAction(linearDynamicGroup, "响应谱分析", ":/mMainWidgets/Icon/CaseType/responseSpectrum.svg", ImplicitResponseSpectrum);
        addCaseAction(linearDynamicGroup, "随机振动分析", ":/mMainWidgets/Icon/CaseType/randomVibration.svg", ImplicitRandomVibration);
        addCaseAction(linearDynamicGroup, "频响分析", ":/mMainWidgets/Icon/CaseType/frequencyResponse.svg", ImplicitFrequency);
        addCaseAction(linearDynamicGroup, "线性瞬态分析", ":/mMainWidgets/Icon/CaseType/linearTransient.svg", ImplicitTransient);
        addCaseAction(linearDynamicGroup, "声固耦合分析", ":/mMainWidgets/Icon/CaseType/acousticCoupling.svg", ASCoupling);
        addCaseAction(linearDynamicGroup, "超单元分析", ":/mMainWidgets/Icon/CaseType/superElement.svg", ImplicitGeneric);
        connect(staticGroup, &QActionGroup::triggered, this, [this](QAction* act) {
            int idx = act->data().toInt();
            slot_createCase((CaseType)idx);
            });

        connect(dynamicGroup, &QActionGroup::triggered, this, [this](QAction* act) {
            int idx = act->data().toInt();
            slot_createCase((CaseType)idx);
            });

        connect(stableGroup, &QActionGroup::triggered, this, [this](QAction* act) {
            int idx = act->data().toInt();
            slot_createCase((CaseType)idx);
            });

        connect(heatGroup, &QActionGroup::triggered, this, [this](QAction* act) {
            int idx = act->data().toInt();
            slot_createCase((CaseType)idx);
            });

        connect(linearDynamicGroup, &QActionGroup::triggered, this, [this](QAction* act) {
            int idx = act->data().toInt();
            slot_createCase((CaseType)idx);
            });

        return caseMenu;
    }

    QMenu* mMenuWidget::getSetMenu()
    {
        //集合创建下拉菜单（类型与“集合”页创建组一致；pickType为拾取类型，withName为按名称创建的集合）
        QMenu* setMenu = new mUiMenu(this);
        ThemeManager::applyMenuStyle(setMenu);

        auto addSetAction = [this, setMenu](const QString& text, const QString& icon, int pickType, const QString& withName = QString()) {
            setMenu->addAction(QIcon(icon), text, this, [this, pickType, withName]() {
                if (pickType > 0)
                    mMechGlobalSignals::getInstance()->activePickSig(pickType);
                if (!withName.isEmpty())
                    slot_createSet_withName(withName);
                else
                    slot_createSet();
                });
            };

        addSetAction("几何点集", ":/mMainWidgets/Icon/choice/dingdian3x.png", 1);
        addSetAction("几何线集", ":/mMainWidgets/Icon/choice/bian3x.png", 2);
        addSetAction("几何面集", ":/mMainWidgets/Icon/choice/mian3x.png", 3);
        addSetAction("几何体集", ":/mMainWidgets/Icon/choice/jiheti3x.png", 4);
        addSetAction("节点集", ":/mMainWidgets/Icon/choice/jiedian3x.png", 5);
        addSetAction("单元边集", ":/mMainWidgets/Icon/choice/danyuanbian3x.png", 6);
        addSetAction("解析表面集", ":/mMainWidgets/Icon/choice/jiexibiaomian3x.png", 6, "解析表面集");
        addSetAction("单元面集", ":/mMainWidgets/Icon/choice/danyuanmian3x.png", 7);
        addSetAction("单元集", ":/mMainWidgets/Icon/choice/danyuan3x.png", 8);
        addSetAction("部件集", ":/mMainWidgets/Icon/choice/bujian3x.png", 9);
        addSetAction("频率点集", ":/mMainWidgets/Icon/choice/freqPoint@3x.png", 0, "频率点集");
        addSetAction("自由度节点集", ":/mMainWidgets/Icon/choice/dofNode@3x.png", 6, "自由度节点集");
        addSetAction("面板集", ":/mMainWidgets/Icon/choice/panel@3x.png", 0, "面板集");
        addSetAction("节点面集", ":/mMainWidgets/Icon/choice/jiedianmian.png", 0, "节点面集");

        return setMenu;
    }


    QMenu* mMenuWidget::getInteractionMenu()
    {
        // 分组标题使用独立的禁用动作（与单位制菜单一致，mUiMenu 原生自绘标题行，
        // 避免 QWidgetAction 宿主的行命中/样式刷新问题）
        auto addTitle = [](QMenu* menu, const QString& text) {
            QAction* title = new QAction(text, menu);
            title->setEnabled(false);
            title->setProperty("mxMenuSectionTitle", true);
            menu->addAction(title);
            };
        QStringList connectorlist = {
            //"弹簧/阻尼",
            "绑定",
            "刚性连接",
            "柔性连接",
            "MPC约束",
            //"连接搜索",
            "离散刚体",
            "螺栓约束",
            "解析刚体",
            "分布式耦合"
        };
        QStringList deputylist = {
            "连接器",
            "fastener",
        };
        QStringList contactlist = {
            "面面接触",
            "点面接触",
            "线面接触",
            "通用接触",
            "自接触",
            "过盈接触",
            "二维接触",
            //"接触对搜索",
        };

        QVector< MxFunctions::ContactType>contacttypelist = {
            SurfaceToSurfaceContact,
            NodeToSurfaceContact,
            LineToSurfaceContact,
            CommonContact,
            SelfContact,
            InterferenceContact,
            TwoDimensionalContact,
        };
        QStringList periodicboundarylist = {
            "平移边界",
            "旋转边界",
        };
        QVector< MxFunctions::PeriodicBoundaryType> periodicboundarytypelist = {
            MxFunctions::PeriodicBoundary_Translation,
            MxFunctions::PeriodicBoundary_Rotation,
        };
        QMenu* InteractionMenu = new mUiMenu(this);
        ThemeManager::applyMenuStyle(InteractionMenu);
        addTitle(InteractionMenu, "连接");
        QActionGroup* staticGroup = new QActionGroup(InteractionMenu);
        staticGroup->setExclusive(false);
        for (auto str : connectorlist) {
            QAction* act = InteractionMenu->addAction(str);
            if (str == "绑定")
            {
                connect(act, &QAction::triggered, this, [this]() {
                    slot_createConnector(MxFunctions::ConnectorType::Connector_Bind);
                    });
            }
            else if (str == "刚性连接")
            {
                connect(act, &QAction::triggered, this, [this]() {
                    slot_createConnector(MxFunctions::ConnectorType::Connector_Rigid);
                    });
            }
            else if (str == "柔性连接")
            {
                connect(act, &QAction::triggered, this, [this]() {
                    slot_createConnector(MxFunctions::ConnectorType::Connector_Flexible);
                    });
            }
            else if (str == "MPC约束")
            {
                connect(act, &QAction::triggered, this, [this]() {
                    slot_createConnector(MxFunctions::ConnectorType::Connector_MPC);
                    });
            }
            else if (str == "离散刚体")
            {
                connect(act, &QAction::triggered, this, [this]() {
                    slot_createConnector(MxFunctions::ConnectorType::Connector_Disperse);
                    });
            }
            else if (str == "螺栓约束")
            {
                connect(act, &QAction::triggered, this, [this]() {
                    slot_createConnector(MxFunctions::ConnectorType::Connector_Bolt);
                    });
            }
            else if (str == "解析刚体")
            {
                connect(act, &QAction::triggered, this, [this]() {
                    slot_createConnector(MxFunctions::ConnectorType::Connector_Rigidbody);
                    });
            }
            else if (str == "分布式耦合")
            {
                connect(act, &QAction::triggered, this, [this]() {
                    slot_createConnector(MxFunctions::ConnectorType::Connector_DistributedCoupling);
                    });
            }
            else if (str == "连接搜索")
            {
                connect(act, &QAction::triggered, this, [this]() {
                    mInfoWindow::getInstance()->searchConnector();
                    });
            }
            staticGroup->addAction(act);
        }

        addTitle(InteractionMenu, "连接副");
        QActionGroup* dynamicGroup = new QActionGroup(InteractionMenu);
        dynamicGroup->setExclusive(false);
        for (auto str : deputylist) {
            QAction* act = InteractionMenu->addAction(str);
            connect(act, &QAction::triggered, this, [this, str]() {
                if (str == "fastener")
                    slot_createDeputy(MxFunctions::DeputyType::Deputy_Fastener);
                else
                    slot_createDeputy(MxFunctions::DeputyType::Deputy_Connector);
                });
            dynamicGroup->addAction(act);
        }

        addTitle(InteractionMenu, "接触");
        QActionGroup* dynamicGroup2 = new QActionGroup(InteractionMenu);
        dynamicGroup2->setExclusive(false);
		for (int i = 0; i < contactlist.size();++i) {
            QString str = contactlist[i];
            QAction* act = InteractionMenu->addAction(str);
            if (str == "接触对搜索")
            {
                connect(act, &QAction::triggered, this, [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->editContactPairPanel();
                    });
            }
            else
            {
				connect(act, &QAction::triggered, this, [this,contacttypelist,i]() {
                    slot_createContact(contacttypelist[i]);
                    });
            }
            dynamicGroup2->addAction(act);
        }

        addTitle(InteractionMenu, "周期性边界");
        QActionGroup* periodicBoundaryGroup = new QActionGroup(InteractionMenu);
        periodicBoundaryGroup->setExclusive(false);
        for (int i = 0; i < periodicboundarylist.size(); ++i) {
            QString str = periodicboundarylist[i];
            QAction* act = InteractionMenu->addAction(str);
            connect(act, &QAction::triggered, this, [this, periodicboundarytypelist, i]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createPeriodicBoundary(periodicboundarytypelist[i]);
                });
            periodicBoundaryGroup->addAction(act);
        }

        return InteractionMenu;
    }

    QMenu* mMenuWidget::getUnitsMenu()
    {
        // 分组标题使用独立的禁用动作，避免 QWidgetAction 宿主被后续命令行覆盖。
        auto addTitle = [](QMenu* menu, const QString& text) {
            QAction* title = new QAction(text, menu);
            title->setEnabled(false);
            title->setProperty("mxMenuSectionTitle", true);
            menu->addAction(title);
            };
        QStringList unitslist = {
                "度量标准(m、kg、N、s、V、A)",
                "度量标准(cm、g、dyne、s、V、A)",
                "度量标准(mm、kg、N、s、mV、mA)",
                "度量标准(mm、t、N、s、mV、mA)",
                "度量标准(mm、dat、N、s、mV、mA)",
                "度量标准(μm、kg、μN、s、V、mA)",
                "美国惯用单位(ft、lbm、lbf、°F、s、V、A)",
                "美国惯用单位(in、lbm、lbf、°F、s、V、A)",
        };
        QStringList anglelist = {
            "度",
            "弧度",
        };
        QStringList speedlist = {
            "rad",
            "RPM",
        };
        QStringList templist = {
            "摄氏度",
            "开尔文",
        };
        // 菜单一站式装配（规则 1.0）：统一样式表 + mUiStyle 自绘行内容
        QMenu* unitsMenu = new mUiMenu(this);
        ThemeManager::applyMenuStyle(unitsMenu);
        addTitle(unitsMenu, "单位系统");
        QActionGroup* unitsGroup = new QActionGroup(unitsMenu);
        unitsGroup->setExclusive(true);
        for (int i = 0; i < unitslist.size(); ++i) {
            QAction* act = unitsMenu->addAction(unitslist[i]);
            act->setData(i);
            act->setCheckable(true);
            act->setChecked(i == 3);
            unitsGroup->addAction(act);
        }

        addTitle(unitsMenu, "角度");
        QActionGroup* angleGroup = new QActionGroup(unitsMenu);
        angleGroup->setExclusive(true);
        for (int i = 0; i < anglelist.size(); ++i) {
            QAction* act = unitsMenu->addAction(anglelist[i]);
            act->setData(i);
            act->setCheckable(true);
            act->setChecked(i == 0);
            angleGroup->addAction(act);
        }

        addTitle(unitsMenu, "转速");
        QActionGroup* speedGroup = new QActionGroup(unitsMenu);
        speedGroup->setExclusive(true);
        for (int i = 0; i < speedlist.size(); ++i) {
            QAction* act = unitsMenu->addAction(speedlist[i]);
            act->setData(i);
            act->setCheckable(true);
            act->setChecked(i == 0);
            speedGroup->addAction(act);
        }

        addTitle(unitsMenu, "温度");
        QActionGroup* tempGroup = new QActionGroup(unitsMenu);
        tempGroup->setExclusive(true);
        for (int i = 0; i < templist.size(); ++i) {
            QAction* act = unitsMenu->addAction(templist[i]);
            act->setData(i);
            act->setCheckable(true);
            act->setChecked(i == 0);
            tempGroup->addAction(act);
        }

        connect(unitsGroup, &QActionGroup::triggered, this, [this, tempGroup](QAction* act) {
            int idx = act->data().toInt();
            bool changedata = idx != getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[0];
            getCVM->getModelData()->getUnitSystemManager()->switchUnits(idx);
            if (idx > 5)
            {
                getCVM->getModelData()->getUnitSystemManager()->swicthTemp(2);
                getCVM->getModelData()->updateDataFromUnitSystem(3);
                tempGroup->setDisabled(true);
            }
            else
            {
                //切回原来的温度单位
                getCVM->getModelData()->getUnitSystemManager()->swicthTemp(tempGroup->checkedAction()->data().toInt());
                getCVM->getModelData()->updateDataFromUnitSystem(3);
                tempGroup->setDisabled(false);
            }
            if (changedata)
                getCVM->getModelData()->updateDataFromUnitSystem(0);
            mInfoWindow::getInstance()->updateAllPanelUnits();
            emit mMechGlobalSignals::getInstance()->clikNodesOnTree(false);
            });
        connect(angleGroup, &QActionGroup::triggered, this, [this](QAction* act) {
            int idx = act->data().toInt();
            bool changedata = idx != getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[1];
            getCVM->getModelData()->getUnitSystemManager()->switchAngle(idx);
            if (changedata)
                getCVM->getModelData()->updateDataFromUnitSystem(1);
            mInfoWindow::getInstance()->updateAllPanelUnits();
            emit mMechGlobalSignals::getInstance()->clikNodesOnTree(false);

            });
        connect(speedGroup, &QActionGroup::triggered, this, [this](QAction* act) {
            int idx = act->data().toInt();
            bool changedata = idx != getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[2];
            getCVM->getModelData()->getUnitSystemManager()->switchSpeed(idx);
            if (changedata)
                getCVM->getModelData()->updateDataFromUnitSystem(2);
            mInfoWindow::getInstance()->updateAllPanelUnits();
            emit mMechGlobalSignals::getInstance()->clikNodesOnTree(false);

            });
        connect(tempGroup, &QActionGroup::triggered, this, [this](QAction* act) {
            int idx = act->data().toInt();
            bool changedata = idx != getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[3];
            getCVM->getModelData()->getUnitSystemManager()->swicthTemp(idx);
            if (changedata)
                getCVM->getModelData()->updateDataFromUnitSystem(3);
            mInfoWindow::getInstance()->updateAllPanelUnits();
            emit mMechGlobalSignals::getInstance()->clikNodesOnTree(false);

            });
        //UI同步
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::unitSystemChanged, this, [this, unitsGroup, unitsMenu](int index) {
            if (auto* act = unitsGroup->actions().value(index))
            {
                if (act->isChecked())
                {
                    return;
                }
                else
                {
                    act->setChecked(true);
                    unitsMenu->triggered(act);
                }
            }
            });
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::angleUnitChanged, this, [this, angleGroup, unitsMenu](int index) {
            if (auto* act = angleGroup->actions().value(index))
            {
                if (act->isChecked())
                {
                    return;
                }
                else
                {
                    act->setChecked(true);
                    unitsMenu->triggered(act);
                }
            }
            });
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::speedUnitChanged, this, [this, speedGroup, unitsMenu](int index) {
            if (auto* act = speedGroup->actions().value(index))
            {
                if (act->isChecked())
                {
                    return;
                }
                else
                {
                    act->setChecked(true);
                    unitsMenu->triggered(act);
                }
            }
            });
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::tempUnitChanged, this, [this, tempGroup, unitsMenu](int index) {
            tempGroup->setDisabled(index == 2);
            if (auto* act = tempGroup->actions().value(index))
            {
                if (act->isChecked())
                {
                    return;
                }
                else
                {
                    act->setChecked(true);
                    unitsMenu->triggered(act);
                }
            }
            });
        return unitsMenu;
    }


    void mMenuWidget::slot_browsertreeSelectedChanged(const QList<mItemKey>& items, bool valid)
    {
        //判断当前选中的第二个选项卡
        QString title = m_menuTabBar_->tabText(1);

        //集合菜单栏
        if (title == "集合")
        {
            auto buttonList = _setGW.at(1)->getButtons();
            if (valid)
            {
                _setGW.at(1)->setEnabled(true);
                for (int i = 0; i < 9; i++)
                {
                    buttonList.at(i)->setEnabled(true);
                }
            }
            else
            {
                _setGW.at(1)->setEnabled(false);
                return;
            }
            //扫描item
            for (int i = 0; i < items.size(); i++)
            {
                auto item = items.at(i);
                int setID = item._phyID;
                auto setData = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setID);
                if (setData == nullptr)
                {
                    return;
                }
                //根据集合类型和数据更新面板
                auto type = setData->getSetType();
                switch (type)
                {
                case MxFunctions::GeoPoint:buttonList.at(0)->setEnabled(false);
                    break;
                case MxFunctions::GeoEdge:buttonList.at(1)->setEnabled(false);
                    break;
                case MxFunctions::GeoFace:buttonList.at(2)->setEnabled(false);
                    break;
                case MxFunctions::GeoSolid:buttonList.at(3)->setEnabled(false);
                    break;
                case MxFunctions::Element:buttonList.at(7)->setEnabled(false);
                    break;
                case MxFunctions::Node:buttonList.at(4)->setEnabled(false);
                    break;
                case MxFunctions::Part:buttonList.at(8)->setEnabled(false);
                    break;
                case MxFunctions::ElementSurface:buttonList.at(6)->setEnabled(false);
                    break;
                case MxFunctions::ElementEdge:buttonList.at(5)->setEnabled(false);
                    break;
                case MxFunctions::DOFNode:
                case MxFunctions::FrequencyPoint:
                case MxFunctions::Panel:
                //解析表面集/节点面集不支持转换（convertFunc无对应数据装载分支，
                //误点会删除原集合并重建空集合），与上面三类一样全部置灰
                case MxFunctions::SurfaceAnalysis:
                case MxFunctions::NodeSurface:
                {
                    for (int i = 0; i <= 8; i++)
                    {
                        buttonList.at(i)->setEnabled(false);
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
        else if (title == "坐标系")
        {
            _transformList->clear();
            _coordinateGW.at(1)->setEnabled(false);
            if (!valid)
            {
                return;
            }
            if (items.size() == 1)
            {
                auto treeItem = items.at(0);
                int coordinateID = treeItem._phyID;
                if (coordinateID == -1)
                {
                    _coordinateGW.at(1)->setEnabled(false);
                    return;
                }
                auto coordinateData = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(coordinateID);
                if (coordinateData == nullptr)
                {
                    return;
                }
                _coordinateGW.at(1)->setEnabled(true);
                for (int i = 0; i < coordinateData->getTransformQueue().size(); ++i)
                {
                    QString type;
                    QString value;
                    coordinateData->getTransformStep(i, type, value);
                    addCoordinateTransform(type, value, coordinateID);
                }

            }

        }
        // 载荷/边界按钮的工况过滤与当前页签无关：树选中变化时始终同步，
        // 保证随后切到“载荷”/“边界”页签时按钮可用状态已正确
        // （原实现依赖 tabText(1) 恰好是载荷/边界页，默认页签顺序下索引1是“模型”，过滤从未生效）
        {
            enum class ManagerFilterType
            {
                None,
                LoadManager,      // 载荷管理器：力学载荷 + 热学载荷 + 动态载荷 + 符号
                BoundaryManager,  // 边界管理器：边界条件 + 符号 + 子模型边界
                PredefinedFieldManager // 预定义场管理器：预定义场 + 符号
            };
            ManagerFilterType managerFilter = ManagerFilterType::None;

            bool allEnabled = true;
            MxFunctions::CaseType restrictCase = (MxFunctions::CaseType)0;
            bool restrictCaseValid = false;
            int restrictCaseID = -1;
            for (const auto& item : items)
            {
                BrowserTreeType seltype = static_cast<BrowserTreeType>(item._treeType);
                if (seltype >= CaseImplicitLinearStaticParentItem && seltype < CaseSettingParentItem)
                {
                    restrictCase = static_cast<MxFunctions::CaseType>(seltype - CaseImplicitLinearStaticParentItem + 1);
                    restrictCaseID = item._phyID;
                    restrictCaseValid = true;
                    break;
                }
                else if (seltype == LoadManagerParentItem || seltype == CaseLoadChildItem)
                {
                    managerFilter = ManagerFilterType::LoadManager;
                    break;
                }
                else if (seltype == BoundaryManagerParentItem || seltype == CaseBCChildItem)
                {
                    managerFilter = ManagerFilterType::BoundaryManager;
                    break;
                }
                else if (seltype == PredefinedFieldManagerParentItem || seltype == CasePredefinedFieldChildItem)
                {
                    managerFilter = ManagerFilterType::PredefinedFieldManager;
                    break;
                }
            }
            // 当没有直接选中工况父节点时，如果当前选中项的父节点或爷爷节点是工况父节点，则按该工况进行过滤
            if (managerFilter == ManagerFilterType::None && !restrictCaseValid && items.size() == 1)
            {
                auto selectedItem = items.at(0);
                auto treeView = mBrowserWindow::getInstance()->getBrowserTree();
                auto parentKey = treeView->getParentKey(selectedItem);
                auto parentType = static_cast<BrowserTreeType>(parentKey._treeType);
                if (parentType >= CaseImplicitLinearStaticParentItem && parentType < CaseSettingParentItem)
                {
                    restrictCase = static_cast<MxFunctions::CaseType>(parentType - CaseImplicitLinearStaticParentItem + 1);
                    restrictCaseValid = true;
                }
                else
                {
                    auto grandParentKey = treeView->getParentKey(parentKey);
                    auto grandParentType = static_cast<BrowserTreeType>(grandParentKey._treeType);
                    if (grandParentType >= CaseImplicitLinearStaticParentItem && grandParentType < CaseSettingParentItem)
                    {
                        restrictCase = static_cast<MxFunctions::CaseType>(grandParentType - CaseImplicitLinearStaticParentItem + 1);
                        restrictCaseValid = true;
                    }
                }
            }
            // 只有为当前工况配置了可用按钮映射时，才按映射限制；
            // 否则保持其他分析工况的全部按钮可用。
            if (restrictCaseValid &&
                (caseLoadBoundaryBtns.contains(restrictCase) ||
                    casePredefinedFieldBtns.contains(restrictCase) ||
                    caseSymbolBtns.contains(restrictCase)))
            {
                allEnabled = false;
            }
            else if (managerFilter != ManagerFilterType::None)
            {
                allEnabled = false;
            }

            // _workingConditionGW 顺序与 tCon 对应：
            // 0: tCon6 预定义场; 1: tCon1 力学载荷, 2: tCon4 热学载荷, 3: tCon5 动态载荷,
            // 4: tCon2 边界条件, 5: tCon3 符号(边界页), 6: tConSub 子模型边界,
            // 7: tCon2_1 转子分析, 8: tCon3_1 符号(载荷页)
            auto isGroupEnabledForManager = [&](ManagerFilterType filter, int groupIndex) -> bool {
                switch (filter)
                {
                case ManagerFilterType::LoadManager:
                    return groupIndex == 1 || groupIndex == 2 || groupIndex == 3 || groupIndex == 5 || groupIndex == 8;
                case ManagerFilterType::BoundaryManager:
                    return groupIndex == 5 || groupIndex == 4 || groupIndex == 6 || groupIndex == 8;
                case ManagerFilterType::PredefinedFieldManager:
                    return groupIndex == 5 || groupIndex == 0 || groupIndex == 8;
                default:
                    return true;
                }
                };

            for (int i = 0; i < _workingConditionGW.size(); ++i)
            {
                auto* gw = _workingConditionGW.at(i);
                if (gw == nullptr)
                    continue;
                bool groupEnabled = allEnabled;
                if (!allEnabled && managerFilter != ManagerFilterType::None)
                    groupEnabled = isGroupEnabledForManager(managerFilter, i);
                auto buttons = gw->getButtons();
                for (auto& btn : buttons)
                {
                    btn->setEnabled(groupEnabled);
                }
            }

            if (!allEnabled && managerFilter == ManagerFilterType::None)
            {
                auto enableButtons = [&](const auto& btnMap) {
                    for (auto it = btnMap.begin(); it != btnMap.end(); ++it)
                    {
                        if (it.value() != nullptr)
                            it.value()->setEnabled(true);
                    }
                    };
                if (caseLoadBoundaryBtns.contains(restrictCase))
                    enableButtons(caseLoadBoundaryBtns[restrictCase]);
                if (casePredefinedFieldBtns.contains(restrictCase))
                    enableButtons(casePredefinedFieldBtns[restrictCase]);
                if (caseSymbolBtns.contains(restrictCase) && caseSymbolBtns[restrictCase] != nullptr)
                {
                    caseSymbolBtns[restrictCase]->setEnabled(true);
                    // 载荷页“重新调整”与边界页同步可用
                    if (btnResizeSymbolLoad != nullptr)
                        btnResizeSymbolLoad->setEnabled(true);
                }
            }

            // 转子分析按钮：仅在当前选中复模态分析且启用转子动力学时可用
            bool rotBtnEnabled = false;
            if (restrictCaseValid && restrictCase == MxFunctions::ImplicitModal && restrictCaseID > 0)
            {
                auto caseBase = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(restrictCaseID);
                if (auto modalData = dynamic_cast<mCaseImplicitModal*>(caseBase))
                {
                    rotBtnEnabled = (modalData->_modalType == "复模态分析") && modalData->_rotationalChecked;
                }
            }
            if (btnRotSpeed != nullptr)
                btnRotSpeed->setEnabled(rotBtnEnabled);
            if (btnRotAcc != nullptr)
                btnRotAcc->setEnabled(rotBtnEnabled);
        }
    }

    void mMenuWidget::slot_updateRotorButtons(int caseID)
    {
        bool rotBtnEnabled = false;
        auto caseBase = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(caseID);
        if (auto modalData = dynamic_cast<mCaseImplicitModal*>(caseBase))
        {
            rotBtnEnabled = (modalData->_modalType == "复模态分析") && modalData->_rotationalChecked;
        }
        if (btnRotSpeed != nullptr)
            btnRotSpeed->setEnabled(rotBtnEnabled);
        if (btnRotAcc != nullptr)
            btnRotAcc->setEnabled(rotBtnEnabled);
    }

    void mMenuWidget::initGeometry()
    {
        // 下方Ribbon工具条上添加相应的显示widget作为Group的载体
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("几何", tempWid);

        std::shared_ptr<mGroupWidget> sketchGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "草图"));
        std::shared_ptr<mGroupWidget> solidGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "实体"));
        std::shared_ptr<mGroupWidget> featureGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "特征"));
        std::shared_ptr<mGroupWidget> changeGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "变换"));
        std::shared_ptr<mGroupWidget> boolCtrlGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "布尔操作"));
        std::shared_ptr<mGroupWidget> extractGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "提取"));
        std::shared_ptr<mGroupWidget> editGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "几何编辑"));
        std::shared_ptr<mGroupWidget> repairGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "几何修复"));
        std::shared_ptr<mGroupWidget> solidifyGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "几何实体化"));
        std::shared_ptr<mGroupWidget> featureRemovalGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "特征去除"));
//		std::shared_ptr<mGroupWidget> directModelingGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "直接建模"));
        std::shared_ptr<mGroupWidget> sharedTopologyGW = std::shared_ptr<mGroupWidget>(this->addGroup("几何", "共享拓扑"));

		_geometryGW.push_back(sketchGW);
		_geometryGW.push_back(solidGW);
		_geometryGW.push_back(featureGW);
		_geometryGW.push_back(changeGW);
		_geometryGW.push_back(boolCtrlGW);
		_geometryGW.push_back(extractGW);
		_geometryGW.push_back(editGW);
		_geometryGW.push_back(repairGW);
		_geometryGW.push_back(solidifyGW);
		_geometryGW.push_back(featureRemovalGW);
		//_geometryGW.push_back(directModelingGW);
        _geometryGW.push_back(sharedTopologyGW);


		if (sketchGW)
		{
			//草图模式
			QToolButton* sketchBtn(sketchGW->addToolBtn("草图模式", ":/mMainWidgets/Icon/geometry/caotu.png", 0, 0, 1, 1, QSize(30, 30)));
			sketchBtn->setProperty("mxHasMenu", true);
			setQToolButton(sketchBtn, "草图模式", ":/mMainWidgets/Icon/geometry/caotu.png", "进入草图绘制模式，可进行创建、修改、\n"
				"约束、拉动等操作。\n");
            sketchBtn->setPopupMode(QToolButton::InstantPopup); // 点击就下拉
            QMenu* sketchMenu = new mUiMenu(this);
            ThemeManager::applyMenuStyle(sketchMenu);
			//connect(sketchBtn, &QToolButton::clicked, this, [this]() {
			//	mInfoWindow::getInstance()->unEditInfoWindow();
			//	MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("草图");
   //             //启用创建平面模式
   //             mGeoSketchSignalManager::instance()->enterSketch();
            //	});

            // XOY平面
            QAction* XOYAction = sketchMenu->addAction(QIcon(":/mMainWidgets/Icon/CenterMainWindow/qianshitu.png"), "XOY平面");
            connect(XOYAction, &QAction::triggered, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("草图");
                //启用创建平面模式
                mGeoSketchSignalManager::instance()->enterSketch(SketchPlane::XOY);
                });

            // XOZ平面
            QAction* XOZAction = sketchMenu->addAction(QIcon(":/mMainWidgets/Icon/CenterMainWindow/xiashitu.png"), "XOZ平面");
            connect(XOZAction, &QAction::triggered, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("草图");
                //启用创建平面模式
                mGeoSketchSignalManager::instance()->enterSketch(SketchPlane::XOZ);
                });

            // YOZ平面
            QAction* YOZAction = sketchMenu->addAction(QIcon(":/mMainWidgets/Icon/CenterMainWindow/zuoshitu.png"), "YOZ平面");
            connect(YOZAction, &QAction::triggered, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("草图");
                //启用创建平面模式
                mGeoSketchSignalManager::instance()->enterSketch(SketchPlane::YOZ);
                });

            // 手动选择平面
            QAction* customAction = sketchMenu->addAction(QIcon(":/mMainWidgets/Icon/choice/mian3x.png"), "手动选择平面");
            connect(customAction, &QAction::triggered, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("草图");
                //启用创建平面模式
                mGeoSketchSignalManager::instance()->enterSketch(SketchPlane::CUSTOM);
                });

            // 三点确定平面
            QAction* threePointAction = sketchMenu->addAction(QIcon(":/mMainWidgets/Icon/choice/dingdian3x.png"), "三点确定平面");
            connect(threePointAction, &QAction::triggered, this, [this]() {
                //在mInfoWindow中显示三点坐标输入面板，确认后进入草图模式
                mGeoSketchSignalManager::instance()->showThreePointPlanePanel();
                });

            sketchBtn->setMenu(sketchMenu);
        }
        if (solidGW)
        {
            // 创建立隐藏的代理按钮，用于复用现有的信号连接机制
            QToolButton* proxyCube = new QToolButton(this); proxyCube->setVisible(false);
            QToolButton* proxyCylinder = new QToolButton(this); proxyCylinder->setVisible(false);
            QToolButton* proxySphere = new QToolButton(this); proxySphere->setVisible(false);
            QToolButton* proxyConoid = new QToolButton(this); proxyConoid->setVisible(false);
            QToolButton* proxyTorus = new QToolButton(this); proxyTorus->setVisible(false);
            QToolButton* proxyFrustum = new QToolButton(this); proxyFrustum->setVisible(false);

            // 通过代理按钮建立信号连接
            mGeoSketchSignalManager::instance()->connectGeometryButton(proxyCube, mxr::SketchGModeType::CUBE);
            mGeoSketchSignalManager::instance()->connectGeometryButton(proxyCylinder, mxr::SketchGModeType::CYLINDER);
            mGeoSketchSignalManager::instance()->connectGeometryButton(proxySphere, mxr::SketchGModeType::SPHERE);
            mGeoSketchSignalManager::instance()->connectGeometryButton(proxyConoid, mxr::SketchGModeType::CONOID);
            mGeoSketchSignalManager::instance()->connectGeometryButton(proxyTorus, mxr::SketchGModeType::TORUS);
            mGeoSketchSignalManager::instance()->connectGeometryButton(proxyFrustum, mxr::SketchGModeType::FRUSTUM);

			// 立方体 - Split Button（默认：立方体；下拉可选：圆柱体、球体、圆锥、圆环、圆台）
			mComboButton* cubeCombo = new mComboButton(solidGW.get());
			cubeCombo->setActionItems({
				{"立方体", ":/mMainWidgets/Icon/geometry/立方体_大@3x.png", "快速创建立方体。", [proxyCube]() { proxyCube->click(); }},
				{"圆柱体", ":/mMainWidgets/Icon/geometry/圆柱体_大@3x.png", "快速创建圆柱体。", [proxyCylinder]() { proxyCylinder->click(); }},
				{"球体", ":/mMainWidgets/Icon/geometry/球体_大@3x.png", "快速创建球体。", [proxySphere]() { proxySphere->click(); }},
				{"圆锥", ":/mMainWidgets/Icon/geometry/圆锥_大@3x.png", "快速创建圆锥。", [proxyConoid]() { proxyConoid->click(); }},
				{"圆环", ":/mMainWidgets/Icon/geometry/圆环_大@3x.png", "快速创建圆环。", [proxyTorus]() { proxyTorus->click(); }},
				{"圆台", ":/mMainWidgets/Icon/geometry/圆台_大@3x.png", "快速创建圆台。", [proxyFrustum]() { proxyFrustum->click(); }}
			});
			cubeCombo->setButtonWidth(60);
			solidGW->addWidget(cubeCombo, 0, 0, 1, 1);
		}
		if (featureGW)
		{
			// 拉伸 - Split Button（默认主功能：拉伸；下拉可选：旋转）
			mComboButton* stretchCombo = new mComboButton(featureGW.get());
			stretchCombo->setActionItems({
				{"拉伸", ":/mMainWidgets/Icon/geometry/拉伸_大@3x.png", "快速拉伸对象。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoFeature(GeoOperationType::GeoStretch);
				}},
				{"旋转", ":/mMainWidgets/Icon/geometry/旋转_大@3x.png", "快速创建旋转。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoFeature(GeoOperationType::GeoRotation);
				}}
			});
			stretchCombo->setButtonWidth(60);
			featureGW->addWidget(stretchCombo, 0, 0, 1, 1);

			// 扫掠 - Split Button（默认主功能：扫掠；下拉可选：放样、拔模、抽壳）
			mComboButton* sweepCombo = new mComboButton(featureGW.get());
			sweepCombo->setActionItems({
				{"扫掠", ":/mMainWidgets/Icon/geometry//扫掠_大@3x.png", "快速创建扫掠。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoFeature(GeoOperationType::GeoSweep);
				}},
				{"放样", ":/mMainWidgets/Icon/geometry/放样_大@3x.png", "快速创建放样。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoFeature(GeoOperationType::GeoLoft);
				}},
				{"拔模", ":/mMainWidgets/Icon/geometry/拔模_大@3x.png", "快速创建拔模。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoFeature(GeoOperationType::GeoWithdrawingPattern);
				}},
				{"抽壳", ":/mMainWidgets/Icon/geometry/抽壳_大@3x.png", "快速创建抽壳。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoFeature(GeoOperationType::GeoShellRemoval);
				}}
			});
			sweepCombo->setButtonWidth(60);
			featureGW->addWidget(sweepCombo, 0, 1, 1, 1);

			// 倒角 - 大按钮（无下拉箭头，文字在图标下方）
			QToolButton* chamferingBtn = featureGW->addToolBtn("倒角", ":/mMainWidgets/Icon/geometry/倒角_大@3x.png", 0, 2, 1, 1, QSize(30, 30));
			setQToolButton(chamferingBtn, "倒角", ":/mMainWidgets/Icon/geometry/daojiao.png", "快速创建倒角。\n", Qt::ToolButtonTextUnderIcon);
			connect(chamferingBtn, &QToolButton::clicked, this, [this, chamferingBtn]() {
				mInfoWindow::getInstance()->unEditInfoWindow();
				mInfoWindow::getInstance()->createGeoFeature(GeoOperationType::GeoChamfering);
				});

			//// 孔 - 大按钮（在几何面上点选创建孔）
			//QToolButton* holeBtn = featureGW->addToolBtn("孔", ":/mMainWidgets/Icon/geometry/孔_大@3x.png", 0, 3, 1, 1, QSize(30, 30));
			//setQToolButton(holeBtn, "孔", ":/mMainWidgets/Icon/geometry/孔_大@3x.png", "在几何面上点选创建孔。\n", Qt::ToolButtonTextUnderIcon);
			//connect(holeBtn, &QToolButton::clicked, this, [this, holeBtn]() {
			//	mInfoWindow::getInstance()->unEditInfoWindow();
			//	mInfoWindow::getInstance()->createGeoFeature(GeoOperationType::GeoHole);
			//	});
		}
		if (changeGW)
		{
			// 平移 - Split Button（默认：平移；下拉可选：旋转、镜像、缩放、移动、定位）
			mComboButton* offsetCombo = new mComboButton(changeGW.get());
			offsetCombo->setActionItems({
                {"移动", ":/mMainWidgets/Icon/geometry/移动_大@3x.png", "快速移动部件。", [this]() {
                    getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::MoveGeoPart);
                    getCVM->getPreViewportWidget()->interaction()->getToolManager()->active<mxr::InteractionMoveGeometrySolidTool>();
                    vector<int> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->
                        getSelectDataByType(SelectionInfoType::GeoPart);
                    getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                        active<mxr::InteractionMoveGeometrySolidTool>()->setGeoTranformType(res, true);
                    ShortcutActionList::instance()->disconnectPartMoveRotate();
                    if (res.size() <= 0)
                        getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
                    else
                        ShortcutActionList::instance()->connectPartMoveRotate();
                }},
				{"平移", ":/mMainWidgets/Icon/geometry/平移_大@3x.png", "快速平移。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoTransform(GeoOperationType::GeoOffset);
				}},
				{"旋转", ":/mMainWidgets/Icon/geometry/旋转_大@3x-2.png", "快速旋转。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoTransform(GeoOperationType::GeoRotate);
				}},
				{"镜像", ":/mMainWidgets/Icon/geometry/镜像_大@3x.png", "快速镜像。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoTransform(GeoOperationType::GeoMirrorimage);
				}},
				{"缩放", ":/mMainWidgets/Icon/geometry/缩放_大@3x.png", "快速缩放。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoTransform(GeoOperationType::GeoZoom);
				}},
				{"定位", ":/mMainWidgets/Icon/geometry/定位_大@3x.png", "快速定位。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoTransform(GeoOperationType::GeoLocation);
				}}
			});
			offsetCombo->setButtonWidth(60);
			changeGW->addWidget(offsetCombo, 0, 0, 2, 1);

            //线性阵列
            QToolButton* linearArrayBtn = changeGW->addToolBtn("线性阵列", ":/mMainWidgets/Icon/geometry/xianxingzhenglie.png", 0, 1, 1, 1, QSize(20, 20));
            setQToolButton(linearArrayBtn, "线性阵列", ":/mMainWidgets/Icon/geometry/xianxingzhenglie.png", "快速创建线性阵列。\n", Qt::ToolButtonTextBesideIcon);
            connect(linearArrayBtn, &QToolButton::clicked, this, [this, linearArrayBtn]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createGeoTransform(GeoOperationType::GeoLinearArray);
                });

			// 环形阵列 - 小按钮
			QToolButton* ringArrayBtn = changeGW->addToolBtn("环形阵列", ":/mMainWidgets/Icon/geometry/huanxingzhenglie.png", 1, 1, 1, 1, QSize(20, 20));
			setQToolButton(ringArrayBtn, "环形阵列", ":/mMainWidgets/Icon/geometry/huanxingzhenglie.png", "快速创建环形阵列。\n", Qt::ToolButtonTextBesideIcon);
			connect(ringArrayBtn, &QToolButton::clicked, this, [this]() {
				mInfoWindow::getInstance()->unEditInfoWindow();
				mInfoWindow::getInstance()->createGeoTransform(GeoOperationType::GeoRingArray);
				});
		}
		if (boolCtrlGW)
		{
			// 布尔运算 - Split Button（默认：布尔求和；下拉可选：布尔求差、布尔求交、共享拓扑）
			mComboButton* boolOperationCombo = new mComboButton(boolCtrlGW.get());
			boolOperationCombo->setActionItems({
				{"布尔求和", ":/mMainWidgets/Icon/geometry/布尔求和_大@3x.png", "快速创建布尔求和。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoBool(GeoOperationType::GeoSummation);
				}},
				{"布尔求差", ":/mMainWidgets/Icon/geometry/布尔求差_大@3x.png", "快速创建布尔求差。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoBool(GeoOperationType::GeoSusubtrac);
				}},
				{"布尔求交", ":/mMainWidgets/Icon/geometry/布尔求交_大@3x.png", "快速创建布尔求交。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoBool(GeoOperationType::GeoInterse);
				}}
			});
			boolOperationCombo->setButtonWidth(70);
			boolCtrlGW->addWidget(boolOperationCombo, 0, 0, 1, 1);
		}
		if (extractGW)
		{
			// 流体域 - Split Button（默认：流体域；下拉可选：中面、梁截面、中线、边界面）
			mComboButton* fluidDomainCombo = new mComboButton(extractGW.get());
			fluidDomainCombo->setActionItems({
				{"流体域", ":/mMainWidgets/Icon/geometry/流体域_大@3x.png", "快速提取流体域。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoExtract(GeoOperationType::GeoFluid);
				}},
				//{"梁截面", ":/mMainWidgets/Icon/geometry/liangjiemian.png", "快速提取梁截面。", [this]() {
				//	// 梁截面功能暂未实现
				//}},
				{"中线", ":/mMainWidgets/Icon/geometry/中线_大@3x.png", "快速提取中线。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoExtract(GeoOperationType::GeoMidLine);
				}},
				{"边界面", ":/mMainWidgets/Icon/geometry/边界面_大@3x.png", "快速提取边界面。", [this]() {
					mInfoWindow::getInstance()->unEditInfoWindow();
					mInfoWindow::getInstance()->createGeoExtract(GeoOperationType::GeoBoundaryFace);
				}}
			});
			fluidDomainCombo->setButtonWidth(60);
			extractGW->addWidget(fluidDomainCombo, 0, 0, 1, 1);

            // 中面 - 大按钮（无下拉箭头，文字在图标下方）
            QToolButton* midFaceBtn = extractGW->addToolBtn("中面", ":/mMainWidgets/Icon/geometry/中面_大@3x.png", 0, 2, 1, 1, QSize(30, 30));
            setQToolButton(midFaceBtn, "中面", ":/mMainWidgets/Icon/geometry/midSurface.png", "快速提取中面。\n", Qt::ToolButtonTextUnderIcon);
            connect(midFaceBtn, &QToolButton::clicked, this, []() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createGeoExtract(GeoOperationType::GeoMidFace);
                });
        }
        if (editGW)
        {
            // 分割 - Split Button（默认：分割线；下拉可选：分割面、分割体）
            mComboButton* segmentationCombo = new mComboButton(editGW.get());
            segmentationCombo->setActionItems({
                {"分割线", ":/mMainWidgets/Icon/geometry/分割线_大@3x.png", "快速分割线。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoSplitLine);
                }},
                {"分割面", ":/mMainWidgets/Icon/geometry/分割面_大@3x.png", "快速分割面。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoSplitFace);
                }},
                {"分割体", ":/mMainWidgets/Icon/geometry/分割体_大@3x.png", "快速分割体。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoSplitSolid);
                }}
                });
                segmentationCombo->setButtonWidth(60);
                editGW->addWidget(segmentationCombo, -1, -1, 1, 1);

            // 合并 - Split Button（默认：合并线；下拉可选：合并面、合并体）
            mComboButton* mergeCombo = new mComboButton(editGW.get());
            mergeCombo->setActionItems({
                {"合并线", ":/mMainWidgets/Icon/geometry/合并线_大@3x.png", "快速合并线。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoMergeLine);
                }},
                {"合并面", ":/mMainWidgets/Icon/geometry/合并面_大@3x.png", "快速合并面。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoMergeFace);
                }},
                {"合并体", ":/mMainWidgets/Icon/geometry/合并体_大@3x.png", "快速合并体。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoMergeSolid);
                }}
                });
                mergeCombo->setButtonWidth(60);
                editGW->addWidget(mergeCombo, -1, -1, 1, 1);

            // 删除 - Split Button（默认：删除面；下拉可选：删除体、删除线）
            mComboButton* deleteCombo = new mComboButton(editGW.get());
            deleteCombo->setActionItems({
                {"删除面", ":/mMainWidgets/Icon/geometry/删除面_大@3x.png", "快速删除面。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoDeleteFace);
                }},
                {"删除体", ":/mMainWidgets/Icon/geometry/删除体_大@3x.png", "快速删除体。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoDeleteSolid);
                }},
                {"删除线", ":/mMainWidgets/Icon/geometry/删除线_大@3x.png", "快速删除独立线。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoDeleteLine);
                }}
                });
                deleteCombo->setButtonWidth(60);
                editGW->addWidget(deleteCombo, -1, -1, 1, 1);

            // 检查 - Split Button（默认：短边；下拉可选：锐边、小型面、干涉检查）
            mComboButton* checkCombo = new mComboButton(editGW.get());
            checkCombo->setActionItems({
                {"短边", ":/mMainWidgets/Icon/geometry/短边_大@3x.png", "快速检查短边。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoCheckShortEdge);
                }},
                {"锐边", ":/mMainWidgets/Icon/geometry/锐边_大@3x.png", "快速检查锐边。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoCheckSharpEdge);
                }},
                {"开放边", ":/mMainWidgets/Icon/geometry/检查开放边_大@3x.png", "快速检查开放边。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoCheckOpenEdge);
                }},
                {"小型面", ":/mMainWidgets/Icon/geometry/小型面_大@3x.png", "快速检查小型面。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoCheckSmallFace);
                }},
                {"微小实体", ":/mMainWidgets/Icon/geometry/检查微小实体_大@3x.png", "快速检查微小实体。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoCheckTinySolid);
                }},
                {"干涉检查", ":/mMainWidgets/Icon/geometry/几何干涉检查_大@3x.png", "几何干涉检查。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoInterferenceCheck);
                }}
                });
                checkCombo->setButtonWidth(70);
                editGW->addWidget(checkCombo, -1, -1, 1, 1);

            // 取消裁剪面 - Split Button（默认：取消裁剪面；下拉可选：修补缝隙、线延长、面偏置、特征投影、面延伸、线连接）
            mComboButton* cancelTrimSurfCombo = new mComboButton(editGW.get());
            cancelTrimSurfCombo->setActionItems({
                {"取消裁剪面", ":/mMainWidgets/Icon/geometry/取消裁剪面_大@3x.png", "取消裁剪面。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoCancelTrimFace);
                }},
                {"线连接", ":/mMainWidgets/Icon/geometry/线连接_大@3x.png", "线连接。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoLineCombine);
                }},
                {"线延长", ":/mMainWidgets/Icon/geometry/线延长_大@3x.png", "线延长。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoLineLengthen);
                }},
                {"面延伸", ":/mMainWidgets/Icon/geometry/面延伸_大@3x.png", "面延伸。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoFaceExtend);
                }},
                {"面偏置", ":/mMainWidgets/Icon/geometry/面偏置_大@3x.png", "面偏置。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoFaceOffset);
                }},
                {"修补缝隙", ":/mMainWidgets/Icon/geometry/修补缝隙_大@3x.png", "修补缝隙。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoRepairCracks);
                }},
                {"填充曲面", ":/mMainWidgets/Icon/geometry/面延伸_大@3x.png", "填充曲面。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoFillSurface);
                }}

                });
                cancelTrimSurfCombo->setButtonWidth(80);
                editGW->addWidget(cancelTrimSurfCombo, 0, 4, 2, 1);

            QToolButton* geoProjectBtn = editGW->addToolBtn("特征投影", ":/mMainWidgets/Icon/geometry/project.png", 0, 5, 1, 1, QSize(20, 20));
            setQToolButton(geoProjectBtn, "特征投影", ":/mMainWidgets/Icon/geometry/project.png", "特征投影。\n", Qt::ToolButtonTextBesideIcon);
            connect(geoProjectBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createGeoEdit(GeoOperationType::GeoFeatureProjection);
                });

            // 几何清理
            //QToolButton* geoCleanBtn = editGW->addToolBtn("几何清理", ":/mMainWidgets/Icon/geometry/jiheqingli.png", 1, 5, 1, 1, QSize(20, 20));
            //setQToolButton(geoCleanBtn, "几何清理", ":/mMainWidgets/Icon/geometry/jiheqingli.png", "几何清理。\n", Qt::ToolButtonTextBesideIcon);
            //connect(geoCleanBtn, &QToolButton::clicked, this, [this]() {
            //    //【空窗收口·G6】几何清理走引擎整件修复 RepairGeometryClean（ShapeFix 全模式，
            //    // 老 doGeoClean 等价；编辑历史经重识别链自动落 GN_Part，等价老 updateEditPara）
            //    try
            //    {
            //        EventDispatcher::mGeoRepairRequest req;
            //        req.repairType = EventDispatcher::RepairGeometryClean;
            //        EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().repairOCC(req);
            //        if (!res.succeeded())
            //        {
            //            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
            //                tr("几何清理失败：%1").arg(QString::fromStdString(res.message)));
            //            return;
            //        }
            //        //被修改部件登记编辑簿记（对齐老 appendEditSet 语义）
            //        for (const auto& obj : res.modifiedObjectIds)
            //        {
            //            getGEOServer.context().PartManager()._editPart.insert(obj.id);
            //        }
            //        getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
            //        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何清理成功！"));
            //    }
            //    catch (...)
            //    {
            //        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何清理失败！"));
            //    }

            //    });
        }
        if (repairGW)
        {
            // 分割边 - Split Button（默认：分割边；下拉可选：额外边、重复面、硬点删除、重复曲线）
            mComboButton* repairEdgeCombo = new mComboButton(repairGW.get());
            repairEdgeCombo->setActionItems({
                {"分割边", ":/mMainWidgets/Icon/geometry/修复分割边_大@3x.png", "分割边。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoRepair(GeoOperationType::GeoRepairSplitEdge);
                }},
                {"额外边", ":/mMainWidgets/Icon/geometry/额外边_大@3x.png", "额外边。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoRepair(GeoOperationType::GeoRepairExtraEdge);
                }},
                {"重复面", ":/mMainWidgets/Icon/geometry/重复面_大@3x.png", "重复面。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoRepair(GeoOperationType::GeoRemoveRepeatFace);
                }},
                {"硬点增删", ":/mMainWidgets/Icon/geometry/硬点删除_大@3x.png", "硬点增删。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoRepair(GeoOperationType::GeoHardPoint);
                }},
                {"重复曲线", ":/mMainWidgets/Icon/geometry/重复曲线_大@3x.png", "检测并移除重复曲线。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoRepair(GeoOperationType::GeoRemoveDuplicateLines);
                }}
                // 拟合曲线暂时隐藏，后端与面板实现保留。
                // {"拟合曲线", ":/mMainWidgets/Icon/geometry/重复曲线_大@3x.png", "拟合曲线。", [this]() {
                //     mInfoWindow::getInstance()->unEditInfoWindow();
                //     mInfoWindow::getInstance()->createGeoRepair(GeoOperationType::GeoFitCurves);
                // }}
            });
            repairEdgeCombo->setButtonWidth(80);
            repairGW->addWidget(repairEdgeCombo, 0, 0, 1, 1);
        }

        if (solidifyGW)
        {
            //拼接
            QToolButton* stitchBtn = solidifyGW->addToolBtn("拼接", ":/mMainWidgets/Icon/geometry/pingjie.png", 0, 0, 1, 1, QSize(20, 20));
            setQToolButton(stitchBtn, "拼接", ":/mMainWidgets/Icon/geometry/pingjie.png", "拼接。\n", Qt::ToolButtonTextBesideIcon);
            connect(stitchBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createGeoSolidify(GeoOperationType::GeoStitch);
                });

            //间距
            QToolButton* gapBtn = solidifyGW->addToolBtn("间距", ":/mMainWidgets/Icon/geometry/jianju.png", 0, 1, 1, 1, QSize(20, 20));
            setQToolButton(gapBtn, "间距", ":/mMainWidgets/Icon/geometry/jianju.png", "间距。\n", Qt::ToolButtonTextBesideIcon);
            connect(gapBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createGeoSolidify(GeoOperationType::GeoGap);
                });

            //缺失面
            QToolButton* missingFaceBtn = solidifyGW->addToolBtn("缺失面", ":/mMainWidgets/Icon/geometry/queshimian.png", 1, 0, 1, 1, QSize(20, 20));
            setQToolButton(missingFaceBtn, "缺失面", ":/mMainWidgets/Icon/geometry/queshimian.png", "缺失面。\n", Qt::ToolButtonTextBesideIcon);
            connect(missingFaceBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createGeoSolidify(GeoOperationType::GeoMissingFace);
                });

            ////逆向建模
            //QToolButton* reverseModelBtn = solidifyGW->addToolBtn("逆向建模", ":/mMainWidgets/Icon/geometry/逆向建模_大@3x.png", 1, 1, 1, 1, QSize(20, 20));
            //setQToolButton(reverseModelBtn, "逆向建模", ":/mMainWidgets/Icon/geometry/fangyang.png", "逆向建模：STL文件或当前模型网格转换为STEP实体，可输出STEP文件或创建几何部件。\n", Qt::ToolButtonTextBesideIcon);
            //connect(reverseModelBtn, &QToolButton::clicked, this, [this]() {
            //    mInfoWindow::getInstance()->unEditInfoWindow();
            //    mInfoWindow::getInstance()->createGeoSolidify(GeoOperationType::GeoReverseModel);
            //    });
        }

        if (featureRemovalGW)
        {
            //特征去除
            QToolButton* autoRemovalBtn(featureRemovalGW->addToolBtn("特征去除", ":/mMainWidgets/Icon/geometry/zidongquchu.png", -1, -1, 1, 1, QSize(30, 30)));
            setQToolButton(autoRemovalBtn, "特征去除", ":/mMainWidgets/Icon/geometry/zidongquchu.png", "自动特征去除。\n");
            autoRemovalBtn->setMinimumWidth(60);  // 设置最小宽度
            autoRemovalBtn->setMinimumHeight(50);  // 设置最小高度
            autoRemovalBtn->installEventFilter(new TooltipInstaller("特征去除",
                QPixmap(":/mMainWidgets/Icon/geometry/zidongquchu.png"),
                "特征去除。\n",
                200, autoRemovalBtn));
            connect(autoRemovalBtn, &QToolButton::clicked, this, [this, autoRemovalBtn]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createGeoRemoval(GeoOperationType::GeoAutoRemoval);
                });
        }
        if (sharedTopologyGW)
        {
            //共享
            QToolButton* shareBtn(sharedTopologyGW->addToolBtn("共享", ":/mMainWidgets/Icon/geometry/共享拓扑_小@3x.png", -1, -1, 1, 1, QSize(30, 30)));
            setQToolButton(shareBtn, "共享", ":/mMainWidgets/Icon/geometry/共享拓扑_大@3x.png", "共享拓扑。\n");
            connect(shareBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createGeoSharedTopology(GeoOperationType::GeoSharedTopology);
                });

            ////取消共享
            //QToolButton* unshareBtn = sharedTopologyGW->addToolBtn("取消共享", ":/mMainWidgets/Icon/geometry/gongxiang.png", 0, 1, 1, 1, QSize(20, 20));
            //setQToolButton(unshareBtn, "取消共享", ":/mMainWidgets/Icon/geometry/gongxiang.png", "取消共享拓扑。\n", Qt::ToolButtonTextBesideIcon);
            //connect(unshareBtn, &QToolButton::clicked, this, [this]() {
            //    mInfoWindow::getInstance()->unEditInfoWindow();
            //    mInfoWindow::getInstance()->createGeoSharedTopology(GeoOperationType::GeoUnSharedTopology);
            //    });

            ////强制共享
            //QToolButton* forceShareBtn = sharedTopologyGW->addToolBtn("强制共享", ":/mMainWidgets/Icon/geometry/gongxiang.png", 1, 1, 1, 1, QSize(20, 20));
            //setQToolButton(forceShareBtn, "强制共享", ":/mMainWidgets/Icon/geometry/gongxiang.png", "强制共享拓扑。\n", Qt::ToolButtonTextBesideIcon);
            //connect(forceShareBtn, &QToolButton::clicked, this, [this]() {
            //    mInfoWindow::getInstance()->unEditInfoWindow();
            //    mInfoWindow::getInstance()->createGeoSharedTopology(GeoOperationType::GeoForceSharedTopology);
            //    });

            QToolButton* forceShareBtn_1 = sharedTopologyGW->addToolBtn("自动共享", ":/mMainWidgets/Icon/geometry/自动共享_小@3x.png", 0, 1, 2, 1, QSize(30, 30));
            setQToolButton(forceShareBtn_1, "自动共享", ":/mMainWidgets/Icon/geometry/自动共享_大@3x.png", "自动共享拓扑。\n");
            connect(forceShareBtn_1, &QToolButton::clicked, this, [this]() {
                //【自动共享·独立功能】旧 mGeoCommandFactory::createShareTopo 全库链语义：
                // 引擎全库融合生成"共享拓扑部件"并删除原部件；与"共享"面板（选面原位
                // 压印）互不相关，不走面板请求形态
                try
                {
                    //建件前收集全库原部件：建件成功后整体删除（旧按钮 deleteGeoTree 同款）
                    std::set<int> oldPartIds;
                    for (int partId : getGEOServer.context().PartManager().getPartList())
                    {
                        oldPartIds.insert(partId);
                    }
                    EventDispatcher::mGeoAutoSharedTopologyRequest req;
                    EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().autoSharedTopologyOCC(req);
                    if (res.succeeded() && !res.generatedObjects.empty())
                    {
                        const int newPartId = res.generatedObjects.back().id.id;
                        auto* geoManager = getCVM->getModelData()->getGeoManager();
                        geoManager->registerCreatedPart(newPartId);  // MeshEntity 重建 + 树追加
                        oldPartIds.erase(newPartId);
                        if (!oldPartIds.empty())
                        {
                            mMechGlobalSignals::getInstance()->deleteGeoTree(oldPartIds);
                        }
                        //融合件内部共享面（一面属两体）回填 _sharedTopoFaces 供剖分共节点
                        geoManager->rebuildSharedTopologyFromGN();
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("共享拓扑创建成功!"));
                    }
                    else
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                            tr("共享拓扑创建失败! ") + QString::fromStdString(res.message));
                    }
                }
                catch (...)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("共享拓扑操作出错！"));
                }
                });
        }
		/*if (directModelingGW)
		{
            mComboButton* directModelingCombo = new mComboButton(directModelingGW.get());
            directModelingCombo->setActionItems({
                {"移动面", ":/mMainWidgets/Icon/geometry/移动面_大@3x.png", "移动面。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoDirectModeling(GeoOperationType::GeoDirectModelingOffsetFace);
                }},
                {"旋转面", ":/mMainWidgets/Icon/geometry/旋转面_大@3x.png", "旋转面。", [this]() {
                     mInfoWindow::getInstance()->unEditInfoWindow();
                     mInfoWindow::getInstance()->createGeoDirectModeling(GeoOperationType::GeoDirectModelingRotateFace);
                }},
                {"替换面", ":/mMainWidgets/Icon/geometry/替换面_大@3x.png", "替换面。", [this]() {
                    mInfoWindow::getInstance()->unEditInfoWindow();
                    mInfoWindow::getInstance()->createGeoDirectModeling(GeoOperationType::GeoDirectModelingReplaceFace);
                }},
                {"调整圆角", ":/mMainWidgets/Icon/geometry/调整圆角_大@3x.png", "调整圆角大小。", [this]() {
                     mInfoWindow::getInstance()->unEditInfoWindow();
                     mInfoWindow::getInstance()->createGeoDirectModeling(GeoOperationType::GeoDirectModelingResizeFillet);
                }}
                });
                directModelingCombo->setButtonWidth(80);
                directModelingGW->addWidget(directModelingCombo, 0, 0, 1, 1);
		}*/
	}

    void mMenuWidget::initSectionShapeMenu()
    {
        //QToolBar* sectionBar = new QToolBar;
        //sectionBar->setIconSize(QSize(24, 24));
        //sectionBar->addAction(QIcon(""), "矩形");
        //sectionBar->addAction(QIcon(""), "圆形");
        //sectionBar->addAction(QIcon(""), "工字钢");
        //// …继续加别的形状…

        //m_toolBarStackedWid_->addWidget(sectionBar);   // 放进堆栈
        /* 2. 新建对应的 RibbonPage（空壳） */
        RibbonPage* newPage = new RibbonPage();
        m_toolBarStackedWid_->addWidget(newPage);
        m_StaWidMap.insert("梁截面", newPage);                    // 记录映射

        /* 3. 往新页面里专门加“截面形状”组 */
        mGroupWidget* shapeGW = new mGroupWidget("梁截面", newPage->widget());
        QHBoxLayout* hLay = qobject_cast<QHBoxLayout*>(newPage->widget()->layout());
        if (hLay) {
            hLay->insertWidget(hLay->count() - 1, shapeGW);         // 插到 stretch 前
            newPage->appendGroup(shapeGW);
        }
        /* 4. 往组里加真正的形状按钮 */

        /*enum SecType {
            Sec_H = 1, Sec_I, Sec_L, Sec_T, Sec_Box, Sec_Rec, Sec_Cir,
            Sec_Ann, Sec_Polygon, Sec_Channel, Sec_Channel1,
            Sec_I1, Sec_T1, Sec_T2, Sec_Box1, Sec_Cross, Sec_Hat, Sec_Z
        };*/

        auto HBtn = shapeGW->addToolBtn("H", ":/mMainWidgets/Icon/sectionShape/H.png", 0, 0, 1, 1, QSize(30, 30));
        setQToolButton(HBtn, "H", ":/mMainWidgets/Icon/sectionShape/H.png", "H。");
        HBtn->setProperty("sectionType", "H");
        connect(HBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_H); });

        auto LBtn = shapeGW->addToolBtn("L", ":/mMainWidgets/Icon/sectionShape/L.png", 0, 1, 1, 1, QSize(30, 30));
        setQToolButton(LBtn, "L", ":/mMainWidgets/Icon/sectionShape/L.png", "L。");
        LBtn->setProperty("sectionType", "L");
        connect(LBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_L); });

        auto IBtn = shapeGW->addToolBtn("I", ":/mMainWidgets/Icon/sectionShape/I.png", 0, 2, 1, 1, QSize(30, 30));
        setQToolButton(IBtn, "I", ":/mMainWidgets/Icon/sectionShape/I.png", "I。");
        IBtn->setProperty("sectionType", "I");
        connect(IBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_I); });

        auto I1Btn = shapeGW->addToolBtn("I1", ":/mMainWidgets/Icon/sectionShape/I1.png", 0, 3, 1, 1, QSize(30, 30));
        setQToolButton(I1Btn, "I1", ":/mMainWidgets/Icon/sectionShape/I1.png", "I1。");
        I1Btn->setProperty("sectionType", "I1");
        connect(I1Btn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_I1); });

        auto TBtn = shapeGW->addToolBtn("T", ":/mMainWidgets/Icon/sectionShape/T.png", 0, 4, 1, 1, QSize(30, 30));
        setQToolButton(TBtn, "T", ":/mMainWidgets/Icon/sectionShape/T.png", "T。");
        TBtn->setProperty("sectionType", "T");
        connect(TBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_T); });

        auto T1Btn = shapeGW->addToolBtn("T1", ":/mMainWidgets/Icon/sectionShape/T1.png", 0, 5, 1, 1, QSize(30, 30));
        setQToolButton(T1Btn, "T1", ":/mMainWidgets/Icon/sectionShape/T1.png", "T1。");
        T1Btn->setProperty("sectionType", "T1");
        connect(T1Btn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_T1); });

        auto T2Btn = shapeGW->addToolBtn("T2", ":/mMainWidgets/Icon/sectionShape/T2.png", 0, 6, 1, 1, QSize(30, 30));
        setQToolButton(T2Btn, "T2", ":/mMainWidgets/Icon/sectionShape/T2.png", "T2。");
        T2Btn->setProperty("sectionType", "T2");
        connect(T2Btn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_T2); });

        auto ZBtn = shapeGW->addToolBtn("Z", ":/mMainWidgets/Icon/sectionShape/Z-3.png", 0, 7, 1, 1, QSize(30, 30));
        setQToolButton(ZBtn, "Z", ":/mMainWidgets/Icon/sectionShape/Z-3.png", "Z。");
        ZBtn->setProperty("sectionType", "Z");
        connect(ZBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_Z); });

        auto HEBtn = shapeGW->addToolBtn("盒", ":/mMainWidgets/Icon/sectionShape/he.png", 0, 8, 1, 1, QSize(30, 30));
        setQToolButton(HEBtn, "盒", ":/mMainWidgets/Icon/sectionShape/he.png", "盒。");
        HEBtn->setProperty("sectionType", "HE");
        connect(HEBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_Box); });

        auto HE1Btn = shapeGW->addToolBtn("盒1", ":/mMainWidgets/Icon/sectionShape/he.png", 0, 9, 1, 1, QSize(30, 30));
        setQToolButton(HE1Btn, "盒1", ":/mMainWidgets/Icon/sectionShape/he.png", "盒1。");
        HE1Btn->setProperty("sectionType", "HE1");
        connect(HE1Btn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_Box1); });

        auto JUXINGBtn = shapeGW->addToolBtn("矩形", ":/mMainWidgets/Icon/sectionShape/juxing.png", 0, 11, 1, 1, QSize(30, 30));
        setQToolButton(JUXINGBtn, "矩形", ":/mMainWidgets/Icon/sectionShape/juxing.png", "矩形。");
        JUXINGBtn->setProperty("sectionType", "JUXING");
        connect(JUXINGBtn, &QToolButton::clicked, [=]() { slot_createSectionShape(MxFunctions::Sec_Rec); });

        auto YUANBtn = shapeGW->addToolBtn("圆", ":/mMainWidgets/Icon/sectionShape/yuan.png", 0, 12, 1, 1, QSize(30, 30));
        setQToolButton(YUANBtn, "圆", ":/mMainWidgets/Icon/sectionShape/yuan.png", "圆。");
        YUANBtn->setProperty("sectionType", "YUAN");
        connect(YUANBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_Cir); });

        auto YUANHUANBtn = shapeGW->addToolBtn("圆环", ":/mMainWidgets/Icon/sectionShape/yuanhuan.png", 0, 13, 1, 1, QSize(30, 30));
        setQToolButton(YUANHUANBtn, "圆环", ":/mMainWidgets/Icon/sectionShape/yuanhuan.png", "圆环。");
        YUANHUANBtn->setProperty("sectionType", "YUANHUAN");
        connect(YUANHUANBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_Ann); });

        auto CAOBtn = shapeGW->addToolBtn("槽", ":/mMainWidgets/Icon/sectionShape/cao.png", 0, 14, 1, 1, QSize(30, 30));
        setQToolButton(CAOBtn, "槽", ":/mMainWidgets/Icon/sectionShape/cao.png", "槽。");
        CAOBtn->setProperty("sectionType", "CAO");
        connect(CAOBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_Channel); });

        auto CAO1Btn = shapeGW->addToolBtn("槽1", ":/mMainWidgets/Icon/sectionShape/cao.png", 0, 15, 1, 1, QSize(30, 30));
        setQToolButton(CAO1Btn, "槽1", ":/mMainWidgets/Icon/sectionShape/cao.png", "槽1。");
        CAO1Btn->setProperty("sectionType", "CAO1B");
        connect(CAO1Btn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_Channel1); });

        auto CAO2Btn = shapeGW->addToolBtn("槽2", ":/mMainWidgets/Icon/sectionShape/cao1.png", 0, 16, 1, 1, QSize(30, 30));
        setQToolButton(CAO2Btn, "槽2", ":/mMainWidgets/Icon/sectionShape/cao1.png", "槽2。");
        CAO2Btn->setProperty("sectionType", "CAO2B");
        connect(CAO2Btn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_Channel2); });

        auto SHIZIBtn = shapeGW->addToolBtn("十字", ":/mMainWidgets/Icon/sectionShape/shizi.png", 0, 17, 1, 1, QSize(30, 30));
        setQToolButton(SHIZIBtn, "十字", ":/mMainWidgets/Icon/sectionShape/shizi.png", "十字。");
        SHIZIBtn->setProperty("sectionType", "SHIZI");
        connect(SHIZIBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_Cross); });

        auto MAOXINGBtn = shapeGW->addToolBtn("帽形", ":/mMainWidgets/Icon/sectionShape/maoxing.png", 0, 18, 1, 1, QSize(30, 30));
        setQToolButton(MAOXINGBtn, "帽形", ":/mMainWidgets/Icon/sectionShape/maoxing.png", "帽形。");
        MAOXINGBtn->setProperty("sectionType", "MAOXING");
        connect(MAOXINGBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_Hat); });

        auto customBtn = shapeGW->addToolBtn("自定义", ":/mMainWidgets/Icon/sectionShape/maoxing.png", 0, 19, 1, 1, QSize(30, 30));
        setQToolButton(customBtn, "自定义", ":/mMainWidgets/Icon/sectionShape/maoxing.png", "自定义。");
        customBtn->setProperty("sectionType", "CUSTOM");
        connect(customBtn, &QToolButton::clicked, [=]() {slot_createSectionShape(MxFunctions::Sec_None); });

        // …继续加别的形状…

        /* 5. 连接信号（示例） */
        // 函数
        connect(HBtn, &QToolButton::clicked, this, [this]() { /* 创建矩形截面 */ });

        connect(LBtn, &QToolButton::clicked, this, [this]() { /* 创建圆形截面 */ });
        connect(IBtn, &QToolButton::clicked, this, [this]() { /* 创建工字钢截面 */ });


        //回退功能
        mGroupWidget* undoGroup = new mGroupWidget("", newPage->widget());
        // 插入到布局 stretch 之前，自动排在「梁截面」分组右侧
        hLay->insertWidget(hLay->count() - 1, undoGroup);
        newPage->appendGroup(undoGroup);
        // 添加回退按钮
        auto undoBtn = undoGroup->addToolBtn("返回", ":/mMainWidgets/Icon/undo.png", 0, 0, 1, 1, QSize(30, 30));
        setQToolButton(undoBtn, "返回", ":/mMainWidgets/Icon/undo.png", "返回到截面属性");
        connect(undoBtn, &QToolButton::clicked, this, [this]()
            {
                MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("截面属性");
            });
    }

    //-------------------important----------------------------------------
    //多体动力学Ribbon页（阶段1：按钮全部接入mInfoWindow入口桩；求解引擎未进Basic，无"求解"按钮）
    void mMenuWidget::initMotionMenu()
    {
        // 原"多体动力学"单页按原型拆分为六个功能区页：体定义/运动副与约束/驱动/连接与力元/传感与控制/求解。
        // 多体形态下六页常驻；隐式形态下非常驻，树点击多体对象时按动态页签位规则临时显示。
        for (const QString& page : { QString::fromUtf8("体定义"), QString::fromUtf8("运动副与约束"),
            QString::fromUtf8("驱动"), QString::fromUtf8("连接与力元"), QString::fromUtf8("传感与控制"),
            QString::fromUtf8("求解") })
        {
            RibbonPage* tempWid = new RibbonPage();
            m_toolBarStackedWid_->addWidget(tempWid);
            m_StaWidMap.insert(page, tempWid);
        }

        // 为各功能区页添加Group分组（原组内按钮与信号槽保持不变，仅重新归页）
        mGroupWidget* tGW0 = this->addGroup("求解", "求解");
        mGroupWidget* tGW_body = this->addGroup("体定义", "体");
        mGroupWidget* tGW2 = this->addGroup("运动副与约束", "基本运动副 ▼");
        mGroupWidget* tGW2b = this->addGroup("运动副与约束", "几何约束 ▼");
        mGroupWidget* tGW3 = this->addGroup("运动副与约束", "耦合副 ▼");
        mGroupWidget* tGW_drive = this->addGroup("驱动", "运动副驱动");
        mGroupWidget* tGW_drive2 = this->addGroup("驱动", "运动体驱动");
        mGroupWidget* tGW5 = this->addGroup("连接与力元", "柔性连接 ▼");
        mGroupWidget* tGW6 = this->addGroup("连接与力元", "加载力 ▼");
        mGroupWidget* tGW_sensor = this->addGroup("传感与控制", "传感器");
        mGroupWidget* tGW_ctrl = this->addGroup("传感与控制", "控制器");
        mGroupWidget* tGW7 = this->addGroup("传感与控制", "控制模块 ▼");
        mGroupWidget* tGW9 = this->addGroup("体定义", "飞机专用模块 ▼");
        mGroupWidget* tGW10 = this->addGroup("体定义", "系统建模工具 ▼");

        QString groupTitleStyle =
            "font-family: 'Microsoft YaHei';"
            "font-size: 12px;"
            "font-weight: normal;"
            "color: #909399;"
            "background: transparent;";
        "margin: 0px auto; "         // 按钮水平居中
            "text-align: center;"        // 文字居中
            "}";


        QString toolButtonStyle =
            "QToolButton {"
            "    font-family: 'Microsoft YaHei';"
            "    font-size: 12px;"
            "    font-weight: normal;"
            "    color: #303133;"
            "    background: transparent;"
            "    border: none;"
            "    text-align: left;"
            "}"
            "QToolButton:hover {"
            "    color: #409EFF;"
            "    background: #ECF5FF;"
            "}"
            "QToolButton:pressed {"
            "    color: #409EFF;"
            "    background: #D9ECFF;"
            "}"
            "QToolButton::menu-indicator {"
            "    image: url(:/mMainWidgets/Icon/homepage/arrow-down3x.png);"   // 你的图标
            "    width: 10px;"
            "    height: 10px;"
            "    subcontrol-position: right bottom;"   // 位置
            "    subcontrol-origin: padding;"
            "    margin-right: 2px;"
            "    margin-bottom: 2px;"
            "}"
            "QToolButton::menu-indicator:hover  {"
            " image: url(:/mMainWidgets/Icon/homepage/down-list1.png);"
            "}"
            "QCheckBox {"
            "    font-family: 'Microsoft YaHei';"
            "    font-size: 12px;"
            "    font-weight: normal;"
            "    color: #303133;"
            "    text-align: left;"
            "}"
            "QCheckBox::indicator {"
            "    width: 14px;"
            "    height: 14px;"
            "}";

        //求解组（MotionEngine已进Basic，恢复求解按钮）
        if (tGW0)
        {
            // 设置基础样式
            tGW0->setStyleSheet(groupTitleStyle);

            QToolButton* driveBtn = tGW0->addToolBtn("新建求解方案", ":/mMainWidgets/Icon/Motion/drive.png", -1, -1, 1, 1, QSize(30, 30));
            driveBtn->setMinimumHeight(45);
            driveBtn->setMinimumWidth(110);
            driveBtn->setStyleSheet(toolButtonStyle);

            connect(driveBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createMotionScenario();
                }
            );

            QToolButton* solveBtn = tGW0->addToolBtn("求解", ":/mMainWidgets/Icon/Motion/solve.png", -1, -1, 1, 1, QSize(30, 30));
            solveBtn->setMinimumHeight(45);
            solveBtn->setMinimumWidth(70);
            solveBtn->setStyleSheet(toolButtonStyle);

            connect(solveBtn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->solveMotionScenario();
                }
            );
        }

        //体组（体定义页）：批量建体在前（多部件 STP 主流程），单个运动体 + 自动柔性体随后
        if (tGW_body)
        {
            tGW_body->setStyleSheet(groupTitleStyle);

            //一键生成运动体：全部部件直接 1:1 建体（不进面板；名字=部件名，重名 -x）
            QToolButton* bodyOneKeyBtn = tGW_body->addToolBtn("一键生成运动体", ":/mMainWidgets/Icon/xuanzequanbu.png", -1, -1, 1, 1, QSize(30, 30));
            bodyOneKeyBtn->setMinimumHeight(45);
            bodyOneKeyBtn->setMinimumWidth(70);
            bodyOneKeyBtn->setStyleSheet(toolButtonStyle);
            bodyOneKeyBtn->setToolTip(QString::fromUtf8("当前模型全部部件各建一个运动体（部件同名，重名自动加 -x 后缀）"));

            connect(bodyOneKeyBtn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->createMotionBodyOneKey();
                }
            );

            //批量定义运动体：进面板选部件子集（列表/视口拾取），确定后逐部件建体
            QToolButton* bodyBatchBtn = tGW_body->addToolBtn("批量定义运动体", ":/mMainWidgets/Icon/Motion/bodyBatch.png", -1, -1, 1, 1, QSize(30, 30));
            bodyBatchBtn->setMinimumHeight(45);
            bodyBatchBtn->setMinimumWidth(70);
            bodyBatchBtn->setStyleSheet(toolButtonStyle);

            connect(bodyBatchBtn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->createMotionBodyBatch();
                }
            );

            QToolButton* mbBtn = tGW_body->addToolBtn("运动体", ":/mMainWidgets/Icon/Motion/movingBody.png", -1, -1, 1, 1, QSize(30, 30));
            mbBtn->setMinimumHeight(45);
            mbBtn->setMinimumWidth(70);
            mbBtn->setStyleSheet(toolButtonStyle);

            connect(mbBtn, &QToolButton::pressed, this, [this]() {mInfoWindow::getInstance()->createMotionBody();
                }
            );

            //自动柔性体并入"体"组（刚柔耦合闭环入口：面板内自带材料/网格/模态参数）
            QToolButton* autoFlexBtn = tGW_body->addToolBtn("自动柔性体", ":/mMainWidgets/Icon/Motion/mnf.png", -1, -1, 1, 1, QSize(30, 30));
            autoFlexBtn->setMinimumHeight(45);
            autoFlexBtn->setMinimumWidth(70);
            autoFlexBtn->setStyleSheet(toolButtonStyle);

            connect(autoFlexBtn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->createMotionAutoFlex();
                }
            );

            //标记点（运动副/力元定位参考，树"标记点"分组的创建入口）
            QToolButton* markerBtn = tGW_body->addToolBtn("标记点", ":/mMainWidgets/Icon/caotu/dian.png", -1, -1, 1, 1, QSize(30, 30));
            markerBtn->setMinimumHeight(45);
            markerBtn->setMinimumWidth(70);
            markerBtn->setStyleSheet(toolButtonStyle);

            connect(markerBtn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->createMotionMarker();
                }
            );
        }

        //运动副驱动组（驱动页）：按"控制模式"入口创建；加速度模式面板暂不支持，入口禁用
        if (tGW_drive)
        {
            tGW_drive->setStyleSheet(groupTitleStyle);

            auto addDriveBtn = [this, &toolButtonStyle](mGroupWidget* grp, const QString& text, bool enabled) {
                QToolButton* btn = grp->addToolBtn(text, ":/mMainWidgets/Icon/Motion/drive.png", -1, -1, 1, 1, QSize(30, 30));
                btn->setMinimumHeight(45);
                btn->setMinimumWidth(70);
                btn->setStyleSheet(toolButtonStyle);
                if (!enabled)
                {
                    btn->setEnabled(false);
                    btn->setToolTip(QString::fromUtf8("暂不支持"));
                }
                return btn;
            };

            QToolButton* posBtn = addDriveBtn(tGW_drive, "位置驱动", true);
            connect(posBtn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->createMotionDrive(DriveTargetType::Joint, MechMotionData::DriveControlMode::Position);
                });

            QToolButton* velBtn = addDriveBtn(tGW_drive, "速度驱动", true);
            connect(velBtn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->createMotionDrive(DriveTargetType::Joint, MechMotionData::DriveControlMode::Velocity);
                });

            //加速度模式：面板控制模式下拉仅"位置/速度"两项，暂不支持
            addDriveBtn(tGW_drive, "加速度驱动", false);
        }

        //运动体驱动组（驱动页）：面板仅有运动副驱动编辑器，三种模式全部暂不支持（禁用占位）
        if (tGW_drive2)
        {
            tGW_drive2->setStyleSheet(groupTitleStyle);

            auto addBodyDriveBtn = [this, &toolButtonStyle, tGW_drive2](const QString& text) {
                QToolButton* btn = tGW_drive2->addToolBtn(text, ":/mMainWidgets/Icon/Motion/drive.png", -1, -1, 1, 1, QSize(30, 30));
                btn->setMinimumHeight(45);
                btn->setMinimumWidth(70);
                btn->setStyleSheet(toolButtonStyle);
                btn->setEnabled(false);
                btn->setToolTip(QString::fromUtf8("暂不支持"));
                return btn;
            };
            addBodyDriveBtn("位置驱动");
            addBodyDriveBtn("速度驱动");
            addBodyDriveBtn("加速度驱动");
        }

        //传感器组（传感与控制页）：按类型铺开七个入口，直建对应类型面板
        if (tGW_sensor)
        {
            tGW_sensor->setStyleSheet(groupTitleStyle);

            //紧凑宽按钮：文字在图标右侧，两行铺排（上行 4 个、下行 3 个），同弹簧组样式；类型图标 sensor_*.png
            auto addSensorBtn = [this, &toolButtonStyle, tGW_sensor](int row, int col, const QString& text, const QString& icon, MechMotionData::SensorType type) {
                QToolButton* btn = tGW_sensor->addToolBtn(text, icon, row, col, 1, 1, QSize(20, 20));
                btn->setFixedHeight(26);
                btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                btn->setObjectName("wideButton");
                btn->setStyleSheet(toolButtonStyle);
                btn->setMinimumWidth(80);
                connect(btn, &QToolButton::clicked, this, [this, type]() {
                    mInfoWindow::getInstance()->createMotionSensor(type);
                });
                return btn;
            };

            addSensorBtn(0, 0, "体位移", ":/mMainWidgets/Icon/Motion/sensor_displacement.png", MechMotionData::SensorType::BodyDisplacement);
            addSensorBtn(0, 1, "体速度", ":/mMainWidgets/Icon/Motion/sensor_velocity.png", MechMotionData::SensorType::BodyVelocity);
            addSensorBtn(0, 2, "体加速度", ":/mMainWidgets/Icon/Motion/sensor_acceleration.png", MechMotionData::SensorType::BodyAcceleration);
            addSensorBtn(0, 3, "关节反力", ":/mMainWidgets/Icon/Motion/sensor_jointReaction.png", MechMotionData::SensorType::JointReaction);
            addSensorBtn(1, 0, "驱动输出", ":/mMainWidgets/Icon/Motion/sensor_driveActuation.png", MechMotionData::SensorType::DriveActuation);
            addSensorBtn(1, 1, "距离", ":/mMainWidgets/Icon/Motion/sensor_distance.png", MechMotionData::SensorType::Distance);
            addSensorBtn(1, 2, "角度", ":/mMainWidgets/Icon/Motion/sensor_angle.png", MechMotionData::SensorType::Angle);
        }
        if (tGW_ctrl)
        {
            tGW_ctrl->setStyleSheet(groupTitleStyle);

            QToolButton* controllerBtn = tGW_ctrl->addToolBtn("控制器", ":/mMainWidgets/Icon/Motion/controlCalculation.png", -1, -1, 1, 1, QSize(30, 30));
            controllerBtn->setMinimumHeight(45);
            controllerBtn->setMinimumWidth(70);
            controllerBtn->setStyleSheet(toolButtonStyle);

            connect(controllerBtn, &QToolButton::pressed, this, [this]() {
                mInfoWindow::getInstance()->createMotionController();
                }
            );
        }

        //运动副组拆分：基本运动副 + 几何约束（运动副与约束页）
        if (tGW2)
        {
            // 直接应用之前定义的样式表
            tGW2->setStyleSheet(groupTitleStyle);

            //紧凑宽按钮：一列两个（文字在图标右侧）
            auto addJointBtn = [this, &toolButtonStyle](mGroupWidget* grp, const QString& text, const QString& icon, int row, int col, JointType jt) {
                QToolButton* btn = grp->addToolBtn(text, icon, row, col, 1, 1, QSize(20, 20));
                btn->setFixedHeight(26);
                btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                btn->setObjectName("wideButton");
                btn->setStyleSheet(toolButtonStyle);
                btn->setMinimumWidth(80);
                connect(btn, &QToolButton::pressed, this, [this, jt]() {
                    mInfoWindow::getInstance()->createMotionJoint(jt);
                });
                return btn;
            };

            //常用四副：各自独占一列（跨两行大按钮，图标上文字下）
            auto addCommonJointBtn = [this, &toolButtonStyle](mGroupWidget* grp, const QString& text, const QString& icon, int col, JointType jt) {
                QToolButton* btn = grp->addToolBtn(text, icon, 0, col, 2, 1, QSize(30, 30));
                btn->setMinimumHeight(50);
                btn->setMinimumWidth(70);
                btn->setStyleSheet(toolButtonStyle);
                connect(btn, &QToolButton::pressed, this, [this, jt]() {
                    mInfoWindow::getInstance()->createMotionJoint(jt);
                });
                return btn;
            };

            addCommonJointBtn(tGW2, "旋转副",   ":/mMainWidgets/Icon/Motion/joint_revolute.png", 0, JointType::Revolute);
            addCommonJointBtn(tGW2, "移动副",   ":/mMainWidgets/Icon/Motion/joint_prismatic.png", 1, JointType::Translational);
            addCommonJointBtn(tGW2, "柱面副",   ":/mMainWidgets/Icon/Motion/joint_cylindrical.png", 2, JointType::Cylindrical);
            addCommonJointBtn(tGW2, "球面副",   ":/mMainWidgets/Icon/Motion/joint_spherical.png", 3, JointType::Spherical);

            //其余按列排放：一列上下两个（第4~7列）
            addJointBtn(tGW2, "固定副",   ":/mMainWidgets/Icon/Motion/joint_fixed.png", 0, 4, JointType::Fixed);
            addJointBtn(tGW2, "万向节",   ":/mMainWidgets/Icon/Motion/joint_universal.png", 1, 4, JointType::Universal);
            addJointBtn(tGW2, "恒速副",   ":/mMainWidgets/Icon/Motion/joint_cv.png", 0, 5, JointType::ConstantVelocity);
            addJointBtn(tGW2, "螺纹副",   ":/mMainWidgets/Icon/Motion/joint_screw.png", 1, 5, JointType::Screw);
            addJointBtn(tGW2, "平面副",   ":/mMainWidgets/Icon/Motion/joint_planar.png", 0, 6, JointType::Planar);
            addJointBtn(tGW2, "点线副",   ":/mMainWidgets/Icon/Motion/joint_pointLine.png", 1, 6, JointType::PointLine);
            addJointBtn(tGW2, "点面副",   ":/mMainWidgets/Icon/Motion/joint_pointPlane.png", 0, 7, JointType::PointPlane);

            //几何约束：一列两个，三列两行
            addJointBtn(tGW2b, "点在面上副", ":/mMainWidgets/Icon/Motion/joint_pointOnSurface.png", 0, 0, JointType::PointOnSurface);
            addJointBtn(tGW2b, "点在线上副", ":/mMainWidgets/Icon/Motion/joint_pointOnCurve.png", 1, 0, JointType::PointOnCurve);
            addJointBtn(tGW2b, "线在线上副", ":/mMainWidgets/Icon/Motion/joint_curveOnCurve.png", 0, 1, JointType::CurveOnCurve);
            addJointBtn(tGW2b, "平行约束", ":/mMainWidgets/Icon/Motion/joint_parallel.png", 1, 1, JointType::Parallel);
            addJointBtn(tGW2b, "垂直约束", ":/mMainWidgets/Icon/Motion/joint_perpendicular.png", 0, 2, JointType::Perpendicular);
            addJointBtn(tGW2b, "方向约束", ":/mMainWidgets/Icon/Motion/joint_direction.png", 1, 2, JointType::Direction);
        }
        //几何约束组样式（按钮由上方 tGW2 块统一填充）
        if (tGW2b)
        {
            tGW2b->setStyleSheet(groupTitleStyle);
        }

        // 耦合副组：三个各自独占一列（跨两行大按钮，与基本运动副常用四副同款，填满组高）
        if (tGW3)
        {
            tGW3->setStyleSheet(groupTitleStyle);

            auto addCouplingBtn = [this, &toolButtonStyle, tGW3](const QString& text, const QString& icon, int col, CouplingType jt) {
                QToolButton* btn = tGW3->addToolBtn(text, icon, 0, col, 2, 1, QSize(30, 30));
                btn->setMinimumHeight(50);
                btn->setMinimumWidth(70);
                btn->setStyleSheet(toolButtonStyle);
                connect(btn, &QToolButton::pressed, this, [this, jt]() {
                    mInfoWindow::getInstance()->createMotionCoupling(jt);
                });
                return btn;
            };

            addCouplingBtn("齿轮副",     ":/mMainWidgets/Icon/Motion/coupling_gear.png",     0, CouplingType::Gear);
            addCouplingBtn("齿轮齿条副", ":/mMainWidgets/Icon/Motion/coupling_gearRack.png", 1, CouplingType::GearRack);
            addCouplingBtn("线缆副",     ":/mMainWidgets/Icon/Motion/coupling_cable.png",    2, CouplingType::Cable);
        }
        //柔性连接组
        if (tGW5)
        {
            tGW5->setStyleSheet(groupTitleStyle);
            QToolButton* springBtn = tGW5->addToolBtn("体点弹簧", ":/mMainWidgets/Icon/Motion/force_spring.png", 0, 0, 1, 1, QSize(20, 20));
            springBtn->setFixedHeight(26);
            springBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
            springBtn->setObjectName("wideButton");
            springBtn->setStyleSheet(toolButtonStyle);
            springBtn->setMinimumWidth(80);
            connect(springBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createMotionSpring(static_cast<int>(MechMotionData::SpringType::BodyPoint));
                });

            //衬套移回"加载力"组（力元家族）

            QToolButton* ctBtn = tGW5->addToolBtn("拉压弹簧", ":/mMainWidgets/Icon/Motion/tcSpring.png", 0, 1, 1, 1, QSize(20, 20));
            ctBtn->setFixedHeight(26);
            ctBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
            ctBtn->setObjectName("wideButton");
            ctBtn->setStyleSheet(toolButtonStyle);
            ctBtn->setMinimumWidth(80);
            connect(ctBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createMotionSpring(static_cast<int>(MechMotionData::SpringType::Translational));
            });

            QToolButton* beamBtn = tGW5->addToolBtn("梁单元", ":/mMainWidgets/Icon/Motion/beam.png", 1, 0, 1, 1, QSize(20, 20));
            beamBtn->setFixedHeight(26);
            beamBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
            beamBtn->setObjectName("wideButton");
            beamBtn->setStyleSheet(toolButtonStyle);
            beamBtn->setMinimumWidth(80);
            //梁单元暂不支持：面板未实现，禁用占位
            beamBtn->setEnabled(false);
            beamBtn->setToolTip(QString::fromUtf8("暂不支持"));

            QToolButton* tsBtn = tGW5->addToolBtn("扭转弹簧", ":/mMainWidgets/Icon/Motion/torsionSpring.png", 0, 2, 1, 1, QSize(20, 20));
            tsBtn->setFixedHeight(26);
            tsBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
            tsBtn->setObjectName("wideButton");
            tsBtn->setStyleSheet(toolButtonStyle);
            tsBtn->setMinimumWidth(80);
            connect(tsBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createMotionSpring(static_cast<int>(MechMotionData::SpringType::Torsional));
            });


            QToolButton* axletreeBtn = tGW5->addToolBtn("轴承", ":/mMainWidgets/Icon/Motion/axletree.png", 1, 1, 1, 1, QSize(20, 20));
            axletreeBtn->setFixedHeight(26);
            axletreeBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
            axletreeBtn->setObjectName("wideButton");
            axletreeBtn->setStyleSheet(toolButtonStyle);
            axletreeBtn->setMinimumWidth(80);
            //轴承暂不支持：面板未实现，禁用占位
            axletreeBtn->setEnabled(false);
            axletreeBtn->setToolTip(QString::fromUtf8("暂不支持"));
        }

        //加载力组（两行×七列；按求解装配（mMotionSolverThread）实际支持的类型启用，
        //未传输的类型禁用占位：重力/力场/一般作用力/线性弹簧阻尼/轮胎力/电磁力/自定义力）
        if (tGW6)
        {
            tGW6->setStyleSheet(groupTitleStyle);

            auto addForceBtn = [this, &toolButtonStyle, tGW6](int row, int col, const QString& text, const QString& icon, bool enabled) {
                QToolButton* btn = tGW6->addToolBtn(text, icon, row, col, 1, 1, QSize(20, 20));
                btn->setFixedHeight(26);
                btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                btn->setObjectName("wideButton");
                btn->setStyleSheet(toolButtonStyle);
                btn->setMinimumWidth(80);
                if (!enabled)
                {
                    btn->setEnabled(false);
                    btn->setToolTip(QString::fromUtf8("暂不支持"));
                }
                return btn;
            };

            QToolButton* fb = nullptr;
            //第一行：求解支持的类型（矢量力→ADMForce、矢量力矩→ADMTorque、三点力/矩、标量力/矩、接触）
            fb = addForceBtn(0, 0, "矢量力", ":/mMainWidgets/Icon/Motion/force_vector.png", true);
            connect(fb, &QToolButton::pressed, this, [this]() { mInfoWindow::getInstance()->createMotionVectorForce(MechMotionData::VectorLoadType::Force); });
            fb = addForceBtn(0, 1, "矢量力矩", ":/mMainWidgets/Icon/Motion/force_torque.png", true);
            connect(fb, &QToolButton::pressed, this, [this]() { mInfoWindow::getInstance()->createMotionVectorForce(MechMotionData::VectorLoadType::Torque); });
            fb = addForceBtn(0, 2, "三点力", ":/mMainWidgets/Icon/Motion/force_tri.png", true);
            connect(fb, &QToolButton::pressed, this, [this]() { mInfoWindow::getInstance()->createMotionVectorForce(MechMotionData::VectorLoadType::TriForce); });
            fb = addForceBtn(0, 3, "三点力矩", ":/mMainWidgets/Icon/Motion/force_tri_torque.png", true);
            connect(fb, &QToolButton::pressed, this, [this]() { mInfoWindow::getInstance()->createMotionVectorForce(MechMotionData::VectorLoadType::TriTorque); });
            fb = addForceBtn(0, 4, "标量力", ":/mMainWidgets/Icon/Motion/force.png", true);
            connect(fb, &QToolButton::pressed, this, [this]() { mInfoWindow::getInstance()->unEditInfoWindow(); mInfoWindow::getInstance()->createMotionVectorForce(MechMotionData::VectorLoadType::ScalarForce); });
            fb = addForceBtn(0, 5, "标量力矩", ":/mMainWidgets/Icon/Motion/moment.png", true);
            connect(fb, &QToolButton::pressed, this, [this]() { mInfoWindow::getInstance()->unEditInfoWindow(); mInfoWindow::getInstance()->createMotionVectorForce(MechMotionData::VectorLoadType::ScalarTorque); });
            //接触力：直连创建（当前仅 3D 接触一种类型）
            fb = addForceBtn(0, 6, "接触力", ":/mMainWidgets/Icon/Motion/contact.png", true);
            connect(fb, &QToolButton::pressed, this, [this]() { mInfoWindow::getInstance()->createMotionContact(0); });
            fb = addForceBtn(0, 7, "衬套", ":/mMainWidgets/Icon/Motion/bush.png", true);
            connect(fb, &QToolButton::pressed, this, [this]() { mInfoWindow::getInstance()->createMotionVectorForce(MechMotionData::VectorLoadType::Bushing); });

            //第二行：求解暂未传输的类型，禁用占位
            addForceBtn(1, 0, "重力", ":/mMainWidgets/Icon/Motion/force_gravity.png", false);
            addForceBtn(1, 1, "力场", ":/mMainWidgets/Icon/Motion/force_field.png", false);
            addForceBtn(1, 2, "一般作用力", ":/mMainWidgets/Icon/Motion/force_general.png", false);
            addForceBtn(1, 3, "线性弹簧阻尼", ":/mMainWidgets/Icon/Motion/force_spring.png", false);
            addForceBtn(1, 4, "轮胎力", ":/mMainWidgets/Icon/Motion/force_tire.png", false);
            addForceBtn(1, 5, "电磁力", ":/mMainWidgets/Icon/Motion/electromagnetism.png", false);
            addForceBtn(1, 6, "自定义力", ":/mMainWidgets/Icon/Motion/custom.png", false);
        }
        //控制模块组
        if (tGW7)
        {
            tGW7->setStyleSheet(groupTitleStyle);
            QToolButton* ciBtn = tGW7->addToolBtn("控制输入", ":/mMainWidgets/Icon/Motion/controlInput.png", 0, 0, 1, 1, QSize(20, 20));
            ciBtn->setFixedHeight(26);
            ciBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            ciBtn->setObjectName("wideButton");
            ciBtn->setStyleSheet(toolButtonStyle);
            ciBtn->setMinimumWidth(80);
            connect(ciBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createControlInput();
            });

            QToolButton* ecBtn = tGW7->addToolBtn("机电耦合", ":/mMainWidgets/Icon/Motion/electromechanicalCoupling.png", 1, 0, 1, 1, QSize(20, 20));
            ecBtn->setFixedHeight(26);
            ecBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            ecBtn->setObjectName("wideButton");
            ecBtn->setStyleSheet(toolButtonStyle);
            ecBtn->setMinimumWidth(80);
            ecBtn->setPopupMode(QToolButton::InstantPopup);
            QMenu* ecMenu = new mUiMenu(this);
            ThemeManager::applyMenuStyle(ecMenu);
            ecMenu->addAction(QIcon(":/mMainWidgets/Icon/Motion/mech_electro_import.png"), "机电导入", this, [this]() {
                mInfoWindow::getInstance()->createElectroImport();
            });
            ecMenu->addAction(QIcon(":/mMainWidgets/Icon/Motion/mech_electro_export.png"), "机电导出", this, [this]() {
                mInfoWindow::getInstance()->createElectroExport();
            });
            ecMenu->addSeparator();
            ecMenu->addAction(QIcon(":/mMainWidgets/Icon/Motion/mech_hydraulic.png"), "液压联合仿真", this, [this]() {
                mInfoWindow::getInstance()->createHydraulic();
            });
            ecBtn->setMenu(ecMenu);

            QToolButton* rlBtn = tGW7->addToolBtn("强化学习", ":/mMainWidgets/Icon/Motion/controlCalculation.png", 0, 1, 1, 1, QSize(20, 20));
            rlBtn->setFixedHeight(26);
            rlBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            rlBtn->setObjectName("wideButton");
            rlBtn->setStyleSheet(toolButtonStyle);
            rlBtn->setMinimumWidth(80);
            connect(rlBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createRLPanel();
            });

            QToolButton* coBtn = tGW7->addToolBtn("控制输出", ":/mMainWidgets/Icon/Motion/controlOutput.png", 1, 1, 1, 1, QSize(20, 20));
            coBtn->setFixedHeight(26);
            coBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            coBtn->setObjectName("wideButton");
            coBtn->setStyleSheet(toolButtonStyle);
            coBtn->setMinimumWidth(80);
        }

        //飞机专用模块组（源版即无动作连接，纯入口展示）
        if (tGW9)
        {
            tGW9->setStyleSheet(groupTitleStyle);
            QToolButton* undercartBtn = tGW9->addToolBtn("起落架", ":/mMainWidgets/Icon/Motion/undercart.png", 0, 0, 1, 1, QSize(20, 20));
            undercartBtn->setFixedHeight(26);
            undercartBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
            undercartBtn->setObjectName("wideButton");
            undercartBtn->setStyleSheet(toolButtonStyle);
            undercartBtn->setMinimumWidth(70);

            QToolButton* tyreBtn = tGW9->addToolBtn("轮胎", ":/mMainWidgets/Icon/Motion/force_tire.png", 1, 0, 1, 1, QSize(20, 20));
            tyreBtn->setFixedHeight(26);
            tyreBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
            tyreBtn->setObjectName("wideButton");
            tyreBtn->setStyleSheet(toolButtonStyle);
            tyreBtn->setMinimumWidth(70);

            QToolButton* pavementBtn = tGW9->addToolBtn("路面", ":/mMainWidgets/Icon/Motion/pavement.png", 0, 1, 1, 1, QSize(20, 20));
            pavementBtn->setFixedHeight(26);
            pavementBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
            pavementBtn->setObjectName("wideButton");
            pavementBtn->setStyleSheet(toolButtonStyle);
            pavementBtn->setMinimumWidth(80);

            QToolButton* rgBtn = tGW9->addToolBtn("报告生成", ":/mMainWidgets/Icon/Motion/aircraftReport.png", 1, 1, 1, 1, QSize(20, 20));
            rgBtn->setFixedHeight(26);
            rgBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
            rgBtn->setObjectName("wideButton");
            rgBtn->setStyleSheet(toolButtonStyle);
            rgBtn->setMinimumWidth(80);
        }

        //系统建模工具组
        if (tGW10)
        {
            tGW10->setStyleSheet(groupTitleStyle);
            QToolButton* mechBtn = tGW10->addToolBtn("机构", ":/mMainWidgets/Icon/Motion/rotaryJoint.png", 0, 0, 1, 1, QSize(20, 20));
            mechBtn->setFixedHeight(26);
            mechBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            mechBtn->setObjectName("wideButton");
            mechBtn->setStyleSheet(toolButtonStyle);
            mechBtn->setMinimumWidth(70);
            connect(mechBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createMechanism();
            });

            QToolButton* transBtn = tGW10->addToolBtn("传动链", ":/mMainWidgets/Icon/Motion/GearPair.png", 1, 0, 1, 1, QSize(20, 20));
            transBtn->setFixedHeight(26);
            transBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            transBtn->setObjectName("wideButton");
            transBtn->setStyleSheet(toolButtonStyle);
            transBtn->setMinimumWidth(70);
            connect(transBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createTransmission();
            });

            QToolButton* mehBtn = tGW10->addToolBtn("机电液", ":/mMainWidgets/Icon/Motion/mech_elec_hyd.png", 0, 1, 1, 1, QSize(20, 20));
            mehBtn->setFixedHeight(26);
            mehBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            mehBtn->setObjectName("wideButton");
            mehBtn->setStyleSheet(toolButtonStyle);
            mehBtn->setMinimumWidth(70);
            connect(mehBtn, &QToolButton::clicked, this, [this]() {
                mInfoWindow::getInstance()->createMechElecHyd();
            });
        }
    }

    //-------------------important----------------------------------------
    void mMenuWidget::initFunctionMenu()
    {
        RibbonPage* newPage = new RibbonPage();
        m_toolBarStackedWid_->addWidget(newPage);
        m_StaWidMap.insert("函数", newPage);                    // 记录映射

        /* 3. 往新页面里专门加“截面形状”组 */
        std::shared_ptr<mGroupWidget> shapeGW = std::shared_ptr<mGroupWidget>(this->addGroup("函数", "创建"));
        std::shared_ptr<mGroupWidget> curveGW = std::shared_ptr<mGroupWidget>(this->addGroup("函数", "曲线"));


        _functionGW.push_back(shapeGW);
        _functionGW.push_back(curveGW);
        if (shapeGW)
        {
            /* 4. 往组里加真正的形状按钮 */

            /*enum SecType {
                Sec_H = 1, Sec_I, Sec_L, Sec_T, Sec_Box, Sec_Rec, Sec_Cir,
                Sec_Ann, Sec_Polygon, Sec_Channel, Sec_Channel1,
                Sec_I1, Sec_T1, Sec_T2, Sec_Box1, Sec_Cross, Sec_Hat, Sec_Z
            };*/

            //【解析 离散 分段 阶跃->屏蔽 】 【表格 可平移表格 可缩放表格 幂函数->添加】
            /*auto HBtn = shapeGW->addToolBtn("解析", ":/mMainWidgets/Icon/function/jiexi.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(HBtn, "解析", ":/mMainWidgets/Icon/function/jiexi.png", "创建函数。");
            HBtn->setProperty("sectionType", "解析");
            connect(HBtn, &QToolButton::clicked, [=]() {slot_createFunction(0); });

            auto LBtn = shapeGW->addToolBtn("离散", ":/mMainWidgets/Icon/function/lisan.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(LBtn, "离散", ":/mMainWidgets/Icon/function/lisan.png", "创建函数。");
            LBtn->setProperty("sectionType", "离散");
            connect(LBtn, &QToolButton::clicked, [=]() {slot_createFunction(1); });

            auto I1Btn = shapeGW->addToolBtn("分段", ":/mMainWidgets/Icon/function/fenduan.png", 0, 3, 1, 1, QSize(30, 30));
            setQToolButton(I1Btn, "分段", ":/mMainWidgets/Icon/function/fenduan.png", "创建函数。");
            I1Btn->setProperty("sectionType", "分段");
            connect(I1Btn, &QToolButton::clicked, [=]() {slot_createFunction(3); });

            auto TBtn = shapeGW->addToolBtn("阶跃", ":/mMainWidgets/Icon/function/jieyue.png", 0, 4, 1, 1, QSize(30, 30));
            setQToolButton(TBtn, "阶跃", ":/mMainWidgets/Icon/function/jieyue.png", "创建函数。");
            TBtn->setProperty("sectionType", "阶跃");
            connect(TBtn, &QToolButton::clicked, [=]() {slot_createFunction(4); });*/

            auto IBtn = shapeGW->addToolBtn("插值", ":/mMainWidgets/Icon/function/chazhi.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(IBtn, "插值", ":/mMainWidgets/Icon/function/chazhi.png", "创建函数。");
            IBtn->setProperty("sectionType", "插值");
            connect(IBtn, &QToolButton::clicked, [=]() {slot_createFunction(2); });

            auto IBtn1 = shapeGW->addToolBtn("表格", ":/mMainWidgets/Icon/function/biaoge.png", 0, 2, 1, 1, QSize(30, 30));
            setQToolButton(IBtn1, "表格", ":/mMainWidgets/Icon/function/biaoge.png", "创建函数。");
            IBtn1->setProperty("sectionType", "表格");
            connect(IBtn1, &QToolButton::clicked, [=]() {slot_createFunction(5); });

            auto IBtn2 = shapeGW->addToolBtn("可平移表格", ":/mMainWidgets/Icon/function/kepingyi.png", 0, 3, 1, 1, QSize(30, 30));
            setQToolButton(IBtn2, "可平移表格", ":/mMainWidgets/Icon/function/kepingyi.png", "创建函数。");
            IBtn2->setProperty("sectionType", "可平移表格");
            connect(IBtn2, &QToolButton::clicked, [=]() {slot_createFunction(6); });

            auto IBtn3 = shapeGW->addToolBtn("可缩放表格", ":/mMainWidgets/Icon/function/kesuofang.png", 0, 4, 1, 1, QSize(30, 30));
            setQToolButton(IBtn3, "可缩放表格", ":/mMainWidgets/Icon/function/kesuofang.png", "创建函数。");
            IBtn3->setProperty("sectionType", "可缩放表格");
            connect(IBtn3, &QToolButton::clicked, [=]() {slot_createFunction(7); });

            auto IBtn4 = shapeGW->addToolBtn("幂函数", ":/mMainWidgets/Icon/function/mihanshu.png", 0, 5, 1, 1, QSize(30, 30));
            setQToolButton(IBtn4, "幂函数", ":/mMainWidgets/Icon/function/mihanshu.png", "创建函数。");
            IBtn4->setProperty("sectionType", "幂函数");
            connect(IBtn4, &QToolButton::clicked, [=]() {slot_createFunction(8); });

            auto IBtn5 = shapeGW->addToolBtn("周期函数", ":/mMainWidgets/Icon/function/mihanshu.png", 0, 6, 1, 1, QSize(30, 30));
            setQToolButton(IBtn5, "周期函数", ":/mMainWidgets/Icon/function/mihanshu.png", "创建函数。");
            IBtn5->setProperty("sectionType", "周期函数");
            connect(IBtn5, &QToolButton::clicked, [=]() {slot_createFunction(9); });

            auto IBtn6 = shapeGW->addToolBtn("调制函数", ":/mMainWidgets/Icon/function/mihanshu.png", 0, 7, 1, 1, QSize(30, 30));
            setQToolButton(IBtn6, "调制函数", ":/mMainWidgets/Icon/function/mihanshu.png", "创建函数。");
            IBtn6->setProperty("sectionType", "调制函数");
            connect(IBtn6, &QToolButton::clicked, [=]() {slot_createFunction(10); });

            auto IBtn7 = shapeGW->addToolBtn("指数衰减函数", ":/mMainWidgets/Icon/function/mihanshu.png", 0, 8, 1, 1, QSize(30, 30));
            setQToolButton(IBtn7, "指数衰减函数", ":/mMainWidgets/Icon/function/mihanshu.png", "创建函数。");
            IBtn7->setProperty("sectionType", "指数衰减函数");
            connect(IBtn7, &QToolButton::clicked, [=]() {slot_createFunction(11); });

        }
        if (curveGW)
        {

            auto HBtn = curveGW->addToolBtn("导出数据", ":/mMainWidgets/Icon/function/daochu.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(HBtn, "导出数据", ":/mMainWidgets/Icon/function/daochu.png", "导出数据。");
            HBtn->setProperty("sectionType", "导出数据");


            auto exportData = [=]() {
                int Id = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getMaxID();

                //if (directModelingGW)
                //{
                //	//直接建模：点击进入/退出直接建模模式（不再强制弹面板，面板仅作可选精确输入）
                //	QToolButton* directModelingBtn(directModelingGW->addToolBtn("直接建模", ":/mMainWidgets/Icon/geometry/zidongquchu.png", -1, -1, 1, 1, QSize(30, 30)));
                //	setQToolButton(directModelingBtn, "直接建模", ":/mMainWidgets/Icon/geometry/zidongquchu.png", "直接建模：拾取面/边后拖拽箭头编辑。\n再次点击或 ESC 退出。\n");
                //	directModelingBtn->setMinimumWidth(60);  // 设置最小宽度
                //	directModelingBtn->setMinimumHeight(50);  // 设置最小高度
                //	directModelingBtn->setCheckable(true);   // 选中态反映模式激活状态
                //	directModelingBtn->installEventFilter(new TooltipInstaller("直接建模",
                //		QPixmap(":/mMainWidgets/Icon/geometry/zidongquchu.png"),
                //		"直接建模：拾取面/边后拖拽箭头编辑。\n再次点击或 ESC 退出。\n",
                //		200, directModelingBtn));
                //	connect(directModelingBtn, &QToolButton::clicked, this, [this, directModelingBtn](bool checked) {
                //		toggleDirectModelingMode(checked);
                //		});
                //	// Controller 模式状态变化时同步按钮选中态（ESC 退出等路径）
                //	connect(DirectModelingController::getInstance(), &DirectModelingController::modeChanged,
                //			directModelingBtn, &QToolButton::setChecked);
                //}
                        //type:0解析 1离散 2插值 3分段 4阶跃
                int type = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->getFunctionType();
                QString fn;
                if (type == 0)
                {
                    const auto& ac = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->analytic();
                    if (ac.expr.isEmpty())
                    {
                        QMessageBox::warning(this,                       // 父窗口
                            "警告",                     // 标题
                            "数据不完整！");// 提示文本
                        return;
                    }
                    fn = QFileDialog::getSaveFileName(this, "导出数据", "analytic.txt", "Text Files (*.txt)");
                    if (fn.isEmpty()) return;
                    QFile f(fn);
                    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
                    QTextStream ts(&f);
                    OUT_HEADER(ts, "Analytic");
                    ts << "# Expression: " << ac.expr << "\n";
                    ts << "# x\ty\n";

                    mu::Parser p;  double x = 0;  p.DefineVar("x", &x);
                    p.SetExpr(ac.expr.toStdString());
                    double dx = (ac.xMax - ac.xMin) / (ac.simple - 1);
                    for (int i = 0; i < ac.simple; ++i) {
                        x = ac.xMin + i * dx;
                        ts << x << '\t' << p.Eval() << '\n';
                    }
                    //_styleDialog->show();

                }
                else if (type == 1)
                {
                    const auto& d = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->discrete();
                    if (d.x.empty())
                    {
                        QMessageBox::warning(this,                       // 父窗口
                            "警告",                     // 标题
                            "数据不完整！");// 提示文本
                        return;
                    }

                    fn = QFileDialog::getSaveFileName(this, "导出数据", "discrete.txt", "Text Files (*.txt)");
                    if (fn.isEmpty()) return;
                    QFile f(fn);  if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
                    QTextStream ts(&f);
                    OUT_HEADER(ts, "Discrete");
                    ts << "# x\ty\n";
                    for (int i = 0; i < d.x.size(); ++i) ts << d.x[i] << '\t' << d.y[i] << '\n';
                }
                else if (type == 2)
                {
                    const auto& ic = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->interp();

                    if (ic.x.empty())
                    {
                        QMessageBox::warning(this,                       // 父窗口
                            "警告",                     // 标题
                            "数据不完整！");// 提示文本
                        return;
                    }
                    fn = QFileDialog::getSaveFileName(this, "导出数据", "interp.txt", "Text Files (*.txt)");
                    if (fn.isEmpty()) return;
                    QFile f(fn);
                    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
                    QTextStream ts(&f);
                    OUT_HEADER(ts, "Interp");
                    ts << "# InterpMode: " << ic.interpMode
                        << "  ExtrapMode: " << ic.extrapMode << "\n";
                    ts << "# x\ty\n";
                    for (size_t i = 0; i < ic.x.size(); ++i)
                        ts << ic.x[i] << '\t' << ic.y[i] << '\n';
                }
                else if (type == 3)
                {
                    const auto& pc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->piecewise();
                    if (pc.start.empty())
                    {
                        QMessageBox::warning(this,                       // 父窗口
                            "警告",                     // 标题
                            "数据不完整！");// 提示文本
                        return;
                    }
                    fn = QFileDialog::getSaveFileName(this, "导出数据", "piecewise.txt", "Text Files (*.txt)");
                    if (fn.isEmpty()) return;
                    QFile f(fn);
                    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
                    QTextStream ts(&f);
                    OUT_HEADER(ts, "Piecewise");
                    ts << "# SegmentIndex\tStart\tEnd\tExpression\n";

                    for (int i = 0; i < pc.start.size(); ++i) {
                        // 确保单元格非空
                        /*auto* itStart = ensureItem(tableWidget_3, i, 1);
                        auto* itEnd = ensureItem(tableWidget_3, i, 2);
                        auto* itExpr = ensureItem(tableWidget_3, i, 3);*/

                        ts << (i + 1) << '\t'
                            << pc.start[i] << '\t'
                            << pc.end[i] << '\t'
                            << pc.expr[i] << '\n';
                    }
                }
                else if (type == 4)
                {
                    const auto& sc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->step();
                    fn = QFileDialog::getSaveFileName(this, "导出数据", "step.txt", "Text Files (*.txt)");
                    if (fn.isEmpty())
                    {
                        return;
                    }
                    QFile f(fn);
                    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
                    QTextStream ts(&f);
                    OUT_HEADER(ts, "Step");
                    ts << "# StepPos: " << sc.pos << "  V0: " << sc.v0 << "  V1: " << sc.v1 << "\n";
                    ts << "# x\ty\n";

                    // 用顶部“变量范围”作为左右区间
                    double x0 = sc.xMin;
                    double x1 = sc.xMax;
                    int N = 10;
                    double dxL = (sc.pos - x0) / (N - 1);
                    double dxR = (x1 - sc.pos) / (N - 1);
                    for (int i = 0; i < N; ++i) ts << (x0 + i * dxL) << '\t' << sc.v0 << '\n';
                    for (int i = 0; i < N; ++i) ts << (sc.pos + i * dxR) << '\t' << sc.v1 << '\n';
                }
                mGlobalSignals::getInstance()->outputMessageSig(QString("成功导出数据到 %1").arg(fn), true);

                };
            connect(HBtn, &QToolButton::clicked, exportData);

            auto LBtn = curveGW->addToolBtn("查看数据", ":/mMainWidgets/Icon/function/chakan.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(LBtn, "查看数据", ":/mMainWidgets/Icon/function/chakan.png", "查看数据。");
            LBtn->setProperty("sectionType", "查看数据");

            auto checkData = [=]() {
                int Id = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getMaxID();

                mTableWidget* tableWidget = mTableDataWindow::getInstance()->getTableWidget();
                tableWidget->clear();
                //type:0解析 1离散 2插值 3分段 4阶跃
                int type = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->getFunctionType();
                if (type == 0)
                {
                    const auto& ac = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->analytic();
                    tableWidget->setRowCount(ac.simple);
                    tableWidget->setColumnCount(2);

                    double step = (ac.xMax - ac.xMin) / (ac.simple - 1);
                    for (int i = 0; i < ac.simple; ++i) {
                        tableWidget->setItem(i, 0, new QTableWidgetItem(MxFunctions::mNumberToString(i + 1)));
                        tableWidget->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(ac.xMin + i * step)));
                    }
                }
                else if (type == 1)
                {
                    const auto& d = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->discrete();
                    tableWidget->setRowCount(d.idx.size());
                    tableWidget->setColumnCount(2);

                    for (int i = 0; i < d.idx.size(); ++i) {
                        tableWidget->setItem(i, 0, new QTableWidgetItem(MxFunctions::mNumberToString(d.x.at(i))));
                        tableWidget->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(d.y.at(i))));
                    }
                }
                else if (type == 2)
                {
                    const auto& ic = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->interp();
                    tableWidget->setRowCount(ic.x.size());
                    tableWidget->setColumnCount(2);
                    for (int i = 0; i < ic.x.size(); ++i) {
                        tableWidget->setItem(i, 0, new QTableWidgetItem(MxFunctions::mNumberToString(ic.x.at(i))));
                        tableWidget->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(ic.y.at(i))));
                    }
                }
                else if (type == 3)
                {
                    const auto& pc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->piecewise();
                    tableWidget->setColumnCount(2);
                }
                else if (type == 4)
                {
                    const auto& sc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(Id)->step();
                    int N = 10;
                    tableWidget->setColumnCount(2);
                    tableWidget->setRowCount(2 * N);
                    double x0 = sc.xMin;
                    double x1 = sc.xMax;

                    double dxL = (sc.pos - x0) / (N - 1);
                    double dxR = (x1 - sc.pos) / (N - 1);
                    for (int i = 0; i < N; ++i)
                    {
                        tableWidget->setItem(i, 0, new QTableWidgetItem(MxFunctions::mNumberToString(x0 + i * dxL)));
                        tableWidget->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(sc.v0)));
                    }
                    for (int i = 0; i < N; ++i)
                    {
                        tableWidget->setItem(i + N, 0, new QTableWidgetItem(MxFunctions::mNumberToString(sc.pos + i * dxR)));
                        tableWidget->setItem(i + N, 1, new QTableWidgetItem(MxFunctions::mNumberToString(sc.v1)));

                    }
                }
                };
            connect(LBtn, &QToolButton::clicked, checkData);

            auto IBtn = curveGW->addToolBtn("设置样式", ":/mMainWidgets/Icon/function/shezhi.png", 0, 2, 1, 1, QSize(30, 30));
            setQToolButton(IBtn, "设置样式", ":/mMainWidgets/Icon/function/shezhi.png", "设置样式。");
            IBtn->setProperty("sectionType", "设置样式");
            auto setStyle = [=]() {
                mCurveWindow::getInstance()->getPlotView()->getPlotViewStyleWidget()->show();
                //_styleDialog->show();
                };
            connect(IBtn, &QToolButton::clicked, setStyle);


        }


    }

    void mMenuWidget::initGeometrySketch()
    {
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("草图", tempWid);

        std::shared_ptr<mGroupWidget> sketchCreateGW = std::shared_ptr<mGroupWidget>(this->addGroup("草图", "创建"));
        //std::shared_ptr<mGroupWidget> sketchConstraintGW = std::shared_ptr<mGroupWidget>(this->addGroup("草图", "约束"));
        std::shared_ptr<mGroupWidget> sketchExitGW = std::shared_ptr<mGroupWidget>(this->addGroup("草图", "退出"));

        sketchCreateGW->m_R_gridLayout_->setSpacing(0);
        //sketchConstraintGW->m_R_gridLayout_->setSpacing(0);
        sketchExitGW->m_R_gridLayout_->setSpacing(0);

        _geometrySketchGW.push_back(sketchCreateGW);
        //_geometrySketchGW.push_back(sketchConstraintGW);
        _geometrySketchGW.push_back(sketchExitGW);

        if (sketchCreateGW)
        {

            //线
            QToolButton* lineBtn(sketchCreateGW->addToolBtn("线", ":/mMainWidgets/Icon/caotu/xian.png", -1, -1, 1, 1, QSize(30, 30)));
            lineBtn->setMinimumWidth(45);  // 设置最小宽度
            lineBtn->setMinimumHeight(50);  // 设置最小高度
            lineBtn->installEventFilter(new TooltipInstaller("线",
                QPixmap(":/mMainWidgets/Icon/caotu/xian.png"),
                "拖动以绘制直线。或者单击以绘制多段线，双击以结束绘制。\n",
                200, lineBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(lineBtn, mxr::Sketch2DGeoType::LINE);

            //圆
            QToolButton* circleBtn(sketchCreateGW->addToolBtn("圆", ":/mMainWidgets/Icon/caotu/yuan.png", -1, -1, 1, 1, QSize(30, 30)));
            circleBtn->setMinimumWidth(45);  // 设置最小宽度
            circleBtn->setMinimumHeight(50);  // 设置最小高度
            circleBtn->installEventFilter(new TooltipInstaller("圆",
                QPixmap(":/mMainWidgets/Icon/caotu/yuan.png"),
                "拖动以创建圆。或者单击以选择圆心，然后再单击以确认半径。\n",
                200, circleBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(circleBtn, mxr::Sketch2DGeoType::CIRCLE);

            //矩形
            QToolButton* rectangleBtn(sketchCreateGW->addToolBtn("矩形", ":/mMainWidgets/Icon/caotu/juxing.png", -1, -1, 1, 1, QSize(30, 30)));
            rectangleBtn->setMinimumWidth(45);  // 设置最小宽度
            rectangleBtn->setMinimumHeight(50);  // 设置最小高度
            rectangleBtn->installEventFilter(new TooltipInstaller("矩形",
                QPixmap(":/mMainWidgets/Icon/caotu/juxing.png"),
                "拖动或单击以绘制矩形。\n",
                200, rectangleBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(rectangleBtn, mxr::Sketch2DGeoType::RECTANGLE);

			//三点圆
			QToolButton* circle3PBtn = sketchCreateGW->addToolBtn("三点圆", ":/mMainWidgets/Icon/caotu/sandianyuan.png", 0, 3, 1, 1, QSize(20, 20));
			circle3PBtn->setFixedHeight(UiMetrics::menuItemHeight());
			circle3PBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
			circle3PBtn->setMinimumWidth(80);  // 设置最小宽度
			circle3PBtn->installEventFilter(new TooltipInstaller("三点圆",
				QPixmap(":/mMainWidgets/Icon/caotu/sandianyuan.png"),
				"单击三个点可创建这些点所确定的圆。",
				200, circle3PBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(circle3PBtn, mxr::Sketch2DGeoType::THREEPOINTCIRCLE);

			//椭圆
			QToolButton* ellipseBtn = sketchCreateGW->addToolBtn("椭圆", ":/mMainWidgets/Icon/caotu/touyuan.png", 0, 4, 1, 1, QSize(20, 20));
			ellipseBtn->setFixedHeight(UiMetrics::menuItemHeight());
			ellipseBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
			ellipseBtn->setMinimumWidth(60);  // 设置最小宽度
			ellipseBtn->installEventFilter(new TooltipInstaller("椭圆",
				QPixmap(":/mMainWidgets/Icon/caotu/touyuan.png"),
				"单击以设置椭圆中心，再单击以设置第一个轴的长度和方位，\n"
				"然后单击以设置第二个轴的长度。",
				200, ellipseBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(ellipseBtn, mxr::Sketch2DGeoType::ELLIPSE);

			//点
			QToolButton* pointBtn = sketchCreateGW->addToolBtn("点", ":/mMainWidgets/Icon/caotu/dian.png", 0, 5, 1, 1, QSize(20, 20));
			pointBtn->setFixedHeight(UiMetrics::menuItemHeight());
			pointBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
			pointBtn->setMinimumWidth(80);  // 设置最小宽度
			pointBtn->installEventFilter(new TooltipInstaller("点",
				QPixmap(":/mMainWidgets/Icon/caotu/dian.png"),
				"单击以创建一个点。",
				200, pointBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(pointBtn, mxr::Sketch2DGeoType::POINT);


            //扫掠弧
			QToolButton*sweepArcBtn = sketchCreateGW->addToolBtn("扫掠弧", ":/mMainWidgets/Icon/caotu/saoluehu.png", 1, 3, 1, 1, QSize(20, 20));
			sweepArcBtn->setFixedHeight(UiMetrics::menuItemHeight());
			sweepArcBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
			sweepArcBtn->setMinimumWidth(80);  // 设置最小宽度
			sweepArcBtn->installEventFilter(new TooltipInstaller("扫掠弧",
				QPixmap(":/mMainWidgets/Icon/caotu/saoluehu.png"),
				"单击以设置弧的圆心，再单击以设置弧的半径和起点，\n。"
				"然后单击以设置弧的终点。",
				200, sweepArcBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(sweepArcBtn, mxr::Sketch2DGeoType::SWEEPARC);

			//三点弧
			QToolButton* Arc3PBtn = sketchCreateGW->addToolBtn("三点弧", ":/mMainWidgets/Icon/caotu/sandianhu.png", 1, 4, 1, 1, QSize(20, 20));
			Arc3PBtn->setFixedHeight(UiMetrics::menuItemHeight());
			Arc3PBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
			Arc3PBtn->setMinimumWidth(80);  // 设置最小宽度
			Arc3PBtn->installEventFilter(new TooltipInstaller("三点弧",
				QPixmap(":/mMainWidgets/Icon/caotu/sandianhu.png"),
				"单击确定起点，再单击确定终点，然后单击确定半径。",
				200, Arc3PBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(Arc3PBtn, mxr::Sketch2DGeoType::THREEPOINTARC);

			//椭圆弧
			QToolButton* ellipseArcBtn = sketchCreateGW->addToolBtn("椭圆弧", ":/mMainWidgets/Icon/caotu/touyuanhu.png", 1, 5, 1, 1, QSize(20, 20));
			ellipseArcBtn->setFixedHeight(UiMetrics::menuItemHeight());
			ellipseArcBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
			ellipseArcBtn->setMinimumWidth(80);  // 设置最小宽度
			ellipseArcBtn->installEventFilter(new TooltipInstaller("椭圆弧",
				QPixmap(":/mMainWidgets/Icon/caotu/touyuanhu.png"),
				"",
				200, ellipseArcBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(ellipseArcBtn, mxr::Sketch2DGeoType::ELLIPSEARC);

			//样条曲线
			QToolButton* splineBtn = sketchCreateGW->addToolBtn("样条曲线", ":/mMainWidgets/Icon/caotu/yangtiaoquxian.png", 0, 6, 1, 1, QSize(20, 20));
			splineBtn->setFixedHeight(UiMetrics::menuItemHeight());
			splineBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
			splineBtn->setMinimumWidth(80);  // 设置最小宽度
			splineBtn->installEventFilter(new TooltipInstaller("样条曲线",
				QPixmap(":/mMainWidgets/Icon/caotu/yangtiaoquxian.png"),
				"单击确定样条曲线的各点，双击可结束样条曲线。",
				200, splineBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(splineBtn, mxr::Sketch2DGeoType::SPLINE);

			//平行四边形
			QToolButton* paraBtn = sketchCreateGW->addToolBtn("平行四边形", ":/mMainWidgets/Icon/caotu/pingxingsibianxing.png", 1, 6, 1, 1, QSize(20, 20));
			paraBtn->setFixedHeight(UiMetrics::menuItemHeight());
			paraBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
			paraBtn->setMinimumWidth(85);  // 设置最小宽度
			paraBtn->installEventFilter(new TooltipInstaller("平行四边形",
				QPixmap(":/mMainWidgets/Icon/caotu/pingxingsibianxing.png"),
				"",
				200, paraBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(paraBtn, mxr::Sketch2DGeoType::PARALLELOGRAM);

			//正多边形
			QToolButton* polygonBtn = sketchCreateGW->addToolBtn("正多边形", ":/mMainWidgets/Icon/caotu/zhengduobianxing.png", 0, 7, 1, 1, QSize(20, 20));
			polygonBtn->setFixedHeight(UiMetrics::menuItemHeight());
			polygonBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
			polygonBtn->setMinimumWidth(80);  // 设置最小宽度
			polygonBtn->installEventFilter(new TooltipInstaller("正多边形",
				QPixmap(":/mMainWidgets/Icon/caotu/zhengduobianxing.png"),
				"单击确定多边形的中点，然后单击确定其直径和方位。",
				200, polygonBtn));
            mGeoSketchSignalManager::instance()->connectSketchGeoButton(polygonBtn, mxr::Sketch2DGeoType::POLYGON);
        }

        //if (sketchConstraintGW)
        //{
        //	//尺寸约束
        //	QToolButton* dimensionBtn(sketchConstraintGW->addToolBtn("尺寸", ":/mMainWidgets/Icon/caotu/chicun.png", -1, -1, 1, 1, QSize(30, 30)));
        //	dimensionBtn->setMinimumWidth(45);  // 设置最小宽度
        //	dimensionBtn->setMinimumHeight(50);  // 设置最小高度
        //	dimensionBtn->installEventFilter(new TooltipInstaller("尺寸",
        //		QPixmap(":/mMainWidgets/Icon/caotu/chicun.png"),
        //		"创建尺寸约束。\n",
        //		200, dimensionBtn));

        //	//点重合
        //	QToolButton* pointOverlapBtn = sketchConstraintGW->addToolBtn("点重合", ":/mMainWidgets/Icon/caotu/dianchonghe.png", 0, 3, 1, 1, QSize(20, 20));
        //	pointOverlapBtn->setFixedHeight(20);
        //	pointOverlapBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	pointOverlapBtn->setMinimumWidth(70);  // 设置最小宽度
        //	pointOverlapBtn->installEventFilter(new TooltipInstaller("点重合",
        //		QPixmap(":/mMainWidgets/Icon/caotu/dianchonghe.png"),
        //		"创建点重合约束。",
        //		200, pointOverlapBtn));

        //	//点线重合
        //	QToolButton* pointOnlineBtn = sketchConstraintGW->addToolBtn("点线重合", ":/mMainWidgets/Icon/caotu/dianxianchonghe.png", 0, 4, 1, 1, QSize(20, 20));
        //	pointOnlineBtn->setFixedHeight(20);
        //	pointOnlineBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	pointOnlineBtn->setMinimumWidth(80);  // 设置最小宽度
        //	pointOnlineBtn->installEventFilter(new TooltipInstaller("点线重合",
        //		QPixmap(":/mMainWidgets/Icon/caotu/dianxianchonghe.png"),
        //		"创建点线重合约束。",
        //		200, pointOnlineBtn));

        //	//竖直约束
        //	QToolButton* verticalBtn = sketchConstraintGW->addToolBtn("竖直", ":/mMainWidgets/Icon/caotu/shuzhi.png", 0, 5, 1, 1, QSize(20, 20));
        //	verticalBtn->setFixedHeight(20);
        //	verticalBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	verticalBtn->setMinimumWidth(60);  // 设置最小宽度
        //	verticalBtn->installEventFilter(new TooltipInstaller("竖直",
        //		QPixmap(":/mMainWidgets/Icon/caotu/shuzhi.png"),
        //		"创建竖直约束。",
        //		200, verticalBtn));

        //	//水平约束
        //	QToolButton* horizontalBtn = sketchConstraintGW->addToolBtn("水平", ":/mMainWidgets/Icon/caotu/shuiping.png", 0, 6, 1, 1, QSize(20, 20));
        //	horizontalBtn->setFixedHeight(20);
        //	horizontalBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	horizontalBtn->setMinimumWidth(60);  // 设置最小宽度
        //	horizontalBtn->installEventFilter(new TooltipInstaller("水平",
        //		QPixmap(":/mMainWidgets/Icon/caotu/shuiping.png"),
        //		"创建水平约束。",
        //		200, horizontalBtn));

        //	//垂直约束
        //	QToolButton* perpendicularBtn = sketchConstraintGW->addToolBtn("垂直", ":/mMainWidgets/Icon/caotu/chuizhi.png", 1, 3, 1, 1, QSize(20, 20));
        //	perpendicularBtn->setFixedHeight(20);
        //	perpendicularBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	perpendicularBtn->setMinimumWidth(60);  // 设置最小宽度
        //	perpendicularBtn->installEventFilter(new TooltipInstaller("垂直",
        //		QPixmap(":/mMainWidgets/Icon/caotu/chuizhi.png"),
        //		"创建垂直约束。",
        //		200, perpendicularBtn));

        //	//平行约束
        //	QToolButton* parallelBtn = sketchConstraintGW->addToolBtn("平行", ":/mMainWidgets/Icon/caotu/pingxing.png", 1, 4, 1, 1, QSize(20, 20));
        //	parallelBtn->setFixedHeight(20);
        //	parallelBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	parallelBtn->setMinimumWidth(60);  // 设置最小宽度
        //	parallelBtn->installEventFilter(new TooltipInstaller("平行",
        //		QPixmap(":/mMainWidgets/Icon/caotu/pingxing.png"),
        //		"创建平行约束。",
        //		200, parallelBtn));

        //	//等长约束
        //	QToolButton* equilongBtn = sketchConstraintGW->addToolBtn("等长", ":/mMainWidgets/Icon/caotu/dengchang.png", 1, 5, 1, 1, QSize(20, 20));
        //	equilongBtn->setFixedHeight(20);
        //	equilongBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	equilongBtn->setMinimumWidth(60);  // 设置最小宽度
        //	equilongBtn->installEventFilter(new TooltipInstaller("等长",
        //		QPixmap(":/mMainWidgets/Icon/caotu/dengchang.png"),
        //		"创建等长约束。",
        //		200, equilongBtn));

        //	//等半径约束
        //	QToolButton* equalRadiusBtn = sketchConstraintGW->addToolBtn("等半径", ":/mMainWidgets/Icon/caotu/dengbanjin.png", 1, 6, 1, 1, QSize(20, 20));
        //	equalRadiusBtn->setFixedHeight(20);
        //	equalRadiusBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	equalRadiusBtn->setMinimumWidth(70);  // 设置最小宽度
        //	equalRadiusBtn->installEventFilter(new TooltipInstaller("等半径",
        //		QPixmap(":/mMainWidgets/Icon/caotu/dengbanjin.png"),
        //		"创建等半径约束。",
        //		200, equalRadiusBtn));

        //	//（椭圆 ）长短径相等
        //	QToolButton* equalEllipseBtn = sketchConstraintGW->addToolBtn("等长短径(椭圆)", ":/mMainWidgets/Icon/caotu/dengchangduanjing.png", 1, 8, 1, 1, QSize(20, 20));
        //	equalEllipseBtn->setFixedHeight(20);
        //	equalEllipseBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	equalEllipseBtn->setMinimumWidth(110);  // 设置最小宽度
        //	equalEllipseBtn->installEventFilter(new TooltipInstaller("等长短径(椭圆)",
        //		QPixmap(":/mMainWidgets/Icon/caotu/dengchangduanjing.png"),
        //		"创建等长短径(椭圆)约束。",
        //		200, equalEllipseBtn));

        //	//相切约束
        //	QToolButton* tangencyBtn = sketchConstraintGW->addToolBtn("相切", ":/mMainWidgets/Icon/caotu/xiangqie.png", 1, 7, 1, 1, QSize(20, 20));
        //	tangencyBtn->setFixedHeight(20);
        //	tangencyBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	tangencyBtn->setMinimumWidth(60);  // 设置最小宽度
        //	tangencyBtn->installEventFilter(new TooltipInstaller("相切",
        //		QPixmap(":/mMainWidgets/Icon/caotu/xiangqie.png"),
        //		"创建相切约束。",
        //		200, tangencyBtn));

        //	//距离约束
        //	QToolButton* distanceBtn = sketchConstraintGW->addToolBtn("距离", ":/mMainWidgets/Icon/caotu/juli.png", 0, 7, 1, 1, QSize(20, 20));
        //	distanceBtn->setFixedHeight(20);
        //	distanceBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	distanceBtn->setMinimumWidth(60);  // 设置最小宽度
        //	distanceBtn->installEventFilter(new TooltipInstaller("距离",
        //		QPixmap(":/mMainWidgets/Icon/caotu/juli.png"),
        //		"创建距离约束。",
        //		200, distanceBtn));

        //	//固定约束
        //	QToolButton* fixedBtn = sketchConstraintGW->addToolBtn("固定", ":/mMainWidgets/Icon/caotu/guding.png", 0, 8, 1, 1, QSize(20, 20));
        //	fixedBtn->setFixedHeight(20);
        //	fixedBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
        //	fixedBtn->setMinimumWidth(60);  // 设置最小宽度
        //	fixedBtn->installEventFilter(new TooltipInstaller("固定",
        //		QPixmap(":/mMainWidgets/Icon/caotu/guding.png"),
        //		"创建固定约束。",
        //		200, fixedBtn));
        //}

		if (sketchExitGW)
		{
			//结束草绘
			QToolButton* exitBtn(sketchExitGW->addToolBtn("结束草图绘制", ":/mMainWidgets/Icon/caotu/jieshucaotu.png", -1, -1, 1, 1, QSize(30, 30)));
			exitBtn->setMinimumWidth(80);  // 设置最小宽度
			exitBtn->setMinimumHeight(50);  // 设置最小高度
			exitBtn->installEventFilter(new TooltipInstaller("结束草图绘制",
				QPixmap(":/mMainWidgets/Icon/caotu/jieshucaotu.png"),
				"退出草图绘制模式。\n",
				200, exitBtn));
			connect(exitBtn, &QToolButton::clicked, this, [this]() {
				mInfoWindow::getInstance()->unEditInfoWindow();
				const int modelIdx = findTabByText("模型");
				if (modelIdx >= 0)
					m_menuTabBar_->setCurrentIndex(modelIdx);
                mGeoSketchSignalManager::instance()->exitSketch();
                MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("几何");
                });
        }

    }

    void mMenuWidget::initMeshMenu()
    {
        // 下方Ribbon工具条上添加相应的显示widget作为Group的载体
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("网格", tempWid);

        mGroupWidget* meshGW1 = this->addGroup("网格", "网格");
        mGroupWidget* meshGW2 = this->addGroup("网格", "网格控制");
        mGroupWidget* meshGW4 = this->addGroup("网格", "网格创建");
        mGroupWidget* meshGW3 = this->addGroup("网格", "网格变换");
        mGroupWidget* meshGW5 = this->addGroup("网格", "网格编辑");
        mGroupWidget* meshGW6 = this->addGroup("网格", "网格检查");

        mGroupWidget* meshGW7 = this->addGroup("网格", "质量检查");
        mGroupWidget* meshGW8 = this->addGroup("网格", "网格统计");

        //第一次点击网格的时候将初始化此容器，后面撤回的时候将该容器给几何高亮，确定剖分后清空该容器

        QString SolidID;
        if (meshGW1)
        {
            QToolButton* createbtn = meshGW1->addToolBtn("一键生成", ":/mMainWidgets/Icon/Mesh/shengcheng", -1, -1, 1, 1, QSize(30, 30), 10);
            setQToolButton(createbtn, "一键生成", ":/mMainWidgets/Icon/Mesh/shengcheng.png", "对当前模型中的全部部件一键生成网格。");

            QToolButton* deletebtn = meshGW1->addToolBtn("一键删除", ":/mMainWidgets/Icon/Mesh/shanchu", -1, -1, 1, 1, QSize(30, 30), 10);
            setQToolButton(deletebtn, "一键删除", ":/mMainWidgets/Icon/Mesh/shanchu.png", "删除当前模型中已生成的网格。");
            mInfoWindow::getInstance()->connectEnable(createbtn, deletebtn);
            connect(createbtn, &QToolButton::clicked, this, [=]() {
                //一键生成					
                mInfoWindow::getInstance()->createMeshAuto();
                createbtn->setEnabled(false);
                deletebtn->setEnabled(false);
                });
            connect(deletebtn, &QToolButton::clicked, this, [=]()
                {
                    //一键删除					
                    mInfoWindow::getInstance()->deleteMeshAuto();

                });

            QToolButton* btn1 = meshGW1->addToolBtn("线网格", ":/mMainWidgets/Icon/Mesh/xian", 0, 1, 1, 1, QSize(20, 20), 10);
			setQToolButton(btn1, "线网格", ":/mMainWidgets/Icon/Mesh/xian.png", "生成线网格。",Qt::ToolButtonTextBesideIcon);
            connect(btn1, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshPanel(2);
                });
            //QToolButton* outGeoFileBtn = tGW->addToolBtn("导出几何模型", ":/mMainWidgets/Icon/homepage/daochu3x", 0, 2, 1, 1, QSize(16, 16));
            //outGeoFileBtn->setFixedHeight(26);
            //outGeoFileBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // 关键：文字在图标右边
            //outGeoFileBtn->setObjectName("wideButton");
            //outGeoFileBtn->setStyleSheet(toolButtonStyle);
            //outGeoFileBtn->setMinimumWidth(100);
            //connect(outGeoFileBtn, &QToolButton::clicked, this, &mMenuWidget::slot_exportGeoFile);

            QToolButton* btn2 = meshGW1->addToolBtn("面网格", ":/mMainWidgets/Icon/Mesh/mian", 1, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn2, "面网格", ":/mMainWidgets/Icon/Mesh/mian.png", "生成面网格。", Qt::ToolButtonTextBesideIcon);
            connect(btn2, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshPanel(FaceMesh);
                });

            QToolButton* btn3 = meshGW1->addToolBtn("体网格", ":/mMainWidgets/Icon/Mesh/ti", 0, 2, 2, 1, QSize(30, 30), 10);
            setQToolButton(btn3, "体网格", ":/mMainWidgets/Icon/Mesh/ti.png", "生成体网格。");
            connect(btn3, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshPanel(SolidMesh);
                });
        }
        if (meshGW2)
        {
            QToolButton* btn = meshGW2->addToolBtn("全局尺寸", ":/mMainWidgets/Icon/Mesh/quanjuchicun", -1, -1, 1, 1, QSize(30, 30), 10);
            setQToolButton(btn, "全局尺寸", ":/mMainWidgets/Icon/Mesh/quanjuchicun.png", "给指定部件设置全局网格尺寸。");
            btn->setVisible(false);

            connect(btn, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createGlobalDimensions();
                });


            QToolButton* btn0 = meshGW2->addToolBtn("局部尺寸", ":/mMainWidgets/Icon/Mesh/jubuchicun", -1, -1, 1, 1, QSize(30, 30), 10);
            setQToolButton(btn0, "局部尺寸", ":/mMainWidgets/Icon/Mesh/jubuchicun.png", "给指定部件设置局部网格尺寸。");
            connect(btn0, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshPanel(MeshPanelType::LocalSize);
                });


			// 边抑制面板：仅包含几何线拾取模块
			QToolButton* btnSuppress = meshGW2->addToolBtn("边抑制", ":/mMainWidgets/Icon/Mesh/splitEdge", 0, 2, 1, 1, QSize(20, 20), 10);
			setQToolButton(btnSuppress, "边抑制", ":/mMainWidgets/Icon/Mesh/splitEdge.png", "边抑制。", Qt::ToolButtonTextBesideIcon);
			connect(btnSuppress, &QToolButton::clicked, this, [=]()
				{
					mInfoWindow::getInstance()->createMeshPanel(EdgeSuppress);
				});



			QToolButton* btn4 = meshGW2->addToolBtn("网格自适应", ":/mMainWidgets/Icon/Mesh/zishiyin", 0, 1, 1, 1, QSize(20, 20), 10);
			setQToolButton(btn4, "网格自适应", ":/mMainWidgets/Icon/Mesh/zishiyin.png", "网格自适应。", Qt::ToolButtonTextBesideIcon);
			connect(btn4, &QToolButton::clicked, this, [=]()
				{
					mInfoWindow::getInstance()->createMeshPanel(AutoAdapt);
				});

            QToolButton* btn5 = meshGW2->addToolBtn("自定义加密", ":/mMainWidgets/Icon/Mesh/jiami", 1, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn5, "自定义加密", ":/mMainWidgets/Icon/Mesh/jiami.png", "自定义加密。", Qt::ToolButtonTextBesideIcon);
            connect(btn5, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshPanel(CustomEncryption);
                });
        }

        if (meshGW3)
        {
            QToolButton* btn2 = meshGW3->addToolBtn("平移", ":/mMainWidgets/Icon/Mesh/translate", 0, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn2, "平移", ":/mMainWidgets/Icon/Mesh/translate.png", "平移。", Qt::ToolButtonTextBesideIcon);
            connect(btn2, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(Translation);


                });
            QToolButton* btn3 = meshGW3->addToolBtn("旋转", ":/mMainWidgets/Icon/Mesh/xuanzhuan2", 1, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn3, "旋转", ":/mMainWidgets/Icon/Mesh/xuanzhuan2.png", "旋转。", Qt::ToolButtonTextBesideIcon);
            connect(btn3, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(Rotation);

                });
            QToolButton* btn1 = meshGW3->addToolBtn("镜像", ":/mMainWidgets/Icon/Mesh/mirror", 0, 2, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn1, "镜像", ":/mMainWidgets/Icon/Mesh/mirror.png", "镜像。", Qt::ToolButtonTextBesideIcon);
            connect(btn1, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(Mirroring);

                });
            QToolButton* btn = meshGW3->addToolBtn("阵列", ":/mMainWidgets/Icon/Mesh/zhenglie", 1, 2, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn, "阵列", ":/mMainWidgets/Icon/Mesh/zhenglie.png", "阵列。", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(Matrix);

                });
            QToolButton* btn4 = meshGW3->addToolBtn("缩放", ":/mMainWidgets/Icon/Mesh/zoom", 0, 3, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn4, "缩放", ":/mMainWidgets/Icon/Mesh/zoom.png", "缩放。", Qt::ToolButtonTextBesideIcon);
            connect(btn4, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MeshZoom);

                });
        }
        if (meshGW4)
        {
            QToolButton* btn0 = meshGW4->addToolBtn("创建节点", ":/mMainWidgets/Icon/Mesh/createPoint", 0, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn0, "创建节点", ":/mMainWidgets/Icon/Mesh/createPoint.png", "创建节点。", Qt::ToolButtonTextBesideIcon);
            connect(btn0, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(CreatePoint);
                });
            QToolButton* btn1 = meshGW4->addToolBtn("创建网格", ":/mMainWidgets/Icon/Mesh/createMesh", 1, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn1, "创建网格", ":/mMainWidgets/Icon/Mesh/createMesh.png", "创建网格。", Qt::ToolButtonTextBesideIcon);
            connect(btn1, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(CreateMesh);

                });
            QToolButton* btn5 = meshGW4->addToolBtn("旋转网格", ":/mMainWidgets/Icon/Mesh/rotateMesh", 0, 2, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn5, "旋转网格", ":/mMainWidgets/Icon/Mesh/rotateMesh.png", "旋转网格。", Qt::ToolButtonTextBesideIcon);
            connect(btn5, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(RotateMesh);

                });
            QToolButton* btn3 = meshGW4->addToolBtn("提取表面网格", ":/mMainWidgets/Icon/Mesh/extractSuface", 1, 2, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn3, "提取表面网格", ":/mMainWidgets/Icon/Mesh/extractSuface.png", "提取表面网格。", Qt::ToolButtonTextBesideIcon);
            connect(btn3, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(ExtractSurface);

                });
            QToolButton* btn = meshGW4->addToolBtn("拉伸网格", ":/mMainWidgets/Icon/Mesh/strechMesh", 0, 3, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn, "拉伸网格", ":/mMainWidgets/Icon/Mesh/strechMesh.png", "拉伸网格。", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MeshStretch);

                });
            QToolButton* btn2 = meshGW4->addToolBtn("创建元网格", ":/mMainWidgets/Icon/Mesh/createMeta", 1, 3, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn2, "创建元网格", ":/mMainWidgets/Icon/Mesh/createMeta.png", "创建元网格。", Qt::ToolButtonTextBesideIcon);
            btn2->setVisible(false); // 功能未开发，暂不显示
            connect(btn2, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(CreateMetaMesh);

                });
        }
        if (meshGW5)
        {
            QToolButton* btn0 = meshGW5->addToolBtn("编辑单元", ":/mMainWidgets/Icon/Mesh/meshEdit", 0, 0, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn0, "编辑单元", ":/mMainWidgets/Icon/Mesh/meshEdit.png", "编辑单元。", Qt::ToolButtonTextBesideIcon);
            connect(btn0, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MeshEdit);

                });

            QToolButton* btn1 = meshGW5->addToolBtn("重剖分", ":/mMainWidgets/Icon/Mesh/chongpoufen", 1, 5, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn1, "网格重剖分", ":/mMainWidgets/Icon/Mesh/chongpoufen.png", "网格重剖分。", Qt::ToolButtonTextBesideIcon);
            connect(btn1, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(Remeshing);

                });
           
            QToolButton* btn8 = meshGW5->addToolBtn("改变阶数", ":/mMainWidgets/Icon/Mesh/changeOrder", 0, 4, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn8, "改变阶数", ":/mMainWidgets/Icon/Mesh/changeOrder.png", "改变阶数。", Qt::ToolButtonTextBesideIcon);
            connect(btn8, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(ChangeOrder);

                });
            QToolButton* btn9 = meshGW5->addToolBtn("网格光滑", ":/mMainWidgets/Icon/Mesh/meshSmooth", 1, 4, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn9, "网格光滑", ":/mMainWidgets/Icon/Mesh/meshSmooth.png", "网格光滑。", Qt::ToolButtonTextBesideIcon);
            connect(btn9, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MeshSmooth);

                });
            QToolButton* btn10 = meshGW5->addToolBtn("共节点", ":/mMainWidgets/Icon/Mesh/sharedNode", 0, 5, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn10, "共节点", ":/mMainWidgets/Icon/Mesh/sharedNode.png", "共节点。", Qt::ToolButtonTextBesideIcon);
            connect(btn10, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(SharedNode);

                });
            QToolButton* btn11 = meshGW5->addToolBtn("重编号", ":/mMainWidgets/Icon/Mesh/renumber", 0, 6, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn11, "重编号", ":/mMainWidgets/Icon/Mesh/renumber.png", "重编号。", Qt::ToolButtonTextBesideIcon);
            connect(btn11, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(Renumber);

                });
            QToolButton* btn12 = meshGW5->addToolBtn("法向", ":/mMainWidgets/Icon/Mesh/faxiang", 1, 6, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn12, "法向", ":/mMainWidgets/Icon/Mesh/faxiang.png", "法向。", Qt::ToolButtonTextBesideIcon);
            connect(btn12, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MeshNormal);

                });
            QToolButton* btn13 = meshGW5->addToolBtn("移动网格", ":/mMainWidgets/Icon/Mesh/moveMesh", 0, 7, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn13, "移动网格", ":/mMainWidgets/Icon/Mesh/moveMesh.png", "移动网格。", Qt::ToolButtonTextBesideIcon);
            connect(btn13, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MoveMesh);

                });
            QToolButton* btn14 = meshGW5->addToolBtn("STL重剖分", ":/mMainWidgets/Icon/Mesh/stlRemesh", 1, 7, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn14, "STL重剖分", ":/mMainWidgets/Icon/Mesh/stlRemesh.png", "STL重剖分。", Qt::ToolButtonTextBesideIcon);
            connect(btn14, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(STLRemesh);
                });
            QToolButton* btn15 = meshGW5->addToolBtn("网格删除", ":/mMainWidgets/Icon/Mesh/deleteMesh", 1, 0, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn15, "网格删除", ":/mMainWidgets/Icon/Mesh/deleteMesh.png", "网格删除。", Qt::ToolButtonTextBesideIcon);
            connect(btn15, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(DeleteMesh);
                });
        }
        if (meshGW6)
        {
            QToolButton* btn = meshGW6->addToolBtn("自由边", ":/mMainWidgets/Icon/Mesh/freeEdge", 0, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn, "自由边", ":/mMainWidgets/Icon/Mesh/freeEdge.png", "自由边。", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MeshPanelType::MeshFreeEdge);

                });
            QToolButton* btn1 = meshGW6->addToolBtn("T形边", ":/mMainWidgets/Icon/Mesh/TEdge", 1, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn1, "T形边", ":/mMainWidgets/Icon/Mesh/TEdge.png", "T形边。", Qt::ToolButtonTextBesideIcon);
            connect(btn1, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(TEdge);

                });
            QToolButton* btn2 = meshGW6->addToolBtn("相交", ":/mMainWidgets/Icon/Mesh/intersect", 0, 2, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn2, "相交", ":/mMainWidgets/Icon/Mesh/intersect.png", "相交。", Qt::ToolButtonTextBesideIcon);
            connect(btn2, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MeshIntersect);

                });
            QToolButton* btn3 = meshGW6->addToolBtn("重叠", ":/mMainWidgets/Icon/Mesh/overlap", 1, 2, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn3, "重叠", ":/mMainWidgets/Icon/Mesh/overlap.png", "重叠。", Qt::ToolButtonTextBesideIcon);
            connect(btn3, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MeshOverlap);

                });
            QToolButton* btn4 = meshGW6->addToolBtn("干涉", ":/mMainWidgets/Icon/Mesh/interference", 0, 3, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn4, "干涉", ":/mMainWidgets/Icon/Mesh/interference.png", "干涉。", Qt::ToolButtonTextBesideIcon);
            connect(btn4, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MeshInterfere);

                });
        }
        if (meshGW7)
        {
            QToolButton* btn = meshGW7->addToolBtn("质量检查", ":/mMainWidgets/Icon/Mesh/check", 0, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn, "质量检查", ":/mMainWidgets/Icon/Mesh/check.png", "质量检查。", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MeshQuality);

                });
        }
        if (meshGW8)
        {
            QToolButton* btn = meshGW8->addToolBtn("网格统计", ":/mMainWidgets/Icon/Mesh/statics", 0, 1, 1, 1, QSize(20, 20), 10);
            setQToolButton(btn, "网格统计", ":/mMainWidgets/Icon/Mesh/statics.png", "网格统计。", Qt::ToolButtonTextBesideIcon);
            connect(btn, &QToolButton::clicked, this, [=]()
                {
                    mInfoWindow::getInstance()->createMeshOptPanel(MeshStatistics);

                });
        }
    }

    void mMenuWidget::initAnalysisCaseMenu()
    {
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("分析工况", tempWid);

        mGroupWidget* tCaseStatic = this->addGroup("分析工况", "静态分析");
        mGroupWidget* tCaseDynamic = this->addGroup("分析工况", "隐式动力学");
        mGroupWidget* tCaseStable = this->addGroup("分析工况", "稳定性分析");
        mGroupWidget* tCaseHeat = this->addGroup("分析工况", "传热分析");
        mGroupWidget* tCaseLinearDynamic = this->addGroup("分析工况", "线性动态分析");
        mGroupWidget* tCaseWizard = this->addGroup("分析工况", "工况向导");

        // 图标在上、文字在下的大按钮，点击直接创建对应类型的分析工况
        auto addCaseBtn = [this](mGroupWidget* group, const QString& text, const QString& icon,
            MxFunctions::CaseType type) {
                QToolButton* btn = group->addToolBtn(text, icon, -1, -1, 1, 1, QSize(30, 30));
                setQToolButton(btn, text, icon, "创建" + text + "分析。");
                btn->setIconSize(QSize(30, 30));
                connect(btn, &QToolButton::clicked, this, [this, type]() {
                    slot_createCase(type);
                    });
            };

        if (tCaseStatic)
        {
            addCaseBtn(tCaseStatic, "线性静力学", ":/mMainWidgets/Icon/CaseType/linearStatic.svg", MxFunctions::ImplicitLinearStatic);
            addCaseBtn(tCaseStatic, "非线性静力学", ":/mMainWidgets/Icon/CaseType/nonlinearStatic.svg", MxFunctions::ImplicitNonLinearStatic);
        }

        if (tCaseDynamic)
        {
            addCaseBtn(tCaseDynamic, "非线性瞬态", ":/mMainWidgets/Icon/CaseType/nonlinearTransient.svg", MxFunctions::ImplicitNonLinearDynamic);
        }

        if (tCaseStable)
        {
            addCaseBtn(tCaseStable, "线性屈曲", ":/mMainWidgets/Icon/CaseType/linearBuckling.svg", MxFunctions::ImplicitLinearBuckling);
            addCaseBtn(tCaseStable, "后屈曲", ":/mMainWidgets/Icon/CaseType/postBuckling.svg", MxFunctions::ImplicitPostBuckling);
        }

        if (tCaseHeat)
        {
            addCaseBtn(tCaseHeat, "稳态传热", ":/mMainWidgets/Icon/CaseType/steadyHeat.svg", MxFunctions::ImplicitSteadyHeat);
            addCaseBtn(tCaseHeat, "瞬态传热", ":/mMainWidgets/Icon/CaseType/transientHeat.svg", MxFunctions::ImplicitTransientHeat);
            addCaseBtn(tCaseHeat, "结构耦合传热", ":/mMainWidgets/Icon/CaseType/structuralCoupledHeat.svg", MxFunctions::ImplicitStructuralCouplingHeat);
        }

        if (tCaseLinearDynamic)
        {
            addCaseBtn(tCaseLinearDynamic, "模态", ":/mMainWidgets/Icon/CaseType/modal.svg", MxFunctions::ImplicitModal);
            addCaseBtn(tCaseLinearDynamic, "响应谱", ":/mMainWidgets/Icon/CaseType/responseSpectrum.svg", MxFunctions::ImplicitResponseSpectrum);
            addCaseBtn(tCaseLinearDynamic, "随机振动", ":/mMainWidgets/Icon/CaseType/randomVibration.svg", MxFunctions::ImplicitRandomVibration);
            addCaseBtn(tCaseLinearDynamic, "频响", ":/mMainWidgets/Icon/CaseType/frequencyResponse.svg", MxFunctions::ImplicitFrequency);
            addCaseBtn(tCaseLinearDynamic, "线性瞬态", ":/mMainWidgets/Icon/CaseType/linearTransient.svg", MxFunctions::ImplicitTransient);
            addCaseBtn(tCaseLinearDynamic, "声固耦合", ":/mMainWidgets/Icon/CaseType/acousticCoupling.svg", MxFunctions::ASCoupling);
            addCaseBtn(tCaseLinearDynamic, "超单元", ":/mMainWidgets/Icon/CaseType/superElement.svg", MxFunctions::ImplicitGeneric);
        }

        if (tCaseWizard)
        {
            // NVH工况由专用向导窗口创建，点击跳转到“NVH”页签
            QToolButton* nvhBtn = tCaseWizard->addToolBtn("NVH分析", ":/mMainWidgets/Icon/CaseType/nvhAnalysis.svg", -1, -1, 1, 1, QSize(30, 30));
            setQToolButton(nvhBtn, "NVH分析", ":/mMainWidgets/Icon/CaseType/nvhAnalysis.svg", "打开NVH分析工况向导。");
            nvhBtn->setIconSize(QSize(30, 30));
            connect(nvhBtn, &QToolButton::clicked, this, [this]() {
                const int nvhIdx = findTabByText("NVH");
                if (nvhIdx >= 0 && m_menuTabBar_->currentIndex() != nvhIdx)
                    m_menuTabBar_->setCurrentIndex(nvhIdx);
                });

            // 时序热分析工况向导（四步向导，批量创建时序工况）
            QToolButton* timeSeriesBtn = tCaseWizard->addToolBtn("时序热分析", ":/mMainWidgets/Icon/CaseType/shixure.svg", -1, -1, 1, 1, QSize(30, 30));
            setQToolButton(timeSeriesBtn, "时序热分析", ":/mMainWidgets/Icon/CaseType/shixure.svg", "打开时序热分析工况向导。");
            timeSeriesBtn->setIconSize(QSize(30, 30));
            connect(timeSeriesBtn, &QToolButton::clicked, this, [this]() {
                // 已存在（含最小化）则直接显示已有窗口；已关闭则恢复默认状态后复用
                if (m_timeSeriesHeatWizard)
                {
                    if (m_timeSeriesHeatWizard->isVisible())
                    {
                        if (m_timeSeriesHeatWizard->isMinimized())
                            m_timeSeriesHeatWizard->showNormal();
                        m_timeSeriesHeatWizard->raise();
                        m_timeSeriesHeatWizard->activateWindow();
                        return;
                    }
                    m_timeSeriesHeatWizard->resetToDefault();
                    m_timeSeriesHeatWizard->show();
                    m_timeSeriesHeatWizard->raise();
                    m_timeSeriesHeatWizard->activateWindow();
                    return;
                }
                m_timeSeriesHeatWizard = new mCaseTimeSeriesHeatWizard(this);
                m_timeSeriesHeatWizard->show();
                });
        }
    }

    void mMenuWidget::initNVHAnalysisMenu()
    {
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("NVH", tempWid);

        auto tUtil = std::shared_ptr<mGroupWidget>(this->addGroup("NVH", "实用工具"));
        _nvhAnalysisGW.append(tUtil);

        if (tUtil)
        {
            QToolButton* modalBtn = tUtil->addToolBtn("模态分析", ":/mMainWidgets/Icon/NVH/motaifenxi.png", 0, 0, 1, 1, QSize(30, 30));
            setQToolButton(modalBtn, "模态分析", ":/mMainWidgets/Icon/NVH/motaifenxi.png", "创建模态分析");
            connect(modalBtn, &QToolButton::clicked, this, [=]() {
                slot_createNVHModelAnalysis();
                });
            QToolButton* superElementBtn = tUtil->addToolBtn("超单元分析", ":/mMainWidgets/Icon/NVH/chaodanyuanfenxi.png", 0, 1, 1, 1, QSize(30, 30));
            setQToolButton(superElementBtn, "超单元分析", ":/mMainWidgets/Icon/NVH/chaodanyuanfenxi.png", "创建超单元分析");
            connect(superElementBtn, &QToolButton::clicked, this, [=]() {
                slot_createNVHSuperElementAnalysis();
                });
            QToolButton* stiffnessBtn = tUtil->addToolBtn("动刚度分析", ":/mMainWidgets/Icon/NVH/donggangdufenxi.png", 0, 2, 1, 1, QSize(30, 30));
            setQToolButton(stiffnessBtn, "动刚度分析", ":/mMainWidgets/Icon/NVH/donggangdufenxi.png", "创建动刚度分析");
            connect(stiffnessBtn, &QToolButton::clicked, this, [=]() {
                slot_createNVHStiffnessAnalysis();
                });
            QToolButton* VTFBtn = tUtil->addToolBtn("VTF分析", ":/mMainWidgets/Icon/NVH/VTFfenxi.png", 0, 3, 1, 1, QSize(30, 30));
            setQToolButton(VTFBtn, "VTF分析", ":/mMainWidgets/Icon/NVH/VTFfenxi.png", "创建VTF分析");
            connect(VTFBtn, &QToolButton::clicked, this, [=]() {
                slot_createNVHVTFAnalysis();
                });
            QToolButton* NTFBtn = tUtil->addToolBtn("NTF分析", ":/mMainWidgets/Icon/NVH/NTFfenxi.png", 0, 4, 1, 1, QSize(30, 30));
            setQToolButton(NTFBtn, "NTF分析", ":/mMainWidgets/Icon/NVH/NTFfenxi.png", "创建NTF分析");
            connect(NTFBtn, &QToolButton::clicked, this, [=]() {
                slot_createNVHNTFAnalysis();
                });
            QToolButton* frequencyResponseBtn = tUtil->addToolBtn("通用频响分析", ":/mMainWidgets/Icon/NVH/tongyongpinxiangfenxi.png", 0, 5, 1, 1, QSize(30, 30));
            setQToolButton(frequencyResponseBtn, "通用频响分析", ":/mMainWidgets/Icon/NVH/tongyongpinxiangfenxi.png", "创建通用频响分析");
            connect(frequencyResponseBtn, &QToolButton::clicked, this, [=]() {
                slot_createNVHFreqResponseAnalysis();
                });
            QToolButton* transientBtn = tUtil->addToolBtn("瞬态分析", ":/mMainWidgets/Icon/NVH/shuntaifenxi.png", 0, 6, 1, 1, QSize(30, 30));
            setQToolButton(transientBtn, "瞬态分析", ":/mMainWidgets/Icon/NVH/shuntaifenxi.png", "创建瞬态分析");
            connect(transientBtn, &QToolButton::clicked, this, [=]() {
                slot_createNVHTransientAnalysis();
                });
            QToolButton* randomVibraBtn = tUtil->addToolBtn("随机振动分析", ":/mMainWidgets/Icon/NVH/suijizhendongfenxi.png", 0, 7, 1, 1, QSize(30, 30));
            setQToolButton(randomVibraBtn, "随机振动分析", ":/mMainWidgets/Icon/NVH/suijizhendongfenxi.png", "创建随机振动分析");
            connect(randomVibraBtn, &QToolButton::clicked, this, [=]() {
                slot_createNVHRandomVibrationAnalysis();
                });
        }

    }

    void mMenuWidget::slot_createNVHModelAnalysis()
    {
        // 模态分析
        // 已存在（含最小化）则直接显示已有窗口，不再重复创建；已关闭则重建
        if (m_caseImplicitModalWindow)
        {
            if (m_caseImplicitModalWindow->isVisible())
            {
                if (m_caseImplicitModalWindow->isMinimized())
                {
                    m_caseImplicitModalWindow->showNormal();
                }
                m_caseImplicitModalWindow->raise();
                m_caseImplicitModalWindow->activateWindow();
                return;
            }
            // 已关闭：恢复默认状态后重新显示，复用实例避免整窗重建卡顿
            m_caseImplicitModalWindow->resetToDefault();
            m_caseImplicitModalWindow->show();
            m_caseImplicitModalWindow->raise();
            m_caseImplicitModalWindow->activateWindow();
            return;
        }
        m_caseImplicitModalWindow = new mCaseImplicitModalWindow(this);
    }

    void mMenuWidget::slot_createNVHStiffnessAnalysis()
    {
        // 动刚度分析
        // 已存在（含最小化）则直接显示已有窗口，不再重复创建；已关闭则重建
        if (m_caseImplicitStiffnessWindow)
        {
            if (m_caseImplicitStiffnessWindow->isVisible())
            {
                if (m_caseImplicitStiffnessWindow->isMinimized())
                {
                    m_caseImplicitStiffnessWindow->showNormal();
                }
                m_caseImplicitStiffnessWindow->raise();
                m_caseImplicitStiffnessWindow->activateWindow();
                return;
            }
            // 已关闭：恢复默认状态后重新显示，复用实例避免整窗重建卡顿
            m_caseImplicitStiffnessWindow->resetToDefault();
            m_caseImplicitStiffnessWindow->show();
            m_caseImplicitStiffnessWindow->raise();
            m_caseImplicitStiffnessWindow->activateWindow();
            return;
        }
        m_caseImplicitStiffnessWindow = new mCaseImplicitStiffnessWindow(this);
    }

    void mMenuWidget::slot_createNVHVTFAnalysis()
    {
        // VTF分析
        // 已存在（含最小化）则直接显示已有窗口，不再重复创建；已关闭则重建
        if (m_caseImplicitVTFWindow)
        {
            if (m_caseImplicitVTFWindow->isVisible())
            {
                if (m_caseImplicitVTFWindow->isMinimized())
                {
                    m_caseImplicitVTFWindow->showNormal();
                }
                m_caseImplicitVTFWindow->raise();
                m_caseImplicitVTFWindow->activateWindow();
                return;
            }
            // 已关闭：恢复默认状态后重新显示，复用实例避免整窗重建卡顿
            m_caseImplicitVTFWindow->resetToDefault();
            m_caseImplicitVTFWindow->show();
            m_caseImplicitVTFWindow->raise();
            m_caseImplicitVTFWindow->activateWindow();
            return;
        }
        m_caseImplicitVTFWindow = new mCaseImplicitVTFWindow(this);
    }

    void mMenuWidget::slot_createNVHSuperElementAnalysis()
    {
        // 超单元分析
        // 已存在（含最小化）则直接显示已有窗口，不再重复创建；已关闭则重建
        if (m_caseImplicitSuperElementWindow)
        {
            if (m_caseImplicitSuperElementWindow->isVisible())
            {
                if (m_caseImplicitSuperElementWindow->isMinimized())
                {
                    m_caseImplicitSuperElementWindow->showNormal();
                }
                m_caseImplicitSuperElementWindow->raise();
                m_caseImplicitSuperElementWindow->activateWindow();
                return;
            }
            // 已关闭：恢复默认状态后重新显示，复用实例避免整窗重建卡顿
            m_caseImplicitSuperElementWindow->resetToDefault();
            m_caseImplicitSuperElementWindow->show();
            m_caseImplicitSuperElementWindow->raise();
            m_caseImplicitSuperElementWindow->activateWindow();
            return;
        }
        m_caseImplicitSuperElementWindow = new mCaseImplicitSuperElementWindow(this);
    }

    void mMenuWidget::slot_createNVHNTFAnalysis()
    {
        // NTF分析
        // 已存在（含最小化）则直接显示已有窗口，不再重复创建；已关闭则重建
        if (m_caseImplicitNTFWindow)
        {
            if (m_caseImplicitNTFWindow->isVisible())
            {
                if (m_caseImplicitNTFWindow->isMinimized())
                {
                    m_caseImplicitNTFWindow->showNormal();
                }
                m_caseImplicitNTFWindow->raise();
                m_caseImplicitNTFWindow->activateWindow();
                return;
            }
            // 已关闭：恢复默认状态后重新显示，复用实例避免整窗重建卡顿
            m_caseImplicitNTFWindow->resetToDefault();
            m_caseImplicitNTFWindow->show();
            m_caseImplicitNTFWindow->raise();
            m_caseImplicitNTFWindow->activateWindow();
            return;
        }
        m_caseImplicitNTFWindow = new mCaseImplicitNTFWindow(this);
    }

    void mMenuWidget::slot_createNVHFreqResponseAnalysis()
    {
        // 通用频响分析
        // 已存在（含最小化）则直接显示已有窗口，不再重复创建；已关闭则重建
        if (m_caseImplicitFreqResponseWindow)
        {
            if (m_caseImplicitFreqResponseWindow->isVisible())
            {
                if (m_caseImplicitFreqResponseWindow->isMinimized())
                {
                    m_caseImplicitFreqResponseWindow->showNormal();
                }
                m_caseImplicitFreqResponseWindow->raise();
                m_caseImplicitFreqResponseWindow->activateWindow();
                return;
            }
            // 已关闭：恢复默认状态后重新显示，复用实例避免整窗重建卡顿
            m_caseImplicitFreqResponseWindow->resetToDefault();
            m_caseImplicitFreqResponseWindow->show();
            m_caseImplicitFreqResponseWindow->raise();
            m_caseImplicitFreqResponseWindow->activateWindow();
            return;
        }
        m_caseImplicitFreqResponseWindow = new mCaseImplicitFrequencyResponseWindow(this);
    }

    void mMenuWidget::slot_createNVHTransientAnalysis()
    {
        // 瞬态分析
        // 已存在（含最小化）则直接显示已有窗口，不再重复创建；已关闭则重建
        if (m_caseImplicitTransientWindow)
        {
            if (m_caseImplicitTransientWindow->isVisible())
            {
                if (m_caseImplicitTransientWindow->isMinimized())
                {
                    m_caseImplicitTransientWindow->showNormal();
                }
                m_caseImplicitTransientWindow->raise();
                m_caseImplicitTransientWindow->activateWindow();
                return;
            }
            // 已关闭：恢复默认状态后重新显示，复用实例避免整窗重建卡顿
            m_caseImplicitTransientWindow->resetToDefault();
            m_caseImplicitTransientWindow->show();
            m_caseImplicitTransientWindow->raise();
            m_caseImplicitTransientWindow->activateWindow();
            return;
        }
        m_caseImplicitTransientWindow = new mCaseImplicitTransientWindow(this);
    }

    void mMenuWidget::slot_createNVHRandomVibrationAnalysis()
    {
        // 随机振动分析
        // 已存在（含最小化）则直接显示已有窗口，不再重复创建；已关闭则重建
        if (m_caseImplicitRandomVibrationWindow)
        {
            if (m_caseImplicitRandomVibrationWindow->isVisible())
            {
                if (m_caseImplicitRandomVibrationWindow->isMinimized())
                {
                    m_caseImplicitRandomVibrationWindow->showNormal();
                }
                m_caseImplicitRandomVibrationWindow->raise();
                m_caseImplicitRandomVibrationWindow->activateWindow();
                return;
            }
            // 已关闭：恢复默认状态后重新显示，复用实例避免整窗重建卡顿
            m_caseImplicitRandomVibrationWindow->resetToDefault();
            m_caseImplicitRandomVibrationWindow->show();
            m_caseImplicitRandomVibrationWindow->raise();
            m_caseImplicitRandomVibrationWindow->activateWindow();
            return;
        }
        m_caseImplicitRandomVibrationWindow = new mCaseImplicitRandomVibrationWindow(this);
    }

    void mMenuWidget::initWorkingConditionMenu()
    {
        // “载荷-边界”页拆分为两个独立页签：
        // 载荷页：力学载荷 / 热学载荷 / 动态载荷 / 符号
        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert("载荷", tempWid);

        // 边界页：预定义场 / 边界条件 / 子模型边界 / 转子分析 / 符号
        RibbonPage* tempBoundaryWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempBoundaryWid);
        m_StaWidMap.insert("边界", tempBoundaryWid);

        mGroupWidget* tCon1 = this->addGroup("载荷", "力学载荷");
        mGroupWidget* tCon4 = this->addGroup("载荷", "热学载荷");
        mGroupWidget* tCon5 = this->addGroup("载荷", "动态载荷");
        mGroupWidget* tCon3_1 = this->addGroup("载荷", "符号");
        mGroupWidget* tCon6 = this->addGroup("边界", "预定义场");
        mGroupWidget* tCon2 = this->addGroup("边界", "边界条件");
        mGroupWidget* tConSub = this->addGroup("边界", "子模型边界");
        mGroupWidget* tCon2_1 = this->addGroup("边界", "转子分析");
        mGroupWidget* tCon3 = this->addGroup("边界", "符号");

        // 横向一排大图标按钮：图标在上、文字在下（与分析工况页一致）
        auto addRowBtn = [this](mGroupWidget* group, const QString& text, const QString& icon,
            const QString& tip = QString()) {
                QToolButton* btn = group->addToolBtn(text, icon, -1, -1, 1, 1, QSize(30, 30));
                setQToolButton(btn, text, icon, tip.isEmpty() ? text + "。" : tip);
                btn->setIconSize(QSize(30, 30));
                return btn;
            };

        if (tCon1)
        {
            btn1Force = addRowBtn(tCon1, "力", ":/mMainWidgets/Icon/Case/li.png");
            connect(btn1Force, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_FORCE);
                });

            btn2Torque = addRowBtn(tCon1, "力矩", ":/mMainWidgets/Icon/Case/liju.png");
            connect(btn2Torque, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_TORQUE);
                });

            btn3Pressure = addRowBtn(tCon1, "压强", ":/mMainWidgets/Icon/Case/yaqiang.png");
            connect(btn3Pressure, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_PRESSURE);
                });

            btn7Surface = addRowBtn(tCon1, "表面载荷", ":/mMainWidgets/Icon/load/biaomianzaihe.png");
            connect(btn7Surface, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_SURFACE);
                });

            btn4Gravity = addRowBtn(tCon1, "重力", ":/mMainWidgets/Icon/Case/zhongli.png");
            connect(btn4Gravity, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_GRAVITY);
                });

            btn6RotatePower = addRowBtn(tCon1, "旋转体力", ":/mMainWidgets/Icon/load/旋转体力@3x.png");
            connect(btn6RotatePower, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_ROTATE_PHYSICAL_POWER);
                });

            btn5Bolt = addRowBtn(tCon1, "螺栓载荷", ":/mMainWidgets/Icon/Case/luoshuanzaihe.png");
            connect(btn5Bolt, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_BOLT);
                });

            btn8Edgeface = addRowBtn(tCon1, "边载荷", ":/mMainWidgets/Icon/Case/bianzaihe.png");
            connect(btn8Edgeface, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_EDGE);
                });
        }

        if (tCon6)
        {
            btn1InitTemp = addRowBtn(tCon6, "初始温度", ":/mMainWidgets/Icon/Case/chushiwendu.png");
            connect(btn1InitTemp, &QToolButton::clicked, this, [this]() {
                slot_createPredefinedFiel(MxFunctions::PredefinedFieldType::PREDEFINED_FIELD_INIT_TEMP);
                });

            btn2TempLoad = addRowBtn(tCon6, "温度载荷", ":/mMainWidgets/Icon/Case/wenduzaihe.png");
            connect(btn2TempLoad, &QToolButton::clicked, this, [this]() {
                slot_createPredefinedFiel(MxFunctions::PredefinedFieldType::PREDEFINED_FIELD_TEMP_LOAD);
                });
        }

        if (tCon2)
        {
            btn1Fixed = addRowBtn(tCon2, "固定约束", ":/mMainWidgets/Icon/Case/gudingyueshu.png");
            connect(btn1Fixed, &QToolButton::clicked, this, [this]() {
                slot_createBoundary(BOUNDARY_IM_FIXED);
                });

            btn4Displacement = addRowBtn(tCon2, "位移/转角", ":/mMainWidgets/Icon/Case/weiyizhuanjiao.png");
            connect(btn4Displacement, &QToolButton::clicked, this, [this]() {
                slot_createBoundary(BOUNDARY_IM_DISPLACEMENT);
                });

            btnSpeed = addRowBtn(tCon2, "速度", ":/mMainWidgets/Icon/Case/speed@3x.png");
            connect(btnSpeed, &QToolButton::clicked, this, [this]() {
                slot_createBoundary(BOUNDARY_IM_SPEED);
                });

            btnAcc = addRowBtn(tCon2, "加速度", ":/mMainWidgets/Icon/Case/acceleration@3x.png");
            connect(btnAcc, &QToolButton::clicked, this, [this]() {
                slot_createBoundary(BOUNDARY_IM_ACC);
                });

            btn6Temp = addRowBtn(tCon2, "温度边界", ":/mMainWidgets/Icon/Case/wendu.png");
            connect(btn6Temp, &QToolButton::clicked, this, [this]() {
                slot_createBoundary(BOUNDARY_IM_TEMP);
                });

            btn7Virtual = addRowBtn(tCon2, "虚拟约束", ":/mMainWidgets/Icon/Case/xuniyueshu.png");
            connect(btn7Virtual, &QToolButton::clicked, this, [this]() {
                slot_createBoundary(BOUNDARY_IM_VIRTUAL);
                });

            btn8Super = addRowBtn(tCon2, "超单元边界", ":/mMainWidgets/Icon/Case/tongyongyueshu.png");
            connect(btn8Super, &QToolButton::clicked, this, [this]() {
                slot_createBoundary(BOUNDARY_IM_SUPERELEMENT);
                });
        }

        if (tConSub)
        {
            btnSubMove = addRowBtn(tConSub, "位移边界", ":/mMainWidgets/Icon/load/位移边界@3x.png", "子模型位移边界。");
            connect(btnSubMove, &QToolButton::clicked, this, [this]() {
                slot_createBoundary(MxFunctions::LoadBoundaryType::BOUNDARY_MOVE);
                });

            btnSubTemp = addRowBtn(tConSub, "温度边界", ":/mMainWidgets/Icon/load/温度边界@3x.png", "子模型温度边界。");
            connect(btnSubTemp, &QToolButton::clicked, this, [this]() {
                slot_createBoundary(MxFunctions::LoadBoundaryType::BOUNDARY_TEMP_NEW);
                });
        }

        if (tCon2_1)
        {
            btnRotSpeed = addRowBtn(tCon2_1, "旋转速度", ":/mMainWidgets/Icon/Case/旋转速度@3x.png");
            connect(btnRotSpeed, &QToolButton::clicked, this, [this]() {
                slot_createRotateAnalyse(MxFunctions::RotateAnalysyType::ROTATIONAL_SPEED);
                });

            btnRotAcc = addRowBtn(tCon2_1, "旋转加速度", ":/mMainWidgets/Icon/Case/旋转加速度@3x.png");
            connect(btnRotAcc, &QToolButton::clicked, this, [this]() {
                slot_createRotateAnalyse(MxFunctions::RotateAnalysyType::ROTATIONAL_ACC);
                });
        }

        if (tCon3)
        {
            btnResizeSymbol = tCon3->addToolBtn("重新调整", ":/mMainWidgets/Icon/Case/chongxintiaozheng.png", -1, -1, 1, 1, QSize(30, 30));
            setQToolButton(btnResizeSymbol, "重新调整", ":/mMainWidgets/Icon/Case/chongxintiaozheng.png", "重新调整。", Qt::ToolButtonTextUnderIcon);
            btnResizeSymbol->setIconSize(QSize(30, 30));
            btnResizeSymbol->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            // 向右偏移 10px，使文字与下方“符号”对齐
            btnResizeSymbol->setStyleSheet("QToolButton { padding-left: 9px; }");
            btnResizeSymbol->installEventFilter(new TooltipInstaller("重新调整",
                QPixmap(":/mMainWidgets/Icon/Case/chongxintiaozheng.png"),
                "", 200, btnResizeSymbol));
            connect(btnResizeSymbol, &QToolButton::clicked, this, &mMenuWidget::slot_resizeSymbol);
        }

        if (tCon4)
        {
            btn1HeatFlux = addRowBtn(tCon4, "热流密度", ":/mMainWidgets/Icon/Case/reliumidu.png");
            connect(btn1HeatFlux, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_HEATFLUX);
                });

            btn3Convection = addRowBtn(tCon4, "对流换热", ":/mMainWidgets/Icon/Case/duiliuhuanre.png");
            connect(btn3Convection, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_CONVECTION);
                });

            btn2Radiation = addRowBtn(tCon4, "热辐射", ":/mMainWidgets/Icon/Case/refushe.png");
            connect(btn2Radiation, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_RADIATION);
                });
        }

        if (tCon5)
        {
            btn1Transient = addRowBtn(tCon5, "时域载荷", ":/mMainWidgets/Icon/Case/shuntaizaihe.png");
            connect(btn1Transient, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_TRANSIENT);
                });

            btn2FreqDomain = addRowBtn(tCon5, "频域载荷", ":/mMainWidgets/Icon/Case/pinyuzaihe.png");
            connect(btn2FreqDomain, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_FREQDOMAIN);
                });

            btn3Combined = addRowBtn(tCon5, "组合载荷", ":/mMainWidgets/Icon/Case/zuhezaihe.png");
            connect(btn3Combined, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_COMBINED);
                });

            btn4Spectrum = addRowBtn(tCon5, "谱载荷", ":/mMainWidgets/Icon/Case/puzaihe.png");
            connect(btn4Spectrum, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_SPECTRUM);
                });

            btn5Acoustic = addRowBtn(tCon5, "声学载荷", ":/mMainWidgets/Icon/Case/shengxuejizhongli.png");
            connect(btn5Acoustic, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_ACOCONCFORCE);
                });

            btn6AcousticFreq = addRowBtn(tCon5, "声学频域载荷", ":/mMainWidgets/Icon/Case/shengxuepinyuzaihe.png");
            connect(btn6AcousticFreq, &QToolButton::clicked, this, [this]() {
                slot_createLoad(MxFunctions::LoadBoundaryType::LOAD_IM_ACOFREQDOMAIN);
                });
        }

        if (tCon3_1)
        {
            btnResizeSymbolLoad = tCon3_1->addToolBtn("重新调整", ":/mMainWidgets/Icon/Case/chongxintiaozheng.png", -1, -1, 1, 1, QSize(30, 30));
            setQToolButton(btnResizeSymbolLoad, "重新调整", ":/mMainWidgets/Icon/Case/chongxintiaozheng.png", "重新调整。", Qt::ToolButtonTextUnderIcon);
            btnResizeSymbolLoad->setIconSize(QSize(30, 30));
            btnResizeSymbolLoad->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            // 向右偏移 10px，使文字与下方“符号”对齐
            btnResizeSymbolLoad->setStyleSheet("QToolButton { padding-left: 9px; }");
            btnResizeSymbolLoad->installEventFilter(new TooltipInstaller("重新调整",
                QPixmap(":/mMainWidgets/Icon/Case/chongxintiaozheng.png"),
                "", 200, btnResizeSymbolLoad));
            connect(btnResizeSymbolLoad, &QToolButton::clicked, this, &mMenuWidget::slot_resizeSymbol);
        }

        // ===== 各分析工况可用的载荷/边界/预定义场按钮映射（单一权威版本）=====
        // 依据《MxSim.Mechanical通用结构仿真软件功能清单》"工况（附件）"支持矩阵，
        // 合并原三份重复的 F–T 注册块（QMap 按键覆盖，旧块仅为历史残留）：
        //  - 频响的模态/直接频响合并为 ImplicitFrequency，瞬态的模态/直接瞬态合并为 ImplicitTransient；
        //  - 功能清单未单列的按钮沿用原行为：表面载荷随力学载荷工况集，声学载荷保留声固耦合；
        //  - 边载荷、超单元边界（通用约束）、初始应力：功能清单未启用，不注册；
        //  - 模态：固定约束+位移/转角；随机振动无可用按钮；超单元仅固定约束、位移/转角（符号置灰）；
        //  - 转子分析按钮不在此注册，由选中变化逻辑按"复模态+转子动力学"专项控制。
        // 线性静力学
        {
            auto ct = MxFunctions::ImplicitLinearStatic;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_FORCE] = btn1Force;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_TORQUE] = btn2Torque;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_PRESSURE] = btn3Pressure;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_SURFACE] = btn7Surface;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_GRAVITY] = btn4Gravity;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_BOLT] = btn5Bolt;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_ROTATE_PHYSICAL_POWER] = btn6RotatePower;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_FIXED] = btn1Fixed;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_DISPLACEMENT] = btn4Displacement;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_VIRTUAL] = btn7Virtual;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_MOVE] = btnSubMove;
            casePredefinedFieldBtns[ct][MxFunctions::PREDEFINED_FIELD_INIT_TEMP] = btn1InitTemp;
            casePredefinedFieldBtns[ct][MxFunctions::PREDEFINED_FIELD_TEMP_LOAD] = btn2TempLoad;
            caseSymbolBtns[ct] = btnResizeSymbol;
        }
        // 非线性静力学
        {
            auto ct = MxFunctions::ImplicitNonLinearStatic;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_FORCE] = btn1Force;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_TORQUE] = btn2Torque;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_PRESSURE] = btn3Pressure;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_SURFACE] = btn7Surface;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_GRAVITY] = btn4Gravity;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_BOLT] = btn5Bolt;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_ROTATE_PHYSICAL_POWER] = btn6RotatePower;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_FIXED] = btn1Fixed;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_DISPLACEMENT] = btn4Displacement;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_VIRTUAL] = btn7Virtual;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_MOVE] = btnSubMove;
            casePredefinedFieldBtns[ct][MxFunctions::PREDEFINED_FIELD_INIT_TEMP] = btn1InitTemp;
            casePredefinedFieldBtns[ct][MxFunctions::PREDEFINED_FIELD_TEMP_LOAD] = btn2TempLoad;
            caseSymbolBtns[ct] = btnResizeSymbol;
        }
        // 模态分析
        {
            auto ct = MxFunctions::ImplicitModal;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_FIXED] = btn1Fixed;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_DISPLACEMENT] = btn4Displacement;
            caseSymbolBtns[ct] = btnResizeSymbol;
        }
        // 响应谱分析
        {
            auto ct = MxFunctions::ImplicitResponseSpectrum;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_SPECTRUM] = btn4Spectrum;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_FIXED] = btn1Fixed;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_DISPLACEMENT] = btn4Displacement;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_MOVE] = btnSubMove;
            caseSymbolBtns[ct] = btnResizeSymbol;
        }
        // 随机振动分析（无可用按钮）
        {
            caseSymbolBtns[MxFunctions::ImplicitRandomVibration] = nullptr;
        }
        // 频响分析（模态频响/直接频响）
        {
            auto ct = MxFunctions::ImplicitFrequency;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_FREQDOMAIN] = btn2FreqDomain;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_COMBINED] = btn3Combined;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_FIXED] = btn1Fixed;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_DISPLACEMENT] = btn4Displacement;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_MOVE] = btnSubMove;
            caseSymbolBtns[ct] = btnResizeSymbol;
        }
        // 瞬态分析（模态瞬态/直接瞬态）
        {
            auto ct = MxFunctions::ImplicitTransient;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_TRANSIENT] = btn1Transient;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_COMBINED] = btn3Combined;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_FIXED] = btn1Fixed;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_DISPLACEMENT] = btn4Displacement;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_MOVE] = btnSubMove;
            caseSymbolBtns[ct] = btnResizeSymbol;
        }
        // 声固耦合分析
        {
            auto ct = MxFunctions::ASCoupling;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_FREQDOMAIN] = btn2FreqDomain;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_COMBINED] = btn3Combined;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_ACOCONCFORCE] = btn5Acoustic;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_ACOFREQDOMAIN] = btn6AcousticFreq;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_FIXED] = btn1Fixed;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_DISPLACEMENT] = btn4Displacement;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_MOVE] = btnSubMove;
            caseSymbolBtns[ct] = btnResizeSymbol;
        }
        // 超单元分析（仅固定约束、位移/转角，符号置灰）
        {
            auto ct = MxFunctions::ImplicitGeneric;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_FIXED] = btn1Fixed;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_DISPLACEMENT] = btn4Displacement;
            caseSymbolBtns[ct] = nullptr;
        }
        // 非线性动力学
        {
            auto ct = MxFunctions::ImplicitNonLinearDynamic;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_FORCE] = btn1Force;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_TORQUE] = btn2Torque;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_PRESSURE] = btn3Pressure;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_SURFACE] = btn7Surface;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_GRAVITY] = btn4Gravity;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_BOLT] = btn5Bolt;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_ROTATE_PHYSICAL_POWER] = btn6RotatePower;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_FIXED] = btn1Fixed;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_DISPLACEMENT] = btn4Displacement;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_SPEED] = btnSpeed;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_ACC] = btnAcc;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_MOVE] = btnSubMove;
            casePredefinedFieldBtns[ct][MxFunctions::PREDEFINED_FIELD_INIT_TEMP] = btn1InitTemp;
            casePredefinedFieldBtns[ct][MxFunctions::PREDEFINED_FIELD_TEMP_LOAD] = btn2TempLoad;
            caseSymbolBtns[ct] = btnResizeSymbol;
        }
        // 线性屈曲 / 后屈曲
        for (auto ct : { MxFunctions::ImplicitLinearBuckling, MxFunctions::ImplicitPostBuckling })
        {
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_FORCE] = btn1Force;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_TORQUE] = btn2Torque;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_PRESSURE] = btn3Pressure;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_SURFACE] = btn7Surface;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_GRAVITY] = btn4Gravity;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_BOLT] = btn5Bolt;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_ROTATE_PHYSICAL_POWER] = btn6RotatePower;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_FIXED] = btn1Fixed;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_DISPLACEMENT] = btn4Displacement;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_MOVE] = btnSubMove;
            caseSymbolBtns[ct] = btnResizeSymbol;
        }
        // 稳态传热 / 瞬态传热
        for (auto ct : { MxFunctions::ImplicitSteadyHeat, MxFunctions::ImplicitTransientHeat })
        {
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_HEATFLUX] = btn1HeatFlux;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_RADIATION] = btn2Radiation;
            caseLoadBoundaryBtns[ct][MxFunctions::LOAD_IM_CONVECTION] = btn3Convection;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_IM_TEMP] = btn6Temp;
            caseLoadBoundaryBtns[ct][MxFunctions::BOUNDARY_TEMP_NEW] = btnSubTemp;
            casePredefinedFieldBtns[ct][MxFunctions::PREDEFINED_FIELD_INIT_TEMP] = btn1InitTemp;
            caseSymbolBtns[ct] = btnResizeSymbol;
        }

        _workingConditionGW = { tCon6, tCon1, tCon4, tCon5, tCon2, tCon3 , tConSub, tCon2_1, tCon3_1 };
    }

    void mMenuWidget::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);

        //先于下方getCVM/viewportPrivate判空早退执行：视口全删（currentViewportChanged(-1)）时
        //也必须把“结果”/“云图”页置为不可用
        refreshPostMenuEnabledState();

        if (!getCVM)
        {
            return;
        }
        if (_connPick)
        {
            QObject::disconnect(_connPick);
        }
        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
        {
            return;
        }

        //模型切换后同步界面单位制：单位制数据随模型各自持有（modelData 的
        //mUnitSystemManager），界面勾选状态需按新当前模型刷新
        //（与打开工程文件时的恢复逻辑一致，走 UI 同步信号避免重复换算数据）
        QVector<int> unitsIdx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        if (unitsIdx.size() == 4)
        {
            emit mMechGlobalSignals::getInstance()->unitSystemChanged(unitsIdx[0]);
            emit mMechGlobalSignals::getInstance()->angleUnitChanged(unitsIdx[1]);
            emit mMechGlobalSignals::getInstance()->speedUnitChanged(unitsIdx[2]);
            emit mMechGlobalSignals::getInstance()->tempUnitChanged(unitsIdx[3]);
        }
        mInfoWindow::getInstance()->updateAllPanelUnits();

        bool enable = viewportPrivate->getViewportType() == Viewport3D ? true : false;
        //禁用和启用按钮（弹簧、阻尼实体化仅在三维视口可用）
        auto actions = _displayGW.at(1)->getButtons();
        for (auto btn : actions)
        {
            if (btn->text() == "弹簧" || btn->text() == "阻尼")
            {
                btn->setEnabled(enable);
            }
        }

        _selectGW.at(0)->setEnabled(enable);
        _selectGW.at(1)->setEnabled(enable);
        _selectGW.at(2)->setEnabled(enable);
        _selectGW.at(3)->setEnabled(enable);
        _selectGW.at(4)->setEnabled(enable);
        auto buttons = _selectGW.at(1)->getButtons();
        //选择
        for (auto btn : buttons)
        {
            if (btn->text() == "圆柱面" || btn->text() == "公共边")
            {
                btn->setEnabled(enable);
            }
        }
        if (viewportPrivate->getViewportType() == Viewport3D && getCVM->getPreRenderWidget())
        {
            _connPick = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMenuWidget::slot_finishedPick);
            slot_finishedPick();
        }
        else
        {
            //后处理禁用显示菜单栏某些按钮、禁用选择菜单栏某些按钮

        }

        //按激活视口类型联动菜单栏工作模式（映射见workModeForViewportType）。
        //setWorkMode同模式幂等，同上下文内部（如后处理视口之间切换）
        //不打扰用户当前页签。
        setWorkMode(workModeForViewportType(viewportPrivate->getViewportType()));
    }

    void mMenuWidget::slot_viewportUserFocused(int viewportID)
    {
        //视口聚焦广播（ViewportContainer::sig_viewportUserFocused）：激活变化场景
        //已由currentViewportChanged->slot_subWindowActivated同步，此处只兜"点击的
        //已是当前激活视口"（同ID被容器/管理器去重早退、无激活信号，菜单会停留在
        //旧模式：启动恢复错位、图表删除后静默转移激活等）。同模式幂等不打扰；
        //图表卡片等非渲染视口查无priv，其上下文由卡片激活专属联动负责
        auto* priv = VMManager ? VMManager->getViewportPrivateByID(viewportID) : nullptr;
        if (priv == nullptr)
        {
            return;
        }
        const MenuWorkMode newMode = workModeForViewportType(priv->getViewportType());
        if (newMode != _workMode)
        {
            setWorkMode(newMode);//内部含syncTreeWindowsForMode，左侧树一并矫正
        }
    }

    void mMenuWidget::slot_finishedPick()
    {
        if (!getCVM)
        {
            return;
        }
        int pickNum = 0;
        auto pickDataType = getCVM->getCurrentPickDataType(pickNum);

        //更新选择菜单栏中的状态
        //扩展
        auto buttonList0 = _selectGW.at(0)->getButtons();

        //特征
        auto buttonList1 = _selectGW.at(1)->getButtons();

        //位置
        auto buttonList2 = _selectGW.at(2)->getButtons();

        //网格
        auto buttonList3 = _selectGW.at(3)->getButtons();

        //转换
        auto buttonList4 = _selectGW.at(4)->getButtons();

        //批量操作
        auto buttonList5 = _selectGW.at(5)->getButtons();

        if (pickNum == 0)
        {
            _selectGW.at(0)->setEnabled(false);
            for (auto btn : buttonList1)
            {
                if (btn->text() == "公共边" || btn->text() == "增长单元")
                {
                    btn->setEnabled(false);
                }
            }
            _selectGW.at(4)->setEnabled(false);
            for (auto btn : buttonList2)
            {
                if (btn->text() == "位置" || btn->text() == "基于已选选择实体")
                {
                    btn->setEnabled(false);
                }
            }
            for (auto btn : buttonList5)
            {
                if (btn->text() == "隐藏当前" || btn->text() == "隐藏其他")
                {
                    btn->setEnabled(false);
                }
            }
        }
        else
        {
            //扩展
            _selectGW.at(0)->setEnabled(true);
            if (pickDataType == PickDataType::GeoFace)
            {
                for (auto btn : buttonList0)
                {
                    btn->setEnabled(true);
                }
            }
            else if (pickDataType == PickDataType::GeoEdge)
            {
                for (auto btn : buttonList0)
                {
                    if (btn->text() == "相邻" || btn->text() == "限值")
                    {
                        btn->setEnabled(true);
                    }
                    else
                    {
                        btn->setEnabled(false);
                    }
                }
            }
            else
            {
                _selectGW.at(0)->setEnabled(false);
            }

            //特征
            for (auto btn : buttonList1)
            {
                if (btn->text() == "公共边")
                {
                    if (pickDataType == PickDataType::GeoFace && pickNum == 2)
                    {
                        btn->setEnabled(true);
                    }
                    else
                    {
                        btn->setEnabled(false);
                    }
                }
            }
            //位置选择/尺寸选择
            for (auto btn : buttonList2)
            {
                if (btn->text() == "位置")
                {
                    if (pickNum == 1 && pickDataType >= PickDataType::GeoPoint && pickDataType <= PickDataType::GeoSolid)
                    {
                        btn->setEnabled(true);
                    }
                    else
                    {
                        btn->setEnabled(false);
                    }
                }
                else if (btn->text() == "基于已选选择实体")
                {
                    if (pickNum >= 1 && pickDataType >= PickDataType::GeoEdge && pickDataType <= PickDataType::GeoSolid)
                    {
                        btn->setEnabled(true);
                    }
                    else
                    {
                        btn->setEnabled(false);
                    }
                }
            }

            //网格
            for (auto btn : buttonList3)
            {
                if (btn->text() == "增长单元")
                {
                    if (pickDataType == PickDataType::Element)
                    {
                        btn->setEnabled(true);
                    }
                    else
                    {
                        btn->setEnabled(false);
                    }
                }
            }

            //转换
            _selectGW.at(4)->setEnabled(true);
            for (auto btn : buttonList4)
            {
                btn->setEnabled(true);
            }
            buttonList4.at(int(pickDataType))->setEnabled(false);


            //模型显隐
            for (auto btn : buttonList5)
            {
                if (btn->text() == "隐藏当前" || btn->text() == "隐藏其他")
                {
                    if ((pickDataType >= PickDataType::GeoPoint && pickDataType <= PickDataType::GeoSolid) || pickDataType == PickDataType::Element)
                    {
                        btn->setEnabled(true);
                    }
                }
            }
        }

    }

    void mMenuWidget::addCoordinateTransform(const QString& type, const QString& value, int id)
    {
        if (type.isEmpty())
        {
            return;
        }
        int coordinateID = id;
        if (id == -1)
        {
            auto items = VMManager->getCurrentViewportPrivate()->getTreeSelectItems();
            if (items.size() != 1)
            {
                return;
            }
            coordinateID = items[0]._phyID;
        }

        auto coordinateData = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(coordinateID);
        if (coordinateData == nullptr)
        {
            return;
        }

        auto item = new QListWidgetItem();
        QWidget* itemWidget = new QWidget;
        QHBoxLayout* layout = new QHBoxLayout(itemWidget);

        QLabel* lbl = new QLabel(type);
        layout->addWidget(lbl);
        if (!type.startsWith("翻转"))
        {
            mCalcLineEdit* edit = new mCalcLineEdit(value);
            edit->setFixedWidth(60);
            layout->addWidget(edit);
            QObject::connect(edit, &QLineEdit::editingFinished, this, [=]() {
                int index = _transformList->row(item);
                coordinateData->replaceTransformStep(index, edit->text());
                mInfoWindow::getInstance()->editCoordinate(coordinateID);
                });
        }
        else
        {
            if ((type.contains("X") && value == "+X") ||
                (type.contains("Y") && value == "+Y") ||
                (type.contains("Z") && value == "+Z"))
            {
                return;
            }
            QComboBox* combo = new QComboBox();
            combo->setStyleSheet(R"(
			QComboBox::drop-down
			{
				subcontrol-origin:padding;
				subcontrol-position:top right;
				width:30px;
				border-top-right-radius:3px;
				border-bottom-right-radius:3px;
			}
			QComboBox::down-arrow
			{
				border-image:url(:/mMainWidgets/Icon/down-list.png);
				width:10px;
				height:10px;
			}
			QComboBox::selected
			{
				background-color: #299DFF;
				color:#FFFFFF;
				border: 1px solid #DCDFE6;
			}
			QComboBox
			{
				height:20px;
				border-radius:4px;
				background-color:#FFFFFF;
				border:1px solid #DCDFE6;
			}
		    )");
            combo->setFixedWidth(60);
            combo->addItems({ QString("+X"),QString("-X"),QString("+Y"),QString("-Y"),QString("+Z"),QString("-Z") });
            combo->setCurrentText(value);
            layout->addWidget(combo);
            QObject::connect(combo, &QComboBox::currentTextChanged, this, [=](const QString& text) {
                int index = _transformList->row(item);
                coordinateData->replaceTransformStep(index, text);
                mInfoWindow::getInstance()->editCoordinate(coordinateID);
                });
        }
        QPushButton* btnUp = new QPushButton("↑");
        QPushButton* btnDown = new QPushButton("↓");
        QPushButton* btnDel = new QPushButton();
        btnDel->setIcon(QIcon(":/mMainWidgets/Icon/close3x.png"));
        btnDel->setFixedSize(40, 20);
        connect(btnDel, &QPushButton::clicked, this, [=]() {
            //获取index，然后
            int index = _transformList->row(item);
            coordinateData->removeTransformStep(index);
            mInfoWindow::getInstance()->editCoordinate(coordinateID);
            _transformList->takeItem(index);
            delete item;
            });

        QObject::connect(btnUp, &QPushButton::clicked, this, [=]() {
            int row = _transformList->row(item);
            if (row > 0) {
                // 保存当前项和上方项及其 widget
                QListWidgetItem* itemAbove = _transformList->item(row - 1);
                QWidget* wItem = _transformList->itemWidget(item);
                QWidget* wAbove = _transformList->itemWidget(itemAbove);

                QLayout* layout = wItem->layout();
                QLayout* layoutAbove = wAbove->layout();

                // 临时解绑，防止 Qt 自动 delete widget
                _transformList->removeItemWidget(item);
                _transformList->removeItemWidget(itemAbove);

                // 把 item 和 aboveItem 都移除列表（takeItem 返回裸指针，不 delete）
                QListWidgetItem* takenItem = _transformList->takeItem(row);
                QListWidgetItem* takenAbove = _transformList->takeItem(row - 1);

                // 重新插入，顺序交换
                _transformList->insertItem(row - 1, takenItem);
                _transformList->insertItem(row, takenAbove);

                auto widget = new QWidget();
                widget->setLayout(layout);

                auto widgetAbove = new QWidget();
                widgetAbove->setLayout(layoutAbove);
                // 恢复 widget 到正确位置
                //wItem->showMaximized();
                _transformList->setItemWidget(takenItem, widget);
                _transformList->setItemWidget(takenAbove, widgetAbove);

                // 同步逻辑层顺序
                if (coordinateData) {
                    coordinateData->moveTransformStep(row, row - 1);
                    mInfoWindow::getInstance()->editCoordinate(coordinateID);
                }
            }
            });


        QObject::connect(btnDown, &QPushButton::clicked, this, [=]() {
            int row = _transformList->row(item);
            if (row >= 0 && row < _transformList->count() - 1) {
                QListWidgetItem* itemBelow = _transformList->item(row + 1);
                QWidget* wItem = _transformList->itemWidget(item);
                QWidget* wBelow = _transformList->itemWidget(itemBelow);

                QLayout* layout = wItem->layout();
                QLayout* layoutBelow = wBelow->layout();

                _transformList->removeItemWidget(item);
                _transformList->removeItemWidget(itemBelow);

                QListWidgetItem* takenBelow = _transformList->takeItem(row + 1);
                QListWidgetItem* takenItem = _transformList->takeItem(row);

                _transformList->insertItem(row, takenBelow);
                _transformList->insertItem(row + 1, takenItem);

                auto widget = new QWidget();
                widget->setLayout(layout);

                auto widgetBelow = new QWidget();
                widgetBelow->setLayout(layoutBelow);

                _transformList->setItemWidget(takenBelow, widgetBelow);
                _transformList->setItemWidget(takenItem, widget);

                if (coordinateData) {
                    coordinateData->moveTransformStep(row, row - 1);
                    mInfoWindow::getInstance()->editCoordinate(coordinateID);
                }
            }
            });

        layout->setContentsMargins(6, 2, 6, 2);
        layout->setSpacing(6);
        layout->addStretch();
        layout->addWidget(btnUp);
        layout->addWidget(btnDown);
        layout->addWidget(btnDel);
        itemWidget->setLayout(layout);
        _transformList->addItem(item);
        _transformList->setItemWidget(item, itemWidget);
        if (id == -1)
        {
            // 同步数据到内部队列（C++逻辑层）
            MechCoordinateData::TransformStep step;
            coordinateData->addTransformStep(type, value);

            mInfoWindow::getInstance()->editCoordinate(coordinateID);
        }
    }

    bool mMenuWidget::eventFilter(QObject* watched, QEvent* event)
    {
        // 检查事件类型是否是鼠标按下，并且我们的列表窗口当前是可见的 
        if (event->type() == QEvent::MouseButtonPress && _transformList->isVisible())
        { // 将通用 QEvent 转换为更具体的 QMouseEvent 以获取鼠标位置 
            auto* mouseEvent = static_cast<QMouseEvent*>(event); // 检查鼠标点击的全局坐标是否在 _transformList 的矩形区域“之外” // geometry() 返回的是窗口相对于其父窗口的坐标和尺寸，但对于顶级窗口，它就是全局坐标 
            if (!_transformList->geometry().contains(mouseEvent->globalPos()))
            { // 如果点击在外部，则隐藏列表窗口 
                _transformList->hide();
            }
        } // 最后，将事件交还给Qt的默认处理机制，以确保其他功能正常工作 
        return QMenuBar::eventFilter(watched, event);
    }

    void mMenuWidget::deleteViewportModel(bool isNew)
    {
        mViewTreeFunction::deleteViewportModel(getCVM->getModelData()->getModelDataID());
        // mpcomm 平台会话中，删除模型后通知平台清空单元格状态
        emit MechCommon::mMechGlobalSignals::getInstance()->mpcommModelCleared();
        if (VMManager->getViewportModelSize() > 0 && !isNew)
        {
            VMManager->setCurrentViewportModel(VMManager->getBeginModelID());
            bool isPost = getCVM->isPostModel();
            if (!isPost)
            {
                mViewTreeFunction::activatePreViewport();
            }
            else
            {
                mViewTreeFunction::activatePostViewport();
            }
        }
        else
        {
            mViewTreeFunction::newViewportModel(isNew);
        }
    }

    int mMenuWidget::isThreadCloseOrFileSave()
    {
        //第一步：读写任务保护——有文件读写线程运行时先确认，确认后中断线程再继续
        if (MechIOFile::mIOFileManager::getInstance()->hasRunningThread())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            int btn = MechMainWindow::mFramelessMessageBox::warning(window(), QString("警告"),
                QString("当前有未完成的读/写文件任务，是否仍要退出？"),
                QString("是"),
                QString("否"));

            if (btn != 0)
            {
                return 1;
            }
            //中断读写线程：mxcae线程协作式取消，第三方文件线程按threadFlag取消
            MechIOFile::mIOFileManager::getInstance()->killFileThread();
            //读取时的置灰/进度条状态原本由完成槽恢复，完成槽已随线程断开，这里补恢复，
            //保证用户后续在保存询问中选择"取消"留在程序时界面仍可用
            mGlobalSignals::getInstance()->setProgressBarValue(100);
            mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
        }

        //第二步：保存询问（不判断数据是否修改，每次退出均询问）
        return confirmSaveMxsesBeforeDiscard(QString("退出前是否保存当前文件？")) ? 0 : 1;
    }

	bool mMenuWidget::confirmSaveMxsesBeforeDiscard(const QString& question)
	{
		int btn = MechMainWindow::mFramelessMessageBox::warning(window(), QString("提示"),
			question,
			QString("保存"),
			QString("不保存"),
			QString("取消"));

		switch (btn)
		{
		case 0:
		{
			//保存mxses会话文件：每次都弹出保存对话框让用户显式选路径，
			//取消对话框或保存失败视为取消后续动作
			QString workingdir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
			QString savepath = QFileDialog::getSaveFileName(window(),
				"保存会话文件",
				workingdir,
				QString("MxSim Session(*.mxses)"));
			if (savepath.isEmpty() || !mMxsesRestore::saveSession(savepath))
			{
				return false;
			}
			//保存为异步写线程，等待写完成后再继续（后续动作会丢弃/清空当前数据）；
			//排除用户输入事件，避免等待期间继续操作模型破坏序列化数据
			mGlobalSignals::getInstance()->outputMessageSig(QString("正在保存会话文件，请稍候......"));
			QEventLoop loop;
			connect(MechIOFile::mIOFileManager::getInstance(), &MechIOFile::mIOFileManager::writeMxsesFinishedSig,
				&loop, &QEventLoop::quit);
			loop.exec(QEventLoop::ExcludeUserInputEvents);
			return true;
		}
		case 1:
			//不保存，继续后续动作
			return true;
		case 2:
		default:
			//取消后续动作
			return false;
		}
	}

    void mMenuWidget::addMenu(const QString& menuName)
    {
        // 添加一个tab标签
        int tabIndex = m_menuTabBar_->addTab(menuName);

		// 页签栏样式由 ThemeManager::applyPalette 统一下发：此处不能再清空样式表——
		// 动态页签（草图/截面属性/后处理等）在运行期添加，清一次就把已下发的
		// 页签栏主题 QSS（底色/滚动按钮配色）整体抹掉
		//m_menuTabBar_->setStyle(new PlainTextTabStyle(m_menuTabBar_->style()));
		// 2. 你已有的 tabBar 上套样式（任意地方执行一次即可）

        //qDebug() << "style class =" << m_menuTabBar_->style()->metaObject()->className();
        return;
    }

    RibbonPage* mMenuWidget::pageForTabText(const QString& title) const
    {
        //Post模式同名页优先解析到后处理独立页（页键见IRibbonHost.h的kPostXxxPageKey；
        //value()只读查询：operator[]会在页尚未注册时向map插入nullptr污染同名复用判断）
        if (_workMode == MenuWorkMode::Post)
        {
            if (RibbonPage* page = m_StaWidMap.value(title + "#Post", nullptr))
                return page;
        }
        return m_StaWidMap.value(title, nullptr);
    }

    void mMenuWidget::slot_CurrentTabChanged(int index)
    {
        QString title = m_menuTabBar_->tabText(index);
        RibbonPage* widget = pageForTabText(title);
        if (widget == nullptr) return;
        m_toolBarStackedWid_->setCurrentWidget(widget);
        widget->updateMenuGeometry();

        //菜单栏页签切换不退出草图：草图状态与菜单浏览解耦，
        //退出仅由明确路径触发（结束草图绘制按钮/模型树复制、删除模型等），
        //"草图"页签在退出前保留在栏上，可切回继续使用
        mGeoSketchSignalManager::instance()->clearButtonSelection();
    }

    void mMenuWidget::slot_sketchExited()
    {
        //移除动态页签位上的"草图"页签。若它恰为当前页签，QTabBar 会自动选中相邻页签，
        //触发的 slot_CurrentTabChanged 中 currentStatus 已为 false，不会重入 exitSketch
        const int sketchIdx = findTabByText(QString::fromUtf8("草图"));
        if (sketchIdx >= 0)
        {
            m_menuTabBar_->removeTab(sketchIdx);
        }
    }

    RibbonPage* mMenuWidget::createRibbonPage(const QString& name)
    {
        //同名页已存在时直接复用（原initXXXMenu重复注册会在stacked容器中遗留孤儿页）；
        //null条目（历史时序问题遗留）不复用，新建覆盖
        auto it = m_StaWidMap.find(name);
        if (it != m_StaWidMap.end() && it.value() != nullptr)
            return it.value();

        RibbonPage* tempWid = new RibbonPage();
        m_toolBarStackedWid_->addWidget(tempWid);
        m_StaWidMap.insert(name, tempWid);
        return tempWid;
    }

    void mMenuWidget::switchToPage(const QString& name)
    {
        switchMenuBarWindow(name);
    }

    void mMenuWidget::installTooltip(QWidget* target, const QString& name, const QString& image,
        const QString& describe, int msec)
    {
        if (target == nullptr)
            return;
        target->installEventFilter(new TooltipInstaller(name, QPixmap(image), describe, msec, target));
    }

    mGroupWidget* mMenuWidget::addGroup(QString menuName, QString txtGroupTitle)
    {
        RibbonPage* tempPage = m_StaWidMap.value(menuName, nullptr); // menuName对应的ribbonPage
        if (tempPage == nullptr)
            return NULL;
        QWidget* tempWid = tempPage->widget();// 获得该rinbboPage对应的widget
        mGroupWidget* newGW = new mGroupWidget(txtGroupTitle);// 新建GroupWidget并设置标题

        // 获取当前布局，布局插入新Group，新Group添加到ribbonPage
        QHBoxLayout* tempHB = (QHBoxLayout*)(tempWid->layout());
        tempHB->insertWidget(tempHB->count() - 1, newGW);
        tempPage->appendGroup(newGW);

        return newGW;
    }
    void mMenuWidget::addSpacerItem(QString menuName)
    {
        RibbonPage* tempPage = m_StaWidMap.value(menuName, nullptr); // menuName对应的ribbonPage
        if (tempPage == nullptr)
            return;
        QWidget* tempWid = tempPage->widget();// 获得该rinbboPage对应的widget

        // 获取当前布局，布局插入新Group，新Group添加到ribbonPage
        QHBoxLayout* tempHB = (QHBoxLayout*)(tempWid->layout());
        tempHB->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    }

	void mMenuWidget::setQToolButton(QToolButton* btn,QString name,QString image,QString describe, Qt::ToolButtonStyle style)
	{
		// 大/小按钮形态由创建时的 uiBigButton 标记决定，尺寸随档位下发
		const bool isBig = btn->property("uiBigButton").toBool();
		const int btnH = isBig ? UiMetrics::bigButtonHeight() : UiMetrics::halfButtonHeight();
		btn->setFixedHeight(btnH);
		// 图标尺寸直接给档位值（大图标 menuLargeIconSize=H+4、小图标 icon=max(H/2+3,G+10)），
		// 不再用"按钮高-留白"反推——那会大于图标规范尺寸，且与自绘不一致
		const int iconEdge = isBig ? UiMetrics::menuLargeIconSize() : UiMetrics::menuIconSize();
		btn->setIconSize(QSize(iconEdge, iconEdge));
		// 只有 QToolButton 才有 toolButtonStyle；mRibbonButton 形态由构造时的 Mode 决定
		if (auto* tb = qobject_cast<QToolButton*>(btn))
			tb->setToolButtonStyle(style);
		btn->installEventFilter(new TooltipInstaller(name,
			QPixmap(image),
			describe,
			200, btn));
		// 像素字号直接生效（构造函数的第二参是磅值，会被下一行覆盖，不必传）
		QFont font(UiMetrics::uiFontFamily());
		font.setPixelSize(UiMetrics::buttonFontSize());
		btn->setFont(font);
		// 内容决定最小宽度；同列按钮的最终等宽由 mGroupWidget::updateLayout 统一计算
		btn->setFixedWidth(ThemeManager::ribbonButtonMinWidth(btn, btn->property("mxHasMenu").toBool()));
	}

    void mMenuWidget::switchMenuBarWindow(const QString& name)
    {
        if (getCVM)
        {
            //当前视口

            if (name == "梁截面")
            {
                VMManager->showPropShapeViewport();
                //禁用菜单栏视口按钮
                if (_homeGW.size() >= 4)
                {
                    _homeGW.at(3)->setEnabled(false);
                }
            }
            else
            {
                VMManager->hidePropShapeViewport();
                if (_homeGW.size() >= 4)
                {
                    _homeGW.at(3)->setEnabled(true);
                }
            }

        }

        if (name.isEmpty())
            return;

        //多体形态守卫：被彻底隐藏的隐式页（模型/网格/材料/属性/相互作用/分析工况/载荷/边界/NVH/优化）
        //不显示、不落动态页签位、不切换当前页——树点击隐式对象时保持多体功能区上下文不漂移
        if (_ribbonForm == MechCommon::MenuRibbonForm::Motion && _motionFormHiddenMenus.contains(name))
        {
            return;
        }

        //请求的页专属另一工作模式时（如曲线工作台入口发“曲线”、后处理功能发“动画”）：
        //先切换到该模式（setWorkMode重建页签集合并落默认页），再继续后续页级切换——菜单切换
        //请求由此具备工作上下文切换语义，弥补渲染视口激活之外的模式切换通道（曲线图表卡片
        //不进渲染视口注册表，曲线树显示/节点选中不会触发视口激活）。三模式共享的“主页”不触发；
        //前后处理双名单页名（主页/显示/选择/模型，同在_residentMenuNames与_postMenuCandidates）
        //同不触发强制切模式：请求按当前模式就近解析（Pre→前处理同名常驻页分支，Post→
        //pageForTabText落#Post独立页）——否则前处理树根单击发射的“模型”
        //（mTreeView::slot_singleClickItem）会被ownedByPost劫持切进后处理模式
        const bool residentOwned = _residentMenuNames.contains(name);
        const bool ownedByCurve = !residentOwned && _curveMenuCandidates.contains(name);
        const bool ownedByPost = !residentOwned && _postMenuCandidates.contains(name);
        if (ownedByCurve && !ownedByPost && _workMode != MenuWorkMode::Curve)
        {
            setWorkMode(MenuWorkMode::Curve);
        }
        else if (ownedByPost && !ownedByCurve && _workMode != MenuWorkMode::Post)
        {
            setWorkMode(MenuWorkMode::Post);
        }

        //Post/Curve模式限定页签集合：已勾选页签正常切换；候选内但未勾选的页签临时插入显示——
        //功能被触发时其控制页必须可达（如动画在播需要“动画”页操纵），临时性体现在不改勾选配置，
        //跨模式切换时setWorkMode按勾选集合重建、临时页签自动消失；候选外请求仍直接忽略。
        //临时页签默认放页签栏最右（见下方addTab）
        if (_workMode != MenuWorkMode::Pre && !visibleTabsForMode(_workMode).contains(name))
        {
            const QStringList order = candidateNamesForMode(_workMode);
            if (!order.contains(name))
                return;

            //已临时插入过则只切换不再插入（每次树点击/功能触发都会重发切换请求，须幂等，
            //否则同名页签越插越多）；未插入时默认放最右——追加到所有常驻页签之后
            //（“常驻区|动态区”分界线由MyTabBar画在首个非常驻页签前），不按候选名单序插中段
            if (findTabByText(name) < 0)
            {
                m_menuTabBar_->addTab(name);

                //临时插入的“结果”/“云图”页签默认enabled，按激活视口重应用启停状态
                refreshPostMenuEnabledState();
            }
            if (mGlobalSignals::getInstance()->getTreeMenuLinkMode() != TreeMenuLinkMode::None)
            {
                const int idx = findTabByText(name);
                if (idx >= 0 && m_menuTabBar_->currentIndex() != idx)
                    m_menuTabBar_->setCurrentIndex(idx);
            }

            //临时页签同一时刻只保留一个：目标页签就位（含切换）后，移除此前临时插入的其他
            //页签（不在勾选集合内的页签即临时页签，页栏上其余页签均来自勾选集合）——否则
            //“云图”临时页签在位时再触发“动画”，两条临时页签并存；先插后切再删，避免删到
            //当前页签引发QTabBar自动选中相邻页签的瞬时页面切换
            const QStringList checkedTabs = visibleTabsForMode(_workMode);
            for (int i = m_menuTabBar_->count() - 1; i >= 0; --i)
            {
                const QString tabText = m_menuTabBar_->tabText(i);
                if (tabText != name && !checkedTabs.contains(tabText))
                    m_menuTabBar_->removeTab(i);
            }
            return;
        }

        //常驻菜单：直接切换到对应固定页签（被“视图”菜单隐藏的常驻菜单不再显示）
        if (_residentMenuNames.contains(name))
        {
            if (mGlobalSignals::getInstance()->getTreeMenuLinkMode() != TreeMenuLinkMode::LinkMenu)
                return;

            const int residentIdx = findTabByText(name);
            if (residentIdx >= 0 && m_menuTabBar_->currentIndex() != residentIdx)
                m_menuTabBar_->setCurrentIndex(residentIdx);
            return;
        }

        //目标页签已在TabBar上（如Post/Curve模式的固定页签、Pre模式已显示的动态页）：直接切换，
        //不走动态页签位换名（防止把“结果”等固定页签改名成其他页）
        const int existingIdx = findTabByText(name);
        if (existingIdx >= 0)
        {
            if (m_menuTabBar_->currentIndex() == existingIdx)
                slot_CurrentTabChanged(existingIdx); //索引未变时手动同步页面（与动态页签位行为一致）
            else if (mGlobalSignals::getInstance()->getTreeMenuLinkMode() != TreeMenuLinkMode::None)
                m_menuTabBar_->setCurrentIndex(existingIdx);
            return;
        }

        //草图期间其他动态菜单（坐标系/截面属性等）占用动态页签位会把"草图"页签顶掉，
        //"结束草图绘制"按钮随之不可达、草图无法退出——顶占前先退出草图（数据按当前模型落盘）。
        //常驻页签切换不经过此处、不影响草图；name为"草图"自身是重进/切回，不退出
        if (_workMode == MenuWorkMode::Pre
            && name != QString::fromUtf8("草图")
            && mGeoSketchSignalManager::instance()->currentStatus())
        {
            mGeoSketchSignalManager::instance()->exitSketch();
            //退出时 sketchExited 已移除"草图"页签，动态页签位空出，
            //后续逻辑将以新增页签方式放置目标菜单
        }

        //非常驻菜单（草图/截面属性/后处理各页等）：显示在最右侧动态页签位
        int dynamicIdx = -1;
        for (int i = 0; i < m_menuTabBar_->count(); ++i)
        {
            if (!_residentMenuNames.contains(m_menuTabBar_->tabText(i)))
            {
                dynamicIdx = i;
                break;
            }
        }
        if (dynamicIdx < 0)
            dynamicIdx = m_menuTabBar_->addTab(name);
        else
            m_menuTabBar_->setTabText(dynamicIdx, name);

        if (m_menuTabBar_->currentIndex() == dynamicIdx)
            slot_CurrentTabChanged(dynamicIdx); //动态页签文字变化但索引未变，手动切换页面
        else if(mGlobalSignals::getInstance()->getTreeMenuLinkMode() != TreeMenuLinkMode::None)
            m_menuTabBar_->setCurrentIndex(dynamicIdx);
    }

    void mMenuWidget::slot_openMaterial()
    {
        m_materialWindow->show();
        m_materialWindow->raise();
    }

    void mMenuWidget::slot_createMaterial()
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMaterial();
    }

    void mMenuWidget::slot_createCase(MxFunctions::CaseType type)
    {
        if (!getCVM)
        {
            return;
        }
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createCase(type);
    }

    void mMenuWidget::slot_createPredefinedFiel(MxFunctions::PredefinedFieldType type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createPredefinedField(type);
    }

    void mMenuWidget::slot_createLoad(MxFunctions::LoadBoundaryType type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createLoad(type);
    }

    void mMenuWidget::slot_openCustomMaterial()
    {
        emit MechCommon::mMechGlobalSignals::getInstance()->loadCustomDepotSig();
        m_customMaterialWindow->show();
        m_customMaterialWindow->raise();
    }

    void mMenuWidget::slot_FrpCompMaterial()
    {
        //初始化复合纤维增强弹窗
        if (m_FrpCompMaterialWindow)
        {
            m_FrpCompMaterialWindow->show();
            m_FrpCompMaterialWindow->raise();
        }
        else
        {
            m_FrpCompMaterialWindow = new mFrpCompMaterialWindow(this);
            m_FrpCompMaterialWindow->show();
            m_FrpCompMaterialWindow->raise();
        }
    }

	void mMenuWidget::slot_createBoundary(MxFunctions::LoadBoundaryType type)
	{
		mInfoWindow::getInstance()->unEditInfoWindow();
		mInfoWindow::getInstance()->createBoundary(type);
	}

    void mMenuWidget::slot_createRotateAnalyse(MxFunctions::RotateAnalysyType type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createRotateAnalyse(type);
    }

    void mMenuWidget::slot_createConnector(MxFunctions::ConnectorType type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createConnector(type);
    }

    void mMenuWidget::slot_createDeputy(MxFunctions::DeputyType type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createDeputy(type);
    }

    void mMenuWidget::slot_createContact(MxFunctions::ContactType type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createContact(type);
    }

    void mMenuWidget::slot_createElementSetCtrl()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createElementSetCtrl();
    }

    void mMenuWidget::slot_createNonStructuralQuality()
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createNonStructuralQuality();
    }

    void mMenuWidget::slot_createDiscreteFieldNoType()
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createDiscreteField();
    }

    void mMenuWidget::slot_createDiscreteField(MxFunctions::DiscreteSpace spaceType)
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createDiscreteField(spaceType);
    }

    void mMenuWidget::slot_createAnalysisField()
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createAnalysisField();
    }

    void mMenuWidget::slot_createSet()
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        deferCreateSet(QString());
    }

    void mMenuWidget::slot_createSet_withName(QString setName)
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        deferCreateSet(setName);
    }

    void mMenuWidget::deferCreateSet(const QString& setName)
    {
        //创建集合的完整链路（unEditInfoWindow 整体卸下面板 -> 重挂 -> 折叠框5ms延时刷新
        //-> okSet 同步回树触发 editSet 再次卸下面板重挂）虽是同步执行，但收尾依赖
        //事件循环中的定时器与动画回调；连点会让下一轮卸载/重挂插进上一轮收尾的空档，
        //被 setParent(nullptr) 的控件此时被显示即成独立浮窗。静置期内仅保留最后一次请求。
        if (m_createSetDebounceTimer == nullptr)
        {
            m_createSetDebounceTimer = new QTimer(this);
            m_createSetDebounceTimer->setSingleShot(true);
            connect(m_createSetDebounceTimer, &QTimer::timeout, this, [this]()
            {
                if (!m_hasPendingCreateSet)
                {
                    return;
                }
                m_hasPendingCreateSet = false;
                mInfoWindow::getInstance()->unEditInfoWindow();
                if (m_pendingCreateSetName.isEmpty())
                {
                    mInfoWindow::getInstance()->createSet();
                }
                else
                {
                    mInfoWindow::getInstance()->createSet(m_pendingCreateSetName);
                }
            });
        }
        m_pendingCreateSetName = setName;
        m_hasPendingCreateSet = true;
        m_createSetDebounceTimer->start(250);
    }

    void mMenuWidget::slot_createStrual()
    {
        mInfoWindow::getInstance()->createOptimization();
    }

    void mMenuWidget::slot_createRunnerStrual()
    {
        mInfoWindow::getInstance()->createRunnerOptimization();
    }

    void mMenuWidget::slot_createSection(int type)
    {
        mSectionWindow::getInstance()->createSection(type);
    }

    void mMenuWidget::slot_createFREQ()
    {
        mInfoWindow::getInstance()->creatFREQ();
    }

    void mMenuWidget::slot_createCoordinate()
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createCoordinate();
    }

    void mMenuWidget::slot_createMyPanel()
    {
#ifdef _RELEASE
        // 该槽函数禁止在Release下使用，以防外发
        assert(0);
#endif // _RELEASE
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMyPanel();

    }

    void mMenuWidget::slot_createFunction(int type)
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createFunction(type);
    }

    void mMenuWidget::slot_createSectionShape(int type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createSectionShape(type);
    }

    void mMenuWidget::slot_startSolver()
    {
        if (getCVM->isPostModel())
        {
            return;
        }
        vector<int> caseIDs = getCVM->getModelData()->getAllPhyID(Physical_Case);
        if (caseIDs.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig("无分析工况可求解！");
            return;
        }

        mSolveTaskCenter::getInstance()->quickSolve(getCVM->getModelData()->getModelDataID(), caseIDs[0]);
    }

    void mMenuWidget::slot_cmdExecute()
    {
        disconnect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::threadExecuteFinishSig, this, &mMenuWidget::slot_cmdExecute);
        slot_startSolver();
    }

    void mMenuWidget::slot_New()
    {
        // mpcomm 平台模式下仅允许单模型，禁止新建
        if (MechCommon::mMechGlobalSignals::getInstance()->_isPlatformMode)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("平台模式下仅支持单模型，禁止新建。"));
            return;
        }
        mViewTreeFunction::newViewportModel();
        mBrowserWindow::getInstance()->getBrowserTree()->addModelBrowserTree();
    }
    void mMenuWidget::slot_NVHCurve()
    {
        PostCurve::mOpenCurveDlg::getInstance()->show();  //【集成修正】PostCurve 命名空间限定
    }


    void mMenuWidget::slot_openFile()
    {
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }
        QString openFile = QFileDialog::getOpenFileName(this
            , "打开文件"
            , defaultDir
            , QString("MxSim File(*.mxcae)"));

        if (openFile.isEmpty())
        {
            return;
        }
        int index = openFile.lastIndexOf("/");
        MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = openFile.left(index);

        //打开文件前清空当前模型（数据/视口/模型树全部还原），保留当前模型名称与ID
        mBrowserWindow::getInstance()->getBrowserTree()->clearCurrentModelData();

        //打开mxcae文件
        MechIOFile::mIOFileManager::getInstance()->readMxcaeFile(openFile);
        MAppConfiguration::mAppConfiguration::getInstance()->appendRecentMxCaeFile(openFile);
    }

	bool mMenuWidget::slot_saveFile()
	{
		bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
		if (hasThread)
		{
			mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
			return false;
		}

        QString savepath = getCVM->getMxcaePath();
        //判断当前工程文件路径是否为空，若为空，则选择保存路径
        if (savepath.isEmpty())
        {
            QString workingdir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
            QString breeName = mBrowserWindow::getInstance()->getBrowserTree()->getModelBrowserTreeName();
            if (!breeName.isEmpty())
            {
                workingdir = workingdir + "/" + breeName;
            }
            savepath = QFileDialog::getSaveFileName(this,
                "保存文件",
                workingdir,
                QString("MxSim File(*.mxcae)"));

            if (savepath.isEmpty())
            {
                return false;
            }
            getCVM->setMxcaePath(savepath);
        }

        //保存文件
        MechIOFile::mIOFileManager::getInstance()->writeMxcaeFile(savepath);
        return true;
    }

    void mMenuWidget::slot_saveAsFile()
    {
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法保存该模型，请选择前处理模型！"));
            return;
        }

        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        QString workingdir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QString savepath = QFileDialog::getSaveFileName(this,
            "保存文件",
            workingdir,
            QString("MxSim File(*.mxcae)"));

        if (savepath.isEmpty())
        {
            return;
        }
        getCVM->setMxcaePath(savepath);
        MechIOFile::mIOFileManager::getInstance()->writeMxcaeFile(savepath);
    }

    void mMenuWidget::slot_saveMxsesFile()
    {
        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        QString workingdir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QString savepath = QFileDialog::getSaveFileName(this,
            "保存会话文件",
            workingdir,
            QString("MxSim Session(*.mxses)"));
        if (savepath.isEmpty())
        {
            return;
        }
        mMxsesRestore::saveSession(savepath);
    }

    void mMenuWidget::slot_openMxsesFile()
    {
        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QString openFile = QFileDialog::getOpenFileName(this,
            "打开会话文件",
            defaultDir,
            QString("MxSim Session(*.mxses)"));
        if (openFile.isEmpty())
        {
            return;
        }
        //打开会话将清空当前全部数据，先询问是否保存当前内容
        if (!confirmSaveMxsesBeforeDiscard(QString("打开会话文件将清空当前所有数据，是否先保存？")))
        {
            return;
        }
        mMxsesRestore::openSession(openFile);
    }

    void mMenuWidget::slot_importMxFile()
    {
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入该模型，请选择前处理模型！"));
            return;
        }

        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = R"(All Files(*.mx *.bdf *.nas *.fem *.cdb *.inp);;
								MxSim(*.mx);;
								Nastran(*.bdf);;
								Nastran(*.nas);;
								OptiStruct(*.fem);;
								Ansys(*.cdb);;
								Abaqus(*.inp)
							   )";
        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("导入计算模型"),
            defaultDir,
            filesuffix
        );

        importMxFile(fileName);
    }
    // mpcomm：几何导入完成统一回报点（几何导入会经历 导入→更新模型树→渲染 三步，
    // 在调用点 emit 时数据可能尚未就绪，故挂在 importGeoModelRenderFinished 上）
    void mMenuWidget::reportMpcommGeometryOnRenderFinished(const QString& filePath)
    {
        if (getCVM == nullptr || getCVM->getPreRenderWidget() == nullptr)
            return;
        // 该信号每次导入都会发且导入入口不止一个：若上次导入挂的回报 lambda
        // 因导入失败未触发而残留，下次会连同本次一起触发造成重复回报，
        // 所以每次挂接前清掉自己挂过的所有旧连接（仅断开指向 this 的，
        // 不影响其他模块在同一信号上的连接）。
        disconnect(getCVM->getPreRenderWidget(), &CCSMRendWidget::importGeoModelRenderFinished, this, nullptr);
        connect(getCVM->getPreRenderWidget(), &CCSMRendWidget::importGeoModelRenderFinished, this,
            [this, filePath]()
            {
                emit MechCommon::mMechGlobalSignals::getInstance()->mpcommImportGeometryFinished(filePath);
                });
    }

    // mpcomm 平台触发：导入指定几何文件（不回传 committed，由桥接回声抑制）
    void mMenuWidget::slot_mpcommImportGeometry(const QString& filePath)
    {
        mGlobalSignals::getInstance()->outputMessageSig(QString("[mpcomm] geometry import requested: %1").arg(filePath));

        if (filePath.isEmpty())
            return;
        if (getCVM == nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("[mpcomm] geometry import failed: 当前视口模型为空"));
            return;
        }
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入该模型，请选择前处理模型！"));
            return;
        }
        if (!QFile::exists(filePath))
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("几何文件不存在：%1").arg(filePath));
            return;
        }

        // 走新架构统一几何导入入口（与手动"导入几何模型"同链路：含 CAD 格式
        // 转换、就绪校验、最近打开登记与导入后 FitView）。平台触发的导入完成
        // 后同样回传 committed（与手动导入一致，平台侧对同值重复提交有幂等处理）
        reportMpcommGeometryOnRenderFinished(filePath);

        mGlobalSignals::getInstance()->outputMessageSig(QString("[mpcomm] geometry importing: %1").arg(filePath));

        APIServer::APIParams params;
        params.insert("filePath", { filePath });
        QString strErr;
        const QVariant importResult =
            mGeoPythonAPIRegistrar::instance()->onImportGeoFile(params, strErr);
        if (!importResult.toBool())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("[mpcomm] geometry import failed: %1").arg(strErr));
        }
    }

    // mpcomm 平台触发：导入指定计算文件(.mx)（不回传 committed，由桥接回声抑制）
    void mMenuWidget::slot_mpcommImportCalcFile(const QString& filePath)
    {
        mGlobalSignals::getInstance()->outputMessageSig(QString("[mpcomm] calc file import requested: %1").arg(filePath));

        if (filePath.isEmpty())
            return;
        if (getCVM == nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("[mpcomm] calc file import failed: 当前视口模型为空"));
            return;
        }
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入该模型，请选择前处理模型！"));
            return;
        }
        if (!QFile::exists(filePath))
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("计算文件不存在：%1").arg(filePath));
            return;
        }

        // 重导计算文件前清掉既有工况：.mx 自带 *subcase 工况数据，
        // 若保留之前按 templateId 创建的空工况，会出现"空工况在前、文件工况在后"
        // 两个工况并存，求解/设置面板默认取第一个（空的）导致设置显示无数据。
        vector<int> caseIDs = getCVM->getModelData()->getAllPhyID(Physical_Case);
        for (int caseID : caseIDs)
        {
            //删除面板
            mInfoWindow::getInstance()->deleteCase(caseID);
            //删除数据（重构后数据层删除入口为 deleteByPhysicalType）
            getCVM->getModelData()->deleteByPhysicalType(Physical_Case, caseID);
        }

        MechIOFile::mIOFileManager::getInstance()->slot_importSolveFile(filePath);
    }

    // mpcomm 平台触发：接收到上游场数据（mpif），执行点到点映射示例
    void mMenuWidget::slot_mpcommImportMpif(const QString& filePath)
    {
        mGlobalSignals::getInstance()->outputMessageSig(QString("[mpcomm] mpif received: %1").arg(filePath));
        if (filePath.isEmpty())
            return;
        runMpifMappingDemo(filePath);
    }

    void mMenuWidget::slot_openMotionLoadImport(const QString& filePath)
    {
        // 若平台曾下发 coupling 文件且未指定路径，则预填第一个 pending coupling
        QString path = filePath;
        if (path.isEmpty()) {
            const QStringList pending = qApp->property("mpcomm_pending_coupling").toStringList();
            if (!pending.isEmpty())
                path = pending.first();
        }
        mInfoWindow::getInstance()->openMotionLoadImport(path);
    }

    void mMenuWidget::runMpifMappingDemo(const QString& filePath)
    {
        if (filePath.isEmpty()) {
            mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("[mpif-demo] 空文件路径"));
            return;
        }

        // ================= 第一步：解析文档，打印全部 set 及变量清单 =================
        const auto document = ftc::read_mpif_file(filePath.toLocal8Bit().constData());
        if (!document) {
            QString diagnostics;
            if (const auto* list = document.diagnostics_if()) {
                for (const auto& d : list->items()) {
                    if (!diagnostics.isEmpty()) diagnostics += "; ";
                    diagnostics += QString::fromStdString(ftc::error_code_name(d.code));
                    diagnostics += ": ";
                    diagnostics += QString::fromStdString(d.message);
                }
            }
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("[mpif-demo] 读取失败: %1; %2").arg(filePath).arg(diagnostics));
            return;
        }
        const ftc::MpifDocument& doc = *document.value_if();

        const auto sets = doc.sets();
        mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("[mpif-demo] ========== 第一步：解析 =========="));
        mGlobalSignals::getInstance()->outputMessageSig(QString("[mpif-demo] 文件: %1 set 数: %2").arg(filePath).arg(sets.size()));

        // 源 set 选择规则：优先找带变量的（先 NODE，再 FACE，再 CELL），找不到再退回第一个非空集合
        std::size_t chosenIdx = sets.size();
        ftc::EntityKind chosenKind = ftc::EntityKind::Node;
        auto pickVariableSource = [&]() {
            for (std::size_t i = 0; i < sets.size(); ++i) {
                const auto svOpt = doc.find_set_by_id(sets[i].id());
                if (!svOpt.has_value()) continue;
                const ftc::SetView& sv = *svOpt;
                if (!sv.nodes().variables.empty() && !sv.nodes().records.empty()) { chosenIdx = i; chosenKind = ftc::EntityKind::Node; return; }
                if (!sv.faces().variables.empty() && !sv.faces().records.empty()) { chosenIdx = i; chosenKind = ftc::EntityKind::Face; return; }
                if (!sv.cells().variables.empty() && !sv.cells().records.empty()) { chosenIdx = i; chosenKind = ftc::EntityKind::Cell; return; }
            }
            for (std::size_t i = 0; i < sets.size(); ++i) {
                const auto svOpt = doc.find_set_by_id(sets[i].id());
                if (!svOpt.has_value()) continue;
                const ftc::SetView& sv = *svOpt;
                if (!sv.nodes().records.empty()) { chosenIdx = i; chosenKind = ftc::EntityKind::Node; return; }
                if (!sv.faces().records.empty()) { chosenIdx = i; chosenKind = ftc::EntityKind::Face; return; }
                if (!sv.cells().records.empty()) { chosenIdx = i; chosenKind = ftc::EntityKind::Cell; return; }
            }
        };
        pickVariableSource();

        for (std::size_t i = 0; i < sets.size(); ++i) {
            // SetView 构造是私有的，经 MpifDocument 公共接口取视图
            const auto svOpt = doc.find_set_by_id(sets[i].id());
            if (!svOpt.has_value()) continue;
            const ftc::SetView& sv = *svOpt;
            mGlobalSignals::getInstance()->outputMessageSig(QString("[mpif-demo]   set[%1] id=%2 name=%3 节点=%4 面=%5 单元=%6")
                .arg(i).arg(sv.id()).arg(QString::fromStdString(std::string(sv.name())))
                .arg(sv.nodes().records.size())
                .arg(sv.faces().records.size())
                .arg(sv.cells().records.size()));
            for (const auto& v : sv.nodes().variables) {
                mGlobalSignals::getInstance()->outputMessageSig(QString("[mpif-demo]     变量(NODE): %1 分量数=%2")
                    .arg(QString::fromStdString(v.name)).arg(v.component_count));
            }
            for (const auto& v : sv.faces().variables) {
                mGlobalSignals::getInstance()->outputMessageSig(QString("[mpif-demo]     变量(FACE): %1 分量数=%2")
                    .arg(QString::fromStdString(v.name)).arg(v.component_count));
            }
            for (const auto& v : sv.cells().variables) {
                mGlobalSignals::getInstance()->outputMessageSig(QString("[mpif-demo]     变量(CELL): %1 分量数=%2")
                    .arg(QString::fromStdString(v.name)).arg(v.component_count));
            }
        }
        if (sets.empty() || chosenIdx >= sets.size()) {
            mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("[mpif-demo] 文档无 set 或全部为空，示例结束"));
            return;
        }

        // ================= 第二步：取选中的 set + 第一个变量，造目标点 =================
        const auto setViewOpt = doc.find_set_by_id(sets[chosenIdx].id());
        if (!setViewOpt.has_value()) {
            mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("[mpif-demo] 选中 set 视图获取失败，示例结束"));
            return;
        }
        const ftc::SetView& setView = *setViewOpt;

        // 按实体类型选 section 与变量；源坐标优先用节点坐标，FACE/CELL 时用节点包围盒近似
        const ftc::NodeSectionData& nodeSection = setView.nodes();
        const ftc::FaceSectionData& faceSection = setView.faces();
        const ftc::CellSectionData& cellSection = setView.cells();
        const ftc::VariableDescriptor* chosenVar = nullptr;
        std::size_t recordCount = 0;
        switch (chosenKind) {
        case ftc::EntityKind::Node:
            if (!nodeSection.variables.empty() && !nodeSection.records.empty()) {
                chosenVar = &nodeSection.variables[0];
                recordCount = nodeSection.records.size();
            }
            break;
        case ftc::EntityKind::Face:
            if (!faceSection.variables.empty() && !faceSection.records.empty()) {
                chosenVar = &faceSection.variables[0];
                recordCount = faceSection.records.size();
            }
            break;
        case ftc::EntityKind::Cell:
            if (!cellSection.variables.empty() && !cellSection.records.empty()) {
                chosenVar = &cellSection.variables[0];
                recordCount = cellSection.records.size();
            }
            break;
        }
        if (chosenVar == nullptr || recordCount == 0) {
            mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("[mpif-demo] 选中 set 无可用变量或记录，示例结束"));
            return;
        }
        const std::string variableName = chosenVar->name;
        mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("[mpif-demo] ========== 第二步：选择 =========="));
        mGlobalSignals::getInstance()->outputMessageSig(QString("[mpif-demo] 选择 set id=%1 变量=%2 实体类型=%3")
            .arg(setView.id())
            .arg(QString::fromStdString(variableName))
            .arg(chosenKind == ftc::EntityKind::Node ? "NODE" : chosenKind == ftc::EntityKind::Face ? "FACE" : "CELL"));

        // 源 set 节点坐标包围盒（作为目标点生成区域；FACE/CELL 变量时仍用节点包围盒）
        double minX = std::numeric_limits<double>::max(), minY = minX, minZ = minX;
        double maxX = std::numeric_limits<double>::lowest(), maxY = maxX, maxZ = maxX;
        for (const auto& rec : nodeSection.records) {
            minX = std::min(minX, rec.coordinates[0]); maxX = std::max(maxX, rec.coordinates[0]);
            minY = std::min(minY, rec.coordinates[1]); maxY = std::max(maxY, rec.coordinates[1]);
            minZ = std::min(minZ, rec.coordinates[2]); maxZ = std::max(maxZ, rec.coordinates[2]);
        }
        const double cx = (minX + maxX) / 2.0, cy = (minY + maxY) / 2.0, cz = (minZ + maxZ) / 2.0;
        const double dx = std::max(maxX - minX, 1e-6), dy = std::max(maxY - minY, 1e-6), dz = std::max(maxZ - minZ, 1e-6);

        // 6 个目标点：域内 3（中心/四分之一/靠近 min 角）、域外 2（沿 +X/+Y 超出 20%）、
        // 精确命中 1（第一个源节点的坐标）
        ftc::TargetLocationSetData targetData;
        targetData.kind = ftc::TargetLocationKind::Point;
        targetData.location_ids = { 1, 2, 3, 4, 5, 6 };
        targetData.coordinates = {
            cx, cy, cz,                                                    // 1 域内：中心
            minX + dx * 0.25, minY + dy * 0.25, minZ + dz * 0.25,          // 2 域内：四分之一
            minX + dx * 0.05, minY + dy * 0.05, minZ + dz * 0.05,          // 3 域内：靠近 min 角
            maxX + dx * 0.2, cy, cz,                                       // 4 域外：+X 超出
            cx, maxY + dy * 0.2, cz,                                       // 5 域外：+Y 超出
            nodeSection.records[0].coordinates[0],                          // 6 精确命中源节点
            nodeSection.records[0].coordinates[1],
            nodeSection.records[0].coordinates[2]
        };
        auto targetSet = ftc::TargetLocationSet::create(std::move(targetData));
        if (!targetSet) {
            mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("[mpif-demo] 目标点集创建失败"));
            return;
        }
        mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("[mpif-demo] 目标点: 6 个 (域内 3 / 域外 2 / 精确命中 1)"));

        // ================= 第三步：Nearest 映射，打印结果 =================
        ftm::MappingOptions options;
        options.source_kind = chosenKind;
        options.method = ftm::MappingMethod::Nearest;
        options.outside_domain_policy = ftm::OutsideDomainPolicy::Nearest;
        auto plan = ftm::MappingPlan::prepare(setView, targetSet.value_if()->view(), options);
        if (!plan) {
            mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("[mpif-demo] MappingPlan::prepare 失败"));
            return;
        }
        ftm::FieldBinding binding;
        binding.source_name = variableName;
        binding.target_name = variableName;
        auto output = plan.value_if()->apply_field(setView, binding);
        if (!output) {
            mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("[mpif-demo] apply_field 失败"));
            return;
        }
        const ftm::MappingOutput& out = *output.value_if();

        mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("[mpif-demo] ========== 第三步：映射结果 =========="));
        const auto fieldResult = out.fields().find_field(variableName);
        if (!fieldResult) {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("[mpif-demo] 结果中未找到变量: %1").arg(QString::fromStdString(variableName)));
            return;
        }
        const auto& field = *fieldResult.value_if();
        const auto locIds = field.location_ids();
        const auto values = field.values();
        const auto status = out.report().location_status();
        const auto targetCoords = out.target_locations();
        const int compCount = field.component_count();

        for (std::size_t i = 0; i < locIds.size(); ++i) {
            QString valueStr;
            for (int c = 0; c < compCount; ++c) {
                if (c > 0) valueStr += ",";
                valueStr += QString::number(values[i * compCount + c]);
            }
            const char* statusName = "Unknown";
            if (i < status.size()) {
                statusName = ftm::location_mapping_status_name(status[i]);
            }
            mGlobalSignals::getInstance()->outputMessageSig(QString("[mpif-demo]   目标点 id=%1 (%2,%3,%4) -> 值=(%5) 状态=%6")
                .arg(locIds[i])
                .arg(targetCoords.coordinates()[i * 3 + 0])
                .arg(targetCoords.coordinates()[i * 3 + 1])
                .arg(targetCoords.coordinates()[i * 3 + 2])
                .arg(valueStr)
                .arg(statusName));
        }
        // 覆盖率 = 直接映射成功数 / 目标点总数（MappingQualitySummary::coverage
        // 是 prepare 期审计口径，这里按结果行数直观算）
        const double coverage = out.report().target_count() > 0
            ? static_cast<double>(out.report().mapped_count())
                / static_cast<double>(out.report().target_count()) * 100.0
            : 0.0;
        mGlobalSignals::getInstance()->outputMessageSig(QString("[mpif-demo] 统计: 目标点 %1, 映射成功 %2, 覆盖率 %3%")
            .arg(out.report().target_count())
            .arg(out.report().mapped_count())
            .arg(coverage, 0, 'f', 1));
        mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("[mpif-demo] 示例完成"));
    }

    // mpcomm 平台触发：按 templateId 判定分析类型并创建对应工况
    void mMenuWidget::slot_mpcommCreateCaseByTemplateId(const QString& templateId)
    {
        const QString tid = templateId.trimmed();

        // 平台 templateId -> 结构工况类型
        static const QHash<QString, MxFunctions::CaseType> templateToCase = {
            { QStringLiteral("analysis.static_structural"),     MxFunctions::ImplicitLinearStatic },
            { QStringLiteral("analysis.transient_structural"),  MxFunctions::ImplicitTransient },
            { QStringLiteral("analysis.modal"),                 MxFunctions::ImplicitModal },
            { QStringLiteral("analysis.harmonic_response"),     MxFunctions::ImplicitFrequency },
            { QStringLiteral("analysis.response_spectrum"),     MxFunctions::ImplicitResponseSpectrum },
            { QStringLiteral("analysis.random_vibration"),      MxFunctions::ImplicitRandomVibration },
            { QStringLiteral("analysis.buckling"),              MxFunctions::ImplicitLinearBuckling },
        };

        const auto it = templateToCase.constFind(tid);
        if (it == templateToCase.constEnd())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("[mpcomm] 未识别的分析类型 templateId：%1").arg(tid));
            return;
        }

        mGlobalSignals::getInstance()->outputMessageSig(QString("[mpcomm] create case by templateId: %1").arg(tid));
        slot_createCase(it.value());
    }

    void mMenuWidget::importMxFile(const QString& fileName)
    {
        if (!fileName.isEmpty()) {
            QFileInfo fileinfo(fileName);
            QString suffix = fileinfo.suffix();
            if (suffix == "mx")
            {
                MechIOFile::mIOFileManager::getInstance()->slot_importSolveFile(fileName);
            }
            else
            {
                m_transFileInfoDialog->transMxFile(fileName);
            }
            //加入最近打开
            MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
            auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
            QSettings Settings(path, QSettings::IniFormat);

            // 添加一条“最近打开”
            QString file = fileName;
            QString key = QString("recent/%1").arg(QDateTime::currentMSecsSinceEpoch());
            QString num = key.split('/').last();//时间戳

            Settings.beginGroup("recent");
            for (auto k : Settings.childKeys())
            {
                if (Settings.value(k) == fileName)
                    Settings.remove(QString("%1").arg(k));
            }
            Settings.endGroup();
            Settings.setValue(key, file);
        }
    }
    void mMenuWidget::slot_importMotionJsonFile()
    {
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入运动数据，请选择前处理模型！"));
            return;
        }

        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = "Motion JSON(*.motion.json)";
        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("导入运动数据"),
            defaultDir,
            filesuffix
        );

        if (!fileName.isEmpty()) {
            MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
            MechIOFile::mIOFileManager::getInstance()->slot_importMotionJsonFile(fileName);
        }
    }

    void mMenuWidget::slot_exportMxFile()
    {
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导出该模型，请选择前处理模型！"));
            return;
        }

        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = R"(MxSim(*.mx))";
        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("导出计算模型"),
            defaultDir,
            filesuffix
        );

        if (!fileName.isEmpty()) {
            MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
            MechIOFile::mIOFileManager::getInstance()->slot_exportSolveFile(fileName);
        }
    }

    void mMenuWidget::slot_exportMotionJsonFile()
    {
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导出运动数据，请选择前处理模型！"));
            return;
        }

        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = "Motion JSON(*.motion.json)";
        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("导出运动数据"),
            defaultDir,
            filesuffix
        );

        if (!fileName.isEmpty()) {
            MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
            MechIOFile::mIOFileManager::getInstance()->slot_exportMotionJsonFile(fileName);
        }
    }

    void mMenuWidget::slot_exportGeoFile()
    {
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导出该模型，请选择前处理模型！"));
            return;
        }

        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = R"(STP(*.stp);;
								STEP(*.step);;
								IGES(*.iges);;
								IGS(*.igs))";
        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("导出几何模型"),
            defaultDir,
            filesuffix
        );

        if (!fileName.isEmpty()) {
            MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
            getCVM->getModelData()->getGeoManager()->slotExportGeoFiles(fileName);

            // mpcomm：手动导出几何完成 → 回传 committed(geometry)。
            // （导出是异步 TaskManager 任务，无完成信号；此处按"派发即成功"
            //  上报，失败时几何管理器只输出消息不通知调用方，平台侧会拿到
            //  一个不存在的路径——可接受，导出失败的场景平台行保持原状）
            emit MechCommon::mMechGlobalSignals::getInstance()
                ->mpcommExportGeometryFinished(fileName);
        }
    }

    void mMenuWidget::slot_importInheritanceGeoFile()
    {
        emit MechCommon::mMechGlobalSignals::getInstance()->updateStatusImportGeo();
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入该模型，请选择前处理模型！"));
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = R"(All Files(*.step *.iges *.stp *.igs *.obj *.brep *.stl *.emn);;
								STEP(*.step *.stp);;
								IGES(*.iges *.igs);;
								BREP(*.brep);;
								OBJ(*.obj);;
								STL(*.stl);;
								IDF(*.emn);;
		                              PRT(*.prt);;
		                              X_T(*.x_t)
							   )";

        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("导入几何模型"),
            defaultDir,
            filesuffix

        );

        if (!fileName.isEmpty())
        {
            //记录所有维度几何的最小id
            auto pointIds = getGEOServer.geoCalculate().getAllGeoID(0); //【引擎化】dim0=点
            auto lineIds = getGEOServer.geoCalculate().getAllGeoID(1); //【引擎化】dim1=线
            auto faceIds = getGEOServer.geoCalculate().getAllGeoID(2); //【引擎化】dim2=面
            auto solidIds = getGEOServer.geoCalculate().getAllGeoID(3); //【引擎化】dim3=体

            int pointMinId = 0;
            int lineMinId = 0;
            int faceMinId = 0;
            int solidMinId = 0;

            if (pointIds.size() > 0)
            {
                pointMinId = *pointIds.begin();
            }
            if (lineIds.size() > 0)
            {
                lineMinId = *lineIds.begin();
            }
            if (faceIds.size() > 0)
            {
                faceMinId = *faceIds.begin();
            }
            if (solidIds.size() > 0)
            {
                solidMinId = *solidIds.begin();
            }

            if (pointMinId || lineMinId || faceMinId || solidMinId)
            {
                //删除所有原部件
                std::vector<int> partIds; { auto* pm = getCVM->getModelData()->getGeoManager()->getGeoPartMgr(); if (pm) for (int pid : pm->getPartList()) partIds.push_back(pid); } //【引擎化】GN 部件枚举
                mMechGlobalSignals::getInstance()->deleteGeoTree(std::set<int>{partIds.begin(), partIds.end()});

                // 设置全局点线面体的id为最小id-1，(暂不考虑id不连续的场景)
                auto geoData = getCVM->getModelData()->getGeoData();
                MxFunctions::_globalPointId = pointMinId ? (pointMinId - 1) : 0;
                MxFunctions::_globalLineId = lineMinId ? (lineMinId - 1) : 0;
                MxFunctions::_globalFaceId = faceMinId ? (faceMinId - 1) : 0;
                MxFunctions::_globalSolidId = solidMinId ? (solidMinId - 1) : 0;

                //删除所有渲染符号
                auto deputyIds = getCVM->getModelData()->getAllPhyID(Physical_Deputy);
                auto connectorIds = getCVM->getModelData()->getAllPhyID(Physical_Connector);
                auto contactIds = getCVM->getModelData()->getAllPhyID(Physical_Contact);
                auto lbdIds = getCVM->getModelData()->getAllPhyID(Physical_LoadBoundary);

                std::vector<QString> symbolNames = {};

                for (const auto id : deputyIds)
                {
                    auto name = getCVM->getModelData()->getRenderNameByTypeId(Physical_Deputy, id);
                    symbolNames.push_back(name);
                }

                for (const auto id : connectorIds)
                {
                    auto name = getCVM->getModelData()->getRenderNameByTypeId(Physical_Connector, id);
                    symbolNames.push_back(name);
                }

                for (const auto id : contactIds)
                {
                    auto name = getCVM->getModelData()->getRenderNameByTypeId(Physical_Contact, id);
                    symbolNames.push_back(name);
                }

                for (const auto id : lbdIds)
                {
                    auto name = getCVM->getModelData()->getRenderNameByTypeId(Physical_LoadBoundary, id);
                    symbolNames.push_back(name);
                }

                getCVM->getPreRenderWidget()->GetSymbolController()->deleteSymbol({ symbolNames.begin(),symbolNames.end() });
                getCVM->getPreRenderWidget()->GetSymbolController()->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();
            }

            //开始导入部件，如果导入失败无法复原上一步操作
            mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
            auto modeldata = getCVM->getModelData();
            auto geomanager = modeldata->getGeoManager();
            // mpcomm：几何导入完成后回传 committed(geometry)
            reportMpcommGeometryOnRenderFinished(fileName);
            geomanager->slotImportGeoFiles(fileName);
            //加入最近打开
            MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
            auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
            QSettings Settings(path, QSettings::IniFormat);
            // 添加一条“最近打开”
            QString file = fileName;
            QString key = QString("recent/%1").arg(QDateTime::currentMSecsSinceEpoch());//当前时间距离 1970-01-01 00:00:00 UTC 的毫秒数（int64）

            Settings.beginGroup("recent");
            for (auto k : Settings.childKeys())
            {
                if (Settings.value(k) == fileName)
                    Settings.remove(QString("%1").arg(k));
            }
            Settings.endGroup();
            Settings.setValue(key, file);

            connect(getCVM->getPreRenderWidget(), &CCSMRendWidget::importGeoModelRenderFinished, [=]()
                {
                    QTimer::singleShot(2000, this, [=]() {
                        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
                        QString num = key.split('/').last();
                        getCVM->modelFastCapture(fileName, num);
                        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
                        if (viewportPrivate == nullptr)
                        {
                            return;
                        }
                        mBaseRend* rend = viewportPrivate->getRendWidget();
                        if (rend == nullptr)
                        {
                            return;
                        }


                        rend->FitView();
                        VMManager->setCameraSyncTimer(500, 30, [rend]() {
                            emit rend->signal_CameraSync();
                            });

                        //disconnect(getCVM->getPreRenderWidget(), &CCSMRendWidget::importGeoModelRenderFinished, nullptr, nullptr);
                        });
                });
        }
    }

    QString mMenuWidget::geoImportFileFilter()
    {
        // 导入格式清单按几何后端与 CGM 交换开关组合发放（后端宏由根 CMakeLists
        // 依 RebuildGeometry 导出的路由 GEOENGINE_SELECTED_BACKEND_KIND 发放，
        // HAS_CGM_INTEROP 由本工程 CGM_VERSION≠None 发放）：
        //  - CGM 后端：STEP/IGES + 九种商用 CAD 直读（InterOp Direct Suite，
        //    BREP/OBJ/STL/IDF 不支持，需 OCC 后端）
        //  - OCC 后端 + CGM 交换启用：OCC 能力清单（STEP/IGES/BREP/IDF）+ 九种
        //    商用 CAD（convertToStepIfNeeded 经 CGMDataExchange 转 STEP 后导入）
        //  - OCC 后端（默认）：STEP/IGES/BREP/IDF
        // 后端宏缺失（旧安装树/未知后端）时按 OCC 清单回退，与 FindGeometry
        // 的路由回退语义一致。.prt/.asm 在多个 CAD 间有扩展名歧义，
        // 实际格式由 CGM InterOp 按文件内容识别。
        // 菜单栏导入与树视图右键导入共用本过滤器，两处清单保持一致。
#if defined(MXGEO_BACKEND_CGM) && MXGEO_BACKEND_CGM
        return R"(All Files(*.step *.stp *.iges *.igs *.catpart *.catproduct *.ipt *.iam *.sldprt *.sldasm *.prt *.asm *.x_t *.x_b *.par *.psm);;
        STEP(*.step *.stp);;
        IGES(*.iges *.igs);;
        CATIA V5(*.catpart *.catproduct);;
        Autodesk Inventor(*.ipt *.iam);;
        SOLIDWORKS零件(*.prt *.sldprt);;
        SOLIDWORKS装配体(*.asm *.sldasm);;
        Parasolid(*.x_t *.x_b);;
        Solid Edge(*.par *.psm *.asm);;
        Unigraphics/NX(*.prt);;
        Creo Elements/ProE(*.prt *.asm)
          )";
#elif defined(HAS_CGM_INTEROP)
        return R"(All Files(*.step *.stp *.iges *.igs *.brep *.emn *.catpart *.catproduct *.ipt *.iam *.sldprt *.sldasm *.prt *.asm *.x_t *.x_b *.par *.psm);;
        STEP(*.step *.stp);;
        IGES(*.iges *.igs);;
        BREP(*.brep);;
        IDF(*.emn);;
        CATIA V5(*.catpart *.catproduct);;
        Autodesk Inventor(*.ipt *.iam);;
        SOLIDWORKS零件(*.prt *.sldprt);;
        SOLIDWORKS装配体(*.asm *.sldasm);;
        Parasolid(*.x_t *.x_b);;
        Solid Edge(*.par *.psm *.asm);;
        Unigraphics/NX(*.prt);;
        Creo Elements/ProE(*.prt *.asm)
          )";
#else
        return R"(All Files(*.step *.stp *.iges *.igs *.brep *.emn);;
        STEP(*.step *.stp);;
        IGES(*.iges *.igs);;
        BREP(*.brep);;
        IDF(*.emn)
          )";
#endif
    }

    void mMenuWidget::importGeometryViaServer(QObject* timerParent, const QString& fileName)
    {
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入该模型，请选择前处理模型！"));
            return;
        }

        //STL/OBJ为三角面片格式，唯一导入入口是“导入网格模型”（产物为网格部件）
        const QString importSuffix = QFileInfo(fileName).suffix().toLower();
        if (importSuffix == "stl" || importSuffix == "obj")
        {
            mGlobalSignals::getInstance()->outputMessageSig(1,
                QString("几何模型导入不支持%1文件，请通过“导入网格模型”导入！")
                    .arg(importSuffix.toUpper()), true);
            return;
        }

        QString importFile;
        if (!mGeoPythonAPIRegistrar::convertToStepIfNeeded(fileName, importFile)) //【GUI修复】成员函数改 static 后的类名调用
            return;

        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
        mGlobalSignals::getInstance()->setProgressBarValue(0);
        mGlobalSignals::getInstance()->outputMessageSig(
            QString("开始导入几何文件 %1").arg(QFileInfo(fileName).fileName()), true);

        EventDispatcher::mGeoImportRequest request;
        request.filePath = QDir::toNativeSeparators(importFile).toUtf8().constData();
        // 按扩展名自动识别格式，支持 STEP/IGES/BREP/IDF
        //（OBJ/STL 为三角面片格式，入口已前移至“导入网格模型”）；
        // CGM 转换出的文件本身就是 STEP。
        request.format = EventDispatcher::mGeoFileFormat::Auto;
        request.sourceUnit = EventDispatcher::mGeoLengthUnit::AutoFromFile;

        // 撤消接入（异步操作三段式，等价 invokeOperation 的挂钩窗口）：
        // 导入提交发生在工作线程，门面无法走 invokeOperation 同步包裹——
        // 前态快照在发起前抓（此刻 worker 尚未写任何数据），轮询到终态后
        // 按成败入栈/丢弃。多部件导入整体为一个撤消条目（ImportGeometry）。
        // busy 期间库侧 undo/redo 自动拒绝（防止与 worker 写入交错）。
        void* historyToken = getGEOServer.historyOperationBegin();

        const EventDispatcher::mGeoDataExchangeTaskHandle handle =
            getGEOServer.dataExchange().importGeometryAsync(request);

        auto* timer = new QTimer(timerParent);

        connect(timer, &QTimer::timeout, timerParent,
            [timerParent, timer, operationId = handle.operationId, fileName,
             historyToken]()
            {
                const auto info =
                    getGEOServer.dataExchange().queryOperation(operationId);

                if (!info.found)
                    return;

                if (info.progress >= 90.0)
                    mGlobalSignals::getInstance()->setProgressBarValue(info.progress - 5.0);
                else
                    mGlobalSignals::getInstance()->setProgressBarValue(info.progress);

                const bool finished =
                    info.state ==
                    EventDispatcher::mGeoDataExchangeTaskState::Succeeded ||
                    info.state ==
                    EventDispatcher::mGeoDataExchangeTaskState::Failed ||
                    info.state ==
                    EventDispatcher::mGeoDataExchangeTaskState::Cancelled;

                if (!finished)
                    return;

                timer->stop();
                timer->deleteLater();

                if (info.state !=
                    EventDispatcher::mGeoDataExchangeTaskState::Succeeded)
                {
                    // 失败/取消：worker 已回滚部分提交，丢弃前态快照不入栈。
                    getGEOServer.historyOperationFailed(historyToken);
                    mGlobalSignals::getInstance()->outputMessageSig(
                        1, QString("几何导入失败：%1")
                        .arg(QString::fromUtf8(info.result.message.c_str())));
                    mGlobalSignals::getInstance()->setProgressBarValue(100);
                    mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
                    return;
                }

                // 成功：前态快照入撤消栈（整个导入 = 一个撤消条目）。
                getGEOServer.historyOperationSucceeded(
                    historyToken, EventDispatcher::mGeoOperationKind::ImportGeometry);

                for (int partId : info.result.affectedPartIds)
                {
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(partId);
                }

                mGlobalSignals::getInstance()->outputMessageSig(
                    QString("几何导入完成：%1").arg(fileName));
                mGlobalSignals::getInstance()->setProgressBarValue(100);
                mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
            });

        timer->start(100);
    }

    void mMenuWidget::slot_importGeoFile()
    {
        emit MechCommon::mMechGlobalSignals::getInstance()->updateStatusImportGeo();
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入该模型，请选择前处理模型！"));
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

		// 后端感知的导入格式清单（与树视图右键导入共用 geoImportFileFilter）
		QString filesuffix = geoImportFileFilter();

        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("导入几何模型"),
            defaultDir,
            filesuffix

        );

        if (!fileName.isEmpty())
        {
            // mpcomm：几何导入完成后回传 committed(geometry)（内部挂在渲染完成
            // 信号上；须在 onImportGeoFile 之前挂接，导入快速完成时不漏报）
            reportMpcommGeometryOnRenderFinished(fileName);
            APIServer::APIParams params;
            params.insert("filePath", { fileName });
            QString strErr;
            mGeoPythonAPIRegistrar::instance()->onImportGeoFile(params, strErr);
        }
    }

	void mMenuWidget::slot_importMeshFile()
	{
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入该模型，请选择前处理模型！"));
            return;
        }

        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = R"(All Files(*.obj *.off *.ply);;
								obj(*.obj);;
                                off(*.off);;
                                ply(*.ply);;
							   )";
        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("导入网格模型"),
            defaultDir,
            filesuffix
        );

        QFileInfo fileinfo(fileName);
        if (fileName.isEmpty())return;
        QString suffix = fileinfo.suffix().toLower();
        QStringList meshTypes = { "stl","obj","off","ply"};
        // STL与obj/off/ply同走网格导入链，保留为网格部件（不再改道几何链转BRep）
        if(meshTypes.contains(suffix)) {
            MechIOFile::mIOFileManager::getInstance()->readMeshIOFile(fileName);
        }
        
        //加入最近打开
        MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
        auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
        QSettings Settings(path, QSettings::IniFormat);
        // 添加一条“最近打开”
        QString file = fileName;
        QString key = QString("recent/%1").arg(QDateTime::currentMSecsSinceEpoch());//当前时间距离 1970-01-01 00:00:00 UTC 的毫秒数（int64）

        Settings.beginGroup("recent");
        for (auto k : Settings.childKeys())
        {
            if (Settings.value(k) == fileName)
                Settings.remove(QString("%1").arg(k));
        }
        Settings.endGroup();
        Settings.setValue(key, file);

        connect(getCVM->getPreRenderWidget(), &CCSMRendWidget::importGeoModelRenderFinished, [=]()
            {
                QTimer::singleShot(2000, this, [=]() {
                    mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
                    QString dir = QDir::currentPath();
                    QString num = key.split('/').last();
                    QString name = dir + "/../../Setting/Recent/" + QFileInfo(fileName).baseName() + num + ".png";
                    QDir().mkpath(dir + "/../../Setting/Recent");          // 自动建目录
                    if (getCVM->getPreRenderWidget() == nullptr)
                        return;
                    getCVM->getPreRenderWidget()->FastCapture(name);

                    auto viewportPrivate = VMManager->getCurrentViewportPrivate();
                    if (viewportPrivate == nullptr)
                    {
                        return;
                    }
                    mBaseRend* rend = viewportPrivate->getRendWidget();
                    if (rend == nullptr)
                    {
                        return;
                    }


                    rend->FitView();
                    VMManager->setCameraSyncTimer(500, 30, [rend]() {
                        emit rend->signal_CameraSync();
                        });
                    });
            });
        return;
        
        //if (!fileName.isEmpty()) {
        //    QFileInfo fileinfo(fileName);
        //    QString suffix = fileinfo.suffix();
        //    if (suffix == "mx")
        //    {
        //        MechIOFile::mIOFileManager::getInstance()->slot_importSolveFile(fileName);
        //    }
        //    else
        //    {
        //        m_transFileInfoDialog->transMxFile(fileName);
        //    }
        //    //加入最近打开
        //    auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
        //    QSettings Settings(path, QSettings::IniFormat);

        //    // 添加一条“最近打开”
        //    QString file = fileName;
        //    QString key = QString("recent/%1").arg(QDateTime::currentMSecsSinceEpoch());
        //    QString num = key.split('/').last();//时间戳

        //    Settings.beginGroup("recent");
        //    for (auto k : Settings.childKeys())
        //    {
        //        if (Settings.value(k) == fileName)
        //            Settings.remove(QString("%1").arg(k));
        //    }
        //    Settings.endGroup();
        //    Settings.setValue(key, file);
        //}
    }
    void mMenuWidget::slot_exportMeshFile()
    {

        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导出该模型，请选择前处理模型！"));
            return;
        }

        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = R"(All Files(*.inp *.obj *.stl *.vtk);;
                                inp(*.inp);;
								obj(*.obj);;
								stl(*.stl);;
								vtk(*.vtk);;
                                )";
        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("导出网格模型"),
            defaultDir,
            filesuffix
        );

        if (fileName.isEmpty()) return;

        MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
        GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());
        QFileInfo fileInfo(fileName);
        QString suffix = fileInfo.suffix();        // "pdf"  (不含点)
        /*  if(suffix=="msh")
              meshOutputFluentMSH(true, false, fileName);
          else if(suffix == "cgns")
              meshOutputCGNS(true, false,fileName);
          else if (suffix == "xyz")
              meshOutputPlot3d(true, false, fileName);*/
        if (suffix == "inp")
            meshOutputINP(true, false, fileName);
        else if (suffix == "stl")
            meshOutputSTL(true, false, fileName);
        else if (suffix == "obj")
            meshOutputOBJ(true, fileName);
        else if (suffix == "vtk")
            meshOutputVTK(true, fileName);
       /* if (!fileName.isEmpty()) {
            getCVM->getModelData()->getGeoManager()->slotExportGeoFiles(fileName);
        }*/
	}

    void mMenuWidget::slot_openPostNVHFile()
    {
        m_menuTabBar_->setCurrentIndex(5);
        slot_CurrentTabChanged(5);
        slot_NVHCurve();
    }

    void mMenuWidget::slot_testButton()
    {
#ifdef _RELEASE
        // 该槽函数禁止在Release下使用，以防外发
        assert(0);
#endif // _RELEASE

        // -----------------测试代码---------------------
        const auto& r = getCVM->getPostRenderWidget()->getPostRender();
        // S5 G5（D9 载体）：切面直调族迁对象清单——addObject(CuttingPlaneParams) 建面返回 id
        //（index=-1=追加新槽位），updateObject(IsShow 掩码) 显隐（旧 createCuttingPlane/
        // setIsShowCuttingPlane 语义承接）
        mxr::CuttingPlaneParams planeParams;
        planeParams.normal = QVector3D(0, 1, 0);
        planeParams.vertex = QVector3D(0, 0, 0);
        const int planeId = r->addObject(planeParams);
        if (planeId < 0)   // 建面失败（无就绪实例等）——引擎 fail-soft，无对象可更新
            return;
        mxr::CuttingPlaneParams showParams;
        showParams.isShow = true;
        showParams.updateMask = mxr::CuttingPlaneParams::Mask_IsShow;
        r->updateObject(planeId, showParams);
        //mxr::timeit("输出VRML ", &mVRMLExporter::exportAllModel, exporter, "./out.wrl");
    }

    void mMenuWidget::slot_ioFileError(MechCommon::IOFileErrorType type)
    {
        switch (type)
        {
        case MechCommon::UnknownError:
            mGlobalSignals::getInstance()->outputMessageSig(2, "未知错误!", true);
            break;
        case MechCommon::FileNotExist:
            mGlobalSignals::getInstance()->outputMessageSig(2, "文件不存在!", true);
            break;
        case MechCommon::FileNotOpen:
            mGlobalSignals::getInstance()->outputMessageSig(2, "无法打开文件!", true);
            break;
        case MechCommon::FileVersionFalse:
            mGlobalSignals::getInstance()->outputMessageSig(2, "文件版本不匹配!", true);
            break;
        case MechCommon::DatabaseNull:
            mGlobalSignals::getInstance()->outputMessageSig(2, "数据库不存在!", true);
            break;
        case MechCommon::ModelMismatching:
            mGlobalSignals::getInstance()->outputMessageSig(2, "模型不匹配!", true);
            break;
        case MechCommon::ResultTypeNotExist:
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前结果不存在!", true);
            break;
        default:
            mGlobalSignals::getInstance()->outputMessageSig(2, "未知错误!", true);
            break;
        }
    }

    void mMenuWidget::slot_readSolveFinished()
    {
        //获取是否成功读取
        bool isSuccess = MechIOFile::mIOFileManager::getInstance()->isSuccessReadSolve();
        QString fn = MechIOFile::mIOFileManager::getInstance()->getReadSolveFileName();
        //释放线程
        MechIOFile::mIOFileManager::getInstance()->releaseReadSolveThread();

        // mpcomm：计算文件(.mx)导入完成回报（不做回声抑制，平台对同值
        // 重复 committed 幂等）
        emit MechCommon::mMechGlobalSignals::getInstance()
            ->mpcommImportCalcFileFinished(isSuccess, fn);

        if (isSuccess)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("%1 %2 %3")
                .arg("文件").arg(fn).arg("读取成功!"));
            //更新界面
            mGlobalSignals::getInstance()->setProgressBarValue(97);
            mGlobalSignals::getInstance()->updateModelTree();
            mGlobalSignals::getInstance()->setProgressBarValue(99);
            getCVM->updateAllModelOperate();
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("%1 %2 %3")
                .arg("文件").arg(fn).arg("读取失败!"));
        }
        mGlobalSignals::getInstance()->PythonthreadfinshSig();
        mGlobalSignals::getInstance()->setProgressBarValue(100);
        mMechGlobalSignals::getInstance()->threadExecuteFinishSig();

        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
        //模型快照（mpcomm 平台模式下软件在后台，截图等界面操作会导致进程崩溃，跳过）
        if (!MechCommon::mMechGlobalSignals::getInstance()->_isPlatformMode)
        {
            QTimer::singleShot(2000, this, [=]() {
                auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
                QSettings Settings(path, QSettings::IniFormat);
                Settings.beginGroup("recent");
                QStringList keys = Settings.childKeys();


                QString id = keys[Settings.childKeys().size() - 1];//时间戳
                Settings.endGroup();

                getCVM->modelFastCapture(fn, id);
                });
        }


	}

    void mMenuWidget::slot_readMeshFinished()
    {
        //获取是否成功读取
        bool isSuccess = MechIOFile::mIOFileManager::getInstance()->isSuccessReadMesh();
        QString fn = MechIOFile::mIOFileManager::getInstance()->getReadMeshFileName();
        QVector<meshReadPart> parts = MechIOFile::mIOFileManager::getInstance()->getMeshParts();
        //释放线程
        MechIOFile::mIOFileManager::getInstance()->releaseReadMeshThread();
        if (isSuccess)
        {
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("%1 %2 %3")
                .arg("文件").arg(fn).arg("读取成功!"));
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("共读取 %1 个部件").arg(parts.size()));
            //后续由parts构建显示数据时在此接入
            //导入链路（mIMeshIOFile::praseParts）只写中央 MechModelData 的 meshData，
            //GlobalMeshMessage 由各操作入口懒设置（同 slot_exportMeshFile），
            //此处统一补设，保证导入完成即全局可见
            GlobalMeshMessage::getInstance()->setMeshData(getCVM->getModelData()->getMeshData());
            mGlobalSignals::getInstance()->updateModelTree();
            getCVM->updateAllModelOperate();
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("%1 %2 %3")
                .arg("文件").arg(fn).arg("读取失败!"));
        }
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
        //模型快照（mpcomm 平台模式下软件在后台，截图等界面操作会导致进程崩溃，跳过）
        if (!MechCommon::mMechGlobalSignals::getInstance()->_isPlatformMode)
        {
            QTimer::singleShot(2000, this, [=]() {
                auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
                QSettings Settings(path, QSettings::IniFormat);
                Settings.beginGroup("recent");
                QStringList keys = Settings.childKeys();


                QString id = keys[Settings.childKeys().size() - 1];//时间戳
                Settings.endGroup();

                QString dir = QDir::currentPath();
                QString name = dir + "/../../Setting/Recent/" + QFileInfo(fn).baseName() + id + ".png";
                QDir().mkpath(dir + "/../../Setting/Recent");          // 自动建目录
                if (getCVM->getPreRenderWidget() == nullptr)
                    return;
                getCVM->getPreRenderWidget()->FastCapture(name);
                });
        }
    }

    void mMenuWidget::slot_writeSolveFinished()
    {
        //获取文件名
        QString fn = MechIOFile::mIOFileManager::getInstance()->getWriteSolveFileName();;

        //获取是否成功读取
        bool isSuccess = MechIOFile::mIOFileManager::getInstance()->isSuccessWriteSolve();
        //释放线程
        MechIOFile::mIOFileManager::getInstance()->releaseWriteSolveThread();
        if (isSuccess)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("文件 %2 导出成功!").arg(fn));

            // mpcomm：手动导出计算文件完成 → 回传 committed(mesh+setup)。
            // （求解流程的自动导出不经此槽：求解走求解任务中心自己的写文件线程，
            //  不会触发 writeSolveFinishedSig，故此处收到的即手动导出）
            emit MechCommon::mMechGlobalSignals::getInstance()
                ->mpcommExportCalcFileFinished(true, fn);

            //更新界面
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("文件 %2 导出失败!").arg(fn));
        }
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
    }

    void mMenuWidget::slot_readMxCaeFinished()
    {
        //获取是否成功读取
        bool isSuccess = MechIOFile::mIOFileManager::getInstance()->isSuccessReadMxcae();
        //获取文件名
        QString fn = MechIOFile::mIOFileManager::getInstance()->getReadMxcaeFileName();
        //释放线程
        MechIOFile::mIOFileManager::getInstance()->releaseReadMxcaeThread();

        if (isSuccess)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("文件 %2 读取成功!").arg(fn));

            //单模型工程文件：数据已读入当前模型，仅需刷新当前模型的树与渲染
            int currentModelID = -1;
            if (getCVM != nullptr && getCVM->getModelData() != nullptr)
            {
                currentModelID = getCVM->getModelData()->getModelDataID();
                MechPreData::mViewportModelManager::getInstance()->setCurrentViewportModel(currentModelID);
                mGlobalSignals::getInstance()->updateModelTree();
                MechPreData::mViewportModel* viewportModel = MechPreData::mViewportModelManager::getInstance()->getViewportModel(currentModelID);
                if (viewportModel != nullptr)
                {
                    viewportModel->updateAllModelOperate();
                }

                //单位制已随模型数据反序列化恢复，同步界面单位制菜单/状态栏勾选与面板显示
                QVector<int> unitsIdx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
                if (unitsIdx.size() == 4)
                {
                    emit mMechGlobalSignals::getInstance()->unitSystemChanged(unitsIdx[0]);
                    emit mMechGlobalSignals::getInstance()->angleUnitChanged(unitsIdx[1]);
                    emit mMechGlobalSignals::getInstance()->speedUnitChanged(unitsIdx[2]);
                    emit mMechGlobalSignals::getInstance()->tempUnitChanged(unitsIdx[3]);
                }
                mInfoWindow::getInstance()->updateAllPanelUnits();
            }
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("文件 %2 读取失败!").arg(fn));
        }
        mGlobalSignals::getInstance()->PythonthreadfinshSig();
        mGlobalSignals::getInstance()->setProgressBarValue(100);
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
    }

    void mMenuWidget::slot_writeMxCaeFinished()
    {
        //获取文件名
        QString fn = MechIOFile::mIOFileManager::getInstance()->getWriteMxcaeFileName();
        //释放线程
        MechIOFile::mIOFileManager::getInstance()->releaseWriteMxcaeThread();

        //自动保存写入不输出提示信息；
        //写工程文件期间已不置灰界面（modelDataThreadStatusChangedSig配套取消），无需恢复
        if (!MechIOFile::isAutoSaveFileName(fn))
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("文件 %2 保存成功!").arg(fn));
        }
    }

    void mMenuWidget::slot_writeMfFinished()
    {
        //获取文件名
        QString fn = MechIOFile::mIOFileManager::getInstance()->getWriteMfFileName();;

        //获取是否成功读取
        bool isSuccess = MechIOFile::mIOFileManager::getInstance()->isSuccessWriteMf();
        //释放线程
        MechIOFile::mIOFileManager::getInstance()->releaseWriteMfThread();
        if (isSuccess)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("文件 %2 导出成功!").arg(fn));
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("文件 %2 导出失败!").arg(fn));
        }
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
    }

    void mMenuWidget::slot_readMfFinished()
    {
        //获取是否成功读取
        bool isSuccess = MechIOFile::mIOFileManager::getInstance()->isSuccessReadMf();
        QString fn = MechIOFile::mIOFileManager::getInstance()->getReadMfFileName();
        //释放线程
        MechIOFile::mIOFileManager::getInstance()->releaseReadMfThread();

        if (isSuccess)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("%1 %2 %3")
                .arg("文件").arg(fn).arg("读取成功!"));
            //更新界面
            mGlobalSignals::getInstance()->setProgressBarValue(99);
            getCVM->updateAllModelOperate();
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("%1 %2 %3")
                .arg("文件").arg(fn).arg("读取失败!"));
        }
        mGlobalSignals::getInstance()->setProgressBarValue(100);
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);

    }

    //自动保存相关方法始终编译（保证moc产物与宏开关无关，仅入口调用受开关控制）
    void mMenuWidget::addAutoSaveBtn(mGroupWidget* group)
    {
        if (group == nullptr)
        {
            return;
        }

        _autoSaveBtn = group->addToolBtn("自动保存", ":/mMainWidgets/Icon/homepage/baocun3x.png", 0, 8, 1, 1, QSize(20, 20));
        _autoSaveBtn->setObjectName("wideButton");
        setQToolButton(_autoSaveBtn, "自动保存", ":/mMainWidgets/Icon/homepage/baocun3x.png", "开启或关闭自动保存，下拉箭头可选择保存间隔", Qt::ToolButtonTextBesideIcon);
        _autoSaveBtn->setCheckable(true);
        //点击主体切换开关，下拉箭头选择保存间隔
        _autoSaveBtn->setPopupMode(QToolButton::MenuButtonPopup);
        _autoSaveBtn->setStyleSheet(R"(
QToolButton#wideButton::menu-button {
    background-color: transparent;
    border: none;
    width: 14px;
    height: 14px;
    subcontrol-position: right center;
    subcontrol-origin: padding;
    right: 2px;
}

QToolButton#wideButton::menu-button:!hover {
    background-color: transparent;
    border: none;
}

QToolButton#wideButton::menu-arrow {
    image: url(:/mMainWidgets/Icon/homepage/arrow-down3x.png);
    width: 10px;
    height: 10px;
}
)");

        //保存间隔下拉菜单（可选项，间隔本身还可通过 mAutoSaveManager::setIntervalMinutes 设置）
        QMenu* intervalMenu = new mUiMenu(_autoSaveBtn);
        ThemeManager::applyMenuStyle(intervalMenu);
        QActionGroup* intervalGroup = new QActionGroup(intervalMenu);
        QList<QPair<int, QString>> intervals = { {2, "2分钟"}, {5, "5分钟"}, {10, "10分钟"}, {30, "30分钟"} };
        for (const auto& pair : intervals)
        {
            QAction* act = intervalMenu->addAction(pair.second);
            act->setCheckable(true);
            act->setData(pair.first);
            intervalGroup->addAction(act);
            if (pair.first == MechIOFile::mAutoSaveManager::defaultIntervalMinutes())
            {
                act->setChecked(true);
            }
        }
        connect(intervalGroup, &QActionGroup::triggered, this, &mMenuWidget::slot_setAutoSaveInterval);
        _autoSaveBtn->setMenu(intervalMenu);

        connect(_autoSaveBtn, &QToolButton::toggled, this, &mMenuWidget::slot_toggleAutoSave);
    }

    void mMenuWidget::slot_toggleAutoSave(bool checked)
    {
        MechIOFile::mAutoSaveManager* manager = MechIOFile::mAutoSaveManager::getInstance();
        manager->setAutoSaveEnabled(checked);
        if (checked)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("自动保存已开启：每 %1 分钟自动保存工程文件到 %2")
                .arg(manager->intervalMinutes()).arg(MechIOFile::autoSaveDir()));
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("自动保存已关闭"));
        }
    }

    void mMenuWidget::slot_setAutoSaveInterval(QAction* action)
    {
        if (action == nullptr)
        {
            return;
        }
        MechIOFile::mAutoSaveManager* manager = MechIOFile::mAutoSaveManager::getInstance();
        manager->setIntervalMinutes(action->data().toInt());
        mGlobalSignals::getInstance()->outputMessageSig(QString("自动保存间隔已设置为 %1 分钟").arg(manager->intervalMinutes()));
    }

    void mMenuWidget::checkAutoSaveRecovery()
    {
        // mpcomm 平台模式下软件在后台运行，弹模态询问框会卡死主线程，跳过恢复检查
        //if (MechCommon::mMechGlobalSignals::getInstance()->_isPlatformMode)
        //{
        //    return;
        //}
        if (!MechIOFile::mAutoSaveManager::hasRecoveryFile())
        {
            return;
        }
        QString fn = MechIOFile::autoSaveFilePath();
        int btn = QMessageBox::question(this, QString("自动保存"),
            QString("检测到上次运行留下的自动保存文件：\n%1\n\n是否打开该文件恢复上次的工作内容？").arg(fn),
            QString("是"),
            QString("否"));
        if (btn == 0)
        {
            restoreAutoSaveFile();
        }
        else
        {
            //放弃恢复：删除自动保存文件，正常初始化
            MechIOFile::mAutoSaveManager::clearRecoveryFile();
            mGlobalSignals::getInstance()->outputMessageSig(QString("已放弃恢复，自动保存文件已删除"));
        }
    }

    void mMenuWidget::restoreAutoSaveFile()
    {
        QString restoreFile = MechIOFile::autoSaveFilePath();
        if (!QFile::exists(restoreFile))
        {
            return;
        }

        //与slot_openFile一致：打开文件前清空已有模型，文件数据读入下方新建的模型
        std::vector<int> oldModelIDs = MechPreData::mViewportModelManager::getInstance()->getAllModelDataID();
        for (int oldID : oldModelIDs)
        {
            mBrowserWindow::getInstance()->getBrowserTree()->removeModelRootItem(oldID);
            mViewTreeFunction::deleteViewportModel(oldID);
        }

        //新模型承接文件数据
        slot_New();

        //打开自动保存的mxcae文件（异步读取）
        MechIOFile::mIOFileManager::getInstance()->readMxcaeFile(restoreFile);

        //读取成功后删除自动保存文件，避免下次启动重复询问；读取失败则保留，
        //用户可再次尝试恢复或手动打开原始工程文件。
        //slot_readMxCaeFinished会先于本回调释放读线程，成功与否须经
        //mIOFileManager记录的最近一次读取结果判断
        std::shared_ptr<QMetaObject::Connection> conn(new QMetaObject::Connection);
        *conn = connect(MechIOFile::mIOFileManager::getInstance(), &MechIOFile::mIOFileManager::readMxcaeFinishedSig, this, [this, conn]()
            {
                disconnect(*conn);
                if (MechIOFile::mIOFileManager::getInstance()->lastReadMxcaeSucceeded())
                {
                    MechIOFile::mAutoSaveManager::clearRecoveryFile();
                }
            });
    }


    void mMenuWidget::slotShowMeshMenu()
    {

    }


	void mMenuWidget::slotShowSectionShape()
	{
		MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("截面形状");

    }

    void mMenuWidget::slot_resizeSymbol()
    {
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();

        std::vector<QString>symbols;
        //载荷
        auto allLoadIds = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadID();
        for (auto id : allLoadIds)
            symbols.push_back(QString("load_%1").arg(id));
        //边界
        auto allBoundaryIds = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getAllBoundaryID();
        for (auto id : allBoundaryIds)
            symbols.push_back(QString("boundary_%1").arg(id));
        //预定义场
        auto allPredefinedFieldIds = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField).getAllPredefinedFieldID();
        for (auto id : allPredefinedFieldIds)
            symbols.push_back(QString("predefinedfield_%1").arg(id));

        symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), symbols, 0.04);
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mMenuWidget::hideTransformList()
    {
        if (_transformList && _transformList->isVisible())
            _transformList->hide();
    }

    void mMenuWidget::onFilterTextChanged(const QString& text, QTreeWidget* tree)
    {

        QString key = text.trimmed();

        // === 1. 先全部复位 ===
        QTreeWidgetItemIterator it(tree);
        while (*it) {
            (*it)->setHidden(false);
            ++it;
        }

        // === 2. 无关键字 → 全部可见，直接返回 ===
        if (key.isEmpty()) return;

        // === 3. 真正过滤 ===
        it = QTreeWidgetItemIterator(tree);
        while (*it) {
            QTreeWidgetItem* item = *it;
            bool match = item->text(0).contains(key, Qt::CaseInsensitive);

            // 3-A 自己先显示/隐藏
            item->setHidden(!match);

            // 3-B 只要匹配，就展开整条祖先链
            if (match) {
                QTreeWidgetItem* p = item->parent();
                while (p) {
                    p->setHidden(false);
                    p->setExpanded(true);
                    p = p->parent();
                }
            }
            ++it;
        }

        // === 4. 父节点匹配 → 把它的所有子节点强制显示 ===
        it = QTreeWidgetItemIterator(tree);
        while (*it) {
            QTreeWidgetItem* item = *it;
            if (item->parent() && item->parent()->text(0).contains(key, Qt::CaseInsensitive)) {
                item->setHidden(false);
                item->parent()->setExpanded(true);
            }
            ++it;

        }
    }


    bool mMenuWidget::hasCaseLoadBoundaryBtn(MxFunctions::CaseType caseType, MxFunctions::LoadBoundaryType lbType) const
    {
        auto it = caseLoadBoundaryBtns.find(caseType);
        if (it == caseLoadBoundaryBtns.end())
            return false;
        return it.value().contains(lbType);
    }

    bool mMenuWidget::hasCasePredefinedFieldBtn(MxFunctions::CaseType caseType, MxFunctions::PredefinedFieldType pfType) const
    {
        auto it = casePredefinedFieldBtns.find(caseType);
        if (it == casePredefinedFieldBtns.end())
            return false;
        return it.value().contains(pfType);
    }

    // mMenuWidget.cpp

    CopyButton::CopyButton(QWidget* parent)
        : QPushButton(parent), menu(nullptr)
    {
    }

    // ------------------------------------------------------------------
    // 直接建模模式切换（菜单按钮进入/退出）
    // ------------------------------------------------------------------
	void mMenuWidget::onSectionButtonClicked()
	{
		auto* btn = qobject_cast<QToolButton*>(sender());
		if (!btn) return;

		QString typeStr = btn->property("sectionType").toString();
		if (typeStr.isEmpty()) return;

		/* 直接走老信号，但把类型塞进去 */
		emit createSectionShapeSig(typeStr);   // 新信号参数改成 QString
	}

	// mMenuWidget.cpp

	// ------------------------------------------------------------------
	// 直接建模模式切换（菜单按钮进入/退出）
	// ------------------------------------------------------------------
	void mMenuWidget::toggleDirectModelingMode(bool enter)
	{
		DirectModelingController* controller = DirectModelingController::getInstance();

		if (enter)
		{
			// 懒创建预览显示组件并接线（渲染层粘合，生命周期与 mMenuWidget 一致）
			//if (!m_directModelingPreview)
				//m_directModelingPreview = new DirectModelingPreviewDisplay(controller, this);
			//m_directModelingPreview->install();
			controller->enterDirectModelingMode();
		}
		else
		{
			controller->exitDirectModelingMode();
			//if (m_directModelingPreview)
				//m_directModelingPreview->uninstall();
		}
    }

    void CopyButton::paintEvent(QPaintEvent* event)
    {
        QPushButton::paintEvent(event);
    }

    void CopyButton::mousePressEvent(QMouseEvent* event)
    {
        QPushButton::mousePressEvent(event);
    }

} // namespace MechMainWindow
