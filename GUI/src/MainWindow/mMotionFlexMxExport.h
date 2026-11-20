#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>

namespace mMotionFlexBridge {
struct FlexInput;
}

namespace mMotionFlexMxExport {

//【MNF 离线缩减·mx 导出】把一条 AutoFlex 条目（FlexInput：体网格+材料+附着组）
//序列化为结构隐式求解器的 .mx 输入（CMS/CB 工况 + globaloutput mnf），交由求解器
//离线生成 .mnf（ADAMS 二进制方言），供多体侧 MnfLoader 使用。
//
//卡片口径与已验证样例（d3t4_mnf_test_op.mx，已成功产出 mnf）逐卡同构：
// implicit_analysis / subcase "cms" / analysis generic / cms
// globaloutput type=mnf / node / element D3T4 / connector rigid(RBE2 接口)
// material isolinearelastic / density / section / solidsection
// constraint ASET / CMSMethod CB / unit MGG-N-MM-S / set / end
//
//单位约定：FlexInput.rho 已是 tonne/mm³、E 为 MPa（N/mm²）——与 MGG,N,MM,S 自洽。
//多附着组：每组一个 RBE2（master=组质心虚拟节点，id 从 maxNode+1 顺排），
//全部 master 进同一 ASET 集（set1）。
//
//返回 false 时 *err 给出原因（附着组为空/网格不可用/文件写入失败等）。
bool exportMxForMnf(const mMotionFlexBridge::FlexInput& in, const QString& mxPath, QString* err);

}  // namespace mMotionFlexMxExport
