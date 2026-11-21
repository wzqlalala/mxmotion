#include "NativeMotionStatics.h"
#include "NativeMotionRuntime.h"

#include "motion/functions/Function_Const.h"
#include "motion/physics/LinkTSDA.h"
#include "motion/timestepper/StaticAnalysis.h"

#include <Eigen/QR>
#include <Eigen/SparseQR>
#include <QJsonArray>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace MotionOcc {
namespace {
[[noreturn]] void fail(const QString& text){throw std::runtime_error(text.toStdString());}
double maximum(const motion::MotionVectorDynamic<>& vector)
{
    return vector.size()?vector.cwiseAbs().maxCoeff():0;
}

class CheckedStaticAnalysis final : public motion::MotionStaticAnalysis {
public:
    CheckedStaticAnalysis(NativeMotionSystem& system,const QJsonObject& scenario,const SolverCallbacks& callbacks)
        :system(system),scenario(scenario),callbacks(callbacks){}
    NativeStaticReport report;
private:
    NativeMotionSystem& system;
    const QJsonObject& scenario;
    const SolverCallbacks& callbacks;
    void StaticAnalysis() override
    {
        const int n=system.GetNcoords_v(),m=system.GetNconstr();
        const int maxIterations=scenario.value("staticMaxIterations").toInt(100);
        const double tolerance=scenario.value("staticTolerance").toDouble(1e-8);
        const double constraintTolerance=scenario.value("staticConstraintTolerance").toDouble(1e-7);
        const double damping=scenario.value("staticStepDamping").toDouble(1);
        const bool verbose=scenario.value("solverVerbose").toBool();
        motion::MotionStateDelta velocity,delta,zeroAcceleration;
        velocity.setZero(n,&system);delta.setZero(n,&system);zeroAcceleration.setZero(n,&system);
        X.setZero(system.GetNcoords_x(),&system);double time=0;
        system.StateGather(X,velocity,time);velocity.setZero(n,&system);L.setZero(m);
        const auto scatter=[&](const motion::MotionState& state){
            system.StateScatter(state,velocity,0,true);
            system.StateScatterReactions(L);
            // SMC contact forces and their K/R blocks are contact-container
            // data. StateScatter alone does not rebuild contacts at a trial
            // geometry, so a Newton residual must explicitly refresh them.
            system.ComputeCollisions();
        };
        scatter(X);
        system.StateScatterAcceleration(zeroAcceleration);
        motion::MotionVectorDynamic<> force(n),reaction(n),residual(n),constraints(m),nextL(m);
        double referenceForce=0;
        const auto evaluate=[&]{
            force.setZero();reaction.setZero();constraints.setZero();
            system.LoadResidual_F(force,1);
            if(n<=192&&m>0&&system.GetNcontacts()>0){
                // A contact manifold can redistribute its corner forces while
                // keeping the same gap. Re-equilibrate the support reactions
                // at the current trial geometry; a stale previous manifold's
                // torque must not prevent a real decrease of the free force.
                // This changes only L, never position or the actual contact
                // law. The accepted check below still uses F + Cq^T L.
                system.prepareStaticDescriptor();motion::MotionSparseMatrix cq;
                system.GetConstraintJacobianMatrix(&cq);
                if(cq.rows()!=m||cq.cols()!=n)fail("接触静力约束矩阵尺寸改变。");
                const Eigen::MatrixXd transpose=cq.transpose();
                L=transpose.completeOrthogonalDecomposition().solve(-force);
                system.StateScatterReactions(L);
            }
            system.LoadResidual_CqL(reaction,L,1);
            system.LoadConstraint_C(constraints,1,false);
            residual=force+reaction;
            if(!force.allFinite()||!reaction.allFinite()||!constraints.allFinite())fail("静力迭代出现非有限力或约束残差。");
            // Freeze the initial generalized-load infinity norm. Normalizing
            // each row by its *current net force* makes an unconstrained row's
            // ratio exactly one throughout nonlinear convergence, and treats
            // roundoff at unloaded FEM nodes as larger than a real 10 N load.
            // A fixed reference both measures progress and remains independent
            // of the trial reaction or a deteriorating iterate.
            if(referenceForce==0)referenceForce=std::max(1.0,maximum(force));
            report.residual=maximum(residual)/referenceForce;
            report.constraintResidual=maximum(constraints);
        };
        for(int iteration=0;iteration<=maxIterations;++iteration){
            if(callbacks.stopRequested&&callbacks.stopRequested())throw NativeStaticCancelled{};
            scatter(X);
            system.prepareStaticDescriptor();
            evaluate();report.iterations=iteration;
            if(verbose)qInfo()<<"static"<<iteration<<report.residual<<report.constraintResidual;
            if(report.residual<=tolerance&&report.constraintResidual<=constraintTolerance){
                system.StateScatterReactions(L);
                system.StateScatterAcceleration(zeroAcceleration);
                return;
            }
            if(iteration==maxIterations)break;
            delta.setZero(n,&system);nextL.setZero();
            if(n<=192){
                // Several legacy forces (notably RSDA and follower loads) do
                // not contribute an analytic K block. Differentiate the real
                // engine residual for small mechanisms; never replace those
                // loads with an artificial spring or accept a zero correction.
                Eigen::MatrixXd matrix=Eigen::MatrixXd::Zero(n+m,n+m);
                motion::MotionVectorDynamic<> plus(n),minus(n);
                for(int column=0;column<n;++column){
                    if(callbacks.stopRequested&&callbacks.stopRequested())throw NativeStaticCancelled{};
                    const double h=1e-5;
                    motion::MotionStateDelta perturb;perturb.setZero(n,&system);perturb[column]=h;
                    const motion::MotionState xplus=X+perturb;
                    scatter(xplus);plus.setZero();system.LoadResidual_F(plus,1);
                    perturb[column]=-h;const motion::MotionState xminus=X+perturb;
                    scatter(xminus);minus.setZero();system.LoadResidual_F(minus,1);
                    matrix.block(0,column,n,1)=-(plus-minus)/(2*h);
                }
                scatter(X);system.prepareStaticDescriptor();
                motion::MotionSparseMatrix cq;
                system.GetConstraintJacobianMatrix(&cq);
                if(cq.rows()!=m||cq.cols()!=n)fail("静力约束矩阵尺寸与实际自由度不一致。");
                matrix.block(0,n,n,m)=-Eigen::MatrixXd(cq.transpose());
                matrix.block(n,0,m,n)=Eigen::MatrixXd(cq);
                Eigen::VectorXd rhs(n+m);rhs.head(n)=force;rhs.tail(m)=-constraints;
                // Equilibrate rows and columns before rank-revealing QR. This
                // keeps mm geometry and kg-mm-s force coefficients comparable.
                Eigen::VectorXd rowScale(n+m),columnScale(n+m);
                for(int row=0;row<n+m;++row){rowScale[row]=1/std::max(1.0,matrix.row(row).cwiseAbs().maxCoeff());matrix.row(row)*=rowScale[row];rhs[row]*=rowScale[row];}
                for(int column=0;column<n+m;++column){columnScale[column]=1/std::max(1e-12,matrix.col(column).cwiseAbs().maxCoeff());matrix.col(column)*=columnScale[column];}
                const auto qr=matrix.completeOrthogonalDecomposition();
                const Eigen::VectorXd solution=qr.solve(rhs);
                const double linearError=(matrix*solution-rhs).cwiseAbs().maxCoeff();
                if(!solution.allFinite()||linearError>1e-7*std::max(1.0,rhs.cwiseAbs().maxCoeff()))
                    fail("静力平衡不存在或切线系统奇异：存在未平衡载荷的自由运动方向，请补充实际支承/弹性约束。");
                const Eigen::VectorXd actual=solution.cwiseProduct(columnScale);
                delta=actual.head(n);nextL=actual.tail(m);
            }else{
                // Full FEA/CMS supply their actual sparse stiffness. Scale the
                // KKT rows and columns just as in the small-system path: mm
                // stiffness can be 1e8 while interface constraint rows are 1.
                // An unscaled rank-revealing solve can otherwise discard real
                // attachment rows and report a numerically small force error.
                motion::MotionSparseMatrix stiffness,cq;
                system.GetStiffnessMatrix(&stiffness);system.GetConstraintJacobianMatrix(&cq);
                if(stiffness.rows()!=n||stiffness.cols()!=n||cq.rows()!=m||cq.cols()!=n)
                    fail("静力有限元 K/Cq 矩阵尺寸与实际自由度不一致。");
                std::vector<Eigen::Triplet<double>> coefficients;
                coefficients.reserve(stiffness.nonZeros()+2*cq.nonZeros());
                for(int row=0;row<stiffness.outerSize();++row)for(motion::MotionSparseMatrix::InnerIterator it(stiffness,row);it;++it)
                    coefficients.emplace_back(it.row(),it.col(),it.value());
                for(int row=0;row<cq.outerSize();++row)for(motion::MotionSparseMatrix::InnerIterator it(cq,row);it;++it){
                    coefficients.emplace_back(n+it.row(),it.col(),it.value());
                    coefficients.emplace_back(it.col(),n+it.row(),-it.value());
                }
                Eigen::SparseMatrix<double> matrix(n+m,n+m);matrix.setFromTriplets(coefficients.begin(),coefficients.end());
                Eigen::VectorXd rhs(n+m),rowScale=Eigen::VectorXd::Ones(n+m),columnScale=Eigen::VectorXd::Zero(n+m);
                rhs.head(n)=force;rhs.tail(m)=-constraints;
                for(int column=0;column<matrix.outerSize();++column)for(Eigen::SparseMatrix<double>::InnerIterator it(matrix,column);it;++it)
                    rowScale[it.row()]=std::max(rowScale[it.row()],std::abs(it.value()));
                rowScale=rowScale.cwiseInverse();rhs=rhs.cwiseProduct(rowScale);
                for(int column=0;column<matrix.outerSize();++column)for(Eigen::SparseMatrix<double>::InnerIterator it(matrix,column);it;++it){
                    it.valueRef()*=rowScale[it.row()];columnScale[it.col()]=std::max(columnScale[it.col()],std::abs(it.value()));
                }
                for(int column=0;column<n+m;++column)columnScale[column]=1/std::max(1e-12,columnScale[column]);
                for(int column=0;column<matrix.outerSize();++column)for(Eigen::SparseMatrix<double>::InnerIterator it(matrix,column);it;++it)
                    it.valueRef()*=columnScale[it.col()];
                matrix.makeCompressed();
                Eigen::SparseQR<Eigen::SparseMatrix<double>,Eigen::COLAMDOrdering<int>> solver;
                solver.compute(matrix);
                if(solver.info()!=Eigen::Success)fail("静力有限元稀疏切线分解失败，请检查支承/接口约束。");
                const Eigen::VectorXd solution=solver.solve(rhs);
                if(solver.info()!=Eigen::Success||!solution.allFinite()||(matrix*solution-rhs).cwiseAbs().maxCoeff()>1e-7*std::max(1.0,rhs.cwiseAbs().maxCoeff()))
                    fail("静力有限元切线不平衡：检查未约束刚体运动或缺少切线刚度的载荷。");
                const Eigen::VectorXd actual=solution.cwiseProduct(columnScale);delta=actual.head(n);nextL=actual.tail(m);
            }
            const double oldMerit=std::max(report.residual/tolerance,report.constraintResidual/constraintTolerance);
            const auto previousX=X;const auto previousL=L;
            bool accepted=false;
            for(int backtrack=0;backtrack<12;++backtrack){
                const double step=damping*std::pow(.5,backtrack);
                motion::MotionStateDelta correction=delta*step;
                X=previousX+correction;L=previousL+(nextL-previousL)*step;
                scatter(X);evaluate();
                const double merit=std::max(report.residual/tolerance,report.constraintResidual/constraintTolerance);
                if(merit<oldMerit||merit<=1){accepted=true;break;}
            }
            if(!accepted)fail(QStringLiteral("静力迭代没有降低实际平衡残差（相对残差 %1，约束残差 %2）；没有输出伪收敛结果。")
                .arg(report.residual,0,'g',10).arg(report.constraintResidual,0,'g',10));
            if(callbacks.progress)callbacks.progress(.1*static_cast<double>(iteration+1)/maxIterations);
        }
        fail(QStringLiteral("静力平衡达到 %1 次迭代仍未收敛：相对力残差 %2（容差 %3），约束残差 %4（容差 %5）。")
            .arg(report.iterations).arg(report.residual,0,'g',10).arg(tolerance,0,'g',10)
            .arg(report.constraintResidual,0,'g',10).arg(constraintTolerance,0,'g',10));
    }
};
}

