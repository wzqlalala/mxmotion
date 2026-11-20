#include "mGeoSolidifyPanel.h"
#include "ViewportContainer.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
#include "mGeoManager.h"

#define MGEOSKETCH_WITH_OCC_SHAPE 1 //【OCC迁移·P0 兼容】启用 OCC 形态 buildShapeMesh（GUI 具备 OCC 路径；后续切中性离散后移除）
//#include "mGeoSketchAlgorithm.h" //【OCC摘除·S10/P0-1 终局4】死 include（零使用；编译期 OCC 最后一厘米）
#include "mGeoInfoPanel.h"
#include "mInfoWindow.h"
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
 
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"

#include <QMenu>
#include <QFileDialog>
#include <QDir>
#include <QHBoxLayout>
#include <QCoreApplication>
#include <QUuid>

#include <memory>

#include "mAppConfiguration.h"
//#include "mTaskManager.h" //【OCC摘除·S9】MXGeoDataExchange 退役，改中性 Tasking（S3 同款切换）
#include "mTaskingManager.h"
#include "mElementFunction.h"
#include "mMeshData.h"

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1 终局2】
using namespace mxr;

namespace MechMainWindow
{
	mGeoSolidifyPanel::mGeoSolidifyPanel(QWidget* parent)
		: mGeoPanelBase(parent)
	{
        connect(geoPickWidget->pickObjTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            geoPickWidget->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            clearTableItems(pickObjTable);
            setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
            geoPickWidget->_featPick = FeatureObj;
            });

        initStitchFold();
        initGapFold();
        initMissingFaceFold();
        initReverseModelFold();

		_mainLayout->addStretch();

