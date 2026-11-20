#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QGridLayout>
#include <QListWidget>
#include <QToolButton>
#include <QButtonGroup>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QRegularExpression>
#include <QMessageBox>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mPostDataBase.h"
#include "mGlobalSignals.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


using namespace PostData;

namespace PostWindow
{
	class MPOSTWINDOW_EXPORT mPostPathPanel : public mPanelBase
	{
		Q_OBJECT
	public:

        friend class NodeIdDelegate;
		explicit mPostPathPanel(QWidget* parent = nullptr);

        //连接拾取信号
        void connectPickSig();

        //断开拾取信号
        void disconnectPickSig();

        //取消编辑
        void uneditPathPanel();

		/** 视口绑定打开（用户定案 2026-11-04）：路径不按次生成实例——数据键=当前
		 * 激活后处理视口名，同视口重复打开沿用既有配置，切换视口后打开自动跟随 */
		void editPathForViewport(int caseid);

		//设置当前显示节点
		void setCurViewName(const QString& viewName);

		//更新界面数据
		void updatePathGUI();

		//初始化界面
		void initUI();

		void addItem(QString str = "");

        void addPickItem(int id);

		void insertItemAt(int row, const QString& text);

		void selectLastRow();

        //解析输入的id
        QVector<int> parseIds(const QString& input);

        //存储路径数据
        void storagePathData();

        //更新节点状态
        void updateItemStatus();

        //高亮路径
        void highlightPath(QVector<int> ids, bool isIntersect = false);

        //清空高亮
        void clearhighlight();

	private slots:

	private slots:
        //拾取信号
        void slot_finishedPick();

		/** 添加 */
		void slot_addItem(QString str = "");

		/** 移除 */
		void slot_removeItem();

		/** 清空 */
		void slot_clearItems();

		/** 编辑完成*/
		void slot_itemChanged(QListWidgetItem* item);

    signals:

    protected:
        void hideEvent(QHideEvent* event) override;

	private:
		QVBoxLayout*							_mainLayout;
        QPointer<mPostDataBase>					_postDB{};
		QMetaObject::Connection                 _conn;                      //拾取信号连接
		QString									_curViewName;				//当前路径节点

		QComboBox*								_comboPathType;
		QListWidget*							_listPathIds;
		QToolButton*							_btnFront;
		QToolButton*							_btnEnd;
		QToolButton*							_btnAdd;
		QToolButton*							_btnRemove;
		QToolButton*							_btnClear;


	};

    class NodeIdDelegate : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        explicit NodeIdDelegate(QObject* parent = nullptr)
            : QStyledItemDelegate(parent) {}

        QWidget* createEditor(QWidget* parent,
            const QStyleOptionViewItem& option,
            const QModelIndex& index) const override
        {
            QLineEdit* editor = new QLineEdit(parent);

            // 设置输入验证器（可选：实时阻止非法字符）
            // 允许数字、逗号、冒号、空格
            QRegularExpression rx("[0-9,:\\s]*");
            QRegularExpressionValidator* validator =
                new QRegularExpressionValidator(rx, editor);
            editor->setValidator(validator);

            return editor;
        }

        void setEditorData(QWidget* editor,
            const QModelIndex& index) const override
        {
            QString value = index.model()->data(index, Qt::EditRole).toString();
            QLineEdit* lineEdit = static_cast<QLineEdit*>(editor);
            lineEdit->setText(value);
        }

        void setModelData(QWidget* editor,
            QAbstractItemModel* model,
            const QModelIndex& index) const override
        {
            QLineEdit* lineEdit = static_cast<QLineEdit*>(editor);
            QString value = lineEdit->text().trimmed();

            // 验证格式
            QString errorMsg;
            if (!validateNodeId(value, errorMsg))
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, errorMsg, true);
                // 不保存数据，保持编辑状态
                return;
            }

            model->setData(index, value, Qt::EditRole);
        }

    private:
        // 验证节点ID格式
        bool validateNodeId(const QString& input, QString& errorMsg) const
        {
            if (input.isEmpty())
            {
                errorMsg = "输入不能为空";
                return false;
            }

            // 格式1: 单个ID（纯数字）
            QRegularExpression singleIdRx("^\\d+$");
            if (singleIdRx.match(input).hasMatch())
            {
                return true;
            }

            // 格式2: 多个ID用逗号分隔（1,2,3）
            QRegularExpression multiIdRx("^\\d+(,\\d+)*$");
            if (multiIdRx.match(input).hasMatch())
            {
                return true;
            }

            // 格式3: 范围格式（20:30:2）
            QRegularExpression rangeRx("^\\d+:\\d+:\\d+$");
            if (rangeRx.match(input).hasMatch())
            {
                // 验证范围逻辑：start:end:step
                QStringList parts = input.split(':');
                int start = parts[0].toInt();
                int end = parts[1].toInt();
                int step = parts[2].toInt();

                if (step <= 0)
                {
                    errorMsg = "步长必须大于0";
                    return false;
                }

                if (start >= end)
                {
                    errorMsg = "起始值必须小于结束值";
                    return false;
                }

                return true;
            }

            // 都不匹配
            errorMsg = "格式错误！\n支持格式:"
                "1. 单个ID: 123  "
                "2. 多个ID: 1,2,3  "
                "3. 范围: 20:30:2 (起始:结束:步长)";
            return false;
        }
    };
}
