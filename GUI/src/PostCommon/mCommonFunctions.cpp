#include "mCommonFunctions.h"
#include "mOutputMessage.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mBasicEnum.h"
#include <math.h>
//QT
#include <QPixmap>
#include <QRegExp>
#include <QRegExpValidator>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QFile>
#include <QCoreApplication>
#include <qdebug.h>
#include "mOtherFunction.h"
#include "mxr/view/REBUILD_PostRender.h"   // S5：G5 台账助手（listObjects/addObject/removeObject）
namespace PostCommon
{
    constexpr float EPSILON_FLOAT = 1e-6f;
	void mCommonFunctions::setButtonColor(QPushButton* button, const QColor& color)
	{

		if (button == nullptr)
		{
			return;
		}
		QPixmap cpix(QSize(50, 40));
		cpix.fill(color);
		button->setIcon(cpix);
		button->setText(color.name());
	}

	QStringList mCommonFunctions::filterList(const QStringList& list1, const QString& filter)
	{
		QStringList list2;

		//判断filter是否为空
		if (filter.isEmpty())
		{
			return list2;
		}

		//判断filter是否为*
		if (filter == "*")
		{
			return list1;
		}

		QString reStr = filter;
		//判断filter开头是否有*，若没有，则在开头添加^
		if (filter.at(0) != "*")
		{
			reStr = "^" + reStr;
		}
		//判断filter结尾是否有*，若没有，则在开头添加$
		if (filter.at(filter.size() - 1) != "*")
		{
			reStr = reStr + "$";
		}
		//将所有*替换成(\S)+
		while (reStr.contains("*"))
		{
			reStr = reStr.replace("*", "(\\S)+");
		}
		
		QRegExp re(reStr);
		QRegExpValidator v(re, 0);

		//开始过滤
		int pos = 0;
		QValidator::State vstate;
		const int count1 = list1.size();
		for (int i = 0; i < count1; ++i)
		{
			QString str = list1.at(i);
			vstate = v.validate(str, pos);

			if (vstate == QValidator::Acceptable)
			{
				list2.append(str);
			}
		}
		return list2;
	}

	QStringList mCommonFunctions::filterList1(const QStringList& list1, const QString& filter)
	{
		if (filter.isEmpty())
		{
			return list1;
		}
		QStringList filterList;
		const int nlist = list1.size();
		for (int i = 0; i < nlist; ++i)
		{
			QString tepStr = list1.at(i);
			if (tepStr.contains(filter,Qt::CaseInsensitive))
			{
				filterList.append(tepStr);
			}
		}
		return filterList;
	}

	void mCommonFunctions::clearStackedWidget(QStackedWidget * stackedwidget)
	{
		int count = stackedwidget->count();
		for (int i = 0; i < count; ++i)
		{
			QWidget *w = stackedwidget->widget(i);
			//stackedwidget->removeWidget(w);
			if (w != nullptr)
			{
				delete w;
				w = nullptr;
			}
		}
		delete stackedwidget;
		stackedwidget = new QStackedWidget;
	}

	void mCommonFunctions::clearListWidget(QListWidget* listwidget)
	{
		if (listwidget == nullptr)
		{
			return;
		}

		const int nrow = listwidget->count();
		for (int i = 0; i < nrow; ++i)
		{
			QListWidgetItem* tepItem = listwidget->item(i);
			if (tepItem != nullptr)
			{
				delete tepItem;
				tepItem = nullptr;
			}
		}
		listwidget->clear();
	}

	QStringList mCommonFunctions::getSelectedList(QListWidget* listWidget)
	{
		QStringList selectedList;
		if (listWidget == nullptr)
		{
			return selectedList;
		}
		
		const int nitem = listWidget->count();
		for (int i = 0; i < nitem; ++i)
		{
			QListWidgetItem* tepItem = listWidget->item(i);
			if (tepItem != nullptr)
			{
				if (tepItem->isSelected())
				{
					QString itemname = tepItem->text();
					selectedList.append(itemname);
				}
			}
		}
		return selectedList;
	}

	QStringList mCommonFunctions::getUnselectedList(QListWidget * listWidget)
	{
		QStringList unselectedList;
		if (listWidget == nullptr)
		{
			return unselectedList;
		}

		const int nitem = listWidget->count();
		for (int i = 0; i < nitem; ++i)
		{
			QListWidgetItem* tepItem = listWidget->item(i);
			if (tepItem != nullptr)
			{
				if (!tepItem->isSelected())
				{
					QString itemname = tepItem->text();
					unselectedList.append(itemname);
				}
			}
		}
		return unselectedList;
	}

	QList<int> mCommonFunctions::getSelectedIndexes(QListWidget* listwidget)
	{
		QList<int> lists;
		if (listwidget == nullptr)
		{
			return lists;
		}

		const int nitem = listwidget->count();
		for (int i = 0; i < nitem; ++i)
		{
			QListWidgetItem* tepItem = listwidget->item(i);
			if (tepItem != nullptr)
			{
				if (tepItem->isSelected())
				{
					lists.append(i);
				}
			}
		}
		return lists;
	}

	QString mCommonFunctions::getSelectedString(QListWidget* listwidget)
	{
		if (listwidget == nullptr)
		{
			return QString();
		}
		if (listwidget->count() == 0)
		{
			return QString();
		}
		//获取项
		QListWidgetItem* item = nullptr;
		QList<QListWidgetItem*> items = listwidget->selectedItems();
		if (items.size() == 0)
		{
			//返回列表中第一项的名称
			item = listwidget->item(0);
		}
		else
		{
			//返回选中的第一项名称
			item = items.at(0);
		}
		//返回名称
		if (item != nullptr)
		{
			return item->text();
		}
		else
		{
			return QString();
		}
	}

	QStringList mCommonFunctions::getCheckedItemNames(QListWidget * listwidget)
	{
		QStringList names;
		if (listwidget == nullptr)
		{
			return names;
		}
		const int nitem = listwidget->count();
		for (int i = 0; i < nitem; ++i)
		{
			QListWidgetItem* item = listwidget->item(i);
			if (item != nullptr&&item->checkState() == Qt::Checked)
			{
				names.append(item->text());
			}
		}
		return names;
	}

	void mCommonFunctions::setListWidgetItemSelected(QListWidget * listwidget, bool isselect)
	{
		if (listwidget == nullptr)
		{
			return;
		}
		const int n = listwidget->count();
		for (int i = 0; i < n; ++i)
		{
			QListWidgetItem* item = listwidget->item(i);
			if (item != nullptr)
			{
				item->setSelected(isselect);
			}
		}
	}

	void mCommonFunctions::setListWidgetItemSelected(QListWidget * listwidget, const QStringList & names)
	{
		if (listwidget == nullptr)
		{
			return;
		}
		const int n = listwidget->count();
		for (int i = 0; i < n; ++i)
		{
			QListWidgetItem* item = listwidget->item(i);
			if (item != nullptr)
			{
				QString name = item->text();
				if (names.contains(name))
				{
					item->setSelected(true);
				}
				else
				{
					item->setSelected(false);
				}
			}
		}
	}

	void mCommonFunctions::setListWidgetItemChecked(QListWidget * listwidget, const QStringList & names)
	{
		if (listwidget == nullptr)
		{
			return;
		}
		const int nitem = listwidget->count();
		for (int i = 0; i < nitem; ++i)
		{
			QListWidgetItem* item = listwidget->item(i);
			if (item != nullptr)
			{
				if (names.contains(item->text()))
				{
					item->setCheckState(Qt::Checked);
				}
				else
				{
					item->setCheckState(Qt::Unchecked);
				}
			}
		}
	}

