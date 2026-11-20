#include "mPostExplodePanel.h"
#include "mCommonFunctions.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/REBUILD_PostModelRender.h"
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mPostModelData.h"
#include "mMechGlobalSignals.h"
#include "mOtherFunction.h"
#include "mPostObjectEditors.h"
#include "mPostPartScopeField.h"
#include "mPostGeneralPickUp.h"

#include "mGroupBox.h"
#include "mDirectionRow.h"
#include "mPanelTable.h"
#include "mPrimaryButton.h"
#include "UiMetrics.h"
#include "UiPalette.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QBrush>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleValidator>
#include <QEvent>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <algorithm>
#include <functional>

using namespace PostData;
namespace PostWindow
{
    namespace
    {
        //令牌（--std-* 标准档；出处 V1.3 原型 CSS 变量定义块，与 mPostSectionCut 一致）
        constexpr int kControl = 26;		//--std-control 控件外高
        constexpr int kInner = 24;			//行内控件高（--std-control - 2）
        constexpr int kActionW = 40;		//--std-manager-action-width 应用钮宽
        constexpr int kGroupInset = 7;		//--std-group-inset
        constexpr int kRadius = 4;			//--std-radius（V1.3：2→4）
        constexpr int kGap = 6;				//--std-gap
        constexpr int kColumnGap = 8;		//--std-column-gap

        const char* kTextColor = "#303313";		//--std-text / --std-label
        const char* kMuted = "#788391";			//--std-muted
        const char* kDisabledFg = "#A6AFBB";		//--std-disabled
        const char* kCtrlBorder = "#DCE2E9";		//--std-border
        const char* kHoverBg = "#F0F7FF";		//--std-hover
        const char* kSelectedBg = "#E8F3FF";		//--std-selected
        const char* kPrimary = "#1890FF";		//--std-primary / --std-focus
        const char* kError = "#BF3434";			//--std-error
        const char* kPanelBg = "#F5F7FA";		//--std-panel-color（禁用底）

        //坐标输入格式（X,Y,Z 三段数值）
        const QRegularExpression kCoordRe(
            QString("\\s*-?\\d+(\\.\\d+)?\\s*,\\s*-?\\d+(\\.\\d+)?\\s*,\\s*-?\\d+(\\.\\d+)?\\s*"));

        //2x 采样绘制线条图标（近似原型 inline SVG 语义；DPR 固定 2.0，
        //painter 只做 viewBox→逻辑坐标映射，不与 DPR 叠乘）
        QIcon makeIcon(int px, const QColor& color, const std::function<void(QPainter&)>& paint)
        {
            QPixmap pm(px * 2, px * 2);
            pm.setDevicePixelRatio(2.0);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing);
            QPen pen(color, 1.4);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            p.setPen(pen);
            paint(p);
            return QIcon(pm);
        }

        //减小（原型 explosionDecrease 24 viewBox "M5 12h14"）
        QIcon minusIcon(const QColor& color)
        {
            return makeIcon(20, color, [](QPainter& p)
                {
                    p.scale(20.0 / 24.0, 20.0 / 24.0);
                    p.drawLine(QPointF(5, 12), QPointF(19, 12));
                });
        }

        //增大（原型 explosionIncrease 24 viewBox "M5 12h14M12 5v14"）
        QIcon plusIcon(const QColor& color)
        {
            return makeIcon(20, color, [](QPainter& p)
                {
                    p.scale(20.0 / 24.0, 20.0 / 24.0);
                    p.drawLine(QPointF(5, 12), QPointF(19, 12));
                    p.drawLine(QPointF(12, 5), QPointF(12, 19));
                });
        }

        //坐标串解析（三段数值；非法返回 false）
        bool parseCoord(const QString& text, QVector3D* out)
        {
            if (!kCoordRe.match(text).hasMatch())
                return false;
            const QStringList parts = text.split(',');
            if (parts.size() != 3)
                return false;
            bool okX = false, okY = false, okZ = false;
            const float x = parts.at(0).trimmed().toFloat(&okX);
            const float y = parts.at(1).trimmed().toFloat(&okY);
            const float z = parts.at(2).trimmed().toFloat(&okZ);
            if (!okX || !okY || !okZ)
                return false;
            if (out)
                *out = QVector3D(x, y, z);
            return true;
        }

