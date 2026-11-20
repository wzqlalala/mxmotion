#ifndef COLORBARWIDGET_H
#define COLORBARWIDGET_H

#include <QWidget>
#include <QColor>
#include <QVector>
#include <QLineEdit>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostWindow
{
	class ColorBarWidget : public QWidget
	{
		Q_OBJECT

	public:
		explicit ColorBarWidget(QWidget *parent = nullptr);
		void setEditlineHide();
		void setDefaultColorBar();
        void getColorAndValue();
        void getDefaultColorAndValue(QVector<QColor> colors, QVector<QString> values);
        int getDefaultColorNum();

	protected:
		void paintEvent(QPaintEvent *event) override;
		void mouseMoveEvent(QMouseEvent *event) override;
		void mousePressEvent(QMouseEvent *event) override;

    private slots:
        /** 读取后处理文件完成 */
        void slot_finishedImportFiles(const QString& postname);

    private:
		void updateHoverIndex(const QPoint &pos);
		QRect numberRect(int index);

	signals:
		void colorChanged(int index, const QColor &color);

    private:
        QVector<QColor> colors_;
        QVector<QString> values_;
        QVector<QColor> defaultColors_;
        QVector<QString> defaultValues_;
        int colorBarNum_;
        int colorIndex_;
        int valueIndex_;
        QLineEdit *edit_;
	};
}
#endif // COLORBARWIDGET_H
