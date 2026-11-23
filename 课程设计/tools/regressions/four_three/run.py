"""Replay the known four-three omission without any match archive."""

import argparse
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[2]
LOSING = (9, 4)
MATE_THRESHOLD = 99_999_000
ENGINE = (
    'core/Board.cpp', 'core/Zobrist.cpp', 'ai/Evaluator.cpp', 'ai/MoveGen.cpp', 'ai/Pattern.cpp',
    'ai/Search.cpp', 'ai/TranspositionTable.cpp',
)


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def instrument(source):
    for before, after in (
        ('namespace { constexpr int INF',
         'void fourThreeArm(int,Point);\nvoid fourThreeValue(int,int);\n'
         'void fourThreeInclude(const Board&,int,std::vector<Point>&);\n'
         'void fourThreeReply(const Board&,int,Point,int,int,int);\n\n'
         'namespace { constexpr int INF'),
        ('            int value=childScore(position,opponent,depth-1,alpha,beta,1,first);',
         '            fourThreeArm(depth,move);\n'
         '            int value=childScore(position,opponent,depth-1,alpha,beta,1,first);'),
        ('            if(timeUp_) break;\n            first=false;',
         '            if(timeUp_) break;\n            fourThreeValue(value,alpha);\n            first=false;'),
        ('    auto moves=moveGen_.generateMoves(board,color,12);',
         '    auto moves=moveGen_.generateMoves(board,color,12);\n    fourThreeInclude(board,ply,moves);'),
        ('        if(timeUp_) return 0;\n        first=false;',
         '        if(timeUp_) return 0;\n'
         '        fourThreeReply(board,ply,move,score,alpha,beta);\n        first=false;'),
    ):
        require(source.count(before) == 1,
                f'Search.cpp changed at probe anchor: {before!r}; use --expect safe for an unmodified check')
        source = source.replace(before, after)
    return source


def build(src, output, overlay=None, *, enable_four_three=False):
    files = [overlay if overlay and name == 'ai/Search.cpp' else src / name for name in ENGINE]
    flags = ['-DFOUR_THREE_TEST_ENABLED'] if enable_four_three else []
    subprocess.run(['g++', '-std=c++17', '-O3', '-DNDEBUG', *flags, f'-I{src}', f'-I{src / "ai"}',
                    HERE / 'probe.cpp', *files, '-o', output], check=True, timeout=90)


def probe(exe, mode):
    p = subprocess.run([exe, mode], text=True, capture_output=True, timeout=60)
    require(p.returncode == 0, f'{mode} probe failed: {p.stderr.strip()}')
    output = {}
    for line in p.stdout.splitlines():
        name, *numbers = line.split()
        require(name not in output, f'duplicate probe row: {name}')
        output[name] = tuple(map(int, numbers))
    require({k: len(v) for k, v in output.items()} == {'candidate': 9, 'root': 6, 'branch': 6},
            f'unexpected probe output: {p.stdout}')
    return output


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=PROJECT / 'src',
                        help='engine src directory (default: submission project)')
    parser.add_argument('--expect', choices=('known', 'safe'), default='safe')
    args = parser.parse_args()
    src = args.source.resolve(strict=True)
    with TemporaryDirectory(prefix='gomoku-four-three-') as tmp:
        temp = Path(tmp)
        baseline = temp / 'baseline.exe'
        build(src, baseline, enable_four_three=args.expect == 'safe')
        stock = probe(baseline, 'off')
        x, y, depth, score, *_ = stock['root']
        print('冲四证书通过：唯一挡点和双胜点均由引擎棋盘逐格核验。')
        mode = '显式开启四三补搜' if args.expect == 'safe' else '冻结基线'
        print(f'{mode}：第 {depth} 层，选 ({x},{y})，分数 {score}。')
        if args.expect == 'safe':
            require((x, y) != LOSING, '仍选择独立证书已证明会输的 (9,4)')
            print('通过：本盘未选择已证败着。')
            return

        rank, cap, retained, four, open_four, three, five, enemy_four, enemy_five = stock['candidate']
        require(rank > 12 and not retained and (four, open_four, five, enemy_four, enemy_five)
                == (1, 0, 0, 0, 0) and three > 0, '首攻不再符合已知四三遗漏')
        require((x, y, depth) == (*LOSING, 6) and score >= MATE_THRESHOLD, '未复现已知近似胜分')
        print(f'候选检查：首攻排名第 {rank}，实际生成 {cap} 手，首攻不在名单内。')
        source = temp / 'Search_probe.cpp'
        source.write_text(instrument((src / 'ai/Search.cpp').read_text(encoding='utf-8')), encoding='utf-8')
        overlay = temp / 'overlay.exe'
        build(src, overlay, source)
        off, on, off_again = (probe(overlay, mode) for mode in ('off', 'on', 'off'))
        require(off == off_again and off['root'] == stock['root'] and
                off['candidate'] == on['candidate'] == stock['candidate'], '开关关闭时未复现基线')
        require(off['branch'][:2] == (1, score) and off['branch'][3:] == (0, 0, 0),
                '关闭时不应搜索证书首攻')
        seen, original_score, _, inserted, completed, exact = on['branch']
        require(seen and inserted == 1 and completed >= 1 and exact and original_score <= -MATE_THRESHOLD,
                '补回后未证实首攻被搜索并使原着翻成近似败分')
        nx, ny, nd, ns, *_ = on['root']
        require(nd == depth and (nx, ny) != LOSING and ns < MATE_THRESHOLD, '根未在同深避开假胜着')
        print(f'单手补回：原着分数 {score} → {original_score}；改选 ({nx},{ny})，根分 {ns}。')
        print('已知缺陷复现通过；这不表示正式版已修复。')


if __name__ == '__main__':
    try:
        run()
    except (OSError, RuntimeError, subprocess.SubprocessError) as exc:
        raise SystemExit(f'失败：{exc}')