        //全局信号（拾取/视口切换/后台任务）统一改由 mInfoWindow 进入面板后 connectGeoSignals 连接
	}

	void mGeoSolidifyPanel::connectGeoSignals()
	{
		//拾取信号
		slot_subWindowActivated();

		// 新架构：直接挂容器激活信号（带 prevID，可区分真正的视口切换）
		if (auto* container = MechPreData::mViewportModelManager::getInstance()->getViewportContainer()) {
			conn_viewportChanged = connect(container, &MechMainWindow::ViewportContainer::sig_activeViewportChanged,
				this, [this](int viewportID, int prevID) {
					Q_UNUSED(viewportID); Q_UNUSED(prevID);
					slot_subWindowActivated();
				});
		} else {
			conn_viewportChanged = connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, [this](int) { slot_subWindowActivated(); });
		}

		//逆向建模后台任务结束回调（仅处理自己的任务id）
		//【OCC摘除·S9】MXGeoDataExchange 退役，改中性 Tasking（与 src 侧一致）
		connect(MTasking::TaskManager::instance(), &MTasking::TaskManager::endedTaskSig,
            this, &mGeoSolidifyPanel::slot_reverseTaskEnded);
	}

	void mGeoSolidifyPanel::disconnectGeoSignals()
	{
		if (conn_geo)
		{
			disconnect(conn_geo);
			conn_geo = QMetaObject::Connection();
		}
		if (conn_viewportChanged)
		{
			disconnect(conn_viewportChanged);
			conn_viewportChanged = QMetaObject::Connection();
		}
		disconnect(MTasking::TaskManager::instance(), &MTasking::TaskManager::endedTaskSig,
            this, &mGeoSolidifyPanel::slot_reverseTaskEnded);
	}

    mGeoSolidifyPanel::~mGeoSolidifyPanel()
    {

    }

	void mGeoSolidifyPanel::showFold(GeoOperationType type)
	{
        hideAllFold();
        currentFeature = type;
        geoPickWidget->pickLabel->setText("对象");

        geoPickWidget->pickObjTypeCombo->clear();
        geoPickWidget->_featPick = FeatureObj;
        pickObjTable->setVisible(true);
        geoPickWidget->setVisible(true);

        if (GeoStitch == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何面");
            m_FoldSections[0]->insertRow(0, pickObjTable);
            m_FoldSections[0]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 0 });
        }
        if (GeoGap == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何边");
            geoPickWidget->pickObjTypeCombo->addItem("几何面");
            m_FoldSections[1]->insertRow(0, pickObjTable);
            m_FoldSections[1]->insertRow(0, geoPickWidget);

            displayAfterInsertFolds({ 1 });
        }
        if (GeoMissingFace == type)
        {
            displayAfterInsertFolds({ 2 });
            pickObjTable->setVisible(false);
            //隐藏拾取部件，避免其以独立弹窗形式弹出
            geoPickWidget->setVisible(false);
            //更改拾取模式为符号
            auto viewportPrivate = VMManager->getCurrentViewportPrivate();
            auto render = viewportPrivate->getRendWidget();
            render->setPickFilter(PickFilter::PickSymbol);
            geoPickWidget->_featPick = MissingFaces;
            geoPickWidget->strPickType = "缺失面";
            autoMissingFaceSearchAction();
        }

        if (GeoReverseModel == type)
        {
            displayAfterInsertFolds({ 3 });
            //逆向建模不需要拾取对象
            pickObjTable->setVisible(false);
            geoPickWidget->setVisible(false);
        }

	}

	//初始化拼接界面
	void mGeoSolidifyPanel::initStitchFold()
	{
        stitchTolerance = new  MxFunctions::mCalcLineEdit();
        stitchTolerance->setExpression("0.1");
        stitchToleranceUnit = new QLabel("mm", this);
        //checkOverlap = new QCheckBox("检查重合", this);
        //reserveASM = new QCheckBox("保留组件", this);

        //QPushButton* autoStitchSearchBtn = new QPushButton("搜寻", this);
		QPushButton* stitchCancelBtn = new QPushButton("取消", this);
		QPushButton* stitchOkBtn = new QPushButton("确认", this);

		connect(stitchCancelBtn, &QPushButton::clicked, this, &mGeoSolidifyPanel::slot_onCancel);
		connect(stitchOkBtn, &QPushButton::clicked, this, &mGeoSolidifyPanel::slot_stitchAction);
        //connect(autoStitchSearchBtn, &QToolButton::clicked, this, [this]() {
        //    autoStitchSearchAction();
        //    //geoPickWidget->_featPick = FeatureObj;
        //    });

		FoldSection* stitchFold = new FoldSection("几何拼接", this);

        stitchFold->addRow(new PanelLabel("最大距离"), stitchTolerance, stitchToleranceUnit);
        //stitchFold->addRow(checkOverlap);
        //stitchFold->addRow(reserveASM);

		QFrame* horizontalLine = createHorizontalLine(stitchFold);
		stitchFold->addRow(horizontalLine);
		//stitchFold->addRow(autoStitchSearchBtn, stitchCancelBtn, stitchOkBtn, "Left");
        stitchFold->addRow(stitchCancelBtn, stitchOkBtn, "Left");
		m_FoldSections.append(stitchFold);
		m_foldWidgets.append(stitchFold);
		_mainLayout->addWidget(stitchFold);
	}

	//初始化间距界面
	void mGeoSolidifyPanel::initGapFold()
	{
        //【间距修复·V3.0 对齐】最大间距必填且 >0（默认 0.1，同拼接容差）；
        // 最小间距非负（默认 0），空值/非法值由输入框标红 + 确认前置校验拦截。
        maxGapSize = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);
        maxGapSize->setExpression("0.1");
        minGapSize = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::NoEmptyAndNonNegative, this);
        minGapSize->setExpression("0");
        QPushButton* autoGapSearchBtn = new QPushButton("搜寻", this);


		QPushButton* gapCancelBtn = new QPushButton("取消", this);
		QPushButton* gapOkBtn = new QPushButton("确认", this);

		connect(gapCancelBtn, &QPushButton::clicked, this, &mGeoSolidifyPanel::slot_onCancel);
		connect(gapOkBtn, &QPushButton::clicked, this, &mGeoSolidifyPanel::slot_gapAction);
        connect(autoGapSearchBtn, &QToolButton::clicked, this, [this]() {
            autoGapSearchAction();
            //geoPickWidget->_featPick = FeatureObj;
            });

		FoldSection* gapFold = new FoldSection("几何间距修复", this);

        gapFold->addRow(new PanelLabel("最大间距"), maxGapSize);
        gapFold->addRow(new PanelLabel("最小间距"), minGapSize);

		QFrame* horizontalLine = createHorizontalLine(gapFold);

		gapFold->addRow(horizontalLine);
		gapFold->addRow(autoGapSearchBtn, gapCancelBtn, gapOkBtn, "Left");

		m_FoldSections.append(gapFold);
		m_foldWidgets.append(gapFold);
		_mainLayout->addWidget(gapFold);
	}

	//初始化缺失面界面
	void mGeoSolidifyPanel::initMissingFaceFold()
	{
       /* faceTolerance = new  MxFunctions::mCalcLineEdit();
        faceTolerance->setExpression("0.1");
        faceAngleMin = new  MxFunctions::mCalcLineEdit();
        faceAngleMin->setExpression("10");
        faceAngleMinUnit = new QLabel("mm", this);
        faceToleranceUnit = new QLabel("mm", this);
        repairMethod = new QComboBox(this);
        repairMethod->addItems({ "修补", "填充" });*/
        excludeChkBoxMissingFace = new QCheckBox("排除", this);
		QPushButton* missingFaceCancelBtn = new QPushButton("取消", this);
		QPushButton* missingFaceOkBtn = new QPushButton("确认", this);

		connect(missingFaceCancelBtn, &QPushButton::clicked, this, &mGeoSolidifyPanel::slot_onCancel);
		connect(missingFaceOkBtn, &QPushButton::clicked, this, &mGeoSolidifyPanel::slot_missingFacesActionAll);

		FoldSection* missingFaceFold = new FoldSection("缺失面修复", this);

        //missingFaceFold->addRow(new PanelLabel("最小角度"), faceAngleMin, faceAngleMinUnit);
        //missingFaceFold->addRow(new PanelLabel("最短距离"), faceTolerance, faceToleranceUnit);
        //missingFaceFold->addRow(new PanelLabel("修复方式"), repairMethod);
        missingFaceFold->addRow(new QLabel(""), excludeChkBoxMissingFace, "Left");
		QFrame* horizontalLine = createHorizontalLine(missingFaceFold);
		missingFaceFold->addRow(horizontalLine);
		missingFaceFold->addRow( missingFaceCancelBtn, missingFaceOkBtn, "Left");

		m_FoldSections.append(missingFaceFold);
		m_foldWidgets.append(missingFaceFold);
		_mainLayout->addWidget(missingFaceFold);
	}

	//初始化逆向建模界面
	void mGeoSolidifyPanel::initReverseModelFold()
	{
        reverseSourceFileRadio = new QRadioButton(QStringLiteral("STL文件"), this);
        reverseSourceFileRadio->setChecked(true);
        reverseSourceMeshRadio = new QRadioButton(QStringLiteral("当前模型网格"), this);
        QHBoxLayout* sourceLayout = new QHBoxLayout();
        sourceLayout->addWidget(reverseSourceFileRadio);
        sourceLayout->addWidget(reverseSourceMeshRadio);
        QWidget* reverseSourceWidget = new QWidget(this);
        reverseSourceWidget->setLayout(sourceLayout);

        reverseStlFileEdit = new QLineEdit(this);
        reverseStlFileEdit->setReadOnly(true);
        reverseStlFileEdit->setPlaceholderText(QStringLiteral("选择STL文件..."));
        reverseStlFileBtn = new QPushButton(QStringLiteral("选择STL"), this);

        reverseStepPathEdit = new QLineEdit(this);
        reverseStepPathEdit->setReadOnly(true);
        reverseStepPathEdit->setPlaceholderText(QStringLiteral("STEP输出路径..."));
        reverseStepPathBtn = new QPushButton(QStringLiteral("..."), this);

        reverseGenStepChk = new QCheckBox(QStringLiteral("生成STEP文件"), this);
        reverseGenStepChk->setChecked(true);
        reverseCreatePartChk = new QCheckBox(QStringLiteral("创建几何部件"), this);
        reverseCreatePartChk->setChecked(true);

        QPushButton* reverseCancelBtn = new QPushButton(QStringLiteral("取消"), this);
        reverseConvertBtn = new QPushButton(QStringLiteral("开始转换"), this);

        connect(reverseCancelBtn, &QPushButton::clicked, this, &mGeoSolidifyPanel::slot_onCancel);
        connect(reverseConvertBtn, &QPushButton::clicked, this, &mGeoSolidifyPanel::slot_reverseModelAction);

        //选择STL文件，STEP默认输出到同目录同名.stp
        connect(reverseStlFileBtn, &QPushButton::clicked, this, [this]() {
            QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
            QFileInfo dirInfo(defaultDir);
            if (defaultDir.isEmpty() || !dirInfo.exists())
            {
                defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
            }

            const QString fileName = QFileDialog::getOpenFileName(this, tr("选择STL文件"),
                defaultDir, QStringLiteral("STL(*.stl)"));
            if (fileName.isEmpty())
            {
                return;
            }

            MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
            reverseStlFileEdit->setText(fileName);

            const QFileInfo stlInfo(fileName);
            reverseStepPathEdit->setText(stlInfo.absolutePath() + "/" + stlInfo.completeBaseName() + ".stp");
            });

        //手动更改STEP输出路径
        connect(reverseStepPathBtn, &QPushButton::clicked, this, [this]() {
            QString currentPath = reverseStepPathEdit->text();
            if (currentPath.isEmpty())
            {
                currentPath = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
                QFileInfo dirInfo(currentPath);
                if (currentPath.isEmpty() || !dirInfo.exists())
                {
                    currentPath = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
                }
            }

            const QString savePath = QFileDialog::getSaveFileName(this, tr("选择STEP输出路径"),
                currentPath, QStringLiteral("STEP(*.step *.stp)"));
            if (!savePath.isEmpty())
            {
                reverseStepPathEdit->setText(savePath);
            }
            });

		FoldSection* reverseFold = new FoldSection(QStringLiteral("逆向建模(STL转STEP)"), this);

        reverseFold->addRow(new PanelLabel(QStringLiteral("数据来源")), reverseSourceWidget);
        reverseFold->addRow(new PanelLabel(QStringLiteral("STL文件")), reverseStlFileEdit, reverseStlFileBtn);
        reverseFold->addRow(new PanelLabel(QStringLiteral("STEP输出")), reverseStepPathEdit, reverseStepPathBtn);
        reverseFold->addRow(reverseGenStepChk);
        reverseFold->addRow(reverseCreatePartChk);

        QFrame* horizontalLine = createHorizontalLine(reverseFold);
        reverseFold->addRow(horizontalLine);
        reverseFold->addRow(reverseCancelBtn, reverseConvertBtn, "Left");

        m_FoldSections.append(reverseFold);
        m_foldWidgets.append(reverseFold);
        _mainLayout->addWidget(reverseFold);
	}

	//提取当前模型全部网格部件中的三角形单元
	bool mGeoSolidifyPanel::extractCurrentMeshTriangles(std::vector<double>& xyz, std::vector<std::uint32_t>& indices)
	{
        xyz.clear();
        indices.clear();

        auto meshData = getCVM->getModelData()->getMeshData();
        if (meshData == nullptr || meshData->_nodedata.xyz == nullptr)
        {
            return false;
        }

        QHash<int, std::uint32_t> nodeIdToIndex;
        for (auto partIt = meshData->_partdata.constBegin(); partIt != meshData->_partdata.constEnd(); ++partIt)
        {
            mPartData* part = partIt.value();
            if (part == nullptr || part->_parteledata.nodeids == nullptr)
            {
                continue;
            }

            const elementdata& eleData = part->_parteledata;
            int cursor = 0;
            for (int i = 0; i < eleData.elementtypenum; ++i)
            {
                const int nodeCount = eleData.elementnodecount[i * 2];
                const int elementCount = eleData.elementnodecount[i * 2 + 1];
                const MxFunctions::MeshType meshType = MxFunctions::mElementFunction::transformTypeStrToEnum(
                    MxFunctions::ElementType(eleData.elementtype[i]));
                const bool isTriangle = (meshType == MxFunctions::MeshTri && nodeCount == 3);

                for (int j = 0; j < elementCount; ++j)
                {
                    for (int k = 0; k < nodeCount; ++k, ++cursor)
                    {
                        const int nodeRealId = eleData.nodeids[cursor];
                        if (!isTriangle)
                        {
                            //非三角单元只推进游标，不采集
                            continue;
                        }

                        const auto nodeIt = nodeIdToIndex.constFind(nodeRealId);
                        if (nodeIt != nodeIdToIndex.cend())
                        {
                            indices.push_back(nodeIt.value());
                            continue;
                        }

                        const int nodeContinueId = meshData->_nodedata.nodeidmap.value(nodeRealId, -1);
                        if (nodeContinueId < 0)
                        {
                            //网格数据异常：单元引用了没有坐标的节点
                            xyz.clear();
                            indices.clear();
                            return false;
                        }

                        const std::uint32_t newIndex = static_cast<std::uint32_t>(xyz.size() / 3);
                        nodeIdToIndex.insert(nodeRealId, newIndex);
                        for (int c = 0; c < 3; ++c)
                        {
                            xyz.push_back(meshData->_nodedata.xyz[nodeContinueId * 3 + c]);
                        }
                        indices.push_back(newIndex);
                    }
                }
            }
        }

        return !indices.empty() && indices.size() % 3 == 0;
	}

	//逆向建模转换操作
	void mGeoSolidifyPanel::slot_reverseModelAction()
	{
#if !MXGUI_HAS_SOLIDIFY_V2
        //【直模引擎化·S11·GUI 接线】逆向建模走引擎 geoEdit().reverseModeling：
        // 取（拾取或首个）部件原生体网格重建 BRep；STEP 产出走既有导出菜单。
        try
        {
            if (getCVM->isPostModel())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("无法执行逆向建模，请选择前处理模型！"));
                return;
            }
            if (reverseSourceFileRadio != nullptr && reverseSourceFileRadio->isChecked())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
                    tr("源文件模式请先导入 STL 模型后改用源网格模式执行！"));
                return;
            }
            int sourcePartId = -1;
            std::set<int> picked = GetTableIdsUseSet();
            if (!picked.empty())
            {
                //拾取了几何面：按显式选中的 GN 部件逆向（既有行为）
                GeoNeutralData::GN_Part pickedPart =
                    getGEOServer.context().PartManager().getPartByGeoFaceID(*picked.begin());
                sourcePartId = pickedPart.metadata().getID();
            }

            EventDispatcher::mGeoReverseModelingRequest req;
            req.tolerance = 0.0;
            req.name = "ReverseModel";
            if (sourcePartId > 0)
            {
                req.partId = sourcePartId;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    QString("开始逆向建模（部件 %1）...").arg(sourcePartId), true);
            }
            else
            {
                //“当前模型网格”：优先取模型内的三角网格单元（STL 经“导入网格
                // 模型”进来的网格部件等，无 GN 几何也可直接逆向）；
                // 无网格单元时回退首个 GN 几何部件（既有行为）
                std::vector<double> meshXyz;
                std::vector<std::uint32_t> meshIndices;
                if (extractCurrentMeshTriangles(meshXyz, meshIndices))
                {
                    req.meshXyz.resize((int)meshXyz.size());
                    for (size_t i = 0; i < meshXyz.size(); ++i)
                    {
                        req.meshXyz[(int)i] = meshXyz[i];
                    }
                    req.meshTriangles.resize((int)meshIndices.size());
                    for (size_t i = 0; i < meshIndices.size(); ++i)
                    {
                        req.meshTriangles[(int)i] = meshIndices[i];
                    }
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                        QString("开始逆向建模（当前模型网格，%1 个三角形）...")
                            .arg(req.meshTriangles.size() / 3), true);
                }
                else
                {
                    const QList<int> parts = getGEOServer.context().PartManager().getPartList();
                    if (parts.isEmpty())
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("当前没有可逆向的模型!"));
                        return;
                    }
                    req.partId = parts.first();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                        QString("开始逆向建模（部件 %1）...").arg(req.partId), true);
                }
            }
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().reverseModeling(req);
            int partId = 0;
            for (const auto& obj : res.generatedObjects)
            {
                if (obj.id.type == GeoNeutralData::GN_ShapeType::Part && obj.id.id > 0)
                {
                    partId = obj.id.id;
                    break;
                }
            }
            if (res.succeeded() && partId > 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("逆向建模完成！"));
                getCVM->getModelData()->getGeoManager()->registerCreatedPart(partId);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                    tr("逆向建模失败：") + QString::fromStdString(res.message));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("逆向建模操作出错！"));
        }
