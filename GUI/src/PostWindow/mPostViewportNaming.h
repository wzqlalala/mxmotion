#pragma once
#pragma execution_character_set("utf-8")

//Qt
#include <QString>
#include <QStringList>
#include <QRegularExpression>

namespace PostWindow
{
    //////////////////////////////////////////////////////////////////////////
    // 后处理视口命名协议（单一铸造源，2026-10 批次A）：全部视口名的构造集中于此，
    // 业务代码禁止散拼字符串或按子串猜测格式——历史上切面图漏下划线、裸初始视口靠
    // 字面量识别、编号扫描靠 viewport.contains("Cloud") 等均源于散拼。
    // 视口名为进程内标识（guidata 键，按视口名存取），不持久化：同会话创建/查询
    // 都走本工具即保持一致，调整格式无存量兼容负担。
    //
    // 格式约定（四个族）：
    //   初始云图视口（导入即建，无树节点，首个云图收编）  Post_Cloud{modelID}
    //   云图视口（用户创建/缓存回放）                    Post_{modelID}_Cloud{resultID}
    //   工况系（曲线/Word报告/路径）                      Case{caseID}_{Kind}{id}
    //   派生系（挂靠云图视口的子结果）                    {云图视口名}_{Kind}{id}
    //////////////////////////////////////////////////////////////////////////
    namespace mViewportName
    {
        //导入时创建的初始云图视口（无云图子节点承载；常驻，模型删除前不可依赖其名变化）
        inline QString initialCloud(int modelID)
        {
            return QString("Post_Cloud%1").arg(modelID);
        }

        //用户创建/缓存回放的云图视口
        inline QString cloud(int modelID, int resultID)
        {
            return QString("Post_%1_Cloud%2").arg(modelID).arg(resultID);
        }

        //工况系视口：kind 取 "Curve" / "ExportWord" / "Path"
        inline QString caseScoped(const QString& kind, int caseID, int id)
        {
            return QString("Case%1_%2%3").arg(caseID).arg(kind).arg(id);
        }

        //派生系视口：挂靠某云图视口的子结果，kind 取 "Animation"/"Linear"/"Scalar"/
        //"Section"/"Isosurface"/"Vector"/"StreamLine"/"Set"/"Line"/"Arrow"/"Text"/"Log"
        inline QString derived(const QString& resViewName, const QString& kind, int id)
        {
            return QString("%1_%2%3").arg(resViewName).arg(kind).arg(id);
        }

        //////////////////////////////////////////////////////////////////////////
        // 派生级联改名（源云图改名时派生视口前缀跟随）：创建真实派生视口的八种 Kind
        //（Line/Arrow/Text/Log 为注释标记，无独立视口，不参与级联）
        //////////////////////////////////////////////////////////////////////////
        inline const QStringList& derivedKinds()
        {
            static const QStringList kinds = {
                QStringLiteral("Animation"), QStringLiteral("Linear"), QStringLiteral("Scalar"),
                QStringLiteral("Section"), QStringLiteral("Isosurface"), QStringLiteral("Vector"),
                QStringLiteral("StreamLine"), QStringLiteral("Set") };
            return kinds;
        }

        //viewportName 形如 oldPrefix_KindN（八种 Kind 之一、N 为纯数字尾缀）时返回
        //newPrefix_KindN；否则返回空串（非派生或前缀不匹配——级联仅精确前缀命中，
        //避免误伤恰好以 oldPrefix 开头的无关视口名）
        inline QString cascadeDerivedName(const QString& viewportName, const QString& oldPrefix, const QString& newPrefix)
        {
            if (oldPrefix.isEmpty() || !viewportName.startsWith(oldPrefix))
                return QString();
            for (const QString& kind : derivedKinds())
            {
                const QRegularExpression re(QRegularExpression::escape(oldPrefix)
                    + '_' + kind + "(\\d+)$");
                QRegularExpressionMatch m = re.match(viewportName);
                if (m.hasMatch())
                    return newPrefix + QLatin1Char('_') + kind + m.captured(1);
            }
            return QString();
        }
    }
}