	void mCommonFunctions::addCheckableListWidget(QListWidget* listwidget, const QStringList& texts)
	{
		if (listwidget == nullptr)
		{
			return;
		}
		const int nitem = texts.size();
		for (int i = 0; i < nitem; ++i)
		{
			QString txt = texts.at(i);
			QListWidgetItem* item = new QListWidgetItem(txt);
			item->setCheckState(Qt::Unchecked);
			listwidget->addItem(item);
		}
	}

	void mCommonFunctions::setListWidgetUnchecked(QListWidget* listwidget)
	{
		if (listwidget == nullptr)
		{
			return;
		}
		const int nitem = listwidget->count();
		for (int i = 0; i < nitem; ++i)
		{
			QListWidgetItem* item = listwidget->item(i);
			if (item != nullptr&&item->checkState() == Qt::Checked)
			{
				item->setCheckState(Qt::Unchecked);
			}
		}
	}

	void mCommonFunctions::setListWidgetUnchecked1(QListWidget * listwidget)
	{
		if (listwidget == nullptr)
		{
			return;
		}
		const int nitem = listwidget->count();
		for (int i = 0; i < nitem; ++i)
		{
			QListWidgetItem* item = listwidget->item(i);
			if (item != nullptr)
			{
				item->setCheckState(Qt::Unchecked);
			}
		}
	}

	void mCommonFunctions::setListWidgetItemState(QListWidget* listwidget, QListWidgetItem* item)
	{
		if (listwidget == nullptr || item == nullptr)
		{
			return;
		}

		Qt::CheckState cs = item->checkState();
		if (cs == Qt::Checked)
		{
			item->setCheckState(Qt::Unchecked);
		}
		else
		{
			//设置复选框状态
			const int nitem = listwidget->count();
			for (int i = 0; i < nitem; ++i)
			{
				QListWidgetItem* tepItem = listwidget->item(i);
				if (tepItem != nullptr&&tepItem != item)
				{
					tepItem->setCheckState(Qt::Unchecked);
				}
			}
			item->setCheckState(Qt::Checked);
		}
	}

	bool mCommonFunctions::isListWidgetContainItem(QListWidget* listwidget, const QString& itemtext)
	{
		if (listwidget == nullptr)
		{
			return false;
		}

		const int nrow = listwidget->count();
		for (int i = 0; i < nrow; ++i)
		{
			QListWidgetItem* tepItem = listwidget->item(i);
			if (tepItem != nullptr&&tepItem->text()==itemtext)
			{
				return true;
			}
		}
		return false;
	}

	QString mCommonFunctions::transformListToString(const std::set<int> ids, int type)
	{
		QString idstring;
		if (ids.size() == 0)
		{
			return idstring;
		}

		//将ID转成QString
		std::set<int>::iterator it;
		for (it = ids.begin(); it != ids.end(); it++)
		{
			int id = *it;
			if (it == ids.begin())
			{
				idstring = MxFunctions::mNumberToString(id);
			}
			else
			{
				idstring += QString(",%1").arg(id);
			}
		}
		//添加类型关键字
		if (type == 0)
		{
			idstring = QString("Node[%1]").arg(idstring);
		}
		else
		{
			idstring = QString("Ele[%1]").arg(idstring);
		}

		return idstring;
	}

