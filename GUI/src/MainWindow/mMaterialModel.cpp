#include "mMaterialModel.h"

//QT
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QDebug>
#include <qxmlstream.h>
#include <QXmlStreamReader>
//MxFunctions
#include <mGlobalSignals.h>
#include "mMaterialBase.h"
#include "mOtherFunction.h"

namespace MechMainWindow
{
	mMaterialModel::mMaterialModel(QObject* parent)
		: QStandardItemModel(parent)
	{
        setHorizontalHeaderLabels({ "ID", "Name" });
        loadMaterials();


 /*       QString filepath = QCoreApplication::applicationDirPath() + "/materials/General_Materials.xml";
        parseMatXML(filepath);*/
    }

	mMaterialModel::~mMaterialModel()
	{}

    void mMaterialModel::registerMeta()
    {
        qRegisterMetaType<MechMainWindow::MaterialScalar>("MechMainWindow::MaterialScalar");
        qRegisterMetaType<MechMainWindow::MaterialCurve>("MechMainWindow::MaterialCurve");
        MechMaterialData::mMaterialBase::registerMeta();
    }

    Material mMaterialModel::getMaterialById(int id)
    {
        //清空材料属性
        m_Materials[id].props.clear();

        //加载
		QString filepath = QCoreApplication::applicationDirPath() + "/materials/" + m_Materials[id].name + ".mat";
        loadMaterialByFile(id, filepath);

        return m_Materials[id];
    }

    QSet<QString> mMaterialModel::getAllMaterialTypes()
    {
        return m_MatTypes;
    }

    bool mMaterialModel::findMateiral(const QString& name)
    {
        if (m_MatNames.find(name) != m_MatNames.end())
        {
            return true;
        }
        return false;
    }

