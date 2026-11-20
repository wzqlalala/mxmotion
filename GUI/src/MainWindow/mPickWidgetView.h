#pragma once
#pragma execution_character_set("utf-8")

#include "mPickWidgetViewBase.h"
#include <QSet>
#include <QHash>
#include "mmainwindow_global.h"
#include "mRoundListWidget.h"

namespace MechMainWindow
{
    class DeleteItemWidget;

    class MMAINWINDOW_EXPORT mPickWidgetView : public mPickWidgetViewBase
    {
        Q_OBJECT

    public:
        explicit mPickWidgetView(
            mColorBlock colorBlock = CBNone,
            bool isHidenCalcBtn = true,
            bool isHidenSelectSetBtn = false,
            bool isHidenPickInfoBtn = false,
            mPickMode pickMode = PickMore,
            QWidget* parent = nullptr);
        ~mPickWidgetView() override;

        // 集合相关
        void setCurrentData(const std::vector<PickDisplayItem>& items, const QSet<QString>& names) override;
        void setSelectSetList(const QHash<QString, QString>& setToType = {}) override;
        void setSelectSetBtnVisible(bool visible) override;
        bool isSelectSetBtnVisible() const;
        QSet<QString> currentSelectSetNames() const;

        // 重写基类接口
        void setPickMode(mPickMode pickMode, bool clearWhenSwitch = false) override;

    protected:
        // 重写基类虚函数
        void initUI() override;
        void refreshDataWidgets() override;
        void enableAllWidget(bool flag) override;
        bool hasData() const override;

        size_t getTotalCount() const override;
        QString getCustomDisplayText() const override;

    private:
        void updatePickInfoList();
        void hidenPickInfoBtn();
        void updateSelectSetBtnVisible();
        void updateCalcBtnVisible();
        void updateSelectSetList();

        // 集合选择成员
        QToolButton* _selectSetBtn = nullptr;
        mRoundListWidget* _selectSetList = nullptr;
        QHash<QString, QString> _setToType;
        QSet<QString> _selectedSetNames;
        bool _isHidenSelectSetBtn = false;

        // 拾取详情成员
        QToolButton* _pickInfoBtn = nullptr;
        mRoundListWidget* _pickInfoList = nullptr;
        bool _isHidenPickInfoBtn = false;

        // 计算按钮成员
        QToolButton* _calcBtn = nullptr;
        bool _isHidenCalcBtn = true;
    };

    /**
    * @brief 拾取信息列表项：文字 + 右侧删除按钮（子类独有组件）
    */
    class DeleteItemWidget : public QWidget
    {
        Q_OBJECT
    public:
        DeleteItemWidget(const QString& text, bool isGroup, QWidget* parent = nullptr);

    signals:
        void deleteClicked();

    private:
        void initUI(const QString& text, bool isGroup);
    };
}