/*
 * 文件：mPostNotesPanel.cpp
 * 说明：后处理标注面板实现，用于控制后处理三维视口右上角的信息标注。
 */
#include <QCoreApplication>
#include <QElapsedTimer>
#include "mPostNotesPanel.h"

#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPostGUIData.h"
#include "mModelData.h"
#include "mPostDataManager.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPostResultDataStruct.h"

#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
// S5 门面直接替换：会话头已删除（REBUILD_PostRenderSession 转私有宿主）——
// 值类型经 mxr/view/REBUILD_PostRender.h（内含 REBUILD_PostRenderTypes.h）带入
#include "mxr/view/mPostFrameText.h"

#include <QListWidgetItem>
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QDebug>
#include <QCursor>
#include "mFbdColorPicker.h"
#include "mPrimaryButton.h"
#include "mOtherFunction.h"

using namespace MxFunctions;
using namespace MechPreData;
using namespace PostData;

namespace PostWindow
{
    /**
     * @brief 匿名命名空间
     *
     * 定义标注字段的静态配置数据，仅在本翻译单元内可见。
     */
    namespace
    {
        /**
         * @brief 单个标注字段的配置信息
         */
        struct NoteItem
        {
            QString textCN;    ///< 中文显示文本
            QString textEN;    ///< 英文显示文本
            QString key;       ///< 内部键值，用于判断字段类型
            bool defaultChecked; ///< 默认是否勾选
        };

        /**
         * @brief 标注字段默认配置列表
         *
         * 按顺序显示在 QListWidget 中，其中 model/case/frame 默认勾选。
         */
        const QList<NoteItem> kNoteItems = {
            { "模型", "Model", "model", true },
            { "工况", "Case", "case", true },
            { "帧号", "Frame Number", "frame", true },
            { "时间", "Time", "time", false },
            { "文件", "Model File Name", "file", false },
            { "模态频率值", "Modal Frequency value", "fre", false },
            { "模态特征值", "Modal eigenvalus", "igen", false }
        };
    }

    /**
     * @brief 构造函数
     * @param parent 父窗口
     *
     * 完成以下工作：
     * 1. 创建主布局和各折叠块；
     * 2. 创建标注列表、样式设置、确定按钮等控件；
     * 3. 初始化界面状态、构建字段列表、设置翻译文本；
     * 4. 连接所有信号槽，包括按钮点击、字号变化、语言切换等。
     */
    mPostNotesPanel::mPostNotesPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        /********************主布局********************/
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        /********************标注折叠块********************/
        _noteFold = new FoldSection("标注", this);

        // 标注字段列表
        _listWidget = new QListWidget(this);
        _listWidget->setObjectName(QString::fromUtf8("listWidget"));

        // 全选按钮
        _pushButton_select = new QPushButton(this);
        _pushButton_select->setObjectName(QString::fromUtf8("pushButton_select"));
        _pushButton_select->setMinimumSize(QSize(60, 0));

        // 全不选按钮
        _pushButton_deselect = new QPushButton(this);
        _pushButton_deselect->setObjectName(QString::fromUtf8("pushButton_deselect"));
        _pushButton_deselect->setMinimumSize(QSize(60, 0));

        // 将全选/全不选按钮放入水平布局，默认靠右
        QHBoxLayout* selectBtnLayout = new QHBoxLayout();
        selectBtnLayout->addStretch();
        selectBtnLayout->addWidget(_pushButton_select);
        selectBtnLayout->addWidget(_pushButton_deselect);

        _noteFold->addRow(_listWidget);
        _noteFold->addRow(selectBtnLayout);
        _noteFold->refreshLayout();
        _mainLayout->addWidget(_noteFold);
        m_foldWidgets.append(_noteFold);

        /********************样式折叠块********************/
        _styleFold = new FoldSection("样式", this);

