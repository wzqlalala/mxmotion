#include "mMotionVectorSymbolHelper.h"
#include "mPhysicalFieldEnum.h"

#include "mViewportModelManager.h"
#include "mMotionRefMap.h"
#include "mMotionVectorForce.h"
#include "mMotionScalarForce.h"
#include "mMotionScalarTorque.h"
#include "mMotionThreePointForce.h"
#include "mMotionThreePointTorque.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SpaceTree.h"

#include <QVector>
#include <vector>
#include <cmath>

using namespace mxr;
using namespace MxFunctions;
namespace MechMainWindow
{
    namespace
    {
        constexpr float kSymbolPi = 3.14159265358979323846f;

        void appendSymbolLine(
            QVector<QVector3D>& points,
            std::vector<unsigned int>& lineIndex,
            const QVector3D& p0,
            const QVector3D& p1)
        {
            const unsigned int index =
                static_cast<unsigned int>(points.size());

            points.append(p0);
            points.append(p1);

            lineIndex.push_back(index);
            lineIndex.push_back(index + 1);
        }

        void appendSymbolCircle(
            QVector<QVector3D>& points,
            std::vector<unsigned int>& lineIndex,
            const QVector3D& center,
            const QVector3D& u,
            const QVector3D& v,
            float radius,
            int segments)
        {
            QVector3D previous = center + u * radius;

            for (int i = 1; i <= segments; ++i) {
                const float angle =
                    2.0f * kSymbolPi * static_cast<float>(i) /
                    static_cast<float>(segments);

                const QVector3D current =
                    center +
                    u * (radius * std::cos(angle)) +
                    v * (radius * std::sin(angle));

                appendSymbolLine(points, lineIndex, previous, current);
                previous = current;
            }
        }

        void prepareSymbolBasis(
            const QVector3D& inputAxis,
            QVector3D& n,
            QVector3D& u,
            QVector3D& v)
        {
            n = inputAxis;

            if (n.lengthSquared() < 1e-12f)
                n = QVector3D(0.0f, 0.0f, 1.0f);
            else
                n.normalize();

            mMotionVectorSymbolHelper::buildOrthonormalBasis(n, u, v);

            // 保证 u、v、n 构成右手坐标系
            if (QVector3D::dotProduct(
                    QVector3D::crossProduct(u, v), n) < 0.0f) {
                v = -v;
            }
        }

        void commitLineSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector<QVector3D>& points,
            const std::vector<unsigned int>& lineIndex,
            const QVector3D& textPosition)
        {
            auto symbol =
                symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    symbolName);

            symbol->_vexter = points;
            symbol->_lineIndex = lineIndex;

            symbol->_lineColor = QVector3D(204, 153, 0);
            symbol->_faceColor = QVector3D(204, 153, 0);

            symbol->_lineIsFixedPixel = false;
            symbol->_lineDepthState = false;
            symbol->_faceIsFixedPixel = false;
            symbol->_faceDepthState = false;
            symbol->_showState = true;

            // symbol->_txt_pos = {
            //     {symbolName, textPosition}
            // };
            // symbol->_textColor = QVector3D(0, 0, 0);

