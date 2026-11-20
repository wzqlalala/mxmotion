#include "mPostObjectEditors.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QGuiApplication>

namespace
{
	//令牌（--std-* 标准档；与截面剖切/标签面板同口径）
	constexpr int kControl = 26;			//--std-control
	constexpr int kGap = 6;					//--std-gap
	constexpr int kRadius = 3;				//--ui-popup-radius（弹窗档）
	const char* kTextColor = "#303313";
	const char* kMuted = "#788391";
	const char* kDisabledFg = "#A6AFBB";
	const char* kCtrlBorder = "#DCE2E9";
	const char* kHoverBg = "#F0F7FF";
	const char* kSelectedBg = "#E8F3FF";
	const char* kPrimary = "#1890FF";
	const char* kPrimaryHover = "#1677C8";
	const char* kError = "#BF3434";
	const char* kHeaderColor = "#E9EDF3";

	QString defaultButtonQss()
	{
		return QString("QPushButton{border:1px solid %1;border-radius:4px;background:#FFFFFF;"
			"color:%2;padding:0 7px;}"
			"QPushButton:hover{background:%3;}"
			"QPushButton:disabled{background:#F5F7FA;color:%4;}")
			.arg(kCtrlBorder, kTextColor, kHoverBg, kDisabledFg);
	}

	QString primaryButtonQss()
	{
		return QString("QPushButton{border:1px solid %1;border-radius:4px;background:%1;"
			"color:#FFFFFF;padding:0 7px;}"
			"QPushButton:hover{background:%2;}"
			"QPushButton:disabled{background:#F5F7FA;color:%3;border-color:%4;}")
			.arg(kPrimary, kPrimaryHover, kDisabledFg, kCtrlBorder);
	}

	/* 未保存确认窗（原型 #postObjectDirtyDialog：宽 min(14*26, 视口-边距)，标题栏
	 * "未保存的修改"（#E9EDF3 底），正文，三等宽按钮 继续编辑/放弃并切换/保存并切换
	 * （末者主蓝）；Escape/关闭=继续编辑） */
	class PostObjectDirtyDialog : public QDialog
	{
	public:
		enum Choice { Stay = 0, Discard = 1, Save = 2 };

		explicit PostObjectDirtyDialog(QWidget* parent) : QDialog(parent)
		{
			setObjectName("postObjectDirtyDialog");
			setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
			setModal(true);
			setStyleSheet(QString("QDialog{background:#FFFFFF;border:1px solid %1;"
				"border-radius:%2px;}").arg(kCtrlBorder).arg(kRadius));

			QVBoxLayout* lay = new QVBoxLayout(this);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(0);

			_header = new QLabel(this);
			_header->setObjectName("postObjectDirtyTitle");
			_header->setFixedHeight(kControl);
			_header->setStyleSheet(QString("QLabel{background:%1;border-bottom:1px solid %2;"
				"color:%3;padding-left:8px;}").arg(kHeaderColor, kCtrlBorder, kTextColor));
			lay->addWidget(_header);

			_message = new QLabel(this);
			_message->setObjectName("postObjectDirtyMessage");
			_message->setWordWrap(true);
			_message->setStyleSheet(QString("QLabel{color:%1;padding:8px;background:transparent;}")
				.arg(kTextColor));
			lay->addWidget(_message);

			QWidget* actions = new QWidget(this);
			actions->setObjectName("postObjectDirtyActions");
			QHBoxLayout* actionLayout = new QHBoxLayout(actions);
			actionLayout->setContentsMargins(8, 0, 8, 8);
			actionLayout->setSpacing(kGap);
			_stayBtn = new QPushButton(actions);
			_stayBtn->setStyleSheet(defaultButtonQss());
			_discardBtn = new QPushButton(actions);
			_discardBtn->setStyleSheet(defaultButtonQss());
			_saveBtn = new QPushButton(actions);
			_saveBtn->setStyleSheet(primaryButtonQss());
			for (QPushButton* btn : { _stayBtn, _discardBtn, _saveBtn })
			{
				btn->setFixedHeight(kControl);
				btn->setCursor(Qt::PointingHandCursor);
				actionLayout->addWidget(btn, 1);
			}
			lay->addWidget(actions);

			connect(_stayBtn, &QPushButton::clicked, this, [this]() { _choice = Stay; accept(); });
			connect(_discardBtn, &QPushButton::clicked, this, [this]() { _choice = Discard; accept(); });
			connect(_saveBtn, &QPushButton::clicked, this, [this]() { _choice = Save; accept(); });
			//Esc/右上关闭=继续编辑（原型 cancel 事件 preventDefault + finish('stay')）
			connect(this, &QDialog::rejected, this, [this]() { _choice = Stay; });
		}