        // 字体颜色标签与选择按钮
        _label_color = new PanelLabel("字体颜色");
        _pushButton_color = new QPushButton(this);
        _pushButton_color->setObjectName(QString::fromUtf8("pushButton_color"));
        _pushButton_color->setMinimumSize(QSize(24, 24));
        _pushButton_color->setMaximumSize(QSize(24, 24));
        _pushButton_color->setText(QString(""));
        //色块样式自管（applySwatchColor 按取色实时重发）——挂 ThemeManager 按钮
        //豁免约定（mxBtnNoTheme），防弹窗形态 applyDialogTheme 整表替换冲掉色块底色
        _pushButton_color->setProperty("mxBtnNoTheme", true);
        //色块初始显示默认色（共享 applySwatchColor 口径：底色+浅灰描边+2px 圆角，
        //白底面板上白色块也可见）
        applySwatchColor(_pushButton_color, _color);

        // 字体大小标签与输入框
        _label_fontSize = new PanelLabel("字体大小");
        _label_fontSize->setObjectName(QString::fromUtf8("label_fontSize"));

        _spinBox_fontSize = new QSpinBox(this);
        _spinBox_fontSize->setObjectName(QString::fromUtf8("spinBox_fontSize"));
        _spinBox_fontSize->setRange(8, 72);          // 限制合理字号范围
        _spinBox_fontSize->setValue(_fontSize);    // 默认值 32 像素
        _spinBox_fontSize->setSuffix(" px");

        //行结构对齐云图变量组口径：addRow(标签, 控件) 两列表单行（标签列统一 90px
        //对齐、控件拉满到统一右对齐线）。不再手搓 QHBoxLayout+addStretch——那种
        //行控件只按内容宽，宽弹窗（560px）下右侧大片留白与云图面板不一致
        _styleFold->addRow(_label_color, _pushButton_color);
        //QSpinBox 默认水平策略不随表单 AllNonFixedFieldsGrow 拉伸，显式放开到
        //字段列宽（与云图变量组下拉框同视觉）
        _spinBox_fontSize->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _styleFold->addRow(_label_fontSize, _spinBox_fontSize);
        _styleFold->refreshLayout();
        _mainLayout->addWidget(_styleFold);
        m_foldWidgets.append(_styleFold);

        /********************应用折叠块********************/
        _applyFold = new FoldSection("应用", this);

        //确定=主操作按钮，取共享库 mPrimaryButton（样式统一走 UiMetrics/UiPalette，
        //文本由 retranslateUi 填充）。挂 ThemeManager 按钮豁免约定（mxBtnNoTheme）：
        //本面板会弹成 QDialog（openNotesAsDialog），applyDialogTheme 的整表替换会
        //把主色 QSS 换成通用白底按钮，dock 形态与弹窗形态将两副面孔
        _pushButton_ok = new mPrimaryButton(QString(), this);
        _pushButton_ok->setObjectName(QString::fromUtf8("pushButton_ok"));
        _pushButton_ok->setMinimumSize(QSize(80, 0));
        _pushButton_ok->setProperty("mxBtnNoTheme", true);

        QHBoxLayout* applyBtnLayout = new QHBoxLayout();
        applyBtnLayout->addStretch();
        applyBtnLayout->addWidget(_pushButton_ok);

        _applyFold->addRow(applyBtnLayout);
        _applyFold->refreshLayout();
        _mainLayout->addWidget(_applyFold);
        m_foldWidgets.append(_applyFold);

        /********************结束********************/
        _mainLayout->addStretch();

        /********************初始化与信号连接********************/
        initialUI();
        buildItems(_language);
        retranslateUi();

        // 按钮信号
        connect(_pushButton_select, SIGNAL(clicked()), this, SLOT(selectAll()));
        connect(_pushButton_deselect, SIGNAL(clicked()), this, SLOT(deselectAll()));
        connect(_pushButton_color, SIGNAL(clicked()), this, SLOT(setFontColor()));
        connect(_pushButton_ok, SIGNAL(clicked()), this, SLOT(slot_applyNotes()));

