#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 弹窗几何记忆工具（仅当次软件运行内）
//
// 弹窗记住用户本次运行中调整过的大小（普通对话框连同位置）：
//   - 同一次运行内：隐藏或关闭弹窗后再打开，仍是用户调整后的尺寸；
//   - 关闭软件再启动：恢复代码里 resize() 的默认尺寸（不跨重启记忆）。
// 存储：进程内存缓存（static 表，随进程退出清空），不读写 Settings.ini——
// 旧版本写入 ini 的几何记录不再读取（残留键无害）。
//
// ── 新弹窗接入（只需一行）────────────────────────────────────────
// 在弹窗构造函数 resize(默认尺寸) 之后调用：
//
//   DlgGeom::bind(this, "MyDlg");                            //记 大小+位置
//   DlgGeom::bind(this, "MyDlg", DlgGeom::Mode::SizeOnly);   //只记 大小
//
// bind 内部完成两件事，之后无需再写任何代码：
//   1. 立即恢复本次运行中已保存的几何（无记录则保持默认 resize 值）；
//   2. 挂事件过滤器（以弹窗为父对象，随弹窗销毁），此后每次隐藏
//      （✕关闭 / hide() / exec() 结束 / 销毁前）自动保存。
//
// 说明：
//   - SizeOnly 用于无边框弹窗等"位置由调用方每次指定"的场景
//     （如 mPropertyEditorPopup 落点到图元旁）；默认尺寸取 bind 时的
//     当前尺寸，恢复时钳制到 [200x200, 主屏可用区域]，非法值回退默认。
//   - dlgKey 为会话缓存键名（与旧 ini 键同形便于排查），类改名不影响键。
// =============================================================================

#include <QHash>
#include <QVariant>
#include <QGuiApplication>
#include <QScreen>
#include <QObject>
#include <QEvent>
#include <QWidget>
#include <QSize>

namespace PostCurve
{
	namespace DlgGeom
	{
		//持久化模式：Geometry=大小+位置（普通对话框）；SizeOnly=仅宽高
		//（无边框弹窗，位置由调用方每次指定）
		enum class Mode { Geometry, SizeOnly };

		//会话级几何缓存（仅当次软件运行有效，进程结束即失）。键与旧版
		//Settings.ini 键同形（PostCurve/<dlgKey>/Geometry、/Width、/Height），
		//值=saveGeometry 字节流或宽高整数值
		inline QHash<QString, QVariant>& sessionStore()
		{
			static QHash<QString, QVariant> store;
			return store;
		}

		//该弹窗在本次运行中是否已保存过几何（供"无记录才居中于父窗口"
		//等首开逻辑查询；替代旧版对 ini 的 contains 检查）
		inline bool hasSessionGeometry(const QString& dlgKey)
		{
			const QString base = "PostCurve/" + dlgKey;
			return sessionStore().contains(base + "/Geometry")
				|| (sessionStore().contains(base + "/Width")
					&& sessionStore().contains(base + "/Height"));
		}

		//普通对话框：保存完整几何（大小+位置，含最大化状态）
		inline void saveWidgetGeometry(QWidget* w, const QString& dlgKey)
		{
			sessionStore()["PostCurve/" + dlgKey + "/Geometry"] = w->saveGeometry();
		}

		//普通对话框：恢复几何；本次运行无记录或还原失败时保持默认 resize() 的值
		inline void restoreWidgetGeometry(QWidget* w, const QString& dlgKey)
		{
			const QString key = "PostCurve/" + dlgKey + "/Geometry";
			if (!sessionStore().contains(key))
				return;
			w->restoreGeometry(sessionStore().value(key).toByteArray());
		}

		//无边框弹窗：只保存宽高
		inline void saveWidgetSize(QWidget* w, const QString& dlgKey)
		{
			const QString base = "PostCurve/" + dlgKey;
			sessionStore()[base + "/Width"] = w->width();
			sessionStore()[base + "/Height"] = w->height();
		}

		//无边框弹窗：恢复宽高，钳制到 [minSize, 主屏可用区域]，越界/无记录回退默认
		inline void restoreWidgetSize(QWidget* w, const QString& dlgKey,
			const QSize& defaultSize, const QSize& minSize = QSize(200, 200))
		{
			const QString base = "PostCurve/" + dlgKey;
			if (!sessionStore().contains(base + "/Width") || !sessionStore().contains(base + "/Height"))
				return;

			int width = sessionStore().value(base + "/Width", defaultSize.width()).toInt();
			int height = sessionStore().value(base + "/Height", defaultSize.height()).toInt();
			QSize avail = QGuiApplication::primaryScreen()->availableGeometry().size();
			width = qBound(minSize.width(), width, avail.width());
			height = qBound(minSize.height(), height, avail.height());
			w->resize(width, height);
		}

		//内部：隐藏即保存的事件过滤器（父对象为弹窗，随弹窗销毁，无需手动清理）
		class mGeomSaveFilter : public QObject
		{
		public:
			mGeomSaveFilter(QWidget* target, const QString& key, Mode mode)
				: QObject(target), _key(key), _mode(mode)
			{
				target->installEventFilter(this);
			}

			bool eventFilter(QObject* watched, QEvent* event) override
			{
				if (event->type() == QEvent::Hide)
				{
					if (QWidget* w = qobject_cast<QWidget*>(watched))
					{
						if (_mode == Mode::Geometry)
							saveWidgetGeometry(w, _key);
						else
							saveWidgetSize(w, _key);
					}
				}
				return QObject::eventFilter(watched, event);   //只旁观不拦截
			}

		private:
			QString _key;
			Mode _mode;
		};

		//一行接入：恢复本次运行已保存的几何 + 之后每次隐藏自动保存（见文件头注释）
		inline void bind(QWidget* w, const QString& dlgKey,
			Mode mode = Mode::Geometry, const QSize& minSize = QSize(200, 200))
		{
			if (!w)
				return;
			if (mode == Mode::Geometry)
				restoreWidgetGeometry(w, dlgKey);
			else
				restoreWidgetSize(w, dlgKey, w->size(), minSize);   //默认尺寸=bind时（resize后）的当前尺寸
			new mGeomSaveFilter(w, dlgKey, mode);
		}
	}
}