        //条目 kind：0=不显示爆炸（常驻）/1=自动爆炸图（常驻）/2=用户自建
        const int kKindNone = 0;
        const int kKindAuto = 1;
        const int kKindUser = 2;
    }

    mPostExplodePanel::mPostExplodePanel(QWidget* parent)
        : mPanelBase(parent)
    {
        setupUI();

        //初始条目：不显示爆炸（激活）+ 自动爆炸图
        _entries.append({ 1, kKindNone, QString(), false });
        _entries.append({ 2, kKindAuto, QString(), false });
        _activeId = 1;
        rebuildTable();
        updateExplodeCtrlsEnabled();	//"不显示爆炸"激活：底部调节控件初始置灰
        updateEditState();				//无自建条目：编辑区禁用（V1.3 空态）

        // 与云图面板统一：折叠区注册到基类统一管理
        m_foldWidgets.append(_manageFold);
        m_foldWidgets.append(_ctrlFold);

        // 切换界面语言
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
        slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

        this->setMinimumWidth(300);
    }

    void mPostExplodePanel::regLabel(PanelLabel* label, const char* cn, const char* en)
    {
        _labelTexts.append({ label, cn, en });
        label->setText(QString::fromUtf8(cn));
    }

    void mPostExplodePanel::setupUI()
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        /********************爆炸图管理********************/
        _manageFold = new FoldSection(QString::fromUtf8("爆炸图管理"), this);
        //列表边界同字段边界（V1.3 --std-field-inset；贴边即破版，截面力轮同法收敛）
        if (auto* form = _manageFold->layout())
            form->setContentsMargins(MxFunctions::UiMetrics::fieldInset(), 10,
                MxFunctions::UiMetrics::fieldInset(), 10);

        //搜索框（V1.3 #explosionSearch：仅筛管理列表，不改选择/激活）
        _searchEdit = new QLineEdit(this);
        _searchEdit->setObjectName("explodeSearch");
        _searchEdit->setFixedHeight(kControl);
        _searchEdit->setStyleSheet(
            QString("QLineEdit{background:#FFFFFF;border:1px solid %1;border-radius:%2px;"
                "padding:0 %3px;color:%4;}")
            .arg(kCtrlBorder, QString::number(kRadius), QString::number(kGroupInset), kTextColor));
        _manageFold->addRow(_searchEdit);
        connect(_searchEdit, &QLineEdit::textChanged, this, &mPostExplodePanel::slot_searchChanged);

        //爆炸图列表（M2 管理表：复选列 22/名称/操作列 62；行高 26；
        //最小六行 184、最大十行 288——M2/CAEManagerTableHeight 口径。
        //共享管理表格件：样式/行高/表头/行线委托由 mPanelTable 统一（UiPalette/UiMetrics））
        _table = new MxFunctions::mPanelTable(0, 3, this);
        _table->setObjectName("explodeTable");
        _table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        _table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        _table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        _table->setColumnWidth(0, 22);
        _table->setColumnWidth(2, 62);
        //行选中态只由勾选决定：点击名称不产生选择高亮
        _table->setSelectionMode(QAbstractItemView::NoSelection);
        _table->horizontalHeader()->setSortIndicatorShown(false);
        _table->setSortingEnabled(false);
        _table->setMinimumHeight(184);	//最小六行（表头+6 行+边框，M2 口径）
        _table->setMaximumHeight(288);	//最大十行，超出滚动
        _manageFold->addRow(_table);
        //名称列点击表头排序（V1.3：未排序↕→升序↑→降序↓循环；手工稳定排序自建
        //条目段后重灌——sortItems 与行控件不同步会把复选/激活钮留在原行）
        connect(_table->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int section) {
            if (section != 1) return;
            _nameSortState = (_nameSortState == 1) ? 2 : 1;
            int first = -1, count = 0;
            for (int i = 0; i < _entries.size(); ++i)
                if (_entries.at(i).kind == kKindUser) { if (first < 0) first = i; ++count; }
            if (first < 0 || count < 2) { slot_changeLanguage(_langEn ? 1 : 0); return; }
            const bool asc = (_nameSortState == 1);
            std::stable_sort(_entries.begin() + first, _entries.begin() + first + count,
                [asc](const ExplodeEntry& a, const ExplodeEntry& b) {
                    const int cmp = a.name.localeAwareCompare(b.name);
                    return asc ? cmp < 0 : cmp > 0;
                });
            rebuildTable();
            slot_changeLanguage(_langEn ? 1 : 0);	//表头箭头文案刷新
            });

        //表尾（V1.3 .measurement-manager-actions：全选+计数（左）+ 删除/新建爆炸图（右））
        QWidget* footerRow = new QWidget(this);
        QHBoxLayout* footerLayout = new QHBoxLayout(footerRow);
        footerLayout->setContentsMargins(0, 0, 0, 0);
        footerLayout->setSpacing(kGap);
        const char* btnQss =
            "QPushButton{border:1px solid %1;border-radius:4px;background:#FFFFFF;color:#303313;}"
            "QPushButton:hover{background:#F0F7FF;}"
            "QPushButton:disabled{background:#F5F7FA;color:#A6AFBB;}";
        _btnSelectAll = new QPushButton(footerRow);
        _btnSelectAll->setObjectName("explodeSelectAll");
        _btnSelectAll->setStyleSheet(QString(btnQss).arg(kCtrlBorder));
        _btnSelectAll->setFixedSize(kActionW, kControl);
        _btnSelectAll->setCursor(Qt::PointingHandCursor);
        footerLayout->addWidget(_btnSelectAll);
        _selectedCount = new QLabel(footerRow);
        _selectedCount->setObjectName("explodeSelectedCount");
        _selectedCount->setStyleSheet(QString("QLabel{color:%1;font-size:11px;background:transparent;}").arg(kMuted));
        footerLayout->addWidget(_selectedCount);
        footerLayout->addStretch();
        _btnDelete = new QPushButton(footerRow);
        _btnDelete->setObjectName("explodeDelete");
        _btnDelete->setStyleSheet(QString(btnQss).arg(kCtrlBorder));
        _btnDelete->setFixedSize(kActionW, kControl);
        _btnDelete->setCursor(Qt::PointingHandCursor);
        _btnDelete->setToolTip(QString::fromUtf8("删除勾选的爆炸图，包含搜索范围外已勾选项"));
        footerLayout->addWidget(_btnDelete);
        _btnNew = new MxFunctions::mPrimaryButton(QString::fromUtf8("新建"), footerRow);
        _btnNew->setObjectName("explodeNew");
        _btnNew->setFixedWidth(kActionW);	//两字宽 40（M2；全局 QSS 钮宽下限不适用）
        _btnNew->setCursor(Qt::PointingHandCursor);
        footerLayout->addWidget(_btnNew);
        _manageFold->addRow(footerRow);

        connect(_btnSelectAll, &QPushButton::clicked, this, &mPostExplodePanel::slot_selectAllClicked);
        connect(_btnNew, &QPushButton::clicked, this, &mPostExplodePanel::slot_newView);
        connect(_btnDelete, &QPushButton::clicked, this, &mPostExplodePanel::slot_deleteViews);
        connect(_table, &QTableWidget::cellClicked, this, &mPostExplodePanel::slot_cellClicked);

        _mainLayout->addWidget(_manageFold);

        /********************爆炸图控制********************/
        //V1.3 运行时重排终态：控制区为可折叠 details（explosionControlFold），不再锁定
        _ctrlFold = new FoldSection(QString::fromUtf8("爆炸图控制"), this);
        if (auto* form = _ctrlFold->layout())
            form->setContentsMargins(MxFunctions::UiMetrics::fieldInset(), 10,
                MxFunctions::UiMetrics::fieldInset(), 10);

        //输入框统一样式（白底描边圆角）
        const QString lineEditQss = QString(
            "QLineEdit{background:#FFFFFF;border:1px solid %1;border-radius:%2px;"
            "padding:0 %3px;color:%4;}")
            .arg(kCtrlBorder, QString::number(kRadius), QString::number(kGroupInset), kTextColor);
        //下拉统一样式（裸控件口径）
        const QString comboQss = QString(
            "QComboBox{border:1px solid %1;border-radius:%2px;background:#FFFFFF;"
            "color:%3;padding:0 %4px;}"
            "QComboBox:disabled{background:%5;color:%6;}")
            .arg(kCtrlBorder, QString::number(kRadius), kTextColor, QString::number(kGroupInset), kPanelBg, kDisabledFg);

        //爆炸图名称（V1.3：字段区首行；编辑缓冲随保存/取消回填）
        _nameEdit = new QLineEdit(this);
        _nameEdit->setObjectName("explodeName");
        _nameEdit->setFixedHeight(kControl);
        _nameEdit->setStyleSheet(lineEditQss);
        {
            PanelLabel* label = new PanelLabel;
            regLabel(label, "爆炸图名称", "Name");
            _ctrlFold->addRow(label, _nameEdit);
        }

        //选择部件：mPost 通用拾取条（云图/向量图/阵列同款）。范围弹窗/材料级联自拾取条
        //接入起不可达——隐形 mPostPartScopeField 降级文案控制器（"所有部件"空态文案）；
        //已拾取部件入编辑缓冲（ExplodeDraft.pickedParts），执行经 getSelectedPartIds 消费
        _pickController = mPostGeneralPickUp::createPickWidget(CBBlue, this);
        _pickController->widget()->setObjectName("explodePartPick");  //ui-shot 沿用旧名
        _pickController->setSelectSetBtnVisible(false);               //后处理无集合语义
        _pickController->setKeepHighlightOnDeactivate(false);         //解除拾取即清视口高亮
        _pickController->setAllowedTypes([]() {
            QSet<MxFunctions::PickDataType> types;
            types.insert(MxFunctions::PickDataType::Part);           //本面板仅部件
            return types;
            });
        _scopeCtl = new mPostPartScopeField(this);
        _scopeCtl->setObjectType(1);	//部件（文案来源）
        _scopeCtl->hide();
        _pickController->setDefultTip(_scopeCtl->scopeText());
        {
            PanelLabel* label = new PanelLabel;
            regLabel(label, "选择部件", "Parts");
            _ctrlFold->addRow(label, _pickController->widget());
        }
        //范围文本变化（语言切换）→ 拾取条空态默认文案（云图/向量图/阵列同款）
        connect(_scopeCtl, &mPostPartScopeField::scopeTextChanged, this, [this](const QString& text) {
            _pickController->setDefultTip(text);
            });
        //拾取数据变化（视口拾取/详情弹窗删项/清空）→ 编辑缓冲 dirty 通知（装载期屏蔽）
        connect(_pickController, &mPostPickWidgetController::dataChanged, this, [this]() {
            if (!_loadingDraft && _editor) _editor->notifyEditChanged();
            });
        //激活/失活收口（云图/向量图/阵列同款；勿用 activePickSig——只同步前处理工具栏）：
        //激活=校验后处理上下文（无效立即失活回退）+视口设部件拾取过滤器；失活=清过滤器
        //与拾取高亮+恢复工具栏全部拾取类型；收割/高亮/重入回显由拾取栈控制器自管
        connect(_pickController, &mPostPickWidgetController::pickActiveChanged, this,
            [this](bool active) {
                mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
                if (active)
                {
                    if (postRend == nullptr || postRend->getPostRender() == nullptr)
                    {
                        _pickController->setPickActive(false);
                        return;
                    }
                    postRend->setPickFilter(mxr::PickFilter::PickMeshPart);
                }
                else
                {
                    if (postRend != nullptr)
                        postRend->setPickFilter(mxr::PickFilter::PickNothing);
                    emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>());
                }
            });

        //爆炸模式（平移/从模型中心爆炸/从所选部件中心爆炸）
        _comboMode = new QComboBox(this);
        _comboMode->setObjectName("comboBox_explodeMode");
        _comboMode->setFixedHeight(kControl);
        _comboMode->setStyleSheet(comboQss);
        {
            PanelLabel* label = new PanelLabel;
            regLabel(label, "爆炸模式", "Mode");
            _ctrlFold->addRow(label, _comboMode);
        }
        connect(_comboMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mPostExplodePanel::slot_modeChanged);

        //平移-距离（模式=平移时显示）
        _editDistance = new QLineEdit(this);
        _editDistance->setObjectName("lineEdit_explodeDistance");
        _editDistance->setFixedHeight(kControl);
        _editDistance->setText("1");
        _editDistance->setValidator(new QRegularExpressionValidator(
            QRegularExpression(QString("-?\\d+(\\.\\d+)?")), _editDistance));
        _editDistance->setStyleSheet(lineEditQss);
        {
            PanelLabel* label = new PanelLabel;
            regLabel(label, "距离", "Distance");
            _ctrlFold->addRow(label, _editDistance);
        }

        //平移方向：骑线细线框（V1.3 终态 post-direction-wire；mGroupBox 无勾选图标），
        //内含 定义方式（L13 下拉+反向切换钮）与方向类型条件行
        _dirBox = new MxFunctions::mGroupBox(QString::fromUtf8("平移方向"), QPixmap(), QPixmap(), this);
        _dirBox->setObjectName("explodeDirBox");
        QGridLayout* dirGrid = _dirBox->grid();
        dirGrid->setHorizontalSpacing(kColumnGap);
        dirGrid->setVerticalSpacing(kGap);
        dirGrid->setColumnStretch(0, 0);
        dirGrid->setColumnStretch(1, 1);

        _dirRow = new MxFunctions::mDirectionRow(_dirBox);
        _dirRow->setObjectName("explodeDirRow");
        _dirRow->combo()->setObjectName("comboBox_explodeDirection");
        _dirRow->reverseButton()->setObjectName("pushButton_explodeReverse");
        {
            PanelLabel* label = new PanelLabel;
            regLabel(label, "定义方式", "Definition");
            dirGrid->addWidget(label, 0, 0);
            dirGrid->addWidget(_dirRow, 0, 1);
        }
        connect(_dirRow->combo(), QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            applyModeRows();
            refreshErrorLine();
            if (!_loadingDraft && _editor) _editor->notifyEditChanged();
            });
        connect(_dirRow, &MxFunctions::mDirectionRow::reversedChanged, this, [this](bool) {
            if (!_loadingDraft && _editor) _editor->notifyEditChanged();	//反向属编辑缓冲
            });

        //起点/终点子块（V1.3 终态：[拾取x|选择节点(置灰)] [x坐标|输入]；应用钮已删）
        const char* pickBtnQss =
            "QPushButton{background:#FFFFFF;border:1px solid %1;border-radius:4px;color:#788391;}"
            "QPushButton:disabled{background:#F5F7FA;color:#A6AFBB;}";
        auto buildPointBlock = [this, dirGrid, pickBtnQss, &lineEditQss](
            const char* pickCn, const char* pickEn, const char* coordCn, const char* coordEn,
            QPushButton*& pickBtn, QLineEdit*& edit, const char* pickObj, const char* editObj, int gridRow)
        {
            QWidget* block = new QWidget(_dirBox);
            QGridLayout* grid = new QGridLayout(block);
            grid->setContentsMargins(0, 0, 0, 0);
            grid->setHorizontalSpacing(kColumnGap);
            grid->setVerticalSpacing(kGap);
            grid->setColumnStretch(0, 0);
            grid->setColumnStretch(1, 1);

            PanelLabel* pickLabel = new PanelLabel;
            regLabel(pickLabel, pickCn, pickEn);
            pickBtn = new QPushButton(QString::fromUtf8("选择节点"), block);
            pickBtn->setObjectName(pickObj);
            pickBtn->setFixedHeight(kControl);
            pickBtn->setEnabled(false);	//节点拾取待接入
            pickBtn->setStyleSheet(QString(pickBtnQss).arg(kCtrlBorder));
            pickBtn->setToolTip(QString::fromUtf8("拾取功能开发中"));
            grid->addWidget(pickLabel, 0, 0);
            grid->addWidget(pickBtn, 0, 1);

            PanelLabel* coordLabel = new PanelLabel;
            regLabel(coordLabel, coordCn, coordEn);
            edit = new QLineEdit(block);
            edit->setObjectName(editObj);
            edit->setFixedHeight(kControl);
            edit->setText(QString("0,0,0"));
            edit->setValidator(new QRegularExpressionValidator(kCoordRe, edit));
            edit->setStyleSheet(lineEditQss);
            grid->addWidget(coordLabel, 1, 0);
            grid->addWidget(edit, 1, 1);

            dirGrid->addWidget(block, gridRow, 0, 1, 2);
            block->setVisible(false);
            return block;
        };
        _startRowWidget = buildPointBlock("拾取起点", "Pick start", "起点坐标", "Start coords",
            _startPickBtn, _startEdit, "pushButton_explodeStartPick", "lineEdit_explodeStart", 1);
        _endRowWidget = buildPointBlock("拾取终点", "Pick end", "终点坐标", "End coords",
            _endPickBtn, _endEdit, "pushButton_explodeEndPick", "lineEdit_explodeEnd", 2);
        _endEdit->setText(QString("1,0,0"));

        //方向=选择对象：选择方向子行（拾取待接入，点击提示无效）
        {
            QWidget* row = new QWidget(_dirBox);
            _selDirRowWidget = row;
            row->setObjectName("explodeSelDirRow");
            QHBoxLayout* l = new QHBoxLayout(row);
            l->setContentsMargins(0, 0, 0, 0);
            l->setSpacing(kColumnGap);
            PanelLabel* tag = new PanelLabel;
            regLabel(tag, "选择方向", "Direction");
            l->addWidget(tag);
            _selDirBtn = new QPushButton(row);
            _selDirBtn->setObjectName("pushButton_explodeSelDir");
            _selDirBtn->setFixedHeight(kControl);
            _selDirBtn->setEnabled(false);	//方向对象拾取待接入（原型 dataset.unavailable）
            _selDirBtn->setStyleSheet(QString(pickBtnQss).arg(kCtrlBorder));
            _selDirBtn->setToolTip(QString::fromUtf8("请选择有效的方向对象。"));
            l->addWidget(_selDirBtn, 1);
            dirGrid->addWidget(row, 3, 0, 1, 2);
            row->setVisible(false);
            connect(_selDirBtn, &QPushButton::clicked, this, [this]() {
                int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(lang == 0
                    ? QString::fromUtf8("请选择有效的方向对象。")
                    : QString("Please select a valid direction object."));
                });
        }

        //方向=向量：方向向量子行
        {
            QWidget* row = new QWidget(_dirBox);
            _vecRowWidget = row;
            row->setObjectName("explodeVecRow");
            QHBoxLayout* l = new QHBoxLayout(row);
            l->setContentsMargins(0, 0, 0, 0);
            l->setSpacing(kColumnGap);
            PanelLabel* tag = new PanelLabel;
            regLabel(tag, "方向向量", "Vector");
            l->addWidget(tag);
            _vecEdit = new QLineEdit(row);
            _vecEdit->setObjectName("lineEdit_explodeVector");
            _vecEdit->setFixedHeight(kControl);
            _vecEdit->setText("1,0,0");
            _vecEdit->setValidator(new QRegularExpressionValidator(kCoordRe, _vecEdit));
            _vecEdit->setStyleSheet(lineEditQss);
            l->addWidget(_vecEdit, 1);
            dirGrid->addWidget(row, 4, 0, 1, 2);
            row->setVisible(false);
        }

        _ctrlFold->addRow(_dirBox);

        //缩放方式（等比例/分方向；"从模型中心爆炸/从所选部件中心爆炸"时显示）
        _comboScale = new QComboBox(this);
        _comboScale->setObjectName("comboBox_explodeScale");
        _comboScale->setFixedHeight(kControl);
        _comboScale->setStyleSheet(comboQss);
        {
            PanelLabel* label = new PanelLabel;
            regLabel(label, "缩放方式", "Scale by");
            _ctrlFold->addRow(label, _comboScale);
        }
        connect(_comboScale, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mPostExplodePanel::slot_scaleChanged);

        //变形系数（等比例单值，默认 1）
        _editFactor = new QLineEdit(this);
        _editFactor->setObjectName("lineEdit_explodeFactor");
        _editFactor->setFixedHeight(kControl);
        _editFactor->setText("1");
        _editFactor->setValidator(new QDoubleValidator(_editFactor));
        _editFactor->setStyleSheet(lineEditQss);
        {
            PanelLabel* label = new PanelLabel;
            regLabel(label, "变形系数", "Factor");
            _ctrlFold->addRow(label, _editFactor);
        }

        //变形系数分方向：X/Y/Z方向系数三行独立行（V1.3 vote-scale-rows 行级布局，
        //非一行三输入框——变形控制轮同款裁定）
        auto buildAxisRow = [this, &lineEditQss](const char* cn, const char* en,
            QLineEdit*& edit, const char* objectName)
        {
            edit = new QLineEdit(this);
            edit->setObjectName(objectName);
            edit->setFixedHeight(kControl);
            edit->setText("1");
            edit->setValidator(new QDoubleValidator(edit));
            edit->setStyleSheet(lineEditQss);
            PanelLabel* label = new PanelLabel;
            regLabel(label, cn, en);
            _ctrlFold->addRow(label, edit);
        };
        buildAxisRow("X方向系数", "X factor", _editX, "lineEdit_explodeScaleX");
        buildAxisRow("Y方向系数", "Y factor", _editY, "lineEdit_explodeScaleY");
        buildAxisRow("Z方向系数", "Z factor", _editZ, "lineEdit_explodeScaleZ");

        //内联错误行（V1.3 explosionSettingsError：红字 11px，空文案收起）
        _errorLine = new QLabel(this);
        _errorLine->setObjectName("explodeErrorLine");
        _errorLine->setWordWrap(true);
        _errorLine->setStyleSheet(QString("QLabel{color:%1;font-size:11px;background:transparent;}")
            .arg(kError));
        _errorLine->setVisible(false);
        _ctrlFold->addRow(_errorLine);

        //底部操作行（V1.3 explosion-view-footer：自适应视图 ‖ 复位 40 ‖ − 26 ‖ ＋ 26 主蓝）
        QWidget* fitRow = new QWidget(this);
        fitRow->setObjectName("explodeFitRow");
        QHBoxLayout* fitLayout = new QHBoxLayout(fitRow);
        fitLayout->setContentsMargins(0, 0, 0, 0);
        fitLayout->setSpacing(kGap);
        _checkAutofit = new QCheckBox(fitRow);
        _checkAutofit->setObjectName("checkBox_explodeAutofit");
        fitLayout->addWidget(_checkAutofit);
        fitLayout->addStretch();
        _btnReset = new QPushButton(fitRow);
        _btnReset->setObjectName("pushButton_explodeReset");
        _btnReset->setFixedSize(kActionW, kControl);
        _btnReset->setStyleSheet(QString(btnQss).arg(kCtrlBorder));
        _btnReset->setCursor(Qt::PointingHandCursor);
        fitLayout->addWidget(_btnReset);
        _btnMinus = new QPushButton(fitRow);
        _btnMinus->setObjectName("pushButton_explodeMinus");
        _btnMinus->setFixedSize(kControl, kControl);
        _btnMinus->setStyleSheet(
            QString("QPushButton{border:1px solid %1;border-radius:4px;background:#FFFFFF;}"
                "QPushButton:hover{background:%2;}"
                "QPushButton:disabled{background:#F5F7FA;border-color:%1;}")
            .arg(kCtrlBorder, kHoverBg));
        _btnMinus->setCursor(Qt::PointingHandCursor);
        _btnMinus->installEventFilter(this);
        fitLayout->addWidget(_btnMinus);
        _btnPlus = new MxFunctions::mPrimaryButton(QString(), fitRow);
        _btnPlus->setObjectName("pushButton_explodePlus");
        _btnPlus->setFixedSize(kControl, kControl);
        _btnPlus->setCursor(Qt::PointingHandCursor);
        _btnPlus->installEventFilter(this);
        fitLayout->addWidget(_btnPlus);
        refreshStepIcons();
        _ctrlFold->addRow(fitRow);

        connect(_btnPlus, &QPushButton::clicked, this, &mPostExplodePanel::slot_plusSlot);
        connect(_btnMinus, &QPushButton::clicked, this, &mPostExplodePanel::slot_minusSlot);
        connect(_btnReset, &QPushButton::clicked, this, &mPostExplodePanel::slot_reset);

        /******** 统一编辑契约操作区（V1.3：控制区底部整行双按钮） ********/
        _editor = new mPostObjectEditController(this);
        _editor->setSnapshotFn([this]()
            {
                //快照=当前 UI 状态（编辑中实时值），基线在 beginEdit/markSaved 时取
                return draftSignature(collectUIDraft(), _nameEdit->text());
            });
        _editor->setSaveFn([this](bool immediate) { return commitEdit(immediate); });
        _editor->setCancelFn([this]() { cancelEdit(); });
        _editor->setCurrentNameFn([this]()
            {
                const ExplodeEntry* e = editingEntry();
                return e ? e->name : QString::fromUtf8("当前爆炸图");
            });
        connect(_editor, &mPostObjectEditController::saveRequested, this,
            [this](bool immediate) { if (!immediate) commitEdit(false); });
        connect(_editor, &mPostObjectEditController::cancelRequested, this, [this]() { cancelEdit(); });
        _editor->attachFooter(nullptr, -1, _ctrlFold);
        _ctrlFold->addRow(_editor->footerWidget());

        //编辑字段变更通知（dirty 重算/即时保存调度；装载期间由 _loadingDraft 屏蔽）
        const auto notify = [this]()
            {
                if (!_loadingDraft && _editor)
                    _editor->notifyEditChanged();
            };
        connect(_nameEdit, &QLineEdit::textChanged, this, [this, notify](const QString&) {
            notify();
            //名称行编辑时恢复为字段校验态（清掉 commit 写入的名称错误）
            if (_errorLine->isVisible() && settingsError().isEmpty())
            {
                const QString nameErr = explodeNameError();
                _errorLine->setText(nameErr);
                _errorLine->setVisible(!nameErr.isEmpty());
            }
            });
        connect(_editDistance, &QLineEdit::textChanged, this, [this, notify](const QString&) { notify(); refreshErrorLine(); });
        connect(_editFactor, &QLineEdit::textChanged, this, [this, notify](const QString&) { notify(); refreshErrorLine(); });
        for (QLineEdit* edit : { _editX, _editY, _editZ })
            connect(edit, &QLineEdit::textChanged, this, [this, notify](const QString&) { notify(); refreshErrorLine(); });
        for (QLineEdit* edit : { _startEdit, _endEdit, _vecEdit })
            connect(edit, &QLineEdit::textChanged, this, [this, notify](const QString&) { notify(); refreshErrorLine(); });
        connect(_checkAutofit, &QCheckBox::toggled, this, [notify](bool) { notify(); });
        connect(_comboMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, notify](int) { notify(); });
        connect(_comboScale, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, notify](int) { notify(); });

        //控制折叠组收起=退出拾取（云图/向量图/阵列"收折叠组退拾取"同口径）
        connect(_ctrlFold, &FoldSection::foldExpandChanged, this, [this](bool expand) {
            if (!expand && _pickController != nullptr && _pickController->isPickActive())
                _pickController->setPickActive(false);
            });

        _mainLayout->addWidget(_ctrlFold);

        _mainLayout->addStretch();
    }

    // ============ 选择部件（mPost 拾取条数据读取） ============

    std::set<int> mPostExplodePanel::pickedPartIds() const
    {
        std::set<int> ids;
        if (_pickController == nullptr) return ids;
        const auto& picked = _pickController->getAllData().pickedData;
        const auto it = picked.find(MxFunctions::PickDataType::Part);
        if (it != picked.end())
            for (const auto& item : it->second)
                if (item.part_id >= 0)
                    ids.insert(item.part_id);
        return ids;
    }

    void mPostExplodePanel::hideEvent(QHideEvent* event)
    {
        //切到其他功能面板（面板宿主 setMainWidget 换主控件即隐藏本面板）：
        //先把当前爆炸状态落账到激活的自建条目，再清除爆炸显示并立即重绘，
        //让截面/云图等其他功能看到装配态模型；切回时由 showEvent 按激活条目恢复。
        //拾取条同步收口（云图/向量图/阵列同款：激活态解除+清高亮，弹窗随宿主隐藏）
        if (_pickController != nullptr)
            _pickController->setPickWidgetVisible(false);
        captureActiveEntryState();
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend != nullptr)
        {
            if (auto postRender = postRend->getPostRender())
            {
                // 爆炸图收编（unify-addobject-arch）：清除=removeObject（无对象时等价现状空爆安全）
                const int _exId = PostCommon::firstSceneObjectId(postRender.get(), mxr::SceneObjectType::ExplodedView);
                if (_exId >= 0)
                    postRender->removeObject(_exId);
                postRend->updatePaint();
            }
        }
        mPanelBase::hideEvent(event);
    }

    void mPostExplodePanel::showEvent(QShowEvent* event)
    {
        mPanelBase::showEvent(event);
        //隐藏期 setPickWidgetVisible(false) 的对称恢复（已拾取数据保留在控件数据模型）
        if (_pickController != nullptr)
            _pickController->setPickWidgetVisible(true);
        //切回爆炸图面板：按当前激活条目恢复视口
        //(自建→应用记录的爆炸状态；自动→重新生成；不显示→保持装配态不处理)
        ExplodeEntry* e = entryById(_activeId);
        if (e == nullptr) return;
        if (e->kind == kKindUser)
            applyEntryState(e);
        else if (e->kind == kKindAuto)
            autogenerate();
    }

    // ============ 爆炸图管理 ============

    int mPostExplodePanel::entryIdAt(int row) const
    {
        QTableWidgetItem* nameIt = _table->item(row, 1);
        return nameIt ? nameIt->data(Qt::UserRole).toInt() : -1;
    }

    mPostExplodePanel::ExplodeEntry* mPostExplodePanel::entryById(int id)
    {
        for (ExplodeEntry& e : _entries)
            if (e.id == id) return &e;
        return nullptr;
    }

    void mPostExplodePanel::rebuildTable()
    {
        _table->setRowCount(0);
        for (const ExplodeEntry& e : _entries)
        {
            const int r = _table->rowCount();
            _table->insertRow(r);

            //复选列占位（真实复选框以控件覆盖，居中；M2 复选 13 水平垂直居中）
            QTableWidgetItem* checkIt = new QTableWidgetItem();
            checkIt->setFlags(Qt::ItemIsEnabled);
            _table->setItem(r, 0, checkIt);

            //复选控件（常驻条目置灰不可勾选，仅自建条目可勾选产生选中状态）
            QWidget* checkHost = new QWidget(_table);
            checkHost->setStyleSheet("background:transparent;");
            QHBoxLayout* checkLayout = new QHBoxLayout(checkHost);
            checkLayout->setContentsMargins(0, 0, 0, 0);
            checkLayout->setAlignment(Qt::AlignCenter);
            QCheckBox* check = new QCheckBox(checkHost);
            check->setCursor(Qt::PointingHandCursor);
            check->setProperty("entryId", e.id);
            check->setChecked(e.checked);
            check->setEnabled(e.kind == kKindUser);
            check->setStyleSheet("QCheckBox{background:transparent;}");
            checkLayout->addWidget(check);
            _table->setCellWidget(r, 0, checkHost);
            connect(check, &QCheckBox::toggled, this, &mPostExplodePanel::slot_checkToggled);

            //名称（UserRole 存条目 id；常驻条目名随语言重译，自建名保持创建时文本）
            QString name = e.name;
            if (e.kind == kKindNone)
                name = _langEn ? QString("No Explode") : QString::fromUtf8("不显示爆炸");
            else if (e.kind == kKindAuto)
                name = _langEn ? QString("Auto Exploded View") : QString::fromUtf8("自动爆炸图");
            QTableWidgetItem* nameIt = MxFunctions::mPanelTable::makeNameItem(name, e.id);
            nameIt->setFlags(Qt::ItemIsEnabled);	//NoSelection：无选中语义
            nameIt->setForeground(QBrush(e.id == _editingId && e.kind == kKindUser
                ? QColor(0x17, 0x6B, 0xA6) : QColor(kTextColor)));
            nameIt->setToolTip(e.kind == kKindUser
                ? QString::fromUtf8("编辑 %1").arg(name) : name);
            _table->setItem(r, 1, nameIt);

            //操作列占位（背景随勾选行高亮，操作按钮浮于其上）
            QTableWidgetItem* opIt = new QTableWidgetItem();
            opIt->setFlags(Qt::ItemIsEnabled);
            _table->setItem(r, 2, opIt);

            //操作（激活/已激活，50×24；激活=selected 底蓝字，M2/剖切同款形态）
            QWidget* opHost = new QWidget(_table);
            opHost->setStyleSheet("background:transparent;");
            QHBoxLayout* opLayout = new QHBoxLayout(opHost);
            opLayout->setContentsMargins(0, 0, 0, 0);
            opLayout->setAlignment(Qt::AlignCenter);
            QPushButton* opBtn = new QPushButton(opHost);
            opBtn->setProperty("entryId", e.id);
            opBtn->setCheckable(true);
            opBtn->setChecked(e.id == _activeId);
            opBtn->setFixedSize(50, kInner);
            opBtn->setCursor(Qt::PointingHandCursor);
            opBtn->setStyleSheet(
                QString("QPushButton{border:1px solid %1;border-radius:4px;background:#FFFFFF;color:#303313;}"
                    "QPushButton:hover{background:%2;}")
                .arg(kCtrlBorder, kHoverBg)
                + QString("QPushButton:checked{background:%1;color:#176BA6;border-color:%2;}")
                .arg(kSelectedBg, kPrimary));
            opBtn->setText(e.id == _activeId
                ? (_langEn ? QString("Active") : QString::fromUtf8("已激活"))
                : (_langEn ? QString("Activate") : QString::fromUtf8("激活")));
            opLayout->addWidget(opBtn);
            _table->setCellWidget(r, 2, opHost);
            connect(opBtn, &QPushButton::clicked, this, &mPostExplodePanel::slot_activateView);

            refreshRowCheckVisual(r);
        }

        //搜索过滤（保持行序不变，仅隐藏不匹配的自建条目行）+ 管理区状态
        const QString query = _searchEdit ? _searchEdit->text().trimmed() : QString();
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            const ExplodeEntry* e = entryById(entryIdAt(r));
            if (!e) continue;
            _table->setRowHidden(r, e->kind == kKindUser && !query.isEmpty()
                && !e->name.contains(query, Qt::CaseInsensitive));
        }
        refreshManagerState();
    }

    void mPostExplodePanel::refreshRowCheckVisual(int row)
    {
        ExplodeEntry* e = entryById(entryIdAt(row));
        //整行高亮：复选格/名称格/操作格空白处一并着色（操作按钮浮于色块之上；
        //共享件口径：selected/surface 随 UiPalette）
        _table->setRowChecked(row, e != nullptr && e->checked);
    }

    void mPostExplodePanel::renderRow(int row)
    {
        const int id = entryIdAt(row);
        ExplodeEntry* e = entryById(id);
        if (!e) return;

        QString name = e->name;
        if (e->kind == kKindNone)
            name = _langEn ? QString("No Explode") : QString::fromUtf8("不显示爆炸");
        else if (e->kind == kKindAuto)
            name = _langEn ? QString("Auto Exploded View") : QString::fromUtf8("自动爆炸图");
        _table->item(row, 1)->setText(name);

        QWidget* opHost = _table->cellWidget(row, 2);
        QPushButton* opBtn = opHost ? opHost->findChild<QPushButton*>() : nullptr;
        if (opBtn)
        {
            const bool active = e->id == _activeId;
            opBtn->setChecked(active);
            opBtn->setText(active
                ? (_langEn ? QString("Active") : QString::fromUtf8("已激活"))
                : (_langEn ? QString("Activate") : QString::fromUtf8("激活")));
        }
    }

    void mPostExplodePanel::slot_checkToggled()
    {
        QCheckBox* check = qobject_cast<QCheckBox*>(sender());
        if (!check) return;
        ExplodeEntry* e = entryById(check->property("entryId").toInt());
        if (!e) return;
        e->checked = check->isChecked();
        for (int r = 0; r < _table->rowCount(); ++r)
            if (entryIdAt(r) == e->id)
                refreshRowCheckVisual(r);
    }

    void mPostExplodePanel::slot_selectAllClicked()
    {
        //V1.3：全选仅作用可见自建条目，隐藏勾选保留
        bool hasVisible = false;
        bool allChecked = true;
        const QString query = _searchEdit->text().trimmed();
        for (const ExplodeEntry& e : _entries)
        {
            if (e.kind != kKindUser) continue;
            if (!query.isEmpty() && !e.name.contains(query, Qt::CaseInsensitive)) continue;
            hasVisible = true;
            if (!e.checked) allChecked = false;
        }
        if (!hasVisible) { refreshManagerState(); return; }

        const bool next = !allChecked;
        for (ExplodeEntry& e : _entries)
        {
            if (e.kind != kKindUser) continue;
            if (!query.isEmpty() && !e.name.contains(query, Qt::CaseInsensitive)) continue;
            e.checked = next;
        }
        rebuildTable();
    }

    void mPostExplodePanel::slot_newView()
    {
        beginCreate();
    }

    void mPostExplodePanel::slot_deleteViews()
    {
        //移除勾选条目；常驻条目不删
        QList<ExplodeEntry> remain;
        bool activeRemoved = false;
        bool editingRemoved = false;
        for (const ExplodeEntry& e : _entries)
        {
            if (e.checked && e.kind == kKindUser)
            {
                if (e.id == _activeId)
                    activeRemoved = true;
                if (e.id == _editingId)
                    editingRemoved = true;
                continue;
            }
            remain.append(e);
        }
        if (remain.size() == _entries.size())
            return;

        _entries = remain;
        if (activeRemoved)
            _activeId = 1;	//回落到"不显示爆炸"

        //V1.3：编辑对象独立于删除；被删则回落首个自建条目，删空退出编辑态
        if (editingRemoved)
        {
            _editingId = -1;
            for (const ExplodeEntry& e : _entries)
                if (e.kind == kKindUser) { _editingId = e.id; break; }
        }
        rebuildTable();
        if (_editingId != -1)
        {
            loadDraftToUI(editingEntry()->draft);
            _nameEdit->setText(editingEntry()->name);
            _editor->beginEdit();
        }
        else
        {
            _editor->endEdit();
            _nameEdit->clear();
        }
        updateEditState();
        if (activeRemoved)
            activateEntry(_activeId, true);
    }

    void mPostExplodePanel::slot_activateView()
    {
        QPushButton* btn = qobject_cast<QPushButton*>(sender());
        if (!btn) return;
        const int id = btn->property("entryId").toInt();
        activateEntry(id);
        //同 id 再点为 no-op：恢复按钮勾选视觉（checkable 点击会先翻转勾选态）
        if (id == _activeId)
            for (int r = 0; r < _table->rowCount(); ++r)
                if (entryIdAt(r) == id) { renderRow(r); break; }
    }

    void mPostExplodePanel::activateEntry(int id, bool force)
    {
        ExplodeEntry* e = entryById(id);
        if (!e || (!force && id == _activeId)) return;

        //切换前先把视口当前爆炸状态落账到原激活的自建条目（记录爆炸：
        //+/-等操作实时作用于视口，条目状态在切换瞬间记录）
        captureActiveEntryState();

        //"不显示爆炸"清除爆炸显示；"自动爆炸图"自动生成；自建爆炸图应用其记录的爆炸状态
        //激活态恒有且只有 1 个：先切换按钮态，再按条目类型应用到视口
        _activeId = id;
        for (int r = 0; r < _table->rowCount(); ++r)
            renderRow(r);
        updateExplodeCtrlsEnabled();	//"不显示爆炸"激活时置灰底部调节控件

        if (e->kind == kKindUser)
        {
            applyEntryState(e);
            return;
        }

        if (e->kind == kKindNone)
        {
            mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
            if (postRend == nullptr) return;
            auto postRender = postRend->getPostRender();
            if (postRender == nullptr) return;

            // 爆炸图收编（unify-addobject-arch）：清除=removeObject（无对象时等价现状空爆安全）
            const int _exId = PostCommon::firstSceneObjectId(postRender.get(), mxr::SceneObjectType::ExplodedView);
            if (_exId >= 0)
                postRender->removeObject(_exId);
            if (_checkAutofit->isChecked())
            {
                postRend->FitView();
                postRend->updatePaint();
            }
            else {
                postRend->updatePaint();
            }
        }
        else if (e->kind == kKindAuto)
        {
            autogenerate();
        }
    }

    void mPostExplodePanel::captureActiveEntryState()
    {
        ExplodeEntry* cur = entryById(_activeId);
        if (cur == nullptr || cur->kind != kKindUser) return;
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;	//渲染端不可用：保留条目原记录
        // 爆炸图收编（unify-addobject-arch）：快照读=getObject(id).explodeStates
        //（查询输出成员，现状 getExplodeStates 读路径同源）
        const int _exId = PostCommon::firstSceneObjectId(postRender.get(), mxr::SceneObjectType::ExplodedView);
        cur->state = (_exId >= 0) ? postRender->getObject(_exId).explodeStates : QHash<int, QVector3D>();
    }

    void mPostExplodePanel::applyEntryState(ExplodeEntry* e)
    {
        if (e == nullptr) return;
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        //空记录=未爆炸状态（如刚复位过），按清除处理；非空按记录的各部件位移覆盖
        //（爆炸图收编 unify-addobject-arch：清除=removeObject；恢复=explodeStates
        //覆盖段 updateObject——绝对位移覆盖语义原样，台账无对象时 addObject 建账）
        if (e->state.isEmpty())
        {
            const int _exId = PostCommon::firstSceneObjectId(postRender.get(), mxr::SceneObjectType::ExplodedView);
            if (_exId >= 0)
                postRender->removeObject(_exId);
        }
        else
        {
            mxr::ExplodedViewParams _ex;
            _ex.explodeStates = e->state;
            PostCommon::ensureUpdateSceneObject(postRender.get(), _ex);
        }
        postRender->updateHighLightRender();
        if (_checkAutofit->isChecked())
        {
            postRend->FitView();
            postRend->updatePaint();
        }
        else {
            postRend->updatePaint();
            postRend->SetRotateCenterToModelCenter();
        }
    }

    // ============ 统一编辑契约（V1.3；仅自建条目可编辑） ============

    mPostExplodePanel::ExplodeEntry* mPostExplodePanel::editingEntry()
    {
        ExplodeEntry* e = entryById(_editingId);
        return (e && e->kind == kKindUser) ? e : nullptr;
    }

    mPostExplodePanel::ExplodeDraft mPostExplodePanel::collectUIDraft() const
    {
        ExplodeDraft d;
        d.pickedParts = pickedPartIds();
        d.mode = _comboMode->currentIndex();
        d.scale = _comboScale->currentIndex();
        d.factor = _editFactor->text();
        d.x = _editX->text();
        d.y = _editY->text();
        d.z = _editZ->text();
        d.distance = _editDistance->text();
        d.direction = _dirRow->combo()->currentIndex();
        d.reversed = _dirRow->reversed();
        d.start = _startEdit->text();
        d.end = _endEdit->text();
        d.vector = _vecEdit->text();
        d.autofit = _checkAutofit->isChecked();
        return d;
    }

    void mPostExplodePanel::loadDraftToUI(const ExplodeDraft& d)
    {
        _loadingDraft = true;
        //拾取条反显（displayData 不发 dataChanged，装载期屏障双保险）：
        //Part 条目按收割口径构造（result_id=part_id=部件号，result_id2 无关）
        {
            PickWidgetData data;
            std::vector<RenderPickResultData> items;
            items.reserve(d.pickedParts.size());
            for (int id : d.pickedParts)
                items.emplace_back(id, id, -1);
            if (!items.empty())
                data.pickedData.emplace(MxFunctions::PickDataType::Part, std::move(items));
            _pickController->displayData(data);
        }
        _comboMode->setCurrentIndex(d.mode);
        _comboScale->setCurrentIndex(d.scale);
        _editFactor->setText(d.factor);
        _editX->setText(d.x);
        _editY->setText(d.y);
        _editZ->setText(d.z);
        _editDistance->setText(d.distance);
        _dirRow->combo()->setCurrentIndex(d.direction);
        _dirRow->setReversed(d.reversed);
        _startEdit->setText(d.start);
        _endEdit->setText(d.end);
        _vecEdit->setText(d.vector);
        _checkAutofit->setChecked(d.autofit);
        _loadingDraft = false;
        applyModeRows();
        refreshErrorLine();
    }

    QString mPostExplodePanel::draftSignature(const ExplodeDraft& d, const QString& name)
    {
        QStringList ids;
        for (int id : d.pickedParts) ids.append(QString::number(id));
        return name + QChar(1)
            + ids.join(QChar(2)) + QChar(1)
            + QString::number(d.mode) + QChar(1) + QString::number(d.scale) + QChar(1)
            + d.factor + QChar(1) + d.x + QChar(1) + d.y + QChar(1) + d.z + QChar(1)
            + d.distance + QChar(1) + QString::number(d.direction) + QChar(1)
            + QString::number(d.reversed ? 1 : 0) + QChar(1)
            + d.start + QChar(1) + d.end + QChar(1) + d.vector + QChar(1)
            + QString::number(d.autofit ? 1 : 0);
    }

    QString mPostExplodePanel::explodeNameError() const
    {
        const QString name = _nameEdit->text();
        if (name.trimmed().isEmpty())
            return _langEn ? QString("Enter a name") : QString::fromUtf8("请输入爆炸图名称");
        for (const ExplodeEntry& e : _entries)
            if (e.kind == kKindUser && e.id != _editingId && e.name == name.trimmed())
                return _langEn ? QString("Name already exists") : QString::fromUtf8("爆炸图名称已存在");
        return QString();
    }

    void mPostExplodePanel::updateEditState()
    {
        //V1.3 updateEditState：无编辑对象（常驻条目激活/删空）时控制区整体禁用
        const bool editing = (_editingId != -1);
        if (QWidget* content = _ctrlFold->contentWidget())
            content->setEnabled(editing);
        _nameEdit->setPlaceholderText(editing ? QString()
            : (_langEn ? QString("Click New to add") : QString::fromUtf8("点击新建添加爆炸图")));
    }

    void mPostExplodePanel::selectEntry(int id)
    {
        ExplodeEntry* target = entryById(id);
        if (!target || target->kind != kKindUser || id == _editingId)
            return;
        const auto action = [this, id]()
        {
            ExplodeEntry* e = entryById(id);
            if (!e) return;
            _editingId = id;
            _nameEdit->setText(e->name);
            loadDraftToUI(e->draft);
            _editor->beginEdit();
            rebuildTable();
            updateEditState();
        };
        _editor->checkSwitch(action);
    }

    void mPostExplodePanel::beginCreate()
    {
        const auto action = [this]()
        {
            //V1.3：登记唯一 ID+默认名并选中激活；空记录按"清除爆炸"应用=复位样式
            ExplodeEntry e;
            e.id = _nextViewId++;
            e.kind = kKindUser;
            QStringList used;
            for (const ExplodeEntry& it : _entries) used.append(it.name);
            e.name = postObjectUniqueName(QString::fromUtf8("自定义爆炸图_%1").arg(++_newViewCount), used);

            //V1.3 终态：新条目追加到列表末尾（explosionRecords.push）、不改动勾选集
            _entries.append(e);

            //切换激活前先落账原激活自建条目；新建条目激活+进入编辑（勾选集不动）
            captureActiveEntryState();
            const int newId = e.id;
            _activeId = newId;
            _editingId = newId;
            _searchEdit->blockSignals(true);
            _searchEdit->clear();
            _searchEdit->blockSignals(false);
            _nameEdit->setText(e.name);
            loadDraftToUI(e.draft);
            _editor->beginEdit();
            rebuildTable();
            updateExplodeCtrlsEnabled();
            updateEditState();
            applyEntryState(entryById(newId));	//空记录按"清除爆炸"应用：新建即呈现复位样式
            _nameEdit->setFocus();
        };
        _editor->checkSwitch(action);
    }

    void mPostExplodePanel::cancelEdit()
    {
        ExplodeEntry* e = editingEntry();
        if (!e) return;
        _nameEdit->setText(e->name);
        loadDraftToUI(e->draft);
        _editor->markSaved();
        rebuildTable();
    }

    bool mPostExplodePanel::commitEdit(bool immediate)
    {
        ExplodeEntry* e = editingEntry();
        if (!e) return false;

        //校验：名称（空/重名）+ 控制字段（原型 validateExplosionSettings）
        const QString nameProblem = explodeNameError();
        const bool settingsOk = settingsError().isEmpty();
        if (!nameProblem.isEmpty() || !settingsOk)
        {
            if (!nameProblem.isEmpty())
                _errorLine->setText(nameProblem);	//名称错误优先入错误行（原型 9548）
            _errorLine->setVisible(true);
            if (!immediate && !nameProblem.isEmpty())
                _nameEdit->setFocus();
            return false;
        }

        e->draft = collectUIDraft();
        e->name = _nameEdit->text().trimmed();
        if (!immediate)
        {
            _nameEdit->setText(e->name);	//名称 trim 归一回显
            loadDraftToUI(e->draft);
        }
        _editor->markSaved();
        rebuildTable();
        return true;
    }

    void mPostExplodePanel::refreshManagerState()
    {
        //全选文案/删除可用性/已选计数（含搜索范围外勾选提示）
        const QString query = _searchEdit ? _searchEdit->text().trimmed() : QString();
        int visibleCount = 0, visibleChecked = 0, hiddenChecked = 0, totalChecked = 0;
        for (const ExplodeEntry& e : _entries)
        {
            if (e.kind != kKindUser) continue;
            const bool visible = query.isEmpty() || e.name.contains(query, Qt::CaseInsensitive);
            if (visible)
            {
                ++visibleCount;
                if (e.checked) ++visibleChecked;
            }
            else if (e.checked)
                ++hiddenChecked;
            if (e.checked) ++totalChecked;
        }
        const bool all = visibleCount > 0 && visibleChecked == visibleCount;
        _btnSelectAll->setText(all ? QString::fromUtf8("取消") : QString::fromUtf8("全选"));
        _btnSelectAll->setEnabled(visibleCount > 0);
        _btnDelete->setEnabled(totalChecked > 0);
        _selectedCount->setText(hiddenChecked > 0
            ? QString::fromUtf8("已选 %1 项（筛选外 %2）").arg(totalChecked).arg(hiddenChecked)
            : QString::fromUtf8("已选 %1 项").arg(totalChecked));
    }

    void mPostExplodePanel::slot_searchChanged(const QString&)
    {
        //仅筛列表，不改选择/激活（原型 filter）
        const QString query = _searchEdit->text().trimmed();
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            const int id = entryIdAt(r);
            const ExplodeEntry* e = entryById(id);
            if (!e) continue;
            const bool hide = e->kind == kKindUser && !query.isEmpty()
                && !e->name.contains(query, Qt::CaseInsensitive);
            _table->setRowHidden(r, hide);
        }
        refreshManagerState();
    }

    void mPostExplodePanel::slot_cellClicked(int row, int column)
    {
        //行点击=选中编辑（V1.3：行空白处点击即 fbd-name-button 语义；复选/激活钮
        //为单元格控件自成点击，不进此槽）
        Q_UNUSED(column);
        const int id = entryIdAt(row);
        ExplodeEntry* e = entryById(id);
        if (e && e->kind == kKindUser)
            selectEntry(id);
    }

    // ============ 爆炸图控制 ============

    void mPostExplodePanel::slot_modeChanged(int)
    {
        applyModeRows();
        refreshErrorLine();
    }

    void mPostExplodePanel::slot_scaleChanged(int)
    {
        applyModeRows();
        refreshErrorLine();
    }

    void mPostExplodePanel::applyModeRows()
    {
        //0=平移：距离+平移方向骑线框；1/2=从模型中心爆炸/从所选部件中心爆炸：
        //缩放方式+变形系数（等比例单值/分方向三行）
        const bool translate = _comboMode->currentIndex() == 0;
        const bool center = !translate;
        const bool uniform = _comboScale->currentIndex() == 0;	//等比例

        //方向附属子块在骑线框内部，按方向类型显隐：
        //0/1/2=X/Y/Z轴无附属；3=起点-终点；4=选择对象；5=向量
        const int dir = _dirRow->combo()->currentIndex();
        _ctrlFold->setRowVisible(_editDistance, translate);
        _ctrlFold->setRowVisible(_dirBox, translate);
        const bool startEnd = dir == 3;
        _startRowWidget->setVisible(startEnd);
        _endRowWidget->setVisible(startEnd);
        _selDirRowWidget->setVisible(dir == 4);
        _vecRowWidget->setVisible(dir == 5);
        _ctrlFold->setRowVisible(_comboScale, center);
        _ctrlFold->setRowVisible(_editFactor, center && uniform);
        _ctrlFold->setRowVisible(_editX, center && !uniform);
        _ctrlFold->setRowVisible(_editY, center && !uniform);
        _ctrlFold->setRowVisible(_editZ, center && !uniform);
        _ctrlFold->refreshLayout();
    }

    void mPostExplodePanel::updateExplodeCtrlsEnabled()
    {
        //"不显示爆炸"激活时无爆炸可调：自适应视图/复位/−/+ 置灰；
        //其余条目（自动爆炸图/自建）保持可调
        const bool enabled = (_activeId != 1);
        _checkAutofit->setEnabled(enabled);
        _btnReset->setEnabled(enabled);
        _btnMinus->setEnabled(enabled);
        _btnPlus->setEnabled(enabled);
        refreshStepIcons();
    }

    QString mPostExplodePanel::settingsError() const
    {
        const bool translate = _comboMode->currentIndex() == 0;
        if (translate)
        {
            const QString dist = _editDistance->text().trimmed();
            bool distOk = false;
            const double distVal = dist.toDouble(&distOk);
            if (dist.isEmpty() || !distOk || !qIsFinite(distVal))
                return _langEn ? QString("Enter a valid translation distance.")
                    : QString::fromUtf8("请输入有效的平移距离。");
            //方向可解性（原型 resolveExplosionDirection）
            const int dir = _dirRow->combo()->currentIndex();
            if (dir == 4)
                return _langEn ? QString("Select a valid direction object.")
                    : QString::fromUtf8("请选择有效的方向对象。");
            if (dir == 3)
            {
                QVector3D s, e;
                const bool ok = parseCoord(_startEdit->text(), &s) && parseCoord(_endEdit->text(), &e);
                if (!ok || qFuzzyCompare(s, e))
                    return _langEn
                        ? QString("Enter three valid values; vector must be nonzero and points must differ.")
                        : QString::fromUtf8("请输入三个有效数值，方向向量不能为零，起点与终点不能重合。");
            }
            else if (dir == 5)
            {
                QVector3D v;
                if (!parseCoord(_vecEdit->text(), &v) || v.lengthSquared() < 1e-12f)
                    return _langEn
                        ? QString("Enter three valid values; vector must be nonzero and points must differ.")
                        : QString::fromUtf8("请输入三个有效数值，方向向量不能为零，起点与终点不能重合。");
            }
        }
        else
        {
            const QStringList fields = _comboScale->currentIndex() == 0
                ? QStringList{ _editFactor->text() }
                : QStringList{ _editX->text(), _editY->text(), _editZ->text() };
            for (const QString& f : fields)
            {
                const QString t = f.trimmed();
                bool ok = false;
                const double v = t.toDouble(&ok);
                if (t.isEmpty() || !ok || !qIsFinite(v))
                    return _langEn ? QString("Enter a valid scale factor.")
                        : QString::fromUtf8("请输入有效的变形系数。");
            }
        }
        return QString();
    }

    void mPostExplodePanel::refreshErrorLine()
    {
        const QString err = settingsError();
        _errorLine->setText(err);
        _errorLine->setVisible(!err.isEmpty());
    }

    std::set<int> mPostExplodePanel::getSelectedPartIds()
    {
        std::set<int> partIds;
        if (_postDB == nullptr) return partIds;

        //拾取条已拾取部件 → 仅该集合爆炸（对齐旧版"通过材料"勾选部件的行为口径，
        //原范围弹窗/材料级联随拾取条接入下线）；一个都没拾取时保持全部，与未筛选衔接
        std::set<int> picked = pickedPartIds();
        if (!picked.empty())
            return picked;

        QVector<int> ids = _postDB->getAllPartId();
        for (auto id : ids)
            partIds.insert(id);
        return partIds;
    }

    QVector3D mPostExplodePanel::getTranslateVector(bool isPlus)
    {
        //方向类型：0/1/2=X/Y/Z轴；3=起点-终点；4=选择对象（待接入拾取）；5=向量。
        //统一按单位方向 × 距离计算（isPlus 决定正负向；反向钮翻转，V1.3 reversed）
        float sign = isPlus ? 1.0f : -1.0f;
        if (_dirRow->reversed())
            sign = -sign;
        const float dist = _editDistance->text().toFloat() * sign;
        QVector3D dir(0, 0, 0);

        switch (_dirRow->combo()->currentIndex())
        {
        case 0:
            dir = QVector3D(1, 0, 0);
            break;
        case 1:
            dir = QVector3D(0, 1, 0);
            break;
        case 2:
            dir = QVector3D(0, 0, 1);
            break;
        case 3:	//终点-起点
        {
            QVector3D s, e;
            if (parseCoord(_startEdit->text(), &s) && parseCoord(_endEdit->text(), &e))
                dir = e - s;
            break;
        }
        case 5:	//方向向量
        {
            QVector3D v;
            if (parseCoord(_vecEdit->text(), &v))
                dir = v;
            break;
        }
        default:
            return QVector3D(0, 0, 0);
        }

        if (dir.lengthSquared() < 1e-12f) return QVector3D(0, 0, 0);
        return dir.normalized() * dist;
    }

    QVector3D mPostExplodePanel::getModelCenterVector(bool isPlus)
    {
        float sign = isPlus ? 1.0f : -1.0f;

        QVector3D dis(0, 0, 0);
        if (_comboScale->currentIndex() == 0) //等比例
        {
            float val = _editFactor->text().toFloat() * sign;
            dis = QVector3D(val, val, val);
        }
        else //分方向
        {
            float x = _editX->text().toFloat() * sign;
            float y = _editY->text().toFloat() * sign;
            float z = _editZ->text().toFloat() * sign;
            dis = QVector3D(x, y, z);
        }
        return dis;
    }

    void mPostExplodePanel::applyExplode(bool plus)
    {
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        std::set<int> partIds = getSelectedPartIds();

        //0=平移；1/2=从模型中心/从所选部件中心爆炸（"从所选部件中心"暂同模型中心接口，待接入）
        //爆炸图收编（unify-addobject-arch）：按部件平移/沿模型中心两模式=addObject
        //（Q7=A 重复创建=改参替换保持既有 id，无需先 remove 再 add）
        if (_comboMode->currentIndex() == 0)
        {
            mxr::ExplodedViewParams _ex;
            _ex.mode = mxr::ExplodedViewParams::Mode::TranslatePart;
            _ex.partNames = partIds;
            _ex.translation = getTranslateVector(plus);
            postRender->addObject(_ex);
        }
        else
        {
            mxr::ExplodedViewParams _ex;
            _ex.mode = mxr::ExplodedViewParams::Mode::ModelCenter;
            _ex.partNames = partIds;
            _ex.factor = getModelCenterVector(plus);
            postRender->addObject(_ex);
        }

        postRender->updateHighLightRender();
        if (_checkAutofit->isChecked())
        {
            postRend->FitView();
            postRend->updatePaint();
        }
        else {
            postRend->updatePaint();
            postRend->SetRotateCenterToModelCenter();
        }
    }

    void mPostExplodePanel::autogenerate()
    {
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        // 爆炸图收编（unify-addobject-arch）：自动模式=addObject（Q7=A 改参替换）
        {
            mxr::ExplodedViewParams _ex;
            _ex.mode = mxr::ExplodedViewParams::Mode::Auto;
            postRender->addObject(_ex);
        }

        //自动生成后同步激活"自动爆炸图"条目（保持列表激活态与视口一致）
        if (_activeId != 2)
        {
            _activeId = 2;
            for (int r = 0; r < _table->rowCount(); ++r)
                renderRow(r);
        }

        if (_checkAutofit->isChecked())
        {
            postRend->FitView();
            postRend->updatePaint();
        }
        else {
            postRend->updatePaint();
            postRend->SetRotateCenterToModelCenter();
        }
    }

    void mPostExplodePanel::slot_plusSlot()
    {
        applyExplode(true);
    }

    void mPostExplodePanel::slot_minusSlot()
    {
        applyExplode(false);
    }

    void mPostExplodePanel::slot_reset()
    {
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        if (_pickController->isPickActive())
            _pickController->setPickActive(false);	//拾取态收口
        _pickController->displayData(PickWidgetData{});	//部件选择复位为"所有部件"（清空拾取数据）
        // 爆炸图收编（unify-addobject-arch）：清除=removeObject（无对象时等价现状空爆安全）
        {
            const int _exId = PostCommon::firstSceneObjectId(postRender.get(), mxr::SceneObjectType::ExplodedView);
            if (_exId >= 0)
                postRender->removeObject(_exId);
        }

        //复位作用于当前激活条目：同步清零其记录的爆炸状态；
        //激活态保持不变（不回落"不显示爆炸"），复位后可继续 +/- 重新调整
        ExplodeEntry* active = entryById(_activeId);
        if (active != nullptr && active->kind == kKindUser)
            active->state.clear();

        if (_checkAutofit->isChecked())
        {
            postRend->FitView();
            VMManager->setCameraSyncTimer(500, 30, [postRend]() {
                emit postRend->signal_CameraSync();
                });
            postRend->updatePaint();
        }
        else {
            postRend->SetRotateCenterToModelCenter();
            postRend->updatePaint();
        }
    }

    // ============ 外部调用接口 ============

    void mPostExplodePanel::newExplodePanel(int caseid)
    {
        _postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
    }

    void mPostExplodePanel::setCurViewName(const QString& viewName)
    {
        _curViewName = viewName;
    }

    void mPostExplodePanel::setExplodeTranslate(int x, int y, int z)
    {
        //新方向下拉仅支持单方向，三点平移暂按 X 分量处理（TODO：拆分为多方向调用）
        _comboMode->blockSignals(true);
        _dirRow->combo()->blockSignals(true);
        _comboMode->setCurrentIndex(0);
        _dirRow->combo()->setCurrentIndex(0);	//X轴
        _comboMode->blockSignals(false);
        _dirRow->combo()->blockSignals(false);
        _editDistance->setText(MxFunctions::mNumberToString(x));
        applyModeRows();
        applyExplode(true);
    }

    void mPostExplodePanel::setExplodeModelCenterConsistent(int value)
    {
        _comboMode->blockSignals(true);
        _comboScale->blockSignals(true);
        _comboMode->setCurrentIndex(1);
        _comboScale->setCurrentIndex(0);
        _comboMode->blockSignals(false);
        _comboScale->blockSignals(false);
        _editFactor->setText(MxFunctions::mNumberToString(value));
        applyModeRows();
        applyExplode(true);
    }

    void mPostExplodePanel::setExplodeModelCenterInconsistent(int x, int y, int z)
    {
        _comboMode->blockSignals(true);
        _comboScale->blockSignals(true);
        _comboMode->setCurrentIndex(1);
        _comboScale->setCurrentIndex(1);
        _comboMode->blockSignals(false);
        _comboScale->blockSignals(false);
        _editX->setText(MxFunctions::mNumberToString(x));
        _editY->setText(MxFunctions::mNumberToString(y));
        _editZ->setText(MxFunctions::mNumberToString(z));
        applyModeRows();
        applyExplode(true);
    }

    void mPostExplodePanel::setExplodeAutoFit(int type) {}

    void mPostExplodePanel::setExplodePlus()
    {
        slot_plusSlot();
    }

    void mPostExplodePanel::setExplodeMinus()
    {
        slot_minusSlot();
    }

    void mPostExplodePanel::setExplodeReset()
    {
        slot_reset();
    }

    void mPostExplodePanel::setExplodeAutoGenerate()
    {
        //模式下拉无"自动生成"项，保持当前选择不变，仅触发自动生成
        autogenerate();
    }

    void mPostExplodePanel::setExplodeTranslateX(int x)
    {
        _comboMode->blockSignals(true);
        _dirRow->combo()->blockSignals(true);
        _comboMode->setCurrentIndex(0);
        _dirRow->combo()->setCurrentIndex(0);
        _comboMode->blockSignals(false);
        _dirRow->combo()->blockSignals(false);
        _editDistance->setText(MxFunctions::mNumberToString(x));
        applyModeRows();
        applyExplode(true);
    }

    void mPostExplodePanel::setExplodeTranslateY(int y)
    {
        _comboMode->blockSignals(true);
        _dirRow->combo()->blockSignals(true);
        _comboMode->setCurrentIndex(0);
        _dirRow->combo()->setCurrentIndex(1);
        _comboMode->blockSignals(false);
        _dirRow->combo()->blockSignals(false);
        _editDistance->setText(MxFunctions::mNumberToString(y));
        applyModeRows();
        applyExplode(true);
    }

    void mPostExplodePanel::setExplodeTranslateZ(int z)
    {
        _comboMode->blockSignals(true);
        _dirRow->combo()->blockSignals(true);
        _comboMode->setCurrentIndex(0);
        _dirRow->combo()->setCurrentIndex(2);
        _comboMode->blockSignals(false);
        _dirRow->combo()->blockSignals(false);
        _editDistance->setText(MxFunctions::mNumberToString(z));
        applyModeRows();
        applyExplode(true);
    }

    void mPostExplodePanel::updatePickFilterAndSymbol(bool flag)
    {
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        postRend->setPickFilter(mxr::PickFilter::PickNothing);
        // 清空当前拾取
        getCVM->clearPick();
    }

    // ============ −/＋ 图标随使能态换色 ============

    void mPostExplodePanel::refreshStepIcons()
    {
        _btnMinus->setIcon(minusIcon(_btnMinus->isEnabled()
            ? QColor(kTextColor) : QColor(kDisabledFg)));
        _btnPlus->setIcon(plusIcon(_btnPlus->isEnabled()
            ? QColor("#FFFFFF") : QColor(kDisabledFg)));
    }

    bool mPostExplodePanel::eventFilter(QObject* watched, QEvent* event)
    {
        if ((watched == _btnMinus || watched == _btnPlus)
            && event->type() == QEvent::EnabledChange)
            refreshStepIcons();
        return mPanelBase::eventFilter(watched, event);
    }

    // ============ 语言切换 ============

    void mPostExplodePanel::slot_changeLanguage(int language)
    {
        const int lang = language;
        _langEn = (lang == 1);

        _manageFold->setTitle(_langEn ? QString("Explode View Management") : QString::fromUtf8("爆炸图管理"));
        _searchEdit->setPlaceholderText(_langEn ? QString("Search exploded views")
            : QString::fromUtf8("搜索爆炸图名称"));
        //名称表头带排序箭头（V1.3：未排序↕/升序↑/降序↓）
        const QString nameHeaderText = (_langEn ? QString("Name") : QString::fromUtf8("名称"))
            + QString::fromUtf8(_nameSortState == 0 ? " ↕" : _nameSortState == 1 ? " ↑" : " ↓");
        _table->setHorizontalHeaderItem(0, new QTableWidgetItem(QString()));
        _table->setHorizontalHeaderItem(1, new QTableWidgetItem(nameHeaderText));
        _table->setHorizontalHeaderItem(2, new QTableWidgetItem(
            _langEn ? QString("Action") : QString::fromUtf8("操作")));
        _btnDelete->setText(_langEn ? QString("Del") : QString::fromUtf8("删除"));
        _btnNew->setText(_langEn ? QString("New") : QString::fromUtf8("新建"));
        if (_editor)
            _editor->retranslate(_langEn);
        updateEditState();	//名称占位文案随语言刷新

        _ctrlFold->setTitle(_langEn ? QString("Explode View Control") : QString::fromUtf8("爆炸图控制"));
        _dirBox->setTitle(_langEn ? QString("Translation Direction") : QString::fromUtf8("平移方向"));

        //登记表标签全量重刷（显式表防序号漂移）
        for (const LabelText& lt : _labelTexts)
            lt.label->setText(_langEn ? QString::fromUtf8(lt.en) : QString::fromUtf8(lt.cn));

        //选择部件（隐形控制器重译 → scopeTextChanged → 拾取条空态文案联动；
        //拾取条内建计数/详情文案为拾取栈标准中文，云图/向量图/阵列同款限制留档）
        _scopeCtl->refreshLanguage(lang);

        //爆炸模式
        _comboMode->blockSignals(true);
        int mode = _comboMode->currentIndex();
        _comboMode->clear();
        _comboMode->addItem(_langEn ? QString("Translation") : QString::fromUtf8("平移"));
        _comboMode->addItem(_langEn ? QString("Explosion - center of the model")
            : QString::fromUtf8("从模型中心爆炸"));
        _comboMode->addItem(_langEn ? QString("Explosion - selected component center")
            : QString::fromUtf8("从所选部件中心爆炸"));
        _comboMode->setCurrentIndex(mode < 0 ? 0 : mode);
        _comboMode->blockSignals(false);

        //缩放方式
        _comboScale->blockSignals(true);
        int scale = _comboScale->currentIndex();
        _comboScale->clear();
        _comboScale->addItem(_langEn ? QString("Uniform") : QString::fromUtf8("等比例"));
        _comboScale->addItem(_langEn ? QString("Directional") : QString::fromUtf8("分方向"));
        _comboScale->setCurrentIndex(scale < 0 ? 0 : scale);
        _comboScale->blockSignals(false);

        //平移方向-定义方式（X轴/Y轴/Z轴/起点-终点/选择对象/向量）
        _dirRow->combo()->blockSignals(true);
        int dir = _dirRow->combo()->currentIndex();
        _dirRow->combo()->clear();
        _dirRow->combo()->addItem(_langEn ? QString("X Axis") : QString::fromUtf8("X轴"));
        _dirRow->combo()->addItem(_langEn ? QString("Y Axis") : QString::fromUtf8("Y轴"));
        _dirRow->combo()->addItem(_langEn ? QString("Z Axis") : QString::fromUtf8("Z轴"));
        _dirRow->combo()->addItem(_langEn ? QString("Start - End") : QString::fromUtf8("起点-终点"));
        _dirRow->combo()->addItem(_langEn ? QString("Select Object") : QString::fromUtf8("选择对象"));
        _dirRow->combo()->addItem(_langEn ? QString("Vector") : QString::fromUtf8("向量"));
        _dirRow->combo()->setCurrentIndex(dir < 0 ? 0 : dir);
        _dirRow->combo()->blockSignals(false);

        //起点/终点-选择节点（拾取待接入置灰）与选择方向按钮
        _startPickBtn->setText(_langEn ? QString("Pick Node") : QString::fromUtf8("选择节点"));
        _endPickBtn->setText(_langEn ? QString("Pick Node") : QString::fromUtf8("选择节点"));
        _selDirBtn->setText(_langEn ? QString("Select Object") : QString::fromUtf8("选择对象"));

        //底部操作行
        _checkAutofit->setText(_langEn ? QString("Auto Fit View") : QString::fromUtf8("自适应视图"));
        _btnReset->setText(_langEn ? QString("Reset") : QString::fromUtf8("复位"));
        _btnMinus->setToolTip(_langEn ? QString("Decrease explosion") : QString::fromUtf8("减小爆炸量"));
        _btnPlus->setToolTip(_langEn ? QString("Increase explosion") : QString::fromUtf8("增大爆炸量"));

        //常驻条目名/操作钮文案随语言重灌（保留勾选与激活态）
        rebuildTable();
        refreshManagerState();
        applyModeRows();
        refreshErrorLine();
    }
}
