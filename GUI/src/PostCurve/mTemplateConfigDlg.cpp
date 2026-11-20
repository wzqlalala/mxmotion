#include "mTemplateConfigDlg.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QButtonGroup>
#include <QFileDialog>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QApplication>
#include <QMouseEvent>
#include <QMainWindow>

#include "mDlgGeometry.h"
#include "mCurveDataSourceStore.h"
#include "mCurveImportCoordinator.h"
#include <memory>
#include "CurveCore/mCurveDocController.h"
#include "ThemeManager.h"   //自样式弹窗豁免标记（kSelfStyledProp）

namespace PostCurve
{
    mTemplateConfigDlg* mTemplateConfigDlg::m_instance = nullptr;

    mTemplateConfigDlg* mTemplateConfigDlg::getInstance(QWidget* parent)
    {
        if (!m_instance)
            m_instance = new mTemplateConfigDlg(parent);
        return m_instance;
    }

    void mTemplateConfigDlg::showInstance(QWidget* parent)
    {
        //父窗口沿父链走到顶层主窗口（同 mOpenCurveDlg::showInstance：调用方传的
        //window() 在停靠/MDI 环境里可能是小面板，直接作父会居中到面板上）
        QWidget* host = parent;
        while (host && host->parentWidget())
            host = host->parentWidget();
        if (!qobject_cast<QMainWindow*>(host))
        {
            const QWidgetList tops = QApplication::topLevelWidgets();
            for (QWidget* w : tops)
            {
                if (qobject_cast<QMainWindow*>(w))
                {
                    host = w;
                    break;
                }
            }
        }
        if (!host)
            host = parent;
        mTemplateConfigDlg* dlg = getInstance(host);
        //本次运行首开居中于父窗口（有会话几何则 bind 已恢复位置，不再居中）
        if (dlg->_needCenter)
        {
            dlg->_needCenter = false;
            if (QWidget* pw = dlg->parentWidget())
                dlg->move(pw->frameGeometry().center()
                    - QPoint(dlg->width() / 2, dlg->height() / 2));
        }
        if (dlg->isMinimized())
            dlg->showNormal();
        dlg->show();
        dlg->raise();
        dlg->activateWindow();
    }

    mTemplateConfigDlg::mTemplateConfigDlg(QWidget* parent)
        //与曲线页面并行操作：置顶避免点击页面后被原生视口覆盖（同 mOpenCurveDlg）
        : QDialog(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        //自样式弹窗：豁免 ThemeManager 整窗重发，保留自带设计稿 QSS
        setProperty(MechMainWindow::ThemeManager::kSelfStyledProp, true);
        resize(640, 300);
        DlgGeom::bind(this, "TemplateConfigDlg");
        _needCenter = !DlgGeom::hasSessionGeometry("TemplateConfigDlg");
        setAutoFillBackground(true);
        //窗口细边框+objectName 选择器（PostCurve 命名空间内 QSS 裸类名匹配不上）
        setObjectName("templateConfigDlg");
        setAttribute(Qt::WA_StyledBackground, true);
        buildUi();
    }

    void mTemplateConfigDlg::buildUi()
    {
        QVBoxLayout* outer = new QVBoxLayout(this);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);