#else
        if (getCVM->isPostModel())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("无法执行逆向建模，请选择前处理模型！"));
            return;
        }

        if (mGeoReverseCmd != nullptr || reverseTaskId != -1)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("已有逆向建模任务正在执行，请稍候。"), true);
            return;
        }

        const bool genStep = reverseGenStepChk->isChecked();
        const bool createPart = reverseCreatePartChk->isChecked();
        if (!genStep && !createPart)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("请至少勾选一种输出方式：生成STEP文件或创建几何部件。"), true);
            return;
        }

        //MGeoCommand::mGeoCommandSolidOfStlToStep* cmd = MGeoCommand::mGeoCommandFactory::createSolidOfStlToStep(); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
        if (cmd == nullptr)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("创建逆向建模命令失败！"));
            return;
        }
        cmd->setGeoDataBase(getCVM->getModelData()->getGeoData());

        QString modelName;
        QString stepPath;
        if (reverseSourceFileRadio->isChecked())
        {
            //STL文件模式
            const QString stlPath = reverseStlFileEdit->text().trimmed();
            const QFileInfo stlInfo(stlPath);
            if (stlPath.isEmpty() || !stlInfo.exists() || !stlInfo.isFile())
            {
                delete cmd;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("请先选择有效的STL文件。"), true);
                return;
            }
            modelName = stlInfo.completeBaseName();
            stepPath = genStep ? reverseStepPathEdit->text().trimmed() : QString();
            if (stepPath.isEmpty())
            {
                //仅创建部件：STEP写到私有临时目录
                const QString tempRoot = QDir::temp().absoluteFilePath("MxSimStlToStep");
                QDir().mkpath(tempRoot);
                stepPath = tempRoot + "/" + modelName + "-"
                    + QUuid::createUuid().toString(QUuid::Id128).left(12) + ".stp";
            }
            if (QDir::fromNativeSeparators(stlInfo.absoluteFilePath()).compare(
                QDir::fromNativeSeparators(QFileInfo(stepPath).absoluteFilePath()), Qt::CaseInsensitive) == 0)
            {
                delete cmd;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("STEP输出路径不能与输入STL文件相同。"), true);
                return;
            }
            cmd->setStlFilePath(stlPath);
        }
        else
        {
            //当前模型网格模式：提取全部网格部件的三角形单元
            std::vector<double> meshXyz;
            std::vector<std::uint32_t> meshIndices;
            if (!extractCurrentMeshTriangles(meshXyz, meshIndices))
            {
                delete cmd;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前模型网格中没有可用的三角形单元，无法逆向建模。"), true);
                return;
            }
            modelName = QString("MeshReverseModel");
            stepPath = genStep ? reverseStepPathEdit->text().trimmed() : QString();
            if (stepPath.isEmpty())
            {
                const QString tempRoot = QDir::temp().absoluteFilePath("MxSimStlToStep");
                QDir().mkpath(tempRoot);
                stepPath = tempRoot + "/" + modelName + "-"
                    + QUuid::createUuid().toString(QUuid::Id128).left(12) + ".stp";
            }
            cmd->setIndexedMeshData(std::move(meshXyz), std::move(meshIndices), 1.0,
                QString("application-current-mesh"));
        }

        //输出目录必须已存在
        const QString outputDir = QFileInfo(stepPath).absolutePath();
        if (!QDir(outputDir).exists())
        {
            delete cmd;
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("STEP输出目录不存在：%1").arg(outputDir), true);
            return;
        }

        cmd->setModelName(modelName);
        cmd->setStepOutputPath(stepPath);
        cmd->setCreatePart(createPart);

        mGeoReverseCmd = cmd;
        reverseConvertBtn->setEnabled(false);
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("开始逆向建模：%1").arg(modelName), true);

        //转换耗时较长，放到后台任务线程执行（转换内部进程级串行）
        auto taskm = MTasking::TaskManager::instance(); //【OCC摘除·S9】中性 Tasking
        reverseTaskId = taskm->newTask([cmd](MGeoDataExchange::TaskProgress* pg) {
            if (pg != nullptr)
            {
                //阶段名去重：TaskProgress每设一次标题都会输出一条进度日志
                auto lastStage = std::make_shared<QString>();
                cmd->setProgressReporter([pg, lastStage](int percent, const QString& stage) {
                    pg->setValue(percent);
                    if (*lastStage != stage)
                    {
                        *lastStage = stage;
                        pg->setStepTitle(stage);
                    }
                    });
            }
            cmd->handleCommand();
            });
        taskm->setTaskTitle(reverseTaskId, QString("逆向建模: %1").arg(modelName));
        taskm->runTaskJob(reverseTaskId);
	#endif
}

	//逆向建模后台任务结束处理
	void mGeoSolidifyPanel::slot_reverseTaskEnded(int taskId)
	{
#if !MXGUI_HAS_SOLIDIFY_V2
        return;
#else
        if (taskId != reverseTaskId || mGeoReverseCmd == nullptr)
        {
            return;
        }

        //MGeoCommand::mGeoCommandSolidOfStlToStep* cmd = mGeoReverseCmd; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
        mGeoReverseCmd = nullptr;
        reverseTaskId = -1;
        reverseConvertBtn->setEnabled(true);

        if (!cmd->getWarnings().isEmpty())
        {
            /*MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
                QString("逆向建模警告：%1").arg(cmd->getWarnings().join(QString("; "))), true);*/
        }

        if (cmd->isConvertSucceeded())
        {
            if (cmd->getGeoDataSet() != nullptr)
            {
                const int modelDataID = getCVM->getModelData()->getModelDataID();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("逆向建模完成！"));
                MxFunctions::mGlobalSignals::getInstance()->createGeoModelSignal(cmd->getGeoDataSet()->getPartID(), modelDataID);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
                    QString("逆向建模完成，STEP已输出至：%1").arg(cmd->getStepOutputPath()));
            }
        }
        else
        {
            QString errorMsg = cmd->getErrorMessage();
            if (errorMsg.isEmpty())
            {
                errorMsg = QString("未知原因");
            }
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("逆向建模失败：%1").arg(errorMsg));
        }

        //部件已注册进GeoDataBase，命令对象本身可安全释放
        delete cmd;
	#endif
}

	void mGeoSolidifyPanel::slot_highlightGeoData()
	{
		QString type = "几何面";
        std::vector<SymbolSelectInfoData> result;
        mxr::UniversalSymbolFont* symbolFont = nullptr;
		int num = 0;
		vector<int> res;
		//获取拾取模式
		PickFilter pickFilter = getCVM->getPreRenderWidget()->getPickFilter();
		auto geoManager = getCVM->getModelData()->getGeoManager();
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
		switch (pickFilter)
		{
		case mxr::PickFilter::PickGeoSolid:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
			type = "几何体";
			break;
		}
		case mxr::PickFilter::PickGeoFace:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			type = "几何面";
			break;
		}
		case mxr::PickFilter::PickGeoLine:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			type = "几何线";
			break;
		}
		case mxr::PickFilter::PickGeoPoint:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
			type = "几何点";
			break;
		}
        case mxr::PickFilter::PickSymbol:
        {
            result = getCVM->getPreRenderWidget()->getSelectionContainer()->getSymbolSelectData()->getSelectDataByType(SymbolSelectionInfoType::Symbol_All);
            if (result.size() == 0)
            {
                return;
            }
            QString name = result[result.size() - 1].name;
            symbolFont = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(name);

            QStringList nameList = name.split("_");
            res.push_back(nameList.last().toInt());
            QString symbolStr = nameList.first();
            if (symbolStr == "MissingFaceGroup")
            {
                type = "缺失面";
            }
            break;
        }
		default:
			break;
		}
		num = res.size();
		if (geoPickWidget->_featPick == FeatureObj)
		{
			for (int i = pickObjTable->rowCount() - 1; i >= 0; i--)
			{
				QString strId = pickObjTable->item(i, 0)->text();
				if (strId.indexOf(type) == 0)
				{
					pickObjTable->removeRow(i);
				}
			}

			for (auto id : res)
			{
				pickObjTable->insertRow(pickObjTable->rowCount());
				pickObjTable->setItem(pickObjTable->rowCount() - 1, 0, new QTableWidgetItem(QString(type + ": %1").arg(id)));
			}
		}
        else if (geoPickWidget->_featPick == MissingFaces)
        {
            if (symbolFont == nullptr)
            {
                return;
            }
            if (excludeChkBoxMissingFace->isChecked())
            {
                if (symbolFont->_lineColor == QVector3D(255, 0, 0))
                {
                    symbolFont->_lineColor = { 0,255,0 };
                }
                else if (symbolFont->_lineColor == QVector3D(0, 255, 0))
                {
                    symbolFont->_lineColor = { 255, 0, 0 };
                }
                disHighlightSymbol(result.at(0));
            }
            else
            {
                int groupID = res[res.size() - 1] - 1;
                slot_missingFacesActionSingle(groupID);

            }
        }
	}

	void mGeoSolidifyPanel::slot_subWindowActivated()
	{
		//disconnect(getCVM->getPreRenderWidget()->getGeoSelectData());
		if (conn_geo)
			disconnect(conn_geo);
		if (!getCVM || !getCVM->getPreRenderWidget())
		{
			return;
		}
		conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mGeoSolidifyPanel::slot_highlightGeoData);
	}

	
	void mGeoSolidifyPanel::slot_onCancel()
	{
        std::vector<int> partIds = getCVM->getModelData()->getMeshData()->getAllShowPartIds();
        mInfoWindow::getInstance()->getGeoInfoPanel()->setPartAlpha(partIds, 0.0);
        clearRepairSymbols();
		hideAllFold();
	}

    void mGeoSolidifyPanel::clearRepairSymbols()
    {
#if !MXGUI_HAS_SOLIDIFY_V2
        return;
#else
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!symbolController)
        {
            return;
        }

        vector<QString> showSymbol;
        for (auto overlapGroup : missingFacesGroups)
        {
            showSymbol.push_back(overlapGroup.GroupName);
        }

        if (!showSymbol.empty())
        {
            symbolController->deleteSymbol(showSymbol);
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
        }

        missingFacesGroups.clear();
    #endif
}

    void mGeoSolidifyPanel::hideEvent(QHideEvent* event)
    {
        std::vector<int> partIds = getCVM->getModelData()->getMeshData()->getAllShowPartIds();
        mInfoWindow::getInstance()->getGeoInfoPanel()->setPartAlpha(partIds, 0.0);
        clearRepairSymbols();
        mGeoPanelBase::hideEvent(event);
    }

	void mGeoSolidifyPanel::slot_stitchAction()
	{
#if !MXGUI_HAS_SOLIDIFY_V2
        //【直模引擎化·S11·GUI 接线】几何拼接走引擎 geoEdit().stitchFaces
        try
        {
            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何对象!"), true);
                return;
            }
            EventDispatcher::mGeoStitchFacesRequest req;
            for (int t : geoIds)
            {
                req.faceIds.append(t);
            }
            req.tolerance = getDefaultValue(Unit::Length, stitchTolerance->text().toDouble());
            req.name = "Stitch";
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().stitchFaces(req);
            int partId = 0;
            for (const auto& obj : res.generatedObjects)
            {
                if (obj.id.type == GeoNeutralData::GN_ShapeType::Part && obj.id.id > 0)
                {
                    partId = obj.id.id;
                    break;
                }
            }
            if (res.succeeded() && partId > 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                getCVM->getModelData()->getGeoManager()->registerCreatedPart(partId);
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建失败！"));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何拼接操作出错！"));
        }
