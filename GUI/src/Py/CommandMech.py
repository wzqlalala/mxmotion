#-------关联C++库---------------
# 单一脚本同时支持 Debug（MechMainWindowd）与 Release（MechMainWindow）：
# 优先取已加载的 Debug 库，未加载则取 Release 库
import builtins
import ctypes
import json
import platform
from ctypes import *
from pathlib import Path

system = platform.system()
bin_dir = Path(__file__).parent
lib_dir = bin_dir

if system == "Windows":
    pre = ""
    suff = ".dll"
else:
    lib_dir = bin_dir
    pre = "lib"
    suff = ".so"

mw = None
_lib_candidates = [pre + "MechMainWindowd" + suff, pre + "MechMainWindow" + suff]

if system == "Windows":
    # 主库已被主程序加载，通过 GetModuleHandleA 获取已加载模块句柄（Debug/Release 自适应）
    for libname in _lib_candidates:
        h = ctypes.windll.kernel32.GetModuleHandleA(libname.encode('utf-8'))
        if h:
            mw = ctypes.CDLL(libname)
            break
    if mw is None:
        raise OSError(f"MechMainWindow library not loaded (tried {_lib_candidates})")
else:
    for libname in _lib_candidates:
        libpath = lib_dir / libname
        if libpath.exists():
            mw = ctypes.cdll.LoadLibrary(str(libpath))
            break
    if mw is None:
        raise OSError(f"MechMainWindow library not found (tried {_lib_candidates})")
#---------------------------------

# 接触类从子模块导入
from commands.contact import Contact, _set_mw as _contact_set_mw
_contact_set_mw(mw)


def listToString(temp):

    strvalList = None
    if len(temp) == 1:
        strvalList = str(temp[0])
        strvalList = bytes(strvalList, encoding='utf-8')
    elif len(temp) > 1:
        strvalList = ','.join(map(str, temp))
        strvalList = bytes(strvalList, encoding='utf-8')

    return strvalList


def getString(temp):

    String = None
    if temp is None:
        String = None
    else:
        String = bytes(temp, encoding='utf-8')

    return String

def startSolver():
    mw.startSolver()

def optimTest():
    mw.optimTest()


# ============================================================
# 接触二次开发类（JSON全字符串传参，经 executeAPIJson 调 C++ contact API）
# 用法：
#   Contact("C1", type="面面接触").set_master("1/2").set_slave("3/4").set_small_slip(True).create()
#   Contact("C1").set_small_slip(False).update()
# ============================================================


# 导出当前激活后处理数据库中指定变量的文本数据
# 示例：exportPostVariableToText("Displacement", "Magnitude", "D:/output/disp.txt")
# 参数说明：
#   variableName - 变量名，如 "Displacement"、"Stress" 等
#   componentName - 分量名，如 "X"、"Y"、"Z"、"Magnitude"
#   filePath - 输出文本文件路径，内容格式为每行 "ID value"
def exportPostVariableToText(variableName, componentName, filePath):
    mw.exportPostVariableToText(
        variableName.encode('utf-8'),
        componentName.encode('utf-8'),
        filePath.encode('utf-8')
    )


# 将 startSolver 注入 builtins，确保 PyRun_SimpleString 执行时能在全局命名空间找到
def createCube(x, y, z, length, width, height):
    mw.createCube(x, y, z, length, width, height)


def createCylinder(x, y, z, radius, height):
    mw.createCylinder(x, y, z, radius, height)


def createSphere(x, y, z, radius):
    mw.createSphere(x, y, z, radius)


def createCone(x, y, z, radius, height):
    mw.createCone(x, y, z, radius, height)


def createTorus(x, y, z, bigRadius, smallRadius):
    mw.createTorus(x, y, z, bigRadius, smallRadius)


def createFrustum(x, y, z, bottomRadius, topRadius, height):
    mw.createFrustum(x, y, z, bottomRadius, topRadius, height)

def importGeoFile(filePath):
    mw.importGeoFile(filePath.encode('utf-8'))

def meshGenerate(dim,meshType,meshSize,meshOrder):
    mw.meshGenerate(dim,meshType,meshSize,meshOrder)
    
def directModelingOffsetFace(faceIds, distance):
    mw.directModelingOffsetFace(faceIds.encode('utf-8'),distance)
    
def directModelingReplaceFace(sourceFaceIds, targetFaceIds,offset):
    mw.directModelingReplaceFace(sourceFaceIds.encode('utf-8'),targetFaceIds.encode('utf-8'),offset)
    
def directModelingRotateFace(faceIds, angle):
    mw.directModelingRotateFace(faceIds,angle)
    
def directModelingResizeFillet(elementIds, elementType,radius):
    mw.directModelingResizeFillet(elementIds.encode('utf-8'),elementType.encode('utf-8'),radius)


