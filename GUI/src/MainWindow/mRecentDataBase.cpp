#include "mRecentDataBase.h"
#include<QSqlDatabase>
#include<QStandardPaths>
#include<QSqlQuery>
#include<QDateTime>
#include<QVariant>
#include<QAction>
#include<QFileInfo>
#include<QString>
#include<QDir>
#include"mModelData.h"
#include"mViewportModelManager.h"
#include"mViewportModel.h"
#include"mMenuWidget.h"
#include"mIOFileManager.h"
#pragma execution_character_set("utf-8")
mRecentDataBase::mRecentDataBase()
{
}
void mRecentDataBase::initRecentDb()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "recent");
    

    db.setDatabaseName(QDir::currentPath()
        + "/recent.db");
    if (!db.open()) return;

    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS recent("
        "path TEXT PRIMARY KEY, "
        "time INTEGER)");
}
void mRecentDataBase::addRecentPath(const QString& filePath)
{
    QSqlDatabase db = QSqlDatabase::database("recent");
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO recent(path,time) VALUES(?,?)");
    q.addBindValue(filePath);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    q.exec();

    // 可选：只保留最近 N 条
    q.exec("DELETE FROM recent WHERE path NOT IN ("
        "SELECT path FROM recent ORDER BY time DESC LIMIT 15)");
}
void mRecentDataBase::fillRecentMenu(QMenu* menu)
{
    menu->clear();
    QSqlDatabase db = QSqlDatabase::database("recent");
    QSqlQuery q("SELECT path FROM recent ORDER BY time DESC LIMIT 5", db);
    while (q.next()) {
        QString path = q.value(0).toString();
        QAction* act = menu->addAction(path);
        connect(act, &QAction::triggered, [=]() {
            //导入几何模型
        //mMenuWidget::slot_importGeoFile();           // 你的打开函数
        //默认目录


            QString fileName = path;
            //若为计算文件
            QString suffix = QFileInfo(fileName).suffix();
            QStringList meshSuffixs{ "obj","ply","off","stl" };
            if (!fileName.isEmpty()) {
                if (suffix == "mx")
                {
                    MechIOFile::mIOFileManager::getInstance()->slot_importSolveFile(fileName);
                }
                else if (meshSuffixs.contains(suffix)) {
                    MechIOFile::mIOFileManager::getInstance()->readMeshIOFile(fileName);
                }
                else
                {
                    MechMainWindow::mMenuWidget::importGeometryViaServer(this, fileName);
                }

            }
            });
        

        act->setData(path);           // 存完整路径
    }
    if (menu->isEmpty()) menu->addAction(QString("无记录"))->setEnabled(false);
}
void mRecentDataBase::fillRecentTable(QTableWidget* tab)
{
    QSqlDatabase db = QSqlDatabase::database("recent");
    QSqlQuery q("SELECT path, time FROM recent ORDER BY time DESC", db);

    int rows = 0;
    while (q.next()) ++rows;          // 先计数
    tab->setRowCount(rows);
    tab->setColumnCount(2);
    tab->setHorizontalHeaderLabels({ "名称", "路径" });

    q.first();                        // 回到第一条
    int r = 0;
    QString filePath = q.value(0).toString();
    QString name = QFileInfo(filePath).fileName();
    tab->setItem(r, 1, new QTableWidgetItem(q.value(0).toString()));
    tab->setItem(r, 0, new QTableWidgetItem(name));
    ++r;
    while( q.next()) {  // 再遍历一次
        QString filePath = q.value(0).toString();
        QString name = QFileInfo(filePath).fileName();
        tab->setItem(r, 1, new QTableWidgetItem(q.value(0).toString()));
        tab->setItem(r, 0, new QTableWidgetItem(name));
        ++r;
    }
}