		int popup(const QString& message, bool english)
		{
			_header->setText(english ? QString("Unsaved Changes") : QString::fromUtf8("未保存的修改"));
			_message->setText(message);
			_stayBtn->setText(english ? QString("Keep Editing") : QString::fromUtf8("继续编辑"));
			_discardBtn->setText(english ? QString("Discard & Switch") : QString::fromUtf8("放弃并切换"));
			_saveBtn->setText(english ? QString("Save & Switch") : QString::fromUtf8("保存并切换"));
			setMinimumWidth(14 * kControl);	//原型 min(364px, 视口-2*边距)
			adjustSize();
			if (parentWidget())
			{
				QScreen* screen = QGuiApplication::screenAt(parentWidget()->mapToGlobal(QPoint(0, 0)));
				if (screen)
					setMaximumWidth(qMax(200, screen->availableGeometry().width() - 24));
			}
			_choice = Stay;
			_stayBtn->setFocus();
			return exec();
		}

	private:
		QLabel* _header = nullptr;
		QLabel* _message = nullptr;
		QPushButton* _stayBtn = nullptr;
		QPushButton* _discardBtn = nullptr;
		QPushButton* _saveBtn = nullptr;
		int _choice = Stay;
	};
}

namespace PostWindow
{
	mPostObjectEditController::mPostObjectEditController(QObject* parent) : QObject(parent)
	{
	}

	void mPostObjectEditController::attachFooter(QLayout* targetLayout, int insertRow, QWidget* parentWidget)
	{
		if (_footer)
			return;

		QWidget* host = parentWidget ? parentWidget : (targetLayout ? targetLayout->parentWidget() : nullptr);
		_footer = new QWidget(host);
		_footer->setObjectName("postObjectEditFooter");
		QVBoxLayout* footerLayout = new QVBoxLayout(_footer);
		footerLayout->setContentsMargins(0, 0, 0, 0);
		footerLayout->setSpacing(kGap);

		//即时保存复选（原型 .post-object-immediate：左对齐、13px 复选）
		_immediateCheck = new QCheckBox(_footer);
		_immediateCheck->setObjectName("postImmediate");
		_immediateCheck->setStyleSheet(
			"QCheckBox{spacing:4px;background:transparent;color:#303313;}");
		_immediateCheck->setChecked(false);
		_immediateCheck->setEnabled(false);
		footerLayout->addWidget(_immediateCheck);

		//整行等宽双按钮（原型 .post-object-edit-buttons：两列 1fr，高 26）
		_buttonRow = new QWidget(_footer);
		_buttonRow->setObjectName("post-object-edit-buttons");
		QHBoxLayout* buttonLayout = new QHBoxLayout(_buttonRow);
		buttonLayout->setContentsMargins(0, 0, 0, 0);
		buttonLayout->setSpacing(kGap);
		_cancelBtn = new QPushButton(_buttonRow);
		_cancelBtn->setObjectName("postObjectEditCancel");
		_cancelBtn->setStyleSheet(defaultButtonQss());
		_submitBtn = new QPushButton(_buttonRow);
		_submitBtn->setObjectName("postObjectEditSubmit");
		_submitBtn->setStyleSheet(primaryButtonQss());
		for (QPushButton* btn : { _cancelBtn, _submitBtn })
		{
			btn->setFixedHeight(kControl);
			btn->setCursor(Qt::PointingHandCursor);
			btn->setEnabled(false);
			buttonLayout->addWidget(btn, 1);
		}
		footerLayout->addWidget(_buttonRow);

		//状态行（原型 .post-object-save-state：11px muted，空文案收起）
		_statusLabel = new QLabel(_footer);
		_statusLabel->setObjectName("postObjectSaveState");
		_statusLabel->setStyleSheet(QString("QLabel{color:%1;font-size:11px;background:transparent;}")
			.arg(kMuted));
		_statusLabel->setVisible(false);
		footerLayout->addWidget(_statusLabel);

		connect(_immediateCheck, &QCheckBox::toggled, this, [this](bool checked)
			{
				_attempt.clear();
				refresh();
			});
		connect(_cancelBtn, &QPushButton::clicked, this, &mPostObjectEditController::cancelRequested);
		connect(_submitBtn, &QPushButton::clicked, this, [this]() { emit saveRequested(false); });

		if (QBoxLayout* box = qobject_cast<QBoxLayout*>(targetLayout))
		{
			if (insertRow >= 0)
				box->insertWidget(insertRow, _footer);
			else
				box->addWidget(_footer);
		}
		retranslate(_langEn);
		refresh();
	}

	bool mPostObjectEditController::immediateSave() const
	{
		return _immediateCheck && _immediateCheck->isChecked();
	}

	void mPostObjectEditController::setEditorHost(QWidget* host)
	{
		_editorHost = host;
		if (_editorHost)
			_editorHost->setEnabled(_editing);
	}

	void mPostObjectEditController::beginEdit()
	{
		_editing = true;
		_baseline = snapshot();
		_attempt.clear();
		if (_editorHost)
			_editorHost->setEnabled(true);
		refresh();
	}