	QString mCommonFunctions::transformListToString(const std::set<int> nids, const std::set<int> eids)
	{
		QString nidstr = transformListToString(nids, 0);
		QString eidstr = transformListToString(eids, 1);
		QString idstr;
		if (nids.size() > 0 && eids.size() > 0)
		{
			idstr = QString("%1;%2").arg(nidstr).arg(eidstr);
		}
		else if(nids.size()>0&&eids.size()==0)
		{
			idstr = nidstr;
		}
		else if (nids.size() == 0 && eids.size() > 0)
		{
			idstr = eidstr;
		}
		return idstr;
	}
	bool mCommonFunctions::transformMeshStringToSet(const QString & idstring, std::set<int>& mids)
	{
		QStringList idstrlist = idstring.split(",");
		for (QString idstr : idstrlist)
		{
			if (idstr.contains("-"))
			{
				QStringList conidlist = idstr.split("-");
				if (conidlist.size() == 2)
				{
					int startid = conidlist.at(0).simplified().toInt();
					int endid = conidlist.at(1).simplified().toInt();
					for (int i = startid; i <= endid; ++i)
					{
						mids.insert(i);
					}
				}
			}
			else
			{
				mids.insert(idstr.simplified().toInt());
			}
		}
		return true;
	}
	bool mCommonFunctions::transformSetToList(const std::set<int> id_set, QList<int>& id_list)
	{
		if (id_list.size() != 0)
		{
			return false;
		}
		for (auto it = id_set.begin(); it != id_set.end(); it++)
		{
			id_list.append(*it);
		}
		return true;
	}
	bool mCommonFunctions::transformListToSet(const QList<int> id_list, std::set<int>& id_set)
	{
		if (id_set.size() != 0)
		{
			return false;
		}
		for (auto it = id_list.begin(); it != id_list.end(); it++)
		{
			id_set.insert(*it);
		}
		return true;

	}
	int mCommonFunctions::transformStringToList(const QString& idstring, QList<int>& ids)
	{
		QRegExp nrx("(Node\\[)(.*)(?:\\])");
		QRegExp erx("(Ele\\[)(.*)(?:\\])");

		//判断并提取ID字符串
		QString idStr;
		int idType = -1;
		if (idstring.indexOf(nrx) >= 0)
		{
			QStringList list = nrx.capturedTexts();
			if (list.size() == 3)
			{
				idStr = list.at(2);
				idType = 0;
			}
		}
		else if (idstring.indexOf(erx)>= 0)
		{
			QStringList list = erx.capturedTexts();
			if (list.size() == 3)
			{
				idStr = list.at(2);
				idType = 1;
			}
		}
		else
		{
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("ID列表无效！"));
			}
			else
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("ID list is invalid!"));
			}
			
			return -1;
		}

		//分割并存储
		QStringList intIds = idStr.split(",");
		const int idcount = intIds.size();
		for (int i = 0; i < idcount; ++i)
		{
			QString tepId = intIds.at(i).simplified();
			if (!tepId.isEmpty())
			{
				//判断是否有连续ID
				if (tepId.contains(":"))
				{
					QStringList list2 = tepId.split(":");
					if (list2.size() == 2)
					{
						int startId = list2.at(0).toInt();
						int endId = list2.at(1).toInt();
						for (int j = startId; j <= endId; ++j)
						{
							ids.append(j);
						}
					}
				}
				else
				{
					ids.append(tepId.toInt());
				}
			}
		}
		return idType;
	}

	int mCommonFunctions::transformStringToList(const QString& idstring, std::set<int>& ids)
	{
		QRegExp nrx("(Node\\[)(.*)(?:\\])");
		QRegExp erx("(Ele\\[)(.*)(?:\\])");

		//判断并提取ID字符串
		QString idStr;
		int idType = -1;
		if (idstring.indexOf(nrx) >= 0)
		{
			QStringList list = nrx.capturedTexts();
			if (list.size() == 3)
			{
				idStr = list.at(2);
				idType = 0;
			}
		}
		else if (idstring.indexOf(erx) >= 0)
		{
			QStringList list = erx.capturedTexts();
			if (list.size() == 3)
			{
				idStr = list.at(2);
				idType = 1;
			}
		}
		else
		{
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("ID列表无效！"));
			}
			else
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("ID list is invalid!"));
			}
			return -1;
		}

		//分割并存储
		QStringList intIds = idStr.split(",");
		const int idcount = intIds.size();
		for (int i = 0; i < idcount; ++i)
		{
			QString tepId = intIds.at(i).simplified();
			if (!tepId.isEmpty())
			{
				//判断是否有连续ID
				if (tepId.contains(":"))
				{
					QStringList list2 = tepId.split(":");
					if (list2.size() == 2)
					{
						int startId = list2.at(0).toInt();
						int endId = list2.at(1).toInt();
						for (int j = startId; j <= endId; ++j)
						{
							ids.insert(j);
						}
					}
				}
				else
				{
					ids.insert(tepId.toInt());
				}
			}
		}
		return idType;
	}

	bool mCommonFunctions::transformStringToList(const QString& idstring, std::set<int>& nids, std::set<int>& eids)
	{
		QStringList splitStr = idstring.split(";");
		if (splitStr.size() > 2 || splitStr.size() == 0)
		{
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("ID列表无效！"));
			}
			else
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("ID list is invalid!"));
			}
			return false;
		}
		else if(splitStr.size()==2)
		{
			QString idstr1 = splitStr.at(0);
			QString idstr2 = splitStr.at(1);
			if (idstr1.left(4) == QString("Node"))
			{
				transformStringToList(idstr1, nids);
				transformStringToList(idstr2, eids);
			}
			else
			{
				transformStringToList(idstr2, nids);
				transformStringToList(idstr1, eids);
			}
			return true;
		}
		else if (splitStr.size() == 1)
		{
			QString idstr = splitStr.at(0);
			if (idstr.left(4) == QString("Node"))
			{
				transformStringToList(idstr, nids);
			}
			else
			{
				transformStringToList(idstr, eids);
			}
			return true;
		}
		else
		{
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("ID列表无效！"));
			}
			else
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("ID list is invalid!"));
			}
			return false;
		}
	}
	std::set<int> mCommonFunctions::transformQVectorToSet(const QVector<int> vec)
	{
		std::set<int> temp;
		for (int i = 0; i < vec.size(); i++)
		{
			temp.insert(vec[i]);
		}
		return temp;
	}

	QString mCommonFunctions::transformQStringListToQString(const QStringList& strlist)
	{
		QString str;
		const int nlist = strlist.size();
		for (int i = 0; i < nlist; ++i)
		{
			QString cname = strlist.at(i);
			if (i == 0)
			{
				str = cname;
			}
			else
			{
				str = QString("%1,%2").arg(str).arg(cname);
			}
		}
		return str;
	}

	double mCommonFunctions::getThreePointAngle(const QVector3D p1, const QVector3D p2, const QVector3D p3)
	{
		//获取向量1
		QVector3D vec1 = p1 - p3;
		QVector3D vec2 = p2 - p3;
        if (fabs(vec1.lengthSquared()) <= EPSILON_FLOAT * EPSILON_FLOAT || fabs(vec2.lengthSquared()) <= EPSILON_FLOAT * EPSILON_FLOAT)
            return 0.f;

		QVector3D center(0, 0, 0);
		double cos = std::abs(QVector3D::dotProduct(vec1, vec2)) / ((vec1.distanceToPoint(center))*(vec2.distanceToPoint(center)));
		double angle = acos(cos);
		//弧度转角度
		return angle * 180.0 / 3.1415926535;
	}

	QVector3D mCommonFunctions::getCenterPointCoord(const QVector3D p1, const QVector3D p2)
	{
		return (p1 + p2) / 2.0;
	}

	QVector3D mCommonFunctions::getCenterPointCoord(const QVector3D p1, const QVector3D p2, const QVector3D p3)
	{
		return (p1 + p2 + p3) / 3.0;
	}

	QVector3D mCommonFunctions::getCenterPointCoord(const QVector3D p1, const QVector3D p2, const QVector3D p3, const QVector3D p4)
	{
		double x = (p1.x() + p2.x() + p3.x() + p4.x()) / 4.0;
		double y = (p1.y() + p2.y() + p3.y() + p4.y()) / 4.0;
		double z = (p1.z() + p2.z() + p3.z() + p4.z()) / 4.0;
		return QVector3D(x, y, z);
	}

	void mCommonFunctions::getShrinkPosition(QVector3D& p1, QVector3D& p2, double val)
	{
		//计算两点距离
		double xminus = p1.x() - p2.x();
		double yminus = p1.y() - p2.y();
		double zminus = p1.z() - p2.z();
		p1.setX(p1.x() - xminus * 0.5*val);
		p1.setY(p1.y() - yminus * 0.5*val);
		p1.setZ(p1.z() - zminus * 0.5*val);
		p2.setX(p2.x() + xminus * 0.5*val);
		p2.setY(p2.y() + yminus * 0.5*val);
		p2.setZ(p2.z() + zminus * 0.5*val);
	}

	void mCommonFunctions::setChildrenItemState(QTreeWidgetItem* pitem, bool ischeck)
	{
		if (pitem == nullptr)
		{
			return;
		}

		Qt::CheckState cs = Qt::Unchecked;
		if (ischeck)
		{
			cs = Qt::Checked;
		}

		const int ncitem = pitem->childCount();
		for (int i = 0; i < ncitem; ++i)
		{
			QTreeWidgetItem* citem = pitem->child(i);
			if (citem != nullptr)
			{
				citem->setCheckState(0, cs);
			}
		}
	}

	QListWidgetItem* mCommonFunctions::getListWidgetItem(QListWidget* widget, const QString& name)
	{
		if (widget == nullptr)
		{
			return nullptr;
		}
		const int nitem = widget->count();
		for (int i = 0; i < nitem; ++i)
		{
			QListWidgetItem* tepitem = widget->item(i);
			if (tepitem != nullptr)
			{
				if (name == tepitem->text())
				{
					tepitem->setSelected(true);
					return tepitem;
				}
			}
		}
		return nullptr;
	}

	QListWidgetItem* mCommonFunctions::getFirstSelectedItem(QListWidget* widget)
	{
		if (widget == nullptr)
		{
			return nullptr;
		}
		QList<QListWidgetItem*> items = widget->selectedItems();
		if (items.size() == 0)
		{
			return nullptr;
		}
		else
		{
			return items.at(0);
		}
	}

	QStringList mCommonFunctions::getListWidgetText(QListWidget* lw)
	{
		QStringList itemlist;
		if (lw == nullptr)
		{
			return itemlist;
		}
		int nitem = lw->count();
		for (int i = 0; i < nitem; ++i)
		{
			itemlist.append(lw->item(i)->text());
		}
		return itemlist;
	}

	QString mCommonFunctions::transformVector3DToString(const QVector3D pos)
	{
		QString temp = MxFunctions::mNumberToString(pos.x()) + "," + MxFunctions::mNumberToString(pos.y()) + "," + MxFunctions::mNumberToString(pos.z());
		return temp;
	}


	QVector3D mCommonFunctions::transformStringToVector3D(const QString pos)
	{
		QStringList list = pos.split(",");
		QVector3D temp;
		if (list.size() >= 3)
		{
			temp = QVector3D(list[0].toDouble(), list[1].toDouble(), list[2].toDouble());
		}
		return temp;
	}
	QString mCommonFunctions::transformVector_Vector3DToString(const QVector<QVector3D> pos)
	{
		QString temp;
		temp += "(" + MxFunctions::mNumberToString(pos[0].x()) + "," + MxFunctions::mNumberToString(pos[0].y()) + "," + MxFunctions::mNumberToString(pos[0].z()) + "),";
		temp += "(" + MxFunctions::mNumberToString(pos[pos.size()-1].x()) + "," + MxFunctions::mNumberToString(pos[pos.size() - 1].y()) + "," + MxFunctions::mNumberToString(pos[pos.size() - 1].z()) + ")";
		return temp;

	}
	QVector<QVector3D> mCommonFunctions::transformStringToVector_Vector3D(const QString pos)
	{
		QVector<QVector3D> temp_v;
		QString temp_s;
		for (int i = 0; i < pos.size(); i++)
		{
			if (pos[i] != ")" && pos[i] != "(")
			{
				temp_s.append(pos[i]);
			}
		}
		QStringList list = temp_s.split(",");
		if (list.size() != 6)
		{
			return QVector<QVector3D>{ {0, 0, 0}, { 0,0,0 }};
		}
		temp_v.append(QVector3D(list[0].toDouble(), list[1].toDouble(), list[2].toDouble()));
		temp_v.append(QVector3D(list[3].toDouble(), list[4].toDouble(), list[5].toDouble()));
			
		return temp_v;
	}
	bool mCommonFunctions::LineIntersect3D(const QVector3D &line1P1, const QVector3D &line1P2, const QVector3D &line2P1, const QVector3D &line2P2, QVector3D &intersectP)
	{
		QVector3D v1 = line1P2 - line1P1;
		QVector3D v2 = line2P2 - line2P1;
		//首先确定两条直线是否平行，利用向量点乘结果是否等于0来判断，等于0垂直，等于1则平行。
		if (QVector3D::dotProduct(v1, v2) == 1)
		{
			// 两线平行
			return false;
		}
		//确定两条直线在一个平面内，否则无论如何也无法相交，这个用向量叉乘来判断，即判断向量CA和向量AB叉乘得到的向量是否垂直于向量CD。
		QVector3D startPointSeg = line2P1 - line1P1;
		QVector3D vecS1 = QVector3D::crossProduct(v1, v2);            // 有向面积1
		QVector3D vecS2 = QVector3D::crossProduct(startPointSeg, v2); // 有向面积2
		float num = QVector3D::dotProduct(startPointSeg, vecS1);
		if (num >= 1E-05f || num <= -1E-05f)
		{
			return false;
		}

		// 有向面积比值，利用点乘是因为结果可能是正数或者负数
		if (qFuzzyIsNull(vecS1.lengthSquared())) {
			return false;
		}
		float num2 = QVector3D::dotProduct(vecS2, vecS1) / vecS1.lengthSquared();
		if (num2 > 1 || num < 0) {
			return false;//num2的大小还可以判断是延长线相交还是线段相交
		}
		intersectP = line1P1 + v1 * num2;
		return true;
	}
	
	QStringList mCommonFunctions::getVariableComponents(const QString& var)
	{
		QStringList compos;
		if (var == QString("Displacement")|| var == QString("Displacement(R)")||
			var==QString("Velocity")||var==QString("Velocity(R)")||
			var == QString("Acceleration") || var == QString("Acceleration(R)")||
			var == QString("Eddy Current Density(J)")|| var == QString("Magnetic Flux Density(B)")||
			var == QString("Lorentz Force(F)")||var==QString("Velocity(E)")||
			var==QString("Temperature Gradient(N)")||var==QString("Heat-flow Density(N)")||
			var==QString("Temperature Gradient(E)")||var==QString("Heat-flow Density(E)")||
			var == QString("Velocity(N)"))
		{
			compos.append(QStringList{ QString("X"),QString("Y"),QString("Z"),QString("Magnitude") });
		}
		else if (var == QString("Nodal Stresses(2D & 3D)")|| var == QString("Element Stresses(2D & 3D)"))
		{
			compos.append(QStringList{ QString("XX"),QString("YY"),QString("ZZ"),QString("YZ"),QString("XZ"),QString("XY"),
				QString("Mises") ,QString("Max.Principal"),QString("Mid.Principal"),QString("Min.Principal"),
				QString("Max.Principal(Abs)"),QString("Max.Shear"), QString("Tresca"),QString("Pressure") });
		}
		else if (var == QString("Beam Stresses(N)")|| var == QString("Beam Stresses(E)"))
		{
			compos.append(QStringList{ QString("Axial"),QString("SAC"),QString("SAD"),QString("SAE"),QString("SAF"),
				QString("SAMAX"),QString("SAMIN"),QString("SBC"),QString("SBD"),QString("SBE"),QString("SBF"),
				QString("SBMIN"),QString("SBMAX") });
		}
		else if (var == QString("Rod Stresses(N)")|| var == QString("Rod Stresses(E)"))
		{
			compos.append(QStringList{ QString("Axial") });
		}
		else if (var == QString("Plane Stresses(N)")|| var == QString("Plane Stresses(E)"))
		{
			compos.append(QStringList{ QString("XX"),QString("YY"),QString("ZZ"),QString("YZ"),QString("XZ"),QString("XY"),
				QString("Mises") ,QString("Max.Principal"),QString("Mid.Principal"),QString("Min.Principal"),
				QString("Max.Principal(Abs)"),QString("Max.Shear"), QString("Tresca"),QString("Pressure") });
		}
		else if (var == QString("Nodal Strains(2D & 3D)")|| var == QString("Element Strains(2D & 3D)"))
		{
			compos.append(QStringList{ QString("XX"),QString("YY"),QString("ZZ"),QString("YZ"),QString("XZ"),QString("XY"),
				QString("Mises") ,QString("Max.Principal"),QString("Mid.Principal"),QString("Min.Principal"),
				QString("Max.Principal(Abs)"),QString("Max.Shear"), QString("Tresca"),QString("Pressure") });
		}
		else if (var == QString("Beam Strains(N)")|| var == QString("Beam Strains(E)"))
		{
			compos.append(QStringList{ QString("Axial"),QString("SAC"),QString("SAD"),QString("SAE"),QString("SAF"),
				QString("SAMAX"),QString("SAMIN"),QString("SBC"),QString("SBD"),QString("SBE"),QString("SBF"),
				QString("SBMIN"),QString("SBMAX") });
		}
		else if (var == QString("Rod Strains(N)")|| var == QString("Rod Strains(E)"))
		{
			compos.append(QStringList{ QString("Axial") });
		}
		else if (var == QString("Plane Strains(N)")|| var == QString("Plane Strains(E)"))
		{
			compos.append(QStringList{ QString("XX"),QString("YY"),QString("ZZ"),QString("YZ"),QString("XZ"),QString("XY"),
				QString("Mises") ,QString("Max.Principal"),QString("Mid.Principal"),QString("Min.Principal"),
				QString("Max.Principal(Abs)"),QString("Max.Shear"), QString("Tresca"),QString("Pressure") });
		}
		return compos;
	}
	
	int mCommonFunctions::getPostRendType(const QString& var)
	{
		if (var == QString("Element Stresses(2D & 3D)") || var == QString("Beam Stresses(E)") || var == QString("Rod Stresses(E)") ||
			var == QString("Plane Stresses(E)") || var == QString("Element Strains(2D & 3D)") || var == QString("Beam Strains(E)") ||
			var == QString("Rod Strains(E)") || var == QString("Plane Strains(E)")||var==QString("Equivalent Plastic Strains(E)")||
			var== QString("Temperature(E)")||var== QString("Velocity(E)")||var== QString("Pressure(E)")||
			var == QString("Temperature Gradient(E)")|| var == QString("Heat-flow Density(E)"))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	void mCommonFunctions::initMatTableWidget(QTableWidget* tw, const QStringList& headers)
	{
		int size = headers.size();
		tw->setColumnCount(size);
		tw->setRowCount(1);
		tw->verticalHeader()->setHidden(false);
		//tw->verticalHeader()->setHighlightSections(false);
	//	tw->resizeColumnsToContents();
		
		tw->setHorizontalHeaderLabels(headers);
		tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
		tw->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);

		for (int i = 0; i < size; i++)
		{
			tw->setItem(0, i, new QTableWidgetItem(""));
			tw->setColumnWidth(i, 80);
		}
		
	}

	void mCommonFunctions::setTableWidgetStyle(QTableWidget* tw, const QStringList& headers)
	{
		if (tw == nullptr)
		{
			return;
		}
		//初始化表格
		tw->setColumnCount(2);
		tw->setEditTriggers(QAbstractItemView::NoEditTriggers);
		tw->verticalHeader()->setHidden(true);
		tw->resizeColumnsToContents();

		//表头
		tw->setHorizontalHeaderLabels(headers);
		tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		tw->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
		//tw->horizontalHeader()->setHighlightSections(false);
		//tw->horizontalHeader()->setStyleSheet("QHeaderView:section{background:rgb(218,218,218);}");
	}

	void mCommonFunctions::getTableWidgetDatas(QTableWidget* tw, QList<QPair<double, double>>& datas)
	{
		if (tw == nullptr || tw->columnCount() != 2)
		{
			return;
		}
		const int rowcount = tw->rowCount();
		for (int i = 0; i < rowcount; ++i)
		{
			QTableWidgetItem* item1 = tw->item(i, 0);
			QTableWidgetItem* item2 = tw->item(i, 1);
			if (item1 != nullptr&&item2 != nullptr)
			{
				QPair<double, double> datapair;
				datapair.first = item1->text().toDouble();
				datapair.second = item2->text().toDouble();
				datas.append(datapair);
			}
			else if (item1 == nullptr&&item2 != nullptr)
			{
				QPair<double, double> datapair;
				datapair.first = 0;
				datapair.second = item2->text().toDouble();
				datas.append(datapair);
			}
			else if (item1 != nullptr&&item2 == nullptr)
			{
				QPair<double,double> datapair;
				datapair.first = item1->text().toDouble();
				datapair.second = 0;
				datas.append(datapair);
			}
		}
	}

	void mCommonFunctions::setTableWidgetDatas(QTableWidget* tw, QList<QPair<double, double>>& datas)
	{
		if (tw == nullptr || tw->columnCount() != 2)
		{
			return;
		}
		clearTableWidgetDatas(tw);

		const int rowcount = datas.size();
		tw->setRowCount(rowcount);
		for (int i = 0; i < rowcount; ++i)
		{
			QPair<double, double> pairval = datas.at(i);
			tw->setItem(i, 0, new QTableWidgetItem(MxFunctions::mNumberToString(pairval.first)));
			tw->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(pairval.second)));
		}
	}

	void mCommonFunctions::clearTableWidgetDatas(QTableWidget* tw)
	{
		const int count1 = tw->rowCount();
		const int count2 = tw->columnCount();
		for (int i = 0; i < count1; ++i)
		{
			for (int j = 0; j < count2; ++j)
			{
				QTableWidgetItem* item = tw->item(i, j);
				if (item != nullptr)
				{
					delete item;
				}
			}				
		}
		tw->setRowCount(0);
	}

	QStringList mCommonFunctions::getTableWidgetText(QTableWidget* tw)
	{
		QStringList itemlist;
		if (tw == nullptr)
		{
			return itemlist;
		}
		int rows = tw->rowCount();
		int cols = tw->columnCount();
		qDebug() << tw->objectName();
		for (int i = 0; i < rows; ++i)
		{
			for (int j = 0; j < cols; ++j)
			{
				QTableWidgetItem* item = tw->item(i, j);
				if(item == nullptr)
					itemlist.append(QString(""));
				else
					itemlist.append(item->text());
			}		
		}
		return itemlist;
	}

	QStringList mCommonFunctions::getAllManagerSelectedName(QTableWidget* tw, QStringList& namelist)
	{
		int rows = tw->rowCount();
	
		for (int i = 0; i < rows; i++)
		{
			QTableWidgetItem* item = tw->item(i, 0);
			if (item == nullptr) continue;
			QString name = item->text().simplified();
			namelist.append(name);
		}
		return namelist;
	}

	QStringList mCommonFunctions::getAllManagerSelectedNameInSecond(QTableWidget* tw, QStringList& namelist)
	{
		int rows = tw->rowCount();

		for (int i = 0; i < rows; i++)
		{
			QTableWidgetItem* item = tw->item(i, 1);
			if (item == nullptr) continue;
			QString name = item->text().simplified();
			namelist.append(name);
		}
		return namelist;
	}

	bool mCommonFunctions::HaveEmptyOrZeroRow(QTableWidget* tw)
	{
		//获取表格行数和列数
		int rows = tw->rowCount();
		int cols = tw->columnCount();

		//遍历
		for (int i = 0; i < rows; i++)
		{
			bool isEmpty = true;
			bool isZero = true;

			for (int j = 0; j < cols; j++)
			{
				QTableWidgetItem* item = tw->item(i, j);
				if (item != nullptr && !item->text().isEmpty())
				{
					//存在数据
					isEmpty = false;
					if (item->text().toDouble() != 0)
					{
						isZero = false;
					}
				}
			}

			//如果任一项为true，直接返回
			if (isEmpty || isZero)
			{			
				return true;
			}
		}
		return false;
	}

	void mCommonFunctions::setWidgetPosByMouse(QWidget * mainWidget, QWidget * popWidget)
	{
		QPoint mousePos = QCursor().pos();//当前鼠标绝对坐标
		QPoint mainPos = mainWidget->mapToGlobal(mainWidget->pos());//获取父窗口的全局坐标
		int mainLeft = mainPos.x();
		int mainTop = mainPos.y();
		int mainRight = mainPos.x() + mainWidget->width();
		int mainBottom = mainPos.y() + mainWidget->height();
		//设置弹窗的初始中心位置在鼠标点击处
		int popLeft = mousePos.x() - popWidget->width() / 2;
		int popRight = mousePos.x() + popWidget->width() / 2;
		int popTop = mousePos.y() - popWidget->height() / 2;
		int popBottom = mousePos.y() + popWidget->height() / 2;
		// 最终要确定的只有左上角的坐标位置: 

		//弹窗左边超出主窗口
		if (popLeft < mainLeft)
		{
			int temp = mainLeft - popLeft;
			popLeft = popLeft + temp;
			popRight = popRight + temp;
		}
		//弹窗上部分超出主窗口上部值
		if (popTop < mainTop)
		{
			int temp = mainTop - popTop;
			popTop = popTop + temp;
			popBottom = popBottom + temp;
		}

		//最终以下方和右边为边界

		//弹窗右侧超出主窗口右侧边界值
		if (popRight > mainRight)
		{
			int temp = popRight - mainRight;
			popLeft = popLeft - temp;
			popRight = popRight - temp;
		}
		//弹窗下侧超出主窗口下侧边界值
		if (popBottom > mainBottom)
		{
			int temp = popBottom - mainBottom;
			popBottom = popBottom - temp;
			popTop = popTop - temp;
		}

			
		popWidget->move(popLeft, popTop);
	}
	QVector3D mCommonFunctions::caculatePlaneRange(float minx, float miny, float minz, float maxx, float maxy, float maxz, QVector3D normal, float ratio)
	{
		QVector3D vertex0{ minx,miny,minz };
		QVector3D vertex1{ maxx,miny,minz };
		QVector3D vertex2{ maxx,maxy,minz };
		QVector3D vertex3{ minx,maxy,minz };
		QVector3D vertex4{ minx,miny,maxz };
		QVector3D vertex5{ maxx,miny,maxz };
		QVector3D vertex6{ maxx,maxy,maxz };
		QVector3D vertex7{ minx,maxy,maxz };
		QVector<QVector3D> vertexs{ vertex0 ,vertex1 ,vertex2,vertex3,vertex4,vertex5, vertex6, vertex7 };

		QVector3D center{ (minx + maxx) / 2.0f,(miny + maxy) / 2.0f, (minz + maxz) / 2.0f };

		float minDistance = 0, maxDistance = 0;
		QVector3D minVertex, maxVertex;
		//求距离
		for (int i = 0; i < 8; i++)
		{
			float distance = vertexs[i].distanceToPlane(center, normal);
			if (distance<minDistance)
			{
				minDistance = distance;
				minVertex = vertexs[i];
			}
			else if (distance > maxDistance)
			{
				maxDistance = distance;
				maxVertex = vertexs[i];
			}
		}

		//求点
		return center + (ratio - 0.5) * (maxDistance - minDistance)*normal.normalized();
	}
	float mCommonFunctions::caculatePlaneRatio(float minx, float miny, float minz, float maxx, float maxy, float maxz, QVector3D normal, QVector3D vertex)
	{
		QVector3D vertex0{ minx,miny,minz };
		QVector3D vertex1{ maxx,miny,minz };
		QVector3D vertex2{ maxx,maxy,minz };
		QVector3D vertex3{ minx,maxy,minz };
		QVector3D vertex4{ minx,miny,maxz };
		QVector3D vertex5{ maxx,miny,maxz };
		QVector3D vertex6{ maxx,maxy,maxz };
		QVector3D vertex7{ minx,maxy,maxz };
		QVector<QVector3D> vertexs{ vertex0 ,vertex1 ,vertex2,vertex3,vertex4,vertex5, vertex6, vertex7 };

		QVector3D center{ (minx + maxx) / 2.0f,(miny + maxy) / 2.0f, (minz + maxz) / 2.0f };

		float minDistance = 0, maxDistance = 0;
		QVector3D minVertex, maxVertex;
		//求距离
		for (int i = 0; i < 8; i++)
		{
			float distance = vertexs[i].distanceToPlane(center, normal);
			if (distance < minDistance)
			{
				minDistance = distance;
				minVertex = vertexs[i];
			}
			else if (distance > maxDistance)
			{
				maxDistance = distance;
				maxVertex = vertexs[i];
			}
		}

		float distance = vertex.distanceToPlane(center, normal);

		return (distance - minDistance) / (maxDistance - minDistance);
	}
	void mCommonFunctions::caculateModelCenterAndRaduis(float minx, float miny, float minz, float maxx, float maxy, float maxz, QVector3D & center, float & radius)
	{
		center = QVector3D((maxx + minx) / 2.0, (maxy+ miny) / 2.0, (maxz + minz) / 2.0);
		radius = sqrt(pow(maxx - minx, 2) + pow(maxy - miny, 2) + pow(maxz - minz, 2));
	}

	QString mCommonFunctions::getParenthesesContent(const QString & text)
	{
		QRegExp exp("(?:\\()(.*)(?:\\))");
		if (text.indexOf(exp) >= 0)
		{
			QStringList list = exp.capturedTexts();
			if (list.size() == 2)
			{
				return list.at(1);
			}
		}
		return text;
	}

	void mCommonFunctions::copyTableToBoard(QTableWidget * tw)
	{
		if (tw == nullptr)
		{
			return;
		}
		QStringList tablelist;
		const int nrow = tw->rowCount();
		const int ncol = tw->columnCount();
		for (int i = 0; i < nrow; ++i)
		{
			QStringList rowlist;
			for (int j = 0; j < ncol; ++j)
			{
				QTableWidgetItem* item = tw->item(i, j);
				if (item != nullptr)
				{
					rowlist.append(item->text());
				}
			}
			tablelist.append(rowlist.join("\t"));
		}
		QApplication::clipboard()->setText(tablelist.join("\n"));

		//输出信息
		if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("成功复制！"));
		}
		else
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("Successfully copy!"));
		}
	}

	void mCommonFunctions::pasteBoardToTable(QTableWidget * tw, bool isone)
	{
		if (tw == nullptr)
		{
			return;
		}
		//获取选中的单元格位置
		int rowi = 0;
		int coli = 0;
		QList<QTableWidgetItem*> items = tw->selectedItems();
		if (items.size() > 0)
		{
			QTableWidgetItem* item = items.at(0);
			if (item != nullptr)
			{
				rowi = tw->row(item);
				coli = tw->column(item);
			}
		}
		//获取表格列数（一般而言，表格的列数不能改变）
		const int tablecol = tw->columnCount();
		//获取剪切板数据
		QString cliptext = QApplication::clipboard()->text();
		//按行分割
		QStringList tablelist = cliptext.split("\n");
		//去除空行
		tablelist.removeAll("");
		//根据选中的单元格位置和剪切板中的数据行数，设置表格的行数
		const int nrow = tablelist.size();
		if (isone)
		{
			tw->setRowCount(1);
		}
		else
		{
			tw->setRowCount(rowi + nrow);
		}
		//从开始按行遍历剪切板的数据
		for (int i = 0; i < nrow; ++i)
		{
			//逐次获取行的数据
			QString rowtext = tablelist.at(i);
			QStringList rowlist = rowtext.split("\t");
			int ncol = rowlist.size();
			//写入表格中
			int datai = 0;
			for (int j = coli; j < tablecol; ++j)
			{
				if (datai >= rowlist.size())
				{
					break;
				}
				QString itemtext = rowlist.at(datai);
				tw->setItem(rowi + i, j, new QTableWidgetItem(itemtext));

				++datai;
			}
		}
	}

	void mCommonFunctions::transformElementStringToInt(const QString& typestr, int vals[4])
	{
		const QString upperstr = typestr.toUpper();
		if (upperstr == QString("MASS"))
		{
			vals[0] = (int)(PostCommon::Element_MASS);
			vals[1] = 1;
			vals[2] = 0;
			vals[3] = 0;
		}
		else if (upperstr == QString("ROTARYI"))
		{
			vals[0] = (int)(PostCommon::Element_ROTARYI);
			vals[1] = 1;
			vals[2] = 0;
			vals[3] = 0;
		}
		else if (upperstr == QString("T3D2"))
		{
			vals[0] = (int)(PostCommon::Element_T3D2);
			vals[1] = 2;
			vals[2] = 0;
			vals[3] = 0;
		}
		else if (upperstr == QString("B31"))
		{
			vals[0] = (int)(PostCommon::Element_B31);
			vals[1] = 2;
			vals[2] = 0;
			vals[3] = 0;
		}
		else if (upperstr == QString("CONN3D2"))
		{
			vals[0] = (int)(PostCommon::Element_CONN3D2);
			vals[1] = 2;
			vals[2] = 0;
			vals[3] = 0;
		}
		else if (upperstr == QString("DASHPOTA"))
		{
			vals[0] = (int)(PostCommon::Element_DashpotA);
			vals[1] = 2;
			vals[2] = 0;
			vals[3] = 0;
		}
		else if (upperstr == QString("SPRINGA"))
		{
			vals[0] = (int)(PostCommon::Element_SpringA);
			vals[1] = 2;
			vals[2] = 0;
			vals[3] = 0;
		}
		else if (upperstr == QString("R3D3"))
		{
			vals[0] = (int)(PostCommon::Element_R3D3);
			vals[1] = 3;
			vals[2] = 1;
			vals[3] = 3;
		}
		else if (upperstr == QString("R3D4"))
		{
			vals[0] = (int)(PostCommon::Element_R3D4);
			vals[1] = 3;
			vals[2] = 1;
			vals[3] = 4;
		}
		else if (upperstr == QString("S3"))
		{
			vals[0] = (int)(PostCommon::Element_S3);
			vals[1] = 3;
			vals[2] = 1;
			vals[3] = 3;
		}
		else if (upperstr == QString("S4"))
		{
			vals[0] = (int)(PostCommon::Element_S4);
			vals[1] = 4;
			vals[2] = 1;
			vals[3] = 4;
		}
		else if (upperstr == QString("S4R"))
		{
			vals[0] = (int)(PostCommon::Element_S4R);
			vals[1] = 4;
			vals[2] = 1;
			vals[3] = 4;
		}
		else if (upperstr == QString("C3D4"))
		{
			vals[0] = (int)(PostCommon::Element_C3D4);
			vals[1] = 4;
			vals[2] = 4;
			vals[3] = 3;
		}
		else if (upperstr == QString("C3D10"))
		{
			vals[0] = (int)(PostCommon::Element_C3D10);
			vals[1] = 10;
			vals[2] = 4;
			vals[3] = 3;
		}
		else if (upperstr == QString("C3D10M"))
		{
			vals[0] = (int)(PostCommon::Element_C3D10M);
			vals[1] = 10;
			vals[2] = 4;
			vals[3] = 3;
		}
		else if (upperstr == QString("C3D6"))
		{
			vals[0] = (int)(PostCommon::Element_C3D6);
			vals[1] = 6;
			vals[2] = 5;
			vals[3] = 4;
		}
		else if (upperstr == QString("C3D8"))
		{
			vals[0] = (int)(PostCommon::Element_C3D8);
			vals[1] = 8;
			vals[2] = 6;
			vals[3] = 4;
		}
		else if (upperstr == QString("C3D8R"))
		{
			vals[0] = (int)(PostCommon::Element_C3D8R);
			vals[1] = 8;
			vals[2] = 6;
			vals[3] = 4;
		}
		else if (upperstr == QString("C3D8I"))
		{
			vals[0] = (int)(PostCommon::Element_C3D8I);
			vals[1] = 8;
			vals[2] = 6;
			vals[3] = 4;
		}
	}

	void mCommonFunctions::transformIntToElementString(int ElementEnum, QString& typestr)
	{
		if (ElementEnum == PostCommon::Element_MASS)
		{
			typestr = "MASS";
		}
		else if (ElementEnum == PostCommon::Element_ROTARYI)
		{
			typestr = "ROTARYI";
		}
		else if (ElementEnum == PostCommon::Element_T3D2)
		{
			typestr = "T3D2";
		}
		else if (ElementEnum == PostCommon::Element_B31)
		{
			typestr = "B31";
		}
		else if (ElementEnum == PostCommon::Element_CONN3D2)
		{
			typestr = "CONN3D2";
		}
		else if (ElementEnum == PostCommon::Element_DashpotA)
		{
			typestr = "DASHPOTA";
		}
		else if (ElementEnum == PostCommon::Element_SpringA)
		{
			typestr = "SPRINGA";
		}
		else if (ElementEnum == PostCommon::Element_R3D3)
		{
			typestr = "R3D3";
		}
		else if (ElementEnum == PostCommon::Element_R3D4)
		{
			typestr = "R3D4";
		}
		else if (ElementEnum == PostCommon::Element_S3)
		{
			typestr = "S3";
		}
		else if (ElementEnum == PostCommon::Element_S4)
		{
			typestr = "S4";
		}
		else if (ElementEnum == PostCommon::Element_S4R)
		{
			typestr = "S4R";
		}
		else if (ElementEnum == PostCommon::Element_C3D4)
		{
			typestr = "C3D4";
		}
		else if (ElementEnum == PostCommon::Element_C3D10)
		{
			typestr = "C3D10";
		}
		else if (ElementEnum == PostCommon::Element_C3D10M)
		{
			typestr = "C3D10M";
		}
		else if (ElementEnum == PostCommon::Element_C3D6)
		{
			typestr = "C3D6";
		}
		else if (ElementEnum == PostCommon::Element_C3D8)
		{
			typestr = "C3D8";
		}
		else if (ElementEnum == PostCommon::Element_C3D8R)
		{
			typestr = "C3D8R";
		}
		else if (ElementEnum == PostCommon::Element_C3D8I)
		{
			typestr = "C3D8I";
		}

	}

	void mCommonFunctions::getMeshFaceInfo(int type, int vals[3])
	{
		switch (type)
		{
			case (int)(PostCommon::Element_R3D3) :
			case (int)(PostCommon::Element_S3) :
				vals[0] = 1;
				vals[1] = 3;
				vals[2] = 3;
				break;
			case (int)(PostCommon::Element_R3D4) :
			case (int)(PostCommon::Element_S4) :
				vals[0] = 1;
				vals[1] = 4;
				vals[2] = 4;
				break;
			case (int)(PostCommon::Element_C3D4) :
			case (int)(PostCommon::Element_C3D10) :
			case (int)(PostCommon::Element_C3D10M) :
				vals[0] = 4;
				vals[1] = 3;
				vals[2] = 12;
				break;
			case (int)(PostCommon::Element_C3D6) :
				vals[0] = 5;
				vals[1] = 4;
				vals[2] = 20;
				break;
			case (int)(PostCommon::Element_C3D8) :
			case (int)(PostCommon::Element_C3D8R) :
			case (int)(PostCommon::Element_C3D8I) :
				vals[0] = 6;
				vals[1] = 4;
				vals[2] = 24;
				break;
				default:
					break;
		}
	}

	int mCommonFunctions::getMeshDimension(int type)
	{
		switch (type)
		{
			case (int)(PostCommon::Element_R3D3) :
			case (int)(PostCommon::Element_S3) :
			case (int)(PostCommon::Element_R3D4) :
			case (int)(PostCommon::Element_S4) :
				return 2;
			case (int)(PostCommon::Element_C3D4) :
			case (int)(PostCommon::Element_C3D10) :
			case (int)(PostCommon::Element_C3D10M) :
			case (int)(PostCommon::Element_C3D6) :
			case (int)(PostCommon::Element_C3D8) :
			case (int)(PostCommon::Element_C3D8R) :
			case (int)(PostCommon::Element_C3D8I) :
				return 3;
			default:
				return 0;
		}
	}

	QString mCommonFunctions::getNonRepetiveName(const QString& name, const QStringList& namelist)
	{
		int i = 1;
		QString newname = name;
		while (namelist.contains(newname))
		{
			newname = QString("%1_%2").arg(name).arg(i);
			i++;
		}
		return newname;
	}

	QString mCommonFunctions::getDataTypeStr(DATATYPE type)
	{
		switch (type)
		{
		case Data_Part:
			return tr("Part");
		case Data_Material:
			return tr("Material");
		case Data_Load:
			return tr("Load");
		case Data_BC:
			return tr("Boundary");
		case Data_Section:
			return tr("Section");
		default:
			return QString();
		}
	}

	QStringList mCommonFunctions::getSortStringList(const QStringList& namelist)
	{
		QStringList sortList = namelist;
		sortList.sort();
		return sortList;
	}

	double mCommonFunctions::calculateTriangleArea(const QVector3D& A, const QVector3D& B, const QVector3D& C)
	{
		QVector3D AB = B - A;
		QVector3D AC = C - A;
		QVector3D cross = QVector3D::crossProduct(AB, AC);
		double aera = cross.length() / 2.0;
		return aera;
	}

	double mCommonFunctions::calculateQuadrilateralArea(const QVector3D& A, const QVector3D& B, const QVector3D& C, const QVector3D& D)
	{
		double aera1 = calculateTriangleArea(A, B, C);
		double aera2 = calculateTriangleArea(A, C, D);
		return aera1 + aera2;
	}

	double mCommonFunctions::getElementAera(QList<QVector3D> Points)
	{
		double aera = 0.0;
		if (Points.size() == 3)
		{
			aera = calculateTriangleArea(Points[0], Points[1], Points[2]);
		}
		else if (Points.size() == 4)
		{
			aera = calculateQuadrilateralArea(Points[0], Points[1], Points[2], Points[3]);
		}
		return aera;
	}

	QStringList mCommonFunctions::getFNodeOutPut(QStringList output)
	{
		QStringList list;
		QStringList frequency{ "U", "UR", "UT", "CF", "RF", "RM" , "RT" , "TF" , "VF" , "COORD" , "CECHG" , "CECUR" , "EPOT" , "POR"
		, "RCHG" , "RECUR" , "NT" , "PHILSM" , "PSILSM" , "V", "VR", "VT", "A", "AR", "AT", "PABS", "TIEADJUST" , "TIEDSTATUS" };


		for (auto it : output)
		{
			if (frequency.contains(it, Qt::CaseInsensitive))
			{
				list.push_back(it);
			}
		}

		return list;

	}

	QStringList mCommonFunctions::getHNodeOutPut(QStringList output)
	{
		QStringList list;
		QStringList Houtput{ "CECHG", "CECUR", "EPOT", "NT", "PHILSM", "PSILSM" , "RCHG" , "RECUR" , "CF1" , "CF2" , "CF3" , "CM1" , "CM2" , "CM3"
		, "CW" , "RF1" , "RF2" , "RF3" , "RM" , "RM1", "RM2", "RM3", "RT", "RWM" , "TF1" , "TF2" , "TF3" , "TM1" , "TM2" , "TM3" , "U1" , "U2"
		, "U3", "UR", "UR1", "UR2", "UR3", "UT", "VF1", "VF2","VF3", "VM1", "VM2", "VM3", "WARP","COOR1", "COOR2", "COOR3", "CVOL", "PCAV" 
		, "V1", "V2", "V3", "VR1", "VR2", "VR3", "VT","A1", "A2", "A3", "AR", "AR1", "AR2", "AR3", "AT","VR" ,"ACTEMP","APCAV" ,"CEFL","CEFLT"
		,"CMASS", "CMF" ,"CMFL" ,"CMFLT","CTEMP", "MINFL", "MINFLT", "TCMASS", "TINFL","CBLARAT", "CLAREA","CSAREA" ,"PABS","POR", "TCSAREA", "TCVOL" };


		for (auto it : output)
		{
			if (Houtput.contains(it, Qt::CaseInsensitive))
			{
				list.push_back(it);
			}
		}

		return list;
	}

	QString mCommonFunctions::resolveBundledPythonExe(QStringList* searchedPaths)
	{
		//部署事实：Conan python 包(3.7.17@mxsim/stable) Windows Release 变体带 python.exe，
		//Debug 变体只有调试版 python_d.exe——依次探测、命中第一个存在的，全缺失返回空串
		const QString pythonDir = QCoreApplication::applicationDirPath() + "/python";
		const QStringList candidates
		{
			pythonDir + "/python.exe",
			pythonDir + "/python_d.exe"
		};
		for (const QString& candidate : candidates)
		{
			if (QFile::exists(candidate))
				return candidate;
			if (searchedPaths != nullptr)
				searchedPaths->append(candidate);
		}
		return QString();
	}


