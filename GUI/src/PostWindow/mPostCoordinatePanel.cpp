#include "mPostCoordinatePanel.h"


//MxFunctions 公共控件（PostWindow 隔离禁令下以库内控件组合替代前处理
//mPointPickerWidget/mDirectionSelectorWidget——两者均在 MainWindow 模块下）
#include "mChoiceGroup.h"
#include "mCheckComboBox.h"
#include "mGroupBox.h"
#include "mGeneralPickButton.h"
#include "mAxisCoordRow.h"
#include "UiMetrics.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

//GUI 公共
#include "mMechGlobalSignals.h"     //createUpdateCoordinateSig（树"坐标系"分支重建总线）

//QT
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>

namespace
{
    //当前语言（0=中文，对齐 MAppConfiguration::_language 口径）
    bool isCN()
    {
        return MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
    }

    //全局单位轴（方向行"全局X/Y/Z轴"档位的矢量取值）
    const QVector3D kUnitAxes[3] = { QVector3D(1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, 0, 1) };
}

namespace PostWindow
{
    /*
    * ============================ 组合行 ============================
    */

    void mPostCoordinatePanel::PointRow::setCoord(const QVector3D& v)
    {
        if (coord)
            coord->setValues(v.x(), v.y(), v.z());
    }

    QVector3D mPostCoordinatePanel::PointRow::coordValue() const
    {
        if (coord == nullptr)
            return QVector3D();
        double x = 0, y = 0, z = 0;
        coord->values(x, y, z);
        return QVector3D(float(x), float(y), float(z));
    }

    QVector3D mPostCoordinatePanel::DirRow::vectorValue() const
    {
        const int idx = combo ? combo->currentIndex() : 0;
        QVector3D v;
        if (idx >= 0 && idx < 3)
        {
            v = kUnitAxes[idx];
        }
        else if (coord)
        {
            double x = 0, y = 0, z = 0;
            coord->values(x, y, z);
            v = QVector3D(float(x), float(y), float(z));
        }
        if (reverse && reverse->isChecked())
            v = -v;
        return v;
    }

    /*
    * ============================ 构造 ============================
    */

