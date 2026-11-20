#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMotionData_global.h"

#include <QString>
#include <QVector>
#include <QDataStream>

namespace MechMotionData {

    enum class FlexMeshType {
        Linear = 0,      ///< 线性单元
        Quadratic = 1    ///< 二次单元
    };

    enum class FlexSolverType {
        Lanczos = 0,     ///< Lanczos 求解器
        Subspace = 1     ///< 子空间迭代
    };

    enum class FlexConnectionType {
        RBE2 = 0,        ///< 刚性多点约束
        RBE3 = 1,        ///< 柔性多点约束
        Weld = 2         ///< 焊缝连接
    };

    enum class FlexOutputFormat {
        MNFF = 0,        ///< ADAMS MNF 格式
        RST = 1,         ///< Ansys RST 格式
        OP2 = 2          ///< Nastran OP2 格式
    };

    enum class FlexDampingType {
        None = 0,        ///< 无阻尼
        Modal = 1,       ///< 模态阻尼
        Rayleigh = 2     ///< 瑞利阻尼
    };

    /**
     * @brief 自动柔性体数据类
     * @details 存储刚体转柔性体的完整参数配置
     */
    class MXMOTIONDATA_EXPORT mMotionAutoFlex {
    public:
        mMotionAutoFlex(int id, const QString& name = "", bool isUserRename = false);
        mMotionAutoFlex();
        ~mMotionAutoFlex();

        // --- 基础标识 ---
        void setId(int id);
        int getId() const;

        void setName(const QString& name);
        void setName(const QString& name, bool isUserRename);
        QString getName() const;

        bool getIsUserRename() const;

        // --- 目标刚体 ---

        // --- 材料 ---
        void setMaterialId(int materialId);
        int getMaterialId() const;

        // --- 界面节点（attachment points） ---
        void setAttachPoints(const QVector<int>& points);
        QVector<int> getAttachPoints() const;
        void addAttachPoint(int pointId);
        void removeAttachPoint(int pointId);
        void clearAttachPoints();

        // --- 网格参数 ---
        void setElementSize(double size);
        double getElementSize() const;

        void setMeshType(FlexMeshType type);
        FlexMeshType getMeshType() const;

        void setMinElementSize(double size);
        double getMinElementSize() const;

        void setCurvatureRefinement(bool enable);
        bool getCurvatureRefinement() const;

        // --- 模态分析参数 ---
        void setModeCount(int count);
        int getModeCount() const;

        void setFreqCutoff(double freq);
        double getFreqCutoff() const;

        void setSolverType(FlexSolverType type);
        FlexSolverType getSolverType() const;

        void setFreqLower(double freq);
        double getFreqLower() const;

        void setFreqUpper(double freq);
        double getFreqUpper() const;

        void setRigidBodyModeFilter(bool enable);
        bool getRigidBodyModeFilter() const;

        // --- 界面连接参数 ---
        void setConnectionType(FlexConnectionType type);
        FlexConnectionType getConnectionType() const;

        void setInterfaceDofs(int dofs);   ///< 位掩码: 1=X, 2=Y, 3=Z, 4=RX, 5=RY, 6=RZ
        int getInterfaceDofs() const;

        void setConnectionRadius(double radius);
        double getConnectionRadius() const;

        // --- 输出控制 ---
        void setOutputModeShapes(bool enable);
        bool getOutputModeShapes() const;

        void setOutputStressStrain(bool enable);
        bool getOutputStressStrain() const;

        void setOutputFormat(FlexOutputFormat format);
        FlexOutputFormat getOutputFormat() const;

        // --- 阻尼参数 ---
        void setDampingType(FlexDampingType type);
        FlexDampingType getDampingType() const;

        void setModalDampingRatio(double ratio);
        double getModalDampingRatio() const;

        void setRayleighMass(double alpha);
        double getRayleighMass() const;

        void setRayleighStiffness(double beta);
        double getRayleighStiffness() const;

        // --- 启用状态 ---
        void setEnabled(bool enabled);
        bool isEnabled() const;

        // --- 复制 ---
        mMotionAutoFlex* copy(int newId, const QString& newName) const;

        // --- 序列化 ---
        friend QDataStream& operator>>(QDataStream& in, mMotionAutoFlex& data);
        friend QDataStream& operator<<(QDataStream& out, const mMotionAutoFlex& data);

    private:
        int m_id;
        QString m_name;
        bool m_isUserRename;

        int m_materialId;
        QVector<int> m_attachPoints;

        // 网格参数
        double m_elementSize;
        FlexMeshType m_meshType;
        double m_minElementSize;
        bool m_curvatureRefinement;

        // 模态分析参数
        int m_modeCount;
        double m_freqCutoff;
        FlexSolverType m_solverType;
        double m_freqLower;
        double m_freqUpper;
        bool m_rigidBodyModeFilter;

        // 界面连接参数
        FlexConnectionType m_connectionType;
        int m_interfaceDofs;
        double m_connectionRadius;

        // 输出控制
        bool m_outputModeShapes;
        bool m_outputStressStrain;
        FlexOutputFormat m_outputFormat;

        // 阻尼参数
        FlexDampingType m_dampingType;
        double m_modalDampingRatio;
        double m_rayleighMass;
        double m_rayleighStiffness;

        // 启用状态
        bool m_isEnabled;
    };

}
