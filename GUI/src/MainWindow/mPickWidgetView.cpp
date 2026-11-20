#include "mPickWidgetView.h"
#include "mPickWidgetView.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QSize>

namespace MechMainWindow
{
    // ------------------------------------------------------------------
    // 构造与析构
    // ------------------------------------------------------------------
    mPickWidgetView::mPickWidgetView(
        mColorBlock colorBlock,
        bool isHidenCalcBtn,
        bool isHidenSelectSetBtn,
        bool isHidenPickInfoBtn,
        mPickMode pickMode,
        QWidget* parent)
        : mPickWidgetViewBase(pickMode, parent)
        , _isHidenCalcBtn(isHidenCalcBtn)
        , _isHidenSelectSetBtn(isHidenSelectSetBtn)
        , _isHidenPickInfoBtn(isHidenPickInfoBtn)
    {
        // 基类已完成基础UI+颜色块创建，此处扩展子类特有控件
        initUI();
        setColorBlock(colorBlock);

        // 初始显隐控制
        if (_isHidenCalcBtn) _calcBtn->hide();
        if (_isHidenPickInfoBtn) _pickInfoBtn->hide();
        updateSelectSetBtnVisible();
    }

    mPickWidgetView::~mPickWidgetView() = default;

    // ------------------------------------------------------------------
    // 集合选择按钮控制
    // ------------------------------------------------------------------
    void mPickWidgetView::setSelectSetBtnVisible(bool visible)
    {
        if (!_selectSetBtn) return;
        _selectSetBtn->setVisible(visible);
        // 隐藏按钮时同步收起弹窗
        if (!visible && _selectSetList)
            _selectSetList->setVisible(false);
    }

    bool mPickWidgetView::isSelectSetBtnVisible() const
    {
        return _selectSetBtn ? _selectSetBtn->isVisible() : false;
    }

    void mPickWidgetView::updateSelectSetBtnVisible()
    {
        if (_selectSetBtn)
            _selectSetBtn->setVisible(!_isHidenSelectSetBtn);
    }

    void mPickWidgetView::updateCalcBtnVisible()
    {
        if (_isHidenCalcBtn || _pickMode == PickOne)
        {
            _calcBtn->hide();
            return;
        }
        _calcBtn->setVisible(hasData());
    }

    void mPickWidgetView::updateSelectSetList()
    {
        _selectSetList->blockSignals(true);
        for (int i = 0; i < _selectSetList->count(); ++i)
        {
            QListWidgetItem* item = _selectSetList->item(i);
            item->setCheckState(_selectedSetNames.contains(item->text()) ? Qt::Checked : Qt::Unchecked);
        }
        _selectSetList->blockSignals(false);
    }

    // ------------------------------------------------------------------
    // 拾取模式切换（扩展集合数据处理）
    // ------------------------------------------------------------------
    void mPickWidgetView::setPickMode(mPickMode pickMode, bool clearWhenSwitch /*= false*/)
    {
        // 基类处理拾取数据
        mPickWidgetViewBase::setPickMode(pickMode, clearWhenSwitch);
        updateSelectSetBtnVisible();

        // 切换到单选模式时，同步处理集合数据
        if (_pickMode == PickOne)
        {
            if (clearWhenSwitch)
            {
                _selectedSetNames.clear();
            }
            else
            {
                auto sets = currentSelectSetNames();
                if (sets.size() > 1)
                {
                    QSet<QString> lastSet;
                    if (!sets.empty())
                        lastSet.insert(*sets.begin());
                    _selectedSetNames = lastSet;
                }
            }
            refreshDataWidgets();
        }
    }

