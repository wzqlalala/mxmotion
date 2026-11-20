#include "mPickWidgetViewBase.h"

#include "UiMetrics.h"

#include <QApplication>
#include <QEvent>
#include <QHelpEvent>
#include <QSize>

namespace MechMainWindow
{
    const QString mPickWidgetViewBase::lineEditBlackStyle =
        "QLineEdit {background: transparent; border: none; font-size: 12px; color: black; }";
    const QString mPickWidgetViewBase::lineEditBlueStyle =
        "QLineEdit { background: transparent; border: none; font-size: 12px; color: #228DF6; }";
    const QString mPickWidgetViewBase::lineEditRedStyle =
        "QLineEdit { background: transparent; border: none; font-size: 12px; color: red; }";

    mPickWidgetViewBase::mPickWidgetViewBase(
        mPickMode pickMode,
        QWidget* parent)
        : QWidget(parent)
        , _pickMode(pickMode)
    {
        initUI();

        // 创建自定义圆角Tooltip
        _roundTooltip = new mRoundTooltip(this);
        _roundTooltip->hide();

        qApp->installEventFilter(this);
    }

    mPickWidgetViewBase::~mPickWidgetViewBase()
    {
        if (_roundTooltip)
        {
            _roundTooltip->hideTooltip();
            _roundTooltip->deleteLater();
            _roundTooltip = nullptr;
        }
    }

    void mPickWidgetViewBase::setRoundTooltipText(const QString& text)
    {
        _roundTooltipText = text;
        if (_roundTooltip)
            _roundTooltip->setText(text);
    }

    QString mPickWidgetViewBase::roundTooltipText() const
    {
        return _roundTooltipText;
    }

    void mPickWidgetViewBase::setPickMode(mPickMode pickMode, bool clearWhenSwitch /*= false*/)
    {
        _pickMode = pickMode;

        // 切换到单选模式时，按规则截断拾取数据
        if (_pickMode == PickOne)
        {
            if (clearWhenSwitch)
            {
                setCurrentPickData({});
            }
            else
            {
                auto items = currentPickItems();
                if (items.size() > 1)
                {
                    std::vector<PickDisplayItem> lastItem;
                    if (!items.empty())
                        lastItem.push_back(items.back());
                    setCurrentPickData(lastItem);
                }
            }
        }
    }

    void mPickWidgetViewBase::setCurrentPickData(const std::vector<PickDisplayItem>& items)
    {
        _pickItems = items;
        refreshDataWidgets();
    }

    void mPickWidgetViewBase::setCurrentData(const std::vector<PickDisplayItem>& items, const QSet<QString>& /*names*/)
    {
        setCurrentPickData(items);
    }

    void mPickWidgetViewBase::setDefultTip(const QString& str, const QString& topStr)
    {
        _defaultTip = str;
        _topTip = topStr;
        updateLineEditText();
    }

    void mPickWidgetViewBase::setPickActive(bool active)
    {
        _isActive = active;
        enableAllWidget(active);
    }

    std::vector<PickDisplayItem> mPickWidgetViewBase::currentPickItems() const
    {
        return _pickItems;
    }

    QColor mPickWidgetViewBase::colorBlockColor() const
    {
        return _currentColor;
    }

    QString mPickWidgetViewBase::colorBlockKey() const
    {
        return _colorStr;
    }

