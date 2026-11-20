# -*- coding: utf-8 -*-
"""
最优拉丁超立方采样（Optimal LHS）。

从随机 LHS 出发，通过逐列交换迭代优化最小点对距离（maximin 准则），
类似模拟退火但只接受改进解。
"""

from __future__ import annotations

from typing import Any, Dict, List, Optional, Tuple

import numpy as np

from mxsim_optim.base import DOEBase, register_doe
from mxsim_optim.utils import (
    _lhs_unit,
    columnwise_pairwise_min_distance,
    unscale_from_unit,
    validate_positive_int,
)


@register_doe("OPTIMAL LHS")
class OptimalLHS(DOEBase):
    """最优拉丁超立方采样。

    采用逐列成对交换的随机搜索策略，最大化样本点之间的最小欧氏距离。

    超参数
    ------
    n_iterations : int, default=1000
        交换迭代次数。
    seed : int, optional
        随机种子。
    """

    def __init__(
        self,
        design_ranges: List[Tuple[float, float]],
        n_samples: int,
        hyperparams: Optional[Dict[str, Any]] = None,
    ) -> None:
        super().__init__(design_ranges, n_samples, hyperparams)
        self.n_iterations = self.hyperparams.get("n_iterations", 1000)
        self.seed = self.hyperparams.get("seed", None)
        validate_positive_int("n_iterations", self.n_iterations)
        if self.seed is not None:
            validate_positive_int("seed", self.seed, min_val=0)

    def generate(self) -> np.ndarray:
        """生成最优 LHS 采样点。

        Returns
        -------
        np.ndarray, shape (n_samples, n_vars)
        """
        rng = np.random.RandomState(self.seed)

        # 1. 初始 LHS
        U = _lhs_unit(self.n_samples, self.n_vars, seed=rng.randint(0, 2**31))
        best_U = U.copy()
        best_dist = columnwise_pairwise_min_distance(best_U)

        # 2. 逐列交换优化
        for _ in range(self.n_iterations):
            col = rng.randint(0, self.n_vars)
            i, j = rng.choice(self.n_samples, size=2, replace=False)

            # 交换第 col 列的第 i 行和第 j 行
            cand = best_U.copy()
            cand[i, col], cand[j, col] = cand[j, col], cand[i, col]

            d = columnwise_pairwise_min_distance(cand)
            if d > best_dist:
                best_U = cand
                best_dist = d

        # 3. 缩放到物理空间
        return self._apply_discrete(unscale_from_unit(best_U, self.design_ranges))
