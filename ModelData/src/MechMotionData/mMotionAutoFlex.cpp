#include "mMotionAutoFlex.h"

namespace MechMotionData {

    mMotionAutoFlex::mMotionAutoFlex(int id, const QString& name, bool isUserRename)
        : m_id(id)
        , m_name(name)
        , m_isUserRename(isUserRename)
        
        , m_materialId(-1)
        // 网格参数
        , m_elementSize(5.0)
        , m_meshType(FlexMeshType::Linear)
        , m_minElementSize(1.0)
        , m_curvatureRefinement(true)
        // 模态分析参数
        , m_modeCount(10)
        , m_freqCutoff(0.0)   // 0=不截断（此前 100 会静默砍掉高阶模态）
        , m_solverType(FlexSolverType::Lanczos)
        , m_freqLower(0.0)
        , m_freqUpper(1e6)
        , m_rigidBodyModeFilter(true)
        // 界面连接参数
        , m_connectionType(FlexConnectionType::RBE2)
        , m_interfaceDofs(0x3F)  // 全部6个自由度
        , m_connectionRadius(0.0)
        // 输出控制
        , m_outputModeShapes(true)
        , m_outputStressStrain(true)
        , m_outputFormat(FlexOutputFormat::MNFF)
        // 阻尼参数（默认=曲柄摇杆柔性连杆验证过的稳态配置：Rayleigh(10, 2e-4)，
        // 16.5Hz 低阶模态约 6% 阻尼比；纯 Modal 2% 压不住机构耦合的能量注入）
        , m_dampingType(FlexDampingType::Rayleigh)
        , m_modalDampingRatio(0.02)
        , m_rayleighMass(10.0)
        , m_rayleighStiffness(2.0e-4)
        // 启用状态
        , m_isEnabled(true)
    {
    }

    mMotionAutoFlex::mMotionAutoFlex()
        : m_id(-1)
        , m_name("")
        , m_isUserRename(false)
        
        , m_materialId(-1)
        // 网格参数
        , m_elementSize(5.0)
        , m_meshType(FlexMeshType::Linear)
        , m_minElementSize(1.0)
        , m_curvatureRefinement(true)
        // 模态分析参数
        , m_modeCount(10)
        , m_freqCutoff(0.0)   // 0=不截断（此前 100 会静默砍掉高阶模态）
        , m_solverType(FlexSolverType::Lanczos)
        , m_freqLower(0.0)
        , m_freqUpper(1e6)
        , m_rigidBodyModeFilter(true)
        // 界面连接参数
        , m_connectionType(FlexConnectionType::RBE2)
        , m_interfaceDofs(0x3F)
        , m_connectionRadius(0.0)
        // 输出控制
        , m_outputModeShapes(true)
        , m_outputStressStrain(true)
        , m_outputFormat(FlexOutputFormat::MNFF)
        // 阻尼参数（默认=曲柄摇杆柔性连杆验证过的稳态配置：Rayleigh(10, 2e-4)，
        // 16.5Hz 低阶模态约 6% 阻尼比；纯 Modal 2% 压不住机构耦合的能量注入）
        , m_dampingType(FlexDampingType::Rayleigh)
        , m_modalDampingRatio(0.02)
        , m_rayleighMass(10.0)
        , m_rayleighStiffness(2.0e-4)
        // 启用状态
        , m_isEnabled(true)
    {
    }

    mMotionAutoFlex::~mMotionAutoFlex()
    {
    }

    void mMotionAutoFlex::setId(int id)
    {
        m_id = id;
    }

    int mMotionAutoFlex::getId() const
    {
        return m_id;
    }

    void mMotionAutoFlex::setName(const QString& name)
    {
        m_name = name;
    }

    void mMotionAutoFlex::setName(const QString& name, bool isUserRename)
    {
        m_name = name;
        m_isUserRename = isUserRename;
    }

    QString mMotionAutoFlex::getName() const
    {
        return m_name;
    }

    bool mMotionAutoFlex::getIsUserRename() const
    {
        return m_isUserRename;
    }



    void mMotionAutoFlex::setMaterialId(int materialId)
    {
        m_materialId = materialId;
    }

    int mMotionAutoFlex::getMaterialId() const
    {
        return m_materialId;
    }

    void mMotionAutoFlex::setAttachPoints(const QVector<int>& points)
    {
        m_attachPoints = points;
    }

    QVector<int> mMotionAutoFlex::getAttachPoints() const
    {
        return m_attachPoints;
    }

    void mMotionAutoFlex::addAttachPoint(int pointId)
    {
        if (!m_attachPoints.contains(pointId))
            m_attachPoints.append(pointId);
    }

    void mMotionAutoFlex::removeAttachPoint(int pointId)
    {
        m_attachPoints.removeAll(pointId);
    }

    void mMotionAutoFlex::clearAttachPoints()
    {
        m_attachPoints.clear();
    }

