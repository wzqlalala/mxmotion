#pragma once
//解决中文乱码
#pragma execution_character_SectionShape("utf-8")

#include "MXSectionShapeData_global.h"
#include <map>
#include <QString>
#include <QMap>
#include <QList>

#include "mSectionShapeBase.h"
#include <QObject>   // 必须


namespace MechSectionShapeData
{
class MXSECTIONSHAPEDATA_EXPORT mSectionShapeManager : public QObject
{
     Q_OBJECT
public:
    ///* 单例入口 */
    //static mSectionShapeManager* getInstance();

    /* 通过id获取数据 */
    mSectionShapeBase* getSectionShapeData(int id);

    /*
    * 通过名字获取数据
    */
    mSectionShapeBase* getSectionShapeData(const QString& name);

    /*
    * 通过id获取名称
    */
    QString getSectionShapeNameByID(int id);

    /*
    * 通过名称获取id
    */
    int getIDByPropertyShapeName(QString name);
    
    /*
    * 添加函数数据
    */
    void AddSectionShapeData(mSectionShapeBase* sec);

    /*
        * 判断分析属性名是否存在
        */
    bool ExistSectionShapeName(const QString& name);

    /*
    * 重命名属性
    */
    void resetName(const QString& oldName, const QString& newName);

    /*
    * 拷贝属性
    */
    int copySectionShapeData(const int& id);

    /*
    * 删除所有属性
    */
    void deleteAllSectionShape();

    /*
    * 删除属性
    */
    void deleteSectionShape(const int& id);

    /*
    * 获取所有属性ID
    */
    std::vector<int> getAllSectionShapeID();

    /*
    * 获取所有属性名称
    */
    QStringList getAllSectionShapeNames();

    /*
    * 获取所有属性名称根据类型
    */
    QStringList getAllSectionShapeNamesByTypes(MxFunctions::SecType type);

    /*
     * 返回创建的新ID
     */
    int createID();
    int getMaxID();
    int getCurrentID();
    void setCurrentID(int currentID);

    mSectionShapeManager();                                     // 构造私有
    ~mSectionShapeManager();

    friend QDataStream& operator>>(QDataStream& in, mSectionShapeManager& obj);
    friend QDataStream& InStreammSectionShapeManagerV3(QDataStream& in, mSectionShapeManager& mgr);
    friend QDataStream& operator<<(QDataStream& out, mSectionShapeManager& obj);
signals:
    


private:
    Q_DISABLE_COPY(mSectionShapeManager)

    std::map<int, mSectionShapeBase*> _sectionShapeData;
    int _maxID;
    int _currentID;

    ///* 下面两行用于单例 */
    //static mSectionShapeManager* _inst;
    

};

} // namespace MechSectionShapeData