    void mPickWidgetViewBase::setColorBlock(mColorBlock colorBlock)
    {
        _currentColorBlock = colorBlock;
        _colorBlock->show();
        switch (colorBlock)
        {
        case CBPurple:
            _colorBlock->setStyleSheet("background-color: #9130F2; border: none; border-radius: 2px;");
            _colorStr = "CBPurple"; _currentColor = QColor("#9130F2"); break;
        case CBGreen:
            _colorBlock->setStyleSheet("background-color: #17E539; border: none; border-radius: 2px;");
            _colorStr = "CBGreen"; _currentColor = QColor("#17E539"); break;
        case CBOrange:
            _colorBlock->setStyleSheet("background-color: #F27130; border: none; border-radius: 2px;");
            _colorStr = "CBOrange"; _currentColor = QColor("#F27130"); break;
        case CBRed:
            _colorBlock->setStyleSheet("background-color: #FF2C2C; border: none; border-radius: 2px;");
            _colorStr = "CBRed"; _currentColor = QColor("#FF2C2C"); break;
        case CBBlue:
            _colorBlock->setStyleSheet("background-color: #2C4FFF; border: none; border-radius: 2px;");
            _colorStr = "CBBlue"; _currentColor = QColor("#2C4FFF"); break;
        case CBPink:
            _colorBlock->setStyleSheet("background-color: #FF8080; border: none; border-radius: 2px;");
            _colorStr = "CBPink"; _currentColor = QColor("#FF8080"); break;
        case CBSilver:
            _colorBlock->setStyleSheet("background-color: #C0C0C0; border: none; border-radius: 2px;");
            _colorStr = "CBSilver"; _currentColor = QColor("#C0C0C0"); break;
        case CBBrown:
            _colorBlock->setStyleSheet("background-color: #804000; border: none; border-radius: 2px;");
            _colorStr = "CBBrown"; _currentColor = QColor("#804000"); break;
        default:
            _colorBlock->setStyleSheet("background: transparent; border: none;");
            _colorStr = "CBDefault";
            _colorBlock->hide();
            break;
        }
    }

    // ------------------------------------------------------------------
    // UI初始化（修复：初始化走统一刷新入口，确保清空按钮默认隐藏）
    // ------------------------------------------------------------------
    void mPickWidgetViewBase::initUI()
    {
        QHBoxLayout* horizontalLayout = new QHBoxLayout(this);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);

        _frame = new QFrame(this);
        _frame->setContentsMargins(0, 0, 0, 0);
        _frame->setStyleSheet(R"(
            QFrame {
                background-color: #FFFFFF;
                border: 1px solid #000000;
                border-radius: 5px;
            }
            QFrame:hover {
                background-color: #D0E7FD;
            }
        )");
        horizontalLayout->addWidget(_frame);

        this->setFixedHeight(30);
        this->setMouseTracking(true);
        this->setAttribute(Qt::WA_StyledBackground);
        this->setStyleSheet(R"(
            QWidget {
                background-color: white;
                border: 1px solid black;
                border-radius: 5px;
            }
            QWidget * {
                background: transparent;
                border: none;
            }
            QToolTip {
                background-color: #FFFFFF;
                color: #333333;
                border: 1px solid #DDDDDD;
                border-radius: 8px;
                padding: 6px 10px;
            }
        )");

        QHBoxLayout* layout = new QHBoxLayout(_frame);
        layout->setContentsMargins(5, 2, 5, 2);
        layout->setSpacing(5);

        _colorBlock = new QLabel(this);
        _colorBlock->setFixedSize(6, 18);
        _colorBlock->hide();
        layout->addWidget(_colorBlock);

        QFont uiFont(MxFunctions::UiMetrics::uiFontFamily());
        uiFont.setPixelSize(12);
        _lineEdit = new QLineEdit(this);
        _lineEdit->setFont(uiFont);
        _lineEdit->setReadOnly(true);
        _lineEdit->setStyleSheet(lineEditBlueStyle);
        _lineEdit->setFrame(false);
        layout->addWidget(_lineEdit, 1);

        _clearBtn = createIconButton(":/mMainWidgets/Icon/pickIcon/clear.png", "清空");
        _clearBtn->setObjectName("clearBtn");
        connect(_clearBtn, &QToolButton::clicked, this, [this]() {
            emit clearAllRequested();
            if (!_isActive)
            {
                _isActive = true;
                emit activatePick();
            }
            });
        layout->addWidget(_clearBtn);