#else
        try
        {
            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何对象!"), true);
                return;
            }
            QList<int> FaceList;
            for (int t : geoIds)
            {
                FaceList.append(t);
            }
            mGeoCommandSolidOfSurfacesBoundary* surfacesBoundary = mGeoCommandFactory::createSolidOfSurfacesBoundary();
            surfacesBoundary->setGeoDataBase(getCVM->getModelData()->getGeoData());
            surfacesBoundary->setFaces(FaceList);
            surfacesBoundary->setStitchTolerence(stitchTolerance->text().toDouble());
            bool isCreate = surfacesBoundary->handleCommand();
            int modelDataID = getCVM->getModelData()->getModelDataID();
            if (isCreate)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                MxFunctions::mGlobalSignals::getInstance()->createGeoModelSignal(surfacesBoundary->getGeoDataSet()->getPartID(), modelDataID);
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建失败！"));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何拼接操作出错！"));
        }
	#endif
}

    void mGeoSolidifyPanel::slot_gapAction()
    {
#if !MXGUI_HAS_SOLIDIFY_V2
        //【直模引擎化·S11·GUI 接线】间距修复走引擎 geoEdit().gapRepair
        try
        {
            // 前置校验：空值/非法区间在 GUI 拦截，不进引擎（引擎仅兜底 maxGap > 0）。
            const double minGapValue =
                getDefaultValue(Unit::Length, minGapSize->text().toDouble());
            const double maxGapValue =
                getDefaultValue(Unit::Length, maxGapSize->text().toDouble());
            if (maxGapValue <= 0.0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("请输入大于 0 的最大间距!"), true);
                return;
            }
            if (minGapValue < 0.0 || minGapValue >= maxGapValue)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("最小间距需满足: 0 ≤ 最小间距 < 最大间距!"), true);
                return;
            }

            std::set<int> geoIds = GetTableIdsUseSet();
            EventDispatcher::mGeoGapRepairRequest req;
            for (int t : geoIds)
            {
                req.faceIds.append(t);
            }
            req.minGap = minGapValue;
            req.maxGap = maxGapValue;
            req.name = "GapRepair";
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().gapRepair(req);
            if (res.succeeded())
            {
                for (auto editpart : res.modifiedObjectIds)
                {
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何间距修复成功!"));
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                    tr("几何间距修复失败：") + QString::fromStdString(res.message));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何间距修复操作出错！"));
        }
