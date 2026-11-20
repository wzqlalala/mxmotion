#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QSlider>
#include <QMouseEvent>
#include "mmainwindow_global.h"
namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mPostSlider : public QSlider
	{
		Q_OBJECT
    public:
        explicit mPostSlider(QWidget* parent = nullptr);
        explicit mPostSlider(Qt::Orientation orientation, QWidget* parent = nullptr);

    protected:
        void mousePressEvent(QMouseEvent* event) override;
	};
}