	void mPostObjectEditController::endEdit()
	{
		_editing = false;
		_dirty = false;
		_baseline.clear();
		_attempt.clear();
		if (_editorHost)
			_editorHost->setEnabled(false);
		if (_cancelBtn) _cancelBtn->setEnabled(false);
		if (_submitBtn) _submitBtn->setEnabled(false);
		if (_immediateCheck) _immediateCheck->setEnabled(false);
		if (_statusLabel) _statusLabel->setVisible(false);
	}

	void mPostObjectEditController::notifyEditChanged()
	{
		if (_refreshQueued)
			return;
		_refreshQueued = true;
		QTimer::singleShot(0, this, &mPostObjectEditController::slotApplyPendingRefresh);
	}

	void mPostObjectEditController::slotApplyPendingRefresh()
	{
		_refreshQueued = false;
		refresh();
	}

	void mPostObjectEditController::markSaved()
	{
		_baseline = _editing ? snapshot() : QString();
		_attempt.clear();
		refresh();
	}

	void mPostObjectEditController::rebase()
	{
		//行内快捷改动（管理表颜色列等）不入 dirty：直接重取基线（原型 rebase 的简化等价）
		_baseline = _editing ? snapshot() : QString();
		refresh();
	}

	QString mPostObjectEditController::snapshot() const
	{
		return _snapshot ? _snapshot() : QString();
	}

	void mPostObjectEditController::refresh()
	{
		if (!_footer)
			return;
		const bool immediate = _immediateCheck->isChecked();
		const QString sig = _editing ? snapshot() : QString();
		_dirty = _editing && sig != _baseline;

		_immediateCheck->setEnabled(_editing);
		_buttonRow->setVisible(!immediate);
		const bool enable = _editing && _dirty;
		_cancelBtn->setEnabled(enable);
		_submitBtn->setEnabled(enable);
		updateStatusText();

		//即时保存调度（原型 schedule：签名与上次尝试不同才再试，防无效值反复落库）
		if (immediate && _dirty && !_saving && !_dialogOpen && sig != _attempt && sig != _baseline)
		{
			QTimer::singleShot(0, this, [this, sig]()
				{
					if (!_editing || !_immediateCheck->isChecked() || _saving || _dialogOpen)
						return;
					const QString current = snapshot();
					if (current != sig)
					{
						_attempt.clear();
						refresh();
						return;
					}
					if (_attempt == sig)
						return;
					_attempt = sig;
					_saving = true;
					const bool ok = _save ? _save(true) : false;
					_saving = false;
					if (ok)
						markSaved();
					else
						refresh();
				});
		}
	}

	void mPostObjectEditController::updateStatusText()
	{
		if (!_statusLabel)
			return;
		//原型：即时保存开启且仍有未保存修改（自动保存未通过校验）时提示
		const QString text = (_immediateCheck->isChecked() && _dirty)
			? (_langEn ? QString("Changes not saved: complete valid parameters first.")
				: QString::fromUtf8("修改尚未保存，请完善有效参数。"))
			: QString();
		_statusLabel->setText(text);
		_statusLabel->setVisible(!text.isEmpty());
	}

	bool mPostObjectEditController::checkSwitch(const std::function<void()>& action)
	{
		if (!_editing || !_dirty || _bypass)
		{
			action();
			return true;
		}
		PostObjectDirtyDialog dialog(qobject_cast<QWidget*>(parent()));
		const QString name = _currentName ? _currentName() : QString::fromUtf8("当前对象");
		const QString message = _langEn
			? QString("“%1” has unsaved changes. Save before switching?").arg(name)
			: QString::fromUtf8("“%1”有未保存的修改。是否保存后切换？").arg(name);
		_dialogOpen = true;
		const int choice = dialog.popup(message, _langEn);
		_dialogOpen = false;
		if (choice == PostObjectDirtyDialog::Stay)
			return false;
		if (choice == PostObjectDirtyDialog::Save)
		{
			const bool ok = _save ? _save(false) : false;
			if (!ok)
			{
				refresh();	//校验失败保持原对象与输入（原型 finish('save') 失败即止）
				return false;
			}
			markSaved();
		}
		else
		{
			if (_cancelFn)
				_cancelFn();
			markSaved();
		}
		_bypass = true;
		action();
		_bypass = false;
		return true;
	}

	void mPostObjectEditController::retranslate(bool english)
	{
		_langEn = english;
		if (_immediateCheck)
			_immediateCheck->setText(english ? QString("Instant Save") : QString::fromUtf8("即时保存"));
		if (_cancelBtn)
			_cancelBtn->setText(english ? QString("Cancel Editing") : QString::fromUtf8("取消编辑"));
		if (_submitBtn)
			_submitBtn->setText(english ? QString("Save Changes") : QString::fromUtf8("保存修改"));
		updateStatusText();
	}

	QString postObjectUniqueName(const QString& base, const QStringList& usedNames)
	{
		//原型 uniqueName：stem 被占用则 stem_2、stem_3…
		const QString stem = base.trimmed().isEmpty() ? QString::fromUtf8("对象") : base.trimmed();
		QString value = stem;
		for (int n = 2; usedNames.contains(value); ++n)
			value = stem + QStringLiteral("_") + QString::number(n);
		return value;
	}
}
