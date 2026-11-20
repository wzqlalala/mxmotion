#include "mGenericAssignPanel.h"

//Qt
#include <QLabel>
#include <QDir>
#include <QFileDialog>
#include <QApplication>
#include <QToolTip>
#include <QMessageBox>

//Function
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"

//MechModelData
#include "mGenericAssignData.h"
#include "mGlobalParam.h"
#include "mOtherFunction.h"

using namespace MechGlobalParamData;
using namespace MxFunctions;
namespace MechMainWindow
{
    mGenericAssignPanel::mGenericAssignPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        // ========== 超单元调用设置 ==========
        FoldSection* genericFold = new FoldSection("调用设置", this, QColor("#909399"), true);

        // 调用个数行（使用insertRow）
        QHBoxLayout* countLayout = new QHBoxLayout;
        countLayout->setContentsMargins(0, 0, 0, 0);
        countLayout->setSpacing(2);
        _assignNum = new mCalcLineEdit("1", this, true, true);
        QPushButton* okBtn = new QPushButton("确定", this);
        countLayout->addWidget(_assignNum);
        countLayout->addWidget(okBtn);
        genericFold->insertRow(0, new QLabel("调用超单元个数"), countLayout);

        connect(okBtn, &QPushButton::clicked, this, &mGenericAssignPanel::slot_ok);

        m_foldWidgets.append(genericFold);
        _mainLayout->addWidget(genericFold);

        // 默认添加1个超单元
        addImportRow(0);

        // 刚度矩阵
        _stiffnessCheck = new QCheckBox(this);
        _stiffnessLine = new QLineEdit(this);
        _stiffnessLine->setEnabled(false);
        _stiffnessLine->setAttribute(Qt::WA_AlwaysShowToolTips, true);
        _stiffnessLine->setPlaceholderText("多个名字用英文逗号隔开，数量与超单元个数一致");
        QHBoxLayout* stiffnessLayout = new QHBoxLayout;
        stiffnessLayout->setContentsMargins(0, 0, 0, 0);
        stiffnessLayout->setSpacing(2);
        stiffnessLayout->addWidget(_stiffnessCheck);
        stiffnessLayout->addWidget(_stiffnessLine);
        genericFold->insertRow(genericFold->layout()->rowCount(), new QLabel("刚度矩阵"), stiffnessLayout);

        connect(_stiffnessCheck, &QCheckBox::toggled, this, &mGenericAssignPanel::slot_matrixCheckBoxToggled);
        connect(_stiffnessLine, &QLineEdit::textEdited, this, [this]() {
            if (_bSyncingData) return; //同步数据时忽略
            QString errorMsg;
            int expectedCount = _assignNum->text().toInt();
            if (!validateMatrixNames(_stiffnessLine->text(), expectedCount, errorMsg)) {
                showErrorTip(_stiffnessLine, errorMsg);
            }
            else {
                clearErrorTip(_stiffnessLine);
                syncUIToData();
            }
            });

        // 质量矩阵
        _massCheck = new QCheckBox(this);
        _massLine = new QLineEdit(this);
        _massLine->setEnabled(false);
        _massLine->setAttribute(Qt::WA_AlwaysShowToolTips, true);
        _massLine->setPlaceholderText("多个名字用英文逗号隔开，数量与超单元个数一致");
        QHBoxLayout* massLayout = new QHBoxLayout;
        massLayout->setContentsMargins(0, 0, 0, 0);
        massLayout->setSpacing(2);
        massLayout->addWidget(_massCheck);
        massLayout->addWidget(_massLine);
        genericFold->insertRow(genericFold->layout()->rowCount(), new QLabel("质量矩阵"), massLayout);

        connect(_massCheck, &QCheckBox::toggled, this, &mGenericAssignPanel::slot_matrixCheckBoxToggled);
        connect(_massLine, &QLineEdit::textEdited, this, [this]() {
            if (_bSyncingData) return; //同步数据时忽略
            QString errorMsg;
            int expectedCount = _assignNum->text().toInt();
            if (!validateMatrixNames(_massLine->text(), expectedCount, errorMsg)) {
                showErrorTip(_massLine, errorMsg);
            }
            else {
                clearErrorTip(_massLine);
                syncUIToData();
            }
            });

