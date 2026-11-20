#include "mImageGridComboBox.h"
#include <QGridLayout>
#include <QToolButton>
#include <QButtonGroup>
#include <QMouseEvent>
#include <QApplication>
#include <QIcon>

namespace MechMainWindow
{
    mImageGridComboBox::mImageGridComboBox(QWidget* parent)
        : QPushButton(parent),
        m_popup(nullptr),
        m_gridLayout(nullptr),
        m_buttonGroup(new QButtonGroup(this)),
        m_currentIndex(-1), //初始化为 -1
        m_iconSize(24, 24),
        m_columns(4)
    {
        setText("请选择");
        setIconSize(m_iconSize);
        initPopup();
    }

    mImageGridComboBox::~mImageGridComboBox()
    {
        if (m_popup) {
            m_popup->deleteLater();
        }
    }

    void mImageGridComboBox::initPopup()
    {
        m_popup = new QWidget(this, Qt::Popup | Qt::FramelessWindowHint);
        m_popup->setAttribute(Qt::WA_DeleteOnClose, false);
        m_popup->setStyleSheet(
            "QWidget { background-color: white; border: 1px solid #c0c0c0; }"
            "QToolButton { border: none; background: transparent; }"
            "QToolButton:hover { background-color: #d0e6ff; border: 1px solid #a0c8ff; border-radius: 2px; }"
            "QToolButton:checked { background-color: #a0c8ff; border: 1px solid #6da8ff; border-radius: 2px; }"
        );
        m_gridLayout = new QGridLayout(m_popup);
        m_gridLayout->setSpacing(4);
        m_gridLayout->setContentsMargins(6, 6, 6, 6);
    }

    void mImageGridComboBox::setImages(const QStringList& imagePaths, int columns)
    {
        m_imagePaths = imagePaths;
        m_columns = columns;

        // 清空原有布局
        QLayoutItem* child;
        while ((child = m_gridLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                m_buttonGroup->removeButton(qobject_cast<QToolButton*>(child->widget()));
                child->widget()->deleteLater();
            }
            delete child;
        }

        int row = 0, col = 0;
        for (int i = 0; i < m_imagePaths.size(); ++i) {
            const QString& path = m_imagePaths.at(i);
            QToolButton* item = new QToolButton(m_popup);
            item->setIcon(QIcon(path));
            item->setIconSize(m_iconSize);
            item->setFixedSize(m_iconSize.width() + 8, m_iconSize.height() + 8);
            item->setCheckable(true);
            item->setProperty("imageIndex", i); // 改用索引存储

            m_gridLayout->addWidget(item, row, col);
            m_buttonGroup->addButton(item);

            // 捕获索引而不是路径 
            connect(item, &QToolButton::clicked, this, [this, i]() {
                setCurrentIndex(i);
                m_popup->hide();
                });

            col++;
            if (col >= m_columns) {
                col = 0;
                row++;
            }
        }

        m_popup->adjustSize();

        // 默认选中第一个
        if (m_currentIndex == -1 && !m_imagePaths.isEmpty()) {
            setCurrentIndex(0);
        }
        else {
            updateButtonStyle();
        }
    }

    // 获取当前索引 
    int mImageGridComboBox::currentIndex() const
    {
        return m_currentIndex;
    }

    // 通过索引设置当前选中项 
    void mImageGridComboBox::setCurrentIndex(int index)
    {
        if (index < 0 || index >= m_imagePaths.size()) {
            return; // 越界保护
        }

        if (m_currentIndex == index) return;

        m_currentIndex = index;
        m_currentImagePath = m_imagePaths.at(index); // 同步更新路径

        updateButtonStyle();

        // 发出信号
        emit currentIndexChanged(m_currentIndex);
        emit currentImageChanged(m_currentImagePath);
    }

    QString mImageGridComboBox::currentImage() const
    {
        return m_currentImagePath;
    }

    // --- 修改：通过路径设置时，反推索引 ---
    void mImageGridComboBox::setCurrentImage(const QString& imagePath)
    {
        int index = m_imagePaths.indexOf(imagePath);
        if (index != -1) {
            setCurrentIndex(index);
        }
    }

    QSize mImageGridComboBox::iconSize() const
    {
        return m_iconSize;
    }

    void mImageGridComboBox::setIconSize(const QSize& size)
    {
        m_iconSize = size;
        QPushButton::setIconSize(m_iconSize);

        QList<QAbstractButton*> buttons = m_buttonGroup->buttons();
        for (QAbstractButton* btn : buttons) {
            QToolButton* toolBtn = qobject_cast<QToolButton*>(btn);
            if (toolBtn) {
                toolBtn->setIconSize(m_iconSize);
                toolBtn->setFixedSize(m_iconSize.width() + 8, m_iconSize.height() + 8);
            }
        }
        if (m_popup) {
            m_popup->adjustSize();
        }
    }

    int mImageGridComboBox::columns() const
    {
        return m_columns;
    }

    void mImageGridComboBox::setColumns(int cols)
    {
        m_columns = cols;
        if (!m_imagePaths.isEmpty()) {
            setImages(m_imagePaths, m_columns);
        }
    }

    void mImageGridComboBox::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton) {
            showPopup();
        }
        QPushButton::mousePressEvent(event);
    }

    void mImageGridComboBox::updateButtonStyle()
    {
        // 1. 更新主按钮的样式
        if (m_currentIndex >= 0 && m_currentIndex < m_imagePaths.size()) {
            setText("");
            setIcon(QIcon(m_currentImagePath));
        }
        else {
            setText("请选择");
            setIcon(QIcon());
        }

        // 2. 同步下拉框里的高亮状态（通过索引匹配）
        QList<QAbstractButton*> buttons = m_buttonGroup->buttons();
        for (QAbstractButton* btn : buttons) {
            QToolButton* toolBtn = qobject_cast<QToolButton*>(btn);
            if (toolBtn) {
                bool isChecked = (toolBtn->property("imageIndex").toInt() == m_currentIndex);
                toolBtn->setChecked(isChecked);
            }
        }
    }

    void mImageGridComboBox::showPopup()
    {
        if (!m_popup) return;
        QPoint pos = mapToGlobal(QPoint(0, height()));
        m_popup->move(pos);
        m_popup->show();
    }
}