#include "mPostAnnotationDialog.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QMouseEvent>
#include "mUiMenu.h"
#include <QPixmap>

namespace PostCommon
{
    mPostAnnotationDialog::mPostAnnotationDialog(QWidget* parent)
        : QDialog(parent, Qt::Window | Qt::FramelessWindowHint)
    {
        //窗口标题（无边框后不显示，留给任务栏等场景）
        setWindowTitle("注释文本");
        resize(480, 468);

        setAutoFillBackground(true);
        //窗口细边框：白底标题栏与内容区同为浅色，加描边保证弹窗轮廓可辨识
        //（照 mApplyStyleDlg 等弹窗）
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(1, 1, 1, 1);
        mainLayout->setSpacing(0);

        //标题栏（可拖动）：白底 + M徽标 + 深灰标题 + 深色关闭按钮（照 mApplyStyleDlg）
        m_titleBar = new QWidget(this);
        m_titleBar->setObjectName("annotationTitleBar");
        m_titleBar->setFixedHeight(52);
        m_titleBar->setStyleSheet(R"(
            QWidget#annotationTitleBar{
                background:#FFFFFF;
            }
        )");
        QHBoxLayout* titleLayout = new QHBoxLayout(m_titleBar);
        titleLayout->setContentsMargins(20, 20, 20, 10);
        titleLayout->setSpacing(3);
        QLabel* logoLabel = new QLabel(m_titleBar);
        logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
            .scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        QLabel* titleLabel = new QLabel(QStringLiteral("注释文本"), m_titleBar);
        titleLabel->setStyleSheet(
            "QLabel{color:#333333; font-size:13px; font-weight:600; background:transparent;}");
        QToolButton* closeBtn = new QToolButton(m_titleBar);
        closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
        closeBtn->setIconSize(QSize(24, 24));
        closeBtn->setToolTip(QStringLiteral("关闭"));
        closeBtn->setStyleSheet(R"(
            QToolButton{
                background:transparent; border:none;
            }
            QToolButton:hover{ background:#EFF1F2; border-radius:2px; }
        )");
        connect(closeBtn, &QToolButton::clicked, this, &QDialog::reject);
        titleLayout->addWidget(logoLabel);
        titleLayout->addWidget(titleLabel, 1);
        titleLayout->addWidget(closeBtn);

        //主体内容区
        QWidget* body = new QWidget(this);
        QVBoxLayout* bodyLayout = new QVBoxLayout(body);
        bodyLayout->setContentsMargins(20, 16, 20, 20);
        bodyLayout->setSpacing(12);

        // 描述与插入字段按钮
        QHBoxLayout* descLayout = new QHBoxLayout();
        descLayout->setContentsMargins(0, 0, 0, 0);
        QLabel* descLabel = new QLabel("描述:", this);

        m_insertBtn = new QPushButton("＋ 插入字段", this);
        m_insertBtn->setObjectName("insertBtn");
        m_insertBtn->setCursor(Qt::PointingHandCursor);
        m_insertBtn->setFixedHeight(24);
        connect(m_insertBtn, &QPushButton::clicked, this, &mPostAnnotationDialog::showInsertMenu);

        descLayout->addWidget(descLabel);
        descLayout->addStretch();
        descLayout->addWidget(m_insertBtn);
        bodyLayout->addLayout(descLayout);

        // 文本输入框
        m_textEdit = new QTextEdit(this);
        m_textEdit->setText("Y={Y} @ X={X}");
        m_textEdit->setObjectName("textEdit");
        bodyLayout->addWidget(m_textEdit);

        // 底部按钮区
        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(12);
        btnLayout->setContentsMargins(0, 18, 0, 0);
        QPushButton* cancelBtn = new QPushButton("取消", this);
        cancelBtn->setObjectName("cancelBtn");
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setFixedSize(44, 26);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

        QPushButton* saveBtn = new QPushButton("保存", this);
        saveBtn->setObjectName("saveBtn");
        saveBtn->setCursor(Qt::PointingHandCursor);
        saveBtn->setFixedSize(44, 26);
        connect(saveBtn, &QPushButton::clicked, this, &mPostAnnotationDialog::saveText);

        btnLayout->addStretch();
        btnLayout->addWidget(cancelBtn);
        btnLayout->addWidget(saveBtn);
        bodyLayout->addLayout(btnLayout);

        mainLayout->addWidget(m_titleBar);
        mainLayout->addWidget(body, 1);

        // 应用 QSS 样式表
        QString qss = R"(
             QDialog, QDialog QWidget {
                background-color: #ffffff;
             }
             QDialog {
                border: 1px solid #D9DCE0;
             }
             QPushButton#saveBtn {
                background-color: #1385F6 ;
                border: none;
                border-radius: 4px;
                color: #ffffff;
            }
            QPushButton#saveBtn:hover {
                background-color: #1a6bd8;
            }
        )";
        this->setStyleSheet(qss);
    }

    void mPostAnnotationDialog::mousePressEvent(QMouseEvent* event)
    {
        //标题栏按下开始拖动（无边框后无系统标题栏，照 mPropertyEditorPopup）
        if (event->button() == Qt::LeftButton && m_titleBar->underMouse())
        {
            m_dragging = true;
            m_dragOffset = event->globalPos() - frameGeometry().topLeft();
        }
        QDialog::mousePressEvent(event);
    }

    void mPostAnnotationDialog::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton))
            move(event->globalPos() - m_dragOffset);
        QDialog::mouseMoveEvent(event);
    }


    void mPostAnnotationDialog::setAnnotationText(const QString& text)
    {
        m_textEdit->setText(text);
    }

    QString mPostAnnotationDialog::annotationText() const
    {
        return m_textEdit->toPlainText();
    }

    void mPostAnnotationDialog::showInsertMenu()
    {
        MechMainWindow::mUiMenu menu(this);
        QStringList fields = { "X", "Y", "Time", "Label", "Slope", "Curv" };

        for (const QString& field : fields) {
            QAction* action = menu.addAction(field);

            connect(action, &QAction::triggered, [this, field]() {
                // 将光标移动到文本末尾
                m_textEdit->moveCursor(QTextCursor::End);
                // 在光标处插入文本
                m_textEdit->insertPlainText("{" + field + "}");
                });
        }

        // 在按钮下方弹出菜单
        QPoint pos = m_insertBtn->mapToGlobal(QPoint(0, m_insertBtn->height()));
        menu.exec(pos);
    }

    void mPostAnnotationDialog::saveText()
    {
        QString content = m_textEdit->toPlainText();
        emit sig_textSaved(content);
        accept();
    }
}