        //标题栏（可拖动，事件过滤在 eventFilter；样式同 mOpenCurveDlg）
        QWidget* titleBar = new QWidget(this);
        titleBar->setObjectName("TemplateConfigTitleBar");
        titleBar->setFixedHeight(52);
        titleBar->setStyleSheet(
            "QWidget#TemplateConfigTitleBar{background:#FFFFFF;border: none;}");
        QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(20, 20, 20, 10);
        titleLayout->setSpacing(8);
        QLabel* logoLabel = new QLabel(titleBar);
        logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
            .scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        QLabel* titleLabel = new QLabel(QStringLiteral("模板配置"), titleBar);
        titleLabel->setStyleSheet(
            "QLabel{color:#333333; font-size:14px; font-weight:600; background:transparent;}");
        QToolButton* closeBtn1 = new QToolButton(titleBar);
        closeBtn1->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
        closeBtn1->setIconSize(QSize(24, 24));
        closeBtn1->setToolTip(QStringLiteral("关闭"));
        closeBtn1->setFixedSize(20, 20);
        closeBtn1->setStyleSheet(R"(
            QToolButton{
                background:transparent; border:none;
            }
            QToolButton:hover{ background:#EFF1F2; border-radius:4px; }
        )");
        connect(closeBtn1, &QToolButton::clicked, this, &QWidget::close);
        titleLayout->addWidget(logoLabel);
        titleLayout->addWidget(titleLabel, 1);
        titleLayout->addWidget(closeBtn1);
        _titleBar = titleBar;
        _titleBar->installEventFilter(this);
        outer->addWidget(titleBar);

        setStyleSheet(R"(
            QWidget#templateConfigDlg{border:1px solid #D9DCE0;}
            QDialog{
                background-color:#FFFFFF;
            }
            QLineEdit{
                background-color:#FFFFFF;
                border:1px solid #D9DCE0;
                border-radius:4px;
                padding:3px 8px;
            }
            QComboBox{
                background-color:#FFFFFF;
                border:1px solid #D9DCE0;
                border-radius:4px;
                padding:3px 8px;
            }
            QRadioButton{
                background:transparent;
                color:#333333;
            }
            QRadioButton:disabled{
                color:#A8ABB2;
            }
            QPushButton#btnCreate{
                background-color:#1385F6;
                color:#FFFFFF;
                border:none;
                border-radius:4px;
                padding:0px;
            }
            QPushButton#btnCreate:hover{
                background-color:#3D9EF8;
            }
            QPushButton#btnCreate:pressed{
                background-color:#0F6ACB;
            }
            QPushButton#btnCreate:disabled{
                background-color:#9CC3E5;
                color:#FFFFFF;
            }
        )");

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setContentsMargins(20, 24, 20, 20);
        mainLayout->setSpacing(18);

        //行 1：导入结果文件（设计稿：弹出对话框选择 pch 格式的结果文件）
        QHBoxLayout* fileRow = new QHBoxLayout();
        fileRow->setSpacing(12);
        QLabel* fileLabel = new QLabel(QStringLiteral("导入结果文件"));
        fileLabel->setFixedWidth(88);
        _fileEdit = new QLineEdit();
        _fileEdit->setPlaceholderText(QStringLiteral("pch 格式的结果文件"));
        fileRow->addWidget(fileLabel);
        fileRow->addWidget(_fileEdit, 1);
        QPushButton* browseBtn = new QPushButton(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/打开文件.svg")), QString());
        browseBtn->setToolTip(QStringLiteral("打开 pch 结果文件"));
        browseBtn->setAutoDefault(false);   //阻止回车键触发按钮
        browseBtn->setFixedSize(24, 24);    //图标按钮统一规格（同"从文件创建曲线"）
        fileRow->addWidget(browseBtn);
        mainLayout->addLayout(fileRow);

        //行 2：模板选择（设计稿：曲线类型下拉，动刚度曲线/VTF曲线/NTF曲线）
        QHBoxLayout* tplRow = new QHBoxLayout();
        tplRow->setSpacing(12);
        QLabel* tplLabel = new QLabel(QStringLiteral("模板选择"));
        tplLabel->setFixedWidth(88);
        _templateCombo = new QComboBox();
        _templateCombo->addItem(QStringLiteral("动刚度曲线"));
        _templateCombo->addItem(QStringLiteral("VTF曲线"));
        _templateCombo->addItem(QStringLiteral("NTF曲线"));
        _templateCombo->setFixedWidth(320);
        tplRow->addWidget(tplLabel);
        tplRow->addWidget(_templateCombo);
        tplRow->addStretch(1);
        mainLayout->addLayout(tplRow);

        //行 3：形式（动刚度曲线=动刚度形式：位移默认；VTF曲线=VTF形式：速度默认；
        //NTF曲线=NTF形式：声压默认/声压+激励点IPI——各组随模板切换显隐）
        QHBoxLayout* formRow = new QHBoxLayout();
        formRow->setSpacing(12);
        _formLabel = new QLabel(QStringLiteral("动刚度形式"));
        _formLabel->setFixedWidth(88);
        _dispRadio = new QRadioButton(QStringLiteral("位移"));
        _velRadio = new QRadioButton(QStringLiteral("速度"));
        _accRadio = new QRadioButton(QStringLiteral("加速度"));
        _spRadio = new QRadioButton(QStringLiteral("声压"));
        _spIpiRadio = new QRadioButton(QStringLiteral("声压+激励点IPI"));
        _dispRadio->setChecked(true);
        _spIpiRadio->hide();   //初始（动刚度）隐藏 NTF 组
        _spRadio->hide();
        _formGroup = new QButtonGroup(this);
        _formGroup->addButton(_dispRadio);
        _formGroup->addButton(_velRadio);
        _formGroup->addButton(_accRadio);
        _formGroup->addButton(_spRadio);
        _formGroup->addButton(_spIpiRadio);
        formRow->addWidget(_formLabel);
        formRow->addWidget(_dispRadio);
        formRow->addSpacing(12);
        formRow->addWidget(_velRadio);
        formRow->addSpacing(12);
        formRow->addWidget(_accRadio);
        formRow->addSpacing(12);
        formRow->addWidget(_spRadio);
        formRow->addSpacing(12);
        formRow->addWidget(_spIpiRadio);
        formRow->addStretch(1);
        mainLayout->addLayout(formRow);

        mainLayout->addStretch(1);

        //底部按钮区（关闭 + 保存；保存未解析到文件前置灰，同设计稿初始态）
        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(12);
        btnLayout->addStretch();
        QPushButton* closeBtn = new QPushButton(QStringLiteral("关闭"));
        _saveBtn = new QPushButton(QStringLiteral("保存"));
        _saveBtn->setObjectName("btnCreate");
        closeBtn->setFixedSize(44, 26);
        _saveBtn->setFixedSize(44, 26);
        _saveBtn->setEnabled(false);
        _saveBtn->setToolTip(QStringLiteral("提取动刚度参数并保存 CSV 模板"));
        btnLayout->addWidget(closeBtn);
        btnLayout->addWidget(_saveBtn);
        mainLayout->addLayout(btnLayout);

        outer->addLayout(mainLayout);

        connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
        connect(browseBtn, &QPushButton::clicked, this, &mTemplateConfigDlg::slot_browseFile);
        connect(_saveBtn, &QPushButton::clicked, this, &mTemplateConfigDlg::slot_save);
        //模板类型切换：形式单选组显隐/默认值联动（VTF 无位移，默认速度）
        connect(_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { applyTemplateKindUi(); });
        applyTemplateKindUi();   //初始态（动刚度曲线，位移默认）
    }

    CurveTemplateKind mTemplateConfigDlg::currentKind() const
    {
        switch (_templateCombo->currentIndex())
        {
        case 1:
            return CurveTemplateKind::Vtf;
        case 2:
            return CurveTemplateKind::Ntf;
        case 0:
        default:
            return CurveTemplateKind::DynStiffness;
        }
    }

    void mTemplateConfigDlg::applyTemplateKindUi()
    {
        const CurveTemplateKind kind = currentKind();
        //三态形式组显隐与默认值（设计稿：动刚度默认位移、VTF 默认速度、
        //NTF 默认声压；VTF 无位移、NTF 只有 声压/声压+激励点IPI）
        _formLabel->setText(kind == CurveTemplateKind::Vtf ? QStringLiteral("VTF形式")
            : kind == CurveTemplateKind::Ntf ? QStringLiteral("NTF形式")
            : QStringLiteral("动刚度形式"));
        const bool dyn = kind == CurveTemplateKind::DynStiffness;
        const bool ntf = kind == CurveTemplateKind::Ntf;
        _dispRadio->setVisible(dyn);
        _velRadio->setVisible(!ntf);
        _accRadio->setVisible(!ntf);
        _spRadio->setVisible(ntf);
        _spIpiRadio->setVisible(ntf);
        if (dyn)
            _dispRadio->setChecked(true);
        else if (kind == CurveTemplateKind::Vtf)
            _velRadio->setChecked(true);
        else
            _spRadio->setChecked(true);
    }

    DynStiffForm mTemplateConfigDlg::currentForm() const
    {
        if (currentKind() == CurveTemplateKind::Ntf)
            return _spIpiRadio->isChecked()
                ? DynStiffForm::SoundPressureIpi : DynStiffForm::SoundPressure;
        if (_velRadio->isChecked())
            return DynStiffForm::Velocity;
        if (_accRadio->isChecked())
            return DynStiffForm::Acceleration;
        return DynStiffForm::Displacement;
    }

    bool mTemplateConfigDlg::eventFilter(QObject* watched, QEvent* event)
    {
        //标题栏拖动（模式同 mOpenCurveDlg：按下记偏移，移动跟随，释放收尾）
        if (watched == _titleBar)
        {
            switch (event->type())
            {
            case QEvent::MouseButtonPress:
            {
                auto* e = static_cast<QMouseEvent*>(event);
                if (e->button() == Qt::LeftButton)
                {
                    _dragging = true;
                    _dragOffset = e->globalPos() - frameGeometry().topLeft();
                    return true;   //消费按下（不改变标题栏子控件的悬停态）
                }
                break;
            }
            case QEvent::MouseMove:
            {
                auto* e = static_cast<QMouseEvent*>(event);
                if (_dragging && (e->buttons() & Qt::LeftButton))
                    move(e->globalPos() - _dragOffset);
                break;
            }
            case QEvent::MouseButtonRelease:
                _dragging = false;
                break;
            default:
                break;
            }
        }
        return QDialog::eventFilter(watched, event);
    }

    void mTemplateConfigDlg::slot_browseFile()
    {
        //起始目录：当前已选文件目录，否则工作目录惯例（同模块内文件对话框）
        QString dir = _fileEdit->text().trimmed();
        dir = dir.isEmpty() ? QString() : QFileInfo(dir).absolutePath();

        const QString filePath = QFileDialog::getOpenFileName(
            this, QStringLiteral("打开文件"), dir, QStringLiteral("PCH File(*.pch)"));
        if (filePath.isEmpty())
            return;

        //解析并登记到共享数据源仓库（动刚度曲线对话框按同一路径键直取，
        //不重复解析）；失败禁用保存并经输出窗口报错。导入异步化（FR-01）：
        //解析成败经 sig_importFinished 续接
        _fileEdit->setText(filePath);
        _pchFilePath.clear();
        _saveBtn->setEnabled(false);

        //Accepted/DuplicateInFlight：解析在后台执行，成败经信号续接。接收上下文
        //=this（C3：解析中可关本对话框——D5，Qt 随对象销毁自动断连防悬垂）；
        //按键匹配防多导入串扰；命中后自断连。
        //协调器须在 addFileAsync 之前就位（否则首个 sig_importStarted 无订阅方）
        mCurveImportCoordinator* coordinator = mCurveImportCoordinator::getInstance(window());
        auto* store = mCurveDataSourceStore::getInstance();
        const auto accepted = store->addFileAsync(filePath);
        if (accepted == mCurveDataSourceStore::AddFileAsyncResult::AlreadyRegistered)
        {
            //已登记幂等成功：走原内联成功链路（逐字保留，C1）
            _pchFilePath = mCurveDataSourceStore::normalizePath(filePath);
            _saveBtn->setEnabled(true);
            return;
        }
        if (accepted == mCurveDataSourceStore::AddFileAsyncResult::DuplicateInFlight)
            coordinator->notifyDuplicateInFlight(filePath);
        const QString key = mCurveDataSourceStore::normalizePath(filePath);
        const auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(store, &mCurveDataSourceStore::sig_importFinished, this,
            [this, key, conn](const QString& finishedKey, bool ok, const QString& err)
            {
                if (finishedKey != key)
                    return;
                QObject::disconnect(*conn);
                if (ok)
                {
                    _pchFilePath = key;   //键已规范化（与原实现一致）
                    _saveBtn->setEnabled(true);
                }
                else
                {
                    //原失败分支（文案/控件行为逐字保留；保存按钮维持禁用）
                    mCurveDocController::getInstance()->reportError(
                        QStringLiteral("[曲线] pch 解析失败：%1").arg(err));
                    _fileEdit->clear();
                }
            });
    }

    void mTemplateConfigDlg::slot_save()
    {
        const PchReader::PchFile* file = mCurveDataSourceStore::getInstance()->file(_pchFilePath);
        if (!file)
            return;
        const CurveTemplateKind kind = currentKind();
        const DynStiffForm form = currentForm();

        //提取该形式全部数据块；为空=设计稿失败分支：模态提示具体原因
        //（动刚度："...没有提取到动刚度位移参数"；VTF："...没有提取到VTF速度参数"；
        //NTF："...没有提取到NTF声压参数"——前缀=模板类型文本去掉"曲线"后缀）
        const QList<DynStiffRow> rows = buildRowsFromPch(*file, kind, form);
        if (rows.isEmpty())
        {
            const QString reasonText = QStringLiteral("%1%2参数")
                .arg(curveTemplateKindText(kind).remove(QStringLiteral("曲线")),
                    dynStiffFormText(form));
            MechMainWindow::mFramelessMessageBox::warning(this, QStringLiteral("提示"),
                QStringLiteral("保存模板失败，导入的结果文件没有提取到%1。"
                    "请检查文件和设置之后，再尝试保存。").arg(reasonText),
                QStringLiteral("知道了"));
            return;
        }

        //默认文件名随模板与形式（动刚度_位移_Mapping.csv / VTF_速度_Mapping.csv /
        //NTF_声压_mapping.csv——NTF 小写 mapping 对齐设计稿），起始目录 = pch 所在目录
        QString defaultName;
        if (kind == CurveTemplateKind::Ntf)
            defaultName = QStringLiteral("NTF_%1_mapping.csv").arg(dynStiffFormText(form));
        else
            defaultName = QStringLiteral("%1_%2_Mapping.csv")
                .arg(kind == CurveTemplateKind::Vtf ? QStringLiteral("VTF") : QStringLiteral("动刚度"),
                    dynStiffFormText(form));
        QString dir = QFileInfo(_pchFilePath).absolutePath();
        const QString filePath = QFileDialog::getSaveFileName(
            this, QStringLiteral("保存文件"),
            dir.isEmpty() ? defaultName : dir + QLatin1Char('/') + defaultName,
            QStringLiteral("CSV File(*.csv)"));
        if (filePath.isEmpty())
            return;

        QString err;
        if (!writeMappingFile(filePath, kind, form, rows, &err))
        {
            MechMainWindow::mFramelessMessageBox::warning(this, QStringLiteral("提示"),
                QStringLiteral("保存模板失败：%1").arg(err), QStringLiteral("知道了"));
            return;
        }
        mCurveDocController::getInstance()->reportInfo(
            QStringLiteral("[曲线] %1%2模板已保存：%3（%4 条通道）")
            .arg(curveTemplateKindText(kind), dynStiffFormText(form))
            .arg(filePath).arg(rows.size()));
    }
}