# 为所有 C ABI 接口声明签名，避免 ctypes 使用隐式 int 参数/返回值规则。
mw.startSolver.argtypes = []
mw.startSolver.restype = None

mw.executeAPIJson.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
mw.executeAPIJson.restype = None

mw.optimTest.argtypes = []
mw.optimTest.restype = None

mw.exportPostVariableToText.argtypes = [ctypes.c_char_p, ctypes.c_char_p,
                                        ctypes.c_char_p]
mw.exportPostVariableToText.restype = None

mw.importGeoFile.argtypes = [ctypes.c_char_p]
mw.importGeoFile.restype = None


def createFaceToFaceContact(name, zhumian, congmian, xiaohuayi, yaliguoying, yueshufangfa, suofangxishu, faxishu, moca, mocaxishu, rejiechu, tiaozhengzhi,guoyingliang, qiexiangzd, rejiechuzd, jihezd,jianchujianxizd):
    nameStr = getString(name)
    zhumianStr = getString(zhumian)
    congmianStr = getString(congmian)
    xiaohuayiStr = getString(xiaohuayi)
    yaliguoyingStr = getString(yaliguoying)
    yueshufangfaStr = getString(yueshufangfa)
    mocaStr = getString(moca)
    rejiechuStr = getString(rejiechu)
    mw.createFaceToFaceContact(nameStr, zhumianStr, congmianStr, xiaohuayiStr, yaliguoyingStr, yueshufangfaStr,suofangxishu,faxishu, mocaStr, mocaxishu, rejiechuStr,tiaozhengzhi,guoyingliang, qiexiangzd, rejiechuzd, jihezd, jianchujianxizd)

def createSolverControl(CpuCores):
    mw.createSolverControl(CpuCores)

"""创建温度边界
* 参数：
*   name str: 边界名称
*   jihedian str: 几何模型点ID集合，如"1/2/3/..."，可无限输入
*   jihemian str: 几何模型面ID集合，如"1/2/3/..."，可无限输入
*   Temp    double: 温度
*   HanShu  str: 函数名称
"""
def createTemperatureBoundCondit(name, jihedian, jihemian, Temp, HanShu):
    nameStr = getString(name)
    jihedianStr = getString(jihedian)
    jihemianStr = getString(jihemian)
    HanShuStr = getString(HanShu)
    mw.createTemperatureBoundCondit(nameStr, jihedianStr, jihemianStr, Temp, HanShuStr)

def createFixedBoundCondit(name, jihedian, jihemian, fixedType):
    nameStr = getString(name)
    jihedianStr = getString(jihedian)
    jihemianStr = getString(jihemian)
    fixedTypeStr = getString(fixedType)
    mw.createFixedBoundCondit(nameStr, jihedianStr, jihemianStr, fixedTypeStr)

def createLoadForce(name, jihedian, jihemian, x, y, z, hanshu ,zuobiaoxi):
    nameStr = getString(name)
    jihedianStr = getString(jihedian)
    jihemianStr = getString(jihemian)
    hanshuStr = getString(hanshu)
    zuobiaoxiStr = getString(zuobiaoxi)
    mw.createLoadForce(nameStr, jihedianStr, jihemianStr, x, y, z, hanshuStr, zuobiaoxiStr)

"""创建对流换热载荷
* 参数：
*   name str: 载荷名称
*   jihedian str: 几何模型点ID集合，如"1/2/3/..."，可无限输入
*   jihemian str: 几何模型面ID集合，如"1/2/3/..."，可无限输入
*   defineType int: 定义方式：0-统一赋值 1-独立赋值 2-文件定义
*   Template float: 环境温度
*   HeatValue float: 对流换热系数
*   TemplateHanshu str: 环境温度曲线名称
*   HeatHanshu str: 对流换热曲线名称
"""
def createLoadConvection(name, jihedian, jihemian, defineType, Template, HeatValue, TemplateHanshu ,HeatHanshu):
    nameStr = getString(name)
    jihedianStr = getString(jihedian)
    jihemianStr = getString(jihemian)
    TemplateHanshuStr = getString(TemplateHanshu)
    HeatHanshuStr = getString(HeatHanshu)
    mw.createLoadConvection(nameStr, jihedianStr, jihemianStr, defineType, Template, HeatValue, TemplateHanshuStr, HeatHanshuStr)

def createIsotropicMaterial(name, youngsModulus, botson, density):
    nameStr = getString(name)
    mw.createIsotropicMaterial(nameStr, youngsModulus, botson, density);


def createHeatMaterial(name, Val1, density, specificHeat, generationRate):
    nameStr = getString(name)
    mw.createHeatMaterial(nameStr, Val1, density, specificHeat, generationRate);

