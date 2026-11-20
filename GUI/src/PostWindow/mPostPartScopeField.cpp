#include "mPostPartScopeField.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QGuiApplication>
#include <QScreen>
#include <QEvent>
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPostDataManager.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPhysicalFieldEnum.h"

//样式常量同 mPostExplodePanel"选择部件"字段（kCtrlBorder/kMuted），两处保持同款
namespace
{
    const char* kMuted = "#788391";			//辅助文字
    const char* kCtrlBorder = "#DCE2E9";	//控件描边

    //当前后处理数据库的全部部件（ID+名称，顺序与部件树一致；无库/无数据返回空）
    QVector<QPair<int, QString>> currentParts()
    {
        auto* vm = MechPreData::mViewportModelManager::getInstance();
        if (vm == nullptr) return {};
        auto* model = vm->getCurrentViewportModel();
        if (model == nullptr) return {};
        auto* data = model->getModelData();
        if (data == nullptr) return {};
        auto db = data->getManager<PostData::mPostDataManager>(MxFunctions::Physical_Post).getPostDatabase();
        if (db.isNull()) return {};

        QPointer<PostData::mPostModelData> modelData = db->getModelData(1);
        QVector<QPair<int, QString>> parts;
        const QVector<int> partIds = db->getAllPartId();
        parts.reserve(partIds.size());
        for (int id : partIds)
        {
            QString name = modelData ? modelData->getPartNameByPartId(id) : QString();
            if (name.isEmpty())
                name = QString("Part_%1").arg(id);
            parts.append({ id, name });
        }
        return parts;
    }
}

namespace PostWindow
{
	mPostPartScopeField::mPostPartScopeField(QWidget* parent) : QPushButton(parent)
	{
		setObjectName("postPartScopeField");
		setFixedHeight(26);
		setCursor(Qt::PointingHandCursor);
		//直角描边（原型要求，与同行"单元/部件"下拉的直角覆盖一致）
		setStyleSheet(
			QString("QPushButton{background:#FFFFFF;border:1px solid %1;border-radius:0px;}"
				"QPushButton:hover{border:1px solid #91CAFF;}")
			.arg(kCtrlBorder));

		//文字居中 + 右缘拾取图标（explode 字段图标为空占位，此处按原型显示"⊗"）
		QHBoxLayout* fieldLayout = new QHBoxLayout(this);
		fieldLayout->setContentsMargins(8, 0, 8, 0);
		fieldLayout->addStretch();
		_fieldText = new QLabel(this);
		_fieldText->setAttribute(Qt::WA_TransparentForMouseEvents);
		_fieldText->setStyleSheet(QString("QLabel{color:%1;background:transparent;font-size:12px;}").arg(kMuted));
		fieldLayout->addWidget(_fieldText);
		fieldLayout->addStretch();
		_pickIcon = new QLabel(QString::fromUtf8("⊗"), this);
		_pickIcon->setAttribute(Qt::WA_TransparentForMouseEvents);
		_pickIcon->setStyleSheet("QLabel{color:#1890FF;background:transparent;font-size:14px;}");
		fieldLayout->addWidget(_pickIcon);

		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		updateCaptionText(scopeText(_objectType, _scope, lang));

		connect(this, &QPushButton::clicked, this, &mPostPartScopeField::openPopup);
	}

	QString mPostPartScopeField::scopeText(int objectType, int index, int lang)
	{
		static const char* cnPart[] = { "当前显示", "所有部件", "通过集合", "通过材料", "通过属性" };
		static const char* enPart[] = { "Currently Displayed", "All Parts", "By Set", "By Material", "By Property" };
		static const char* cnUnit[] = { "当前显示", "所有单元", "单元集合" };
		static const char* enUnit[] = { "Currently Displayed", "All Elements", "By Element Set" };
		const bool unit = (objectType == 0);
		const int last = unit ? 2 : 4;
		if (index < 0 || index > last) index = last;
		return QString((lang == 0 ? (unit ? cnUnit : cnPart) : (unit ? enUnit : enPart))[index]);
	}