            symbolController->createSymbol(symbol);
        }
    }

    void mMotionVectorSymbolHelper::ensureForceElementSymbolByData(
        mxr::mSymbolController* symbolController,
        MechMotionData::mMotionForceElement* forceData,
        mxr::CCSMRendWidget* rendWidget)
    {
        if (!symbolController || !forceData || !getCVM)
            return;
        const float sceneScale = computeSceneScale(rendWidget);
        auto exists = [symbolController](const QString& name) {
            return symbolController->isExistSymbol(name);
            };

        // 三点力矩
        if (auto* tt = dynamic_cast<MechMotionData::mMotionThreePointTorque*>(forceData))
        {
            const QString symbol = QString("threePointTorque_%1").arg(tt->getId());
            if (exists(symbol)) return;
            QVector3D axis = tt->getLocalTarget() - tt->getLocalOrigin();
            if (axis.length() < 1e-6) axis = QVector3D(1, 0, 0);
            buildVectorTorqueSymbol(symbolController, symbol, tt->getLocalI(), axis, sceneScale);
            return;
        }
        // 三点力
        if (auto* tp = dynamic_cast<MechMotionData::mMotionThreePointForce*>(forceData))
        {
            const QString symbol = QString("threePointForce_%1").arg(tp->getId());
            if (exists(symbol)) return;
            QVector3D dir = tp->getLocalTarget() - tp->getLocalOrigin();
            if (dir.length() < 1e-6) dir = QVector3D(1, 0, 0);
            buildScalarForceSymbol(symbolController, symbol, tp->getLocalI(), dir, sceneScale);
            return;
        }
        // 标量力：两端体均已映射才建符号（与面板渲染一致）
        if (auto* sc = dynamic_cast<MechMotionData::mMotionScalarForce*>(forceData))
        {
            const QString symbol = QString("scalarForce_%1").arg(sc->getId());
            if (exists(symbol)) return;
            if (mMotionRefMap::get(Physical_MotionVectorForce, sc->getId(), Physical_MotionBody, MapRelation_MotionBodyI) == -1
                || mMotionRefMap::get(Physical_MotionVectorForce, sc->getId(), Physical_MotionBody, MapRelation_MotionBodyJ) == -1)
                return;
            QVector3D dir = sc->getLocalJ() - sc->getLocalI();
            if (dir.length() < 1e-12) dir = QVector3D(1, 0, 0);
            buildScalarForceSymbol(symbolController, symbol, sc->getLocalI(), dir, sceneScale);
            return;
        }
        // 标量力矩
        if (auto* st = dynamic_cast<MechMotionData::mMotionScalarTorque*>(forceData))
        {
            const QString symbol = QString("scalarTorque_%1").arg(st->getId());
            if (exists(symbol)) return;
            if (mMotionRefMap::get(Physical_MotionVectorForce, st->getId(), Physical_MotionBody, MapRelation_MotionBodyI) == -1
                || mMotionRefMap::get(Physical_MotionVectorForce, st->getId(), Physical_MotionBody, MapRelation_MotionBodyJ) == -1)
                return;
            QVector3D axis = (st->getAxisMode() == MechMotionData::ScalarTorqueAxisMode::Line)
                ? (st->getLocalJ() - st->getLocalI())
                : st->getFixedAxis();
            if (axis.length() < 1e-12) axis = QVector3D(0, 0, 1);
            buildVectorTorqueSymbol(symbolController, symbol, st->getLocalI(), axis, sceneScale);
            return;
        }
        // 矢量力/矢量力矩/轴套（统一 vectorForce_ 前缀，与面板一致）
        auto* data = dynamic_cast<MechMotionData::mMotionVectorForce*>(forceData);
        if (!data)
            return;
        const QString symbol = QString("vectorForce_%1").arg(data->getId());
        if (exists(symbol)) return;
        if (mMotionRefMap::get(Physical_MotionVectorForce, data->getId(), Physical_MotionBody, MapRelation_MotionActionBody) == -1)
            return;
        QVector3D dir = data->getDirection();
        if (dir.length() < 1e-12) dir = QVector3D(1, 0, 0);
        const auto loadType = data->getLoadType();
        if (loadType == MechMotionData::VectorLoadType::Force)
            buildVectorForceSymbol(symbolController, symbol, data->getActionPoint(), dir, sceneScale);
        else if (loadType == MechMotionData::VectorLoadType::Torque)
            buildVectorTorqueSymbol(symbolController, symbol, data->getActionPoint(), dir, sceneScale);
        else if (loadType == MechMotionData::VectorLoadType::Bushing)
            buildBushingSymbol(symbolController, symbol, data->getActionPoint(), dir, sceneScale);
    }

    void mMotionVectorSymbolHelper::buildBushingSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& axis,
        float sceneScale)
    {
        if (!symbolController)
            return;

        const float scale =
            sceneScale > 1e-6f ? sceneScale : 0.3f;

        QVector3D n, u, v;
        prepareSymbolBasis(axis, n, u, v);

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIndex;

        const float halfLength = scale * 0.55f;
        const float outerRadius = scale * 0.44f;
        const float innerRadius = scale * 0.24f;
        const float rubberRadius = scale * 0.32f;
        const float rubberHalfLength = scale * 0.38f;
        const float corrugation = scale * 0.055f;
        const float shaftLength = scale * 0.30f;

        const QVector3D leftCenter = origin - n * halfLength;
        const QVector3D rightCenter = origin + n * halfLength;

        // 两端安装环
        appendSymbolCircle(
            points, lineIndex,
            leftCenter, u, v,
            outerRadius, 28);

        appendSymbolCircle(
            points, lineIndex,
            leftCenter, u, v,
            innerRadius, 24);

        appendSymbolCircle(
            points, lineIndex,
            rightCenter, u, v,
            outerRadius, 28);

        appendSymbolCircle(
            points, lineIndex,
            rightCenter, u, v,
            innerRadius, 24);

        // 两侧连接轴
        appendSymbolLine(
            points, lineIndex,
            leftCenter - n * shaftLength,
            leftCenter);

        appendSymbolLine(
            points, lineIndex,
            rightCenter,
            rightCenter + n * shaftLength);

        // 弹性层两端轮廓
        appendSymbolCircle(
            points, lineIndex,
            origin - n * rubberHalfLength,
            u, v, rubberRadius, 24);

        appendSymbolCircle(
            points, lineIndex,
            origin + n * rubberHalfLength,
            u, v, rubberRadius, 24);

        // 四条波纹母线，表示衬套的柔性橡胶层
        constexpr int sideCount = 4;
        constexpr int waveSections = 16;

        for (int side = 0; side < sideCount; ++side) {
            const float sideAngle =
                2.0f * kSymbolPi *
                static_cast<float>(side) /
                static_cast<float>(sideCount);

            const QVector3D radial =
                u * std::cos(sideAngle) +
                v * std::sin(sideAngle);

            QVector3D previous =
                origin -
                n * rubberHalfLength +
                radial * rubberRadius;

            for (int i = 1; i <= waveSections; ++i) {
                const float t =
                    static_cast<float>(i) /
                    static_cast<float>(waveSections);

                const float axialPosition =
                    -rubberHalfLength +
                    2.0f * rubberHalfLength * t;

                const float radius =
                    rubberRadius +
                    corrugation *
                    std::sin(4.0f * kSymbolPi * t);

                const QVector3D current =
                    origin +
                    n * axialPosition +
                    radial * radius;

                appendSymbolLine(
                    points, lineIndex,
                    previous, current);

                previous = current;
            }
        }

        commitLineSymbol(
            symbolController,
            symbolName,
            points,
            lineIndex,
            origin + u * (outerRadius * 1.45f));
    }

    void mMotionVectorSymbolHelper::buildVectorForceSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& forceVector,
        float sceneScale)
    {
        if (!symbolController)
            return;

        const float scale =
            sceneScale > 1e-6f ? sceneScale : 0.3f;

        QVector3D n, u, v;
        prepareSymbolBasis(forceVector, n, u, v);

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIndex;

        const float arrowLength = scale * 1.55f;
        const float headLength = scale * 0.38f;
        const float headWidth = scale * 0.18f;
        const float pointRadius = scale * 0.10f;

        const QVector3D tip = origin + n * arrowLength;
        const QVector3D headCenter = tip - n * headLength;

        // 作用点标记
        appendSymbolCircle(
            points, lineIndex,
            origin, u, v,
            pointRadius, 18);

        appendSymbolLine(
            points, lineIndex,
            origin - u * pointRadius,
            origin + u * pointRadius);

        appendSymbolLine(
            points, lineIndex,
            origin - v * pointRadius,
            origin + v * pointRadius);

        // 力矢量主轴
        appendSymbolLine(
            points, lineIndex,
            origin, tip);

        // 四棱锥式箭头，避免改变视角后看不清
        appendSymbolLine(
            points, lineIndex,
            tip, headCenter + u * headWidth);

        appendSymbolLine(
            points, lineIndex,
            tip, headCenter - u * headWidth);

        appendSymbolLine(
            points, lineIndex,
            tip, headCenter + v * headWidth);

        appendSymbolLine(
            points, lineIndex,
            tip, headCenter - v * headWidth);

        commitLineSymbol(
            symbolController,
            symbolName,
            points,
            lineIndex,
            tip + u * (scale * 0.25f));
    }

    void mMotionVectorSymbolHelper::buildScalarForceSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& forceVector,
        float sceneScale)
    {
        // 标量力复用矢量力箭头符号
        buildVectorForceSymbol(symbolController, symbolName, origin, forceVector, sceneScale);
    }

    void mMotionVectorSymbolHelper::buildVectorTorqueSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName,
        const QVector3D& origin,
        const QVector3D& torqueVector,
        float sceneScale)
    {
        if (!symbolController)
            return;

        const float scale =
            sceneScale > 1e-6f ? sceneScale : 0.3f;

        QVector3D n, u, v;
        prepareSymbolBasis(torqueVector, n, u, v);

        QVector<QVector3D> points;
        std::vector<unsigned int> lineIndex;

        const float radius = scale * 0.62f;
        const float centerRadius = scale * 0.09f;
        const float axisHalfLength = scale * 0.25f;
        const float headLength = scale * 0.25f;
        const float headWidth = scale * 0.14f;

        constexpr int arcSegments = 38;

        // 保留约60度缺口
        const float startAngle =
            30.0f * kSymbolPi / 180.0f;

        const float sweepAngle =
            300.0f * kSymbolPi / 180.0f;

        QVector3D previous =
            origin +
            u * (radius * std::cos(startAngle)) +
            v * (radius * std::sin(startAngle));

        for (int i = 1; i <= arcSegments; ++i) {
            const float t =
                static_cast<float>(i) /
                static_cast<float>(arcSegments);

            const float angle =
                startAngle + sweepAngle * t;

            const QVector3D current =
                origin +
                u * (radius * std::cos(angle)) +
                v * (radius * std::sin(angle));

            appendSymbolLine(
                points, lineIndex,
                previous, current);

            previous = current;
        }

        const float endAngle =
            startAngle + sweepAngle;

        const QVector3D radial =
            u * std::cos(endAngle) +
            v * std::sin(endAngle);

        // 圆弧正切方向：n × radial
        QVector3D tangent =
            QVector3D::crossProduct(n, radial);

        if (tangent.lengthSquared() > 1e-12f)
            tangent.normalize();

        const QVector3D tip =
            origin + radial * radius;

        const QVector3D headCenter =
            tip - tangent * headLength;

        // 圆弧箭头
        appendSymbolLine(
            points, lineIndex,
            tip,
            headCenter + radial * headWidth);

        appendSymbolLine(
            points, lineIndex,
            tip,
            headCenter - radial * headWidth);

        // 中心作用点和力矩轴
        appendSymbolCircle(
            points, lineIndex,
            origin, u, v,
            centerRadius, 18);

        appendSymbolLine(
            points, lineIndex,
            origin - n * axisHalfLength,
            origin + n * axisHalfLength);

        commitLineSymbol(
            symbolController,
            symbolName,
            points,
            lineIndex,
            origin + u * (radius * 1.35f));
    }

    void mMotionVectorSymbolHelper::clearVectorSymbol(
        mxr::mSymbolController* symbolController,
        const QString& symbolName)
    {
        if (!symbolController)
            return;

        symbolController->deleteSymbol({ symbolName });
        symbolController->UpdateAllSymbol();
    }

    void mMotionVectorSymbolHelper::buildOrthonormalBasis(
        const QVector3D& axis,
        QVector3D& u,
        QVector3D& v)
    {
        QVector3D n = axis;
        n.normalize();

        QVector3D ref = (std::abs(n.x()) < 0.9f)
            ? QVector3D(1.0f, 0.0f, 0.0f)
            : QVector3D(0.0f, 1.0f, 0.0f);

        u = QVector3D::crossProduct(ref, n);
        if (u.lengthSquared() < 1e-12f)
        {
            u = QVector3D(0.0f, 1.0f, 0.0f);
        }
        u.normalize();

        v = QVector3D::crossProduct(n, u);
        v.normalize();
    }

    float mMotionVectorSymbolHelper::computeSceneScale(
        mxr::CCSMRendWidget* rendWidget)
    {
        if (!rendWidget)
            return 0.3f;

        mxr::AABB viewAABB = rendWidget->getAABBOfView();
        if (!viewAABB.valid())
            return 0.3f;

        const float diagonal = viewAABB.getDiagonal();
        if (diagonal <= 0.0f)
            return 0.3f;

        // 符号基础尺度为视口包围盒对角线的 5%，兼顾大小模型
        float scale = diagonal * 0.05f;
        if (scale < 0.01f)
            scale = 0.01f;
        else if (scale > 10.0f)
            scale = 10.0f;

        return scale;
    }
}