QString NativeStaticReport::description() const
{
    return QStringLiteral("静力平衡已通过实际残差检查：%1 次 Newton 迭代，相对力/力矩平衡残差 %2，约束残差 %3；以 t=0 载荷和零速度求平衡，保留原网格参考构形及弹性预载。")
        .arg(iterations).arg(residual,0,'g',10).arg(constraintResidual,0,'g',10);
}

NativeStaticReport equilibrateNativeMotion(NativeMotionSystem& system,const QJsonObject& scenario,const SolverCallbacks& callbacks)
{
    // Position prescriptions can have a moving continuation in dynamics, but a
    // true stationary equilibrium cannot satisfy nonzero prescribed speed.
    for(const auto& entry:system.nativeMotors){
        if(system.effortMotors.count(entry.first))continue;
        const auto definition=system.sourceInput->document.entity("drives",entry.first);
        const auto function=entry.second->GetMotorFunction();
        const double speed=definition.value("controlMode").toString()=="Velocity"?function->Get_y(0):function->Get_y_dx(0);
        if(std::abs(speed)>1e-8)fail(QStringLiteral("驱动 %1 在 t=0 指定非零速度，不能作为静止平衡；请让初始速度为零或使用纯动力学方案。").arg(entry.first));
    }
    for(const auto& link:system.Get_linklist())if(const auto spring=std::dynamic_pointer_cast<motion::MotionLinkTSDA>(link))spring->IsStiff(true);
    const auto savedSolver=system.GetSolver();
    struct RestoreSolver{NativeMotionSystem& system;std::shared_ptr<motion::MotionSolver> solver;bool stiffContact;~RestoreSolver(){system.SetSolver(solver);system.SetStiffContact(stiffContact);}} restore{system,savedSolver,system.GetStiffContact()};
    system.SetSolverType(motion::MotionSolver::Type::SPARSE_QR);
    system.SetStiffContact(true);
    CheckedStaticAnalysis analysis(system,scenario,callbacks);
    system.DoStaticAnalysis(analysis);
    return analysis.report;
}
}