    void mPickWidgetView::initUI()
    {
        QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(_frame->layout());
        if (!layout) return;

        _selectSetBtn = createIconButton(":/mMainWidgets/Icon/pickIcon/select.png", "选择集合");
        _selectSetList = new mRoundListWidget(this);
        _selectSetList->setVisible(false);
        setQListWidgetStyle(_selectSetList);
        _selectSetList->setSelectionMode(QAbstractItemView::NoSelection);
        registerPopup(_selectSetList);

        connect(_selectSetBtn, &QToolButton::pressed, this, [this]() {
            showPopupBelowWidget(_selectSetBtn, _selectSetList, QSize(150, 200));
            });

        connect(_selectSetList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
            if (!item) return;
            // 点击整行均可切换：仅当点击位置不在原生勾选框上时才手动切换，
            // 避免与勾选框自身的点击行为重复或冲突。
            const QRect rect = _selectSetList->visualItemRect(item);
            // 估算勾选框位置：QListView 在视图左边距处绘制勾选框，尺寸约 16x16，垂直居中
            QRect checkRect(rect.left() + 2,
                rect.top() + (rect.height() - 16) / 2,
                16, 16);
            const QPoint clickPos = _selectSetList->viewport()->mapFromGlobal(QCursor::pos());
            if (!checkRect.contains(clickPos))
            {
                Qt::CheckState newState = (item->checkState() == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
                item->setCheckState(newState);
            }
            });

        connect(_selectSetList, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
            if (!item) return;
            QString name = item->text();
            if (item->checkState() == Qt::Checked)
            {
                if (_pickMode == PickOne)
                {
                    _selectSetList->blockSignals(true);
                    for (int i = 0; i < _selectSetList->count(); ++i)
                    {
                        QListWidgetItem* other = _selectSetList->item(i);
                        if (other != item)
                            other->setCheckState(Qt::Unchecked);
                    }
                    _selectSetList->blockSignals(false);
                    _selectedSetNames.clear();
                }
                _selectedSetNames.insert(name);
            }
            else
            {
                _selectedSetNames.remove(name);
            }
            emit selectSetChanged(_selectedSetNames);
            refreshDataWidgets();
            });

        layout->insertWidget(2, _selectSetBtn);

        _pickInfoBtn = createIconButton(":/mMainWidgets/Icon/pickIcon/unfold.png", "当前拾取数据");
        _pickInfoBtn->setObjectName("pickInfoBtn");
        _pickInfoList = new mRoundListWidget(this);
        _pickInfoList->setVisible(false);
        setQListWidgetStyle(_pickInfoList);
        registerPopup(_pickInfoList);

        connect(_pickInfoBtn, &QToolButton::pressed, this, [this]() {
            if (_pickItems.empty() && _selectedSetNames.empty()) return;
            // 惰性构建：弹窗默认隐藏，打开时才按当前数据逐项创建控件
            updatePickInfoList();
            showPopupBelowWidget(_pickInfoBtn, _pickInfoList, QSize(250, 200));
            });

        layout->insertWidget(3, _pickInfoBtn);

        _calcBtn = createIconButton(":/mMainWidgets/Icon/pickIcon/result.png", "计算主节点");
        _calcBtn->setObjectName("calcBtn");
        connect(_calcBtn, &QToolButton::clicked, this, &mPickWidgetView::calcMainNodeRequested);
        layout->addWidget(_calcBtn);

