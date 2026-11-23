# 四三首攻被裁：回归用例

提交版默认关闭四三补搜。`tools/search_tests.cpp` 包含两个独立用例（`28-22`、`2-17`）：逐格核验四三胜证，以默认策略复现原着的假胜分，显式开启后要求同深避开该着，并核对已预热置换表不串用分数。

从提交仓根目录可单独快速检查旧盘，只需 Python 和现有 Windows/MinGW `g++`；不依赖已编译引擎、旧棋谱目录或 raylib：

```powershell
python -B 课程设计/tools/regressions/four_three/run.py
```

默认从 `课程设计/src` 临时编译（`--expect safe`），只在探针中显式开启修复，要求旧盘不再选择已证败着。需要历史复现时，用 `--source` 指定未修的冻结源码并传入 `--expect known`：

```powershell
python -B 课程设计/tools/regressions/four_three/run.py --source <冻结源码目录> --expect known
```

`known` 会复现近似胜分，并只补回一手首攻确认因果。两个盘面分别来自 `q-horizon-holdout-v1/slot-028` 第 22 手和 `loss-frontier-v1/slot-002` 第 17 手，坐标从零开始。