// —— S5 GUI 迁移（门面直接替换批）：G5 对象台账寻址助手实现 ——
int firstSceneObjectId(mxr::REBUILD_PostRender* facade, mxr::SceneObjectType type)
{
	if (facade == nullptr)
		return -1;
	const auto objects = facade->listObjects();
	for (const auto& rec : objects)
	{
		if (rec.type == type)
			return rec.id;
	}
	return -1;
}

int ensureSceneObjectId(mxr::REBUILD_PostRender* facade,
	const mxr::ISceneObjectParams& createParams)
{
	const int existing = firstSceneObjectId(facade, createParams.type);   // D9 载体：类型取 params.type
	if (existing >= 0)
		return existing;
	return facade->addObject(createParams);
}

void updateRigidWallOperate(mxr::REBUILD_PostRender* facade, int operate,
	const std::set<QString>& partNames)
{
	if (facade == nullptr)
		return;
	mxr::RigidWallParams params;
	params.operate = operate;
	// Qt 5.13.2 无 QVector 迭代器对构造——集合逐个转填
	for (const auto& name : partNames)
		params.partNames.append(name);
	facade->updateObject(ensureSceneObjectId(facade, params), params);
}

void removeAllSceneObjects(mxr::REBUILD_PostRender* facade, mxr::SceneObjectType type)
{
	if (facade == nullptr)
		return;
	// 循环摘除：removeObject 内部对该型承担家族渲染删除+清台账，摘空即止
	while (true)
	{
		const int id = firstSceneObjectId(facade, type);
		if (id < 0)
			break;
		if (!facade->removeObject(id))
			break;
	}
}

int cuttingPlaneIdByIndex(mxr::REBUILD_PostRender* facade, int externalIndex)
{
	if (facade == nullptr)
		return -1;
	const auto records = facade->listObjects();
	for (const auto& rec : records)
	{
		if (rec.type == mxr::SceneObjectType::CuttingPlane && rec.externalIndex == externalIndex)
			return rec.id;
	}
	return -1;
}

// 单实例型 ensure-or-update 一步式（unify-addobject-arch）：台账无该型→addObject 建账，
// 有→updateObject 改参（新六族单实例型共用；多条目型 MeasureAnnotation 按 id 逐条寻址，C3）
void ensureUpdateSceneObject(mxr::REBUILD_PostRender* facade,
	const mxr::ISceneObjectParams& params)
{
	if (facade == nullptr)
		return;
	const int existing = firstSceneObjectId(facade, params.type);
	if (existing >= 0)
		facade->updateObject(existing, params);
	else
		facade->addObject(params);
}
}   // namespace PostCommon