        // 修复：初始化走统一刷新入口，确保清空按钮按数据状态显隐
        refreshDataWidgets();
    }

    QToolButton* mPickWidgetViewBase::createIconButton(const QString& iconPath, const QString& tipStr)
    {
        QToolButton* btn = new QToolButton(this);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setIcon(QIcon(iconPath));
        btn->setIconSize(QSize(24, 24));
        btn->setProperty("roundTooltipText", tipStr);
        btn->setStyleSheet(R"(
        QToolButton::menu-indicator {
            image: none;
            width: 0px;
        }
        QToolButton{
            background: transparent;
            border: none;
        }
        )");
        btn->installEventFilter(this);
        return btn;
    }

    void mPickWidgetViewBase::setQListWidgetStyle(QListWidget* listWidget)
    {
        listWidget->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        listWidget->setDragDropMode(QAbstractItemView::DragDrop);
        listWidget->setSelectionMode(QAbstractItemView::NoSelection);
        listWidget->setAttribute(Qt::WA_ShowWithoutActivating);
        listWidget->setStyleSheet(R"(
            QListWidget {
                background-color: #FFFFFF;
                border: 1px solid #cccccc;
                border-radius: 6px;
                padding: 4px;
            }
            QListWidget::item { margin: 2px; border-radius: 3px; }
            QListWidget::item:selected { background-color: #cce4ff; }
        )");
    }

    void mPickWidgetViewBase::showPopupBelowWidget(QWidget* anchorWidget, QWidget* popup, const QSize& fixedSize)
    {
        if (!anchorWidget || !popup) return;

        QPoint globalPos = anchorWidget->mapToGlobal(QPoint(0, anchorWidget->height()));
        popup->move(globalPos);
        popup->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        popup->setFixedSize(fixedSize);
        popup->show();
        popup->raise();
    }

    void mPickWidgetViewBase::registerPopup(QWidget* popup)
    {
        if (popup && !_registeredPopups.contains(popup))
            _registeredPopups.append(popup);
    }

    bool mPickWidgetViewBase::eventFilter(QObject* watched, QEvent* event)
    {
        if (event->type() == QEvent::ToolTip)
        {
            auto* helpEvent = static_cast<QHelpEvent*>(event);

            if (isPopupCoveringGlobalPos(helpEvent->globalPos()))
                return true;

            auto* watchedWidget = qobject_cast<QWidget*>(watched);
            const bool ownEvent = watchedWidget &&
                (watchedWidget == this || this->isAncestorOf(watchedWidget));
            if (!ownEvent)
                return false;

            QString tooltipText;
            if (watched == this)
            {
                tooltipText = _roundTooltipText;
            }
            else if (auto btn = qobject_cast<QToolButton*>(watched))
            {
                tooltipText = btn->property("roundTooltipText").toString();
            }
            else
            {
                tooltipText = _roundTooltipText;
            }

            if (!tooltipText.isEmpty() && _roundTooltip && !_roundTooltipVisible)
            {
                _roundTooltipVisible = true;
                _roundTooltip->setText(tooltipText);
                _roundTooltip->showAt(helpEvent->globalPos());
            }
            return true;
        }

        if (event->type() == QEvent::Leave || event->type() == QEvent::MouseButtonPress)
        {
            if (_roundTooltip && _roundTooltipVisible)
            {
                _roundTooltipVisible = false;
                _roundTooltip->hideTooltip();
            }
        }

        if (event->type() == QEvent::MouseButtonPress)
        {
            if (isClickOnOwnPopup(watched))
                return QWidget::eventFilter(watched, event);

            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            hidePopupsOnExternalClick(mouseEvent->globalPos());
        }

        // 仅可见且可用时才允许点击激活：禁用状态（如所在折叠框未勾选）下不可响应任何操作
        // 注意：应用级事件过滤器能收到发送给禁用控件的鼠标事件，必须显式判断 isEnabled
        if (event->type() == QEvent::MouseButtonPress && this->isVisible() && this->isEnabled())
        {
            QWidget* targetWidget = qobject_cast<QWidget*>(watched);
            const bool clickOnSelf =
                targetWidget && (targetWidget == this || this->isAncestorOf(targetWidget));

            if (clickOnSelf)
            {
                if (!_isActive)
                {
                    _isActive = true;
                    emit activatePick();
                }
            }
            else if (_isActive)
            {
                _isActive = false;
            }
        }

        return QWidget::eventFilter(watched, event);
    }

    bool mPickWidgetViewBase::isPopupCoveringGlobalPos(const QPoint& globalPos) const
    {
        for (QWidget* popup : _registeredPopups)
        {
            if (popup->isVisible() && popup->geometry().contains(globalPos))
                return true;
        }
        return false;
    }

    bool mPickWidgetViewBase::isClickOnOwnPopup(QObject* watched) const
    {
        QWidget* targetWidget = qobject_cast<QWidget*>(watched);
        if (!targetWidget) return false;

        for (QWidget* popup : _registeredPopups)
        {
            if (popup->isVisible() && (targetWidget == popup || popup->isAncestorOf(targetWidget)))
                return true;
        }
        return false;
    }

    void mPickWidgetViewBase::hidePopupsOnExternalClick(const QPoint& globalPos)
    {
        for (QWidget* popup : _registeredPopups)
        {
            if (popup->isVisible() && !popup->geometry().contains(globalPos))
                popup->hide();
        }
    }

    bool mPickWidgetViewBase::hasData() const
    {
        return !_pickItems.empty();
    }

    size_t mPickWidgetViewBase::getTotalCount() const
    {
        return _pickItems.size();
    }

    QString mPickWidgetViewBase::getCustomDisplayText() const
    {
        return QString();
    }

    void mPickWidgetViewBase::refreshDataWidgets()
    {
        updateLineEditText();
        updateClearBtnVisible();
    }

    void mPickWidgetViewBase::updateLineEditText()
    {
        const size_t totalCount = getTotalCount();

        QString customText = getCustomDisplayText();
        if (!customText.isEmpty())
        {
            _lineEdit->setStyleSheet(lineEditBlackStyle);
            _lineEdit->setText(customText);
            return;
        }

        if (totalCount == 0)
        {
            _lineEdit->setStyleSheet(lineEditBlueStyle);
            _lineEdit->setText(_defaultTip.isEmpty() ? "选择对象" : _defaultTip);
        }
        else if (_pickMode == PickOne && _pickItems.size() == 1)
        {
            PickDisplayItem data = *_pickItems.begin();
            const QString pickType = PickDataTypeToString(data.type);
            _lineEdit->setStyleSheet(lineEditBlackStyle);
            _lineEdit->setText(QString("已选择%1：%2").arg(pickType).arg(data.data.result_id));
        }
        else
        {
            _lineEdit->setStyleSheet(lineEditBlackStyle);
            _lineEdit->setText(QString("已选择%1个").arg(totalCount));
        }
    }

    void mPickWidgetViewBase::updateClearBtnVisible()
    {
        if (!_clearBtn) return;
        _clearBtn->setVisible(hasData());
    }

    void mPickWidgetViewBase::enableAllWidget(bool flag)
    {
        _lineEdit->setReadOnly(true);
        _clearBtn->setEnabled(true);

        if (flag)
        {
            _frame->setStyleSheet(R"(
                QFrame { background-color: #D0E7FD; border: 1px solid #000000; border-radius: 5px; }
            )");
        }
        else
        {
            _frame->setStyleSheet(R"(
                QFrame { background-color: #FFFFFF; border: 1px solid #000000; border-radius: 5px; }
            )");
        }
    }

    void mPickWidgetViewBase::setPickWidgetEnabled(bool enabled)
    {
        setEnabled(enabled);
        enableAllWidget(enabled);
    }

    bool mPickWidgetViewBase::isMeshCategory(MxFunctions::PickDataType type) const
    {
        return type == MxFunctions::PickDataType::Node ||
            type == MxFunctions::PickDataType::Element ||
            type == MxFunctions::PickDataType::ElementEdge ||
            type == MxFunctions::PickDataType::ElementFace ||
            type == MxFunctions::PickDataType::Part;
    }

    bool mPickWidgetViewBase::isGeoCategory(MxFunctions::PickDataType type) const
    {
        return type == MxFunctions::PickDataType::GeoPoint ||
            type == MxFunctions::PickDataType::GeoEdge ||
            type == MxFunctions::PickDataType::GeoFace ||
            type == MxFunctions::PickDataType::GeoSolid;
    }

    QString mPickWidgetViewBase::buildDisplayText(const PickDisplayItem& item) const
    {
        QString prefix = PickDataTypeToString(item.type);
        if (item.source == PickDataSource::Set)
            return prefix + "集: " + item.sourceName;
        return prefix + ": " + QString::number(item.data.result_id);
    }

    QString mPickWidgetViewBase::buildGroupText(MxFunctions::PickDataType type, size_t count) const
    {
        return QString("%1: 已选 %2 个").arg(PickDataTypeToString(type)).arg(count);
    }
}