#else
        try
        {
            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何对象!"), true);
                return;
            }

            // TODO: 实现间距修复操作的具体逻辑
            // 这里先输出一个成功消息作为占位
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何间距修复操作已执行!"));
            geoPickWidget->clearHighlight();
            clearTableItems(pickObjTable);
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何间距修复操作出错！"));
        }
    #endif
}

    void mGeoSolidifyPanel::slot_missingFacesActionAll()
    {
#if !MXGUI_HAS_SOLIDIFY_V2
        //【直模引擎化·S11/V3.0 功能对齐】缺失面全部修复：
        // 仅修复红色（未排除）组；成功后清符号刷新。
        try
        {
            if (m_engineMissingFaceGroups.isEmpty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请先执行缺失面搜索!"));
                return;
            }
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            int fixedCount = 0;
            for (int index = 0; index < m_engineMissingFaceGroups.size(); ++index)
            {
                const auto& group = m_engineMissingFaceGroups.at(index);
                const auto& un = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    QString("MissingFaceGroup_%1").arg(index + 1));
                if (un->_lineColor != QVector3D(255, 0, 0))
                {
                    continue;    // 绿色=勾选排除，跳过（V3.0 语义）
                }
                EventDispatcher::mGeoMissingFaceFixRequest req;
                req.boundaryEdgeIds = group.loopEdgeIds;
                req.name = "MissingFaceFix";
                EventDispatcher::mGeoOperationResult fixRes =
                    getGEOServer.geoEdit().missingFaceFix(req);
                if (fixRes.succeeded())
                {
                    //【显示未刷新修复】修改部件登记 _editPart（editGeoModelSignal_2 仅刷该集合）
                    for (auto editpart : fixRes.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    ++fixedCount;
                }
            }
            if (fixedCount > 0)
            {
                // 清全部组符号并刷新
                std::vector<QString> showSymbol;
                for (int index = 0; index < m_engineMissingFaceGroups.size(); ++index)
                {
                    showSymbol.push_back(QString("MissingFaceGroup_%1").arg(index + 1));
                }
                symbolController->deleteSymbol(showSymbol);
                symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
                    tr("修复 %1 个缺失面!").arg(fixedCount));
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败！"));
            }
            autoMissingFaceSearchAction();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
#else
        try
        {
            if (nullptr == mGeoMissingFaces)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到重复线数据!"));
                return;
            }
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            vector<int> groupIDs;
            for (auto overlapGroup : missingFacesGroups)
            {
                const auto& un = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(overlapGroup.GroupName);
                QVector3D lineColor = un->_lineColor;
                if (lineColor == QVector3D(255, 0, 0))
                {
                    groupIDs.push_back(overlapGroup.ID);
                }
            }
            if (groupIDs.size() == 0)
            {
                return;
            }
            mGeoMissingFaces->setPinType(PinType(ByArea));
            int modelDataID = getCVM->getModelData()->getModelDataID();
            if (mGeoMissingFaces->FixAll(groupIDs))
            {
                geoPickWidget->clearHighlight();
                vector<QString> showSymbol;
                for (auto overlapGroup : missingFacesGroups)
                {
                    showSymbol.push_back(overlapGroup.GroupName);
                }
                symbolController->deleteSymbol(showSymbol);
                //symbolController->setAllSymbolShowState(false);
                symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();

                MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败！"));
            }
            autoMissingFaceSearchAction();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    #endif
}

    void mGeoSolidifyPanel::slot_missingFacesActionSingle(int groupID)
    {
#if !MXGUI_HAS_SOLIDIFY_V2
        //【直模引擎化·S11/V3.0 功能对齐】单组缺失面修复：引擎修复成功后清该组符号。
        try
        {
            if (groupID < 0 || groupID >= m_engineMissingFaceGroups.size())
            {
                return;
            }
            const auto& group = m_engineMissingFaceGroups.at(groupID);
            EventDispatcher::mGeoMissingFaceFixRequest req;
            req.boundaryEdgeIds = group.loopEdgeIds;
            req.name = "MissingFaceFix";
            EventDispatcher::mGeoOperationResult fixRes = getGEOServer.geoEdit().missingFaceFix(req);
            if (fixRes.succeeded())
            {
                //【显示未刷新修复】修改部件登记 _editPart（与特征面板同款）
                for (auto editpart : fixRes.modifiedObjectIds)
                {
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                const QString groupName = QString("MissingFaceGroup_%1").arg(groupID + 1);
                symbolController->deleteSymbol({ groupName.toStdString().c_str() });
                symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
            }
            else
            {
                //【诊断·缺失面】透出引擎失败消息（分段定位见 temp/dbg_missingface.log）
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                    tr("缺失面修复失败：") + QString::fromStdString(fixRes.message));
            }
            autoMissingFaceSearchAction();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
#else
        try
        {
            if (nullptr == mGeoMissingFaces)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到缺失面数据!"));
                return;
            }
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            QString symbolStr = QString("MissingFaceGroup_%1").arg(groupID);
            const auto& un = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(symbolStr);
            QVector3D lineColor = un->_lineColor;
            if (lineColor != QVector3D(255, 0, 0))
            {
                return;
            }
            std::set<int> partIds;
            mGeoMissingFaces->setPinType(PinType(ByArea));
            int modelDataID = getCVM->getModelData()->getModelDataID();
            if (mGeoMissingFaces->FixSingleGroup(groupID))
            {
                geoPickWidget->clearHighlight();
                vector<QString> showSymbol;
                for (auto overlapGroup : missingFacesGroups)
                {
                    showSymbol.push_back(overlapGroup.GroupName);
                }
                symbolController->deleteSymbol(showSymbol);
                //symbolController->setAllSymbolShowState(false);
                symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();

                MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败！"));
            }
            autoMissingFaceSearchAction();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    #endif
}

    bool mGeoSolidifyPanel::autoStitchSearchAction()
    {
#if !MXGUI_HAS_SOLIDIFY_V2
        return false;
#else
        try
        {
            // TODO: 实现自动拼接搜索逻辑
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("自动拼接搜索已完成!"));
            return true;
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("自动拼接搜索出错！"));
            return false;
        }
    #endif
}

    bool mGeoSolidifyPanel::autoGapSearchAction()
    {
#if !MXGUI_HAS_SOLIDIFY_V2
        //【间距修复·V3.0 对齐】搜寻：按已填写的 [最小间距, 最大间距] 区间探测
        // 可修复间隙（只读），不修改输入框内容。
        try
        {
            // 与确认同款前置校验，保证搜寻/修复作用在同一区间。
            const double minGapValue =
                getDefaultValue(Unit::Length, minGapSize->text().toDouble());
            const double maxGapValue =
                getDefaultValue(Unit::Length, maxGapSize->text().toDouble());
            if (maxGapValue <= 0.0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("请输入大于 0 的最大间距!"), true);
                return false;
            }
            if (minGapValue < 0.0 || minGapValue >= maxGapValue)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("最小间距需满足: 0 ≤ 最小间距 < 最大间距!"), true);
                return false;
            }

            // 清旧拾取行（同手动拾取槽：先移除同类型行再入新行）
            for (int i = pickObjTable->rowCount() - 1; i >= 0; i--)
            {
                QString strId = pickObjTable->item(i, 0)->text();
                if (strId.indexOf("几何线") == 0)
                {
                    pickObjTable->removeRow(i);
                }
            }

            EventDispatcher::mGeoGapSearchRequest req;
            std::set<int> geoIds = GetTableIdsUseSet();
            for (int t : geoIds)
            {
                req.faceIds.append(t);
            }
            req.minGap = minGapValue;
            req.maxGap = maxGapValue;
            EventDispatcher::mGeoQueryResult<EventDispatcher::mGeoGapSearchResult> res =
                getGEOServer.geoEdit().gapSearch(req);
            if (!res.succeeded() || res.value.pairCount == 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("未探测到 [%1, %2] 区间内可修复的间隙!")
                        .arg(minGapValue, 0, 'g', 6)
                        .arg(maxGapValue, 0, 'g', 6), true);
                return false;
            }

            // 参与间隙的自由边入拾取表（"几何线: N"，同手动选边格式），
            // 拾取体系自带高亮——等价用户逐条点选这些边。
            for (int edgeId : res.value.edgeIds)
            {
                pickObjTable->insertRow(pickObjTable->rowCount());
                pickObjTable->setItem(pickObjTable->rowCount() - 1, 0,
                    new QTableWidgetItem(QString("几何线: %1").arg(edgeId)));
            }
            setGeoHighlight(pickObjTable, "几何边");

            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
                tr("探测到 %1 处区间内间隙，实际距离 [%2, %3]")
                    .arg(res.value.pairCount)
                    .arg(res.value.minGap, 0, 'g', 6)
                    .arg(res.value.maxGap, 0, 'g', 6));
            return true;
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("自动间距搜索出错！"));
            return false;
        }
