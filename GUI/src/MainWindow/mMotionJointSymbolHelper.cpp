#include "mMotionJointSymbolHelper.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SpaceTree.h"

#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointUniversal.h"
#include "mMotionJointScrew.h"
#include "mMotionJointPerpendicular.h"
#include "mMotionJointConstantVelocity.h"

#include <QVector>
#include <algorithm>
#include <qmath.h>

using namespace mxr;
using namespace MechMotionData;
using namespace MxFunctions;

namespace MechMainWindow
{
    namespace
    {
        // 符号构建共用常量：向量退化判定阈值与圆周率（各 build* 函数统一使用）
        constexpr float EPS = 1.0e-12f;
        constexpr float PI = 3.14159265358979323846f;
    }

    void mMotionJointSymbolHelper::createOrUpdateJointSymbol(mxr::mSymbolController* symbolController,
        MechMotionData::mMotionJointBase* jointData,
        const QVector3D& origin,
        const QVector3D& axis,
        mxr::CCSMRendWidget* rendWidget)
    {
        if (!symbolController || !jointData)
            return;

        createOrUpdateJointSymbol(symbolController,
            jointData->getId(),
            jointData->getType(),
            jointData->getName(),
            origin,
            axis,
            rendWidget);
    }

    void mMotionJointSymbolHelper::createOrUpdateJointSymbol(mxr::mSymbolController* symbolController,
        int jointId,
        MxFunctions::JointType type,
        const QString& /*name*/,
        const QVector3D& origin,
        const QVector3D& axis,
        mxr::CCSMRendWidget* rendWidget)
    {
        if (!symbolController)
            return;

        const float sceneScale = computeSceneScale(rendWidget);
        QString symbolName = QString("motionJoint_%1").arg(jointId);

        // 切换类型时，先删除旧符号，避免同名符号数据类型不一致导致 getOrCreateSymbolData 失败
        symbolController->deleteSymbol({ symbolName });

        switch (type)
        {
        case MxFunctions::JointType::Spherical:
            buildSphericalSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::Fixed:
            buildFixedSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::Revolute:
            buildRevoluteSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::Translational:
            buildTranslationalSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::Cylindrical:
            buildCylindricalSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::Screw:
            buildScrewSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::PointPlane:
            buildPointOnPlaneSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::PointLine:
            buildPointOnLineSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::PointOnCurve:
            buildPointOnCurveSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::PointOnSurface:
            buildPointOnSurfaceSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::CurveOnCurve:
            buildCurveOnCurveSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::Planar:
            buildPlanarSymbol(symbolController, symbolName, origin, axis, true, sceneScale);
            break;
        case MxFunctions::JointType::Parallel:
            buildParallelSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        case MxFunctions::JointType::Direction:
            buildDirectionSymbol(symbolController, symbolName, origin, axis, sceneScale);
            break;
        //case MxFunctions::JointType::Universal:
        //    buildUniversalSymbol(symbolController, symbolName, origin, axis, sceneScale);
        //    break;
        default:
            break;
        }

        symbolController->UpdateAllSymbol();
    }

    void mMotionJointSymbolHelper::createOrUpdateUniversalSymbol(mxr::mSymbolController* symbolController, int jointId, const QString& name, const QVector3D& origin, const QVector3D& spinAxisAction, const QVector3D& crossPinAction, const QVector3D& spinAxisBase, const QVector3D& crossPinBase, mxr::CCSMRendWidget* rendWidget)
    {
        if (!symbolController)
            return;

        const float sceneScale = computeSceneScale(rendWidget);
        QString symbolName = QString("motionJoint_%1").arg(jointId);

        // 切换类型时，先删除旧符号，避免同名符号数据类型不一致导致 getOrCreateSymbolData 失败
        symbolController->deleteSymbol({ symbolName });
        buildUniversalSymbol(symbolController, symbolName, origin, spinAxisAction, crossPinAction, spinAxisBase, crossPinBase, sceneScale);
        symbolController->UpdateAllSymbol();
    }

    void mMotionJointSymbolHelper::createOrUpdatePerpendicularSymbol(mxr::mSymbolController* symbolController,
        int jointId,
        const QString& name,
        const QVector3D& origin,
        const QVector3D& actionDirection,
        const QVector3D& baseDirection,
        mxr::CCSMRendWidget* rendWidget)
    {
        if (!symbolController)
            return;

        const float sceneScale = computeSceneScale(rendWidget);
        QString symbolName = QString("motionJoint_%1").arg(jointId);

        // 切换类型时，先删除旧符号，避免同名符号数据类型不一致导致 getOrCreateSymbolData 失败
        symbolController->deleteSymbol({ symbolName });
        buildPerpendicularSymbol(symbolController, symbolName, origin, actionDirection, baseDirection, sceneScale);
        symbolController->UpdateAllSymbol();
    }

    void mMotionJointSymbolHelper::createOrUpdateConstantVelocitySymbol(mxr::mSymbolController* symbolController,
        int jointId,
        const QString& name,
        const QVector3D& origin,
        const QVector3D& actionDirection,
        const QVector3D& baseDirection,
        mxr::CCSMRendWidget* rendWidget)
    {
        if (!symbolController)
            return;

        const float sceneScale = computeSceneScale(rendWidget);
        QString symbolName = QString("motionJoint_%1").arg(jointId);

        // 切换类型时，先删除旧符号，避免同名符号数据类型不一致导致 getOrCreateSymbolData 失败
        symbolController->deleteSymbol({ symbolName });
        buildConstantVelocitySymbol(symbolController, symbolName, origin, actionDirection, baseDirection, sceneScale);
        symbolController->UpdateAllSymbol();
    }

    void mMotionJointSymbolHelper::clearJointSymbol(mxr::mSymbolController* symbolController, int jointId)
    {
        if (!symbolController)
            return;
        QString symbolName = QString("motionJoint_%1").arg(jointId);
        symbolController->deleteSymbol({ symbolName });
        symbolController->UpdateAllSymbol();
    }

    void mMotionJointSymbolHelper::ensureJointSymbolByData(mxr::mSymbolController* symbolController,
        MechMotionData::mMotionJointBase* jointData,
        mxr::CCSMRendWidget* rendWidget)
    {
        if (!symbolController || !jointData)
            return;
        //已存在的符号不动（保留面板编辑后的最新位姿；显隐由树统一控制）
        QString symbolName = QString("motionJoint_%1").arg(jointData->getId());
        if (symbolController->isExistSymbol(symbolName))
            return;

        if (auto* basicJoint = dynamic_cast<MechMotionData::mMotionJointBasic*>(jointData))
        {
            createOrUpdateJointSymbol(symbolController, jointData->getId(), jointData->getType(),
                jointData->getName(), basicJoint->getOrigin(), basicJoint->getAxis(), rendWidget);
        }
        else if (auto* screwJoint = dynamic_cast<MechMotionData::mMotionJointScrew*>(jointData))
        {
            createOrUpdateJointSymbol(symbolController, jointData->getId(), jointData->getType(),
                jointData->getName(), screwJoint->getOrigin(), screwJoint->getAxis(), rendWidget);
        }
        else if (auto* universalJoint = dynamic_cast<MechMotionData::mMotionJointUniversal*>(jointData))
        {
            createOrUpdateUniversalSymbol(symbolController, jointData->getId(), jointData->getName(),
                universalJoint->getOrigin(),
                universalJoint->getActionBodySpinVector(),
                universalJoint->getActionBodyPinVector(),
                universalJoint->getBaseBodySpinVector(),
                universalJoint->getBaseBodyPinVector(),
                rendWidget);
        }
        else if (auto* perpendicularJoint = dynamic_cast<MechMotionData::mMotionJointPerpendicular*>(jointData))
        {
            createOrUpdatePerpendicularSymbol(symbolController, jointData->getId(), jointData->getName(),
                perpendicularJoint->getOrigin(),
                perpendicularJoint->getActionBodyVector(),
                perpendicularJoint->getBaseBodyVector(),
                rendWidget);
        }
        else if (auto* constantVelocityJoint = dynamic_cast<MechMotionData::mMotionJointConstantVelocity*>(jointData))
        {
            createOrUpdateConstantVelocitySymbol(symbolController, jointData->getId(), jointData->getName(),
                constantVelocityJoint->getOrigin(),
                constantVelocityJoint->getActionBodyVector(),
                constantVelocityJoint->getBaseBodyVector(),
                rendWidget);
        }
    }

    void mMotionJointSymbolHelper::createOrUpdateCouplingSymbol(mxr::mSymbolController* symbolController,
        int couplingId,
        MxFunctions::CouplingType type,
        const QVector3D& jointOrigin1,
        const QVector3D& jointAxis1,
        const QVector3D& jointOrigin2,
        const QVector3D& jointAxis2,
        double ratio,
        mxr::CCSMRendWidget* rendWidget)
    {
        if (!symbolController)
            return;

        const float sceneScale = computeSceneScale(rendWidget);
        QString symbolName = QString("motionCoupling_%1").arg(couplingId);

        // 切换类型时，先删除旧符号，避免同名符号数据类型不一致导致 getOrCreateSymbolData 失败
        symbolController->deleteSymbol({ symbolName });

        switch (type)
        {
        case MxFunctions::CouplingType::Gear:
            buildGearCouplingSymbol(symbolController, symbolName,
                jointOrigin1, jointAxis1,
                jointOrigin2, jointAxis2,
                ratio, sceneScale);
            break;
        case MxFunctions::CouplingType::GearRack:
            buildRackPinionCouplingSymbol(symbolController, symbolName,
                jointOrigin1, jointAxis1,
                jointOrigin2, jointAxis2,
                ratio, sceneScale);
            break;
        default:
            break;
        }

        symbolController->UpdateAllSymbol();
    }

    void mMotionJointSymbolHelper::clearCouplingSymbol(mxr::mSymbolController* symbolController, int couplingId)
    {
        if (!symbolController)
            return;
        QString symbolName = QString("motionCoupling_%1").arg(couplingId);
        symbolController->deleteSymbol({ symbolName });
        symbolController->UpdateAllSymbol();
    }

