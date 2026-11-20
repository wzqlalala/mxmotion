#include "mPostSetPanel.h"

#include "mGlobalSignals.h"
#include "mAppConfiguration.h"

#include "mCommonFunctions.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"

#include "mPostModelData.h"
#include "mPostParseMesh.h"
#include "mPostEnumType.h"
#include "mPostGUIData.h"

#include <mMechGlobalSignals.h>

#include "mFoldSection.h"
#include <UiPalette.h>
#include <UiMetrics.h>

#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/data/SelectDataStruct.h"

#include <algorithm>

using namespace PostCommon;
using namespace PostData;
using namespace MechPreData;

namespace PostWindow
{
    namespace
    {
        //模型页"集合"组按钮序（与 slot_createPostSetTyped 的 setType 一致）
        const char* kSetTypeNames[5] = { "部件集", "节点集", "单元边集", "单元面集", "单元集" };

        //类型中文名 -> 拾取过滤器 / 拾取数据类型
        mxr::PickFilter pickFilterOfType(const QString& typeName)
        {
            if (typeName == "节点集") return mxr::PickFilter::PickMeshNode;
            if (typeName == "单元边集") return mxr::PickFilter::PickMeshLine;
            if (typeName == "单元面集") return mxr::PickFilter::PickMeshFace;
            if (typeName == "单元集") return mxr::PickFilter::PickMeshElement;
            return mxr::PickFilter::PickMeshPart;   // 部件集
        }

        mxr::SelectionInfoType pickDataTypeOfType(const QString& typeName)
        {
            if (typeName == "节点集") return mxr::SelectionInfoType::MeshNode;
            if (typeName == "单元边集") return mxr::SelectionInfoType::MeshLine;
            if (typeName == "单元面集") return mxr::SelectionInfoType::MeshFace;
            if (typeName == "单元集") return mxr::SelectionInfoType::MeshElement;
            return mxr::SelectionInfoType::MeshPart;   // 部件集
        }

        //拾取主钮图标随集合类型联动（CenterMainWindow 拾取图标族，已注册 qrc）
        QString pickIconOfType(const QString& typeName)
        {
            if (typeName == "节点集") return ":/mMainWidgets/Icon/CenterMainWindow/shiqujiedian3x.png";
            if (typeName == "单元集") return ":/mMainWidgets/Icon/CenterMainWindow/shiqudanyuan3x.png";
            if (typeName == "单元面集") return ":/mMainWidgets/Icon/CenterMainWindow/shiqumian3x.png";
            if (typeName == "单元边集") return ":/mMainWidgets/Icon/CenterMainWindow/shiqubian3x.png";
            return ":/mMainWidgets/Icon/CenterMainWindow/shiqubujian3x.png";   // 部件集
        }

        //拾取对象短名（原型 LoadPickSample.names：caption "拾取节点/拾取单元…"）
        QString pickNounOfType(const QString& typeName)
        {
            if (typeName == "节点集") return QString::fromUtf8("节点");
            if (typeName == "单元集") return QString::fromUtf8("单元");
            if (typeName == "单元面集") return QString::fromUtf8("面");
            if (typeName == "单元边集") return QString::fromUtf8("边");
            return QString::fromUtf8("部件");
        }

        //——V1.3 通用拾取入口（令牌/图标/形态与 mPostCurveExtract 同源复刻）——
        constexpr int kControl = 26;        //--std-control 控件外高
        constexpr int kManagerInner = 24;   //--std-manager-inner 表头批量钮内径
        constexpr int kRadius = 4;          //--std-radius
        const char* kLineBorder = "#DCE2E9";   //--std-border
        const char* kTextColor2 = "#303313";   //--std-text
        const char* kMuted = "#788391";         //--std-muted
        const char* kSelectedBg = "#E8F3FF";    //--std-selected
        const char* kPrimary = "#1890FF";       //--std-primary
        const char* kDisabledFg = "#A6AFBB";    //--std-disabled

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

        //拾取行辅助钮图标（V1.3 r-pick-selected-icon：16 viewBox 描边 1.4）：查看=圆圈+右尖号；清空=垃圾桶
        QIcon pickAuxIcon(int kind)
        {
            return makeIcon(20, QColor(kTextColor2), [kind](QPainter& p)
                {
                    p.scale(20.0 / 16.0, 20.0 / 16.0);
                    if (kind == 0)
                    {
                        p.drawEllipse(QPointF(8, 8), 6.0, 6.0);
                        p.drawLine(QPointF(6.5, 5), QPointF(9.5, 8));
                        p.drawLine(QPointF(9.5, 8), QPointF(6.5, 11));
                    }
                    else
                    {
                        p.drawLine(QPointF(3, 4.5), QPointF(13, 4.5));
                        p.drawLine(QPointF(6, 4), QPointF(6, 2.5));
                        p.drawLine(QPointF(6, 2.5), QPointF(10, 2.5));
                        p.drawLine(QPointF(10, 2.5), QPointF(10, 4));
                        p.drawLine(QPointF(4, 5), QPointF(4.5, 13));
                        p.drawLine(QPointF(4.5, 13), QPointF(11.5, 13));
                        p.drawLine(QPointF(11.5, 13), QPointF(12, 5));
                        p.drawLine(QPointF(6.5, 6.5), QPointF(6.5, 11));
                        p.drawLine(QPointF(9.5, 6.5), QPointF(9.5, 11));
                    }
                });
        }

        //V1.3 通用拾取入口钮（load-general-pick-entry）：常态 selected 底 #E8F3FF + 常规描边 +
        //左侧 3px 主色竖条（原型 ::before 蓝条 → paintEvent 自绘）；文案左对齐（caption 左缩进
        //12/右让 52 给框内辅助钮）；按下态 focus 描边；禁用整钮灰（同曲线提取面板）
        class PickEntryButton : public QPushButton
        {
        public:
            explicit PickEntryButton(QWidget* parent = nullptr) : QPushButton(parent)
            {
                setCursor(Qt::PointingHandCursor);
                setStyleSheet(
                    QString("QPushButton{background:%1;border:1px solid %2;border-radius:%3px;"
                        "color:%4;text-align:left;padding-left:12px;padding-right:52px;}"
                        "QPushButton:checked{border-color:%5;}"
                        "QPushButton:disabled{background:#F5F7FA;border-color:%2;color:%6;}")
                    .arg(kSelectedBg, kLineBorder).arg(kRadius).arg(kTextColor2, kPrimary, kDisabledFg));
            }