    bool mMaterialModel::addMaterial(Material& mat)
    {
        if (m_MatNames.contains(mat.name))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("已有名称相同的材料"));
            return false;
        }
        //更新id
        mat.id = m_Materials.size()+1;
        //添加进内存
        m_Materials.append(mat);
        m_MatTypes.insert(mat.category);
        m_MatNames.insert(mat.name);
        //添加进模型
        removeRows(0, rowCount());
        loadIntoModel();
        //添加单独材料文件
        //添加进汇总文件
        return storeMaterialMat(mat)&& addMaterialIntoSummary(mat);
    }

    bool mMaterialModel::updateMaterial(Material& mat)
    {
        return writeMaterialMat(mat);
    }

    void mMaterialModel::removeMaterial(const QSet<int>& idxs)
    {
        //从内存中删除
        QVector<Material> tmpmats = m_Materials;
        m_Materials.clear();
        int cnt = 0;
        for (auto mat : tmpmats)
        {
            if (idxs.contains(mat.id))
            {
                //删除单独材料文件
                deleteMaterialMat(mat);
                //从汇总文件删除
                deleteMaterialFromSummary(mat);
                m_MatNames.remove(mat.name);
                continue;
            }
            mat.id = ++cnt;
            m_Materials.append(mat);
        }
       
        //重新加载内存模型
        removeRows(0, rowCount());
        loadIntoModel();
    }

    void mMaterialModel::copyMaterial(const int& id)
    {
        Material mat = getMaterialById(id);
        QString matname = mat.name;
        int ID = 1;
        while (findMateiral(matname))
        {
            matname = mat.name + "_" + MxFunctions::mNumberToString(ID);
            ++ID;
        }
        Material newmat = mat;
        newmat.name = matname;
        addMaterial(newmat);
    }

    void mMaterialModel::renameMaterial(const QString& oldname, const QString& newname)
    {
        QString oldpath = QCoreApplication::applicationDirPath() + "/materials/" + oldname + ".mat";
        QString newpath = QCoreApplication::applicationDirPath() + "/materials/" + newname + ".mat";
        QFile file(oldpath);
        if (file.rename(newpath))
        {
            renameMaterialFromSummary(oldname, newname);
        }
    }

    void mMaterialModel::compare()
    {  
        //比对材料名称和.mat
        int cnt = 0;
        for (auto mat : m_Materials)
        {
            cnt++;
            QString filepath = QCoreApplication::applicationDirPath() + "/materials/" + mat.name + ".mat";
            QFile file(filepath);
            if (!file.exists())
            {
                continue;
            }
        }
    }

    void mMaterialModel::loadMaterials()
    {
        clear();
        setHorizontalHeaderLabels({ "ID", "Name" });
        QString filepath = QCoreApplication::applicationDirPath() + "/materials/materials.txt";
        QFile f(filepath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }
        m_Materials.clear();
        static QRegularExpression sep(R"(\s*\|\s*)");
        QTextStream ts(&f);
        ts.setCodec("UTF-8");
        int i = 0;
        while (!ts.atEnd())
        {
            QString line = ts.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#'))
            {
                continue;
            }
            QStringList parts = line.split(sep);
            if (parts.size() < 2)
            {
                continue;
            }
            Material mat;
            mat.id = ++i;
            mat.name = parts[0].trimmed();
            mat.category = parts[1].section('-', 0, 0).trimmed();
            m_MatNames.insert(mat.name);
            m_MatTypes.insert(mat.category);
            m_Materials.append(mat);
        }
        //加载所有材料
        //materialsFromCsvToFile(QCoreApplication::applicationDirPath() + "/materials/mats-props.csv");
        //compare();
        loadIntoModel();
    }

    void mMaterialModel::loadIntoModel()
    {
        // 先确保顶层父节点都存在
        m_categoryNodes.clear();
        QList<QString> typeList = m_MatTypes.toList();
        qSort(typeList);
        for (const QString& cat : typeList)
        {
            if (!m_categoryNodes.contains(cat))
            {
                QStandardItem* parent = new QStandardItem(cat);
                parent->setEditable(false);
                appendRow(parent);                 // 顶层
                m_categoryNodes[cat] = parent;     // 缓存，避免重复
            }
        }

        for (const Material& m : m_Materials)
        {
            /* QStandardItem* idItem = new QStandardItem(MxFunctions::mNumberToString(m.id));
             idItem->setData(m.id, Qt::UserRole);
             QStandardItem* nameItem = new QStandardItem(m.name);
             appendRow({ idItem, nameItem });*/
             // 为当前材料找到父节点
            QStandardItem* parentNode = m_categoryNodes.value(m.category);
            if (!parentNode)
            {   // 万一遇到新类型，实时补
                parentNode = new QStandardItem(m.category);
                parentNode->setEditable(false);
                appendRow(parentNode);
                m_categoryNodes[m.category] = parentNode;
            }

            // 创建子节点
            QStandardItem* nameItem = new QStandardItem(m.name);
            nameItem->setEditable(false);
            nameItem->setData(m.id, Qt::UserRole);
            parentNode->appendRow(nameItem);   // 挂在父节点下
        }
    }

    void mMaterialModel::loadMaterialByFile(const int& id, const QString& filepath)
    {
        QFile f(filepath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }
        QTextStream ts(&f);
        qint64 posBeforeRead = ts.pos();
        QString curCategory;

        while (!ts.atEnd()) {
            posBeforeRead = ts.pos();
            QString line = ts.readLine().trimmed();
            if (line.isEmpty())
            {
                continue;
            }
            //匹配属性类型
            if (line.startsWith("**"))
            {
                curCategory = line.mid(2).trimmed();
                continue;
            }
            //匹配属性
            if (line.startsWith('*'))
            {
                MaterialProperty prop;
                prop.category = curCategory;
                line = line.mid(1).trimmed();
                QStringList parts = line.split(',', QString::SkipEmptyParts);
                if (parts.size() == 2)//常量 
                {
                    QString propname = parts[0].trimmed();
                    QString unit = parts[1].section('=', 1).trimmed();
                    double data = ts.readLine().trimmed().toDouble();
                    prop.valuetype = 0;
                    prop.value = QVariant::fromValue<MaterialScalar>({ data, unit });
                    m_Materials[id].props[propname] = prop;
                }
                else if (parts.size() == 3)//变量
                {
                    QString propname = parts[0].trimmed();
                    QString unitx = parts[1].section('=', 1).trimmed();
                    QString unity = parts[2].section('=', 1).trimmed();
                    MaterialCurve curve;
                    curve.xUnit = unitx;
                    curve.yUnit = unity;

                    while (!ts.atEnd())
                    {
                        posBeforeRead = ts.pos();
                        QString line = ts.readLine().trimmed();
                        if (line.isEmpty())
                        {
                            break;
                        }
                        //曲线名
                        if (line.startsWith('"') && line.endsWith('"'))
                        {
                            MaterialSingleCurve cv;
                            bool iscurve = true;
                            while (!ts.atEnd())
                            {
                                posBeforeRead = ts.pos();
                                QString ptLine = ts.readLine().trimmed();
                                if (!ptLine.startsWith('('))
                                {
                                    curve.curves[line.mid(1, line.size() - 2)] = cv;
                                    iscurve = false;
                                    break;
                                }
                                QString core = ptLine.mid(1, ptLine.size() - 2);
                                QStringList pts = core.split(',', QString::SkipEmptyParts);
                                if (pts.size() == 2)
                                {
                                    cv.x << pts[0].toDouble();
                                    cv.y << pts[1].toDouble();
                                }
                            }
                            if (!iscurve)
                            {
                                ts.seek(posBeforeRead);
                                continue;
                            }
                            curve.curves[line] = cv;
                        }
                        else
                        {
                            ts.seek(posBeforeRead);
                            break;
                        }
                    }
                    prop.valuetype = 1;
                    prop.value = QVariant::fromValue<MaterialCurve>(curve);
                    m_Materials[id].props[propname] = prop;
                }
            }
            else
            {
                continue;
            }
        }
    }

    void mMaterialModel::materialsFromCsvToFile(const QString& csvpath)
    {
        QFile f(csvpath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }

        QTextStream ts(&f);
        /* 读表头 */
        QRegularExpression re(R"(^\s*(.*?)\s*\(\s*(.*)\s*\)\s*$)");
        QString headerLine = ts.readLine();
        if (headerLine.isNull())
        {
            return;
        }
        QStringList header = headerLine.split(',');
        QStringList propnames;
        QStringList propunits;
        for (int i = 1;i<header.size();++i)
        {
            int left = header[i].indexOf('(');
            int right = header[i].lastIndexOf(')');
            propnames.append(header[i].left(left).trimmed());
            propunits.append(header[i].mid(left + 1, right - left - 1).trimmed());
            
        }
        propunits[1] = "";
        /* 逐行解析 */
        const QRegularExpression csvRe(
            R"(("(?:[^"]|"")*"|[^,]*)(?:,|$))");     // 一个字段 + 逗号或行尾
       
        int cnt = 0;
        while (!ts.atEnd()) {
            cnt++;
            QString line = ts.readLine().trimmed();
            if (line.isEmpty())
            {
                continue;
            }
            QStringList cells;
            QRegularExpressionMatchIterator it = csvRe.globalMatch(line);
            while (it.hasNext()) 
            {
                QRegularExpressionMatch m = it.next();
                QString cell = m.captured(1);
                if (cell.startsWith('"') && cell.endsWith('"')) {
                    cell = cell.mid(1, cell.size() - 2);   // 去掉外层引号
                    cell.replace("\"\"", "\"");            // 把 "" 还原成 "
                }
                cells << cell;
            }
            if (cells.size() == 0)
            {
                continue;
            }
            Material mat;
            //材料名

            QString matname = cells.first().trimmed();
            mat.name = matname;
            mat.category = "";
            for (int i = 1; i < 10; ++i)
            {
                MaterialProperty prop;
                if (i == 3)
                {
                    //prop.category = CommonProperty;
                }
                else if (i >= 7)
                {
                    //prop.category = ThermalProperty;
                }
                else
                {
                    //prop.category = StructuralProperty;
                }
                prop.valuetype = 0;
                prop.value = QVariant::fromValue<MaterialScalar>({ cells[i].trimmed().toDouble(),  propunits[i-1]});
                mat.props[propnames[i-1]] = prop;
            }

            storeMaterialMat(mat);
        }
    }

    QHash<QString, MaterialProperty> mMaterialModel::getProperty(
        const bool& issingle,
        const QHash<QString, QPair<QString, QString>>& id2prop, 
        const QStringList& proplines)
    {
        QStringList units = {
            "Density",           // 通用（密度）
            "Stress",            // 结构（杨氏模量等）
            "Thermal Conductivity", // 传热
            "Electrical Resistivity", // 导电
            "Specific Heat Capacity", // 传热
            "InvTemp1",          // 传热
            "Temperature"        // 传热
        };
        QStringList units_act = {
           "kg/m^3",           // 通用（密度）
           "N/m^2",            // 结构（杨氏模量等）
           "W/(m.k)", // 传热
           "Ω·m", // 导电
           "J/(kg.K)", // 传热
           "/Kelven",          // 传热
           "K"        // 传热
        };
        QStringList cate = {
            "通用",
            "结构", 
            "传热",
            "导电",
            "传热",
            "传热",
            "传热",
        };
        QHash<QString, QString>unit2category;
        QHash<QString, QString>unit2unitact;
		for (int i = 0; i < 7; ++i)
        {
            unit2unitact[units[i]] = units_act[i];
            unit2category[units_act[i]] = cate[i];
        }
        QHash<QString, MaterialProperty> props;
        if (issingle)
        {
            for (int i = 0; i < proplines.size(); ++i)
            {
                if (QRegularExpression(R"(\bformat\s*=\s*"float")").match(proplines[i]).hasMatch())
                {
                    int idx = proplines[i].indexOf("parameter=\"", 0, Qt::CaseInsensitive);
                    int start = idx + 11;              
                    int end = proplines[i].indexOf('"', start);
                    QString propid = proplines[i].mid(start, end - start);
                    QString propname = id2prop[propid].first;
                    QString unit = unit2unitact[id2prop[propid].second];
                    if (propid == "pa40")
                    {
                        unit = "";
                    }
                    ++i;
                    MaterialScalar scalar;
                    QString val;
                    QRegularExpression re("<Data>(\\d+)</Data>");
                    QRegularExpressionMatch m = re.match(proplines[i]);
                    if (m.hasMatch())
                        val = m.captured(1);
                    scalar.value = val.toDouble();
                    scalar.unit = unit;

                    MaterialProperty prop;
                    prop.value = QVariant::fromValue<MaterialScalar>(scalar);
                    prop.valuetype = 0;
                    prop.category = unit2category[unit];
                    if (propname == "相对磁导率")
                    {
                        prop.category = "导电";
                    }
                    else if (propname == "泊松比")
                    {
                        prop.category = "结构";
                    }

                    props[propname] = prop;
                }
            }
        }
        else
        {
            QString xname;
            QString xunit;
            QVector<QPair<QString, QVector<double>>>X;
            QString yunit;
            QString yname;
            QVector<QPair<QString, QVector<double>>>Y;
            for (int i = 0; i < proplines.size(); ++i)
            {
                if (QRegularExpression(R"(\bformat\s*=\s*"float")").match(proplines[i]).hasMatch())
                {
                    QVector<double>data;
                    //本行取属性名，单位
                    int idx = proplines[i].indexOf("parameter=\"", 0, Qt::CaseInsensitive);
                    int start = idx + 11;
                    int end = proplines[i].indexOf('"', start);
                    QString propid = proplines[i].mid(start, end - start);
                    QString propname = id2prop[propid].first;
                    QString unit = unit2unitact[id2prop[propid].second];
                    if (propid == "pa40")
                    {
                        unit = "";
                    }
                    //下一行取数据
                    ++i;
                    QRegularExpression re("<Data>([^<]+)</Data>");
                    QRegularExpressionMatch m = re.match(proplines[i]);
                    if (m.hasMatch()) {
                        for (const QString& s : m.captured(1).split(','))
                            data.append(s.toDouble());
                    }
                    //第三行确认是InDependent还是Dependent
                    ++i;
                    if (proplines[i].contains("Dependent"))
                    {
                        yname = propname;
                        yunit = unit;
                        Y.append({ yname,data });
                    }
                    else
                    {
                        xname = propname;
                        xunit = unit;
                        X.append({ xname, data });
                        int xlen = X.size();
                        int ylen = Y.size();
                        if (xlen > ylen)
                        {
                            for (int i = ylen; i < xlen; ++i)
                            {
                                Y.append(X[xlen - 1]);
                            }
                        }
						else if (xlen < ylen)
                        {
                            for (int i = xlen; i < ylen; ++i)
                            {
                                X.append(X[xlen - 1]);
                            }
                        }
                    }
                }
            }

            for (int i = 0; i < X.size(); ++i)
            {
                MaterialCurve curves;
                curves.xUnit = xunit;
                curves.yUnit = yunit;
                MaterialSingleCurve curve;
                curve.x = X[i].second;
                curve.y = Y[i].second;
                curves.curves[Y[i].first] = curve;
                curves.xName = X[i].first;
                MaterialProperty prop;
                prop.value = QVariant::fromValue<MaterialCurve>(curves);
                prop.valuetype = 1;
                prop.category = unit2category[yunit];
                props[Y[i].first] = prop;
            }
        }

        return props;
    }

    void mMaterialModel::getId2Prop(const QStringList& paramdetails,
        QString& unit,
        QString& propname)
    {
        propname.clear();
        unit.clear();

        bool gotName = false;      // 已经抓到属性名
        static const QRegularExpression reName("<Name>([^<]+)</Name>");
        static const QRegularExpression reUnitsName("<Units\\s+name\\s*=\\s*\"([^\"]*)\"");

            for (const QString& line : paramdetails) {
                // 1. 抓第一个 Name
                if (!gotName) {
                    QRegularExpressionMatch m = reName.match(line);
                    if (m.hasMatch()) {
                        propname = m.captured(1).trimmed();
                        gotName = true;
                        continue;
                    }
                }

                // 2. 抓 Units 的 name 属性（只要出现一次）
                if (unit.isEmpty()) {
                    QRegularExpressionMatch m = reUnitsName.match(line);
                    if (m.hasMatch()) {
                        unit = m.captured(1).trimmed();
                        break;
                    }
                }
            }
    }

    void mMaterialModel::parseMatXML(const QString& xmlFilePath)
    {
        QFile file(xmlFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }
        QStringList matcate = {
          "Alloy",
          "Concrete",
          "Plastics",
          "Metalloid",
          "Electronics",
          "Gas"
        };
        QStringList matcate_chinese = {
            "金属",
            "陶瓷",
            "塑料",
            "类金属",
            "电子材料",
            "气体"
        };
        QHash<QString, QString>mat2chinese;
        for (int i = 0; i < 6; ++i)
        {
            mat2chinese[matcate[i]] = matcate_chinese[i];
        }
        QStringList lines;
        QTextStream ts(&file);
        ts.setCodec("UTF-8");
        while (!ts.atEnd())
            lines.append(ts.readLine());
        file.close();

        QVector<Material>mats;
        QHash<QString, QPair<QString, QString>>id2prop;//属性id，属性名称，属性单位
        Material currentMaterial;
        QRegularExpression nameRegex(R"(<Name>([^<]*)</Name>)");
        QRegularExpressionMatch match;


        for (int i = 0; i < lines.size(); ++i)
        {
            QString trimmedLine = lines[i].trimmed();
            if (trimmedLine.contains("<Metadata>"))
            {
                ++i;
                while (!lines[i].contains("</Metadata>"))
                {
                    if (lines[i].contains("ParameterDetails"))
                    {
                        //属性id
                        int start = lines[i].indexOf('"');
                        int end = lines[i].indexOf('"', start + 1);
                        QString propid = lines[i].mid(start + 1, end - start - 1);

                        QStringList paramdetails;
                        while (!lines[i].contains("</ParameterDetails>"))
                        {
                            paramdetails.append(lines[i]);
                            ++i;
                        }
                        QString propname = "";
                        QString unit = "";
                        getId2Prop(paramdetails, unit, propname);
                        id2prop[propid] = { propname,unit };
                    }
                    ++i;
                }
            }
        }

        for (int i = 0; i < lines.size(); ++i)
        {
            QString trimmedLine = lines[i].trimmed();
            if (trimmedLine.contains("<Material>"))//遇到材料
            {
                //获取材料名称
                //越过下一行
                i += 2;
                if (i >= lines.size())//超过文件末尾
                {
                    continue;
                }
                QString materialNameLine = lines[i].trimmed();
                if (!materialNameLine.contains("<Name>"))//没有名称
                {
                    continue;
                }
                match = nameRegex.match(materialNameLine);
                if (!match.hasMatch())//名称匹配失败
                {
                    continue;
                }
                currentMaterial.name = match.captured(1);
                //寻找材料类别
                while (!lines[i].contains("<Class>"))
                {
                    ++i;
                }
                i += 1;
                QString materialCategoryLine = lines[i];
                if (!materialCategoryLine.contains("<Name>"))
                {
                    continue;
                }
                match = nameRegex.match(materialCategoryLine);
                if (!match.hasMatch())//名称匹配失败
                {
                    continue;
                }
                currentMaterial.category = match.captured(1);
                if (mat2chinese.find(currentMaterial.category) != mat2chinese.end())
                {
                    currentMaterial.category = mat2chinese[currentMaterial.category];
                }
                ++i;
                if (currentMaterial.name == "FR-4")
                {
                    int a = 0;
                }

                //收集属性
                while (!lines[i].contains("</Material>"))//直到遇到材料尾
                {
                    if (lines[i].contains("<MechPropertyData") && !QRegularExpression(R"(\bproperty\s*=\s*"pr0")").match(lines[i]).hasMatch() && !QRegularExpression(R"(\bproperty\s*=\s*"pr5")").match(lines[i]).hasMatch())
                    {
                        bool issingle = true;
                        QStringList proplines;
                        while (!lines[i].contains("<ParameterValue"))
                        {
                            //在遇到ParameterValue前找Field Variable Compatible确认是否是单值
                            if (lines[i].contains("Field Variable Compatible"))
                            {
                                issingle = false;
                            }
                            ++i;
                        }
                        while (!lines[i].contains("</MechPropertyData>"))
                        {
                            proplines.append(lines[i++]);
                        }
                        auto props = getProperty(issingle, id2prop, proplines);
						for (auto it = props.begin(); it != props.end(); it++)
                        {
                            currentMaterial.props[it.key()] = it.value();
                        }
                    }
                    ++i;
                }

                mats.append(currentMaterial);
                currentMaterial.props.clear();
            }
        }
   
        for (auto mat : mats)
        {
            storeMaterialMat(mat);
            addMaterialIntoSummary(mat);
        }
    }

    bool mMaterialModel::storeMaterialMat(const Material& mat)
    {
        QString filepath = QCoreApplication::applicationDirPath() + "/materials/" + mat.name + ".mat";
        QFile file(filepath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            return false;
        }
        QTextStream out(&file);
        out << QString("**通用") + "\n";
        for (auto it = mat.props.constBegin(); it != mat.props.constEnd(); it++)
        {
            if (it.value().category != "通用")
            {
                continue;
            }
            out << propToQString(it.value(), it.key());
        }
        out << QString("**结构") + "\n";
        for (auto it = mat.props.constBegin(); it != mat.props.constEnd(); it++)
        {
            if (it.value().category != "结构")
            {
                continue;
            }
            out << propToQString(it.value(), it.key());
        }
        out << QString("**传热") + "\n";
        for (auto it = mat.props.constBegin(); it != mat.props.constEnd(); it++)
        {
            if (it.value().category != "传热")
            {
                continue;
            }
            out << propToQString(it.value(), it.key());
        }
        out << QString("**导电") + "\n";
        for (auto it = mat.props.constBegin(); it != mat.props.constEnd(); it++)
        {
            if (it.value().category != "导电")
            {
                continue;
            }
            out << propToQString(it.value(), it.key());
        }
        file.close();
        return true;
    }

    bool mMaterialModel::writeMaterialMat(const Material& mat)
    {
        QString filepath = QCoreApplication::applicationDirPath() + "/materials/" + mat.name + ".mat";
        QFile file(filepath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            return false;
        }

        QTextStream out(&file);
        out << QString("**通用") + "\n";
        for (auto it = mat.props.constBegin(); it != mat.props.constEnd(); it++)
        {
            if (it.value().category != "通用")
            {
                continue;
            }
            out << propToQString(it.value(), it.key());
        }
        out << QString("**结构") + "\n";
        for (auto it = mat.props.constBegin(); it != mat.props.constEnd(); it++)
        {
            if (it.value().category != "结构")
            {
                continue;
            }
            out << propToQString(it.value(), it.key());
        }
        out << QString("**传热") + "\n";
        for (auto it = mat.props.constBegin(); it != mat.props.constEnd(); it++)
        {
            if (it.value().category != "传热")
            {
                continue;
            }
            out << propToQString(it.value(), it.key());
        }
        out << QString("**导电") + "\n";
        for (auto it = mat.props.constBegin(); it != mat.props.constEnd(); it++)
        {
            if (it.value().category != "导电")
            {
                continue;
            }
            out << propToQString(it.value(), it.key());
        }
        file.close();
        return true;
    }

    QString mMaterialModel::propToQString(const MaterialProperty& prop, const QString& propname)
    {
        QString res = "";
        if (prop.valuetype == 0)
        {
            MaterialScalar scalar = prop.value.value<MaterialScalar>();
            res = res + "*" + propname + QString(",单位=") + scalar.unit + "\n" + MxFunctions::mNumberToString(scalar.value) + "\n";
        }
        else
        {
            MaterialCurve curve = prop.value.value<MaterialCurve>();
            res += ("*" + propname + QString(",单位X=") + curve.xUnit + QString(",单位Y=") + curve.yUnit + "\n");
            for (auto it = curve.curves.constBegin(); it != curve.curves.constEnd(); it++)
            {
                res += ("\"" + it.key() + "\"\n");
                for (int i = 0; i < it.value().x.size(); ++i)
                {
                    res += ("(" + MxFunctions::mNumberToString(it.value().x[i]) + "," + MxFunctions::mNumberToString(it.value().y[i]) + ")\n");
                }
            }
        }
        return res;
    }

    bool mMaterialModel::addMaterialIntoSummary(const Material& mat)
    {
        QString filepath = QCoreApplication::applicationDirPath() + "/materials/materials.txt";
        QFile f(filepath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            return false;
        }
        QTextStream out(&f);
        out.setCodec("UTF-8");
        out << mat.name << " | " << mat.category <<"|\n";
        return true;
    }

    void mMaterialModel::deleteMaterialMat(const Material& mat)
    {
        QString filepath = QCoreApplication::applicationDirPath() + "/materials/" + mat.name + ".mat";
        if (QFile::exists(filepath))
        {
            QFile::remove(filepath);
        }
    }
    void mMaterialModel::deleteMaterialFromSummary(const Material& mat)
    {
        QString filepath = QCoreApplication::applicationDirPath() + "/materials/materials.txt";
        QFile f(filepath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }

        QStringList lines;
        QTextStream in(&f);
        in.setCodec("UTF-8");
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.startsWith(mat.name))
            {
                lines << line;
            }
        }
        f.close();

        // 重新写回
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            return;
        }

        QTextStream out(&f);
        out.setCodec("UTF-8");
        for (const QString& l : lines)
        {
            out << l << "\n";
        }
        f.close();
    }

    void mMaterialModel::renameMaterialFromSummary(const QString& oldname, const QString& newname)
    {
        QString filepath = QCoreApplication::applicationDirPath() + "/materials/materials.txt";
        QFile f(filepath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }

        QStringList lines;
        QTextStream in(&f);
        in.setCodec("UTF-8");
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            QStringList parts = line.split(' ', QString::SkipEmptyParts);
            if (!parts.isEmpty() && parts.first() == oldname) 
            { 
                static const QRegularExpression re(R"(^\S+)");
                line.replace(re, newname);
            }
            lines << line;
        }
        f.close();

        // 重新写回
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            return;
        }

        QTextStream out(&f);
        out.setCodec("UTF-8");
        for (const QString& l : lines)
        {
            out << l << "\n";
        }
        f.close();
    }

    Qt::ItemFlags mMaterialModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return QStandardItemModel::flags(index) & ~Qt::ItemIsEditable;
    }

    QVariant mMaterialModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        switch (role) {
        case Qt::BackgroundRole:
            return QColor(240, 240, 240);   // 淡灰背景
        case Qt::ForegroundRole:
            return QColor(Qt::black);       // 黑色文字
        default:
            break;
        }
        return QStandardItemModel::data(index, role);
    }

    /****************************************************Service********************************************************************/

    mMaterialService::mMaterialService(QObject* parent)
        : QObject(parent)
    {
        m_source = new mMaterialModel(this);
        m_proxy = new QSortFilterProxyModel(this);

        m_proxy->setSourceModel(m_source);
        m_proxy->setFilterKeyColumn(0);
        m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxy->setRecursiveFilteringEnabled(true);  
    }

    QAbstractItemModel* mMaterialService::model() const
    {
        return m_proxy;
    }

    void mMaterialService::setFilter(const MaterialFilter& f)
    {
       m_proxy->setFilterFixedString(f.keyword);
    }

    Material mMaterialService::materialById(int id)
    {
        return m_source->getMaterialById(id);
    }

    bool mMaterialService::addMaterial(Material& mat)
    {
        return m_source->addMaterial(mat);
    }

    bool mMaterialService::updateMaterial(Material& mat)
    {
        return m_source->updateMaterial(mat);
    }

    void mMaterialService::removeMaterial(const QSet<int>& idxs)
    {
        m_source->removeMaterial(idxs);
    }

    void mMaterialService::copyMaterial(const int& id)
    {
        m_source->copyMaterial(id);
    }

    void mMaterialService::renameMaterial(const QString& oldname, const QString& newname)
    {
        m_source->renameMaterial(oldname, newname);
    }

    QSet<QString> mMaterialService::getAllMaterialTypes()
    {
        return m_source->getAllMaterialTypes();
    }

    bool mMaterialService::findMateiral(const QString& name)
    {
        return m_source->findMateiral(name);
    }
}