def createSolidProperty(name, matName, partName, coordinateName):
    nameStr = getString(name)
    matNameStr = getString(matName)
    partNameStr = getString(partName)
    coordinateNameStr = getString(coordinateName)
    mw.createSolidProperty(nameStr, matNameStr, partNameStr, coordinateNameStr);

"""创建热学分析
* 参数：
*   name str: 分析名称
*   timeStepType str: 时间步类型：auto————自动，fix————固定
*   initialStepValue float: 初始时间步长
*   totolTimeValue float: 总时间
*   minTimeStep float: 最小步长
*   maxTimeStep float: 最大步长
*   cntnlsub str: 接续分析步：on————开启，off————关闭
*   output str: 输出控制列表字符串，根据该列表反向构建输出控制，如："disp/vel/temp"构建 基本结果——位移、速度、温度
*   load str: 挂载载荷名称列表，不创建载荷，而是根据名称查找已创建的载荷并挂载，如："Load1/Load2/..."
*   boundary str: 挂载边界名称列表，不创建边界，而是根据名称查找已创建的边界并挂载，如："Boundary1/Boundary2/..."
*   preDefineField str: 挂载预定义场名称列表，不创建边界，而是根据名称查找已创建的场并挂载，如："preDefineField1/preDefineField2/..."
"""
def createSteadyHeat(name,timeStepType,initialStepValue,totolTimeValue,minTimeStep,maxTimeStep,cntnlsub,output,load,boundary,preDefineField):
    nameStr = getString(name)
    timeStepTypeStr = getString(timeStepType)
    cntnlsubStr = getString(cntnlsub)

    outputStr = getString(output)
    loadStr = getString(load)
    boundaryStr = getString(boundary)
    preDefineFieldStr = getString(preDefineField)
    mw.createSteadyHeat(nameStr, timeStepTypeStr, initialStepValue, totolTimeValue, minTimeStep,maxTimeStep,cntnlsubStr,outputStr,loadStr,boundaryStr,preDefineFieldStr);

def createLinearStatic(name,solver,inrel,autospc,output,load,boundary):
    nameStr = getString(name)
    solverStr = getString(solver)
    inrelStr = getString(inrel)
    autospcStr = getString(autospc)
    outputStr = getString(output)
    loadStr = getString(load)
    boundaryStr = getString(boundary)
    mw.createLinearStatic(nameStr, solverStr, inrelStr, autospcStr, outputStr,loadStr,boundaryStr);

def createModalAnalysis(name,qiujieqi,jihedabianxing,minPinlv,maxPinlv,jieshu,rongcha,xishu,tezheng,shuchu):
    nameStr = getString(name)
    qiujieqiStr = getString(qiujieqi)
    tezhengStr = getString(tezheng)
    shuchuStr = getString(shuchu)
    mw.createModalAnalysis(nameStr,qiujieqiStr,jihedabianxing,minPinlv,maxPinlv,jieshu,rongcha,xishu,tezhengStr,shuchuStr);
    
def createBind(name,zhumian,congmian,leixing,rongchaleixing,rongcha,tiaozheng):
    nameStr = getString(name)
    zhumianStr = getString(zhumian)
    congmianStr = getString(congmian)
    leixingStr = getString(leixing)
    rongchaleixingStr = getString(rongchaleixing)
    mw.createBind(nameStr,zhumianStr,congmianStr,leixingStr,rongchaleixingStr,rongcha,tiaozheng);

def createInitTempPredefinedField(name,jihedian,jihemian,jiheti,defineType,Temp):
    nameStr = getString(name)
    jihedianStr = getString(jihedian)
    jihemianStr = getString(jihemian)
    jihetiStr = getString(jiheti)
    mw.createInitTempPredefinedField(nameStr,jihedianStr,jihemianStr,jihetiStr,defineType,Temp);
"""创建热学分析
* 参数：
*   mode: 0~8分别对应前、后、上、下、左、右、标准1asiAlgo_AAG视图

setViewDirection(0)
"""
def setViewDirection(mdoe):
    mw.setViewDirection(mdoe);
    


# 显式声明 ctypes 参数/返回类型，避免 float 传参 ABI 错位
mw.createCube.argtypes = [ctypes.c_float, ctypes.c_float, ctypes.c_float,
                          ctypes.c_float, ctypes.c_float, ctypes.c_float]
mw.createCube.restype = None

mw.createCylinder.argtypes = [ctypes.c_float, ctypes.c_float, ctypes.c_float,
                              ctypes.c_float, ctypes.c_float]
mw.createCylinder.restype = None

mw.createSphere.argtypes = [ctypes.c_float, ctypes.c_float, ctypes.c_float,
                            ctypes.c_float]
mw.createSphere.restype = None

mw.createCone.argtypes = [ctypes.c_float, ctypes.c_float, ctypes.c_float,
                        ctypes.c_float, ctypes.c_float]
mw.createCone.restype = None