        // 粘性阻尼矩阵
        _viscousDampingCheck = new QCheckBox(this);
        _viscousDampingLine = new QLineEdit(this);
        _viscousDampingLine->setEnabled(false);
        _viscousDampingLine->setAttribute(Qt::WA_AlwaysShowToolTips, true);
        _viscousDampingLine->setPlaceholderText("多个名字用英文逗号隔开，数量与超单元个数一致");
        QHBoxLayout* dampingLayout = new QHBoxLayout;
        dampingLayout->setContentsMargins(0, 0, 0, 0);
        dampingLayout->setSpacing(2);
        dampingLayout->addWidget(_viscousDampingCheck);
        dampingLayout->addWidget(_viscousDampingLine);
        genericFold->insertRow(genericFold->layout()->rowCount(), new QLabel("粘性阻尼矩阵"), dampingLayout);

        connect(_viscousDampingCheck, &QCheckBox::toggled, this, &mGenericAssignPanel::slot_matrixCheckBoxToggled);
        connect(_viscousDampingLine, &QLineEdit::textEdited, this, [this]() {
            if (_bSyncingData) return; //同步数据时忽略
            QString errorMsg;
            int expectedCount = _assignNum->text().toInt();
            if (!validateMatrixNames(_viscousDampingLine->text(), expectedCount, errorMsg)) {
                showErrorTip(_viscousDampingLine, errorMsg);
            }
            else {
                clearErrorTip(_viscousDampingLine);
                syncUIToData();
            }
            });

        // 材料矩阵
        _materialCheck = new QCheckBox(this);
        _materialLine = new QLineEdit(this);
        _materialLine->setEnabled(false);
        _materialLine->setAttribute(Qt::WA_AlwaysShowToolTips, true);
        _materialLine->setPlaceholderText("多个名字用英文逗号隔开，数量与超单元个数一致");
        QHBoxLayout* materialLayout = new QHBoxLayout;
        materialLayout->setContentsMargins(0, 0, 0, 0);
        materialLayout->setSpacing(2);
        materialLayout->addWidget(_materialCheck);
        materialLayout->addWidget(_materialLine);
        genericFold->insertRow(genericFold->layout()->rowCount(), new QLabel("材料矩阵"), materialLayout);

        connect(_materialCheck, &QCheckBox::toggled, this, &mGenericAssignPanel::slot_matrixCheckBoxToggled);
        connect(_materialLine, &QLineEdit::textEdited, this, [this]() {
            if (_bSyncingData) return; //同步数据时忽略
            QString errorMsg;
            int expectedCount = _assignNum->text().toInt();
            if (!validateMatrixNames(_materialLine->text(), expectedCount, errorMsg)) {
                showErrorTip(_materialLine, errorMsg);
            }
            else {
                clearErrorTip(_materialLine);
                syncUIToData();
            }
            });

