#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//QT
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>

namespace MechMainWindow
{
	/**
	 * @brief 运动表达式构造器（公用控件）
	 * @details 函数目录点选插入 + 运算符按钮 + 多行编辑 + 实时语法校验 + 预览信号。
	 *          校验与求解期求值共用引擎的 MotionFunction_Expr，所见即所得。
	 *          预览不由本控件实现（不知道方案时长），通过 previewRequested 信号交给宿主。
	 *          后续矢量力/弹簧/控制输入等表达式输入均可复用本控件。
	 */
	class MMAINWINDOW_EXPORT mMotionExprBuilderWidget : public QWidget
	{
		Q_OBJECT

	public:
		explicit mMotionExprBuilderWidget(QWidget* parent = nullptr);

		/// 当前表达式（trimmed）
		QString expression() const;

		/// 当前表达式是否通过语法校验
		bool isValid() const;

		/// 程序回填表达式（面板还原用，不触发 expressionAccepted）
		void setExpression(const QString& expr);

	signals:
		/// 文本任意变化（含非法）
		void expressionChanged(const QString& expr);

		/// 表达式变为语法合法（宿主写数据的时机）
		void expressionAccepted(const QString& expr);

		/// 用户点击预览按钮（宿主负责采样画曲线）
		void previewRequested(const QString& expr);

	private slots:
		void slotValidate();

	private:
		//在光标处插入文本，并把光标回退 offset 个字符（落进括号/逗号处）
		void insertAtCursor(const QString& text, int cursorBackOffset);

		//按类别刷新函数下拉
		void refreshFunctionList();

		QPlainTextEdit* m_edit;            ///< 表达式多行编辑区
		QLabel* m_lblStatus;               ///< 校验状态（合法绿色/非法红色+位置）
		QComboBox* m_cmbCategory;          ///< 函数分类
		QComboBox* m_cmbFunction;          ///< 该分类下的函数
		QPushButton* m_btnInsert;          ///< 插入所选函数
		QPushButton* m_btnPreview;         ///< 预览曲线

		QString m_lastAccepted;            ///< 上一次判定合法的表达式（去重发信号）
	};
}
