# -*- coding: utf-8 -*-
"""
标准拉丁超立方采样（LHS）。

每维分成 N 个等概率区间，各区间内随机放置一点，保证投影均匀性。
"""

from __future__ import annotations

from typing import Any, Dict, List, Optional, Tuple

import numpy as np

from mxsim_optim.base import DOEBase, register_doe
from mxsim_optim.utils import _lhs_unit, unscale_from_unit, validate_positive_int


@register_doe("LHS")
class LHS(DOEBase):
    """标准拉丁超立方采样。

    在 *design_ranges* 定义的超矩形内生成 *n_samples* 个点，
    每维均匀分层。

    超参数
    ------
    seed : int, optional
        随机种子（可复现）。
    """

    def __init__(
        self,
        design_ranges: List[Tuple[float, float]],
        n_samples: int,
        hyperparams: Optional[Dict[str, Any]] = None,
    ) -> None:
        super().__init__(design_ranges, n_samples, hyperparams)
        self.seed = self.hyperparams.get("seed", None)
        if self.seed is not None:
            validate_positive_int("seed", self.seed, min_val=0)

    def generate(self) -> np.ndarray:
        """生成 LHS 采样点。

        Returns
        -------
        np.ndarray, shape (n_samples, n_vars)
            物理空间中的样本点矩阵。
        """
        # 1. 在 [0,1]^d 中生成 LHS
        U = _lhs_unit(self.n_samples, self.n_vars, seed=self.seed)
        # 2. 缩放到物理空间
        return self._apply_discrete(unscale_from_unit(U, self.design_ranges))
