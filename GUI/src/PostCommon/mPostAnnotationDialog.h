#include "mcommon_global.h"
#include <QDialog>
#include <QMenu>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPoint>

namespace PostCommon
{
	class POSTCOMMON_EXPORT mPostAnnotationDialog : public QDialog {

		Q_OBJECT

	public:
		explicit mPostAnnotationDialog(QWidget* parent = nullptr);

		//注释模板文本回填/取出（{X}/{Y} 等字段占位符由调用方自行求值）
		void setAnnotationText(const QString& text);
		QString annotationText() const;

	signals:
		//点击"保存"后、窗口关闭前发射，携带编辑框当前文本
		void sig_textSaved(const QString& text);

	private slots:
		// 弹出下拉菜单
		void showInsertMenu();

		// 保存操作
		void saveText();

	protected:
		//标题栏拖动移动窗口（无边框窗口自绘标题栏，照 mPropertyEditorPopup）
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;

	private:
		QTextEdit* m_textEdit;
		QPushButton* m_insertBtn;
		QWidget* m_titleBar = nullptr;	//自绘标题栏（拖动热区）
		bool m_dragging = false;
		QPoint m_dragOffset;
	};
}