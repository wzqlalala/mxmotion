
#include "mMotionScenarioPanel.h"
#include "mMotionSolverThread.h"
#include "mMotionPickerActivate.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMotionPanelColors.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mUnitSystemManager.h"

#include <cmath>

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//BasicData
#include "mMotionBody.h"
#include "mMotionBodyManager.h"
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointManager.h"
#include "mMotionDriveManager.h"
#include "mMotionDriveBase.h"
#include "mMotionJointDrive.h"
#include "mMotionScenario.h"
#include "mMotionScenarioManager.h"
#include "mIOFileManager.h"
#include "mModelData.h"

//Geo
#include "mGeometricFeatureAttribute.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QProgressDialog>
#include <QSpinBox>
using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{

namespace {
    // 各单位制式下的标准重力加速度（对应 CommonData::UnitSystems 的 8 套制式）
    double standardGravityForSystem(int sysIndex)
    {
        static const double table[] = { 9.81, 981.0, 9810.0, 9810.0, 9810.0, 9.81e6, 32.174, 386.09 };
        if (sysIndex < 0 || sysIndex > 7) return 9.81;
        return table[sysIndex];
    }

    // 各单位制式的长度换算因子（与 BasicData 长度表一致，基准 vm=1；制式序 = m,cm,mm,mm,mm,vm,ft,in）
    double lengthFactorForSystem(int sysIndex)
    {
        static const double factors[] = { 1e6, 1e4, 1e3, 1e3, 1e3, 1.0, 304800.0, 25400.0 };
        if (sysIndex < 0 || sysIndex > 7) return 1e6;
        return factors[sysIndex];
    }

    QString gravityUnitString()
    {
        if (!getCVM || !getCVM->getModelData()) return QStringLiteral("m/s²");
        const int idx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[0];
        if (idx < 0 || idx > 7) return QStringLiteral("m/s²");
        return CommonData::UnitSystems[idx][6];
    }
}


    mMotionScenarioPanel::mMotionScenarioPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_motionScenarioData = nullptr;

        //获取当前视口的数据
        m_mainLayout = new QVBoxLayout(this);

        // ==================== 2. 时间控制 ====================
        FoldSection* foldTime = new FoldSection("时间控制", this);

        m_spbEndTime = new MxFunctions::mCalcLineEdit(
            "50", MxFunctions::mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);

        m_spbStepSize = new MxFunctions::mCalcLineEdit(
            "0.01", MxFunctions::mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);

        m_lblTotalSteps = new QLabel("预计输出帧数: 1000 帧");
        m_lblTotalSteps->setStyleSheet("color: gray; font-style: italic;"); // 给个弱化提示的样式

        foldTime->addRow(new QLabel("结束时间 (s):"), m_spbEndTime);
        foldTime->addRow(new QLabel("积分步长 (s):"), m_spbStepSize);
        foldTime->addRow(m_lblTotalSteps); // 放在输入框下面
        m_mainLayout->addWidget(foldTime);

        // ==================== 3. 物理环境 (重力) ====================
        FoldSection* foldEnv = new FoldSection("物理环境", this);

        // 默认重力跟随全局单位制（mm 制给 9810，米制给 9.81）
        double g = 9.81;
        if (getCVM && getCVM->getModelData()) {
            g = standardGravityForSystem(getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[0]);
        }
        m_gravityDirSelector = new mDirectionSelectorWidget(
            QStringLiteral("重力加速度"), true, QStringLiteral("重力大小"), false, QString(), false, this);
        {
            const bool blocked = m_gravityDirSelector->blockSignals(true);
            m_gravityDirSelector->setDirectionMode(mDirectionSelectorWidget::DirectInput);
            m_gravityDirSelector->setDirection(QVector3D(0, 0, -1));
            m_gravityDirSelector->setMagnitudeValue(QString::number(g));
            m_gravityDirSelector->blockSignals(blocked);
        }
        foldEnv->addRow(m_gravityDirSelector);

        // 当前重力单位提示（随全局单位制切换刷新）
        m_lblGravityUnit = new QLabel(this);
        foldEnv->addRow(new QLabel(QStringLiteral("重力单位:")), m_lblGravityUnit);
        refreshGravityUnitLabel();
        m_mainLayout->addWidget(foldEnv);

        // ==================== 求解模式设置 ====================
        FoldSection* foldPreset = new FoldSection("求解模式", this);

        // 使用预设开关
        m_chkUsePreset = new QCheckBox("使用预设");
        m_chkUsePreset->setChecked(true);
        foldPreset->addRow(m_chkUsePreset);

        m_lblPresetMode = new QLabel("预设模式:");
        m_cmbPresetMode = new QComboBox();
        // 显示文本与枚举值显式绑定，不再依赖添加顺序，后续枚举扩展只需在此追加一行
        auto addPresetModeItem = [this](const QString& text, MechMotionData::PresetMode mode) {
            m_cmbPresetMode->addItem(text, static_cast<int>(mode));
            };
        addPresetModeItem("FAST (快速计算)", MechMotionData::PresetMode::FAST);
        // addPresetModeItem("CONTACT (接触优化)", MechMotionData::PresetMode::CONTACT);
        addPresetModeItem("ROBUST (高精度求解)", MechMotionData::PresetMode::ROBUST);
        // addPresetModeItem("HIGH_PRECISION (高精度)", MechMotionData::PresetMode::HIGH_PRECISION);
        foldPreset->addRow(m_lblPresetMode, m_cmbPresetMode);

        m_mainLayout->addWidget(foldPreset);

        // ==================== 积分器设置 ====================
        auto foldIntegrator = new FoldSection("积分器设置", this);foldIntegrator->setParent(nullptr);

        m_cmbIntegratorAdv = new QComboBox();
        m_cmbIntegratorAdv->addItems({
            "EULER_IMPLICIT_LINEARIZED",
            "EULER_IMPLICIT_PROJECTED",
            "EULER_IMPLICIT",
            "TRAPEZOIDAL",
            "TRAPEZOIDAL_LINEARIZED",
            "HHT",
            "HEUN",
            "RUNGEKUTTA45",
            "EULER_EXPLICIT",
            "LEAPFROG",
            "NEWMARK",
            "CUSTOM"
        });
        foldIntegrator->addRow(new QLabel("积分方法:"), m_cmbIntegratorAdv);

        // HHT Alpha / Beta 标签与控件（根据方法动态 addRow/remove）
        m_lblAlpha = new QLabel("Alpha:", this);m_lblAlpha->setParent(nullptr);
        m_spbAlpha = new MxFunctions::mCalcLineEdit(
            "-0.2", MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbAlpha->setParent(nullptr);

        m_lblBeta = new QLabel("Beta:", this);m_lblBeta->setParent(nullptr);
        m_spbBeta = new MxFunctions::mCalcLineEdit(
            "0.25", MxFunctions::mCalcLineEdit::CalcMode::NonNegative, this);
        m_spbBeta->setParent(nullptr);

        // 最大迭代次数 & 收敛容差（所有积分方法共用）
        m_spbIntegMaxIters = new QSpinBox(); m_spbIntegMaxIters->setRange(1, 10000); m_spbIntegMaxIters->setValue(50);
        m_spbIntegTol = new MxFunctions::mCalcLineEdit(
            "1e-8", MxFunctions::mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);
        foldIntegrator->addRow(new QLabel("最大迭代次数:"), m_spbIntegMaxIters);
        foldIntegrator->addRow(new QLabel("收敛容差:"), m_spbIntegTol);

        // 默认不添加到主布局（由预设开关控制）
        // m_mainLayout->addWidget(foldIntegrator);

        // ==================== 求解器设置 ====================
        auto foldSolver = new FoldSection("求解器设置", this);foldSolver->setParent(nullptr);

        m_cmbSolverType = new QComboBox();
        m_cmbSolverType->addItems({
            "PSOR", "PSSOR", "PJACOBI", "PMINRES", "BARZILAIBORWEIN",
            "APGD", "ADDM", "SPARSE_LU", "SPARSE_QR", "PARDISO_MKL",
            "PARDISO_PROJECT", "MUMPS", "GMRES", "MINRES", "BICGSTAB", "CUSTOM"
        });
        foldSolver->addRow(new QLabel("求解器类型:"), m_cmbSolverType);

        m_spbSolverMaxIters = new QSpinBox(); m_spbSolverMaxIters->setRange(1, 10000); m_spbSolverMaxIters->setValue(100);
        foldSolver->addRow(new QLabel("最大迭代次数:"), m_spbSolverMaxIters);

        m_spbSolverTol = new MxFunctions::mCalcLineEdit(
            "1e-10", MxFunctions::mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);
        foldSolver->addRow(new QLabel("收敛容差:"), m_spbSolverTol);

        m_chkSolverVerbose = new QCheckBox("输出日志");
        foldSolver->addRow(m_chkSolverVerbose);

        // 默认不添加到主布局（由预设开关控制）
        // m_mainLayout->addWidget(m_foldSolver);

        // ==================== Lambda 信号绑定 ====================

        // --- 时间控制同步 (带步数自动计算) ---
        auto updateTimeLogic = [this]() {
            double endT = m_spbEndTime->text().toDouble();
            double step = m_spbStepSize->text().toDouble();

            // 更新底层数据
            if (m_motionScenarioData) {
                m_motionScenarioData->setEndTime(endT);
                m_motionScenarioData->setStepSize(step);
            }

            // 智能联动：在界面上实时显示预计的输出总步数
            if (step > 0) {
                int steps = static_cast<int>(std::ceil(endT / step));
                m_lblTotalSteps->setText(QString("预计输出帧数: %1 帧").arg(steps));
            }
            };
        connect(m_spbEndTime, &QLineEdit::editingFinished, this, [updateTimeLogic]() { updateTimeLogic(); });
        connect(m_spbStepSize, &QLineEdit::editingFinished, this, [updateTimeLogic]() { updateTimeLogic(); });

        // --- 重力向量同步（方向控件：方向 × 大小 合成矢量） ---
        auto syncGravity = [this]() {
            if (!m_motionScenarioData || !m_gravityDirSelector)
                return;
            // 起点-终点页仍为出厂默认(起点0,0,0/终点1,0,0)时，切换模式的瞬间默认方向(+X)
            // 会经 directionChanged 顶掉当前重力——改用当前重力方向初始化终点，方向保持不变
            if (m_gravityDirSelector->directionMode() == mDirectionSelectorWidget::CustomCoordinate
                && m_gravityDirSelector->customCoordinatesUntouched())
            {
                const QVector3D g = m_motionScenarioData->getGravity();
                const double glen = g.length();
                const QVector3D gdir = glen > 1e-12 ? g / static_cast<float>(glen) : QVector3D(0, 0, -1);
                m_gravityDirSelector->setCustomCoordinatePoints(QVector3D(0, 0, 0), gdir);
                saveDirPickState(m_gravityDirSelector, m_motionScenarioData->gravityPickState_);
                return;
            }
            QVariant dv = m_gravityDirSelector->getDirection();
            if (!dv.isValid() || dv.isNull())
                return;
            QVector3D dir = dv.value<QVector3D>();
            if (dir.isNull())
                return;
            bool ok = false;
            const double mag = m_gravityDirSelector->magnitudeValue().trimmed().toDouble(&ok);
            // 允许 0（失重工况）：只拦解析失败与负值；0 时方向无意义，直接存零向量
            if (!ok || mag < 0)
                return;
            m_motionScenarioData->setGravity(mag > 0 ? dir.normalized() * mag : QVector3D(0, 0, 0));
            saveDirPickState(m_gravityDirSelector, m_motionScenarioData->gravityPickState_);
            };
        connect(m_gravityDirSelector, &mDirectionSelectorWidget::directionChanged, this, syncGravity);
        connect(m_gravityDirSelector, &mDirectionSelectorWidget::magnitudeChanged, this, syncGravity);
        connect(m_gravityDirSelector, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this, foldEnv]() {
            //模式切换导致内容页高度变化，须刷新折叠区表单布局避免挤压/留白
            foldEnv->refreshLayout();
            });

        // --- 求解模式：使用预设开关 ---
        auto updatePresetLogic = [this, foldPreset, foldIntegrator, foldSolver]() {
            bool usePreset = m_chkUsePreset->isChecked();
            if (m_motionScenarioData)
                m_motionScenarioData->setUsePreset(usePreset);

            if (usePreset) {
                // 预设开启：显示预设模式行，隐藏高级设置
                m_lblPresetMode->setVisible(true);
                m_cmbPresetMode->setVisible(true);

                // 从布局中移除高级积分器和求解器（不销毁）
                if (foldIntegrator->parentWidget() == this)
                    foldIntegrator->setParent(nullptr);
                if (foldSolver->parentWidget() == this)
                    foldSolver->setParent(nullptr);
            }
            else {
                // 预设关闭：隐藏预设模式行，显示高级设置
                m_lblPresetMode->setVisible(false);
                m_cmbPresetMode->setVisible(false);

                // 添加高级积分器和求解器到主布局 stretch 之前
                int stretchIndex = -1;
                for (int i = 0; i < m_mainLayout->count(); ++i) {
                    if (m_mainLayout->itemAt(i)->spacerItem()) {
                        stretchIndex = i;
                        break;
                    }
                }
                if (foldIntegrator->parentWidget() == nullptr) {
                    if (stretchIndex >= 0)
                        m_mainLayout->insertWidget(stretchIndex, foldIntegrator);
                    else
                        m_mainLayout->addWidget(foldIntegrator);
                }
                if (foldSolver->parentWidget() == nullptr) {
                    if (stretchIndex >= 0)
                        m_mainLayout->insertWidget(stretchIndex + 1, foldSolver);
                    else
                        m_mainLayout->addWidget(foldSolver);
                }
            }
            QTimer::singleShot(0, foldPreset, [=]() { foldPreset->refreshLayout(); });

            };

        connect(m_chkUsePreset, &QCheckBox::toggled, this, [updatePresetLogic]() { updatePresetLogic(); });

        // 预设模式下拉框同步：通过 itemData 取枚举值，与显示文本解耦
        connect(m_cmbPresetMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_motionScenarioData)
                m_motionScenarioData->setPresetMode(static_cast<MechMotionData::PresetMode>(m_cmbPresetMode->itemData(index).toInt()));
            });

        // --- 积分器高级设置同步 ---
        connect(m_cmbIntegratorAdv, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, foldIntegrator](int index) {
            if (m_motionScenarioData)
                m_motionScenarioData->setIntegratorType(static_cast<MechMotionData::IntegratorType>(index));

            // 先全部移除
            //if (m_lblAlpha->parentWidget() == m_foldIntegrator)
                m_lblAlpha->setParent(nullptr);
            //if (m_spbAlpha->parentWidget() == m_foldIntegrator)
                m_spbAlpha->setParent(nullptr);
            //if (m_lblBeta->parentWidget() == m_foldIntegrator)
                m_lblBeta->setParent(nullptr);
            //if (m_spbBeta->parentWidget() == m_foldIntegrator)
                m_spbBeta->setParent(nullptr);

            // 根据积分方法切换参数行显示
            if (index == 5) { // HHT
                foldIntegrator->addRow(m_lblAlpha, m_spbAlpha);
                foldIntegrator->addRow(m_lblBeta, m_spbBeta);
            }
            else if (index == 10) { // NEWMARK
                foldIntegrator->addRow(m_lblBeta, m_spbBeta);
            }
            QTimer::singleShot(0, foldIntegrator, [=]() { foldIntegrator->refreshLayout(); });
            });

        connect(m_spbAlpha, &QLineEdit::editingFinished, this, [this]() {
            if (m_motionScenarioData)
                m_motionScenarioData->setIntegratorAlpha(m_spbAlpha->text().toDouble());
            });

        connect(m_spbBeta, &QLineEdit::editingFinished, this, [this]() {
            if (m_motionScenarioData)
                m_motionScenarioData->setIntegratorBeta(m_spbBeta->text().toDouble());
            });

        connect(m_spbIntegMaxIters, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
            if (m_motionScenarioData)
                m_motionScenarioData->setIntegratorMaxIters(val);
            });

        connect(m_spbIntegTol, &QLineEdit::editingFinished, this, [this]() {
            if (m_motionScenarioData)
                m_motionScenarioData->setIntegratorTol(m_spbIntegTol->text().toDouble());
            });

        // --- 求解器高级设置同步 ---
        connect(m_cmbSolverType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_motionScenarioData)
                m_motionScenarioData->setSolverType(static_cast<MechMotionData::SolverType>(index));
            });

        connect(m_spbSolverMaxIters, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
            if (m_motionScenarioData)
                m_motionScenarioData->setSolverMaxIters(val);
            });

        connect(m_spbSolverTol, &QLineEdit::editingFinished, this, [this]() {
            if (m_motionScenarioData)
                m_motionScenarioData->setSolverTolerance(m_spbSolverTol->text().toDouble());
            });

        connect(m_chkSolverVerbose, &QCheckBox::toggled, this, [this](bool checked) {
            if (m_motionScenarioData)
                m_motionScenarioData->setSolverVerbose(checked);
            });

        // ==================== 性能设置（常态化配置，仅做界面展示） ====================
        FoldSection* foldPerf = new FoldSection("性能设置", this);

        // 多线程并行
        QCheckBox* chkMultiThread = new QCheckBox("启用多线程并行计算");
        chkMultiThread->setChecked(true);
        foldPerf->addRow(chkMultiThread);

        QSpinBox* spbThreadCount = new QSpinBox();
        spbThreadCount->setRange(1, 128);
        spbThreadCount->setValue(8);
        spbThreadCount->setSuffix(" 线程");
        foldPerf->addRow(new QLabel("并行线程数:"), spbThreadCount);

        {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            line->setStyleSheet("color: #dcdcdc;");
            foldPerf->addRow(line);
        }

        // CPU 加速
        QCheckBox* chkCpuAccel = new QCheckBox("启用 CPU 指令集加速 (AVX2/SSE4)");
        chkCpuAccel->setChecked(true);
        foldPerf->addRow(chkCpuAccel);

        // GPU 加速
        QCheckBox* chkGpuAccel = new QCheckBox("启用 GPU 加速 (CUDA/OpenCL)");
        chkGpuAccel->setChecked(false);
        foldPerf->addRow(chkGpuAccel);

        // GPU 设备选择（仅做界面）
        QComboBox* cmbGpuDevice = new QComboBox();
        cmbGpuDevice->addItems({ "自动选择", "NVIDIA CUDA", "AMD ROCm", "Intel oneAPI" });
        cmbGpuDevice->setEnabled(false);
        foldPerf->addRow(new QLabel("GPU 后端:"), cmbGpuDevice);

        // GPU 可用时启用下拉框
        connect(chkGpuAccel, &QCheckBox::toggled, cmbGpuDevice, &QComboBox::setEnabled);

        {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            line->setStyleSheet("color: #dcdcdc;");
            foldPerf->addRow(line);
        }

        // 内存优化
        QComboBox* cmbMemMode = new QComboBox();
        cmbMemMode->addItems({ "标准模式", "大内存模式", "核外求解模式", "低内存模式" });
        foldPerf->addRow(new QLabel("内存策略:"), cmbMemMode);

        // 核外求解缓存
        QSpinBox* spbOocCache = new QSpinBox();
        spbOocCache->setRange(1, 64);
        spbOocCache->setValue(4);
        spbOocCache->setSuffix(" GB");
        foldPerf->addRow(new QLabel("核外缓存大小:"), spbOocCache);

        // 矩阵存储格式
        QComboBox* cmbMatrixFmt = new QComboBox();
        cmbMatrixFmt->addItems({ "CSR (压缩稀疏行)", "CSC (压缩稀疏列)", "COO (坐标格式)", "Dense (稠密矩阵)", "Blocked CSR" });
        foldPerf->addRow(new QLabel("稀疏矩阵格式:"), cmbMatrixFmt);

        {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            line->setStyleSheet("color: #dcdcdc;");
            foldPerf->addRow(line);
        }

        // 负载均衡
        QComboBox* cmbLoadBalance = new QComboBox();
        cmbLoadBalance->addItems({ "静态分区", "动态负载均衡", "-guided 调度", "work-stealing" });
        foldPerf->addRow(new QLabel("任务调度策略:"), cmbLoadBalance);

        // 向量化宽度
        QComboBox* cmbSimdWidth = new QComboBox();
        cmbSimdWidth->addItems({ "128-bit (SSE)", "256-bit (AVX/AVX2)", "512-bit (AVX-512)", "自动检测" });
        cmbSimdWidth->setCurrentIndex(3);
        foldPerf->addRow(new QLabel("SIMD 向量宽度:"), cmbSimdWidth);

        m_mainLayout->addWidget(foldPerf);
        m_mainLayout->addStretch();
    }

    void mMotionScenarioPanel::refreshGravityUnitLabel()
    {
        if (m_lblGravityUnit)
            m_lblGravityUnit->setText(gravityUnitString());
    }

    mMotionScenarioPanel::~mMotionScenarioPanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }

    }

    void mMotionScenarioPanel::newMotionScenarioPanel()
    {
        setEditing(true);
        connectSig();

        int motionScenarioID = getCVM->getModelData()->getManager<mMotionScenarioManager>(Physical_MotionScenario).createID();
        m_motionScenarioData = new mMotionScenario(motionScenarioID);

        m_motionScenarioData->setName(getCVM->getModelData()->createNameByType(Physical_MotionScenario, "求解方案"));
        // 数据类默认重力是米制 9.81；按当前全局单位制覆盖（mm 制给 9810 等）
        {
            const double g = standardGravityForSystem(
                getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[0]);
            m_motionScenarioData->setGravity(QVector3D(0, 0, -g));
        }
        getCVM->getModelData()->addPhysicalField<mMotionScenario>(m_motionScenarioData);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionScenarioSig(true, m_motionScenarioData->getId());
    }

    void mMotionScenarioPanel::editMotionScenarioPanel(int id)
    {
        setEditing(true);
        m_motionScenarioData = getCVM->getModelData()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(id);
        if (!m_motionScenarioData)
            return;

        //emit MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);

        connectSig();

        updateUIFromData();

        //恢复重力方向控件的拾取会话：延迟到事件循环下一拍（避开树点击链路后续清理），
        //选择对象模式有拾取数据时重新激活（进面板即可继续拾取并显示高亮）
        QTimer::singleShot(0, this, [this]() {
            if (!m_motionScenarioData)
                return;
            restorePickSessionsOnEntry(QList<mPointPickerWidget*>(), { m_gravityDirSelector });
            });
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionScenarioPanel::onMotionScenarioDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_motionScenarioData == nullptr || id != m_motionScenarioData->getId())
                return;
        }
        m_motionScenarioData = nullptr;
    }

    void mMotionScenarioPanel::uneditMotionScenarioPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_motionScenarioData = nullptr;
        //退出编辑时失活方向控件内部拾取并清理高亮
        if (m_gravityDirSelector)
        {
            const QList<mPickWidgetController*> controllers = {
                m_gravityDirSelector->startPickController(),
                m_gravityDirSelector->endPickController(),
                m_gravityDirSelector->objectPickController() };
            for (auto* controller : controllers)
            {
                if (controller)
                {
                    controller->setPickActive(false);
                    controller->clearAllHighlight();
                }
            }
        }
        disconnectSig();
    }

    void mMotionScenarioPanel::onStartSolve(int scenarioId)
    {
        // 平台触发的求解（经 mpcomm）不弹模态进度框，避免后台求解与
        // 主界面状态冲突导致崩溃；手动求解仍弹进度框。
        const bool fromMpcomm = qApp->property("mpcomm_solve_requested").toBool();
        qApp->setProperty("mpcomm_solve_requested", false);

        if (scenarioId <= 0)
        {
            auto ids = getCVM->getModelData()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getAllIDs();
            if (ids.empty())
            {
                return;
            }
            scenarioId = *ids.begin();
        }

        // 求解前自动导出当前运动数据（.motion.json），并上报平台
        // mesh+setup：用户未手动导入 json 时，求解也能有输入、平台单元格能更新。
        {
            const QString dir = QCoreApplication::applicationDirPath() + "/../WorkingDir";
            QDir().mkpath(dir);
            const QString jsonPath = QString("%1/motion_setup_scenario%2.motion.json")
                .arg(dir).arg(scenarioId);
            MechIOFile::mIOFileManager::getInstance()
                ->slot_exportMotionJsonFile(jsonPath, true);
        }

        // 1. 弹出一个带进度条的对话框（仅手动求解）
        QProgressDialog* progressDlg = nullptr;
        if (!fromMpcomm) {
            progressDlg = new QProgressDialog("正在进行多体动力学求解...", "中止", 0, 100, this);
            progressDlg->setWindowTitle("求解计算");
            progressDlg->setWindowModality(Qt::WindowModal); // 锁死主界面，防止用户乱点
            progressDlg->setMinimumDuration(0); // 立即显示
            progressDlg->setValue(0);
        }

        // 2. 创建后台求解线程 (传入你要计算的 scenarioId)
        // 即使当前面板显示的不是这个 id，只要传进去，线程就会自己去 Manager 里找对应的数据算！
        // 【关键】在 UI 线程把当前视口模型指针捕获下来传给求解线程，
        // 防止求解期间应用失焦导致全局 getCVM 变为 nullptr 引发崩溃。
        mMotionSolverThread* solverThread = new mMotionSolverThread(scenarioId, getCVM, this);

        // 3. 定时轮询进度（物理引擎回调线程不是 Qt 线程，无法安全 emit）
        QTimer* pollTimer = new QTimer(this);
        connect(pollTimer, &QTimer::timeout, this, [=]() {
            if (progressDlg) {
                progressDlg->setValue(solverThread->m_currentProgress.load());
            }
        });
        pollTimer->start(100); // 每 100ms 刷新一次

        // 回报平台（mpcomm）求解已开始
        emit MechCommon::mMechGlobalSignals::getInstance()
            ->mpcommSolverStatusChanged(QStringLiteral("started"), {});

        // 允许用户点击中止按钮停止计算
        if (progressDlg) {
            connect(
                progressDlg,
                &QProgressDialog::canceled,
                this,
                [solverThread, progressDlg]()
                {
                    progressDlg->setLabelText(QStringLiteral("正在中止求解..."));

                    // 防止用户连续点击中止按钮
                    progressDlg->setCancelButton(nullptr);

                    solverThread->requestStopSolve();
                });
        }

        // 接收线程打印的日志 (可以输出到你软件底部的 Output 窗口)
        connect(solverThread, &mMotionSolverThread::messageLogged, this, [](const QString& msg) {
             emit mGlobalSignals::getInstance()->outputMessageSig(msg);
            }, Qt::QueuedConnection);

        // 4. 线程结束后的收尾工作
        connect(
            solverThread,
            &mMotionSolverThread::solveFinished,
            this,
            [=](int sId, bool success)
            {
                pollTimer->stop();
                pollTimer->deleteLater();

                if (progressDlg) {
                    progressDlg->close();
                    progressDlg->deleteLater();
                }

                emit MechCommon::mMechGlobalSignals::getInstance()
                    ->createUpdateMotionScenarioSig(false, sId);

                // 求解成功后把结果落盘成文件（平台上报 resultFiles 需要完成证明）。
                QString resultPath;
                if (success) {
                    resultPath = exportMotionResultToFile(sId);
                }

                // 回报平台（mpcomm）求解完成/失败（走 changed/committed 协议）
                QVariantMap outputs;
                if (!resultPath.isEmpty()) {
                    outputs["resultPath"] = resultPath;
                }
                emit MechCommon::mMechGlobalSignals::getInstance()
                    ->mpcommSolverStatusChanged(
                        success ? QStringLiteral("finished") : QStringLiteral("failed"),
                        outputs);

                // 平台触发的求解（无界面交互）下不弹模态框，避免后台
                // 弹窗与主窗口状态冲突导致崩溃；手动求解仍弹提示。
                if (!fromMpcomm) {
                    if (success)
                    {
                        QMessageBox::information(
                            this,
                            QStringLiteral("求解完成"),
                            QStringLiteral("动力学计算成功，已生成结果数据！"));
                    }
                    else
                    {
                        QMessageBox::warning(
                            this,
                            QStringLiteral("求解结束"),
                            QStringLiteral("计算被中止或发生错误。"));
                    }
                }
            },
            Qt::QueuedConnection);

        connect(
            solverThread,
            &QThread::finished,
            solverThread,
            &QObject::deleteLater);

        // 5. 正式启动后台计算！
        solverThread->start();

        //solverThread->doTransInput();
        //solverThread->doSolve();

    }

    QString mMotionScenarioPanel::exportMotionResultToFile(int scenarioId)
    {
        // 取求解方案与结果
        auto* scenario = getCVM->getModelData()
            ->getManager<mMotionScenarioManager>(Physical_MotionScenario)
            .getData(scenarioId);
        if (!scenario || !scenario->hasResult()) {
            return QString();
        }

        MechMotionData::mMotionResultData* result = scenario->getResult();
        if (!result || result->m_frames.empty()) {
            return QString();
        }

        // 结果文件写到工作目录，文件名带方案 id 避免覆盖。
        QString dir = QCoreApplication::applicationDirPath() + "/../WorkingDir";
        QDir().mkpath(dir);
        const QString path = QString("%1/motion_result_scenario%2.csv")
            .arg(dir).arg(scenarioId);

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return QString();
        }

        QTextStream out(&file);
        // 表头：时间 + 每体(位移x/y/z, 速度x/y/z)
        out << "# MxMotion result, scenario=" << scenarioId << "\n";
        out << "time,bodyId,posX,posY,posZ,velX,velY,velZ,dispX,dispY,dispZ\n";

        for (const auto& frame : result->m_frames) {
            for (const auto& kv : frame.m_bodyFrames) {
                const int bodyId = kv.first;
                const auto& bf = kv.second;
                out << frame.m_time << "," << bodyId << ","
                    << bf.m_position.x() << "," << bf.m_position.y() << ","
                    << bf.m_position.z() << ","
                    << bf.m_velocity.x() << "," << bf.m_velocity.y() << ","
                    << bf.m_velocity.z() << ","
                    << bf.m_displacement.x() << "," << bf.m_displacement.y() << ","
                    << bf.m_displacement.z() << "\n";
            }
        }

        file.close();
        return QFileInfo(path).absoluteFilePath();
    }

    void mMotionScenarioPanel::updateUIFromData()
    {
        if (!getCVM)
        {
            return;
        }

        if (!m_motionScenarioData)
            return;

        const QSignalBlocker bEnd(m_spbEndTime);
        m_spbEndTime->setExpression(QString::number(m_motionScenarioData->getEndTime()));

        const QSignalBlocker bStep(m_spbStepSize);
        m_spbStepSize->setExpression(QString::number(m_motionScenarioData->getStepSize()));

        // 还原预计帧数显示
        if (m_motionScenarioData->getStepSize() > 0) {
            int steps = static_cast<int>(std::ceil(m_motionScenarioData->getEndTime() / m_motionScenarioData->getStepSize()));
            m_lblTotalSteps->setText(QString("预计输出帧数: %1 帧").arg(steps));
        }

        // 还原重力（方向 + 大小拆分反显；方向控件拾取会话模式保留）
        refreshGravityWidget();

        const QSignalBlocker bPresetMode(m_cmbPresetMode);
        // 按枚举值定位条目，避免依赖顺序
        int presetIndex = m_cmbPresetMode->findData(static_cast<int>(m_motionScenarioData->getPresetMode()));
        if (presetIndex >= 0)
            m_cmbPresetMode->setCurrentIndex(presetIndex);

        const QSignalBlocker bIntegAdv(m_cmbIntegratorAdv);
        m_cmbIntegratorAdv->setCurrentIndex(static_cast<int>(m_motionScenarioData->getIntegratorType()));

        const QSignalBlocker bAlpha(m_spbAlpha);
        m_spbAlpha->setExpression(QString::number(m_motionScenarioData->getIntegratorAlpha()));

        const QSignalBlocker bBeta(m_spbBeta);
        m_spbBeta->setExpression(QString::number(m_motionScenarioData->getIntegratorBeta()));

        const QSignalBlocker bIntegIters(m_spbIntegMaxIters);
        m_spbIntegMaxIters->setValue(m_motionScenarioData->getIntegratorMaxIters());

        const QSignalBlocker bIntegTol(m_spbIntegTol);
        m_spbIntegTol->setExpression(QString::number(m_motionScenarioData->getIntegratorTol()));

        const QSignalBlocker bSolverType(m_cmbSolverType);
        m_cmbSolverType->setCurrentIndex(static_cast<int>(m_motionScenarioData->getSolverType()));

        const QSignalBlocker bSolverIters(m_spbSolverMaxIters);
        m_spbSolverMaxIters->setValue(m_motionScenarioData->getSolverMaxIters());

        const QSignalBlocker bSolverTol(m_spbSolverTol);
        m_spbSolverTol->setExpression(QString::number(m_motionScenarioData->getSolverTolerance()));

        const QSignalBlocker bSolverVerbose(m_chkSolverVerbose);
        m_chkSolverVerbose->setChecked(m_motionScenarioData->getSolverVerbose());

        m_chkUsePreset->setChecked(m_motionScenarioData->getUsePreset());

        // 触发一次预设逻辑，确保布局正确
        m_chkUsePreset->toggled(m_motionScenarioData->getUsePreset());
    }

    void mMotionScenarioPanel::connectSig()
    {
        disconnectSig();
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionScenarioPanel::slot_finishedPick);

        // 全局单位制切换：所有方案的重力按长度单位比换算（加速度=长度/秒²，时间恒为秒）
        if (getCVM->getModelData()) {
            //connect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitSystemChanged,
            //    this, &mMotionScenarioPanel::slot_unitSystemResponse, Qt::UniqueConnection);
        }
    }

    void mMotionScenarioPanel::disconnectSig()
    {
        if (conn_geo)
            disconnect(conn_geo);
        if (getCVM && getCVM->getModelData()) {
            //disconnect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitSystemChanged,
            //    this, &mMotionScenarioPanel::slot_unitSystemResponse);
        }
    }

    void mMotionScenarioPanel::slot_unitSystemResponse(int)
    {
        if (!getCVM || !getCVM->getModelData()) return;

        // GUI 本地换算：加速度单位与长度单位同构，比值 = 旧制长度因子 / 新制长度因子
        const int fromIdx = getCVM->getModelData()->getUnitSystemManager()->getHisUnitsIndex()[0];
        const int toIdx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[0];
        const double ratio = lengthFactorForSystem(fromIdx) / lengthFactorForSystem(toIdx);
        if (std::abs(ratio - 1.0) < 1e-12) {
            refreshGravityUnitLabel();
            return;
        }

        auto& mgr = getCVM->getModelData()->getManager<mMotionScenarioManager>(Physical_MotionScenario);
        for (int sid : mgr.getAllIDs()) {
            auto* sc = mgr.getData(sid);
            if (!sc) continue;
            const QVector3D g = sc->getGravity();
            sc->setGravity(g * ratio);
        }

        refreshGravityUnitLabel();
        if (m_motionScenarioData) {
            refreshGravityWidget();
        }
    }

    void mMotionScenarioPanel::refreshGravityWidget()
    {
        if (!m_motionScenarioData || !m_gravityDirSelector)
            return;
        const QVector3D gravity = m_motionScenarioData->getGravity();
        const double mag = gravity.length();
        const QVector3D dir = mag > 1e-12 ? gravity / static_cast<float>(mag) : QVector3D(0, 0, -1);
        const bool blocked = m_gravityDirSelector->blockSignals(true);
        // 方向走数据类里的拾取会话状态（照集中力 dirPickState_ 模式），
        // 无记录（新建/历史数据）回"向量"并写入方向；大小行始终刷新
        restoreDirPickState(m_gravityDirSelector, m_motionScenarioData->gravityPickState_, dir);
        m_gravityDirSelector->setMagnitudeValue(QString::number(mag));
        m_gravityDirSelector->blockSignals(blocked);
    }

    void mMotionScenarioPanel::slot_finishedPick()
    {
        if (!getCVM)
        {
            return;
        }
        //重置面板，类型根据拾取模式来确定
        int pickNum = 0;
        auto pickFilter = getCVM->getCurrentPickDataType(pickNum);
        if (pickFilter == PickDataType::None)
        {
            return;
        }
    }

    void mMotionScenarioPanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
    {
        if (!getCVM)
        {
            return;
        }
        //if (getCVM == _currentViewportModel)
        //{
        //    return;
        //}

        ////获取当前视口的数据
        //_currentViewportModel = getCVM;
    }
}