    void mMotionJointSymbolHelper::buildFixedSymbol(mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& axis,
        float sceneScale)
    {
        QVector3D nAxis = axis;
        if (nAxis.lengthSquared() < 1e-12f)
            nAxis = QVector3D(0, 0, 1);
        else
            nAxis.normalize();

        QVector3D u, v;
        buildOrthonormalBasis(nAxis, u, v);

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        // 辅助闭包：快速添加线段
        auto addLine = [&points, &lineIdx](const QVector3D& a, const QVector3D& b)
            {
                unsigned int i0 = static_cast<unsigned int>(points.size());
                points.append(a);
                points.append(b);
                lineIdx.push_back(i0);
                lineIdx.push_back(i0 + 1);
            };

        // --- 精确的几何比例定义 ---
        float R = sceneScale * 0.45f;      // 中心标准圆半径
        float W = sceneScale * 1.1f;       // 左右顶点的半宽
        float H_top = sceneScale * 0.9f;   // 顶部顶点的高度
        float H_drop = sceneScale * 0.15f; // 左右顶点低于中心线的下沉高度

        QVector3D P_top = origin + v * H_top;
        QVector3D P_left = origin - u * W - v * H_drop;
        QVector3D P_right = origin + u * W - v * H_drop;

        // ==========================================
        // 1. 绘制中心标准圆
        // ==========================================
        int segments = 40;
        int circleBase = points.size();
        for (int i = 0; i < segments; ++i) {
            float theta = 2.0f * static_cast<float>(M_PI) * i / segments;
            points.append(origin + u * (R * cos(theta)) + v * (R * sin(theta)));
        }
        for (int i = 0; i < segments; ++i) {
            lineIdx.push_back(circleBase + i);
            lineIdx.push_back(circleBase + (i + 1) % segments);
        }

        // ==========================================
        // 2. 求解外部点到圆的底部切点 (解析几何)
        // ==========================================
        auto getBottomTangent = [&](const QVector3D& P) -> QVector3D {
            QVector3D CP = P - origin;
            float px = QVector3D::dotProduct(CP, u);
            float py = QVector3D::dotProduct(CP, v);
            float d2 = px * px + py * py;
            float R2 = R * R;
            if (d2 <= R2) return origin - v * R; // 容错保护
            float L = std::sqrt(d2 - R2);
            // 两个切点的 2D 坐标计算
            float tx1 = (R2 * px + R * L * py) / d2;
            float ty1 = (R2 * py - R * L * px) / d2;
            float tx2 = (R2 * px - R * L * py) / d2;
            float ty2 = (R2 * py + R * L * px) / d2;
            // 取 y 值较小的一个作为底部切点
            if (ty1 < ty2) return origin + u * tx1 + v * ty1;
            else return origin + u * tx2 + v * ty2;
            };

        QVector3D T_left = getBottomTangent(P_left);
        QVector3D T_right = getBottomTangent(P_right);

        // ==========================================
        // 3. 绘制外围的轮廓线 (包含顶点处 X 型延长线)
        // ==========================================
        float ext = sceneScale * 0.18f; // 线段端点延长量

        // 顶部的两根斜线 (两端都延长)
        QVector3D dirTL = (P_top - P_left).normalized();
        addLine(P_left - dirTL * ext, P_top + dirTL * ext);

        QVector3D dirTR = (P_top - P_right).normalized();
        addLine(P_right - dirTR * ext, P_top + dirTR * ext);

        // 底部的两根相切线 (从切点出发，穿过左右顶点并延长)
        QVector3D dirBL = (P_left - T_left).normalized();
        addLine(T_left, P_left + dirBL * ext);

        QVector3D dirBR = (P_right - T_right).normalized();
        addLine(T_right, P_right + dirBR * ext);

        // ==========================================
        // 4. 绘制法向剖面线 (Hatching)
        // ==========================================
        int hatchCount = 6;
        float hatchLen = sceneScale * 0.4f;

        // 左上边缘剖面线 (计算 2D 法向量，使其严格垂直于斜线)
        QVector3D E_L = P_top - P_left;
        QVector3D N_L = (u * (-QVector3D::dotProduct(E_L, v)) + v * QVector3D::dotProduct(E_L, u)).normalized();
        for (int i = 1; i <= hatchCount; ++i) {
            float t = static_cast<float>(i) / (hatchCount + 1);
            QVector3D base = P_left + E_L * t;
            addLine(base, base + N_L * hatchLen);
        }

        // 右上边缘剖面线
        QVector3D E_R = P_top - P_right;
        QVector3D N_R = (u * QVector3D::dotProduct(E_R, v) + v * (-QVector3D::dotProduct(E_R, u))).normalized();
        for (int i = 1; i <= hatchCount; ++i) {
            float t = static_cast<float>(i) / (hatchCount + 1);
            QVector3D base = P_right + E_R * t;
            addLine(base, base + N_R * hatchLen);
        }

        // ==========================================
        // 5. 数据装填与渲染
        // ==========================================
        auto symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);
        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;
        // 图片中的黄褐色
        symbol->_lineColor = QVector3D(153, 115, 0);
        symbol->_faceColor = QVector3D(153, 115, 0);
        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;
        // 打开文字显示，J001 居中显示在圆内
        //symbol->_txt_pos = { { symbolName, origin } };
        //symbol->_textColor = QVector3D(0, 0, 0);
        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildCylindricalSymbol(mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& axis,
        float sceneScale)
    {
        // 1. 处理轴线并建立局部正交基
        QVector3D nAxis = axis;
        if (nAxis.lengthSquared() < 1e-12f)
            nAxis = QVector3D(0, 0, 1);
        else
            nAxis.normalize();

        QVector3D u, v;
        buildOrthonormalBasis(nAxis, u, v);

        // 2. 符号基础尺度

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        // 辅助闭包：快速添加线段
        auto addLine = [&points, &lineIdx](const QVector3D& a, const QVector3D& b) {
            unsigned int i0 = static_cast<unsigned int>(points.size());
            points.append(a);
            points.append(b);
            lineIdx.push_back(i0);
            lineIdx.push_back(i0 + 1);
            };

        // 辅助闭包：快速绘制 3D 圆
        auto addCircle = [&](const QVector3D& center, float radius, int segments = 36) {
            int base = points.size();
            for (int i = 0; i < segments; ++i) {
                float theta = 2.0f * static_cast<float>(M_PI) * i / segments;
                points.append(center + u * (radius * cos(theta)) + v * (radius * sin(theta)));
            }
            for (int i = 0; i < segments; ++i) {
                lineIdx.push_back(base + i);
                lineIdx.push_back(base + (i + 1) % segments);
            }
            };

        // 3. 中心细长轴线
        float axisHalfL = sceneScale * 1.5f;
        addLine(origin - nAxis * axisHalfL, origin + nAxis * axisHalfL);

        // 4. 内侧小圆柱 (代表长轴/导轨，展示平动自由度)
        float rIn = sceneScale * 0.15f;
        float hIn = sceneScale * 1.1f;
        QVector3D topIn = origin + nAxis * hIn;
        QVector3D botIn = origin - nAxis * hIn;

        addCircle(topIn, rIn);
        addCircle(botIn, rIn);

        // 内侧圆柱母线 (左右两根最外侧轮廓线)
        addLine(topIn + u * rIn, botIn + u * rIn);
        addLine(topIn - u * rIn, botIn - u * rIn);

        // 5. 外侧大圆柱 (代表滑动套筒，展示转动和滑动)
        float rOut = sceneScale * 0.45f;
        float hOut = sceneScale * 0.6f;// 套筒较短
        QVector3D topOut = origin + nAxis * hOut;
        QVector3D botOut = origin - nAxis * hOut;

        addCircle(topOut, rOut);
        addCircle(botOut, rOut);

        // 外侧套筒母线 (画 4 根线增加立体感)
        addLine(topOut + u * rOut, botOut + u * rOut);
        addLine(topOut - u * rOut, botOut - u * rOut);
        addLine(topOut + v * rOut, botOut + v * rOut);
        addLine(topOut - v * rOut, botOut - v * rOut);

        // 6. 数据装填与渲染设置
        auto symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);
        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        // 保持统一工程黄褐色
        symbol->_lineColor = QVector3D(153, 115, 0);
        symbol->_faceColor = QVector3D(153, 115, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        // 文本稍微向外偏移，避免与套筒内部结构重叠
        //symbol->_txt_pos = { { symbolName, origin + u * rOut * 1.3f } };
        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildTranslationalSymbol(mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& axis,
        float sceneScale)
    {
        // 1. 处理轴线并建立局部正交基
        QVector3D nAxis = axis;
        if (nAxis.lengthSquared() < 1e-12f)
            nAxis = QVector3D(0, 0, 1);
        else
            nAxis.normalize();

        QVector3D u, v;
        buildOrthonormalBasis(nAxis, u, v);

        // 2. 符号基础尺度与长方体参数 (按图片比例，高度明显大于宽高)
        float halfH = sceneScale * 1.2f; //长方体的半高
        float halfW = sceneScale * 0.35f; //长方体底面的半宽 (沿 u 和 v)

        QVector3D topCenter = origin + nAxis * halfH;
        QVector3D botCenter = origin - nAxis * halfH;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        // 3. 计算顶面 4 个角点
        int topBase = points.size();
        points.append(topCenter + u * halfW + v * halfW); // 0: 右前
        points.append(topCenter - u * halfW + v * halfW); // 1: 左前
        points.append(topCenter - u * halfW - v * halfW); // 2: 左后
        points.append(topCenter + u * halfW - v * halfW); // 3: 右后

        // 4. 计算底面 4 个角点
        int botBase = points.size();
        points.append(botCenter + u * halfW + v * halfW); // 4
        points.append(botCenter - u * halfW + v * halfW); // 5
        points.append(botCenter - u * halfW - v * halfW); // 6
        points.append(botCenter + u * halfW - v * halfW); // 7

        // 5. 组装线段索引 (12条边)

        // 顶面边框线
        for (int i = 0; i < 4; ++i) {
            lineIdx.push_back(topBase + i);
            lineIdx.push_back(topBase + (i + 1) % 4);
        }

        // 底面边框线
        for (int i = 0; i < 4; ++i) {
            lineIdx.push_back(botBase + i);
            lineIdx.push_back(botBase + (i + 1) % 4);
        }

        // 四条侧棱线 (连接上下底面)
        for (int i = 0; i < 4; ++i) {
            lineIdx.push_back(topBase + i);
            lineIdx.push_back(botBase + i);
        }

        // 6. 数据装填与渲染设置
        auto symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);
        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        // 保持统一黄褐色
        symbol->_lineColor = QVector3D(153, 115, 0);
        symbol->_faceColor = QVector3D(153, 115, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        // 文字居中显示
        //symbol->_txt_pos = { { symbolName, origin } };
        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildScrewSymbol(mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& axis,
        float sceneScale)
    {
        // 1. 处理轴线并建立局部正交基
        QVector3D nAxis = axis;
        if (nAxis.lengthSquared() < 1e-12f)
            nAxis = QVector3D(0, 0, 1);
        else
            nAxis.normalize();

        QVector3D u, v;
        buildOrthonormalBasis(nAxis, u, v);

        // 2. 符号基础尺度 (修长的螺杆造型)
        float R = sceneScale * 0.35f;      // 螺旋螺纹的半径
        float halfH = sceneScale * 1.0f;   // 螺杆的半高 (比普通圆柱稍微修长一点)

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&points, &lineIdx](const QVector3D& a, const QVector3D& b) {
            unsigned int i0 = static_cast<unsigned int>(points.size());
            points.append(a);
            points.append(b);
            lineIdx.push_back(i0);
            lineIdx.push_back(i0 + 1);
            };

        // 3. 绘制中心长轴 (让中心线稍微出头一点)
        addLine(origin - nAxis * (halfH * 1.2f), origin + nAxis * (halfH * 1.2f));

        // 4. 绘制 3D 螺旋线 (Helix - 螺旋副的灵魂)
        int turns = 4; // 螺纹盘绕圈数
        int segmentsPerTurn = 36;
        int totalSegments = turns * segmentsPerTurn;
        int helixBase = points.size();

        for (int i = 0; i <= totalSegments; ++i) {
            float t = static_cast<float>(i) / totalSegments; // t 从 0 渐变到 1
            float theta = turns * 2.0f * static_cast<float>(M_PI) * t;
            float currentH = -halfH + 2.0f * halfH * t; // 高度从底面匀速平移到顶面

            // 生成纯正的 3D 螺旋空间点
            points.append(origin + nAxis * currentH + u * (R * cos(theta)) + v * (R * sin(theta)));

            // 连线
            if (i > 0) {
                lineIdx.push_back(helixBase + i - 1);
                lineIdx.push_back(helixBase + i);
            }
        }

        // 5. 绘制上下基准圆边界 (增强柱状立体感)
        int circleSegments = 36;
        int topBase = points.size();
        for (int i = 0; i < circleSegments; ++i) {
            float theta = 2.0f * static_cast<float>(M_PI) * i / circleSegments;
            points.append(origin + nAxis * halfH + u * (R * cos(theta)) + v * (R * sin(theta)));
        }
        for (int i = 0; i < circleSegments; ++i) {
            lineIdx.push_back(topBase + i);
            lineIdx.push_back(topBase + (i + 1) % circleSegments);
        }

        int botBase = points.size();
        for (int i = 0; i < circleSegments; ++i) {
            float theta = 2.0f * static_cast<float>(M_PI) * i / circleSegments;
            points.append(origin - nAxis * halfH + u * (R * cos(theta)) + v * (R * sin(theta)));
        }
        for (int i = 0; i < circleSegments; ++i) {
            lineIdx.push_back(botBase + i);
            lineIdx.push_back(botBase + (i + 1) % circleSegments);
        }

        // 6. 绘制左右两根包络外框线 (形成螺纹外侧包络面)
        addLine(origin - nAxis * halfH - u * R, origin + nAxis * halfH - u * R);
        addLine(origin - nAxis * halfH + u * R, origin + nAxis * halfH + u * R);

        // 7. 数据装填与渲染设置
        auto symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);
        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        // 统一工程褐黄色
        symbol->_lineColor = QVector3D(153, 115, 0);
        symbol->_faceColor = QVector3D(153, 115, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        // 文本放置在中心
        //symbol->_txt_pos = { { symbolName, origin } };
        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildRevoluteSymbol(mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& axis,
        float sceneScale)
    {
        // 1. 处理轴线并建立局部正交基
        QVector3D nAxis = axis;
        if (nAxis.lengthSquared() < 1e-12f)
            nAxis = QVector3D(0, 0, 1);
        else
            nAxis.normalize();

        QVector3D u, v;
        buildOrthonormalBasis(nAxis, u, v);

        // 2. 符号基础尺度与圆柱体参数
        float R = sceneScale * 0.45f;
        float halfH = sceneScale * 0.8f;

        // 沿 axis 方向定位上下底面的圆心
        QVector3D topCenter = origin + nAxis * halfH;
        QVector3D botCenter = origin - nAxis * halfH;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        // 3. 绘制顶部完美 3D 圆 (位于与 axis 垂直的 u-v 平面)
        int segments = 40; // 必须能被下面的 numVerticalLines 整除以保证均匀
        int topBase = points.size();
        for (int i = 0; i < segments; ++i) {
            float theta = 2.0f * static_cast<float>(M_PI) * i / segments;
            // u * cos + v * sin 保证了在 3D 空间中这是一个正圆
            points.append(topCenter + u * (R * cos(theta)) + v * (R * sin(theta)));
        }
        for (int i = 0; i < segments; ++i) {
            lineIdx.push_back(topBase + i);
            lineIdx.push_back(topBase + (i + 1) % segments);
        }

        // 4. 绘制底部完美 3D 圆
        int botBase = points.size();
        for (int i = 0; i < segments; ++i) {
            float theta = 2.0f * static_cast<float>(M_PI) * i / segments;
            points.append(botCenter + u * (R * cos(theta)) + v * (R * sin(theta)));
        }
        for (int i = 0; i < segments; ++i) {
            lineIdx.push_back(botBase + i);
            lineIdx.push_back(botBase + (i + 1) % segments);
        }

        // 5. 绘制垂直母线 (连接上下圆的对应点)
        // 根据图片，圆柱大概有 8 根垂直线包围
        int numVerticalLines = 8;
        int step = segments / numVerticalLines;
        for (int i = 0; i < numVerticalLines; ++i) {
            // 直接复用圆周上的顶点索引，渲染最干净，不会有 Z-Fighting
            int tIdx = topBase + i * step;
            int bIdx = botBase + i * step;
            lineIdx.push_back(tIdx);
            lineIdx.push_back(bIdx);
        }

        // 6. 数据装填与渲染设置
        auto symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);
        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        // 保持与之前统一的工程制图黄褐色
        symbol->_lineColor = QVector3D(153, 115, 0);
        symbol->_faceColor = QVector3D(153, 115, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        // 文字居中显示 (例如 "J002" 在圆柱体正中心)
        //symbol->_txt_pos = { { symbolName, origin } };
        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildSphericalSymbol(mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& axis,
        float sceneScale)
    {
        // 1. 处理轴线并建立局部正交基
        QVector3D nAxis = axis;
        if (nAxis.lengthSquared() < 1e-12f)
            nAxis = QVector3D(0, 0, 1);
        else
            nAxis.normalize();

        QVector3D u, v;
        buildOrthonormalBasis(nAxis, u, v);

        // 2. 符号基础尺度 (一个完美的球形线框)
        float R = sceneScale * 0.6f;// 正交圆环的半径

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        // 快速添加线段闭包
        auto addLine = [&points, &lineIdx](const QVector3D& a, const QVector3D& b) {
            unsigned int i0 = static_cast<unsigned int>(points.size());
            points.append(a);
            points.append(b);
            lineIdx.push_back(i0);
            lineIdx.push_back(i0 + 1);
            };

        // 快速绘制 3D 圆的闭包 (接受任意两个正交基向量来确定平面)
        auto addOrthogonalCircle = [&](const QVector3D& center, const QVector3D& dir1, const QVector3D& dir2, float radius, int segments = 36) {
            int base = points.size();
            for (int i = 0; i < segments; ++i) {
                float theta = 2.0f * static_cast<float>(M_PI) * i / segments;
                points.append(center + dir1 * (radius * cos(theta)) + dir2 * (radius * sin(theta)));
            }
            for (int i = 0; i < segments; ++i) {
                lineIdx.push_back(base + i);
                lineIdx.push_back(base + (i + 1) % segments);
            }
            };

        // 3. 绘制三个互相正交的圆环 (代表 X, Y, Z 三个方向的旋转自由度)
        addOrthogonalCircle(origin, u, v, R);// 位于 u-v 平面 (法向为 nAxis)
        addOrthogonalCircle(origin, nAxis, u, R);// 位于 nAxis-u 平面 (法向为 v)
        addOrthogonalCircle(origin, v, nAxis, R);// 位于 v-nAxis 平面 (法向为 u)

        // 4. 绘制中心十字星交叉轴 (强化球心位置和三维空间感)
        //float crossHalfL = R * 1.25f; // 轴线稍微超出圆环一点点
        //addLine(origin - u * crossHalfL, origin + u * crossHalfL);
        //addLine(origin - v * crossHalfL, origin + v * crossHalfL);
        //addLine(origin - nAxis * crossHalfL, origin + nAxis * crossHalfL);

        // 5. 数据装填与渲染设置
        auto symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);
        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        // 统一工程褐黄色
        symbol->_lineColor = QVector3D(153, 115, 0);
        symbol->_faceColor = QVector3D(153, 115, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        // 文本向右上角斜向偏移，以免被三个圆环的密集中心挡住
        //QVector3D textOffset = (u + v).normalized() * (R * 1.2f);
        //symbol->_txt_pos = { { symbolName, origin + textOffset } };
        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildPointOnPlaneSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& planeNormal,
        float sceneScale)
    {
        QVector3D normal = planeNormal;

        if (normal.lengthSquared() < EPS) {
            normal = QVector3D(0.0f, 0.0f, 1.0f);
        }
        else {
            normal.normalize();
        }

        QVector3D u;
        QVector3D v;
        buildOrthonormalBasis(normal, u, v);

        const float planeHalfWidth = sceneScale * 0.95f;
        const float planeHalfHeight = sceneScale * 0.62f;

        const float pointRadius = sceneScale * 0.11f;
        const float normalLength = sceneScale * 0.75f;
        const float arrowLength = sceneScale * 0.16f;
        const float arrowWidth = sceneScale * 0.09f;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&](const QVector3D& start,
            const QVector3D& end)
            {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(start);
                points.append(end);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        auto addCircle = [&](const QVector3D& center,
            const QVector3D& axis,
            float radius,
            int segments)
            {
                QVector3D circleU;
                QVector3D circleV;
                buildOrthonormalBasis(axis, circleU, circleV);

                QVector3D previous =
                    center + circleU * radius;

                for (int i = 1; i <= segments; ++i) {
                    const float angle =
                        2.0f * PI * static_cast<float>(i)
                        / static_cast<float>(segments);

                    const QVector3D current =
                        center
                        + circleU * (std::cos(angle) * radius)
                        + circleV * (std::sin(angle) * radius);

                    addLine(previous, current);
                    previous = current;
                }
            };

        // ============================================================
        // 1. 平面外轮廓
        // ============================================================
        const QVector3D leftBottom =
            origin - u * planeHalfWidth - v * planeHalfHeight;

        const QVector3D rightBottom =
            origin + u * planeHalfWidth - v * planeHalfHeight;

        const QVector3D rightTop =
            origin + u * planeHalfWidth + v * planeHalfHeight;

        const QVector3D leftTop =
            origin - u * planeHalfWidth + v * planeHalfHeight;

        addLine(leftBottom, rightBottom);
        addLine(rightBottom, rightTop);
        addLine(rightTop, leftTop);
        addLine(leftTop, leftBottom);

        // ============================================================
        // 2. 平面网格
        // ============================================================
        for (int i = -2; i <= 2; ++i) {
            if (i == 0) {
                continue;
            }

            const float ratio = static_cast<float>(i) / 3.0f;

            addLine(
                origin + u * (planeHalfWidth * ratio)
                - v * planeHalfHeight,
                origin + u * (planeHalfWidth * ratio)
                + v * planeHalfHeight);
        }

        for (int i = -1; i <= 1; ++i) {
            if (i == 0) {
                continue;
            }

            const float ratio = static_cast<float>(i) / 2.0f;

            addLine(
                origin - u * planeHalfWidth
                + v * (planeHalfHeight * ratio),
                origin + u * planeHalfWidth
                + v * (planeHalfHeight * ratio));
        }

        // ============================================================
        // 3. 中心点标记
        // ============================================================
        const QVector3D pointCenter =
            origin + normal * (sceneScale * 0.015f);

        addCircle(
            pointCenter,
            normal,
            pointRadius,
            20);

        addLine(
            pointCenter - u * pointRadius,
            pointCenter + u * pointRadius);

        addLine(
            pointCenter - v * pointRadius,
            pointCenter + v * pointRadius);

        // ============================================================
        // 4. 平面法向箭头
        // ============================================================
        const QVector3D arrowStart =
            pointCenter + normal * pointRadius;

        const QVector3D arrowTip =
            origin + normal * normalLength;

        addLine(arrowStart, arrowTip);

        addLine(
            arrowTip,
            arrowTip
            - normal * arrowLength
            + u * arrowWidth);

        addLine(
            arrowTip,
            arrowTip
            - normal * arrowLength
            - u * arrowWidth);

        // ============================================================
        // 5. 符号数据
        // ============================================================
        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        //symbol->_txt_pos = {
        //    {
        //        symbolName,
        //        origin
        //            + u * (planeHalfWidth * 0.85f)
        //            + v * (planeHalfHeight * 0.85f)
        //            + normal * (sceneScale * 0.08f)
        //    }
        //};

        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildPointOnLineSymbol(mxr::mSymbolController* symbolController, const QString& symbolName, const QVector3D& origin, const QVector3D& lineDirection, float sceneScale)
    {
        // ============================================================
        // 1. 建立局部坐标系
        // ============================================================
        QVector3D axis = lineDirection;

        if (axis.lengthSquared() < EPS) {
            axis = QVector3D(1.0f, 0.0f, 0.0f);
        }
        else {
            axis.normalize();
        }

        QVector3D u;
        QVector3D v;
        buildOrthonormalBasis(axis, u, v);

        const float lineHalfLength = sceneScale * 1.15f;
        const float endTickLength = sceneScale * 0.24f;
        const float pointRadius = sceneScale * 0.16f;
        const float arrowHalfLength = sceneScale * 0.62f;
        const float arrowLength = sceneScale * 0.15f;
        const float arrowWidth = sceneScale * 0.08f;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&](const QVector3D& start,
            const QVector3D& end)
            {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(start);
                points.append(end);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        auto addCircle = [&](const QVector3D& center,
            const QVector3D& normal,
            float radius,
            int segments)
            {
                QVector3D circleU;
                QVector3D circleV;
                buildOrthonormalBasis(normal, circleU, circleV);

                QVector3D previous = center + circleU * radius;

                for (int i = 1; i <= segments; ++i) {
                    const float angle =
                        2.0f * PI * static_cast<float>(i)
                        / static_cast<float>(segments);

                    const QVector3D current =
                        center
                        + circleU * (std::cos(angle) * radius)
                        + circleV * (std::sin(angle) * radius);

                    addLine(previous, current);
                    previous = current;
                }
            };

        auto addArrowHead = [&](const QVector3D& tip,
            const QVector3D& direction,
            const QVector3D& side)
            {
                QVector3D dir = direction.normalized();
                QVector3D sideDir = side.normalized();

                const QVector3D back =
                    tip - dir * arrowLength;

                addLine(
                    tip,
                    back + sideDir * arrowWidth);

                addLine(
                    tip,
                    back - sideDir * arrowWidth);
            };

        // ============================================================
        // 2. 导向直线
        // ============================================================
        const QVector3D lineStart =
            origin - axis * lineHalfLength;

        const QVector3D lineEnd =
            origin + axis * lineHalfLength;

        addLine(lineStart, lineEnd);

        // 两端横向标记，使无限直线方向更加明显
        addLine(
            lineStart - u * (endTickLength * 0.5f),
            lineStart + u * (endTickLength * 0.5f));

        addLine(
            lineEnd - u * (endTickLength * 0.5f),
            lineEnd + u * (endTickLength * 0.5f));

        // ============================================================
        // 3. 中心约束点
        // ============================================================
        addCircle(
            origin,
            axis,
            pointRadius,
            20);

        addLine(
            origin - u * pointRadius,
            origin + u * pointRadius);

        addLine(
            origin - v * pointRadius,
            origin + v * pointRadius);

        // 中心小菱形，避免只有圆圈时看起来像旋转副
        const float diamondRadius = pointRadius * 0.55f;

        addLine(
            origin + u * diamondRadius,
            origin + v * diamondRadius);

        addLine(
            origin + v * diamondRadius,
            origin - u * diamondRadius);

        addLine(
            origin - u * diamondRadius,
            origin - v * diamondRadius);

        addLine(
            origin - v * diamondRadius,
            origin + u * diamondRadius);

        // ============================================================
        // 4. 沿直线双向移动箭头
        // ============================================================
        const QVector3D arrowStart =
            origin - axis * arrowHalfLength;

        const QVector3D arrowEnd =
            origin + axis * arrowHalfLength;

        addLine(arrowStart, arrowEnd);

        addArrowHead(
            arrowEnd,
            axis,
            u);

        addArrowHead(
            arrowStart,
            -axis,
            u);

        // ============================================================
        // 5. 数据装填
        // ============================================================
        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        //symbol->_txt_pos = {
        //    {
        //        symbolName,
        //        origin
        //            + u * (sceneScale * 0.55f)
        //            + v * (sceneScale * 0.28f)
        //    }
        //};

        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildPointOnCurveSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& curveDirection,
        float sceneScale)
    {
        // ============================================================
        // 1. 建立局部坐标系：axis 为曲线在原点处的切向
        // ============================================================
        QVector3D axis = curveDirection;

        if (axis.lengthSquared() < EPS) {
            axis = QVector3D(1.0f, 0.0f, 0.0f);
        }
        else {
            axis.normalize();
        }

        QVector3D u;
        QVector3D v;
        buildOrthonormalBasis(axis, u, v);

        // 圆弧半径与张角：弧在 axis-v 平面内弯曲穿过原点，圆心位于 -u 侧，
        // 原点处弧切向恰为 axis（与点线副的直导轨形成直观区分）
        const float arcRadius = sceneScale * 1.6f;
        const float arcHalfAngle = 0.62f;
        const int arcSegments = 28;

        const float pointRadius = sceneScale * 0.16f;
        const float arrowHalfLength = sceneScale * 0.5f;
        const float arrowLength = sceneScale * 0.15f;
        const float arrowWidth = sceneScale * 0.08f;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&](const QVector3D& start,
            const QVector3D& end)
            {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(start);
                points.append(end);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        // ============================================================
        // 2. 弯曲导轨（圆弧）
        // ============================================================
        const QVector3D arcCenter = origin - u * arcRadius;

        auto arcPoint = [&](float angle) -> QVector3D {
            // angle=0 对应原点；角度增大沿 +axis 方向前进
            return arcCenter
                + u * (arcRadius * std::cos(angle))
                + axis * (arcRadius * std::sin(angle));
        };

        QVector3D previous = arcPoint(-arcHalfAngle);
        for (int i = 1; i <= arcSegments; ++i) {
            const float angle =
                -arcHalfAngle
                + (2.0f * arcHalfAngle * static_cast<float>(i)
                    / static_cast<float>(arcSegments));

            const QVector3D current = arcPoint(angle);
            addLine(previous, current);
            previous = current;
        }

        // 弧两端横向收尾短线（同点线副两端标记的语言）
        {
            const float endTick = sceneScale * 0.14f;
            const QVector3D startPt = arcPoint(-arcHalfAngle);
            const QVector3D endPt = arcPoint(arcHalfAngle);

            addLine(
                startPt - u * (endTick * 0.5f),
                startPt + u * (endTick * 0.5f));

            addLine(
                endPt - u * (endTick * 0.5f),
                endPt + u * (endTick * 0.5f));
        }

        // ============================================================
        // 3. 中心约束点（圆 + 十字 + 小菱形，与点线副同族语言）
        // ============================================================
        {
            QVector3D circleU;
            QVector3D circleV;
            buildOrthonormalBasis(u, circleU, circleV);

            QVector3D prev = origin + circleU * pointRadius;

            for (int i = 1; i <= 20; ++i) {
                const float angle =
                    2.0f * PI * static_cast<float>(i) / 20.0f;

                const QVector3D current =
                    origin
                    + circleU * (std::cos(angle) * pointRadius)
                    + circleV * (std::sin(angle) * pointRadius);

                addLine(prev, current);
                prev = current;
            }

            // 圆平面取弧所在平面（法向 u），十字与菱形画在 v-axis 平面内
            addLine(
                origin - v * pointRadius,
                origin + v * pointRadius);

            addLine(
                origin - axis * pointRadius,
                origin + axis * pointRadius);

            const float diamondRadius = pointRadius * 0.55f;

            addLine(
                origin + axis * diamondRadius,
                origin + v * diamondRadius);

            addLine(
                origin + v * diamondRadius,
                origin - axis * diamondRadius);

            addLine(
                origin - axis * diamondRadius,
                origin - v * diamondRadius);

            addLine(
                origin - v * diamondRadius,
                origin + axis * diamondRadius);
        }

        // ============================================================
        // 4. 沿切向双向滑动箭头
        // ============================================================
        const QVector3D arrowStart = origin - axis * arrowHalfLength;
        const QVector3D arrowEnd = origin + axis * arrowHalfLength;

        addLine(arrowStart, arrowEnd);

        addLine(
            arrowEnd,
            arrowEnd - axis * arrowLength + v * arrowWidth);

        addLine(
            arrowEnd,
            arrowEnd - axis * arrowLength - v * arrowWidth);

        addLine(
            arrowStart,
            arrowStart + axis * arrowLength + v * arrowWidth);

        addLine(
            arrowStart,
            arrowStart + axis * arrowLength - v * arrowWidth);

        // ============================================================
        // 5. 数据装填
        // ============================================================
        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildCurveOnCurveSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& contactTangent,
        float sceneScale)
    {
        QVector3D axis = contactTangent;

        if (axis.lengthSquared() < EPS) {
            axis = QVector3D(1.0f, 0.0f, 0.0f);
        }
        else {
            axis.normalize();
        }

        QVector3D u;
        QVector3D v;
        buildOrthonormalBasis(axis, u, v);

        // 两条相切曲线：上曲线（动作体）在原点处凹向下，下曲线（基本体）凹向上，
        // 两曲线在原点相切（切向均为 axis），形成"曲线贴曲线"的直观语义
        const float curveHalfLen = sceneScale * 0.9f;
        const float curveWarp = sceneScale * 0.35f;
        const int curveSegments = 20;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&](const QVector3D& start,
            const QVector3D& end)
            {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(start);
                points.append(end);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        // 上曲线（动作体）：y = -warp * (1 - (s/半长)²)，原点处为波谷（凹向下）
        // 下曲线（基本体）：y = +warp * (1 - (s/半长)²)，原点处为波峰（凹向上）
        // 两曲线在原点相切（切向均为 axis）
        auto curvePoint = [&](float s, float sign) -> QVector3D {
            const float ratio = s / curveHalfLen;
            const float h = sign * curveWarp * (1.0f - ratio * ratio);
            return origin + axis * s + v * h;
        };

        // 上曲线
        QVector3D prev = curvePoint(-curveHalfLen, -1.0f);
        for (int i = 1; i <= curveSegments; ++i) {
            const float s = -curveHalfLen + (2.0f * curveHalfLen * static_cast<float>(i) / static_cast<float>(curveSegments));
            const QVector3D cur = curvePoint(s, -1.0f);
            addLine(prev, cur);
            prev = cur;
        }

        // 下曲线
        prev = curvePoint(-curveHalfLen, 1.0f);
        for (int i = 1; i <= curveSegments; ++i) {
            const float s = -curveHalfLen + (2.0f * curveHalfLen * static_cast<float>(i) / static_cast<float>(curveSegments));
            const QVector3D cur = curvePoint(s, 1.0f);
            addLine(prev, cur);
            prev = cur;
        }

        // 两曲线两端横向收尾短线
        {
            const float endTick = sceneScale * 0.12f;
            const QVector3D upperStart = curvePoint(-curveHalfLen, -1.0f);
            const QVector3D upperEnd = curvePoint(curveHalfLen, -1.0f);
            const QVector3D lowerStart = curvePoint(-curveHalfLen, 1.0f);
            const QVector3D lowerEnd = curvePoint(curveHalfLen, 1.0f);

            addLine(upperStart - u * (endTick * 0.5f), upperStart + u * (endTick * 0.5f));
            addLine(upperEnd - u * (endTick * 0.5f), upperEnd + u * (endTick * 0.5f));
            addLine(lowerStart - u * (endTick * 0.5f), lowerStart + u * (endTick * 0.5f));
            addLine(lowerEnd - u * (endTick * 0.5f), lowerEnd + u * (endTick * 0.5f));
        }

        // 接触点（原点）：圆 + 十字
        {
            const float pointRadius = sceneScale * 0.14f;
            QVector3D circleU;
            QVector3D circleV;
            buildOrthonormalBasis(v, circleU, circleV);

            QVector3D prevPt = origin + circleU * pointRadius;
            for (int i = 1; i <= 20; ++i) {
                const float angle = 2.0f * PI * static_cast<float>(i) / 20.0f;
                const QVector3D current =
                    origin
                    + circleU * (std::cos(angle) * pointRadius)
                    + circleV * (std::sin(angle) * pointRadius);
                addLine(prevPt, current);
                prevPt = current;
            }

            addLine(origin - u * pointRadius, origin + u * pointRadius);
            addLine(origin - axis * pointRadius, origin + axis * pointRadius);
        }

        // 数据装填
        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildPointOnSurfaceSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& planeNormal,
        float sceneScale)
    {
        QVector3D normal = planeNormal;

        if (normal.lengthSquared() < EPS) {
            normal = QVector3D(0.0f, 0.0f, 1.0f);
        }
        else {
            normal.normalize();
        }

        QVector3D u;
        QVector3D v;
        buildOrthonormalBasis(normal, u, v);

        const float halfWidth = sceneScale * 0.95f;
        const float halfHeight = sceneScale * 0.62f;
        const float pointRadius = sceneScale * 0.11f;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&](const QVector3D& start,
            const QVector3D& end)
            {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(start);
                points.append(end);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        // ============================================================
        // 1. 起伏离散网格面（5×5 顶点阵，枕形凹陷）
        //    区别于点面副的"平板矩形+规则网格线"：顶点按连续波形起伏，
        //    细分网格 + 三角化对角线表达"离散曲面片"，中心顶点为波谷
        // ============================================================
        const int gridN = 5;
        const float warp = sceneScale * 0.42f;

        // h = -A·(1-s²)·(1-t²)：四周翘起、中心凹陷成碗形波谷
        QVector3D grid[gridN][gridN];
        for (int i = 0; i < gridN; ++i) {
            for (int j = 0; j < gridN; ++j) {
                const float su = -halfWidth + (2.0f * halfWidth * static_cast<float>(i) / static_cast<float>(gridN - 1));
                const float sv = -halfHeight + (2.0f * halfHeight * static_cast<float>(j) / static_cast<float>(gridN - 1));
                const float s = 2.0f * static_cast<float>(i) / static_cast<float>(gridN - 1) - 1.0f;
                const float t = 2.0f * static_cast<float>(j) / static_cast<float>(gridN - 1) - 1.0f;
                const float h = -warp * (1.0f - s * s) * (1.0f - t * t);
                grid[i][j] = origin + u * su + v * sv + normal * h;
            }
        }

        // 横竖网格线（相邻顶点连线）
        for (int i = 0; i < gridN; ++i) {
            for (int j = 0; j < gridN; ++j) {
                if (i + 1 < gridN) addLine(grid[i][j], grid[i + 1][j]);
                if (j + 1 < gridN) addLine(grid[i][j], grid[i][j + 1]);
            }
        }

        // 每个四边形一条交替方向的对角线（三角面片化的视觉语义）
        for (int i = 0; i + 1 < gridN; ++i) {
            for (int j = 0; j + 1 < gridN; ++j) {
                if ((i + j) % 2 == 0)
                    addLine(grid[i][j], grid[i + 1][j + 1]);
                else
                    addLine(grid[i + 1][j], grid[i][j + 1]);
            }
        }

        // ============================================================
        // 2. 中心约束点（圆 + 十字，位于波谷中心顶点上）
        // ============================================================
        const QVector3D pointCenter =
            grid[(gridN - 1) / 2][(gridN - 1) / 2]
            + normal * (sceneScale * 0.015f);

        {
            QVector3D circleU;
            QVector3D circleV;
            buildOrthonormalBasis(normal, circleU, circleV);

            QVector3D prev = pointCenter + circleU * pointRadius;

            for (int i = 1; i <= 20; ++i) {
                const float angle =
                    2.0f * PI * static_cast<float>(i) / 20.0f;

                const QVector3D current =
                    pointCenter
                    + circleU * (std::cos(angle) * pointRadius)
                    + circleV * (std::sin(angle) * pointRadius);

                addLine(prev, current);
                prev = current;
            }
        }

        addLine(
            pointCenter - u * pointRadius,
            pointCenter + u * pointRadius);

        addLine(
            pointCenter - v * pointRadius,
            pointCenter + v * pointRadius);

        // ============================================================
        // 4. 数据装填
        // ============================================================
        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildUniversalSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& spinAxisAction, // 运动体自旋矢量
        const QVector3D& crossPinAction, // 运动体十字销矢量
        const QVector3D& spinAxisBase,   // 基本体自旋矢量
        const QVector3D& crossPinBase,   // 基本体十字销矢量
        float sceneScale)
    {
        // ============================================================
        // 1. 安全归一化
        // ============================================================
        auto safeNormalize = [&](const QVector3D& value,
            const QVector3D& fallback) -> QVector3D {
                if (value.lengthSquared() <= EPS) {
                    return fallback;
                }
                return value.normalized();
            };

        auto makePerpendicular = [&](const QVector3D& axis) -> QVector3D {
            QVector3D helper;

            if (qAbs(axis.x()) < 0.8f) {
                helper = QVector3D(1.0f, 0.0f, 0.0f);
            }
            else if (qAbs(axis.y()) < 0.8f) {
                helper = QVector3D(0.0f, 1.0f, 0.0f);
            }
            else {
                helper = QVector3D(0.0f, 0.0f, 1.0f);
            }

            QVector3D result = QVector3D::crossProduct(axis, helper);

            if (result.lengthSquared() <= EPS) {
                return QVector3D(1.0f, 0.0f, 0.0f);
            }

            return result.normalized();
            };

        QVector3D sA = safeNormalize(
            spinAxisAction,
            QVector3D(1.0f, 0.0f, 0.0f));

        QVector3D sB = safeNormalize(
            spinAxisBase,
            QVector3D(0.0f, 1.0f, 0.0f));

        /*
         * 十字销方向必须位于对应自旋轴的垂直平面内。
         * 这里只修正符号绘制方向，不修改用户输入参数。
         */
        auto orthogonalizeCrossPin =
            [&](const QVector3D& crossPin,
                const QVector3D& spinAxis) -> QVector3D {
                    QVector3D result =
                        crossPin
                        - spinAxis
                        * QVector3D::dotProduct(crossPin, spinAxis);

                    if (result.lengthSquared() <= EPS) {
                        return makePerpendicular(spinAxis);
                    }

                    return result.normalized();
            };

        QVector3D cA = orthogonalizeCrossPin(crossPinAction, sA);
        QVector3D cB = orthogonalizeCrossPin(crossPinBase, sB);

        // ============================================================
        // 2. 符号尺寸
        // ============================================================
        // 中央十字销
        const float pinHalfLength = sceneScale * 0.36f;
        const float pinRadius = sceneScale * 0.075f;

        // 两个半圆叉架
        const float yokeInnerRadius = sceneScale * 0.54f;
        const float yokeOuterRadius = sceneScale * 0.64f;

        // 从叉架中部向外延伸的自旋轴线
        const float spinLineLength = sceneScale * 1.20f;

        constexpr int CIRCLE_SEGMENTS = 12;
        constexpr int ARC_SEGMENTS = 24;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        // ============================================================
        // 3. 基础绘图函数
        // ============================================================
        auto addLine = [&](const QVector3D& start,
            const QVector3D& end) {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(start);
                points.append(end);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        auto addCircle = [&](const QVector3D& center,
            const QVector3D& normal,
            float radius) {
                QVector3D u = makePerpendicular(normal);
                QVector3D v =
                    QVector3D::crossProduct(normal, u).normalized();

                QVector3D previous = center + u * radius;

                for (int i = 1; i <= CIRCLE_SEGMENTS; ++i) {
                    const float angle =
                        2.0f * PI * static_cast<float>(i)
                        / static_cast<float>(CIRCLE_SEGMENTS);

                    const QVector3D current =
                        center
                        + u * (std::cos(angle) * radius)
                        + v * (std::sin(angle) * radius);

                    addLine(previous, current);
                    previous = current;
                }
            };

        /*
         * 绘制一根以 origin 为中心的线框圆柱。
         * 圆柱轴线就是十字销方向。
         */
        auto addCenteredCylinder = [&](const QVector3D& axis,
            float halfLength,
            float radius) {
                const QVector3D start = origin - axis * halfLength;
                const QVector3D end = origin + axis * halfLength;

                QVector3D u = makePerpendicular(axis);
                QVector3D v =
                    QVector3D::crossProduct(axis, u).normalized();

                addCircle(start, axis, radius);
                addCircle(end, axis, radius);

                // 只保留四条母线，避免中心区域过密
                addLine(start + u * radius, end + u * radius);
                addLine(start - u * radius, end - u * radius);
                addLine(start + v * radius, end + v * radius);
                addLine(start - v * radius, end - v * radius);
            };

        /*
         * 在 crossAxis 与 spinAxis 构成的平面内绘制半圆。
         *
         * theta = 0  ：+crossAxis
         * theta = PI ：-crossAxis
         * 中部沿 +spinAxis 方向鼓出
         */
        auto addSemiArc = [&](const QVector3D& spinAxis,
            const QVector3D& crossAxis,
            float radius) {
                QVector3D previous =
                    origin + crossAxis * radius;

                for (int i = 1; i <= ARC_SEGMENTS; ++i) {
                    const float angle =
                        PI * static_cast<float>(i)
                        / static_cast<float>(ARC_SEGMENTS);

                    const QVector3D current =
                        origin
                        + crossAxis * (std::cos(angle) * radius)
                        + spinAxis * (std::sin(angle) * radius);

                    addLine(previous, current);
                    previous = current;
                }
            };

        /*
         * 绘制一个符号化的 U 形叉架。
         */
        auto addYoke = [&](const QVector3D& spinAxis,
            const QVector3D& crossAxis) {
                // 内外两条半圆形成叉架厚度
                addSemiArc(spinAxis, crossAxis, yokeInnerRadius);
                addSemiArc(spinAxis, crossAxis, yokeOuterRadius);

                // 半圆两端封口
                addLine(
                    origin + crossAxis * yokeInnerRadius,
                    origin + crossAxis * yokeOuterRadius);

                addLine(
                    origin - crossAxis * yokeInnerRadius,
                    origin - crossAxis * yokeOuterRadius);

                // 中央十字销到叉架两端的连接线
                addLine(
                    origin + crossAxis * pinHalfLength,
                    origin + crossAxis * yokeInnerRadius);

                addLine(
                    origin - crossAxis * pinHalfLength,
                    origin - crossAxis * yokeInnerRadius);

                // 自旋方向只画中心线，不再画成长圆管
                addLine(
                    origin + spinAxis * yokeOuterRadius,
                    origin + spinAxis * spinLineLength);
            };

        // ============================================================
        // 4. 绘制万向节
        // ============================================================

        // 中央十字轴的两根销
        addCenteredCylinder(cA, pinHalfLength, pinRadius);
        addCenteredCylinder(cB, pinHalfLength, pinRadius);

        // 两个方向不同的半圆叉架
        addYoke(sA, cA);
        addYoke(sB, cB);

        // ============================================================
        // 5. 填充符号数据
        // ============================================================
        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(153, 115, 0);
        symbol->_faceColor = QVector3D(153, 115, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        // ============================================================
        // 6. 文本位置：移到符号平面外侧，避免压在中心十字销上
        // ============================================================
        QVector3D textDirection =
            QVector3D::crossProduct(sA, cA);

        if (textDirection.lengthSquared() <= EPS) {
            textDirection = QVector3D::crossProduct(cA, cB);
        }

        if (textDirection.lengthSquared() <= EPS) {
            textDirection = makePerpendicular(sA);
        }

        textDirection.normalize();

        //symbol->_txt_pos = {
        //    {
        //        symbolName,
        //        origin + textDirection * (sceneScale * 0.82f)
        //    }
        //};

        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildRackPinionSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& gearAxis,
        const QVector3D& rackDirection,
        float pitchRadius,
        float sceneScale)
    {
        // ============================================================
        // 1. 建立齿轮齿条坐标系
        // ============================================================
        QVector3D axis = gearAxis;

        if (axis.lengthSquared() < EPS) {
            axis = QVector3D(0.0f, 0.0f, 1.0f);
        }
        else {
            axis.normalize();
        }

        // 齿条方向必须位于垂直于齿轮轴的平面中
        QVector3D rackDir =
            rackDirection
            - axis * QVector3D::dotProduct(rackDirection, axis);

        if (rackDir.lengthSquared() < EPS) {
            QVector3D temp;
            buildOrthonormalBasis(axis, rackDir, temp);
        }
        else {
            rackDir.normalize();
        }

        // 从齿轮中心指向齿条的方向
        QVector3D contactNormal =
            QVector3D::crossProduct(axis, rackDir);

        if (contactNormal.lengthSquared() < EPS) {
            QVector3D temp;
            buildOrthonormalBasis(axis, rackDir, temp);
            contactNormal = temp;
        }
        else {
            contactNormal.normalize();
        }

        /*
         * pitchRadius 是显示节圆半径。
         * 为避免实际模型尺寸过大或过小导致图标不可见，对显示尺寸进行限制。
         */
        float visualPitchRadius = pitchRadius;

        if (visualPitchRadius <= EPS) {
            visualPitchRadius = sceneScale * 0.52f;
        }

        const float minRadius = sceneScale * 0.38f;
        const float maxRadius = sceneScale * 0.70f;

        if (visualPitchRadius < minRadius) {
            visualPitchRadius = minRadius;
        }
        else if (visualPitchRadius > maxRadius) {
            visualPitchRadius = maxRadius;
        }

        const float gearRootRadius = visualPitchRadius * 0.84f;
        const float gearOuterRadius = visualPitchRadius * 1.14f;
        const float hubRadius = visualPitchRadius * 0.28f;

        const float rackHalfLength = visualPitchRadius * 1.65f;
        const float rackToothHeight = visualPitchRadius * 0.25f;
        const float rackBodyHeight = visualPitchRadius * 0.30f;

        const float rackTipOffset =
            gearOuterRadius * 0.96f;

        const float rackRootOffset =
            rackTipOffset + rackToothHeight;

        const float rackBackOffset =
            rackRootOffset + rackBodyHeight;

        constexpr int GEAR_TEETH = 12;
        constexpr int RACK_TEETH = 7;
        constexpr int CIRCLE_SEGMENTS = 36;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&](const QVector3D& start,
            const QVector3D& end)
            {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(start);
                points.append(end);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        auto addCircle = [&](const QVector3D& center,
            const QVector3D& normal,
            float radius,
            int segments)
            {
                QVector3D circleU;
                QVector3D circleV;
                buildOrthonormalBasis(normal, circleU, circleV);

                QVector3D previous =
                    center + circleU * radius;

                for (int i = 1; i <= segments; ++i) {
                    const float angle =
                        2.0f * PI * static_cast<float>(i)
                        / static_cast<float>(segments);

                    const QVector3D current =
                        center
                        + circleU * (std::cos(angle) * radius)
                        + circleV * (std::sin(angle) * radius);

                    addLine(previous, current);
                    previous = current;
                }
            };

        // ============================================================
        // 2. 绘制齿轮外齿
        // ============================================================
        QVector3D previousGearPoint;

        for (int i = 0; i <= GEAR_TEETH * 4; ++i) {
            const int wrappedIndex =
                i % (GEAR_TEETH * 4);

            const float angle =
                2.0f * PI
                * static_cast<float>(wrappedIndex)
                / static_cast<float>(GEAR_TEETH * 4);

            const int toothPhase = wrappedIndex % 4;

            const float radius =
                (toothPhase == 1 || toothPhase == 2)
                ? gearOuterRadius
                : gearRootRadius;

            const QVector3D current =
                origin
                + rackDir * (std::cos(angle) * radius)
                + contactNormal * (std::sin(angle) * radius);

            if (i > 0) {
                addLine(previousGearPoint, current);
            }

            previousGearPoint = current;
        }

        // 节圆
        addCircle(
            origin,
            axis,
            visualPitchRadius,
            CIRCLE_SEGMENTS);

        // 中心轮毂
        addCircle(
            origin,
            axis,
            hubRadius,
            20);

        // ============================================================
        // 3. 齿轮辐条
        // ============================================================
        constexpr int SPOKE_COUNT = 6;

        for (int i = 0; i < SPOKE_COUNT; ++i) {
            const float angle =
                2.0f * PI
                * static_cast<float>(i)
                / static_cast<float>(SPOKE_COUNT);

            const QVector3D direction =
                rackDir * std::cos(angle)
                + contactNormal * std::sin(angle);

            addLine(
                origin + direction * hubRadius,
                origin + direction * gearRootRadius);
        }

        // 齿轮轴线
        addLine(
            origin - axis * (sceneScale * 0.35f),
            origin + axis * (sceneScale * 0.35f));

        // ============================================================
        // 4. 绘制齿条齿形
        // ============================================================
        const float toothPitch =
            (rackHalfLength * 2.0f)
            / static_cast<float>(RACK_TEETH);

        QVector3D previousRackPoint;

        for (int i = 0; i <= RACK_TEETH * 2; ++i) {
            const float alongRack =
                -rackHalfLength
                + toothPitch * 0.5f * static_cast<float>(i);

            const float offset =
                (i % 2 == 0)
                ? rackRootOffset
                : rackTipOffset;

            const QVector3D current =
                origin
                + rackDir * alongRack
                + contactNormal * offset;

            if (i > 0) {
                addLine(previousRackPoint, current);
            }

            previousRackPoint = current;
        }

        // ============================================================
        // 5. 齿条主体
        // ============================================================
        const QVector3D backLeft =
            origin
            - rackDir * rackHalfLength
            + contactNormal * rackBackOffset;

        const QVector3D backRight =
            origin
            + rackDir * rackHalfLength
            + contactNormal * rackBackOffset;

        const QVector3D rootLeft =
            origin
            - rackDir * rackHalfLength
            + contactNormal * rackRootOffset;

        const QVector3D rootRight =
            origin
            + rackDir * rackHalfLength
            + contactNormal * rackRootOffset;

        addLine(backLeft, backRight);
        addLine(backLeft, rootLeft);
        addLine(backRight, rootRight);

        // 齿条主体内部两条短剖面线
        for (int i = -1; i <= 1; i += 2) {
            const float position =
                rackHalfLength * 0.55f * static_cast<float>(i);

            addLine(
                origin
                + rackDir * position
                + contactNormal * rackRootOffset,
                origin
                + rackDir * position
                + contactNormal * rackBackOffset);
        }

        // ============================================================
        // 6. 齿条运动方向箭头
        // ============================================================
        const float arrowOffset =
            rackBackOffset + sceneScale * 0.14f;

        const QVector3D arrowStart =
            origin
            - rackDir * (rackHalfLength * 0.55f)
            + contactNormal * arrowOffset;

        const QVector3D arrowTip =
            origin
            + rackDir * (rackHalfLength * 0.55f)
            + contactNormal * arrowOffset;

        const float arrowLength = sceneScale * 0.14f;
        const float arrowWidth = sceneScale * 0.08f;

        addLine(arrowStart, arrowTip);

        addLine(
            arrowTip,
            arrowTip
            - rackDir * arrowLength
            + contactNormal * arrowWidth);

        addLine(
            arrowTip,
            arrowTip
            - rackDir * arrowLength
            - contactNormal * arrowWidth);

        // ============================================================
        // 7. 符号数据
        // ============================================================
        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        //symbol->_txt_pos = {
        //    {
        //        symbolName,
        //        origin
        //            - contactNormal * (gearOuterRadius * 1.45f)
        //            + rackDir * (gearOuterRadius * 0.35f)
        //    }
        //};

        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildPlanarSymbol(mxr::mSymbolController* symbolController, const QString& symbolName, const QVector3D& origin, const QVector3D& planeNormal, bool allowRotation, float sceneScale)
    {
        // ============================================================
        // 1. 建立平面局部坐标系
        // ============================================================
        QVector3D normal = planeNormal;

        if (normal.lengthSquared() < EPS) {
            normal = QVector3D(0.0f, 0.0f, 1.0f);
        }
        else {
            normal.normalize();
        }

        QVector3D u;
        QVector3D v;
        buildOrthonormalBasis(normal, u, v);

        const float planeHalfWidth = sceneScale * 1.05f;
        const float planeHalfHeight = sceneScale * 0.72f;

        const float arrowHalfLengthU = sceneScale * 0.66f;
        const float arrowHalfLengthV = sceneScale * 0.43f;

        const float arrowLength = sceneScale * 0.13f;
        const float arrowWidth = sceneScale * 0.07f;

        const float rotationRadius = sceneScale * 0.46f;
        const float normalLength = sceneScale * 0.55f;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&](const QVector3D& start,
            const QVector3D& end)
            {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(start);
                points.append(end);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        auto addArrowHead = [&](const QVector3D& tip,
            const QVector3D& direction,
            const QVector3D& side)
            {
                QVector3D dir = direction.normalized();
                QVector3D sideDir = side.normalized();

                const QVector3D back =
                    tip - dir * arrowLength;

                addLine(
                    tip,
                    back + sideDir * arrowWidth);

                addLine(
                    tip,
                    back - sideDir * arrowWidth);
            };

        auto addDoubleArrow = [&](const QVector3D& center,
            const QVector3D& direction,
            const QVector3D& side,
            float halfLength)
            {
                const QVector3D dir = direction.normalized();

                const QVector3D start =
                    center - dir * halfLength;

                const QVector3D end =
                    center + dir * halfLength;

                addLine(start, end);

                addArrowHead(end, dir, side);
                addArrowHead(start, -dir, side);
            };

        // ============================================================
        // 2. 平面外轮廓
        // ============================================================
        const QVector3D leftBottom =
            origin
            - u * planeHalfWidth
            - v * planeHalfHeight;

        const QVector3D rightBottom =
            origin
            + u * planeHalfWidth
            - v * planeHalfHeight;

        const QVector3D rightTop =
            origin
            + u * planeHalfWidth
            + v * planeHalfHeight;

        const QVector3D leftTop =
            origin
            - u * planeHalfWidth
            + v * planeHalfHeight;

        addLine(leftBottom, rightBottom);
        addLine(rightBottom, rightTop);
        addLine(rightTop, leftTop);
        addLine(leftTop, leftBottom);

        // ============================================================
        // 3. 平面网格
        // ============================================================
        addLine(
            origin - u * planeHalfWidth
            - v * (planeHalfHeight * 0.33f),
            origin + u * planeHalfWidth
            - v * (planeHalfHeight * 0.33f));

        addLine(
            origin - u * planeHalfWidth
            + v * (planeHalfHeight * 0.33f),
            origin + u * planeHalfWidth
            + v * (planeHalfHeight * 0.33f));

        addLine(
            origin - u * (planeHalfWidth * 0.45f)
            - v * planeHalfHeight,
            origin - u * (planeHalfWidth * 0.45f)
            + v * planeHalfHeight);

        addLine(
            origin + u * (planeHalfWidth * 0.45f)
            - v * planeHalfHeight,
            origin + u * (planeHalfWidth * 0.45f)
            + v * planeHalfHeight);

        // ============================================================
        // 4. 平面内两个移动方向
        // ============================================================
        const QVector3D arrowOrigin =
            origin + normal * (sceneScale * 0.025f);

        addDoubleArrow(
            arrowOrigin,
            u,
            v,
            arrowHalfLengthU);

        addDoubleArrow(
            arrowOrigin,
            v,
            u,
            arrowHalfLengthV);

        // ============================================================
        // 5. 平面法向
        // ============================================================
        const QVector3D normalTip =
            origin + normal * normalLength;

        addLine(origin, normalTip);

        // 法向末端短横线，不绘制箭头，避免误解成允许法向移动
        addLine(
            normalTip - u * (sceneScale * 0.10f),
            normalTip + u * (sceneScale * 0.10f));

        // ============================================================
        // 6. 绕平面法向旋转
        // ============================================================
        if (allowRotation) {
            constexpr int ARC_SEGMENTS = 24;

            const float startAngle = PI * 0.15f;
            const float endAngle = PI * 1.72f;

            QVector3D previous =
                arrowOrigin
                + u * (std::cos(startAngle) * rotationRadius)
                + v * (std::sin(startAngle) * rotationRadius);

            QVector3D arcEnd = previous;

            for (int i = 1; i <= ARC_SEGMENTS; ++i) {
                const float ratio =
                    static_cast<float>(i)
                    / static_cast<float>(ARC_SEGMENTS);

                const float angle =
                    startAngle
                    + (endAngle - startAngle) * ratio;

                const QVector3D current =
                    arrowOrigin
                    + u * (std::cos(angle) * rotationRadius)
                    + v * (std::sin(angle) * rotationRadius);

                addLine(previous, current);

                previous = current;
                arcEnd = current;
            }

            // 圆弧末端切线方向
            const QVector3D tangent =
                -u * std::sin(endAngle)
                + v * std::cos(endAngle);

            const QVector3D radial =
                u * std::cos(endAngle)
                + v * std::sin(endAngle);

            addArrowHead(
                arcEnd,
                tangent,
                radial);
        }

        // ============================================================
        // 7. 中心标记
        // ============================================================
        const float centerMark = sceneScale * 0.075f;

        addLine(
            origin - u * centerMark,
            origin + u * centerMark);

        addLine(
            origin - v * centerMark,
            origin + v * centerMark);

        // ============================================================
        // 8. 数据装填
        // ============================================================
        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        //symbol->_txt_pos = {
        //    {
        //        symbolName,
        //        origin
        //            + u * (planeHalfWidth * 0.80f)
        //            + v * (planeHalfHeight * 0.80f)
        //            + normal * (sceneScale * 0.08f)
        //    }
        //};

        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildParallelSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& direction,
        float sceneScale)
    {
        QVector3D axis = direction;

        if (axis.lengthSquared() < EPS)
            axis = QVector3D(0.0f, 0.0f, 1.0f);
        else
            axis.normalize();

        QVector3D u;
        QVector3D v;
        buildOrthonormalBasis(axis, u, v);

        const float halfLength = sceneScale * 0.95f;
        const float spacing = sceneScale * 0.23f;
        const float arrowLength = sceneScale * 0.16f;
        const float arrowWidth = sceneScale * 0.08f;
        const float endCap = sceneScale * 0.09f;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&](const QVector3D& a,
            const QVector3D& b)
            {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(a);
                points.append(b);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        auto addArrow = [&](const QVector3D& tip)
            {
                const QVector3D back =
                    tip - axis * arrowLength;

                addLine(tip, back + u * arrowWidth);
                addLine(tip, back - u * arrowWidth);
            };

        const QVector3D center1 =
            origin + u * spacing;

        const QVector3D center2 =
            origin - u * spacing;

        const QVector3D start1 =
            center1 - axis * halfLength;

        const QVector3D end1 =
            center1 + axis * halfLength;

        const QVector3D start2 =
            center2 - axis * halfLength;

        const QVector3D end2 =
            center2 + axis * halfLength;

        // 两个运动体对应的平行方向
        addLine(start1, end1);
        addLine(start2, end2);

        // 同向箭头
        addArrow(end1);
        addArrow(end2);

        // 尾部端帽
        addLine(
            start1 - u * endCap,
            start1 + u * endCap);

        addLine(
            start2 - u * endCap,
            start2 + u * endCap);

        // 中心原点标记
        addLine(
            origin - v * (sceneScale * 0.08f),
            origin + v * (sceneScale * 0.08f));

        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        //symbol->_txt_pos = {
        //    {
        //        symbolName,
        //        origin
        //            + u * (sceneScale * 0.48f)
        //            + v * (sceneScale * 0.35f)
        //    }
        //};

        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildDirectionSymbol(mxr::mSymbolController* symbolController, const QString& symbolName, const QVector3D& origin, const QVector3D& direction, float sceneScale)
    {
        // 输入方向作为主轴；另外两个绘制方向自动补齐
        QVector3D n = direction;
        if (n.lengthSquared() < 1e-12f)
            n = QVector3D(0.0f, 0.0f, 1.0f);
        else
            n.normalize();

        QVector3D u, v;
        buildOrthonormalBasis(n, u, v);

        // 两个对齐坐标架之间的间隔，以及三轴长度
        const float halfGap = sceneScale * 0.20f;
        const float actionArm = sceneScale * 0.38f;
        const float baseArm = sceneScale * 0.28f;
        const float arrowHeadLen = sceneScale * 0.13f;
        const float arrowHeadWide = sceneScale * 0.08f;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&points, &lineIdx](const QVector3D& a,
            const QVector3D& b)
            {
                const unsigned int i0 = static_cast<unsigned int>(points.size());
                points.append(a);
                points.append(b);
                lineIdx.push_back(i0);
                lineIdx.push_back(i0 + 1);
            };

        auto addArrowHead = [&](const QVector3D& tip,
            const QVector3D& axis,
            const QVector3D& side)
            {
                addLine(tip,
                    tip - axis * arrowHeadLen + side * arrowHeadWide);
                addLine(tip,
                    tip - axis * arrowHeadLen - side * arrowHeadWide);
            };

        // 基本体参考坐标架：不画箭头
        auto drawBaseFrame = [&](const QVector3D& center, float arm)
            {
                addLine(center, center + n * arm);
                addLine(center, center + u * arm);
                addLine(center, center + v * arm);
            };

        // 运动体坐标架：三个正方向均画箭头
        auto drawActionFrame = [&](const QVector3D& center, float arm)
            {
                const QVector3D nTip = center + n * arm;
                const QVector3D uTip = center + u * arm;
                const QVector3D vTip = center + v * arm;

                addLine(center, nTip);
                addLine(center, uTip);
                addLine(center, vTip);

                addArrowHead(nTip, n, u);
                addArrowHead(uTip, u, v);
                addArrowHead(vTip, v, n);
            };

        // 两个坐标架保持同向，表示完整相对姿态被锁定
        const QVector3D baseCenter = origin - n * halfGap;
        const QVector3D actionCenter = origin + n * halfGap;

        drawBaseFrame(baseCenter, baseArm);
        drawActionFrame(actionCenter, actionArm);

        // 连接对应方向，形成“姿态锁定框架”
        addLine(baseCenter, actionCenter);
        addLine(baseCenter + n * baseArm, actionCenter + n * actionArm);
        addLine(baseCenter + u * baseArm, actionCenter + u * actionArm);
        addLine(baseCenter + v * baseArm, actionCenter + v * actionArm);

        auto symbol =
            symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        //symbol->_txt_pos = {
        //    { symbolName,
        //      origin + u * (sceneScale * 0.55f)
        //             + v * (sceneScale * 0.18f) }
        //};
        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildPerpendicularSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& actionDirection,
        const QVector3D& baseDirection,
        float sceneScale)
    {
        QVector3D actionAxis = actionDirection;

        if (actionAxis.lengthSquared() < EPS)
            actionAxis = QVector3D(1.0f, 0.0f, 0.0f);
        else
            actionAxis.normalize();

        QVector3D baseAxis = baseDirection;

        if (baseAxis.lengthSquared() < EPS) {
            QVector3D unused;
            buildOrthonormalBasis(
                actionAxis,
                baseAxis,
                unused);
        }
        else {
            baseAxis.normalize();
        }

        QVector3D planeNormal =
            QVector3D::crossProduct(
                actionAxis,
                baseAxis);

        // 只用于防止渲染退化，正常输入应由界面保证垂直
        if (planeNormal.lengthSquared() < EPS) {
            QVector3D fallbackNormal;

            buildOrthonormalBasis(
                actionAxis,
                baseAxis,
                fallbackNormal);

            planeNormal = fallbackNormal;
        }
        else {
            planeNormal.normalize();
        }

        const float halfLength = sceneScale * 0.95f;
        const float rightAngleSize = sceneScale * 0.26f;
        const float arrowLength = sceneScale * 0.16f;
        const float arrowWidth = sceneScale * 0.08f;
        const float endCap = sceneScale * 0.09f;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&](const QVector3D& a,
            const QVector3D& b)
            {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(a);
                points.append(b);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        auto addArrow = [&](const QVector3D& tip,
            const QVector3D& axis,
            const QVector3D& side)
            {
                const QVector3D back =
                    tip - axis * arrowLength;

                addLine(tip, back + side * arrowWidth);
                addLine(tip, back - side * arrowWidth);
            };

        const QVector3D actionStart =
            origin - actionAxis * halfLength;

        const QVector3D actionEnd =
            origin + actionAxis * halfLength;

        const QVector3D baseStart =
            origin - baseAxis * halfLength;

        const QVector3D baseEnd =
            origin + baseAxis * halfLength;

        // 两个运动体的方向轴
        addLine(actionStart, actionEnd);
        addLine(baseStart, baseEnd);

        // 方向箭头
        addArrow(
            actionEnd,
            actionAxis,
            baseAxis);

        addArrow(
            baseEnd,
            baseAxis,
            actionAxis);

        // 尾部端帽
        addLine(
            actionStart - baseAxis * endCap,
            actionStart + baseAxis * endCap);

        addLine(
            baseStart - actionAxis * endCap,
            baseStart + actionAxis * endCap);

        // 直角标记
        const QVector3D pointA =
            origin + actionAxis * rightAngleSize;

        const QVector3D corner =
            origin
            + actionAxis * rightAngleSize
            + baseAxis * rightAngleSize;

        const QVector3D pointB =
            origin + baseAxis * rightAngleSize;

        addLine(pointA, corner);
        addLine(corner, pointB);

        // 中心小菱形
        const float markerSize = sceneScale * 0.07f;

        addLine(
            origin + actionAxis * markerSize,
            origin + baseAxis * markerSize);

        addLine(
            origin + baseAxis * markerSize,
            origin - actionAxis * markerSize);

        addLine(
            origin - actionAxis * markerSize,
            origin - baseAxis * markerSize);

        addLine(
            origin - baseAxis * markerSize,
            origin + actionAxis * markerSize);

        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        //symbol->_txt_pos = {
        //    {
        //        symbolName,
        //        origin
        //            + planeNormal * (sceneScale * 0.50f)
        //            + actionAxis * (sceneScale * 0.20f)
        //    }
        //};

        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildConstantVelocitySymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& spinAxisAction, // 运动体传动轴方向
        const QVector3D& spinAxisBase,   // 基本体传动轴方向
        float sceneScale)
    {
        // sceneScale 对外暴露；异常时仅做绘制兜底
        const float scale = sceneScale > 1e-5f ? sceneScale : 0.3f;

        QVector3D nA = spinAxisAction;
        QVector3D nB = spinAxisBase;

        if (nA.lengthSquared() < EPS)
            nA = QVector3D(1.0f, 0.0f, 0.0f);
        else
            nA.normalize();

        if (nB.lengthSquared() < EPS)
            nB = QVector3D(0.0f, 1.0f, 0.0f);
        else
            nB.normalize();

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&points, &lineIdx](const QVector3D& a,
            const QVector3D& b)
            {
                const unsigned int i0 = static_cast<unsigned int>(points.size());
                points.append(a);
                points.append(b);
                lineIdx.push_back(i0);
                lineIdx.push_back(i0 + 1);
            };

        auto addArrowHead = [&](const QVector3D& tip,
            const QVector3D& direction,
            const QVector3D& side,
            float headLength,
            float headWidth)
            {
                addLine(tip, tip - direction * headLength + side * headWidth);
                addLine(tip, tip - direction * headLength - side * headWidth);
            };

        // 在指定法向平面内画圆环
        auto addRing = [&](const QVector3D& center,
            const QVector3D& normal,
            float radius)
            {
                QVector3D ringU, ringV;
                buildOrthonormalBasis(normal, ringU, ringV);

                constexpr int segments = 20;
                const unsigned int base = static_cast<unsigned int>(points.size());

                for (int i = 0; i < segments; ++i) {
                    const float angle = 2.0f * PI * i / segments;
                    points.append(center
                        + ringU * (radius * std::cos(angle))
                        + ringV * (radius * std::sin(angle)));
                }

                for (int i = 0; i < segments; ++i) {
                    lineIdx.push_back(base + i);
                    lineIdx.push_back(base + (i + 1) % segments);
                }
            };

        // 画带箭头的旋转环：表示该端轴正在传递角速度
        auto addRotationRing = [&](const QVector3D& center,
            const QVector3D& axis,
            float radius)
            {
                QVector3D ringU, ringV;
                buildOrthonormalBasis(axis, ringU, ringV);

                addRing(center, axis, radius);

                // 在圆环约 45° 位置添加切向箭头
                const float angle = PI * 0.25f;
                const QVector3D tip =
                    center + ringU * (radius * std::cos(angle))
                    + ringV * (radius * std::sin(angle));

                QVector3D tangent =
                    -ringU * std::sin(angle) + ringV * std::cos(angle);
                tangent.normalize();

                addArrowHead(tip,
                    tangent,
                    axis,
                    scale * 0.11f,
                    scale * 0.065f);
            };

        const float cageRadius = scale * 0.19f;
        const float ringRadius = scale * 0.17f;
        const float shaftLength = scale * 0.72f;
        const float ringOffset = scale * 0.47f;

        // 1. 中心球笼：两个相交圆环表达可带夹角传动
        addRing(origin, nA, cageRadius);
        addRing(origin, nB, cageRadius);

        // 2. 两根传动轴
        const QVector3D endA = origin + nA * shaftLength;
        const QVector3D endB = origin + nB * shaftLength;

        addLine(origin + nA * cageRadius, endA);
        addLine(origin + nB * cageRadius, endB);

        // 3. 两端同尺寸旋转环：表示输入、输出等角速度
        const QVector3D ringCenterA = origin + nA * ringOffset;
        const QVector3D ringCenterB = origin + nB * ringOffset;

        addRotationRing(ringCenterA, nA, ringRadius);
        addRotationRing(ringCenterB, nB, ringRadius);

        // 4. 在两轴夹角平分方向放置“=”：明确 1:1 等速关系
        QVector3D bisector = nA + nB;
        if (bisector.lengthSquared() < EPS)
            bisector = nA;
        else
            bisector.normalize();

        QVector3D planeNormal = QVector3D::crossProduct(nA, nB);
        if (planeNormal.lengthSquared() < EPS) {
            QVector3D temp;
            buildOrthonormalBasis(bisector, planeNormal, temp);
        }
        else {
            planeNormal.normalize();
        }

        QVector3D equalOffset =
            QVector3D::crossProduct(planeNormal, bisector);
        if (equalOffset.lengthSquared() < EPS)
            equalOffset = nA;
        else
            equalOffset.normalize();

        const QVector3D equalCenter = origin + bisector * (scale * 0.42f);
        const float equalHalfLength = scale * 0.11f;
        const float equalGap = scale * 0.065f;

        addLine(equalCenter - equalOffset * equalGap
            - planeNormal * equalHalfLength,
            equalCenter - equalOffset * equalGap
            + planeNormal * equalHalfLength);

        addLine(equalCenter + equalOffset * equalGap
            - planeNormal * equalHalfLength,
            equalCenter + equalOffset * equalGap
            + planeNormal * equalHalfLength);

        // 5. 渲染数据
        auto symbol =
            symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        //symbol->_txt_pos = {
        //    { symbolName, equalCenter + planeNormal * (scale * 0.24f) }
        //};
        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildGearCouplingSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& jointOrigin1,
        const QVector3D& jointAxis1,
        const QVector3D& jointOrigin2,
        const QVector3D& jointAxis2,
        double ratio,
        float sceneScale)
    {
        QVector3D axis1 = jointAxis1;
        QVector3D axis2 = jointAxis2;

        if (axis1.lengthSquared() < EPS)
            axis1 = QVector3D(0, 0, 1);
        else
            axis1.normalize();

        if (axis2.lengthSquared() < EPS)
            axis2 = axis1;
        else
            axis2.normalize();

        // 两根轴反向时统一到相同半空间，便于建立显示平面
        if (QVector3D::dotProduct(axis1, axis2) < 0.0f)
            axis2 = -axis2;

        QVector3D planeNormal = axis1 + axis2;

        if (planeNormal.lengthSquared() < EPS)
            planeNormal = axis1;
        else
            planeNormal.normalize();

        // 使用两个旋转副原点确定齿轮排列方向
        QVector3D radialDirection = jointOrigin2 - jointOrigin1;

        radialDirection -=
            planeNormal
            * QVector3D::dotProduct(radialDirection, planeNormal);

        if (radialDirection.lengthSquared() < EPS) {
            QVector3D unused;
            buildOrthonormalBasis(
                planeNormal,
                radialDirection,
                unused);
        }
        else {
            radialDirection.normalize();
        }

        QVector3D tangentDirection =
            QVector3D::crossProduct(
                planeNormal,
                radialDirection).normalized();

        const QVector3D symbolOrigin =
            (jointOrigin1 + jointOrigin2) * 0.5f;

        // 啮合方式由传动比符号决定（与求解语义一致）：
        // 正=外啮合（两外齿圆相切），负=内啮合（大轮内齿圈+内切小齿轮）
        const bool internalMesh = ratio < 0.0;

        // 图标锚点 = 真实啮合点（与引擎 LinkGear 同款公式）：
        // 外啮合 r2 = d/(1+τ)，啮合点在两作用点之间（τ=1 时即中点，与旧版一致）；
        // 内啮合 r2 = d − τd/(τ−1)（带符号，啮合点在连线段外的大轮内齿处）。
        // 内啮合 τ→1 趋于齿条极限（r2 发散），限幅到 ±2d 兜底。
        QVector3D realContact = symbolOrigin;
        {
            QVector3D linkVec = jointOrigin2 - jointOrigin1;

            linkVec -=
                planeNormal
                * QVector3D::dotProduct(linkVec, planeNormal);

            const float d = linkVec.length();

            if (d > EPS) {
                const float t =
                    static_cast<float>(std::abs(ratio));

                float r2;

                if (!internalMesh) {
                    r2 = d / (1.0f + t);
                }
                else {
                    r2 = d - (t * d) / (t - 1.0f);
                    r2 = std::max(-2.0f * d, std::min(2.0f * d, r2));
                }

                // 啮合点 = 第二副作用点 + 指向第一副的方向 × r2
                realContact = jointOrigin2 - radialDirection * r2;
            }
        }

        const float maximumRadius = sceneScale * 0.55f;
        const float minimumRadius = sceneScale * 0.24f;

        // 用传动比控制两个符号齿轮的相对大小，但限制极端比例
        float visualRatio =
            static_cast<float>(std::abs(ratio));

        visualRatio =
            std::max(0.35f, std::min(visualRatio, 2.85f));

        float radius2 =
            maximumRadius / std::max(1.0f, visualRatio);

        float radius1 =
            radius2 * visualRatio;

        radius1 = std::max(radius1, minimumRadius);
        radius2 = std::max(radius2, minimumRadius);

        const float centerDistance =
            radius1 + radius2 + sceneScale * 0.04f;

        QVector3D center1 =
            symbolOrigin
            - radialDirection * (centerDistance * 0.5f);

        QVector3D center2 =
            symbolOrigin
            + radialDirection * (centerDistance * 0.5f);

        // 图标整体平移：图标自身啮合点（center1 + radial×radius1）
        // 与真实啮合点重合，图标即"画在啮合发生处"
        {
            const QVector3D glyphContact =
                center1 + radialDirection * radius1;

            const QVector3D delta = realContact - glyphContact;

            center1 += delta;
            center2 += delta;
        }

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&](const QVector3D& a,
            const QVector3D& b)
            {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(a);
                points.append(b);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        auto addCircle = [&](const QVector3D& center,
            float radius,
            int segments)
            {
                QVector3D previous =
                    center + radialDirection * radius;

                for (int i = 1; i <= segments; ++i) {
                    const float angle =
                        2.0f * PI * static_cast<float>(i)
                        / static_cast<float>(segments);

                    const QVector3D current =
                        center
                        + radialDirection
                        * (std::cos(angle) * radius)
                        + tangentDirection
                        * (std::sin(angle) * radius);

                    addLine(previous, current);
                    previous = current;
                }
            };

        auto addGear = [&](const QVector3D& center,
            float pitchRadius,
            int toothCount)
            {
                const float rootRadius = pitchRadius * 0.83f;
                const float outerRadius = pitchRadius * 1.13f;
                const float hubRadius = pitchRadius * 0.28f;

                const int vertexCount = toothCount * 4;

                QVector3D previous;

                for (int i = 0; i <= vertexCount; ++i) {
                    const int wrapped = i % vertexCount;

                    const float angle =
                        2.0f * PI * static_cast<float>(wrapped)
                        / static_cast<float>(vertexCount);

                    const int phase = wrapped % 4;

                    const float radius =
                        (phase == 1 || phase == 2)
                        ? outerRadius
                        : rootRadius;

                    const QVector3D current =
                        center
                        + radialDirection
                        * (std::cos(angle) * radius)
                        + tangentDirection
                        * (std::sin(angle) * radius);

                    if (i > 0)
                        addLine(previous, current);

                    previous = current;
                }

                addCircle(center, pitchRadius, 28);
                addCircle(center, hubRadius, 18);

                for (int i = 0; i < 4; ++i) {
                    const float angle =
                        0.5f * PI * static_cast<float>(i);

                    QVector3D spokeDirection =
                        radialDirection * std::cos(angle)
                        + tangentDirection * std::sin(angle);

                    addLine(
                        center + spokeDirection * hubRadius,
                        center + spokeDirection * rootRadius);
                }

                // 齿轮轴线
                addLine(
                    center - planeNormal * (sceneScale * 0.15f),
                    center + planeNormal * (sceneScale * 0.15f));
            };

        // 内啮合大轮：光滑轮缘 + 指向圆心的内齿带
        auto addRingGear = [&](const QVector3D& center,
            float pitchRadius,
            int toothCount)
            {
                const float rimOuter = pitchRadius * 1.24f;
                const float rootRadius = pitchRadius * 1.17f;
                const float tipRadius = pitchRadius * 0.87f;

                addCircle(center, rimOuter, 36);

                const int vertexCount = toothCount * 4;

                QVector3D previous;

                for (int i = 0; i <= vertexCount; ++i) {
                    const int wrapped = i % vertexCount;

                    const float angle =
                        2.0f * PI * static_cast<float>(wrapped)
                        / static_cast<float>(vertexCount);

                    const int phase = wrapped % 4;

                    const float radius =
                        (phase == 1 || phase == 2)
                        ? tipRadius
                        : rootRadius;

                    const QVector3D current =
                        center
                        + radialDirection
                        * (std::cos(angle) * radius)
                        + tangentDirection
                        * (std::sin(angle) * radius);

                    if (i > 0)
                        addLine(previous, current);

                    previous = current;
                }

                // 大轮轴线
                addLine(
                    center - planeNormal * (sceneScale * 0.15f),
                    center + planeNormal * (sceneScale * 0.15f));
            };

        if (!internalMesh) {
            addGear(center1, radius1, 12);
            addGear(center2, radius2, 10);

            // 啮合点短标记
            const QVector3D contactPoint =
                center1 + radialDirection * radius1;

            addLine(
                contactPoint - tangentDirection * (sceneScale * 0.08f),
                contactPoint + tangentDirection * (sceneScale * 0.08f));
        }
        else {
            // 内啮合：大轮内齿圈居中，小轮沿两旋转副连线方向内切。
            // 内啮合中大轮必为大：显示比取 |传动比| 与其倒数中较大者，
            // 并限制极端比例，保证小轮可见。
            float visualRatioI = std::abs(ratio);

            visualRatioI = std::max(visualRatioI, 1.0f / std::max(visualRatioI, 0.01f));
            visualRatioI = std::min(visualRatioI, 2.85f);

            const float ringPitch = maximumRadius * 0.82f;

            float pinionRadius = ringPitch / visualRatioI;

            pinionRadius = std::max(pinionRadius, minimumRadius * 0.5f);

            // 大轮圆心同样按真实啮合点锚定：图标自身啮合点
            // （圆心 + radial×ringPitch）与 realContact 重合
            const QVector3D ringCenter =
                realContact - radialDirection * ringPitch;

            const QVector3D pinionCenter =
                ringCenter
                + radialDirection
                * (ringPitch - pinionRadius);

            addRingGear(ringCenter, ringPitch, 16);
            addGear(pinionCenter, pinionRadius, 10);

            // 啮合点：连心线延长大轮一侧的内齿分度圆处
            const QVector3D contactPoint =
                ringCenter + radialDirection * ringPitch;

            addLine(
                contactPoint - tangentDirection * (sceneScale * 0.08f),
                contactPoint + tangentDirection * (sceneScale * 0.08f));
        }

        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        //symbol->_txt_pos = {
        //    {
        //        symbolName,
        //        symbolOrigin
        //            + tangentDirection
        //                * (maximumRadius * 1.45f)
        //    }
        //};

        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildRackPinionCouplingSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& gearOrigin,
        const QVector3D& gearAxis,
        const QVector3D& rackOrigin,
        const QVector3D& rackDirection,
        double displacementPerRadian,
        float sceneScale)
    {
        QVector3D axis = gearAxis;

        if (axis.lengthSquared() < EPS)
            axis = QVector3D(0, 0, 1);
        else
            axis.normalize();

        // 齿条方向必须垂直于齿轮轴
        QVector3D rackDir =
            rackDirection
            - axis
            * QVector3D::dotProduct(rackDirection, axis);

        if (rackDir.lengthSquared() < EPS) {
            QVector3D unused;
            buildOrthonormalBasis(axis, rackDir, unused);
        }
        else {
            rackDir.normalize();
        }

        QVector3D contactNormal =
            QVector3D::crossProduct(axis, rackDir);

        if (contactNormal.lengthSquared() < EPS) {
            QVector3D unused;
            buildOrthonormalBasis(axis, rackDir, unused);

            contactNormal =
                QVector3D::crossProduct(axis, rackDir);
        }

        contactNormal.normalize();

        // 让齿条画在真实移动副原点所在的一侧
        QVector3D toRack = rackOrigin - gearOrigin;

        if (QVector3D::dotProduct(toRack, contactNormal) < 0.0f)
            contactNormal = -contactNormal;

        const float pitchRadius = sceneScale * 0.50f;
        const float rootRadius = pitchRadius * 0.83f;
        const float outerRadius = pitchRadius * 1.13f;
        const float hubRadius = pitchRadius * 0.28f;

        const float rackHalfLength = sceneScale * 1.05f;
        const float toothHeight = sceneScale * 0.14f;
        const float rackBodyHeight = sceneScale * 0.24f;

        const float rackTipOffset = outerRadius * 0.97f;
        const float rackRootOffset =
            rackTipOffset + toothHeight;

        const float rackBackOffset =
            rackRootOffset + rackBodyHeight;

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIdx;

        auto addLine = [&](const QVector3D& a,
            const QVector3D& b)
            {
                const unsigned int index =
                    static_cast<unsigned int>(points.size());

                points.append(a);
                points.append(b);

                lineIdx.push_back(index);
                lineIdx.push_back(index + 1);
            };

        auto addCircle = [&](float radius,
            int segments)
            {
                QVector3D previous =
                    gearOrigin + rackDir * radius;

                for (int i = 1; i <= segments; ++i) {
                    const float angle =
                        2.0f * PI * static_cast<float>(i)
                        / static_cast<float>(segments);

                    const QVector3D current =
                        gearOrigin
                        + rackDir
                        * (std::cos(angle) * radius)
                        + contactNormal
                        * (std::sin(angle) * radius);

                    addLine(previous, current);
                    previous = current;
                }
            };

        // ============================================================
        // 齿轮外齿
        // ============================================================
        constexpr int GEAR_TEETH = 12;
        constexpr int GEAR_VERTICES = GEAR_TEETH * 4;

        QVector3D previousGearPoint;

        for (int i = 0; i <= GEAR_VERTICES; ++i) {
            const int wrapped = i % GEAR_VERTICES;

            const float angle =
                2.0f * PI * static_cast<float>(wrapped)
                / static_cast<float>(GEAR_VERTICES);

            const int phase = wrapped % 4;

            const float radius =
                (phase == 1 || phase == 2)
                ? outerRadius
                : rootRadius;

            const QVector3D current =
                gearOrigin
                + rackDir
                * (std::cos(angle) * radius)
                + contactNormal
                * (std::sin(angle) * radius);

            if (i > 0)
                addLine(previousGearPoint, current);

            previousGearPoint = current;
        }

        addCircle(pitchRadius, 28);
        addCircle(hubRadius, 18);

        // 四根轮辐
        for (int i = 0; i < 4; ++i) {
            const float angle =
                0.5f * PI * static_cast<float>(i);

            QVector3D spokeDirection =
                rackDir * std::cos(angle)
                + contactNormal * std::sin(angle);

            addLine(
                gearOrigin + spokeDirection * hubRadius,
                gearOrigin + spokeDirection * rootRadius);
        }

        // 齿轮轴线
        addLine(
            gearOrigin - axis * (sceneScale * 0.18f),
            gearOrigin + axis * (sceneScale * 0.18f));

        // ============================================================
        // 齿条齿形
        // ============================================================
        constexpr int RACK_TEETH = 8;

        const float toothPitch =
            rackHalfLength * 2.0f
            / static_cast<float>(RACK_TEETH);

        QVector3D previousRackPoint;

        for (int i = 0; i <= RACK_TEETH * 2; ++i) {
            const float alongRack =
                -rackHalfLength
                + toothPitch * 0.5f * static_cast<float>(i);

            const float offset =
                (i % 2 == 0)
                ? rackRootOffset
                : rackTipOffset;

            const QVector3D current =
                gearOrigin
                + rackDir * alongRack
                + contactNormal * offset;

            if (i > 0)
                addLine(previousRackPoint, current);

            previousRackPoint = current;
        }

        const QVector3D rackLeftRoot =
            gearOrigin
            - rackDir * rackHalfLength
            + contactNormal * rackRootOffset;

        const QVector3D rackRightRoot =
            gearOrigin
            + rackDir * rackHalfLength
            + contactNormal * rackRootOffset;

        const QVector3D rackLeftBack =
            gearOrigin
            - rackDir * rackHalfLength
            + contactNormal * rackBackOffset;

        const QVector3D rackRightBack =
            gearOrigin
            + rackDir * rackHalfLength
            + contactNormal * rackBackOffset;

        addLine(rackLeftRoot, rackLeftBack);
        addLine(rackLeftBack, rackRightBack);
        addLine(rackRightBack, rackRightRoot);

        // ============================================================
        // 齿条运动方向箭头
        // ============================================================
        QVector3D motionDirection = rackDir;

        if (displacementPerRadian < 0.0)
            motionDirection = -motionDirection;

        const QVector3D arrowCenter =
            gearOrigin
            + contactNormal
            * (rackBackOffset + sceneScale * 0.13f);

        const float arrowHalfLength = sceneScale * 0.48f;
        const float arrowHeadLength = sceneScale * 0.14f;
        const float arrowHeadWidth = sceneScale * 0.07f;

        const QVector3D arrowStart =
            arrowCenter
            - motionDirection * arrowHalfLength;

        const QVector3D arrowEnd =
            arrowCenter
            + motionDirection * arrowHalfLength;

        addLine(arrowStart, arrowEnd);

        const QVector3D arrowBack =
            arrowEnd
            - motionDirection * arrowHeadLength;

        addLine(
            arrowEnd,
            arrowBack
            + contactNormal * arrowHeadWidth);

        addLine(
            arrowEnd,
            arrowBack
            - contactNormal * arrowHeadWidth);

        auto symbol =
            symbolController
            ->getOrCreateSymbolData<UniversalSymbolFont>(symbolName);

        symbol->_vexter = points;
        symbol->_lineIndex = lineIdx;

        symbol->_lineColor = QVector3D(204, 153, 0);
        symbol->_faceColor = QVector3D(204, 153, 0);

        symbol->_lineIsFixedPixel = false;
        symbol->_lineDepthState = false;
        symbol->_faceIsFixedPixel = false;
        symbol->_faceDepthState = false;
        symbol->_showState = true;

        //symbol->_txt_pos = {
        //    {
        //        symbolName,
        //        gearOrigin
        //            - contactNormal
        //                * (outerRadius * 1.45f)
        //    }
        //};

        //symbol->_textColor = QVector3D(0, 0, 0);

        symbolController->createSymbol(symbol);
    }

    void mMotionJointSymbolHelper::buildOrthonormalBasis(const QVector3D& axis, QVector3D& u, QVector3D& v)
    {
        QVector3D n = axis;
        n.normalize();

        QVector3D ref = (std::abs(n.x()) < 0.9f) ? QVector3D(1, 0, 0) : QVector3D(0, 1, 0);
        u = QVector3D::crossProduct(ref, n);
        if (u.lengthSquared() < 1e-12f)
        {
            u = QVector3D(0, 1, 0);
        }
        u.normalize();

        v = QVector3D::crossProduct(n, u);
        v.normalize();
    }

    float mMotionJointSymbolHelper::computeSceneScale(mxr::CCSMRendWidget* rendWidget)
    {
        if (!rendWidget)
            return 0.3f;

        mxr::AABB viewAABB = rendWidget->getAABBOfView();
        auto ccc = rendWidget->getCameraController();
        auto state = ccc->getCameraState();
        //auto z_near = state.z_near;
        //auto z_far = state.z_far;
        auto zoom = state.ortho_zoom;
        //qDebug() << z_near << z_far << zoom;

        if (!viewAABB.valid())
            return 0.3f;

        //const float diagonal = viewAABB.getDiagonal();
        const float diagonal = zoom;
        if (diagonal <= 0.0f)
            return 0.3f;

        // 符号基础尺度为视口包围盒对角线的 5%，兼顾大小模型
        float scale = diagonal * 0.05f;
        if (scale < 0.01f)
            scale = 0.01f;
        //else if (scale > 10.0f)
        //    scale = 10.0f;

        return scale;
    }

}