        protected:
            void paintEvent(QPaintEvent* event) override
            {
                QPushButton::paintEvent(event);
                QPainter p(this);
                p.setRenderHint(QPainter::Antialiasing);
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(isEnabled() ? kPrimary : kDisabledFg));
                p.drawRoundedRect(QRectF(4, 5, 3, height() - 10), 1.5, 1.5);
            }
        };

        //V1.3 拾取行：入口钮占满整行，查看/清空两钮绝对定位叠于钮框内右侧
        //（clear right:1 / inspect right:27，26×24，透明底悬停浅蓝）
        class PickEntryRow : public QWidget
        {
        public:
            explicit PickEntryRow(QWidget* parent = nullptr) : QWidget(parent)
            {
                setFixedHeight(kControl);
                entry = new PickEntryButton(this);
                entry->setCheckable(true);
                inspect = new QPushButton(this);
                clear = new QPushButton(this);
                for (QPushButton* btn : { inspect, clear })
                {
                    btn->setFixedSize(kControl, kManagerInner);
                    btn->setCursor(Qt::PointingHandCursor);
                    btn->setStyleSheet(
                        "QPushButton{background:transparent;border:0;border-radius:0;}"
                        "QPushButton:hover{background:#F0F7FF;}"
                        "QPushButton:disabled{background:transparent;}");
                }
                inspect->setIcon(pickAuxIcon(0));
                inspect->setToolTip(QString::fromUtf8("查看已选对象"));
                clear->setIcon(pickAuxIcon(1));
                layoutChildren();
            }

            PickEntryButton* entry = nullptr;
            QPushButton* inspect = nullptr;
            QPushButton* clear = nullptr;

        protected:
            void resizeEvent(QResizeEvent* event) override
            {
                QWidget::resizeEvent(event);
                layoutChildren();
            }

        private:
            void layoutChildren()
            {
                entry->setGeometry(0, 0, width(), kControl);
                inspect->setGeometry(width() - 2 * kControl - 1, 1, kControl, kManagerInner);
                clear->setGeometry(width() - kControl - 1, 1, kControl, kManagerInner);
            }
        };

        //成员主 ID 列标题（对齐前处理集合面板 memberIdColumnTitle）
        QString memberIdColumnTitle(const QString& typeName)
        {
            if (typeName == "节点集") return "节点 ID";
            if (typeName == "单元集" || typeName == "单元面集" || typeName == "单元边集") return "单元 ID";
            if (typeName == "部件集") return "部件 ID";
            return "ID";
        }

        //窄按钮样式（拾取/添加/移除/全选/删除/清空/确定共用，令牌与前处理面板同源）
        QString narrowButtonStyle()
        {
            return QString(
                "QPushButton{ font-family:'Microsoft YaHei'; color:%1; background:%2; border:1px solid %3; border-radius:%4px; padding:%5px %6px; }"
                "QPushButton:hover{ background:%7; }"
                "QPushButton:pressed{ background:%8; color:%9; }"
                "QPushButton:checked{ background:%8; color:%9; }"
                "QPushButton:disabled{ color:%10; background:%11; }")
                .arg(UiPalette::textHex(), UiPalette::surfaceHex(), UiPalette::borderHex())
                .arg(UiMetrics::zoneRadius())
                .arg(UiMetrics::stackGap()).arg(UiMetrics::pushbuttonInner())
                .arg(UiPalette::hoverHex())
                .arg(UiPalette::selectedHex(), UiPalette::selectedTextHex())
                .arg(UiPalette::disabledHex(), UiPalette::panelHex());
        }

        //两字操作按钮宽（规则 M 类 managerActionWidth）：2×body + 2×inset + 2×border
        int twoCharButtonWidth()
        {
            return qMax(MxFunctions::UiMetrics::baseWmin(),
                2 * MxFunctions::UiMetrics::bodyFontSize() + 2 * MxFunctions::UiMetrics::pushbuttonInner() + 2);
        }

