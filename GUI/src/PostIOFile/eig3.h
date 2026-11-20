#pragma once
// =============================================================================
// 来源备案（第三方代码引入审计 2026-11-11）：
//   算法：对称 3x3 实矩阵特征分解（EISPACK 的 tred2/tql2 Algol 过程之 C++ 翻译）。
//   出处：公有领域广泛流传的 "eig3" 实现（Connelly Barnes 公开版本，
//         同一版本亦见于 MathWorld "Eigensystem" 词条附码等公开渠道）。
//   许可：Public Domain，无署名/传接义务。
//   引入方式：整文件拷贝，仅包入 namespace MIOFile，算法体未改动。
//   使用方：仅 PostIOFile/Stresslinearition.cpp（应力线性化的主应力/主方向分解）。
// =============================================================================
// Symmetric matrix A => eigenvectors in columns of V, corresponding eigenvalues in d.
namespace MIOFile
{
	void eigen_decomposition(double A[3][3], double V[3][3], double d[3]);
}