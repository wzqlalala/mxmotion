#pragma once

#include <QColor>
#include <QString>

namespace MxFunctions {

class UiPalette final
{
public:
    // These values are persisted by the original ThemeManager.
    enum class Theme { Light = 0, Dark = 1, EyeCare = 2 };

    static Theme theme() { return s_theme; }
    static void setTheme(Theme theme)
    {
        s_theme = theme == Theme::Dark || theme == Theme::EyeCare ? theme : Theme::Light;
    }

    static QColor text() { return QColor::fromRgb(colors().text); }
    static QColor label() { return QColor::fromRgb(colors().label); }
    static QColor muted() { return QColor::fromRgb(colors().muted); }
    static QColor disabled() { return QColor::fromRgb(colors().disabled); }
    static QColor border() { return QColor::fromRgb(colors().border); }
    static QColor surface() { return QColor::fromRgb(colors().surface); }
    static QColor panel() { return QColor::fromRgb(colors().panel); }
    static QColor header() { return QColor::fromRgb(colors().header); }
    static QColor workspace() { return QColor::fromRgb(colors().workspace); }
    static QColor softwareTitleBar() { return surface(); }
    static QColor primary() { return QColor::fromRgb(colors().primary); }
    static QColor selected() { return QColor::fromRgb(colors().selected); }
    static QColor selectedText() { return QColor::fromRgb(colors().selectedText); }
    static QColor hover() { return QColor::fromRgb(colors().hover); }
    static QColor focus() { return primary(); }
    static QColor error() { return QColor::fromRgb(colors().error); }
    static QColor onAction() { return QColor::fromRgb(colors().onAction); }
    static QColor action() { return primary().darker(115); }

    static QString textHex() { return text().name(); }
    static QString labelHex() { return label().name(); }
    static QString mutedHex() { return muted().name(); }
    static QString disabledHex() { return disabled().name(); }
    static QString borderHex() { return border().name(); }
    static QString surfaceHex() { return surface().name(); }
    static QString panelHex() { return panel().name(); }
    static QString headerHex() { return header().name(); }
    static QString workspaceHex() { return workspace().name(); }
    static QString primaryHex() { return primary().name(); }
    static QString selectedHex() { return selected().name(); }
    static QString selectedTextHex() { return selectedText().name(); }
    static QString hoverHex() { return hover().name(); }
    static QString focusHex() { return focus().name(); }
    static QString errorHex() { return error().name(); }
    static QString onActionHex() { return onAction().name(); }
    static QString actionHex() { return action().name(); }

private:
    struct Colors {
        QRgb text, label, muted, disabled, border;
        QRgb surface, panel, header, workspace;
        QRgb primary, selected, selectedText, hover, error, onAction;
    };

    static const Colors& colors()
    {
        static constexpr Colors palettes[] = {
            // Light (the original Fusion default)
            {0x424b57, 0x5c6674, 0x788391, 0xa6afbb, 0xdce2e9,
             0xffffff, 0xf5f7fa, 0xe9edf3, 0xf0f3f6,
             0x1890ff, 0xe8f3ff, 0x176ba6, 0xf0f7ff, 0xbf3434, 0xffffff},
            // Dark
            {0xe5ebf2, 0xc1cddd, 0xa5b2c3, 0x778294, 0x485465,
             0x252c36, 0x2d3541, 0x313b49, 0x1c222b,
             0x77b9f7, 0x344e6b, 0xa6d4ff, 0x303e50, 0xbf3434, 0xffffff},
            // Eye care
            {0x2d3c2e, 0x485d47, 0x617860, 0x7f8f7a, 0xb1c6a8,
             0xedf4e8, 0xe4efde, 0xdce9d5, 0xd4e2cc,
             0x39774a, 0xd7e8cf, 0x285c38, 0xe1f0dc, 0xb53333, 0xffffff}
        };
        return palettes[static_cast<int>(s_theme)];
    }

    inline static Theme s_theme = Theme::Light;
};

} // namespace MxFunctions