	QString mPostPartScopeField::scopeText() const
	{
		return scopeText(_objectType, _scope, 0);	//中文为持久化基准文本
	}

	void mPostPartScopeField::setScope(int index)
	{
		const int last = optionCount() - 1;
		if (index < 0 || index > last) index = last;
		_scope = index;
		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		updateCaptionText(scopeText(_objectType, _scope, lang));
		if (_popup)
			applyScope(_scope);
	}

	void mPostPartScopeField::setObjectType(int type)
	{
		if (type != 0 && type != 1) type = 1;
		if (type == _objectType) return;
		_objectType = type;

		//越界范围回退到该类型末项（部件的通过材料/通过属性在单元下无对应）
		if (_scope >= optionCount())
			_scope = optionCount() - 1;

		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		updateCaptionText(scopeText(_objectType, _scope, lang));
		if (_popup)
		{
			//弹窗高度随选项数收缩（5 项 210 / 3 项 150，边距与"完成拾取"占 60）
			_popup->setFixedHeight(optionCount() * 30 + 60);
			applyPopupLanguage(lang);
			applyScope(_scope);
		}
		//二级弹窗仅部件"通过材料"可达，切到单元后不可能停留，保险关闭
		closeScopeListPopup();
	}

	void mPostPartScopeField::setScopeByText(const QString& text)
	{
		int index = 1;
		for (int i = 0; i < optionCount(); ++i)
		{
			if (scopeText(_objectType, i, 0) == text)
			{
				index = i;
				break;
			}
		}
		setScope(index);
	}

	void mPostPartScopeField::refreshLanguage(int lang)
	{
		updateCaptionText(scopeText(_objectType, _scope, lang));
		applyPopupLanguage(lang);
	}