mw.createTorus.argtypes = [ctypes.c_float, ctypes.c_float, ctypes.c_float,
                           ctypes.c_float, ctypes.c_float]
mw.createTorus.restype = None

mw.createFrustum.argtypes = [ctypes.c_float, ctypes.c_float, ctypes.c_float,
                             ctypes.c_float, ctypes.c_float, ctypes.c_float]
mw.createFrustum.restype = None

mw.createFaceToFaceContact.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,
                             ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_float,
                             ctypes.c_float,ctypes.c_char_p,ctypes.c_float,ctypes.c_char_p,ctypes.c_float,ctypes.c_float,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int]
mw.createFaceToFaceContact.restype = None

mw.createSolverControl.argtypes = [ctypes.c_int]
mw.createSolverControl.restype = None

mw.createFixedBoundCondit.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,ctypes.c_char_p]
mw.createFixedBoundCondit.restype = None

mw.createTemperatureBoundCondit.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,ctypes.c_double,ctypes.c_char_p]
mw.createTemperatureBoundCondit.restype = None

mw.createLoadForce.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_float, ctypes.c_float, ctypes.c_float,ctypes.c_char_p, ctypes.c_char_p]
mw.createLoadForce.restype = None

mw.createLoadConvection.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_double, ctypes.c_double,ctypes.c_char_p, ctypes.c_char_p]
mw.createLoadConvection.restype = None

mw.createIsotropicMaterial.argtypes = [ctypes.c_char_p,ctypes.c_float, ctypes.c_float,ctypes.c_float]
mw.createIsotropicMaterial.restype = None

mw.createHeatMaterial.argtypes = [ctypes.c_char_p,ctypes.c_double, ctypes.c_double,ctypes.c_double,ctypes.c_double]
mw.createHeatMaterial.restype = None

mw.createSolidProperty.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
mw.createSolidProperty.restype = None

mw.createLinearStatic.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
mw.createLinearStatic.restype = None

mw.createSteadyHeat.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_double, ctypes.c_double,ctypes.c_double, ctypes.c_double, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
mw.createSteadyHeat.restype = None

mw.meshGenerate.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_float,
                             ctypes.c_int]
mw.meshGenerate.restype = None

mw.directModelingOffsetFace.argtypes=[ctypes.c_char_p,ctypes.c_float]
mw.directModelingOffsetFace.restype=None

mw.directModelingReplaceFace.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_float]
mw.directModelingReplaceFace.restype = None

mw.directModelingRotateFace.argtypes = [ctypes.c_int, ctypes.c_float]
mw.directModelingRotateFace.restype = None

mw.directModelingResizeFillet.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_float]
mw.directModelingResizeFillet.restype = None
mw.createModalAnalysis.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_float,ctypes.c_float,ctypes.c_int,ctypes.c_float,ctypes.c_float,ctypes.c_char_p,ctypes.c_char_p]
mw.createModalAnalysis.restype = None

mw.createBind.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,ctypes.c_char_p,ctypes.c_float,ctypes.c_int]
mw.createBind.restype = None

mw.createInitTempPredefinedField.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_double]
mw.createInitTempPredefinedField.restype = None

mw.setViewDirection.argtypes = [ctypes.c_int]
mw.setViewDirection.restype = None

# 将函数注入 builtins，确保 PyRun_SimpleString 执行时能在全局命名空间找到
builtins.startSolver = startSolver
builtins.Contact = Contact
builtins.optimTest = optimTest
builtins.exportPostVariableToText = exportPostVariableToText
builtins.createCube = createCube
builtins.createCylinder = createCylinder
builtins.createSphere = createSphere
builtins.createCone = createCone
builtins.createTorus = createTorus
builtins.createFrustum = createFrustum
builtins.importGeoFile = importGeoFile
builtins.meshGenerate=meshGenerate
builtins.directModelingOffsetFace = directModelingOffsetFace
builtins.directModelingReplaceFace = directModelingReplaceFace
builtins.directModelingRotateFace = directModelingRotateFace
builtins.directModelingResizeFillet = directModelingResizeFillet
builtins.createFaceToFaceContact = createFaceToFaceContact
builtins.createSolverControl = createSolverControl
builtins.createFixedBoundCondit = createFixedBoundCondit
builtins.createTemperatureBoundCondit = createTemperatureBoundCondit 
builtins.createLoadForce = createLoadForce
builtins.createLoadConvection = createLoadConvection 
builtins.createIsotropicMaterial = createIsotropicMaterial
builtins.createHeatMaterial = createHeatMaterial
builtins.createSolidProperty = createSolidProperty
builtins.createLinearStatic = createLinearStatic 
builtins.createSteadyHeat = createSteadyHeat
builtins.createModalAnalysis = createModalAnalysis
builtins.createBind = createBind
builtins.createInitTempPredefinedField = createInitTempPredefinedField
builtins.setViewDirection = setViewDirection