        hidenPickInfoBtn();
    }

    void mPickWidgetView::setCurrentData(const std::vector<PickDisplayItem>& items, const QSet<QString>& names)
    {
        _pickItems = items;
        _selectedSetNames = names;
        updateSelectSetList();
        refreshDataWidgets();
    }

    void mPickWidgetView::setSelectSetList(const QHash<QString, QString>& setToType)
    {
        _setToType = setToType;
        _selectSetList->blockSignals(true);
        _selectSetList->clear();
        for (auto it = setToType.begin(); it != setToType.end(); ++it)
        {
            QListWidgetItem* item = new QListWidgetItem(it.key());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            _selectSetList->addItem(item);
        }
        _selectSetList->blockSignals(false);

        updateLineEditText();
    }

    QSet<QString> mPickWidgetView::currentSelectSetNames() const
    {
        return _selectedSetNames;
    }

    void mPickWidgetView::refreshDataWidgets()
    {
        mPickWidgetViewBase::refreshDataWidgets();
        // 拾取信息弹窗默认隐藏：数据变化时不再同步逐项重建弹窗内容
        //（大量拾取时每次刷新创建上千个控件，是进入面板卡顿的主要来源），
        // 改为打开弹窗时按当前数据构建；弹窗处于打开状态时保持实时刷新
        if (_pickInfoList && _pickInfoList->isVisible())
        {
            updatePickInfoList();
        }
        hidenPickInfoBtn();
        updateCalcBtnVisible();
    }

    size_t mPickWidgetView::getTotalCount() const
    {
        return _pickItems.size() + _selectedSetNames.size();
    }

    QString mPickWidgetView::getCustomDisplayText() const
    {
        if (_selectedSetNames.size() == 1 && _pickItems.empty())
        {
            const QString setName = *_selectedSetNames.begin();
            const QString type = _setToType.value(setName, "");
            const QString prefix = type.isEmpty() ? QStringLiteral("已选择集：") : QStringLiteral("已选择%1集：").arg(type);
            const QString elided = _lineEdit->fontMetrics().elidedText(setName, Qt::ElideRight, 120);
            return prefix + elided;
        }
        return QString();
    }

    void mPickWidgetView::enableAllWidget(bool flag)
    {
        mPickWidgetViewBase::enableAllWidget(flag);
        _selectSetBtn->setEnabled(flag);
        _pickInfoBtn->setEnabled(flag);
        _calcBtn->setEnabled(flag);
    }

    bool mPickWidgetView::hasData() const
    {
        return !_pickItems.empty() || !_selectedSetNames.isEmpty();
    }

    void mPickWidgetView::updatePickInfoList()
    {
        _pickInfoList->clear();

        for (const QString& name : _selectedSetNames)
        {
            QString type = _setToType.value(name, "集合");
            PickDisplayItem item(MxFunctions::PickDataType::None, RenderPickResultData(), name, PickDataSource::Set);
            QString text = type + "集: " + name;
            QListWidgetItem* listItem = new QListWidgetItem();
            listItem->setData(Qt::UserRole, QVariant::fromValue(item));
            listItem->setSizeHint(QSize(250, 30));
            _pickInfoList->addItem(listItem);

            DeleteItemWidget* itemWidget = new DeleteItemWidget(text, false, _pickInfoList);
            connect(itemWidget, &DeleteItemWidget::deleteClicked, this, [this, item]() {
                QList<PickDisplayItem> items;
                items.append(item);
                emit removePickRequested(items);
                });
            _pickInfoList->setItemWidget(listItem, itemWidget);
        }

        std::map<MxFunctions::PickDataType, std::vector<PickDisplayItem>> grouped;
        for (const auto& item : _pickItems)
            grouped[item.type].push_back(item);

        for (const auto& pair : grouped)
        {
            MxFunctions::PickDataType type = pair.first;
            size_t count = pair.second.size();

            if (isMeshCategory(type))
            {
                QString text = buildGroupText(type, count);
                QListWidgetItem* listItem = new QListWidgetItem();
                listItem->setData(Qt::UserRole + 1, static_cast<int>(type));
                listItem->setSizeHint(QSize(250, 30));
                _pickInfoList->addItem(listItem);

                DeleteItemWidget* itemWidget = new DeleteItemWidget(text, true, _pickInfoList);
                connect(itemWidget, &DeleteItemWidget::deleteClicked, this, [this, type]() {
                    emit removePickRequestedByType(type);
                    });
                _pickInfoList->setItemWidget(listItem, itemWidget);
            }
            else
            {
                for (const auto& item : pair.second)
                {
                    QString text = buildDisplayText(item);
                    QListWidgetItem* listItem = new QListWidgetItem();
                    listItem->setData(Qt::UserRole, QVariant::fromValue(item));
                    listItem->setSizeHint(QSize(250, 30));
                    _pickInfoList->addItem(listItem);

                    DeleteItemWidget* itemWidget = new DeleteItemWidget(text, false, _pickInfoList);
                    connect(itemWidget, &DeleteItemWidget::deleteClicked, this, [this, item]() {
                        QList<PickDisplayItem> items;
                        items.append(item);
                        emit removePickRequested(items);
                        });
                    _pickInfoList->setItemWidget(listItem, itemWidget);
                }
            }
        }
    }

    void mPickWidgetView::hidenPickInfoBtn()
    {
        if (_isHidenPickInfoBtn || _pickMode == PickOne)
        {
            _pickInfoBtn->hide();
            return;
        }
        _pickInfoBtn->setVisible(hasData());
    }

    DeleteItemWidget::DeleteItemWidget(const QString& text, bool isGroup, QWidget* parent)
        : QWidget(parent)
    {
        initUI(text, isGroup);
    }

    void DeleteItemWidget::initUI(const QString& text, bool isGroup)
    {
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 5, 20, 5);
        layout->setSpacing(10);

        QLabel* label = new QLabel(text, this);
        layout->addWidget(label);

        QPushButton* deleteBtn = new QPushButton(this);
        deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/close3x.png"));
        deleteBtn->setFixedSize(10, 10);
        deleteBtn->setCursor(Qt::PointingHandCursor);
        deleteBtn->setToolTip(isGroup ? "清空该类型全部拾取" : "删除该项");
        deleteBtn->setStyleSheet(
            "QPushButton {"
            "   border: none;"
            "   background: transparent;"
            "   color: #999;"
            "   font-size: 14px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   color: #D0E7FD;"
            "}"
        );
        connect(deleteBtn, &QPushButton::clicked, this, &DeleteItemWidget::deleteClicked);
        layout->addWidget(deleteBtn);
    }
}
