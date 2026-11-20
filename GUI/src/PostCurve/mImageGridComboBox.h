#ifndef IMAGEGRIDCOMBOBOX_H
#define IMAGEGRIDCOMBOBOX_H

#include <QPushButton>
#include <QStringList>
#include <QSize>

class QGridLayout;
class QButtonGroup;

namespace MechMainWindow
{
    class mImageGridComboBox : public QPushButton
    {
        Q_OBJECT
            Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)
            Q_PROPERTY(int columns READ columns WRITE setColumns)

    public:
        explicit mImageGridComboBox(QWidget* parent = nullptr);
        ~mImageGridComboBox();

        void setImages(const QStringList& imagePaths, int columns = 4);

        // --- 新增：通过索引获取和设置 ---
        int currentIndex() const;
        void setCurrentIndex(int index);

        // 保留原有的路径操作方式
        QString currentImage() const;
        void setCurrentImage(const QString& imagePath);

        QSize iconSize() const;
        void setIconSize(const QSize& size);
        int columns() const;
        void setColumns(int cols);

    signals:
        void currentIndexChanged(int index);
        // 原有的带路径信号
        void currentImageChanged(const QString& imagePath);

    protected:
        void mousePressEvent(QMouseEvent* event) override;

    private:
        void initPopup();
        void updateButtonStyle();
        void showPopup();

        QWidget* m_popup;
        QGridLayout* m_gridLayout;
        QButtonGroup* m_buttonGroup;

        QStringList m_imagePaths;

        // 当前索引变量
        int m_currentIndex;

        QString m_currentImagePath;
        QSize m_iconSize;
        int m_columns;
    };
}
#endif // IMAGEGRIDCOMBOBOX_H