    // --- 网格参数 ---
    void mMotionAutoFlex::setElementSize(double size)
    {
        m_elementSize = size;
    }

    double mMotionAutoFlex::getElementSize() const
    {
        return m_elementSize;
    }

    void mMotionAutoFlex::setMeshType(FlexMeshType type)
    {
        m_meshType = type;
    }

    FlexMeshType mMotionAutoFlex::getMeshType() const
    {
        return m_meshType;
    }

    void mMotionAutoFlex::setMinElementSize(double size)
    {
        m_minElementSize = size;
    }

    double mMotionAutoFlex::getMinElementSize() const
    {
        return m_minElementSize;
    }

    void mMotionAutoFlex::setCurvatureRefinement(bool enable)
    {
        m_curvatureRefinement = enable;
    }

    bool mMotionAutoFlex::getCurvatureRefinement() const
    {
        return m_curvatureRefinement;
    }

    // --- 模态分析参数 ---
    void mMotionAutoFlex::setModeCount(int count)
    {
        m_modeCount = count;
    }

    int mMotionAutoFlex::getModeCount() const
    {
        return m_modeCount;
    }

    void mMotionAutoFlex::setFreqCutoff(double freq)
    {
        m_freqCutoff = freq;
    }

    double mMotionAutoFlex::getFreqCutoff() const
    {
        return m_freqCutoff;
    }

    void mMotionAutoFlex::setSolverType(FlexSolverType type)
    {
        m_solverType = type;
    }

    FlexSolverType mMotionAutoFlex::getSolverType() const
    {
        return m_solverType;
    }

    void mMotionAutoFlex::setFreqLower(double freq)
    {
        m_freqLower = freq;
    }

    double mMotionAutoFlex::getFreqLower() const
    {
        return m_freqLower;
    }

    void mMotionAutoFlex::setFreqUpper(double freq)
    {
        m_freqUpper = freq;
    }

    double mMotionAutoFlex::getFreqUpper() const
    {
        return m_freqUpper;
    }

    void mMotionAutoFlex::setRigidBodyModeFilter(bool enable)
    {
        m_rigidBodyModeFilter = enable;
    }

    bool mMotionAutoFlex::getRigidBodyModeFilter() const
    {
        return m_rigidBodyModeFilter;
    }

    // --- 界面连接参数 ---
    void mMotionAutoFlex::setConnectionType(FlexConnectionType type)
    {
        m_connectionType = type;
    }

    FlexConnectionType mMotionAutoFlex::getConnectionType() const
    {
        return m_connectionType;
    }

    void mMotionAutoFlex::setInterfaceDofs(int dofs)
    {
        m_interfaceDofs = dofs;
    }

    int mMotionAutoFlex::getInterfaceDofs() const
    {
        return m_interfaceDofs;
    }

    void mMotionAutoFlex::setConnectionRadius(double radius)
    {
        m_connectionRadius = radius;
    }

    double mMotionAutoFlex::getConnectionRadius() const
    {
        return m_connectionRadius;
    }

    // --- 输出控制 ---
    void mMotionAutoFlex::setOutputModeShapes(bool enable)
    {
        m_outputModeShapes = enable;
    }

    bool mMotionAutoFlex::getOutputModeShapes() const
    {
        return m_outputModeShapes;
    }

    void mMotionAutoFlex::setOutputStressStrain(bool enable)
    {
        m_outputStressStrain = enable;
    }

    bool mMotionAutoFlex::getOutputStressStrain() const
    {
        return m_outputStressStrain;
    }

    void mMotionAutoFlex::setOutputFormat(FlexOutputFormat format)
    {
        m_outputFormat = format;
    }

    FlexOutputFormat mMotionAutoFlex::getOutputFormat() const
    {
        return m_outputFormat;
    }

    // --- 阻尼参数 ---
    void mMotionAutoFlex::setDampingType(FlexDampingType type)
    {
        m_dampingType = type;
    }

    FlexDampingType mMotionAutoFlex::getDampingType() const
    {
        return m_dampingType;
    }

    void mMotionAutoFlex::setModalDampingRatio(double ratio)
    {
        m_modalDampingRatio = ratio;
    }

    double mMotionAutoFlex::getModalDampingRatio() const
    {
        return m_modalDampingRatio;
    }

    void mMotionAutoFlex::setRayleighMass(double alpha)
    {
        m_rayleighMass = alpha;
    }

    double mMotionAutoFlex::getRayleighMass() const
    {
        return m_rayleighMass;
    }

    void mMotionAutoFlex::setRayleighStiffness(double beta)
    {
        m_rayleighStiffness = beta;
    }

    double mMotionAutoFlex::getRayleighStiffness() const
    {
        return m_rayleighStiffness;
    }

    // --- 启用状态 ---
    void mMotionAutoFlex::setEnabled(bool enabled)
    {
        m_isEnabled = enabled;
    }