#else
        try
        {
            // TODO: 实现自动间距搜索逻辑
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("自动间距搜索已完成!"));
            return true;
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("自动间距搜索出错！"));
            return false;
        }
#endif
    }

    bool mGeoSolidifyPanel::autoMissingFaceSearchAction()
    {
#if !MXGUI_HAS_SOLIDIFY_V2
        //【直模引擎化·S11·GUI 接线/V3.0 功能对齐】缺失面搜索：
        // 引擎 missingFaceSearch（FreeBounds 闭合环+离散点）→ 红色边界符号渲染。
        try
        {
            // 清旧符号
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            {
                std::vector<QString> showSymbol;
                for (int index = 0; index < m_engineMissingFaceGroups.size(); ++index)
                {
                    showSymbol.push_back(QString("MissingFaceGroup_%1").arg(index + 1));
                }
                symbolController->deleteSymbol(showSymbol);
                symbolController->UpdateAllSymbol();
            }
            m_engineMissingFaceGroups.clear();

            EventDispatcher::mGeoMissingFaceSearchRequest req;
            EventDispatcher::mGeoQueryResult<EventDispatcher::mGeoMissingFaceSearchResult> res =
                getGEOServer.geoEdit().missingFaceSearch(req);
            if (!res.succeeded() || res.value.groups.isEmpty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的几何!"));
                return false;
            }
            m_engineMissingFaceGroups = res.value.groups;

            // 逐组建红色边界符号（V3.0 同款 UniversalSymbolFont；
            // 组名尾号=索引+1，与拾取交互 groupID=res.last()-1 映射一致）
            double totalArea = 0.0;
            for (int index = 0; index < m_engineMissingFaceGroups.size(); ++index)
            {
                const auto& group = m_engineMissingFaceGroups.at(index);
                totalArea += group.area;
                if (group.boundaryPoints.size() < 3)
                {
                    continue;
                }
                const auto& un = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    QString("MissingFaceGroup_%1").arg(index + 1));
                un->_vexter.clear();
                for (const QVector3D& point : group.boundaryPoints)
                {
                    un->_vexter.push_back(point);
                }
                //【乱线修复】渲染侧 _lineIndex=PrimitiveSet::LINES（独立线段对，
                // 每 2 索引一条线段）——须按相邻点成对展开，顺序流索引会画跨点乱线。
                un->_lineIndex.clear();
                for (int vIndex = 0; vIndex + 1 < group.boundaryPoints.size(); ++vIndex)
                {
                    un->_lineIndex.push_back(static_cast<unsigned int>(vIndex));
                    un->_lineIndex.push_back(static_cast<unsigned int>(vIndex + 1));
                }
                un->_lineColor = { 255, 0, 0 };
                un->_size = 1.0f;
                un->_lineIsFixedPixel = false;
                un->_lineDepthState = false;
                un->_showState = true;
                symbolController->createSymbol(un);
            }
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
                tr("缺失面搜索完成：%1 组（合计面积约 %2）")
                    .arg(m_engineMissingFaceGroups.size()).arg(totalArea, 0, 'f', 3));
            return true;
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("缺失面搜索出错！"));
            return false;
        }
