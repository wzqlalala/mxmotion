#pragma once
#include<QString>
#include<QMenu>
#include<QObject>
#include<QTableWidget>
class mRecentDataBase :public QObject
{
	Q_OBJECT
public:
	mRecentDataBase();
	void initRecentDb();
	void addRecentPath(const QString& filePath);
	void fillRecentMenu(QMenu* menu);
	void fillRecentTable(QTableWidget* tab);
};