    bool mMotionAutoFlex::isEnabled() const
    {
        return m_isEnabled;
    }

    mMotionAutoFlex* mMotionAutoFlex::copy(int newId, const QString& newName) const
    {
        mMotionAutoFlex* newData = new mMotionAutoFlex(newId, newName, m_isUserRename);
        newData->m_materialId = m_materialId;
        newData->m_attachPoints = m_attachPoints;
        // 网格参数
        newData->m_elementSize = m_elementSize;
        newData->m_meshType = m_meshType;
        newData->m_minElementSize = m_minElementSize;
        newData->m_curvatureRefinement = m_curvatureRefinement;
        // 模态分析参数
        newData->m_modeCount = m_modeCount;
        newData->m_freqCutoff = m_freqCutoff;
        newData->m_solverType = m_solverType;
        newData->m_freqLower = m_freqLower;
        newData->m_freqUpper = m_freqUpper;
        newData->m_rigidBodyModeFilter = m_rigidBodyModeFilter;
        // 界面连接参数
        newData->m_connectionType = m_connectionType;
        newData->m_interfaceDofs = m_interfaceDofs;
        newData->m_connectionRadius = m_connectionRadius;
        // 输出控制
        newData->m_outputModeShapes = m_outputModeShapes;
        newData->m_outputStressStrain = m_outputStressStrain;
        newData->m_outputFormat = m_outputFormat;
        // 阻尼参数
        newData->m_dampingType = m_dampingType;
        newData->m_modalDampingRatio = m_modalDampingRatio;
        newData->m_rayleighMass = m_rayleighMass;
        newData->m_rayleighStiffness = m_rayleighStiffness;
        // 启用状态
        newData->m_isEnabled = m_isEnabled;
        return newData;
    }

    QDataStream& operator>>(QDataStream& in, mMotionAutoFlex& data)
    {
        int meshTypeInt, solverTypeInt, connTypeInt, outputFormatInt, dampingTypeInt;

        in >> data.m_id;
        in >> data.m_name;
        in >> data.m_isUserRename;
        in >> data.m_materialId;
        in >> data.m_attachPoints;
        // 网格参数
        in >> data.m_elementSize;
        in >> meshTypeInt;
        data.m_meshType = static_cast<FlexMeshType>(meshTypeInt);
        in >> data.m_minElementSize;
        in >> data.m_curvatureRefinement;
        // 模态分析参数
        in >> data.m_modeCount;
        in >> data.m_freqCutoff;
        in >> solverTypeInt;
        data.m_solverType = static_cast<FlexSolverType>(solverTypeInt);
        in >> data.m_freqLower;
        in >> data.m_freqUpper;
        in >> data.m_rigidBodyModeFilter;
        // 界面连接参数
        in >> connTypeInt;
        data.m_connectionType = static_cast<FlexConnectionType>(connTypeInt);
        in >> data.m_interfaceDofs;
        in >> data.m_connectionRadius;
        // 输出控制
        in >> data.m_outputModeShapes;
        in >> data.m_outputStressStrain;
        in >> outputFormatInt;
        data.m_outputFormat = static_cast<FlexOutputFormat>(outputFormatInt);
        // 阻尼参数
        in >> dampingTypeInt;
        data.m_dampingType = static_cast<FlexDampingType>(dampingTypeInt);
        in >> data.m_modalDampingRatio;
        in >> data.m_rayleighMass;
        in >> data.m_rayleighStiffness;
        // 启用状态
        in >> data.m_isEnabled;
        return in;
    }

    QDataStream& operator<<(QDataStream& out, const mMotionAutoFlex& data)
    {
        out << data.m_id;
        out << data.m_name;
        out << data.m_isUserRename;
        out << data.m_materialId;
        out << data.m_attachPoints;
        // 网格参数
        out << data.m_elementSize;
        out << static_cast<int>(data.m_meshType);
        out << data.m_minElementSize;
        out << data.m_curvatureRefinement;
        // 模态分析参数
        out << data.m_modeCount;
        out << data.m_freqCutoff;
        out << static_cast<int>(data.m_solverType);
        out << data.m_freqLower;
        out << data.m_freqUpper;
        out << data.m_rigidBodyModeFilter;
        // 界面连接参数
        out << static_cast<int>(data.m_connectionType);
        out << data.m_interfaceDofs;
        out << data.m_connectionRadius;
        // 输出控制
        out << data.m_outputModeShapes;
        out << data.m_outputStressStrain;
        out << static_cast<int>(data.m_outputFormat);
        // 阻尼参数
        out << static_cast<int>(data.m_dampingType);
        out << data.m_modalDampingRatio;
        out << data.m_rayleighMass;
        out << data.m_rayleighStiffness;
        // 启用状态
        out << data.m_isEnabled;
        return out;
    }

}