    mPostCoordinatePanel::mPostCoordinatePanel(QWidget* parent) : mPanelBase(parent)
    {
        setupUI();
        connectUI();

        //初始定义方式=三点（原型口径）：归位行显隐/标题/快速创建行
        updatePosFoldLayout();

        //语言切换全量刷新（构造后按当前语言重发一遍文案，保证非中文环境首帧正确）
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)),
            this, SLOT(slot_changeLanguage(int)));
        slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

        //Esc 退出拾取武装态（纯 UI；功能接入后走真实退拾取链）
        bindShortcut(Qt::Key_Escape, [this]() { resetPickArmedState(false); });
    }

    void mPostCoordinatePanel::setupUI()
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(4, 6, 4, 0);
        _mainLayout->setSpacing(MxFunctions::UiMetrics::rowGap());

        /* ========== 基本信息折叠框 ========== */
        _foldBasic = new FoldSection(QStringLiteral(u"基本信息"), this);

        // 名称（默认名随新建生成；空名红框校验，重名校验待功能接入补）
        _nameEdit = new QLineEdit(this);
        _nameEdit->setObjectName(QStringLiteral("coordNameEdit"));
        _nameEdit->setPlaceholderText(QStringLiteral(u"坐标系名称"));
        _nameLabel = new PanelLabel(QStringLiteral(u"名称"));
        _foldBasic->addRow(_nameLabel, _nameEdit);

        // 名称校验错误行（名称下方跨列，按需插拔不占行位）
        _nameErrorLabel = new QLabel(this);
        _nameErrorLabel->setObjectName(QStringLiteral("coordNameError"));
        _nameErrorLabel->setStyleSheet(QStringLiteral("color:#d95151;font-size:11px;"));
        _foldBasic->addRow(_nameErrorLabel);
        _foldBasic->setRowVisible(_nameErrorLabel, false);

        // 类型（直角/柱/球；离散坐标系不在后处理常规类型之列）
        _comboType = new MxFunctions::mCheckComboBox(this);
        _comboType->setObjectName(QStringLiteral("coordTypeCombo"));
        _comboType->addItems({ QStringLiteral(u"直角坐标系"), QStringLiteral(u"柱坐标系"), QStringLiteral(u"球坐标系") });
        _typeLabel = new PanelLabel(QStringLiteral(u"类型"));
        _foldBasic->addRow(_typeLabel, _comboType);

        // 定义方式（三点/基准轴）
        _defineChoice = new MxFunctions::mChoiceGroup({ QStringLiteral(u"三点"), QStringLiteral(u"基准轴") }, this);
        _defineLabel = new PanelLabel(QStringLiteral(u"定义方式"));
        _foldBasic->addRow(_defineLabel, _defineChoice);

        /*
        * 快速创建（默认勾选：拾取到点自动应用并接力下一个拾取）：
        * 跨列行内按 标签列宽+列间距 缩进，与定义方式 mChoiceGroup 左对齐
        */
        _quickCreate = new QCheckBox(QStringLiteral(u"快速创建"), this);
        _quickCreate->setObjectName(QStringLiteral("coordQuickCreate"));
        _quickCreate->setToolTip(QStringLiteral(u"拾取到点后自动应用，并按 原点→轴正向点→平面点 接力"));
        _quickCreate->setChecked(true);
        _quickRow = new QWidget(this);
        _quickRow->setObjectName(QStringLiteral("coordQuickRow"));
        QHBoxLayout* quickLay = new QHBoxLayout(_quickRow);
        quickLay->setContentsMargins(0, 0, 0, 0);
        quickLay->setSpacing(0);
        quickLay->addSpacing(MxFunctions::UiMetrics::labelWidth() + MxFunctions::UiMetrics::columnGap());
        quickLay->addWidget(_quickCreate);
        _foldBasic->addRow(_quickRow);

        m_foldWidgets.append(_foldBasic);
        _mainLayout->addWidget(_foldBasic);

        /* ========== 位置与方向折叠框 ========== */
        _foldPos = new FoldSection(QStringLiteral(u"位置与方向"), this);

        // 坐标系主轴（X/Y/Z 轴，仅直角显示；柱/球固定 Z 为主轴）
        _mainAxisChoice = new MxFunctions::mChoiceGroup({ QStringLiteral(u"X轴"), QStringLiteral(u"Y轴"), QStringLiteral(u"Z轴") }, this);
        _mainAxisChoice->setObjectName(QStringLiteral("coordMainAxis"));
        _mainAxisLabel = new PanelLabel(QStringLiteral(u"坐标系主轴"));
        _foldPos->addRow(_mainAxisLabel, _mainAxisChoice);

        // 第二个轴（基准轴方式；选项随坐标系主轴联动为其余两轴）
        _secondAxisChoice = new MxFunctions::mChoiceGroup({ QStringLiteral(u"Y轴"), QStringLiteral(u"Z轴") }, this);
        _secondAxisChoice->setObjectName(QStringLiteral("coordSecondAxis"));
        _secondAxisLabel = new PanelLabel(QStringLiteral(u"第二个轴"));
        _foldPos->addRow(_secondAxisLabel, _secondAxisChoice);

        // 参考平面（XOY/XOZ，三点方式；选项随主轴联动）
        _planeChoice = new MxFunctions::mChoiceGroup({ QStringLiteral(u"XOY"), QStringLiteral(u"XOZ") }, this);
        _planeChoice->setObjectName(QStringLiteral("coordPlane"));
        _planeLabel = new PanelLabel(QStringLiteral(u"参考平面"));
        _foldPos->addRow(_planeLabel, _planeChoice);

        // 点拾取组合行（外层 mGroupBox 标题=业务名，随类型/主轴联动）
        auto makePointRow = [this](PointRow& row, const QString& objectStem) {
            row.group = new MxFunctions::mGroupBox(QString(), QPixmap(), QPixmap(), this);
            row.group->setObjectName(objectStem + QStringLiteral("Group"));
            QGridLayout* grid = row.group->grid();
            grid->setVerticalSpacing(MxFunctions::UiMetrics::rowGap());

            row.pickBtn = new MxFunctions::mGeneralPickButton(MxFunctions::mGeneralPickButton::JoinMode::Standalone, row.group);
            row.pickBtn->setObjectName(objectStem + QStringLiteral("Pick"));
            row.pickBtn->setCheckable(true);

            row.coord = new MxFunctions::mAxisCoordRow(row.group);
            row.coord->setObjectName(objectStem + QStringLiteral("Coord"));

            row.applyBtn = new QPushButton(QStringLiteral(u"应用"), row.group);
            row.applyBtn->setObjectName(objectStem + QStringLiteral("Apply"));
            row.applyBtn->setFixedSize(40, MxFunctions::UiMetrics::baseH());
            row.applyBtn->setToolTip(QStringLiteral(u"确认当前坐标输入（快速创建勾选时拾取即应用）"));

            grid->addWidget(row.pickBtn, 0, 0);
            grid->addWidget(row.coord, 1, 0);
            grid->addWidget(row.applyBtn, 2, 0, Qt::AlignLeft);
            };
        makePointRow(_originRow, QStringLiteral("coordOrigin"));
        makePointRow(_pt1Row, QStringLiteral("coordPt1"));
        makePointRow(_pt2Row, QStringLiteral("coordPt2"));

        // 方向组合行（基准轴方式；下拉档=全局X/Y/Z轴/自定义矢量，矢量三格仅自定义档显示）
        auto makeDirRow = [this](DirRow& row, const QString& objectStem) {
            row.group = new MxFunctions::mGroupBox(QString(), QPixmap(), QPixmap(), this);
            row.group->setObjectName(objectStem + QStringLiteral("Group"));
            QGridLayout* grid = row.group->grid();
            grid->setVerticalSpacing(MxFunctions::UiMetrics::rowGap());

            row.combo = new MxFunctions::mCheckComboBox(row.group);
            row.combo->setObjectName(objectStem + QStringLiteral("Combo"));
            row.combo->addItems({ QStringLiteral(u"全局X轴"), QStringLiteral(u"全局Y轴"), QStringLiteral(u"全局Z轴"), QStringLiteral(u"自定义矢量") });

            row.coord = new MxFunctions::mAxisCoordRow(row.group);
            row.coord->setObjectName(objectStem + QStringLiteral("Coord"));
            row.coord->setVisible(false);   //默认全局X轴档

            row.reverse = new QCheckBox(QStringLiteral(u"反向"), row.group);
            row.reverse->setObjectName(objectStem + QStringLiteral("Reverse"));

            grid->addWidget(row.combo, 0, 0);
            grid->addWidget(row.coord, 1, 0);
            grid->addWidget(row.reverse, 2, 0, Qt::AlignLeft);
            };
        makeDirRow(_dir1Row, QStringLiteral("coordDir1"));
        makeDirRow(_dir2Row, QStringLiteral("coordDir2"));

        _foldPos->addRow(_originRow.group);
        _foldPos->addRow(_pt1Row.group);
        _foldPos->addRow(_pt2Row.group);
        _foldPos->addRow(_dir1Row.group);
        _foldPos->addRow(_dir2Row.group);

        // 球坐标 θ/φ 说明行（仅球类型显示）
        _sphereNoteLabel = new QLabel(this);
        _sphereNoteLabel->setObjectName(QStringLiteral("coordSphereNote"));
        _sphereNoteLabel->setStyleSheet(QStringLiteral("color:#788391;font-size:11px;"));
        _sphereNoteLabel->setWordWrap(true);
        _foldPos->addRow(_sphereNoteLabel);
        _foldPos->setRowVisible(_sphereNoteLabel, false);

        m_foldWidgets.append(_foldPos);
        _mainLayout->addWidget(_foldPos);

        _mainLayout->addStretch();
    }

    void mPostCoordinatePanel::connectUI()
    {
        //名称：即时校验（空名/重名红框+错误行）；回车提交（查重后会话记录改名）；
        //textChanged 同时作为用户输入痕迹（新建草稿复用判定，程序化回填均屏蔽信号）
        connect(_nameEdit, &QLineEdit::textChanged, this, &mPostCoordinatePanel::slot_onNameTextChanged);
        connect(_nameEdit, &QLineEdit::textChanged, this, [this]() { markDraftTouched(); });
        connect(_nameEdit, &QLineEdit::editingFinished, this, &mPostCoordinatePanel::slot_commitName);

        //类型切换：per-type 草稿保存/恢复 + 行显隐重排
        connect(_comboType, &MxFunctions::mCheckComboBox::currentIndexChanged, this, &mPostCoordinatePanel::slot_onTypeChanged);

        //定义方式切换（三点<->基准轴：方式级缓存保存/恢复）
        connect(_defineChoice, &MxFunctions::mChoiceGroup::currentTextChanged, this, &mPostCoordinatePanel::slot_onDefineChanged);

        //主轴/参考平面/第二轴联动
        connect(_mainAxisChoice, &MxFunctions::mChoiceGroup::currentTextChanged, this, &mPostCoordinatePanel::slot_onMainAxisChanged);
        connect(_planeChoice, &MxFunctions::mChoiceGroup::currentTextChanged, this, &mPostCoordinatePanel::slot_onPlaneChoiceChanged);
        connect(_secondAxisChoice, &MxFunctions::mChoiceGroup::currentTextChanged, this, &mPostCoordinatePanel::slot_onSecondAxisChanged);

        //快速创建勾选态：切换各点行应用钮显隐（勾选=拾取即应用，无需手动点）
        connect(_quickCreate, &QCheckBox::toggled, this, [this](bool) { updateQuickRowVisibility(); });

        /*
        * 拾取钮武装态（纯 UI 互斥点亮；视口拾取链路——PickFilter/FinishPick/
        * 节点坐标回填——为后续功能接入，届时在 armed 分支挂接）
        */
        auto bindPickButton = [this](PointRow& row, QVector<PointRow*> others) {
            connect(row.pickBtn, &QPushButton::clicked, this, [this, &row, others](bool checked) {
                row.pickBtn->setChecked(checked);
                if (checked)
                {
                    for (PointRow* other : others)
                    {
                        if (other && other != &row && other->pickBtn && other->pickBtn->isChecked())
                        {
                            other->pickBtn->setChecked(false);
                            other->pickBtn->setText(pickButtonLabel(false));
                        }
                    }
                }
                row.pickBtn->setText(pickButtonLabel(checked));
                });
            };
        bindPickButton(_originRow, { &_pt1Row, &_pt2Row });
        bindPickButton(_pt1Row, { &_originRow, &_pt2Row });
        bindPickButton(_pt2Row, { &_originRow, &_pt1Row });

        //方向档位切换：自定义矢量三格仅"自定义矢量"档显示；用户输入痕迹标记
        auto bindDirCombo = [this](DirRow& row) {
            connect(row.combo, &MxFunctions::mCheckComboBox::currentIndexChanged, this, [&row](int index) {
                if (row.coord)
                    row.coord->setVisible(index == 3);
                });
            connect(row.combo, &MxFunctions::mCheckComboBox::currentIndexChanged, this, [this](int) {
                markDraftTouched();
                });
            };
        bindDirCombo(_dir1Row);
        bindDirCombo(_dir2Row);

        //反向/自定义矢量编辑：用户输入痕迹标记（真实数据写入随数据层立项接入）
        for (DirRow* row : { &_dir1Row, &_dir2Row })
        {
            connect(row->reverse, &QCheckBox::toggled, this, [this](bool) { markDraftTouched(); });
            connect(row->coord, &MxFunctions::mAxisCoordRow::editingFinished, this, [this]() { markDraftTouched(); });
        }

        //点行坐标手输回车：用户输入痕迹标记（拾取链接入后由拾取回填路径同走标记）
        for (PointRow* row : { &_originRow, &_pt1Row, &_pt2Row })
        {
            connect(row->coord, &MxFunctions::mAxisCoordRow::editingFinished, this, [this]() { markDraftTouched(); });
        }

        //应用钮：确认当前行输入（快速创建勾选时拾取即应用，按钮隐藏）
        auto bindApply = [this](PointRow& row) {
            connect(row.applyBtn, &QPushButton::clicked, this, [this]() { markDraftTouched(); });
            };
        bindApply(_originRow);
        bindApply(_pt1Row);
        bindApply(_pt2Row);
    }

    /*
    * ============================ 新建 ============================
    */

    void mPostCoordinatePanel::newCoordinatePanel(int coordType)
    {
        const int typeIdx = qBound(0, coordType, 2);

        /*
        * 复用守卫（保留并发演进语义）：面板正编辑"未提交过编辑"的新建草稿时不再新建，
        * 把草稿重置为所点类型——重复点击创建入口（含直角/柱/球间切换）不连建多个
        * "坐标系N"；已编辑过的草稿正常另起新建
        */
        if (isEditing() && _isNewDraft && !_draftTouched && _curCoordId >= 0)
        {
            if (CoordRecord* rec = recordById(_curCoordId))
                rec->typeIdx = typeIdx;
            _typeDrafts.clear();
            _methodThreePoints.clear();
            _methodBasisValid = false;
            _methodBasisDir1 = _methodBasisDir2 = 0;
            _methodBasisRev1 = _methodBasisRev2 = false;
            _methodBasisXLine = _methodBasisYLine = QVector3D();
            fillNameAndType(_nameEdit->text(), typeIdx);
            resetInputDefaults();
            resetPickArmedState(false);
            updatePosFoldLayout();
            _draftTouched = false;
            return;
        }

        /*
        * 会话记录登记（界面操作数据载体）：后处理与前处理数据完全分离（用户裁定
        * 2026-11-18，禁止复用前处理 MechCoordinateData 格式），真实数据格式后续立项——
        * 届时把本段替换为后处理数据层创建即可，树/编辑链无需变动
        */
        CoordRecord rec;
        rec.id = _nextCoordId++;
        rec.name = QStringLiteral(u"坐标系_%1").arg(rec.id);
        rec.typeIdx = typeIdx;
        _records.append(rec);
        _curCoordId = rec.id;
        setEditing(true);
        _isNewDraft = true;
        _draftTouched = true;   //先按已编辑计，下方归一化后复位为草稿态

        //新建=全新输入：复位 per-type 草稿/方式级缓存（对齐参考实现"每次新建独立数据"）
        _typeDrafts.clear();
        _methodThreePoints.clear();
        _methodBasisValid = false;
        _methodBasisDir1 = _methodBasisDir2 = 0;
        _methodBasisRev1 = _methodBasisRev2 = false;
        _methodBasisXLine = _methodBasisYLine = QVector3D();

        fillNameAndType(rec.name, typeIdx);
        resetInputDefaults();
        _draftTouched = false;   //归一化复位不算用户编辑，草稿回到"未编辑"可复用态

        //拾取武装态复位后归位首个可见拾取钮
        resetPickArmedState(false);
        updatePosFoldLayout();

        mGlobalSignals::getInstance()->outputMessageSig(
            QString::fromUtf8("成功创建坐标系 %1").arg(rec.name), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCoordinateSig(true, rec.id);
    }

    void mPostCoordinatePanel::editCoordinatePanel(int coordinateId)
    {
        /*
        * 全局坐标系（id=-1）：只读展示——基本信息禁用、位置与方向隐藏
        * （固有定义不提供编辑；无会话记录，面板按固定口径呈现）
        */
        if (coordinateId == -1)
        {
            _curCoordId = -1;
            setEditing(true);
            _isNewDraft = false;   //全局只读不参与新建复用
            _draftTouched = false;
            _typeDrafts.clear();
            _methodThreePoints.clear();
            _methodBasisValid = false;
            _methodBasisDir1 = _methodBasisDir2 = 0;
            _methodBasisRev1 = _methodBasisRev2 = false;
            _methodBasisXLine = _methodBasisYLine = QVector3D();
            fillNameAndType(isCN() ? QStringLiteral(u"全局坐标系") : QStringLiteral("Global CS"), 0);
            resetInputDefaults();
            _foldBasic->setEnabled(false);
            _foldPos->setVisible(false);
            resetPickArmedState(false);
            updatePosFoldLayout();
            return;
        }

        const CoordRecord* rec = recordById(coordinateId);
        if (rec == nullptr)
            return;

        _curCoordId = rec->id;
        setEditing(true);
        _isNewDraft = false;   //编辑既有记录，不参与新建复用
        _draftTouched = false;

        //进入编辑=全新草稿（跨编辑不保留上一对象的输入缓存）
        _typeDrafts.clear();
        _methodThreePoints.clear();
        _methodBasisValid = false;
        _methodBasisDir1 = _methodBasisDir2 = 0;
        _methodBasisRev1 = _methodBasisRev2 = false;
        _methodBasisXLine = _methodBasisYLine = QVector3D();

        fillNameAndType(rec->name, rec->typeIdx);
        resetInputDefaults();

        _foldBasic->setEnabled(true);
        _foldPos->setVisible(true);
        resetPickArmedState(false);
        updatePosFoldLayout();
    }

    void mPostCoordinatePanel::uneditCoordinatePanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        _curCoordId = -1;
        _isNewDraft = false;   //退出编辑后草稿状态失效（下次新建不复用旧记录）
        _draftTouched = false;
        //面板控件保留最后状态（下次 new/edit 走 fillNameAndType+resetInputDefaults 全量重置）
    }

    void mPostCoordinatePanel::onCoordinateRenamedExternally(int coordinateId, const QString& newName)
    {
        CoordRecord* rec = recordById(coordinateId);
        if (rec == nullptr)
            return;
        rec->name = newName;
        if (_curCoordId != coordinateId)
            return;
        //面板正编辑该记录：名称行跟随（屏蔽信号防触发校验/提交），再重估校验行
        _nameEdit->blockSignals(true);
        _nameEdit->setText(newName);
        _nameEdit->blockSignals(false);
        slot_onNameTextChanged(newName);
    }

    /*
    * ============================ 会话记录访问（树联动单一数据源） ============================
    */

    mPostCoordinatePanel::CoordRecord* mPostCoordinatePanel::recordById(int id)
    {
        for (CoordRecord& rec : _records)
            if (rec.id == id)
                return &rec;
        return nullptr;
    }

    const mPostCoordinatePanel::CoordRecord* mPostCoordinatePanel::recordById(int id) const
    {
        for (const CoordRecord& rec : _records)
            if (rec.id == id)
                return &rec;
        return nullptr;
    }

    bool mPostCoordinatePanel::coordinateNameExists(const QString& name, int excludeId) const
    {
        for (const CoordRecord& rec : _records)
            if (rec.id != excludeId && rec.name == name)
                return true;
        return false;
    }

    QVector<QPair<int, QString>> mPostCoordinatePanel::coordinateTreeRecords() const
    {
        QVector<QPair<int, QString>> out;
        out.reserve(_records.size());
        for (const CoordRecord& rec : _records)
            out.append({ rec.id, rec.name });
        return out;
    }

    bool mPostCoordinatePanel::deleteCoordinateRecords(const QList<int>& ids)
    {
        bool removed = false;
        for (int id : ids)
        {
            for (auto it = _records.begin(); it != _records.end(); ++it)
            {
                if (it->id == id)
                {
                    //编辑对象被删时先退出编辑态（当前编辑引用即将失效）
                    if (_curCoordId == id)
                    {
                        setEditing(false);
                        _curCoordId = -1;
                        _isNewDraft = false;
                        _draftTouched = false;
                    }
                    _records.erase(it);
                    removed = true;
                    break;
                }
            }
        }
        if (removed)
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCoordinateSig(false, -1);
        return removed;
    }

    int mPostCoordinatePanel::copyCoordinateRecord(int id)
    {
        const CoordRecord* src = recordById(id);
        if (src == nullptr)
            return -1;

        CoordRecord rec;
        rec.id = _nextCoordId++;
        //副本命名：源名_副本 起步，重名递增后缀
        QString base = src->name + QStringLiteral(u"_副本");
        QString name = base;
        for (int n = 2; coordinateNameExists(name, -1); ++n)
            name = base + QString("_%1").arg(n);
        rec.name = name;
        rec.typeIdx = src->typeIdx;
        _records.append(rec);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCoordinateSig(true, rec.id);
        return rec.id;
    }

    void mPostCoordinatePanel::clearCoordinateRecords()
    {
        if (_records.isEmpty())
            return;
        setEditing(false);
        _curCoordId = -1;
        _isNewDraft = false;
        _draftTouched = false;
        _records.clear();
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCoordinateSig(false, -1);
    }

    void mPostCoordinatePanel::renameCoordinateRecord(int coordinateId, const QString& newName)
    {
        //查重由调用方（树 renameValidator）完成，此处为最终写入+面板跟随
        onCoordinateRenamedExternally(coordinateId, newName);
    }

    /*
    * ============================ 面板回填辅助 ============================
    */

    void mPostCoordinatePanel::fillNameAndType(const QString& name, int typeIdx)
    {
        //名称（屏蔽信号防触发校验/痕迹标记）
        _nameEdit->blockSignals(true);
        _nameEdit->setText(name);
        _nameEdit->blockSignals(false);

        //类型
        const int idx = qBound(0, typeIdx, 2);
        _comboType->blockSignals(true);
        _comboType->setCurrentIndex(idx);
        _comboType->blockSignals(false);
        _curTypeIdx = idx;
    }

    void mPostCoordinatePanel::resetInputDefaults()
    {
        //定义方式/主轴复位默认（三点、X轴；屏蔽信号防连锁）
        _defineChoice->blockSignals(true);
        _defineChoice->setCurrentIndex(0);
        _defineChoice->blockSignals(false);
        _mainIndex = 0;
        _mainAxisChoice->blockSignals(true);
        _mainAxisChoice->setCurrentIndex(0);
        _mainAxisChoice->blockSignals(false);
        applyAxisReferences();   //参考平面/第二轴选项归位（_dirIndex 置 0）

        //点位/方向复位默认
        _originRow.setCoord(QVector3D(0, 0, 0));
        _pt1Row.setCoord(QVector3D(0, 0, 0));
        _pt2Row.setCoord(QVector3D(0, 0, 0));
        _dir1Row.combo->setCurrentIndex(0);
        _dir2Row.combo->setCurrentIndex(0);
        _dir1Row.reverse->setChecked(false);
        _dir2Row.reverse->setChecked(false);

        updatePointGroupTitles();
        slot_onNameTextChanged(_nameEdit->text());
    }

    void mPostCoordinatePanel::slot_debugNextType()
    {
        //ui-shot 状态驱动：走真实下拉切换链（含 per-type 草稿/行显隐联动）
        const int next = (_comboType->currentIndex() + 1) % 3;
        _comboType->setCurrentIndex(next);
    }

    void mPostCoordinatePanel::slot_debugToggleDefine()
    {
        //ui-shot 状态驱动：走真实定义方式切换链（三点↔基准轴方式级缓存+行显隐联动）
        _defineChoice->setCurrentIndex(_defineChoice->currentIndex() == 0 ? 1 : 0);
    }

    /*
    * ============================ 类型/定义方式联动 ============================
    */

    void mPostCoordinatePanel::slot_onTypeChanged(int index)
    {
        if (index < 0 || index > 2)
            return;
        const int prev = _curTypeIdx;
        _curTypeIdx = index;

        //类型独立草稿：切换前保存当前类型输入(原型三类型各自draft)
        saveTypeDraft(prev);
        restoreTypeDraft(index);

        //布局刷新须在草稿恢复之后：restore 屏蔽信号不触发定义方式槽，
        //在前刷新会按上一类型遗留的方式摆布局
        updatePosFoldLayout();
        //类型入会话记录（树复制语义需保留类型）；用户输入痕迹标记
        if (CoordRecord* rec = (_curCoordId >= 0 ? recordById(_curCoordId) : nullptr))
            rec->typeIdx = _curTypeIdx;
        markDraftTouched();
    }

    void mPostCoordinatePanel::slot_onDefineChanged(const QString& text)
    {
        Q_UNUSED(text);
        /*
        * 三点<->基准轴切换输入各自保存恢复(原型 basisData/threeData)：
        * 离开三点时缓存三点坐标；离开基准轴时缓存方向档位/反向；
        * 恢复无缓存时保留原点、清空另两点(原型 points=[origin,null,null])
        */
        const bool toBasis = (_defineChoice->currentIndex() == 1);
        if (toBasis)
        {
            _methodThreePoints = { _originRow.coordValue(), _pt1Row.coordValue(), _pt2Row.coordValue() };
            if (_methodBasisValid)
            {
                _dir1Row.combo->setCurrentIndex(_methodBasisDir1);
                _dir2Row.combo->setCurrentIndex(_methodBasisDir2);
                _dir1Row.reverse->setChecked(_methodBasisRev1);
                _dir2Row.reverse->setChecked(_methodBasisRev2);
            }
            _pt1Row.setCoord(QVector3D(0, 0, 0));
            _pt2Row.setCoord(QVector3D(0, 0, 0));
        }
        else
        {
            _methodBasisDir1 = _dir1Row.combo->currentIndex();
            _methodBasisDir2 = _dir2Row.combo->currentIndex();
            _methodBasisRev1 = _dir1Row.reverse->isChecked();
            _methodBasisRev2 = _dir2Row.reverse->isChecked();
            _methodBasisValid = true;
            if (_methodThreePoints.size() == 3)
            {
                _originRow.setCoord(_methodThreePoints.at(0));
                _pt1Row.setCoord(_methodThreePoints.at(1));
                _pt2Row.setCoord(_methodThreePoints.at(2));
            }
            else
            {
                _pt1Row.setCoord(QVector3D(0, 0, 0));
                _pt2Row.setCoord(QVector3D(0, 0, 0));
            }
        }
        updatePosFoldLayout();
        markDraftTouched();   //用户输入痕迹（真实数据写入随数据层立项接入）
    }

    void mPostCoordinatePanel::slot_onMainAxisChanged(const QString& text)
    {
        Q_UNUSED(text);
        /*
        * 参考平面/第二个轴选项=包含主轴的两平面(第二轴由平面推导)，
        * 主轴切换后无条件重选第一项(原型 selected=referencePlanes[0])
        */
        _mainIndex = _mainAxisChoice->currentIndex();
        applyAxisReferences();
        markDraftTouched();   //用户输入痕迹（真实数据写入随数据层立项接入）
    }

    void mPostCoordinatePanel::slot_onPlaneChoiceChanged(const QString& text)
    {
        Q_UNUSED(text);
        _dirIndex = _planeChoice->currentIndex();
        updatePointGroupTitles();
        markDraftTouched();
    }

    void mPostCoordinatePanel::slot_onSecondAxisChanged(const QString& text)
    {
        Q_UNUSED(text);
        _dirIndex = _secondAxisChoice->currentIndex();
        updatePointGroupTitles();
        markDraftTouched();
    }

    /*
    * ============================ 草稿 ============================
    */

    void mPostCoordinatePanel::saveTypeDraft(int typeIdx)
    {
        if (typeIdx < 0 || typeIdx > 2)
            return;
        TypeDraft& d = _typeDrafts[typeIdx];
        d.valid = true;
        d.method = (_defineChoice->currentIndex() == 0) ? QStringLiteral(u"三点") : QStringLiteral(u"基准轴");
        d.origin = _originRow.coordValue();
        d.xPoint = _pt1Row.coordValue();
        d.xyPoint = _pt2Row.coordValue();
        d.dirMode1 = _dir1Row.combo->currentIndex();
        d.dirMode2 = _dir2Row.combo->currentIndex();
        d.rev1 = _dir1Row.reverse->isChecked();
        d.rev2 = _dir2Row.reverse->isChecked();
        d.mainIndex = _mainIndex;
        d.dirIndex = _dirIndex;
    }

    void mPostCoordinatePanel::restoreTypeDraft(int typeIdx)
    {
        auto it = _typeDrafts.find(typeIdx);
        if (it == _typeDrafts.end() || !it.value().valid)
            return;
        const TypeDraft d = it.value();

        //面板控件同步(屏蔽信号避免连锁触发)
        _defineChoice->blockSignals(true);
        _defineChoice->setCurrentIndex(d.method == QStringLiteral(u"三点") ? 0 : 1);
        _defineChoice->blockSignals(false);

        _mainIndex = d.mainIndex;
        _mainAxisChoice->blockSignals(true);
        _mainAxisChoice->setCurrentIndex(d.mainIndex);
        _mainAxisChoice->blockSignals(false);
        applyAxisReferences();   //选项集随主轴重算(内部 _dirIndex 置 0)

        _dirIndex = d.dirIndex;
        _planeChoice->blockSignals(true);
        _planeChoice->setCurrentIndex(d.dirIndex);
        _planeChoice->blockSignals(false);
        _secondAxisChoice->blockSignals(true);
        _secondAxisChoice->setCurrentIndex(d.dirIndex);
        _secondAxisChoice->blockSignals(false);

        _originRow.setCoord(d.origin);
        _pt1Row.setCoord(d.xPoint);
        _pt2Row.setCoord(d.xyPoint);
        _dir1Row.combo->setCurrentIndex(d.dirMode1);
        _dir2Row.combo->setCurrentIndex(d.dirMode2);
        _dir1Row.reverse->setChecked(d.rev1);
        _dir2Row.reverse->setChecked(d.rev2);
        updatePointGroupTitles();
    }

    /*
    * ============================ 行显隐/标题联动 ============================
    */

    void mPostCoordinatePanel::updatePosFoldLayout()
    {
        const bool isCartesian = _comboType->currentIndex() == 0;
        const bool threePoints = (_defineChoice->currentIndex() == 0);

        /*
        * 行显隐走 FoldSection::setRowVisible（行级摘除/按 addRow 逻辑行序插回，
        * 并同步内容区高度帽）——隐藏行不留残距、插回不漂移
        */
        _foldPos->setRowVisible(_mainAxisChoice, isCartesian);                    //主轴行仅直角
        _foldPos->setRowVisible(_secondAxisChoice, isCartesian && !threePoints);  //第二轴=直角+基准轴
        _foldPos->setRowVisible(_planeChoice, isCartesian && threePoints);        //参考平面=直角+三点
        _foldPos->setRowVisible(_pt1Row.group, threePoints);                      //轴正向点=三点
        _foldPos->setRowVisible(_pt2Row.group, threePoints);                      //平面点=三点
        _foldPos->setRowVisible(_dir1Row.group, !threePoints);                    //主轴方向=基准轴
        _foldPos->setRowVisible(_dir2Row.group, !threePoints);                    //第二轴方向=基准轴
        _foldPos->setRowVisible(_sphereNoteLabel, _comboType->currentIndex() == 2); //球面说明仅球

        updatePointGroupTitles();
        updateQuickRowVisibility();

        _foldPos->refreshLayout();
        _foldBasic->refreshLayout();

        //行显隐变化后刷新拾取武装归位（隐藏行的拾取钮不再占武装态）
        activateFirstVisiblePickButton();
    }

    void mPostCoordinatePanel::updatePointGroupTitles()
    {
        /*
        * 原型 legend 联动：三点标题随主轴/参考平面/类型；基准轴随主轴/第二轴/类型
        * （柱/球固定 Z 为主轴、X 为第二轴语义）
        */
        const int typeIdx = _comboType->currentIndex();
        const bool threePoints = (_defineChoice->currentIndex() == 0);
        const bool cn = isCN();

        _originRow.group->setTitle(cn ? QStringLiteral(u"原点") : QStringLiteral("Origin"));
        if (threePoints)
        {
            if (typeIdx == 0)
            {
                const QString ax = axLetter(_mainIndex);
                _pt1Row.group->setTitle(cn ? ax + QStringLiteral(u"轴正向点") : ax + QStringLiteral(" Axis Point"));
                const QString plane = _planeChoice->currentText();   //XOY/XOZ 语言无关
                _pt2Row.group->setTitle(cn ? plane + QStringLiteral(u"平面") : plane + QStringLiteral(" Plane"));
            }
            else if (typeIdx == 2)
            {
                _pt1Row.group->setTitle(cn ? QStringLiteral(u"极轴正向点") : QStringLiteral("Polar Axis Point"));
                _pt2Row.group->setTitle(cn ? QStringLiteral(u"零经度参考点") : QStringLiteral("Zero-longitude Point"));
            }
            else
            {
                _pt1Row.group->setTitle(cn ? QStringLiteral(u"Z轴正向点") : QStringLiteral("Z Axis Point"));
                _pt2Row.group->setTitle(cn ? QStringLiteral(u"零角径向") : QStringLiteral("Zero-angle Radial"));
            }
            //方向组随基准轴口径预设（该方式下隐藏，标题保持规范值）
            _dir1Row.group->setTitle(cn ? QStringLiteral(u"坐标系Z轴") : QStringLiteral("CS Z Axis"));
            _dir2Row.group->setTitle(cn ? QStringLiteral(u"坐标系X轴") : QStringLiteral("CS X Axis"));
        }
        else
        {
            if (typeIdx == 0)
            {
                const QString ax = axLetter(_mainIndex);
                const QString ax2 = _secondAxisChoice->currentText().left(1);
                _dir1Row.group->setTitle(cn ? QStringLiteral(u"坐标系") + ax + QStringLiteral(u"轴") : QStringLiteral("CS ") + ax + QStringLiteral(" Axis"));
                _dir2Row.group->setTitle(cn ? QStringLiteral(u"坐标系") + ax2 + QStringLiteral(u"轴") : QStringLiteral("CS ") + ax2 + QStringLiteral(" Axis"));
            }
            else
            {
                _dir1Row.group->setTitle(cn ? QStringLiteral(u"坐标系Z轴") : QStringLiteral("CS Z Axis"));
                _dir2Row.group->setTitle(cn ? QStringLiteral(u"坐标系X轴") : QStringLiteral("CS X Axis"));
            }
            //点组随三点口径预设（该方式下隐藏，标题保持规范值）
            _pt1Row.group->setTitle(cn ? QStringLiteral(u"Z轴正向点") : QStringLiteral("Z Axis Point"));
            _pt2Row.group->setTitle(cn ? QStringLiteral(u"零角径向") : QStringLiteral("Zero-angle Radial"));
        }
    }

    void mPostCoordinatePanel::applyAxisReferences()
    {
        /*
        * referencePlanes 口径：选项=包含主轴的两平面(X→XOY/XOZ、Y→XOY/YOZ、Z→XOZ/YOZ)，
        * 第二轴由平面推导；主轴切换后无条件取第一项(原型 selected=referencePlanes[0])
        */
        const QString axis = axLetter(_mainIndex);
        const bool cn = isCN();
        QStringList planes, seconds;
        const QString codes[3] = { QStringLiteral("XY"), QStringLiteral("XZ"), QStringLiteral("YZ") };
        for (const QString& code : codes)
        {
            if (code.contains(axis))
            {
                QString sec = code;
                sec.remove(axis);
                planes << (code.left(1) + QStringLiteral("O") + code.right(1));
                seconds << (cn ? sec + QStringLiteral(u"轴") : sec + QStringLiteral(" Axis"));
            }
        }
        _planeChoice->blockSignals(true);
        _planeChoice->setOptions(planes);
        _planeChoice->setCurrentIndex(0);
        _planeChoice->blockSignals(false);
        _secondAxisChoice->blockSignals(true);
        _secondAxisChoice->setOptions(seconds);
        _secondAxisChoice->setCurrentIndex(0);
        _secondAxisChoice->blockSignals(false);
        _dirIndex = 0;
        updatePointGroupTitles();
    }

    void mPostCoordinatePanel::updateQuickRowVisibility()
    {
        //快速创建仅"三点"定义方式提供（基准轴方式为方向定义，无拾取接力语义）
        const bool showQuick = (_defineChoice->currentIndex() == 0);
        _foldBasic->setRowVisible(_quickRow, showQuick);
        _foldBasic->refreshLayout();
        //应用按钮：快速创建可用且勾选时隐藏(拾取即应用)，其余手动点应用
        const bool manualApply = !showQuick || !_quickCreate->isChecked();
        for (PointRow* row : { &_originRow, &_pt1Row, &_pt2Row })
        {
            if (row && row->applyBtn)
                row->applyBtn->setVisible(manualApply);
        }
    }

    /*
    * ============================ 拾取武装态（纯 UI） ============================
    */

    void mPostCoordinatePanel::activateFirstVisiblePickButton()
    {
        PointRow* const allRows[3] = { &_originRow, &_pt1Row, &_pt2Row };

        /*
        * 可见性判定用 isHidden()（自身是否被行显隐刻意隐藏）而非 isVisible()：
        * 后者在任一祖先未 show 时恒为 false（ui-shot invoke 时序/宿主切换瞬间），
        * 会把已点亮的拾取钮误熄灭且无法再点亮（踩坑 17 原样场景）
        */
        auto rowActive = [](const PointRow* row) -> bool
            {
                return row && row->group && !row->group->isHidden() && row->group->isEnabled();
            };

        //失活所有不可见行的拾取钮（避免隐藏行占用武装态）
        for (PointRow* row : allRows)
        {
            if (row && row->pickBtn && !rowActive(row) && row->pickBtn->isChecked())
            {
                row->pickBtn->setChecked(false);
                row->pickBtn->setText(pickButtonLabel(false));
            }
        }

        //按 原点→轴正向点→平面点 点亮首个可见行的拾取钮
        for (PointRow* row : allRows)
        {
            if (rowActive(row) && row->pickBtn)
            {
                if (!row->pickBtn->isChecked())
                {
                    row->pickBtn->setChecked(true);
                    row->pickBtn->setText(pickButtonLabel(true));
                }
                break;
            }
        }
    }

    void mPostCoordinatePanel::resetPickArmedState(bool keepFirstVisible)
    {
        for (PointRow* row : { &_originRow, &_pt1Row, &_pt2Row })
        {
            if (row && row->pickBtn)
            {
                row->pickBtn->setChecked(false);
                row->pickBtn->setText(pickButtonLabel(false));
            }
        }
        if (keepFirstVisible)
            activateFirstVisiblePickButton();
    }

    QString mPostCoordinatePanel::pickButtonLabel(bool armed) const
    {
        return armed
            ? (isCN() ? QStringLiteral(u"正在拾取节点…") : QStringLiteral("Picking node..."))
            : (isCN() ? QStringLiteral(u"拾取节点") : QStringLiteral("Pick Node"));
    }

    QString mPostCoordinatePanel::axLetter(int axisIdx)
    {
        static const QString letters[3] = { QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z") };
        return letters[qBound(0, axisIdx, 2)];
    }

    /*
    * ============================ 名称校验 ============================
    */

    void mPostCoordinatePanel::slot_onNameTextChanged(const QString& text)
    {
        Q_UNUSED(text);
        QString err;
        const QString name = _nameEdit->text().trimmed();
        const CoordRecord* rec = (_curCoordId >= 0 && isEditing()) ? recordById(_curCoordId) : nullptr;
        if (name.isEmpty())
        {
            err = isCN() ? QStringLiteral(u"名称不能为空") : QStringLiteral("Name cannot be empty");
        }
        else if (rec != nullptr && name != rec->name)
        {
            //重名即时校验（会话记录查重，排除自身当前名）
            if (coordinateNameExists(name, rec->id))
                err = isCN() ? QStringLiteral(u"该名称已存在，请使用其他名称")
                             : QStringLiteral("Name already exists");
        }
        _nameEdit->setStyleSheet(err.isEmpty() ? QString() : QStringLiteral("border:1px solid #d95151;"));
        _nameErrorLabel->setText(err);
        _foldBasic->setRowVisible(_nameErrorLabel, !err.isEmpty());
        _foldBasic->refreshLayout();
    }

    void mPostCoordinatePanel::slot_commitName()
    {
        CoordRecord* rec = (_curCoordId >= 0 && isEditing()) ? recordById(_curCoordId) : nullptr;
        if (rec == nullptr)
            return;
        const auto rollback = [this](const QString& cur)
        {
            //空名/重名回滚当前名（校验行同步消除）
            _nameEdit->blockSignals(true);
            _nameEdit->setText(cur);
            _nameEdit->blockSignals(false);
            slot_onNameTextChanged(cur);
        };
        const QString next = _nameEdit->text().trimmed();
        if (next.isEmpty() || next == rec->name)
        {
            rollback(rec->name);
            return;
        }
        //重名拒绝并回滚（即时校验已提示过，此处兜底）
        if (coordinateNameExists(next, rec->id))
        {
            rollback(rec->name);
            return;
        }
        rec->name = next;
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCoordinateSig(false, rec->id);
    }

    /*
    * ============================ 语言切换 ============================
    */

    void mPostCoordinatePanel::slot_changeLanguage(int language)
    {
        Q_UNUSED(language);
        const bool cn = isCN();

        //折叠组标题
        _foldBasic->setTitle(cn ? QStringLiteral(u"基本信息") : QStringLiteral("Basic Info"));
        _foldPos->setTitle(cn ? QStringLiteral(u"位置与方向") : QStringLiteral("Position & Orientation"));

        //行标签
        _nameLabel->setText(cn ? QStringLiteral(u"名称") : QStringLiteral("Name"));
        _typeLabel->setText(cn ? QStringLiteral(u"类型") : QStringLiteral("Type"));
        _defineLabel->setText(cn ? QStringLiteral(u"定义方式") : QStringLiteral("Definition"));
        _mainAxisLabel->setText(cn ? QStringLiteral(u"坐标系主轴") : QStringLiteral("Primary Axis"));
        _secondAxisLabel->setText(cn ? QStringLiteral(u"第二个轴") : QStringLiteral("Second Axis"));
        _planeLabel->setText(cn ? QStringLiteral(u"参考平面") : QStringLiteral("Ref Plane"));

        //名称占位与快速创建
        _nameEdit->setPlaceholderText(cn ? QStringLiteral(u"坐标系名称") : QStringLiteral("Coordinate system name"));
        _quickCreate->setText(cn ? QStringLiteral(u"快速创建") : QStringLiteral("Quick Create"));
        _quickCreate->setToolTip(cn ? QStringLiteral(u"拾取到点后自动应用，并按 原点→轴正向点→平面点 接力")
            : QStringLiteral("Auto-apply on pick, relaying origin → axis point → plane point"));
        _sphereNoteLabel->setText(cn
            ? QStringLiteral(u"球坐标系：θ 为极角（与 +Z 轴夹角），φ 为方位角（XOY 面内自 +X 轴起）")
            : QStringLiteral("Spherical: θ = polar angle (from +Z), φ = azimuth (from +X in XOY)"));

        //类型下拉（blockSignals 保选重灌，防 currentIndexChanged 触发类型切换槽）
        int idx = _comboType->currentIndex();
        _comboType->blockSignals(true);
        _comboType->clear();
        _comboType->addItems(cn
            ? QStringList{ QStringLiteral(u"直角坐标系"), QStringLiteral(u"柱坐标系"), QStringLiteral(u"球坐标系") }
            : QStringList{ QStringLiteral("Cartesian"), QStringLiteral("Cylindrical"), QStringLiteral("Spherical") });
        _comboType->setCurrentIndex(idx);
        _comboType->blockSignals(false);

        //定义方式（setOptions 按文字保选中会回落第 0 项并发信号——先 blockSignals+索引回置）
        idx = _defineChoice->currentIndex();
        _defineChoice->blockSignals(true);
        _defineChoice->setOptions(cn
            ? QStringList{ QStringLiteral(u"三点"), QStringLiteral(u"基准轴") }
            : QStringList{ QStringLiteral("3 Points"), QStringLiteral("Datum Axes") });
        _defineChoice->setCurrentIndex(idx);
        _defineChoice->blockSignals(false);

        //主轴（选项文字随语言重建；平面/第二轴选项在 applyAxisReferences 内按语言重算）
        idx = _mainAxisChoice->currentIndex();
        _mainAxisChoice->blockSignals(true);
        _mainAxisChoice->setOptions(cn
            ? QStringList{ QStringLiteral(u"X轴"), QStringLiteral(u"Y轴"), QStringLiteral(u"Z轴") }
            : QStringList{ QStringLiteral("X Axis"), QStringLiteral("Y Axis"), QStringLiteral("Z Axis") });
        _mainAxisChoice->setCurrentIndex(idx);
        _mainAxisChoice->blockSignals(false);
        const int keepDir = _dirIndex;
        applyAxisReferences();   //重灌平面/第二轴（内部 _dirIndex 置 0）
        _dirIndex = keepDir;
        _planeChoice->blockSignals(true);
        _planeChoice->setCurrentIndex(keepDir);
        _planeChoice->blockSignals(false);
        _secondAxisChoice->blockSignals(true);
        _secondAxisChoice->setCurrentIndex(keepDir);
        _secondAxisChoice->blockSignals(false);

        //方向行下拉与附属件
        for (DirRow* row : { &_dir1Row, &_dir2Row })
        {
            if (!row || !row->combo)
                continue;
            idx = row->combo->currentIndex();
            row->combo->blockSignals(true);
            row->combo->clear();
            row->combo->addItems(cn
                ? QStringList{ QStringLiteral(u"全局X轴"), QStringLiteral(u"全局Y轴"), QStringLiteral(u"全局Z轴"), QStringLiteral(u"自定义矢量") }
                : QStringList{ QStringLiteral("Global X"), QStringLiteral("Global Y"), QStringLiteral("Global Z"), QStringLiteral("Custom Vector") });
            row->combo->setCurrentIndex(idx);
            row->combo->blockSignals(false);
            if (row->reverse)
                row->reverse->setText(cn ? QStringLiteral(u"反向") : QStringLiteral("Reverse"));
        }

        //点行应用钮/拾取钮文案
        for (PointRow* row : { &_originRow, &_pt1Row, &_pt2Row })
        {
            if (!row)
                continue;
            if (row->applyBtn)
                row->applyBtn->setText(cn ? QStringLiteral(u"应用") : QStringLiteral("Apply"));
            if (row->pickBtn)
                row->pickBtn->setText(pickButtonLabel(row->pickBtn->isChecked()));
        }

        //组标题与错误行按新语言重估
        updatePointGroupTitles();
        slot_onNameTextChanged(_nameEdit->text());
    }

    /*
    * ============================ 生命周期 ============================
    */

    void mPostCoordinatePanel::showEvent(QShowEvent* event)
    {
        /*
        * 与 hideEvent 对称：面板重显（宿主挂载序的 hide→show、dock 切回）时
        * 恢复首个可见拾取钮的武装态——hide 侧熄灭而 show 侧不恢复会造成
        * "面板其实一直可见、武装态却丢了"的不对称（ui-shot 挂载序列实证）
        */
        activateFirstVisiblePickButton();
        mPanelBase::showEvent(event);
    }

    void mPostCoordinatePanel::hideEvent(QHideEvent* event)
    {
        //面板切走（共享 dock 切页/总复位）时退出拾取武装态；
        //功能接入后在此扩展：退视口拾取（PickNothing）+断 FinishPick 信号
        resetPickArmedState(false);
        mPanelBase::hideEvent(event);
    }
}
