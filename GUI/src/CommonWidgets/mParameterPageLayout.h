#pragma once

#include <QStackedLayout>
#include <QWidget>

namespace MechMainWindow
{
    class ParameterPageLayout final : public QStackedLayout
    {
    public:
        explicit ParameterPageLayout(QWidget* host)
            : QStackedLayout(host)
        {
            setContentsMargins(0, 0, 0, 0);
            host->setSizePolicy(host->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
            connect(this, &QStackedLayout::currentChanged, this, [this] { invalidate(); });
        }

        QSize sizeHint() const override
        {
            auto hint = QStackedLayout::sizeHint();
            if (const auto* page = itemAt(currentIndex()))
                hint.setHeight(page->sizeHint().height());
            return hint;
        }

        QSize minimumSize() const override
        {
            auto minimum = QStackedLayout::minimumSize();
            if (const auto* page = itemAt(currentIndex()))
                minimum.setHeight(page->minimumSize().height());
            return minimum;
        }

        bool hasHeightForWidth() const override
        {
            const auto* page = itemAt(currentIndex());
            return page && page->hasHeightForWidth();
        }

        int heightForWidth(int width) const override
        {
            const auto* page = itemAt(currentIndex());
            return page ? qMax(page->minimumSize().height(), page->heightForWidth(width)) : 0;
        }
    };
}