        // 字号变化仅保存值，点击确定后统一应用
        connect(_spinBox_fontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &mPostNotesPanel::setFontSize);

        // 全局语言切换信号
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
        slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

        // 监听云图切帧信号：云图面板手动/自动切帧时同步刷新标注
        connectPostCloudFrameChanged();
    }

    /**
     * @brief 析构函数
     *
     * 断开可能存在的帧变化信号连接，避免对象销毁后仍收到信号。
     */
    mPostNotesPanel::~mPostNotesPanel()
    {
        disconnectFrameChanged();
        disconnectPostCloudFrameChanged();
    }

    /**
     * @brief 初始化/刷新面板
     * @param caseId 当前后处理工况 ID
     *
     * 在面板重新显示时调用（如用户切换工况、导入完成后自动显示），
     * 保存当前工况 ID，重新连接帧变化信号，并立即应用一次标注。
     */
    void mPostNotesPanel::newNotesPanel(int caseId)
    {
        _caseId = caseId;
        disconnectFrameChanged();
        disconnectPostCloudFrameChanged();
        connectFrameChanged();
        connectPostCloudFrameChanged();
        slot_applyNotes();
    }

    /**
     * @brief 响应语言切换
     * @param language 0: 中文, 1: 英文
     *
     * 更新界面文本和字段列表的显示语言。
     */
    void mPostNotesPanel::slot_changeLanguage(int language)
    {
        _language = language;
        retranslateUi();
        buildItems(language);
    }

    /**
     * @brief 初始化界面显示状态
     *
     * 根据当前 _color 设置颜色按钮背景，
     * 并将字号输入框同步为 _fontSize。
     */
    void mPostNotesPanel::initialUI()
    {
        applySwatchColor(_pushButton_color, _color);

        _spinBox_fontSize->setValue(_fontSize);
    }

    /**
     * @brief 刷新界面文本翻译
     *
     * 先调用 Qt 翻译接口设置默认文本，再根据当前 _language 切换中英文。
     */
    void mPostNotesPanel::retranslateUi()
    {
        _noteFold->setTitle(QCoreApplication::translate("mPostNotesPanel", "标注"));
        _styleFold->setTitle(QCoreApplication::translate("mPostNotesPanel", "样式"));
        _applyFold->setTitle(QCoreApplication::translate("mPostNotesPanel", "应用"));
        _pushButton_select->setText(QCoreApplication::translate("mPostNotesPanel", "全选"));
        _pushButton_deselect->setText(QCoreApplication::translate("mPostNotesPanel", "全不选"));

        if (_language == 0)
        {
            _noteFold->setTitle("标注");
            _styleFold->setTitle("样式");
            _applyFold->setTitle("应用");
            _pushButton_select->setText("全选");
            _pushButton_deselect->setText("全不选");
            _label_color->setText("字体颜色");
            _label_fontSize->setText("字体大小");
            _pushButton_ok->setText("确定");
        }
        else if (_language == 1)
        {
            _noteFold->setTitle("Note");
            _styleFold->setTitle("Style");
            _applyFold->setTitle("Apply");
            _pushButton_select->setText("Select all");
            _pushButton_deselect->setText("Deselect all");
            _label_color->setText("Font color");
            _label_fontSize->setText("Font size");
            _pushButton_ok->setText("OK");
        }
    }

    /**
     * @brief 根据语言构建标注字段列表
     * @param language 0: 中文, 1: 英文
     *
     * 清空旧列表，按 kNoteItems 顺序生成 QListWidgetItem，
     * 设置勾选状态、内部键值和可勾选标志。
     */
    void mPostNotesPanel::buildItems(int language)
    {
        _listWidget->clear();
        for (const auto& item : kNoteItems)
        {
            QString displayText = (language == 0) ? item.textCN : item.textEN;
            QListWidgetItem* listItem = new QListWidgetItem(displayText);
            listItem->setCheckState(item.defaultChecked ? Qt::Checked : Qt::Unchecked);
            listItem->setData(Qt::UserRole, item.key);
            listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
            _listWidget->addItem(listItem);
        }
    }

    /**
     * @brief 全选所有标注字段
     */
    void mPostNotesPanel::selectAll()
    {
        for (int i = 0; i < _listWidget->count(); ++i)
        {
            _listWidget->item(i)->setCheckState(Qt::Checked);
        }
    }

    /**
     * @brief 全不选所有标注字段
     */
    void mPostNotesPanel::deselectAll()
    {
        for (int i = 0; i < _listWidget->count(); ++i)
        {
            _listWidget->item(i)->setCheckState(Qt::Unchecked);
        }
    }

    /**
     * @brief 色块按钮换色（共享口径，同 mPostTagPanel::applySwatchColor）
     */
    void mPostNotesPanel::applySwatchColor(QPushButton* swatch, const QColor& color)
    {
        swatch->setStyleSheet(QString(
            "QPushButton{background:%1;border:1px solid #DCE2E9;border-radius:2px;}")
            .arg(color.name()));
    }

    /**
     * @brief 打开字体颜色取色弹窗
     *
     * 使用共享可复用取色控件 FbdColorPicker（V1.3 面板同款：测量/标签/截面力/
     * 剖切均用它），替代原 QColorDialog 系统对话框；取色变化实时回写 _color
     * 并刷新色块，随"确定"统一应用到视口。
     */
    void mPostNotesPanel::setFontColor()
    {
        if (!_colorPicker)
        {
            _colorPicker = new FbdColorPicker(this);
            connect(_colorPicker, &FbdColorPicker::colorChanged, this,
                [this](const QColor& color) {
                    if (!color.isValid())
                        return;
                    _color = color;
                    applySwatchColor(_pushButton_color, _color);
                });
        }
        _colorPicker->setCurrentColor(_color);
        _colorPicker->popupAt(QCursor::pos());
    }

    /**
     * @brief 设置字体大小
     * @param size 字体大小（像素）
     *
     * 保存字号，仅在点击确定时通过 applyToCurrentViewport() 应用。
     */
    void mPostNotesPanel::setFontSize(int size)
    {
        if (size <= 0)
            return;
        _fontSize = size;
    }

    /**
     * @brief 获取当前选中的标注字段项
     * @return 勾选状态的 QListWidgetItem 列表
     */
    QList<QListWidgetItem*> mPostNotesPanel::getSelectedItems() const
    {
        QList<QListWidgetItem*> selected;
        for (int i = 0; i < _listWidget->count(); ++i)
        {
            QListWidgetItem* item = _listWidget->item(i);
            if (item->checkState() == Qt::Checked)
                selected.append(item);
        }
        return selected;
    }

    /**
     * @brief 获取当前后处理渲染对象
     * @return 当前活动视口的后处理渲染对象指针，失败返回 nullptr
     */
    mxr::REBUILD_PostRend* mPostNotesPanel::getCurrentPostRend() const
    {
        mViewportModel* vm = getCVM;
        if (!vm)
            return nullptr;
        return vm->getPostRenderWidget();
    }

    /**
     * @brief 组装标注文本内容
     * @return 每条标注一行字符串列表
     *
     * 根据当前选中的字段键值，从后处理数据库中读取对应信息：
     * - model: 模型名称
     * - case: 工况名
     * - frame: 当前帧名
     * - time: 当前帧时间
     * - file: 结果文件路径
     * - fre: 模态频率（仅频率工况有效）
     * - igen: 模态特征值（仅频率工况有效）
     */
    QStringList mPostNotesPanel::getNoteContents() const
    {
        QStringList contents;
        mxr::REBUILD_PostRend* postRend = getCurrentPostRend();
        if (!postRend)
            return contents;

        auto postRender = postRend->getPostRender();
        if (!postRender)
            return contents;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (!guidata)
            return contents;

        QString postName = postRend->getPostDataName();
        QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postDB == nullptr)
            return contents;

        int fi = guidata->_cloudFrame;
        mPostModelData* modeldata = postDB->getModelData(fi);
        if (modeldata == nullptr)
            return contents;

        QString frameName = postDB->getFrameNameByFrameNum(fi);

        QList<QListWidgetItem*> items = getSelectedItems();
        for (QListWidgetItem* item : items)
        {
            QString key = item->data(Qt::UserRole).toString();
            if (key == "model")
            {
                contents.append(QString("Model:%1  ").arg(postDB->getName()));
            }
            else if (key == "case")
            {
                contents.append(QString("Case:%1  ").arg(postDB->getFrameCase(fi)));
            }
            else if (key == "frame")
            {
                contents.append(QString("Frame:%1  ").arg(frameName));
            }
            else if (key == "time")
            {
                if (!modeldata->_frameInfo._isFrequency)
                    contents.append(QString("Time:%1  ").arg(MxFunctions::mNumberToString(postDB->getFrameTime(fi), 'f', 6)));
                else
                    contents.append(QString("Time: "));
            }
            else if (key == "file")
            {
                contents.append(QString("File:%1  ").arg(postDB->getFile()));
            }
            else if (key == "fre")
            {
                if (modeldata->_frameInfo._isFrequency)
                    contents.append(QString("Frequency:%1 ").arg(MxFunctions::mNumberToString(modeldata->_frameInfo._frequency._modalFreq, 'E', 3)));
                else
                    contents.append(QString("Frequency: "));
            }
            else if (key == "igen")
            {
                if (modeldata->_frameInfo._isFrequency)
                    contents.append(QString("Eigenvalue:%1 ").arg(MxFunctions::mNumberToString(modeldata->_frameInfo._frequency._modalEigen, 'E', 3)));
                else
                    contents.append(QString("Eigenvalue: "));
            }
        }
        return contents;
    }

    /**
     * @brief 将组装好的标注文本应用到当前视口
     *
     * 1. 获取当前后处理渲染对象和 mPostFrameText；
     * 2. 过滤空内容并按行填充到 mPostFrameText；
     * 3. 清除旧的多余行文本；
     * 4. 设置文字颜色、字体大小；
     * 5. 触发视口更新。
     */
    void mPostNotesPanel::applyToCurrentViewport()
    {
        mxr::REBUILD_PostRend* postRend = getCurrentPostRend();
        if (!postRend)
            return;

        auto postRender = postRend->getPostRender();
        if (!postRender)
            return;

        mxr::mPostFrameText* viewtext = postRender->getPostFrameText();
        if (!viewtext)
            return;

        QStringList contents = getNoteContents();

        // 过滤空内容，每项单独一行，避免拼接后过长导致右上角被裁剪
        QStringList lines;
        const int MAX_CHARS = 30;  // 每行最大字符数

        for (const QString& content : contents) {
            QString trimmed = content.trimmed();
            if (trimmed.isEmpty())
                continue;

            // 如果长度在限制内，直接添加
            if (trimmed.length() <= MAX_CHARS) {
                lines.append(trimmed);
            }
            else {
                // 否则循环截取
                int start = 0;
                while (start < trimmed.length()) {
                    QString part = trimmed.mid(start, MAX_CHARS);
                    lines.append(part);
                    start += MAX_CHARS;
                }
            }
        }

        //【R13】内容比对门：标注内容未变时跳过右上角文本实体删建（播放换帧每帧
        // 触发本函数，实测 apply 链 ~4.5ms/帧）
        if (lines == _lastAppliedLines)
            return;
        _lastAppliedLines = lines;

        // 清空之前的显示（保留索引 0，通常用于应用自身标题）
        QHash<int, FontText> oldText = viewtext->getTopRightText();
        QList<int> oldKeys = oldText.keys();
        for (int key : oldKeys)
        {
            if (key > 0)
                viewtext->setTopRightText(key, QString(""));
        }

        // 设置新的多行标注文本
        for (int i = 0; i < lines.size(); ++i)
            viewtext->setTopRightText(i + 1, lines[i]);

        // 设置字体颜色
        QVector3D fontColor;
        fontColor.setX(_color.red() / 255.0);
        fontColor.setY(_color.green() / 255.0);
        fontColor.setZ(_color.blue() / 255.0);
        viewtext->setTopRightTextColor(fontColor);

        // 设置字体大小
        viewtext->setTopRightTextSize(static_cast<float>(_fontSize));

        postRend->updatePaint();
    }

    /**
     * @brief 确定按钮槽
     *
     * 调用 applyToCurrentViewport() 将当前设置应用到视口。
     */
    void mPostNotesPanel::slot_applyNotes()
    {
        applyToCurrentViewport();
    }

    /**
     * @brief 后处理帧变化槽
     * @param frame 当前帧号（未使用，仅用于触发刷新）
     *
     * 当用户播放动画或云图面板切帧时，自动重新计算并显示当前帧的标注信息。
     */
    void mPostNotesPanel::slot_frameChanged(int frame)
    {
        Q_UNUSED(frame)
        // S5 D15 契约：frameChanged 到达=代理数据已绑定且新鲜（D14 纪元绑定前惰性重读）、
        // 渲染路径退化安全（SetColor 负下标拒绝+updateColor 界外键跳过）——GUI 收到即用，
        // 无需延迟防御。旧 singleShot(0) hack 已拆（根因已渲染侧修复）。
        applyToCurrentViewport();
    }

    /**
     * @brief 连接后处理渲染的帧变化信号
     *
     * 先断开旧连接，再获取当前视口的 postRender 对象并建立 frameChanged 信号连接，
     * 使标注能随动画帧自动刷新。
     */
    void mPostNotesPanel::connectFrameChanged()
    {
        disconnectFrameChanged();
        mxr::REBUILD_PostRend* postRend = getCurrentPostRend();
        if (!postRend)
            return;

        auto postRender = postRend->getPostRender();
        if (!postRender)
            return;

        // S5 门面直接替换：帧重估 hook 单通道接门面 frameChanged(frameId, source)
        //（B1b(M10) 语义延续：批注跟随所有帧变化——播放推进/手动切帧均重估）；
        // 会话类转私有宿主后经 getPostRender() 门面订阅，legacy 分支退役。
        // 双拷贝（PostWindow/MainWindow）同步改造——漏接任一份即标注跟随改造漏接
        _frameChangedConn = connect(postRend->getPostRender().get(), &mxr::REBUILD_PostRender::frameChanged,
            this, [this](int frameId, mxr::FrameChangeSource source)
            {
                Q_UNUSED(frameId);
                Q_UNUSED(source);
                applyToCurrentViewport();
            });
    }

    /**
     * @brief 断开帧变化信号连接
     */
    void mPostNotesPanel::disconnectFrameChanged()
    {
        if (_frameChangedConn)
        {
            disconnect(_frameChangedConn);
            _frameChangedConn = QMetaObject::Connection();
        }
    }

    /**
     * @brief 连接云图切帧信号
     *
     * 建立与 MechCommon::mMechGlobalSignals::postCloudFrameChanged 的连接，
     * 使云图面板手动/自动切帧都能驱动标注刷新。
     */
    void mPostNotesPanel::connectPostCloudFrameChanged()
    {
        disconnectPostCloudFrameChanged();
        _postCloudFrameChangedConn = connect(
            MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::postCloudFrameChanged,
            this, &mPostNotesPanel::slot_frameChanged);
    }

    /**
     * @brief 断开云图切帧信号连接
     */
    void mPostNotesPanel::disconnectPostCloudFrameChanged()
    {
        if (_postCloudFrameChangedConn)
        {
            disconnect(_postCloudFrameChangedConn);
            _postCloudFrameChangedConn = QMetaObject::Connection();
        }
    }

}
