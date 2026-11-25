# 四三首攻被裁：回归用例

教学引擎的 `search_regressions` 自带两个独立用例（`28-22`、`2-17`）。它逐格核验四三胜证，在关闭修复时复现原着的假胜分，开启后要求同深避开该着，并核对已预热置换表不串用分数。回归不等于棋力验证。

从教学仓根目录可单独快速检查旧盘，只需 Python 和现有 Windows/MinGW `g++`；不依赖已编译引擎、旧棋谱目录或 raylib：

```powershell
python -B Gomuku/tools/regressions/four_three/run.py
```

默认从 `Gomuku/src` 临时编译（`--expect safe`），要求未插桩的搜索在旧盘不再选择已证败着。需要历史复现时，必须用 `--source` 指定未修的冻结源码并显式传入 `--expect known`：

```powershell
python -B Gomuku/tools/regressions/four_three/run.py --source Gomuku/build-match/line-mask-pair-v1/variant/src --expect known
```

`known` 会复现近似胜分，并只补回一手首攻确认因果。两个盘面分别来自 `q-horizon-holdout-v1/slot-028` 第 22 手和 `loss-frontier-v1/slot-002` 第 17 手，坐标从零开始。
