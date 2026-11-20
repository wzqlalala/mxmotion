#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//QT
#include <QLineEdit>
#include <QValidator>
#include <vector>

namespace PostWindow
{
	/*
	* ID输入校验器（自前处理 mSelectMenuFunction.h 的 IDValidator 分离移植：
	* PostWindow 不依赖 MainWindow，解析规则保持一致）
	*/
	class mPostIdValidator : public QValidator
	{
	public:
		explicit mPostIdValidator(QObject* parent = nullptr) : QValidator(parent) {}

		State validate(QString& input, int& pos) const override;
	};

	/*
	* 后处理"按照Id选择"输入框（自前处理 mInputIDLineEdit 分离移植，格式一致）：
	* 单个ID、start:end、start:end:step，逗号分隔多段（如 4, 5:10, 60:100:2）
	*/
	class mPostIdLineEdit : public QLineEdit
	{
		Q_OBJECT

	public:
		explicit mPostIdLineEdit(QWidget* parent = nullptr);
		~mPostIdLineEdit();

		//获取解析后的ID列表（输入完成时自动解析）
		std::vector<int> getIds();

		//校验状态只改变背景色，不替换主题 QSS 的边框、字体和内边距
		enum class InputState { Normal, Invalid, Incomplete };
		void setInputState(InputState state);

	private slots:
		void slot_parseIdString(const QString& text);
		void slot_textChanged(const QString& text);

	private:
		std::vector<int> _ids;
	};
}
