#include "mDockBase.h"
#include <QVBoxLayout>
#include <QLabel>

namespace MechMainWindow
{
	mDockBase::mDockBase(QWidget* parent):QDockWidget(parent)
	{
		_windowTitle = "";
		_windowArea = Qt::RightDockWidgetArea;
		_windowVisible = true;
	}

	mDockBase::~mDockBase()
	{
	}

	void mDockBase::setDockWindowTitle(const QString& title)
	{
		this->setWindowTitle(title);
		_windowTitle = title;
	}

	QString mDockBase::getDockWindowTitle()
	{
		return _windowTitle;
	}

	void mDockBase::setDockWindowArea(Qt::DockWidgetAreas area)
	{
		_windowArea = area;
	}

	Qt::DockWidgetAreas mDockBase::getDockWindowArea()
	{
		return _windowArea;
	}

	void mDockBase::setDockWindowVisible(bool visi)
	{
		this->setVisible(visi);
		_windowVisible = visi;
	}

	bool mDockBase::getDockWindowVisible()
	{
		return _windowVisible;
	}
}