        void outputBilingual(int level, const QString& cn, const QString& en, bool popup = false)
        {
            const bool isCn = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
            mGlobalSignals::getInstance()->outputMessageSig(level, isCn ? cn : en, popup);
        }
    }

    mPostSetPanel::mPostSetPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        /********************集合定义（对齐前处理集合面板）********************/
        _defineFold = new FoldSection("集合定义", this);

        //名称
        _labelName = new PanelLabel("名称");
        _lineEditName = new QLineEdit();
        _lineEditName->setFixedHeight(UiMetrics::baseH());
        connect(_lineEditName, &QLineEdit::editingFinished, this, &mPostSetPanel::slot_commitName);
        _defineFold->addRow(_labelName, _lineEditName);

        //集合类型：置灰只读（原型不可修改，由入口按钮类型决定）
        _hintLabel = new PanelLabel("集合类型");
        _labelType = new QLineEdit("节点集");
        _labelType->setFixedHeight(UiMetrics::baseH());
        _labelType->setReadOnly(true);
        _labelType->setEnabled(false);
        _defineFold->addRow(_hintLabel, _labelType);

        //拾取对象（V1.3 通用拾取入口，同曲线提取面板）：全宽入口钮（左侧蓝竖条+动态文案）
        //+ 钮框内右侧 查看(禁用占位)/清空 两小图标钮；下方"自动添加"复选+"添加"按钮行
        _labelPick = new PanelLabel("拾取对象");
        PickEntryRow* pickRow = new PickEntryRow(this);
        _pickEntry = pickRow->entry;
        _pickInspectBtn = pickRow->inspect;
        _pickInspectBtn->setEnabled(false);
        _pickClearBtn = pickRow->clear;
        _pickClearBtn->setToolTip(QString::fromUtf8("清空拾取对象"));
        _pickEntry->setText(QString::fromUtf8("拾取%1").arg(pickNounOfType("节点集")));
        _defineFold->addRow(_labelPick, pickRow);

        //添加行（原型 collection-auto-add-row）：自动添加复选（左）+ 添加按钮（右）
        QWidget* addRow = new QWidget(this);
        auto* addRowLayout = new QHBoxLayout(addRow);
        addRowLayout->setContentsMargins(0, 0, 0, 0);
        _autoAddCheck = new QCheckBox(QString::fromUtf8("自动添加"), addRow);
        _autoAddCheck->setStyleSheet(QString("QCheckBox{ color:%1; border:none; }").arg(kTextColor2));
        _addBtn = new QPushButton(QString::fromUtf8("添加"), addRow);
        _addBtn->setFixedHeight(UiMetrics::baseH());
        _addBtn->setStyleSheet(narrowButtonStyle());
        _addBtn->setFixedWidth(twoCharButtonWidth());
        _addBtn->setEnabled(false);
        _addBtn->setToolTip(QString::fromUtf8("拾取后点击添加，已有集合成员不变"));
        addRowLayout->addWidget(_autoAddCheck);
        addRowLayout->addStretch();
        addRowLayout->addWidget(_addBtn);
        _defineFold->addRow(addRow);

        _mainLayout->addWidget(_defineFold);

        /********************集合数据：成员表格 + 全选/计数/删除********************/
        _dataFold = new FoldSection("集合数据", this);

        _tableMembers = new QTableWidget(this);
        _tableMembers->setObjectName("tableSetMembers");
        _tableMembers->setSelectionBehavior(QAbstractItemView::SelectRows);
        _tableMembers->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _tableMembers->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _tableMembers->verticalHeader()->setVisible(false);
        _tableMembers->verticalHeader()->setDefaultSectionSize(UiMetrics::controlHeight());
        _tableMembers->horizontalHeader()->setStretchLastSection(true);
        _tableMembers->horizontalHeader()->setFixedHeight(UiMetrics::baseH());
        _tableMembers->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
        _tableMembers->setStyleSheet(QString(
            "QTableWidget{ border:1px solid %1; gridline-color:%1; }"
            "QTableWidget::item:selected{ background:%2; color:%3; }"
            "QHeaderView::section{ background:%4; color:%5; border:none; border-bottom:1px solid %1; }")
            .arg(UiPalette::borderHex(), UiPalette::selectedHex(), UiPalette::selectedTextHex())
            .arg(UiPalette::headerHex(), UiPalette::textHex()));
        _dataFold->addRow(_tableMembers);

        _widgetMemberFooter = new QWidget(this);
        auto* memberFooterLayout = new QHBoxLayout(_widgetMemberFooter);
        memberFooterLayout->setContentsMargins(0, 0, 0, 0);
        const QString btnStyle = narrowButtonStyle();
        _btnSelectAllMembers = new QPushButton("全选", _widgetMemberFooter);
        _btnSelectAllMembers->setFixedHeight(UiMetrics::baseH());
        _btnSelectAllMembers->setStyleSheet(btnStyle);
        _btnSelectAllMembers->setFixedWidth(twoCharButtonWidth());
        _labelMemberCount = new QLabel("已选 0 / 共 0", _widgetMemberFooter);
        _btnRemoveMembers = new QPushButton("删除", _widgetMemberFooter);
        _btnRemoveMembers->setFixedHeight(UiMetrics::baseH());
        _btnRemoveMembers->setStyleSheet(btnStyle);
        _btnRemoveMembers->setFixedWidth(twoCharButtonWidth());
        _btnRemoveMembers->setEnabled(false);
        memberFooterLayout->addWidget(_btnSelectAllMembers);
        memberFooterLayout->addWidget(_labelMemberCount);
        memberFooterLayout->addStretch();
        memberFooterLayout->addWidget(_btnRemoveMembers);
        _dataFold->addRow(_widgetMemberFooter);

        _mainLayout->addWidget(_dataFold);

        //原型无底部操作按钮：拾取/删除即实时提交，无确定/清空
        _mainLayout->addStretch();

        //信号
        connect(_tableMembers, &QTableWidget::itemSelectionChanged, this, &mPostSetPanel::slot_memberSelectionChanged);
        connect(_btnSelectAllMembers, &QPushButton::clicked, this, &mPostSetPanel::slot_selectAllMembers);
        connect(_btnRemoveMembers, &QPushButton::clicked, this, &mPostSetPanel::slot_removeMembers);
        connect(_pickEntry, &QPushButton::toggled, this, &mPostSetPanel::slot_pickToggled);
        connect(_pickClearBtn, &QPushButton::clicked, this, &mPostSetPanel::slot_clearPick);
        connect(_autoAddCheck, &QCheckBox::toggled, this, &mPostSetPanel::slot_autoAddToggled);
        connect(_addBtn, &QPushButton::clicked, this, &mPostSetPanel::slot_addMembers);
    }

    mPostSetPanel::~mPostSetPanel()
    {
    }

    void mPostSetPanel::newSetPanel(int caseid, int setType)
    {
        //库定位：caseid>=0 按工况；caseid<0 且面板已有库（树删除后同型重开）复用；
        //caseid<0 且无库（旧"后处理-集合"入口/ui-shot）沿用 getPostDatabase(-1) 默认库语义
        QPointer<mPostDataBase> postDB = (caseid >= 0 || _db == nullptr)
            ? getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid)
            : _db;
        if (postDB == nullptr) return;
        _db = postDB;

        //切换激活后处理视口的渲染器，避免误操作旧模型
        _rWidget = getCVM->getPostRenderWidget();
        updateContextByViewName(QString());

        //类型（-1 为旧"后处理-集合"入口，默认节点集）
        if (setType < 0 || setType > 4)
            setType = 1;
        _typeName = QString::fromUtf8(kSetTypeNames[setType]);
        _members.clear();
        resetPickArea();

        //自动命名（类型_1 起，避开库内已有集合）并立即建空集上树——对齐前处理"立即创建上树"
        QString base = _typeName;
        QString name;
        for (int i = 1; i < 10000; ++i)
        {
            QString candidate = QString("%1_%2").arg(base).arg(i);
            if (!isNameTakenInDatabase(candidate))
            {
                name = candidate;
                break;
            }
        }
        if (name.isEmpty()) return;
        _setName = name;

        _labelType->setText(_typeName);
        refreshPickCaption();
        _lineEditName->setText(name);
        persistMembersToDatabase(_setName, _typeName);
        refreshMemberTable();

        outputBilingual(0,
            QString::fromUtf8("请在视图中拾取对象创建集合：%1").arg(name),
            QString::fromUtf8("Pick objects in the view to create set: %1").arg(name));
    }

    void mPostSetPanel::editSet(const QString& name, const QString& typeName)
    {
        _typeName = typeName;
        _setName = name;
        _members.clear();
        resetPickArea();

        //从库内反装载成员
        _members.clear();
        if (_db != nullptr)
        {
            if (typeName == "节点集")
            {
                for (int id : _db->getNodeSets().value(name)) _members.push_back({ id, -1 });
            }
            else if (typeName == "单元集")
            {
                for (int id : _db->getElementSets().value(name)) _members.push_back({ id, -1 });
            }
            else if (typeName == "部件集")
            {
                for (int id : _db->getPartSets().value(name)) _members.push_back({ id, -1 });
            }
            else if (typeName == "单元面集")
            {
                for (const auto& m : _db->getFaceSets().value(name)) _members.push_back({ m.first, m.second });
            }
            else if (typeName == "单元边集")
            {
                for (const auto& m : _db->getEdgeSets().value(name)) _members.push_back({ m.first, m.second });
            }
        }

        _labelType->setText(_typeName);
        refreshPickCaption();
        _lineEditName->setText(name);
        refreshMemberTable();

        //编辑反显：树单击集合进编辑时高亮其成员（对齐前处理 editSetPanel"反显+存储高亮"；
        //拾取激活中不覆盖拾取缓冲）
        if (!_pickArmed && !_members.empty())
            replayHighlight();

        outputBilingual(0,
            QString::fromUtf8("编辑集合：%1（%2 个成员）").arg(name).arg(_members.size()),
            QString::fromUtf8("Edit set: %1 (%2 members)").arg(name).arg(_members.size()));
    }

    //拾取区复位（新建/编辑集合共用）：失活拾取、清待添加暂存、自动添加复位、caption 回初始
    void mPostSetPanel::resetPickArea()
    {
        if (_pickArmed)
            _pickEntry->setChecked(false);   // 触发 slot_pickToggled 失活清过滤器
        _pending.clear();
        _autoAdd = false;
        _autoAddCheck->blockSignals(true);
        _autoAddCheck->setChecked(false);
        _autoAddCheck->blockSignals(false);
        _addBtn->setEnabled(false);
    }

    //拾取入口动态文案（原型 collection onChange）：
    // 手动模式=拾取缓冲中尚未入表的对象数（"待添加 N 个"）；自动模式=缓冲对象数（"已拾取 N 个"）；
    // 无拾取/无差集=初始"拾取节点/拾取单元…"
    void mPostSetPanel::refreshPickCaption()
    {
        const QString noun = pickNounOfType(_typeName);
        const QString initial = QString::fromUtf8("拾取%1").arg(noun);
        if (!_pickArmed || _rWidget == nullptr)
        {
            _pickEntry->setText(initial);
            _pickEntry->setToolTip(QString::fromUtf8("激活后限定为%1拾取").arg(noun));
            return;
        }

        const auto& picked = _rWidget->getSelectionContainer()->getMeshSelectData()
            ->getSelectDataByType(pickDataTypeOfType(_typeName));
        if (picked.empty())
        {
            _pickEntry->setText(initial);
            return;
        }

        if (_autoAdd)
        {
            _pickEntry->setText(QString::fromUtf8("已拾取 %1 个").arg(picked.size()));
            _pickEntry->setToolTip(QString::fromUtf8("新拾取对象自动加入表格，重复对象不再添加"));
        }
        else
        {
            //待添加 = 缓冲对象中尚未是成员的差集
            std::vector<MemberEntry> entries = resolvePickedEntries(picked);
            int pendingCount = 0;
            for (const auto& e : entries)
            {
                bool exists = false;
                for (const auto& m : _members)
                {
                    if (m.id == e.id && m.sub == e.sub) { exists = true; break; }
                }
                if (!exists) ++pendingCount;
            }
            _pickEntry->setText(pendingCount > 0
                ? QString::fromUtf8("待添加 %1 个").arg(pendingCount)
                : initial);
            _pickEntry->setToolTip(QString::fromUtf8("拾取后点击添加，已有集合成员不变"));
        }
    }

    //兼容旧挂载链（slot_updatePostSetTree→editSet(viewname)）：仅重定位上下文
    void mPostSetPanel::setCurViewName(const QString& viewName)
    {
        updateContextByViewName(viewName);
    }

    void mPostSetPanel::editSetByName(const QString& name)
    {
        const QString typeName = setTypeNameInDatabase(name);
        if (typeName.isEmpty())
        {
            outputBilingual(2,
                QString::fromUtf8("未找到集合：%1").arg(name),
                QString::fromUtf8("Set not found: %1").arg(name), true);
            return;
        }
        editSet(name, typeName);
    }

    void mPostSetPanel::onSetRenamed(const QString& oldName, const QString& newName)
    {
        //正在编辑的集合被树上重命名：当前名与名称框跟随新名（成员不变，库 key 已迁移）
        if (_setName != oldName) return;
        _setName = newName;
        _lineEditName->setText(newName);
    }

    void mPostSetPanel::onSetsDeleted(const QStringList& names)
    {
        //正在编辑的集合被树上删除：同型重开一个新集（自动命名上树），避免后续拾取复活被删集合
        if (_setName.isEmpty() || !names.contains(_setName)) return;

        int typeIndex = 1;
        for (int i = 0; i < 5; ++i)
        {
            if (_typeName == QString::fromUtf8(kSetTypeNames[i])) { typeIndex = i; break; }
        }
        newSetPanel(-1, typeIndex);
    }

    void mPostSetPanel::updateContextByViewName(const QString& viewName)
    {
        // 根据 viewName 重新定位当前应操作的渲染窗口和后处理数据库
        mxr::REBUILD_PostRend* rend = viewName.isEmpty() ? getCVM->getPostRenderWidget()
            : getCVM->getPostRenderByViewName(viewName);
        if (rend == nullptr)
            rend = getCVM->getPostRenderWidget();
        _rWidget = rend;

        if (_rWidget != nullptr)
        {
            QString postName = _rWidget->getPostDataName();
            if (!postName.isEmpty())
            {
                //按视口名重定位库；定位失败保留现有 _db（按钮入口已按工况定位），
                //不置空——否则后续落库静默失败（用户报障：点单元集/节点集不建树节点）
                QPointer<mPostDataBase> db = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
                if (db != nullptr)
                    _db = db;
            }

            //拾取容器数据到达通知（重复连接由 UniqueConnection 防护）
            if (auto* container = _rWidget->getSelectionContainer())
            {
                connect(container, &mxr::SelectionDataContainer::ReadyToReadContainer,
                    this, &mPostSetPanel::slot_pickDataArrived, Qt::UniqueConnection);
            }
        }
    }

    QPointer<mPostModelData> mPostSetPanel::currentModelData()
    {
        if (_db == nullptr) return nullptr;

        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
        if (rebuildPostRend == nullptr) return nullptr;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr) return nullptr;

        return _db->getModelData(guidata->_cloudFrame);
    }

    bool mPostSetPanel::isNameTakenInDatabase(const QString& name, const QString& exceptName) const
    {
        if (_db == nullptr || name == exceptName) return false;
        return _db->getElementSets().contains(name)
            || _db->getNodeSets().contains(name)
            || _db->getFaceSets().contains(name)
            || _db->getEdgeSets().contains(name)
            || _db->getPartSets().contains(name);
    }

    QString mPostSetPanel::setTypeNameInDatabase(const QString& name) const
    {
        if (_db == nullptr) return QString();
        if (_db->getElementSets().contains(name)) return "单元集";
        if (_db->getNodeSets().contains(name)) return "节点集";
        if (_db->getFaceSets().contains(name)) return "单元面集";
        if (_db->getEdgeSets().contains(name)) return "单元边集";
        if (_db->getPartSets().contains(name)) return "部件集";
        return QString();
    }

    std::vector<std::pair<int, int>> mPostSetPanel::resolveFaceSetMembers(const std::vector<mxr::MeshSelectInfoData>& faceData)
    {
        std::vector<std::pair<int, int>> members;
        QPointer<mPostModelData> model = currentModelData();
        if (!model) return members;

        for (const auto& entry : faceData)
        {
            //拾取/生成的面数据：part_id=部件ID，data_index.face_index=渲染面ID（部件面表下标）
            int partId = entry.part_id;
            int surfaceId = entry.data_index.face_index;
            if (partId == INT_MIN || surfaceId < 0) continue;

            QPointer<mPostPartData> part = model->getPartData(partId);
            if (!part || surfaceId >= static_cast<int>(part->_partfacedata.face_elem_id.size())) continue;

            int ownerCid = part->_partfacedata.face_elem_id[surfaceId];
            int localFace = part->_partfacedata.face_owner1_local[surfaceId];
            if (ownerCid < 0 || ownerCid >= static_cast<int>(part->_parteledata.cell_rid.size()) || localFace < 0) continue;

            //库内面集成员口径：(单元真实ID, 0基局部面号)，与 setFaceVisual/face_owner1_local 一致
            members.emplace_back(part->_parteledata.cell_rid[ownerCid], localFace);
        }
        return members;
    }

    std::vector<std::pair<int, int>> mPostSetPanel::resolveEdgeSetMembers(const std::vector<mxr::MeshSelectInfoData>& lineData)
    {
        std::vector<std::pair<int, int>> members;
        QPointer<mPostModelData> model = currentModelData();
        if (!model) return members;

        for (const auto& entry : lineData)
        {
            //边拾取数据为节点对编码：data_id/ data_index=边两端节点真实ID（小/大）
            int lo = entry.data_id.element_id;
            int hi = entry.data_index.line_index;
            if (lo == INT_MIN || hi == INT_MIN || lo == hi) continue;

            //节点对 -> 所属单元：取任一包含该节点对的单元及其局部边号即可
            //（边显隐最终按节点对生效，归属单元不影响结果；多面体无标准边表，跳过）
            QHash<int, QList<int>> candidates = model->getPartEleIDsByNodeIDs({ lo, hi });
            bool found = false;
            for (auto it = candidates.constBegin(); it != candidates.constEnd() && !found; ++it)
            {
                for (int eleId : it.value())
                {
                    int flag = -1;
                    std::vector<int> nodeIds = model->getNodeIdsByEleIdWithoutBuild(it.key(), eleId, flag);
                    if (nodeIds.empty()) continue;

                    PostCelllType cellType = PostData::postElementFunction::MapFunctionTypeToPostCellType(
                        model->getEleTypeByEleId(eleId));
                    const int edgeCount = mPostParseMesh::GetCellEdgeCount(cellType);
                    for (int e = 0; e < edgeCount; ++e)
                    {
                        int l0 = -1, l1 = -1;
                        if (!mPostParseMesh::GetCellEdgeNodes(cellType, e, l0, l1)) continue;
                        if (l0 >= static_cast<int>(nodeIds.size()) || l1 >= static_cast<int>(nodeIds.size())) continue;
                        int n0 = nodeIds[l0], n1 = nodeIds[l1];
                        if ((n0 == lo && n1 == hi) || (n0 == hi && n1 == lo))
                        {
                            //库内边集成员口径：(单元真实ID, 0基局部边号)，与 setEdgeVisual 的边表一致
                            members.emplace_back(eleId, e);
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
            }
        }
        return members;
    }

    std::vector<mxr::MeshSelectInfoData> mPostSetPanel::membersToPickData() const
    {
        std::vector<mxr::MeshSelectInfoData> data;
        if (_typeName == "节点集")
        {
            for (const auto& m : _members)
            {
                mxr::MeshSelectInfoData info;
                info.data_id.node_id = m.id;
                info.data_id.element_id = m.id;
                data.push_back(info);
            }
        }
        else if (_typeName == "单元集")
        {
            QPointer<mPostModelData> model = const_cast<mPostSetPanel*>(this)->currentModelData();
            for (const auto& m : _members)
            {
                mxr::MeshSelectInfoData info;
                info.part_id = model ? model->getPartIDByEleID(m.id) : 0;
                if (info.part_id < 0) info.part_id = 0;
                info.data_id.element_id = m.id;
                info.dim = 3;
                data.push_back(info);
            }
        }
        else if (_typeName == "部件集")
        {
            for (const auto& m : _members)
            {
                mxr::MeshSelectInfoData info;
                info.part_id = m.id;
                data.push_back(info);
            }
        }
        else if (_typeName == "单元面集" || _typeName == "单元边集")
        {
            //面：局部面号反查渲染面ID；边：局部边号展开为节点对
            QPointer<mPostModelData> model = const_cast<mPostSetPanel*>(this)->currentModelData();
            if (!model) return data;
            const bool isFace = (_typeName == "单元面集");
            for (const auto& m : _members)
            {
                int partId = model->getPartIDByEleID(m.id);
                if (partId < 0) continue;
                QPointer<mPostPartData> part = model->getPartData(partId);
                if (!part) continue;

                if (isFace)
                {
                    //面表反查：(cell_rid, face_owner1_local) 命中的面表下标即渲染面ID
                    int surfaceId = -1;
                    const int faceCount = static_cast<int>(part->_partfacedata.face_elem_id.size());
                    for (int idx = 0; idx < faceCount; ++idx)
                    {
                        int cid = part->_partfacedata.face_elem_id[idx];
                        if (cid < 0 || cid >= static_cast<int>(part->_parteledata.cell_rid.size())) continue;
                        if (part->_parteledata.cell_rid[cid] == m.id
                            && part->_partfacedata.face_owner1_local[idx] == m.sub)
                        {
                            surfaceId = idx;
                            break;
                        }
                    }
                    if (surfaceId < 0) continue;
                    mxr::MeshSelectInfoData info;
                    info.part_id = partId;
                    info.data_id.element_id = m.id;
                    info.data_index.face_index = surfaceId;
                    info.dim = 2;
                    data.push_back(info);
                }
                else
                {
                    //边表展开节点对（与 setEdgeVisual 同一边表）
                    auto cidIt = part->_parteledata.cell_rid_to_cid.find(m.id);
                    if (cidIt == part->_parteledata.cell_rid_to_cid.cend()) continue;
                    int flag = -1;
                    std::vector<int> nodeIds = model->getNodeIdsByEleIdWithoutBuild(partId, m.id, flag);
                    if (nodeIds.empty()) continue;
                    PostCelllType cellType = PostData::postElementFunction::MapFunctionTypeToPostCellType(
                        model->getEleTypeByEleId(m.id));
                    int l0 = -1, l1 = -1;
                    if (!mPostParseMesh::GetCellEdgeNodes(cellType, m.sub, l0, l1)) continue;
                    if (l0 >= static_cast<int>(nodeIds.size()) || l1 >= static_cast<int>(nodeIds.size())) continue;
                    int n0 = nodeIds[l0], n1 = nodeIds[l1];
                    mxr::MeshSelectInfoData info;
                    info.part_id = partId;
                    info.data_id.element_id = qMin(n0, n1);
                    info.data_index.line_index = qMax(n0, n1);
                    info.dim = 1;
                    data.push_back(info);
                }
            }
        }
        return data;
    }

    bool mPostSetPanel::persistMembersToDatabase(const QString& name, const QString& typeName)
    {
        if (_db == nullptr || name.isEmpty()) return false;

        if (typeName == "节点集")
        {
            std::vector<int> ids;
            for (const auto& m : _members) ids.push_back(m.id);
            QHash<QString, std::vector<int>> sets = _db->getNodeSets();
            sets.insert(name, std::move(ids));
            _db->setNodeSets(sets);
        }
        else if (typeName == "单元集")
        {
            std::vector<int> ids;
            for (const auto& m : _members) ids.push_back(m.id);
            QHash<QString, std::vector<int>> sets = _db->getElementSets();
            sets.insert(name, std::move(ids));
            _db->setElementSets(sets);
        }
        else if (typeName == "部件集")
        {
            QVector<int> ids;
            for (const auto& m : _members) ids.append(m.id);
            QHash<QString, QVector<int>> sets = _db->getPartSets();
            sets.insert(name, std::move(ids));
            _db->setPartSets(sets);
        }
        else if (typeName == "单元面集")
        {
            std::vector<std::pair<int, int>> members;
            for (const auto& m : _members) members.emplace_back(m.id, m.sub);
            QHash<QString, std::vector<std::pair<int, int>>> sets = _db->getFaceSets();
            sets.insert(name, std::move(members));
            _db->setFaceSets(sets);
        }
        else if (typeName == "单元边集")
        {
            std::vector<std::pair<int, int>> members;
            for (const auto& m : _members) members.emplace_back(m.id, m.sub);
            QHash<QString, std::vector<std::pair<int, int>>> sets = _db->getEdgeSets();
            sets.insert(name, std::move(members));
            _db->setEdgeSets(sets);
        }
        else
        {
            return false;
        }

        //刷新模型树：集合分类子树按库内哈希重建（新建空集也立即上树）
        MechCommon::mMechGlobalSignals::getInstance()->updatePostModelTree();
        return true;
    }

    //拾取容器数据 -> 成员口径（面/边经渲染面ID/节点对换算）
    std::vector<mPostSetPanel::MemberEntry> mPostSetPanel::resolvePickedEntries(const std::vector<mxr::MeshSelectInfoData>& picked)
    {
        std::vector<MemberEntry> incoming;
        if (_typeName == "节点集")
        {
            for (const auto& d : picked) incoming.push_back({ d.data_id.node_id, -1 });
        }
        else if (_typeName == "单元集")
        {
            for (const auto& d : picked) incoming.push_back({ d.data_id.element_id, -1 });
        }
        else if (_typeName == "部件集")
        {
            for (const auto& d : picked)
                if (d.part_id != INT_MIN) incoming.push_back({ d.part_id, -1 });
        }
        else if (_typeName == "单元面集")
        {
            for (const auto& m : resolveFaceSetMembers(picked)) incoming.push_back({ m.first, m.second });
        }
        else
        {
            for (const auto& m : resolveEdgeSetMembers(picked)) incoming.push_back({ m.first, m.second });
        }
        return incoming;
    }

    //并入成员（按 主ID+子号 去重）；返回是否发生变化
    bool mPostSetPanel::mergePendingIntoMembers(const std::vector<MemberEntry>& entries)
    {
        bool changed = false;
        for (const auto& e : entries)
        {
            bool exists = false;
            for (const auto& m : _members)
            {
                if (m.id == e.id && m.sub == e.sub) { exists = true; break; }
            }
            if (!exists) { _members.push_back(e); changed = true; }
        }
        return changed;
    }

    void mPostSetPanel::absorbPickData(bool addOrRemove)
    {
        if (_rWidget == nullptr || _typeName.isEmpty()) return;

        const mxr::SelectionInfoType selType = pickDataTypeOfType(_typeName);
        const auto& picked = _rWidget->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(selType);
        if (picked.empty()) return;

        std::vector<MemberEntry> incoming = resolvePickedEntries(picked);

        //并入/去除（按 主ID+子号 去重）
        bool changed = false;
        if (addOrRemove)
        {
            changed = mergePendingIntoMembers(incoming);
        }
        else
        {
            for (const auto& e : incoming)
            {
                for (auto it = _members.begin(); it != _members.end();)
                {
                    if (it->id == e.id && it->sub == e.sub) { it = _members.erase(it); changed = true; }
                    else ++it;
                }
            }
        }
        if (!changed) return;

        persistMembersToDatabase(_setName, _typeName);
        refreshMemberTable();
        replayHighlight();
    }

    void mPostSetPanel::refreshMemberTable()
    {
        _tableMembers->setSortingEnabled(false);
        //clearContents/setRowCount 不会清除跨列合并（空态行的 setSpan），残留 span 会把
        //首个成员行"序号"格横向吞掉 ID 列——每次重建前先清全部 span
        _tableMembers->clearSpans();

        //列头：序号 + 类型ID + （面集=面号 / 边集=边编号，库内 0 基子号按 1 基展示）
        QStringList headers{ "序号", memberIdColumnTitle(_typeName) };
        const bool isFace = (_typeName == "单元面集");
        const bool isEdge = (_typeName == "单元边集");
        if (isFace) headers << "面号";
        else if (isEdge) headers << "边编号";
        _tableMembers->clearContents();
        _tableMembers->setColumnCount(headers.size());
        _tableMembers->setHorizontalHeaderLabels(headers);
        _tableMembers->setRowCount(static_cast<int>(_members.size()));

        for (int row = 0; row < static_cast<int>(_members.size()); ++row)
        {
            const auto& m = _members[row];
            auto* seqItem = new QTableWidgetItem(QString::number(row + 1));
            seqItem->setFlags(seqItem->flags() & ~Qt::ItemIsEditable);
            seqItem->setTextAlignment(Qt::AlignCenter);
            _tableMembers->setItem(row, 0, seqItem);
            auto* idItem = new QTableWidgetItem(QString::number(m.id));
            idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
            idItem->setTextAlignment(Qt::AlignCenter);
            _tableMembers->setItem(row, 1, idItem);
            if (isFace || isEdge)
            {
                auto* subItem = new QTableWidgetItem(QString::number(m.sub + 1));
                subItem->setFlags(subItem->flags() & ~Qt::ItemIsEditable);
                subItem->setTextAlignment(Qt::AlignCenter);
                _tableMembers->setItem(row, 2, subItem);
            }
        }

        //空态行（原型 collection-empty，文案随模式）：跨列居中、不可选
        if (_members.empty())
        {
            _tableMembers->setRowCount(1);
            auto* emptyItem = new QTableWidgetItem(_autoAdd
                ? QString::fromUtf8("暂无对象，拾取后自动添加。")
                : QString::fromUtf8("暂无对象，请拾取后点击添加。"));
            emptyItem->setFlags(Qt::NoItemFlags);
            emptyItem->setTextAlignment(Qt::AlignCenter);
            emptyItem->setForeground(QColor(kMuted));
            _tableMembers->setItem(0, 0, emptyItem);
            _tableMembers->setSpan(0, 0, 1, headers.size());
        }
        slot_memberSelectionChanged();
    }

    void mPostSetPanel::replayHighlight()
    {
        if (_rWidget == nullptr || _typeName.isEmpty()) return;

        std::vector<mxr::MeshSelectInfoData> data = membersToPickData();
        const mxr::SelectionInfoType selType = pickDataTypeOfType(_typeName);

        _replaying = true;
        auto* meshSelect = _rWidget->getSelectionContainer()->getMeshSelectData();
        meshSelect->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, selType, {});
        if (!data.empty())
            meshSelect->UpdateSelectDataByType(mxr::SelectionOperate::ADD, selType, data);
        MechPreData::UpdatePostWidgetHighlight(_rWidget);
        _rWidget->updatePaint();
        _replaying = false;
    }

    void mPostSetPanel::slot_commitName()
    {
        if (_db == nullptr || _setName.isEmpty()) return;

        const QString next = _lineEditName->text().trimmed();
        //空名或未变化：回退为当前名称（对齐前处理 slot_commitName）
        if (next.isEmpty() || next == _setName)
        {
            _lineEditName->setText(_setName);
            return;
        }
        //重名拒绝并回退
        if (isNameTakenInDatabase(next, _setName))
        {
            outputBilingual(2,
                QString::fromUtf8("集合名称 %1 已存在").arg(next),
                QString::fromUtf8("Set name %1 already exists").arg(next), true);
            _lineEditName->setText(_setName);
            return;
        }

        //库内 key 迁移 + 刷树
        QString oldName = _setName;
        if (_typeName == "节点集")
        {
            auto sets = _db->getNodeSets(); sets.insert(next, sets.value(oldName)); sets.remove(oldName); _db->setNodeSets(sets);
        }
        else if (_typeName == "单元集")
        {
            auto sets = _db->getElementSets(); sets.insert(next, sets.value(oldName)); sets.remove(oldName); _db->setElementSets(sets);
        }
        else if (_typeName == "部件集")
        {
            auto sets = _db->getPartSets(); sets.insert(next, sets.value(oldName)); sets.remove(oldName); _db->setPartSets(sets);
        }
        else if (_typeName == "单元面集")
        {
            auto sets = _db->getFaceSets(); sets.insert(next, sets.value(oldName)); sets.remove(oldName); _db->setFaceSets(sets);
        }
        else
        {
            auto sets = _db->getEdgeSets(); sets.insert(next, sets.value(oldName)); sets.remove(oldName); _db->setEdgeSets(sets);
        }
        _setName = next;
        _lineEditName->setText(next);
        MechCommon::mMechGlobalSignals::getInstance()->updatePostModelTree();
    }

    void mPostSetPanel::slot_memberSelectionChanged()
    {
        //总数按成员数（空态提示行不计入）；空态行 NoItemFlags 不可选，selected 恒 0
        const int total = static_cast<int>(_members.size());
        const int selected = _tableMembers->selectionModel() ? _tableMembers->selectionModel()->selectedRows().count() : 0;
        _labelMemberCount->setText(QString("已选 %1 / 共 %2").arg(selected).arg(total));
        _btnSelectAllMembers->setText(selected > 0 && selected == total ? QString::fromUtf8("取消") : QString::fromUtf8("全选"));
        _btnRemoveMembers->setEnabled(selected > 0);
    }

    void mPostSetPanel::slot_selectAllMembers()
    {
        const int total = static_cast<int>(_members.size());
        const int selected = _tableMembers->selectionModel() ? _tableMembers->selectionModel()->selectedRows().count() : 0;
        _tableMembers->clearSelection();
        if (total > 0 && selected != total)
            _tableMembers->selectAll();
    }

    void mPostSetPanel::slot_removeMembers()
    {
        const QModelIndexList selectedRows = _tableMembers->selectionModel()
            ? _tableMembers->selectionModel()->selectedRows() : QModelIndexList();
        if (selectedRows.isEmpty()) return;

        //按行号倒序移除（表格行序与 _members 序一致）
        std::vector<int> rows;
        for (const QModelIndex& index : selectedRows)
            rows.push_back(index.row());
        std::sort(rows.begin(), rows.end(), [](int a, int b) { return a > b; });
        for (int row : rows)
        {
            if (row >= 0 && row < static_cast<int>(_members.size()))
                _members.erase(_members.begin() + row);
        }

        persistMembersToDatabase(_setName, _typeName);
        refreshMemberTable();
        replayHighlight();
    }

    void mPostSetPanel::slot_pickToggled(bool checked)
    {
        _pickArmed = checked;
        if (_rWidget == nullptr) { _pickEntry->setChecked(false); _pickArmed = false; return; }

        if (checked)
        {
            _rWidget->setPickFilter(pickFilterOfType(_typeName));
        }
        else
        {
            //失活即收尾（对齐前处理通用拾取控件语义）：清当前类型拾取缓冲与高亮，成员保留
            _rWidget->setPickFilter(mxr::PickFilter::PickNothing);
            _pending.clear();
            _addBtn->setEnabled(false);
            if (!_typeName.isEmpty())
            {
                _replaying = true;
                _rWidget->getSelectionContainer()->getMeshSelectData()
                    ->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, pickDataTypeOfType(_typeName), {});
                MechPreData::UpdatePostWidgetHighlight(_rWidget);
                _rWidget->updatePaint();
                _replaying = false;
            }
        }
        refreshPickCaption();
    }

    //清空拾取对象（入口钮框内垃圾桶）：清拾取缓冲与暂存，caption 复位（armed 保持）
    void mPostSetPanel::slot_clearPick()
    {
        if (_rWidget == nullptr || _typeName.isEmpty()) return;
        _replaying = true;
        _rWidget->getSelectionContainer()->getMeshSelectData()
            ->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, pickDataTypeOfType(_typeName), {});
        MechPreData::UpdatePostWidgetHighlight(_rWidget);
        _rWidget->updatePaint();
        _replaying = false;
        _pending.clear();
        _addBtn->setEnabled(false);
        refreshPickCaption();
    }

    //自动添加切换（原型 collection-auto-add）：开启时把暂存待添加立即并入
    void mPostSetPanel::slot_autoAddToggled(bool checked)
    {
        _autoAdd = checked;
        if (checked)
        {
            if (!_pending.empty())
            {
                if (mergePendingIntoMembers(_pending))
                {
                    persistMembersToDatabase(_setName, _typeName);
                    refreshMemberTable();
                }
                _pending.clear();
            }
            //开启后已有拾取缓冲也立即吸收（原型 autoCheck change→addMembers()）
            if (_pickArmed)
                absorbPickData(true);
        }
        _addBtn->setEnabled(false);
        refreshPickCaption();
        refreshMemberTable();   //空态文案随模式切换
    }

    //添加（手动模式）：把拾取缓冲中尚未入表的对象并入成员（原型 collectionAddMembers）
    void mPostSetPanel::slot_addMembers()
    {
        if (_rWidget == nullptr || _typeName.isEmpty() || !_pickArmed) return;

        const auto& picked = _rWidget->getSelectionContainer()->getMeshSelectData()
            ->getSelectDataByType(pickDataTypeOfType(_typeName));
        if (picked.empty()) { _addBtn->setEnabled(false); return; }

        std::vector<MemberEntry> entries = resolvePickedEntries(picked);
        if (mergePendingIntoMembers(entries))
        {
            persistMembersToDatabase(_setName, _typeName);
            refreshMemberTable();
            replayHighlight();
        }
        _pending.clear();
        _addBtn->setEnabled(false);
        refreshPickCaption();
    }

    void mPostSetPanel::slot_pickDataArrived()
    {
        //高亮回放自身写容器也会触发本信号，回放期间忽略
        if (_replaying || !_pickArmed || _typeName.isEmpty()) return;

        if (_autoAdd)
        {
            //自动添加：新拾取对象直接并入成员（重复不加）
            absorbPickData(true);
        }
        else
        {
            //手动模式：暂存待添加，caption/添加钮刷新（并入延迟到点"添加"）
            const auto& picked = _rWidget->getSelectionContainer()->getMeshSelectData()
                ->getSelectDataByType(pickDataTypeOfType(_typeName));
            _pending = resolvePickedEntries(picked);
            _addBtn->setEnabled(!_pending.empty());
        }
        refreshPickCaption();
    }
}
