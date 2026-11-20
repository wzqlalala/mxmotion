#include "mmainwindow_global.h"
#include "mAPIServer.h"

//Qt
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "mOtherFunction.h"
#include "mPythonAgent.h"

extern "C"
{
    MMAINWINDOW_EXPORT void startSolver()
    {
        getAPISchedulingServer->executeAPIAsync("startSolver", APIServer::APIParams{});
    }

    // 示例用法（Python）：exportPostVariableToText("Displacement", "Magnitude", "D:/output/disp.txt")
    // 参数说明：variableName-变量名，componentName-分量名（如 X/Y/Z/Magnitude），filePath-输出文本文件路径
    MMAINWINDOW_EXPORT void exportPostVariableToText(const char* variableName,
        const char* componentName, const char* filePath)
    {
        QString variableNameStr = QString::fromUtf8(variableName);
        QString componentNameStr = QString::fromUtf8(componentName);
        QString filePathStr = QString::fromUtf8(filePath);

        APIServer::APIParams params;
        params["variableName"] = QStringList{ variableNameStr };
        params["componentName"] = QStringList{ componentNameStr };
        params["filePath"] = QStringList{ filePathStr };

        getAPISchedulingServer->executeAPIAsync("exportPostVariableToText", params);
    }


    MMAINWINDOW_EXPORT void createCube(float x, float y, float z, float length, float width, float height)
    {
        APIServer::APIParams params;
        params.insert("origin", QStringList{ MxFunctions::mNumberToString(x), MxFunctions::mNumberToString(y), MxFunctions::mNumberToString(z) });
        params.insert("size", QStringList{ MxFunctions::mNumberToString(length), MxFunctions::mNumberToString(width), MxFunctions::mNumberToString(height) });
        getAPISchedulingServer->executeAPIAsync("createCube", params);
    }

    MMAINWINDOW_EXPORT void createCylinder(float x, float y, float z, float radius, float height)
    {
        APIServer::APIParams params;
        params.insert("origin", QStringList{ MxFunctions::mNumberToString(x), MxFunctions::mNumberToString(y), MxFunctions::mNumberToString(z) });
        params.insert("radius", QStringList{ MxFunctions::mNumberToString(radius) });
        params.insert("height", QStringList{ MxFunctions::mNumberToString(height) });
        getAPISchedulingServer->executeAPIAsync("createCylinder", params);
    }

    MMAINWINDOW_EXPORT void createSphere(float x, float y, float z, float radius)
    {
        APIServer::APIParams params;
        params.insert("origin", QStringList{ MxFunctions::mNumberToString(x), MxFunctions::mNumberToString(y), MxFunctions::mNumberToString(z) });
        params.insert("radius", QStringList{ MxFunctions::mNumberToString(radius) });
        getAPISchedulingServer->executeAPIAsync("createSphere", params);
    }

    MMAINWINDOW_EXPORT void createCone(float x, float y, float z, float radius, float height)
    {
        APIServer::APIParams params;
        params.insert("origin", QStringList{ MxFunctions::mNumberToString(x), MxFunctions::mNumberToString(y), MxFunctions::mNumberToString(z) });
        params.insert("radius", QStringList{ MxFunctions::mNumberToString(radius) });
        params.insert("height", QStringList{ MxFunctions::mNumberToString(height) });
        getAPISchedulingServer->executeAPIAsync("createCone", params);
    }

    MMAINWINDOW_EXPORT void createTorus(float x, float y, float z, float bigRadius, float smallRadius)
    {
        APIServer::APIParams params;
        params.insert("origin", QStringList{ MxFunctions::mNumberToString(x), MxFunctions::mNumberToString(y), MxFunctions::mNumberToString(z) });
        params.insert("bigRadius", QStringList{ MxFunctions::mNumberToString(bigRadius) });
        params.insert("smallRadius", QStringList{ MxFunctions::mNumberToString(smallRadius) });
        getAPISchedulingServer->executeAPIAsync("createTorus", params);
    }

    MMAINWINDOW_EXPORT void createFrustum(float x, float y, float z, float bottomRadius, float topRadius, float height)
    {
        APIServer::APIParams params;
        params.insert("origin", QStringList{ MxFunctions::mNumberToString(x), MxFunctions::mNumberToString(y), MxFunctions::mNumberToString(z) });
        params.insert("bottomRadius", QStringList{ MxFunctions::mNumberToString(bottomRadius) });
        params.insert("topRadius", QStringList{ MxFunctions::mNumberToString(topRadius) });
        params.insert("height", QStringList{ MxFunctions::mNumberToString(height) });
        getAPISchedulingServer->executeAPIAsync("createFrustum", params);
    }

    MMAINWINDOW_EXPORT void importGeoFile(char* filePath)
    {
        if (filePath == nullptr)
            return;

        /*char* p = filePath;

        while (*p)
        {
            if (*p == '\\')
            {
                *p = '/';
            }

            p++;
        }*/

        QString filePathStr = QString::fromUtf8(filePath);

        APIServer::APIParams params;
        params["filePath"] = QStringList{ filePathStr };

        getAPISchedulingServer->executeAPIAsync("importGeoFile", params);
    }

    MMAINWINDOW_EXPORT void meshGenerate(int dim, int meshType, float meshSize, int meshOrder)
    {
        APIServer::APIParams params;
        params.insert("factor", QStringList{ MxFunctions::mNumberToString(dim), MxFunctions::mNumberToString(meshType), MxFunctions::mNumberToString(meshSize), MxFunctions::mNumberToString(meshOrder) });
        getAPISchedulingServer->executeAPIAsync("meshGenerate", params);
    }

    MMAINWINDOW_EXPORT void createFaceToFaceContact(const char* name, const char* zhumian, const char* congmian, const char* xiaohuayi, const char* yaliguoying, const char* yueshufangfa, float suofangxishu,
        float faxishu, const char* moca, float mocaxishu, const char* rejiechu, float tiaozhengzhi, float guoyingliang, int qiexiangzd, int rejiechuzd, int jihezd, int jiechujianxizd)
    {
        QString nameStr = QString::fromUtf8(name);                  //面面接触名称
        QString zhumianStr = QString::fromUtf8(zhumian);            //主面 格式 :"1/2/3/4/5/6"    可以输入无限多个
        QString congmianStr = QString::fromUtf8(congmian);          //从面 格式:1/2/3/4/5/6        可以输入无限多个
        QString xiaohuayiStr = QString::fromUtf8(xiaohuayi);        //小滑移:关闭 开启
        QString yaliguoyingStr = QString::fromUtf8(yaliguoying);    //压力过盈:硬接触 完全绑定接触  法向不分离接触 线性接触
        QString yueshufangfaStr = QString::fromUtf8(yueshufangfa);  //约束方法:罚函数法 增广-摄动拉格朗日乘子法
        //suofangxishu                                              //缩放系数
        //faxishu                                                   //罚系数
        QString mocaStr = QString::fromUtf8(moca);                  //摩擦公式:无摩擦/通用接触摩擦/粗糙
        //mocaxishu                                                 //静摩擦系数
        QString rejiechuStr = QString::fromUtf8(rejiechu);          //热接触间隙传导（表格）  格式："1/2/3/4" //可无限输入
        //tiaozhengzhi                                              //调整值
        //QieXiangZD                                                //切向折叠框 0为false 1为true
        //ReJieChuZD                                                //热接触折叠框  0为false 1为true
        //JiHeZD                                                    //几何折叠框  0为false 1为true
        //jiechujianxizd                                            //接触间隙折叠框


        APIServer::APIParams params;
        params["Name"] = QStringList{ name };
        if (!zhumianStr.isEmpty())
            params["Master"] = zhumianStr.split("/");
        if (!congmianStr.isEmpty())
            params["Slave"] = congmianStr.split("/");
        params["XiaoHuaYi"] = QStringList{ xiaohuayiStr };
        params["YaLiGuoYing"] = QStringList{ yaliguoyingStr };
        params["YueShuFangFa"] = QStringList{ yueshufangfaStr };
        params["SuoFangXiShu"] = QStringList{ MxFunctions::mNumberToString(suofangxishu) };
        params["FaXiShu"] = QStringList{ MxFunctions::mNumberToString(faxishu) };
        params["MoCa"] = QStringList{ mocaStr };
        params["MoCaXiShu"] = QStringList{ MxFunctions::mNumberToString(mocaxishu) };
        if (!rejiechuStr.isEmpty())
            params["ReJieChu"] = rejiechuStr.split("/");
        params["TiaoZhengZhi"] = QStringList{ MxFunctions::mNumberToString(tiaozhengzhi) };
        params["GuoYingLiang"] = QStringList{ MxFunctions::mNumberToString(guoyingliang) };
        params["QieXiangZD"] = QStringList{ MxFunctions::mNumberToString(qiexiangzd) };
        params["ReJieChuZD"] = QStringList{ MxFunctions::mNumberToString(rejiechuzd) };
        params["JiHeZD"] = QStringList{ MxFunctions::mNumberToString(jihezd) };
        params["JieChuJianXiZD"] = QStringList{ MxFunctions::mNumberToString(jiechujianxizd) };

        getAPISchedulingServer->executeAPIAsync("createFaceToFaceContact", params);
    }

    MMAINWINDOW_EXPORT void createSolverControl(int CPUCores)
    {
        APIServer::APIParams params;
        params["CPUCores"] = QStringList{ MxFunctions::mNumberToString(CPUCores) };
        getAPISchedulingServer->executeAPIAsync("createSolverControl", params);
    }


    /**创建温度边界
    * 参数：
    *   name str: 边界名称
    *   jihedian str: 几何模型点ID集合，如"1/2/3/..."，可无限输入
    *   jihemian str: 几何模型面ID集合，如"1/2/3/..."，可无限输入
    *   Temp    double: 温度
    *   HanShu  str: 函数名称
    */
    MMAINWINDOW_EXPORT void createTemperatureBoundCondit(const char* name, const char* jihedian, const char* jihemian, double Temp, const char* HanShu)
    {
        QString nameStr = QString::fromUtf8(name);                    //温度边界名称
        QString jihedianStr = QString::fromUtf8(jihedian);            //几何点 格式 :"1/2/3/4/5/6"    可以输入无限多个
        QString jihemianStr = QString::fromUtf8(jihemian);            //几何面 格式 :"1/2/3/4/5/6"    可以输入无限多个
        QString HanShuStr = QString::fromUtf8(HanShu);               //函数

        APIServer::APIParams params;
        params["Name"] = QStringList{ name };
        if (!jihedianStr.isEmpty())
            params["JiHeDian"] = jihedianStr.split("/");
        if (!jihemianStr.isEmpty())
            params["JiHeMian"] = jihemianStr.split("/");
        params["Temp"] = QStringList{ MxFunctions::mNumberToString(Temp) };
        params["HanShu"] = QStringList{ HanShuStr };
        getAPISchedulingServer->executeAPIAsync("createTemperatureBoundCondit", params);
    }

    MMAINWINDOW_EXPORT void createFixedBoundCondit(const char* name, const char* jihedian, const char* jihemian, const char* fixedType)
    {
        QString nameStr = QString::fromUtf8(name);                    //固定约束名称
        QString jihedianStr = QString::fromUtf8(jihedian);            //几何点 格式 :"1/2/3/4/5/6"    可以输入无限多个
        QString jihemianStr = QString::fromUtf8(jihemian);            //几何面 格式 :"1/2/3/4/5/6"    可以输入无限多个
        QString typeStr = QString::fromUtf8(fixedType);               //类型

        APIServer::APIParams params;
        params["Name"] = QStringList{ name };
        if (!jihedianStr.isEmpty())
            params["JiHeDian"] = jihedianStr.split("/");
        if (!jihemianStr.isEmpty())
            params["JiHeMian"] = jihemianStr.split("/");
        params["FixedType"] = QStringList{ typeStr };
        getAPISchedulingServer->executeAPIAsync("createFixedBoundCondit", params);
    }

    /**创建对流换热载荷
    * 参数：
    *   name str: 载荷名称
    *   jihedian str: 几何模型点ID集合，如"1/2/3/..."，可无限输入
    *   jihemian str: 几何模型面ID集合，如"1/2/3/..."，可无限输入
    *   defineType int: 定义方式：0-统一赋值 1-独立赋值 2-文件定义
    *   Template double: 环境温度
    *   HeatValue double: 对流换热系数
    *   TemplateHanshu str: 环境温度曲线名称
    *   HeatHanshu str: 对流换热曲线名称
    */
    MMAINWINDOW_EXPORT void createLoadConvection(const char* name, const char* jihedian, const char* jihemian,
        int defineType, double Template, double HeatValue, const char* TemplateHanshu, const char* HeatHanshu)
    {
        QString nameStr = QString::fromUtf8(name);                    //名称
        QString jihedianStr = QString::fromUtf8(jihedian);            //几何点 格式 :"1/2/3/4/5/6"    可以输入无限多个
        QString jihemianStr = QString::fromUtf8(jihemian);            //几何面 格式 :"1/2/3/4/5/6"    可以输入无限多个
        QString TemplateHanshuStr = QString::fromUtf8(TemplateHanshu);
        QString HeatHanshuStr = QString::fromUtf8(HeatHanshu);

        APIServer::APIParams params;
        params["Name"] = QStringList{ name };
        if (!jihedianStr.isEmpty())
            params["JiHeDian"] = jihedianStr.split("/");
        if (!jihemianStr.isEmpty())
            params["JiHeMian"] = jihemianStr.split("/");
        params["defineType"] = QStringList{ MxFunctions::mNumberToString(defineType) };
        params["Template"] = QStringList{ MxFunctions::mNumberToString(Template) };
        params["HeatValue"] = QStringList{ MxFunctions::mNumberToString(HeatValue) };
        params["TemplateHanshu"] = QStringList{ TemplateHanshuStr };
        params["HeatHanshu"] = QStringList{ HeatHanshuStr };
        getAPISchedulingServer->executeAPIAsync("createLoadConvection", params);
    }

    MMAINWINDOW_EXPORT void createLoadForce(const char* name, const char* jihedian, const char* jihemian, float x, float y, float z, const char* hanshu, const char* zuobiaoxi)
    {
        QString nameStr = QString::fromUtf8(name);                    //名称
        QString jihedianStr = QString::fromUtf8(jihedian);            //几何点 格式 :"1/2/3/4/5/6"    可以输入无限多个
        QString jihemianStr = QString::fromUtf8(jihemian);            //几何面 格式 :"1/2/3/4/5/6"    可以输入无限多个
        QString hanshuStr = QString::fromUtf8(hanshu);
        QString zuobiaoxiStr = QString::fromUtf8(zuobiaoxi);

        APIServer::APIParams params;
        params["Name"] = QStringList{ name };
        if (!jihedianStr.isEmpty())
            params["JiHeDian"] = jihedianStr.split("/");
        if (!jihemianStr.isEmpty())
            params["JiHeMian"] = jihemianStr.split("/");
        params["X"] = QStringList{ MxFunctions::mNumberToString(x) };
        params["Y"] = QStringList{ MxFunctions::mNumberToString(y) };
        params["Z"] = QStringList{ MxFunctions::mNumberToString(z) };
        params["HanShu"] = QStringList{ hanshu };
        params["ZuoBiaoXi"] = QStringList{ zuobiaoxi };
        getAPISchedulingServer->executeAPIAsync("createLoadForce", params);
    }

    MMAINWINDOW_EXPORT void createIsotropicMaterial(const char* matName, float youngsModulus, float botson, float density)
    {
        APIServer::APIParams params;
        params.insert("matName", QStringList{ QString(matName) });
        params.insert("youngsModulus", QStringList{ MxFunctions::mNumberToString(youngsModulus) });
        params.insert("botson", QStringList{ MxFunctions::mNumberToString(botson) });
        params.insert("density", QStringList{ MxFunctions::mNumberToString(density) });
        getAPISchedulingServer->executeAPIAsync("createIsotropicMaterial", params);
    }
    MMAINWINDOW_EXPORT void createHeatMaterial(const char* matName, double Val1, double density, double specificHeat, double generationRate)
    {
        APIServer::APIParams params;
        params.insert("matName", QStringList{ QString(matName) });
        params.insert("Val1", QStringList{ MxFunctions::mNumberToString(Val1) });
        params.insert("density", QStringList{ MxFunctions::mNumberToString(density) });
        params.insert("specificHeat", QStringList{ MxFunctions::mNumberToString(specificHeat) });
        params.insert("generationRate", QStringList{ MxFunctions::mNumberToString(generationRate) });
        getAPISchedulingServer->executeAPIAsync("createHeatMaterial", params);
    }

    MMAINWINDOW_EXPORT void createSolidProperty(const char* name, const char* matName, const char* partName, const char* coordinateName)
    {
        QString partNameStr = QString::fromUtf8(partName);
        APIServer::APIParams params;
        params["name"] = QStringList{ QString(name) };
        params["matName"] = QStringList{ QString(matName) };
        if (!partNameStr.isEmpty())
            params["partName"] = partNameStr.split("/");
        params["coordinateName"] = QStringList{ QString(name) };
        getAPISchedulingServer->executeAPIAsync("createSolidProperty", params);
    }

    MMAINWINDOW_EXPORT void createLinearStatic(const char* name, const char* solver, const char* inrel, const char* autospc, const char* output, const char* load, const char* boundary)
    {
        QString outputStr = QString::fromUtf8(output);
        QString loadStr = QString::fromUtf8(load);
        QString boundaryStr = QString::fromUtf8(boundary);
        APIServer::APIParams params;
        params.insert("name", QStringList{ QString(name) });
        params.insert("solver", QStringList{ QString(solver) });
        params.insert("inrel", QStringList{ QString(inrel) });
        params.insert("autospc", QStringList{ QString(autospc) });
        if (!outputStr.isEmpty())
            params["output"] = outputStr.split("/");
        if (!loadStr.isEmpty())
            params["load"] = loadStr.split("/");
        if (!boundaryStr.isEmpty())
            params["boundary"] = boundaryStr.split("/");
        getAPISchedulingServer->executeAPIAsync("createLinearStatic", params);
    }

    /**创建热学分析
    * 参数：
    * name str : 分析名称
    * timeStepType str : 时间步类型：auto————自动，fix————固定
    * initialStepValue float : 初始时间步长
    * totolTimeValue float : 总时间
    * minTimeStep float : 最小步长
    * maxTimeStep float : 最大步长
    * cntnlsub str : 接续分析步：on————开启，off————关闭
    * output str : 输出控制列表字符串，根据该列表反向构建输出控制，如："disp/vel/temp"构建 基本结果——位移、速度、温度
    * load str : 挂载载荷名称列表，不创建载荷，而是根据名称查找已创建的载荷并挂载，如："Load1/Load2/..."
    * boundary str : 挂载边界名称列表，不创建边界，而是根据名称查找已创建的边界并挂载，如："Boundary1/Boundary2/..."
    */
    MMAINWINDOW_EXPORT void createSteadyHeat(const char* name, const char* timeStepType,
        double initialStepValue, double totolTimeValue, double minTimeStep, double maxTimeStep, const char* cntnlsub,
        const char* output, const char* load, const char* boundary, const char* preDefineField)
    {
        QString outputStr = QString::fromUtf8(output);
        QString loadStr = QString::fromUtf8(load);
        QString boundaryStr = QString::fromUtf8(boundary);
        QString preDefineFieldStr = QString::fromUtf8(preDefineField);
        APIServer::APIParams params;
        params.insert("name", QStringList{ QString(name) });
        params.insert("timeStepType", QStringList{ QString(timeStepType) });
        params.insert("initialStepValue", QStringList{ MxFunctions::mNumberToString(initialStepValue) });
        params.insert("totolTimeValue", QStringList{ MxFunctions::mNumberToString(totolTimeValue) });
        params.insert("minTimeStep", QStringList{ MxFunctions::mNumberToString(minTimeStep) });
        params.insert("maxTimeStep", QStringList{ MxFunctions::mNumberToString(maxTimeStep) });
        params.insert("cntnlsub", QStringList{ QString(cntnlsub) });
        if (!outputStr.isEmpty())
            params["outputStr"] = outputStr.split("/");
        if (!loadStr.isEmpty())
            params["loadStr"] = loadStr.split("/");
        if (!boundaryStr.isEmpty())
            params["boundaryStr"] = boundaryStr.split("/");
        if (!preDefineFieldStr.isEmpty())
            params["preDefineFieldStr"] = preDefineFieldStr.split("/");
        getAPISchedulingServer->executeAPIAsync("createSteadyHeat", params);
    }

    MMAINWINDOW_EXPORT void createModalAnalysis(const char* name, const char* qiujieqi, int jihedabianxing, float minPinlv, float maxPinlv, int jieshu, float rongcha, float xishu, const char* tezheng,
        const char* shuchu)
    {
        QString nameStr = QString::fromUtf8(name);    //模态分析名称
        QString qiejieStr = QString::fromUtf8(qiujieqi); //模态求解器：自动多重子结构  兰索斯
        //jihedabianxing                              //是否勾选几何大变形 0不勾选 1勾选
        //minPinlv                                    //最小频率
        //maxPinlv                                    //最大频率
        //jieshu                                      //阶数
        //rongcha                                     //容差
        //xishu                                       //放大系数
        QString tezhengStr = QString::fromUtf8(tezheng); //特征向量：无 广义质量 最大位移
        QString shuchuStr = QString::fromUtf8(shuchu); //输出控制  用/隔开

        APIServer::APIParams params;
        params["Name"] = QStringList{ QString(nameStr) };
        params["QiuJie"] = QStringList{ QString(qiejieStr) };
        params["DaBianXing"] = QStringList{ MxFunctions::mNumberToString(jihedabianxing) };
        params["MinPinLv"] = QStringList{ MxFunctions::mNumberToString(minPinlv) };
        params["MaxPinLv"] = QStringList{ MxFunctions::mNumberToString(maxPinlv) };
        params["JieShu"] = QStringList{ MxFunctions::mNumberToString(jieshu) };
        params["RongCha"] = QStringList{ MxFunctions::mNumberToString(rongcha) };
        params["XiShu"] = QStringList{ MxFunctions::mNumberToString(xishu) };
        params["TeZheng"] = QStringList{ QString(tezhengStr) };
        if (!shuchuStr.isEmpty())
            params["ShuChu"] = shuchuStr.split("/");

        getAPISchedulingServer->executeAPIAsync("createModalAnalysis", params);
    }

    MMAINWINDOW_EXPORT void createInitTempPredefinedField(const char* name, const char* jihedian, const char* jihemian, const char* jiheti, int defineType = 0, double Temp = 20)
    {
        QString nameStr = QString::fromUtf8(name);                    //名称
        QString jihedianStr = QString::fromUtf8(jihedian);            //几何点 格式 :"1/2/3/4/5/6"    可以输入无限多个
        QString jihemianStr = QString::fromUtf8(jihemian);            //几何面 格式 :"1/2/3/4/5/6"    可以输入无限多个
        QString jihetiStr = QString::fromUtf8(jiheti);                //几何体 格式 :"1/2/3/4/5/6"    可以输入无限多个
        APIServer::APIParams params;
        params["Name"] = QStringList{ QString(nameStr) };
        if (!jihedianStr.isEmpty())
            params["JiHeDian"] = jihedianStr.split("/");
        if (!jihemianStr.isEmpty())
            params["JiHeMian"] = jihemianStr.split("/");
        if (!jihetiStr.isEmpty())
            params["JiHeTi"] = jihetiStr.split("/");
        params["defineType"] = QStringList{ MxFunctions::mNumberToString(defineType) };
        params["Temp"] = QStringList{ MxFunctions::mNumberToString(Temp) };

        getAPISchedulingServer->executeAPIAsync("createInitTempPredefinedField", params);
    }

    MMAINWINDOW_EXPORT void setViewDirection(int mode) {
        APIServer::APIParams params;
        params["mode"] = QStringList{ MxFunctions::mNumberToString(mode) };
        getAPISchedulingServer->executeAPIAsync("setViewDirection", params);
    }

    MMAINWINDOW_EXPORT void optimTest() {
        APIServer::APIParams params;
        getAPISchedulingServer->executeAPIAsync("optimTest", params);
    }


    /** 将 Python 传入的英文逗号分隔 ID 字符串转换为 API 参数列表。 */
    QStringList DecodeDirectModelingIds(const char* encodedIds)
    {
        QStringList result;
        if (encodedIds == nullptr)
            return result;

        const QStringList values = QString::fromUtf8(encodedIds).split(QLatin1Char(','));
        for (const QString& value : values)
        {
            const QString trimmedValue = value.trimmed();
            if (!trimmedValue.isEmpty())
                result.append(trimmedValue);
        }

        if (result.isEmpty() && QString::fromUtf8(encodedIds).toInt() > 0)
        {
            result.append(QString::fromUtf8(encodedIds));
        }

        return result;
    }

    MMAINWINDOW_EXPORT void directModelingOffsetFace(char* faceIds, float distance)
    {
        APIServer::APIParams params;
        params.insert(QStringLiteral("faceIds"), DecodeDirectModelingIds(faceIds));
        params.insert(QStringLiteral("distance"), QStringList{ MxFunctions::mNumberToString(distance) });

        getAPISchedulingServer->executeAPIAsync(QStringLiteral("directModelingOffsetFace"), params);
    }

    MMAINWINDOW_EXPORT void directModelingReplaceFace(char* sourceFaceIds, char* targetFaceIds, float offset)
    {
        APIServer::APIParams params;
        params.insert(QStringLiteral("sourceFaceIds"), DecodeDirectModelingIds(sourceFaceIds));
        params.insert(QStringLiteral("targetFaceIds"), DecodeDirectModelingIds(targetFaceIds));
        params.insert(QStringLiteral("offset"), QStringList{ MxFunctions::mNumberToString(offset) });

        getAPISchedulingServer->executeAPIAsync(QStringLiteral("directModelingReplaceFace"), params);
    }

    MMAINWINDOW_EXPORT void directModelingRotateFace(int faceId, float angle, int axisEdgeId)
    {
        APIServer::APIParams params;
        params.insert(QStringLiteral("faceId"), QStringList{ QString::number(faceId) });
        params.insert(QStringLiteral("angle"), QStringList{ MxFunctions::mNumberToString(angle) });
        if (axisEdgeId >= 0)
        {
            params.insert(QStringLiteral("axisEdgeId"), QStringList{ QString::number(axisEdgeId) });
        }

        getAPISchedulingServer->executeAPIAsync(QStringLiteral("directModelingRotateFace"), params);
    }

    MMAINWINDOW_EXPORT void directModelingResizeFillet(char* elementId, char* elementType, float radius)
    {
        APIServer::APIParams params;
        params.insert(QStringLiteral("elementIds"), DecodeDirectModelingIds(elementId));
        params.insert(QStringLiteral("elementType"), QStringList{ QString::fromUtf8(elementType) });
        params.insert(QStringLiteral("radius"), QStringList{ MxFunctions::mNumberToString(radius) });

        getAPISchedulingServer->executeAPIAsync(QStringLiteral("directModelingResizeFillet"), params);
    }

    /** 通用JSON入口：Python侧把所有参数组装成JSON字符串传入，这里统一解析成 APIParams 后调对应API。
    * 所有值按字符串存储：JSON字符串→单元素QStringList；字符串数组→多元素；数字/布尔一律转字符串。
    */
    MMAINWINDOW_EXPORT void executeAPIJson(const char* apiKey, const char* jsonStr)
    {
        if (apiKey == nullptr || jsonStr == nullptr)
            return;

        const QString apiKeyStr = QString::fromUtf8(apiKey);
        const QByteArray jsonBytes = QByteArray::fromRawData(jsonStr, static_cast<int>(strlen(jsonStr)));

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        {
            qWarning() << "executeAPIJson: JSON parse failed:" << parseError.errorString();
            return;
        }

        APIServer::APIParams params;
        const QJsonObject obj = doc.object();
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
        {
            const QString key = it.key();
            const QJsonValue value = it.value();
            if (value.isArray())
            {
                QStringList list;
                const QJsonArray arr = value.toArray();
                for (const QJsonValue& item : arr)
                {
                    if (item.isString())
                        list.append(item.toString());
                    else if (item.isBool())
                        list.append(item.toBool() ? QStringLiteral("true") : QStringLiteral("false"));
                    else if (item.isDouble())
                        list.append(QString::number(item.toDouble()));
                }
                params.insert(key, list);
            }
            else if (value.isString())
            {
                params.insert(key, QStringList{ value.toString() });
            }
            else if (value.isBool())
            {
                params.insert(key, QStringList{ value.toBool() ? QStringLiteral("true") : QStringLiteral("false") });
            }
            else if (value.isDouble())
            {
                params.insert(key, QStringList{ QString::number(value.toDouble()) });
            }
        }

        // 脚本文件批量执行期间改为同步执行：executeAPIAsync 经 QTimer::singleShot 延迟到事件循环，
        // 届时 setScriptFileRunning 已复位，且脚本各命令间失去先后顺序（如 set_master 可能在 create 前执行）
        if (MechPreData::mPythonAgent::getInstance()->isScriptFileRunning())
            getAPISchedulingServer->executeAPI(apiKeyStr, params);
        else
            getAPISchedulingServer->executeAPIAsync(apiKeyStr, params);
    }

    MMAINWINDOW_EXPORT void createBind(const char* name, const char* zhumian, const char* congmian, const char* leixing, const char* rongchaleixing, float rongcha, int tiaozheng)
    {
        QString nameStr = QString::fromUtf8(name);      //绑定名称
        QString zhumianStr = QString::fromUtf8(zhumian); //主面:"1/2/3"
        QString congmianStr = QString::fromUtf8(congmian); //从面:"1/2/3"
        QString leixingStr = QString::fromUtf8(leixing); //类型  点面绑定 面面绑定
        QString rongchaleixingStr = QString::fromUtf8(rongchaleixing); //容差类型  自定义容差 abaqus optistruct
        //rongcha                                                      //容差
        //tiaozheng                                                    //0不勾选 1勾选

        APIServer::APIParams params;
        params["Name"] = QStringList{ QString(nameStr) };
        if(!zhumianStr.isEmpty())
        params["ZhuMian"] = zhumianStr.split("/");
        if(!congmianStr.isEmpty())
        params["CongMian"] = congmianStr.split("/");
        params["LeiXing"] = QStringList{ QString(leixingStr) };
        params["RongChaLeiXing"] = QStringList{ QString(rongchaleixingStr) };
        params["RongCha"] = QStringList{ MxFunctions::mNumberToString(rongcha) };
        params["TiaoZheng"] = QStringList{ MxFunctions::mNumberToString(tiaozheng) };  
        getAPISchedulingServer->executeAPIAsync("createBind", params);
    }
}