	void mPostPartScopeField::ensurePopup()
	{
		if (_popup) return;
		_popupItems.clear();	//重建路径：清上一实例陈旧指针（子件已随旧弹窗销毁）

		//无标题栏的紧凑面板（选项文字居中 + 选中项通栏浅蓝高亮 + 底部通栏浅蓝"完成拾取"），
		//样式带 ID 选择器防级联到子控件
		_popup = new QWidget(this, Qt::Popup | Qt::FramelessWindowHint);
		_popup->setObjectName("partScopePopup");
		_popup->setAttribute(Qt::WA_StyledBackground, true);
		_popup->setStyleSheet("#partScopePopup{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:4px;}");
		//高度随选项数（5 项 210 / 3 项 150）：边距 16 + 选项行 + "完成拾取"行 + 弹性 14
		_popup->setFixedSize(120, optionCount() * 30 + 60);
		_popup->installEventFilter(this);	//主弹窗隐藏时联动关闭二级弹窗

		QVBoxLayout* popupLayout = new QVBoxLayout(_popup);
		popupLayout->setContentsMargins(1, 8, 1, 8);
		popupLayout->setSpacing(0);

		//范围选项（单选：文字外围绘制边框的紧凑按钮，整行居中；点击仅切换范围，弹窗保持打开）
		for (int i = 0; i < 5; ++i)
		{
			QWidget* row = new QWidget(_popup);
			QHBoxLayout* rowLayout = new QHBoxLayout(row);
			rowLayout->setContentsMargins(0, 0, 0, 0);
			QPushButton* item = new QPushButton(row);
			item->setCursor(Qt::PointingHandCursor);
			item->setFixedHeight(30);
			item->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
			rowLayout->addStretch();
			rowLayout->addWidget(item);
			rowLayout->addStretch();
			popupLayout->addWidget(row);
			_popupItems.append(item);
			connect(item, &QPushButton::clicked, this, [this, i]() {
				applyScope(i);
				emit scopeChanged(i);
				if (i == 3)
					openScopeListPopup();
				else
					closeScopeListPopup();
				});
		}

		popupLayout->addStretch();

		//完成拾取（与上方范围选项同尺寸的边框按钮，居中放置在底部）
		QWidget* okRow = new QWidget(_popup);
		QHBoxLayout* okLayout = new QHBoxLayout(okRow);
		okLayout->setContentsMargins(0, 0, 0, 0);
		_btnFinishPick = new QPushButton(okRow);
		_btnFinishPick->setCursor(Qt::PointingHandCursor);
		_btnFinishPick->setFixedHeight(30);
		_btnFinishPick->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		_btnFinishPick->setStyleSheet(
			"QPushButton{background:#E8F3FF;border:1px solid #91CAFF;border-radius:3px;color:#1890FF;padding:0 16px;}"
			"QPushButton:hover{background:#D0E7FD;border:1px solid #69B4FF;}");
		okLayout->addStretch();
		okLayout->addWidget(_btnFinishPick);
		okLayout->addStretch();
		popupLayout->addWidget(okRow);

		connect(_btnFinishPick, &QPushButton::clicked, this, [this]() {
			closeScopeListPopup();
			_popup->close();
			emit finishPickRequested();
			});

		applyPopupLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);
		applyScope(_scope);
	}

	void mPostPartScopeField::applyScope(int index)
	{
		_scope = index;
		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		updateCaptionText(scopeText(_objectType, index, lang));

		const int count = optionCount();
		for (int i = 0; i < _popupItems.size(); ++i)
		{
			//超出当前类型选项数的条目隐藏（单元=3 项/部件=5 项）；
			//文字外围绘制边框：普通项灰框白底，选中项蓝框浅蓝底蓝字；
			//选项零间距堆叠成组，仅可见首末项保留外侧圆角，接缝处直角拼接
			QPushButton* item = _popupItems.at(i);
			item->setVisible(i < count);
			if (i >= count) continue;
			const bool first = (i == 0);
			const bool last = (i == count - 1);
			const QString radius = first ? "3px 3px 0 0" : last ? "0 0 3px 3px" : "0px";
			item->setStyleSheet(i == index
				? QString("QPushButton{border:1px solid #91CAFF;border-radius:%1;background:#E8F3FF;color:#1890FF;padding:0 16px;}").arg(radius)
				: QString("QPushButton{border:1px solid #D9E1EA;border-radius:%1;background:#FFFFFF;color:#424B57;padding:0 16px;}"
						  "QPushButton:hover{background:#F0F7FF;}").arg(radius));
		}
	}

	void mPostPartScopeField::applyPopupLanguage(int lang)
	{
		if (!_popup) return;
		const int count = optionCount();
		for (int i = 0; i < _popupItems.size(); ++i)
		{
			_popupItems.at(i)->setVisible(i < count);
			if (i < count)
				_popupItems.at(i)->setText(scopeText(_objectType, i, lang));
		}
		_btnFinishPick->setText(lang == 0 ? QString::fromUtf8("完成拾取") : QString("Finish Picking"));
		if (_scopeListPopup && _scopeListScope == 3)
			fillScopeList();	//材料二级弹窗占位条目/搜索占位随语言刷新
	}

	void mPostPartScopeField::ensureScopeListPopup()
	{
		if (_scopeListPopup) return;

		//作为主弹窗的子弹窗级联在右侧（主弹窗关闭时一并关闭）
		_scopeListPopup = new QWidget(_popup, Qt::Popup | Qt::FramelessWindowHint);
		_scopeListPopup->setObjectName("partScopeListPopup");
		_scopeListPopup->setAttribute(Qt::WA_StyledBackground, true);
		_scopeListPopup->setStyleSheet("#partScopeListPopup{background:#FFFFFF;border:1px solid #91CAFF;border-radius:4px;}");
		//宽度固定、高度自适应：弹窗高度 = 上边距 + 搜索框/材料列表内容 + 下边距
		_scopeListPopup->setFixedWidth(180);

		QVBoxLayout* layout = new QVBoxLayout(_scopeListPopup);
		layout->setContentsMargins(8, 10, 8, 10);
		layout->setSpacing(8);

		//顶部搜索框（按条目名过滤）
		_scopeSearchEdit = new QLineEdit(_scopeListPopup);
		_scopeSearchEdit->setFixedHeight(30);
		_scopeSearchEdit->setStyleSheet(
			"QLineEdit{background:#FFFFFF;border:1px solid #D9E1EA;border-radius:3px;padding:0 8px;color:#424B57;}"
			"QLineEdit:focus{border:1px solid #91CAFF;}");
		layout->addWidget(_scopeSearchEdit);

		//勾选列表（对齐设计稿：全包围长方形边框容器，行上下零间距、贴边框铺满，
		//行间以细分隔线区分；行内缩 1px 让出边框线宽，否则白底行会盖住边框）
		_scopeRowsWidget = new QWidget(_scopeListPopup);
		_scopeRowsWidget->setObjectName("scopeRowsBox");
		_scopeRowsWidget->setAttribute(Qt::WA_StyledBackground, true);
		_scopeRowsWidget->setStyleSheet(
			"#scopeRowsBox{background:#FFFFFF;border:1px solid #D9E1EA;}");
		_scopeRowsLayout = new QVBoxLayout(_scopeRowsWidget);
		_scopeRowsLayout->setContentsMargins(1, 1, 1, 1);
		_scopeRowsLayout->setSpacing(0);
		layout->addWidget(_scopeRowsWidget);

		connect(_scopeSearchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
			for (QCheckBox* check : _scopeRowChecks)
				check->setVisible(text.isEmpty() || check->text().contains(text, Qt::CaseInsensitive));
			});
	}

	void mPostPartScopeField::fillScopeList()
	{
		int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;

		_scopeSearchEdit->blockSignals(true);
		_scopeSearchEdit->clear();
		_scopeSearchEdit->setPlaceholderText(lang == 0
			? QString::fromUtf8("搜索部件") : QString("Search Parts"));
		_scopeSearchEdit->blockSignals(false);

		//重建部件勾选行（真实部件条目：取当前数据库全部部件，顺序与部件树/旧版参数
		//对话框的部件列表一致；条目名为文件内部件名，不随界面语言变化）；
		//按 _scopeCheckedIds 恢复上次勾选，勾选变化实时回写，弹窗关闭再开保持一致
		_scopeRowChecks.clear();
		while (QLayoutItem* it = _scopeRowsLayout->takeAt(0))
		{
			if (QWidget* w = it->widget()) w->deleteLater();
			delete it;
		}
		const QVector<QPair<int, QString>> parts = currentParts();
		_scopeRowsWidget->setVisible(!parts.isEmpty());
		for (int n = 0; n < parts.size(); ++n)
		{
			const int partId = parts.at(n).first;
			const QString& name = parts.at(n).second;
			QCheckBox* check = new QCheckBox(name, _scopeRowsWidget);
			check->setCursor(Qt::PointingHandCursor);
			check->setFixedHeight(36);
			//行贴容器边框铺满（零纵向间距），行间用底部分隔线区分，
			//末行不画线避免与容器下边框叠线；悬停/勾选整行浅蓝底
			check->setStyleSheet(n < parts.size() - 1
				? "QCheckBox{background:#FFFFFF;color:#424B57;padding:0 8px;spacing:8px;border:0;border-bottom:1px solid #E4E8EE;}"
				  "QCheckBox:hover{background:#F0F7FF;}"
				  "QCheckBox:checked{background:#E8F3FF;}"
				: "QCheckBox{background:#FFFFFF;color:#424B57;padding:0 8px;spacing:8px;border:0;}"
				  "QCheckBox:hover{background:#F0F7FF;}"
				  "QCheckBox:checked{background:#E8F3FF;}");
			check->setChecked(_scopeCheckedIds.count(partId) > 0);
			connect(check, &QCheckBox::toggled, this, [this, partId](bool on) {
				if (on) _scopeCheckedIds.insert(partId);
				else _scopeCheckedIds.erase(partId);
				emit scopeMaterialsChanged();	//V1.3 编辑缓冲 dirty 口径（宿主重算签名）
				});
			_scopeRowsLayout->addWidget(check);
			_scopeRowChecks.append(check);
		}
	}

	void mPostPartScopeField::openScopeListPopup()
	{
		if (!_popup) return;
		ensureScopeListPopup();
		_scopeListScope = 3;	//仅"通过材料"级联二级弹窗（同爆炸图）
		fillScopeList();
		//高度随内容（上边距+搜索框/列表+下边距）在每次打开前重算
		_scopeListPopup->adjustSize();

		//级联在主弹窗右侧、顶边对齐；右侧越界翻转到主弹窗左侧，再回落屏幕内
		const QPoint mainGlobal = _popup->mapToGlobal(QPoint(0, 0));
		QPoint pos(mainGlobal.x() + _popup->width() + 4, mainGlobal.y());
		QScreen* scr = QGuiApplication::screenAt(pos);
		const QRect avail = (scr ? scr : QGuiApplication::primaryScreen())->availableGeometry();
		if (pos.x() + _scopeListPopup->width() > avail.right())
			pos.setX(mainGlobal.x() - _scopeListPopup->width() - 4);
		pos.setX(qBound(avail.left() + 4, pos.x(), qMax(avail.left() + 4, avail.right() - _scopeListPopup->width() - 4)));
		pos.setY(qBound(avail.top(), pos.y(), qMax(avail.top(), avail.bottom() - _scopeListPopup->height())));
		_scopeListPopup->move(pos);
		_scopeListPopup->raise();
		_scopeListPopup->show();
	}

	void mPostPartScopeField::closeScopeListPopup()
	{
		_scopeListScope = -1;
		if (_scopeListPopup && _scopeListPopup->isVisible())
			_scopeListPopup->close();
	}

	bool mPostPartScopeField::eventFilter(QObject* watched, QEvent* event)
	{
		//主弹窗因点击外部被 Qt 关闭时，联动关闭二级弹窗
		if (watched == _popup && event->type() == QEvent::Hide)
			closeScopeListPopup();
		return QPushButton::eventFilter(watched, event);
	}

	void mPostPartScopeField::openPopup()
	{
		//V1.3 外部入口（对象选择复合行 inspect 槽）：转发内部开关逻辑
		showPopup();
	}

	void mPostPartScopeField::updateCaptionText(const QString& text)
	{
		if (_fieldText->text() == text)
			return;
		_fieldText->setText(text);
		emit scopeTextChanged(text);
	}

	void mPostPartScopeField::showPopup()
	{
		ensurePopup();
		if (_popup->isVisible())
		{
			_popup->close();
			return;
		}
		applyPopupLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);
		applyScope(_scope);

		//锚定锚点部件（V1.3 复合行形态：外置锚点；缺省锚定自身）右缘、与锚点顶对齐
		//（共享折叠组被等值面面板接管期间宿主面板隐藏、其 mapToGlobal 不可靠，锚点
		//取始终可见的复合行/按钮）；右侧越界翻转到锚点左侧
		const QWidget* anchor = _anchor ? _anchor : this;
		const QPoint btnGlobal = anchor->mapToGlobal(QPoint(0, 0));
		QPoint pos(btnGlobal.x() + anchor->width() + 2, btnGlobal.y() - 2);
		QScreen* scr = QGuiApplication::screenAt(pos);
		const QRect avail = (scr ? scr : QGuiApplication::primaryScreen())->availableGeometry();
		if (pos.x() + _popup->width() > avail.right())
			pos.setX(btnGlobal.x() - _popup->width() - 2);
		if (pos.x() < avail.left())
			pos.setX(avail.left() + 4);
		pos.setY(qBound(avail.top(), pos.y(), qMax(avail.top(), avail.bottom() - _popup->height())));
		_popup->move(pos);
		_popup->show();
	}
}