        _mainLayout->addStretch();
    }

    mGenericAssignPanel::~mGenericAssignPanel()
    {
    }

    // 显示错误提示（强制在输入框右上角显示）
    void mGenericAssignPanel::showErrorTip(QLineEdit* lineEdit, const QString& errorMsg)
    {
        lineEdit->setToolTip(errorMsg);
        lineEdit->setStyleSheet("border: 1px solid #f56c6c;");

        // 强制在输入框右上角显示提示
        QPoint globalPos = lineEdit->mapToGlobal(QPoint(lineEdit->width(), 0));
        QToolTip::showText(globalPos, errorMsg, lineEdit);
    }

    // 清除错误提示
    void mGenericAssignPanel::clearErrorTip(QLineEdit* lineEdit)
    {
        lineEdit->setToolTip("");
        lineEdit->setStyleSheet("");
        QToolTip::hideText();
    }

    void mGenericAssignPanel::createGenericAssign()
    {
        if (!getCVM) return;
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        if (globalParam->getGenericAssignData())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个超单元调用！"), true);
            return;
        }

        _genericAssign = globalParam->createGenericAssignData();
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建超单元调用"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateGenericAssignSig(true);
    }

    void mGenericAssignPanel::editGenericAssignPanel()
    {
        if (!getCVM) return;
        _genericAssign = getCVM->getModelData()->getGlobalParam()->getGenericAssignData();
        if (_genericAssign == nullptr) return;

        int size = _genericAssign->_filePath.size();
        int currentCount = m_foldWidgets[0]->layout()->rowCount() - 1 - 4; // 减去标题行和4行固定矩阵

        if (currentCount != size)
        {
            _assignNum->setExpression(MxFunctions::mNumberToString(size));
            // 使用差量更新：只调整末尾行数（不触发 slot_ok，避免把空 UI 回写到数据）
            adjustImportRows(size);
        }

        // 同步所有行的路径显示（安全遍历）
        QFormLayout* formLayout = m_foldWidgets[0]->layout();
        for (int i = 1; i <= currentCount; ++i)
        {
            QLayoutItem* fieldItem = formLayout->itemAt(i, QFormLayout::FieldRole);
            if (!fieldItem) continue;

            QHBoxLayout* hbox = qobject_cast<QHBoxLayout*>(fieldItem->layout());
            if (!hbox || hbox->count() == 0) continue;

            QLineEdit* line = qobject_cast<QLineEdit*>(hbox->itemAt(0)->widget());
            if (!line) continue;

            int index = line->objectName().toInt();
            if (index >= 0 && index < _genericAssign->_filePath.size())
            {
                const QString& path = _genericAssign->_filePath[index];

                QFileInfo fi(path);
                QString fileName = fi.fileName();

                line->setText(fileName);
                line->setToolTip(path);
            }
        }

        // 同步矩阵配置数据到UI
        syncDataToUI();
    }

    void mGenericAssignPanel::addImportRow(int index, QString path)
    {
        QLineEdit* line = new QLineEdit;
        line->setText(path);
        line->setToolTip(path);
        QPushButton* pb = new QPushButton("设置", this);
        line->setObjectName(MxFunctions::mNumberToString(index));

        // 创建字段布局（包含输入框和按钮）
        QHBoxLayout* fieldLayout = new QHBoxLayout;
        fieldLayout->setContentsMargins(0, 0, 0, 0);
        fieldLayout->setSpacing(2); // 与原addRow保持一致的间距
        fieldLayout->addWidget(line);
        fieldLayout->addWidget(pb);

        // 关键：在矩阵行之前插入（矩阵是最后4行）
        QFormLayout* formLayout = m_foldWidgets[0]->layout();
        int insertPosition = formLayout->rowCount() - 4;

        // 使用你提供的insertRow重载：insertRow(int row, QWidget* label, QLayout* field)
        m_foldWidgets[0]->insertRow(insertPosition, new QLabel("超单元文件"), fieldLayout);

        connect(pb, &QPushButton::clicked, this, [this, line]() {
            if (!_genericAssign) return;

            QString defaultDir;
            QFileInfo srcInfo(line->toolTip());
            if (srcInfo.exists())
            {
                defaultDir = srcInfo.absolutePath();
            }
            else
            {
                defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
                QFileInfo info(defaultDir);
                if (defaultDir.isEmpty() || !info.exists())
                {
                    defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
                }
            }

            QString filesuffix = R"(All Files(*.pch))";
            QString path = QFileDialog::getOpenFileName(
                this,
                tr("导入超单元文件"),
                defaultDir,
                filesuffix
            );

            if (path.isEmpty())
                return;

            QFileInfo fi(path);
            QString fileName = fi.fileName();

            line->setText(fileName);
            line->setToolTip(path);
            int index = line->objectName().toInt();
            _genericAssign->_filePath[index] = path;
            });
    }

    void mGenericAssignPanel::slot_matrixCheckBoxToggled(bool checked)
    {
        //关键：如果正在同步数据，直接返回，不执行任何操作
        if (_bSyncingData) return;

        QCheckBox* checkBox = qobject_cast<QCheckBox*>(sender());
        if (!checkBox) return;

        // 设置对应输入框的启用状态和提示
        if (checkBox == _stiffnessCheck) {
            _stiffnessLine->setEnabled(checked);
            if (checked && _stiffnessLine->text().trimmed().isEmpty()) {
                showErrorTip(_stiffnessLine, "请输入矩阵名字");
            }
            else if (!checked) {
                clearErrorTip(_stiffnessLine);
            }
        }
        else if (checkBox == _massCheck) {
            _massLine->setEnabled(checked);
            if (checked && _massLine->text().trimmed().isEmpty()) {
                showErrorTip(_massLine, "请输入矩阵名字");
            }
            else if (!checked) {
                clearErrorTip(_massLine);
            }
        }
        else if (checkBox == _viscousDampingCheck) {
            _viscousDampingLine->setEnabled(checked);
            if (checked && _viscousDampingLine->text().trimmed().isEmpty()) {
                showErrorTip(_viscousDampingLine, "请输入矩阵名字");
            }
            else if (!checked) {
                clearErrorTip(_viscousDampingLine);
            }
        }
        else if (checkBox == _materialCheck) {
            _materialLine->setEnabled(checked);
            if (checked && _materialLine->text().trimmed().isEmpty()) {
                showErrorTip(_materialLine, "请输入矩阵名字");
            }
            else if (!checked) {
                clearErrorTip(_materialLine);
            }
        }

        // 同步到数据（只有用户手动操作才会执行）
        syncUIToData();
    }

    void mGenericAssignPanel::syncDataToUI()
    {
        if (!_genericAssign) return;

        //进入数据同步模式，忽略所有UI信号
        _bSyncingData = true;

        int expectedCount = _assignNum->text().toInt();
        QString errorMsg;

        // 刚度矩阵
        _stiffnessCheck->setChecked(_genericAssign->_stiffnessMatrixEnabled);
        _stiffnessLine->setEnabled(_genericAssign->_stiffnessMatrixEnabled);
        _stiffnessLine->setText(_genericAssign->_stiffnessMatrixNames);
        if (_genericAssign->_stiffnessMatrixEnabled) {
            if (!validateMatrixNames(_genericAssign->_stiffnessMatrixNames, expectedCount, errorMsg)) {
                showErrorTip(_stiffnessLine, errorMsg);
            }
            else {
                clearErrorTip(_stiffnessLine);
            }
        }
        else {
            clearErrorTip(_stiffnessLine);
        }

        // 质量矩阵
        _massCheck->setChecked(_genericAssign->_massMatrixEnabled);
        _massLine->setEnabled(_genericAssign->_massMatrixEnabled);
        _massLine->setText(_genericAssign->_massMatrixNames);
        if (_genericAssign->_massMatrixEnabled) {
            if (!validateMatrixNames(_genericAssign->_massMatrixNames, expectedCount, errorMsg)) {
                showErrorTip(_massLine, errorMsg);
            }
            else {
                clearErrorTip(_massLine);
            }
        }
        else {
            clearErrorTip(_massLine);
        }

        // 粘性阻尼矩阵
        _viscousDampingCheck->setChecked(_genericAssign->_viscousDampingMatrixEnabled);
        _viscousDampingLine->setEnabled(_genericAssign->_viscousDampingMatrixEnabled);
        _viscousDampingLine->setText(_genericAssign->_viscousDampingMatrixNames);
        if (_genericAssign->_viscousDampingMatrixEnabled) {
            if (!validateMatrixNames(_genericAssign->_viscousDampingMatrixNames, expectedCount, errorMsg)) {
                showErrorTip(_viscousDampingLine, errorMsg);
            }
            else {
                clearErrorTip(_viscousDampingLine);
            }
        }
        else {
            clearErrorTip(_viscousDampingLine);
        }

        // 材料矩阵
        _materialCheck->setChecked(_genericAssign->_materialMatrixEnabled);
        _materialLine->setEnabled(_genericAssign->_materialMatrixEnabled);
        _materialLine->setText(_genericAssign->_materialMatrixNames);
        if (_genericAssign->_materialMatrixEnabled) {
            if (!validateMatrixNames(_genericAssign->_materialMatrixNames, expectedCount, errorMsg)) {
                showErrorTip(_materialLine, errorMsg);
            }
            else {
                clearErrorTip(_materialLine);
            }
        }
        else {
            clearErrorTip(_materialLine);
        }

        // 🟢 关键：退出数据同步模式，恢复UI信号响应
        _bSyncingData = false;
    }

    void mGenericAssignPanel::syncUIToData()
    {
        if (!_genericAssign) return;

        _genericAssign->_stiffnessMatrixEnabled = _stiffnessCheck->isChecked();
        _genericAssign->_stiffnessMatrixNames = _stiffnessLine->text().trimmed();

        _genericAssign->_massMatrixEnabled = _massCheck->isChecked();
        _genericAssign->_massMatrixNames = _massLine->text().trimmed();

        _genericAssign->_viscousDampingMatrixEnabled = _viscousDampingCheck->isChecked();
        _genericAssign->_viscousDampingMatrixNames = _viscousDampingLine->text().trimmed();

        _genericAssign->_materialMatrixEnabled = _materialCheck->isChecked();
        _genericAssign->_materialMatrixNames = _materialLine->text().trimmed();
    }

    bool mGenericAssignPanel::validateMatrixNames(const QString& text, int expectedCount, QString& errorMsg)
    {
        if (text.isEmpty()) {
            errorMsg = "请输入矩阵名字";
            return false;
        }

        // 同时检查所有常见的中文分隔符
        if (text.contains("，") || text.contains("、") || text.contains("；") || text.contains("；")) {
            errorMsg = "请使用英文逗号(,)分隔多个矩阵名字";
            return false;
        }

        // 检查连续逗号、首尾逗号
        if (text.contains(",,") || text.startsWith(',') || text.endsWith(',')) {
            errorMsg = "格式错误：不能有连续逗号或首尾逗号";
            return false;
        }

        // 检查名字数量是否正确
        QStringList names = text.split(',', QString::SkipEmptyParts);
        if (names.size() != expectedCount) {
            errorMsg = QString("数量错误：应为%1个，当前输入%2个").arg(expectedCount).arg(names.size());
            return false;
        }

        // 完全放开名字字符限制
        return true;
    }


    void mGenericAssignPanel::adjustImportRows(int number)
    {
        int rowCount = m_foldWidgets[0]->layout()->rowCount() - 4; // 减去4行固定矩阵
        int currentCount = rowCount - 1; // 减去标题行

        // 增加超单元
        if (number > currentCount) {
            for (int i = currentCount; i < number; ++i) {
                addImportRow(i); // 现在会自动插入到矩阵上方
            }

            if (_genericAssign) {
                _genericAssign->_filePath.resize(number);
            }
        }
        // 减少超单元
        else if (number < currentCount) {
            while (m_foldWidgets[0]->layout()->rowCount() > number + 1 + 4) { // 标题行+文件行+4行矩阵
                // 删除最后一个文件行（倒数第5行，因为最后4行是矩阵）
                int deleteRow = m_foldWidgets[0]->layout()->rowCount() - 5;
                m_foldWidgets[0]->removeRow(deleteRow);
            }

            if (_genericAssign) {
                _genericAssign->_filePath.resize(number);
            }
        }
        else {
            return;
        }

        // 刷新布局
        QTimer::singleShot(0, m_foldWidgets[0], [=]() {
            m_foldWidgets[0]->refreshLayout();
            });
    }

    void mGenericAssignPanel::slot_ok()
    {
        int number = _assignNum->text().toInt();
        if (number <= 0) {
            // 超单元个数输入错误仍保留弹窗（系统级错误提示）
            QMessageBox::warning(this, "输入错误", "调用超单元个数必须大于0");
            _assignNum->setText("1");
            return;
        }

        adjustImportRows(number);

        // 同步数据
        syncUIToData();

        // 超单元个数变化后重新验证所有矩阵输入
        if (_genericAssign) {
            int expectedCount = number;
            QString errorMsg;

            if (_stiffnessCheck->isChecked()) {
                if (!validateMatrixNames(_stiffnessLine->text(), expectedCount, errorMsg)) {
                    showErrorTip(_stiffnessLine, errorMsg);
                }
                else {
                    clearErrorTip(_stiffnessLine);
                }
            }

            if (_massCheck->isChecked()) {
                if (!validateMatrixNames(_massLine->text(), expectedCount, errorMsg)) {
                    showErrorTip(_massLine, errorMsg);
                }
                else {
                    clearErrorTip(_massLine);
                }
            }

            if (_viscousDampingCheck->isChecked()) {
                if (!validateMatrixNames(_viscousDampingLine->text(), expectedCount, errorMsg)) {
                    showErrorTip(_viscousDampingLine, errorMsg);
                }
                else {
                    clearErrorTip(_viscousDampingLine);
                }
            }

            if (_materialCheck->isChecked()) {
                if (!validateMatrixNames(_materialLine->text(), expectedCount, errorMsg)) {
                    showErrorTip(_materialLine, errorMsg);
                }
                else {
                    clearErrorTip(_materialLine);
                }
            }
        }
    }
}