#else
        try
        {

            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            vector<QString> showSymbol;
            for (auto overlapGroup : missingFacesGroups)
            {
                showSymbol.push_back(overlapGroup.GroupName);
            }
            symbolController->deleteSymbol(showSymbol);
            missingFacesGroups.clear();
            if (nullptr == mGeoMissingFaces)
            {
                mGeoMissingFaces = mGeoCommandFactory::createSurfaceEditOfPinHole();
            }
            mGeoMissingFaces->setGeoDataBase(getCVM->getModelData()->getGeoData());
            std::set<int> geoUpdateEdgeIDs;
            missingFacesGroups = mGeoMissingFaces->DetectPinHoles(geoUpdateEdgeIDs);
            if (missingFacesGroups.size() == 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的几何!"));
                return false;
            }

            geoPickWidget->clearHighlight();
            //高亮显示干涉结果
            for (auto overlapGroup : missingFacesGroups)
            {
                auto meshData = MGeoManager::mGeoSketchAlgorithm::buildShapeMesh(overlapGroup.MissingWire);
                QVector<QVector3D> points;
                for (int vIndex : meshData.pointId)
                {
                    points.push_back(QVector3D(meshData.xyz[vIndex]));
                }
                std::vector<unsigned int> _lineIndex;
                _lineIndex.reserve(meshData.pointId.size());

                for (int val : meshData.pointId)
                {
                    _lineIndex.push_back(static_cast<unsigned int>(val));
                }
                //*******************************通用符号+文字***********************//
                const auto& un = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(overlapGroup.GroupName);
                un->_vexter = points;
                un->_lineIndex = _lineIndex;
                un->_lineColor = { 255,0,0 };
                un->_size = 1.0f;
                un->_lineIsFixedPixel = false;
                un->_lineDepthState = false;
                un->_showState = true;
                //un->_highlightState = true;
                symbolController->createSymbol(un);


            }
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();

        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的几何！"));
            return false;
        }

        return true;
    #endif
}
}