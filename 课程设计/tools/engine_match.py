"""Paired freestyle Gomoku matches. Run with --help for options."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import math
from pathlib import Path
import random
import statistics
import subprocess
import time

SIZE = 15


def won(cells, point):
    x, y = point
    color = cells[point]
    for dx, dy in ((1, 0), (0, 1), (1, 1), (1, -1)):
        count = 1
        for sign in (-1, 1):
            xx, yy = x + dx * sign, y + dy * sign
            while cells.get((xx, yy)) == color:
                count += 1
                xx, yy = xx + dx * sign, yy + dy * sign
        if count >= 5:
            return True
    return False


def legal(cells, point):
    return all(0 <= coordinate < SIZE for coordinate in point) and point not in cells


def load_openings(path):
    openings, ids, positions = [], set(), set()
    for line_no, line in enumerate(path.read_text(encoding='ascii').splitlines(), 1):
        fields = line.partition('#')[0].split()
        if not fields:
            continue
        try:
            opening, family, *coords = fields
            if not (coords and len(coords) % 2 == 0 and len(coords) < 2 * SIZE**2):
                raise ValueError('expected coordinate pairs')
            values = [int(n) for n in coords]
            moves = tuple(zip(values[::2], values[1::2]))
            if opening in ids or moves in positions:
                raise ValueError('duplicate opening')
            cells = {}
            for i, point in enumerate(moves):
                if not legal(cells, point):
                    raise ValueError(f'illegal opening move {point}')
                cells[point] = i % 2 + 1
                if won(cells, point):
                    raise ValueError('terminal opening')
        except ValueError as exc:
            raise ValueError(f'{path}:{line_no}: {exc}') from exc
        ids.add(opening)
        positions.add(moves)
        openings.append((opening, family, moves))
    if not openings:
        raise ValueError(f'{path}: no openings')
    return openings


def read_move(path):
    fields = path.read_text(encoding='ascii').split()
    if len(fields) not in (4, 8):
        raise ValueError(f'expected 4 or 8 fields, got {len(fields)}')
    x, y, depth, nodes = map(int, fields[:4])
    score, reason, status, ms = 0, 'none', 'none', 0
    if len(fields) == 8:
        score, reason, status, ms = fields[4:]
        score, ms = int(score), int(ms)
        if status != 'move':
            raise ValueError(f'invalid move status: {status}')
    if min(depth, nodes, ms) < 0:
        raise ValueError('negative move statistics')
    return (x, y), depth, nodes, score, reason, status, ms


def play(args, index, opening, new_color):
    name, family, initial = opening
    black, white = 'BW'
    game = args.directory / f'{index:04d}-new-{black if new_color == 1 else white}'
    game.mkdir()
    history, reply = game / 'position.txt', game / 'move.txt'
    record_path = game / 'game.txt'
    moves = list(initial)
    cells = {p: i % 2 + 1 for i, p in enumerate(moves)}
    winner, termination = 0, 'draw'
    depths, elapsed = {'old': [], 'new': []}, {'old': [], 'new': []}
    with record_path.open('x', encoding='ascii') as record:
        record.write(f'gomoku-record 1\nmeta mode=match budget_ms={args.budget_ms} '
                     f'opening_plies={len(initial)} new_color={new_color}\n')

        def record_move(ply, point, source, ms=0, depth=0, nodes=0, score=0, reason='none', status='none'):
            x, y = point
            record.write(f'move {ply} {ply} {x} {y} {"BW"[(ply - 1) % 2]} {source} '
                         f'{ms} {depth} {nodes} {score} {reason} {status}\n')
            record.flush()

        for ply, point in enumerate(initial, 1):
            record_move(ply, point, 'human')
        while len(moves) < SIZE**2:
            color = len(moves) % 2 + 1
            side = 'new' if color == new_color else 'old'
            exe = getattr(args, side)
            history.write_text(''.join(f'{x} {y}\n' for x, y in moves), encoding='ascii')
            reply.unlink(missing_ok=True)
            try:
                proc = subprocess.run([str(exe), str(history), str(reply), str(args.budget_ms)],
                                      stdin=subprocess.DEVNULL, capture_output=True,
                                      timeout=args.timeout_seconds, check=True)
            except subprocess.TimeoutExpired:
                winner, termination = 3 - color, 'timeout'
                break
            except subprocess.CalledProcessError as exc:
                raise RuntimeError(f'{game.name}, ply {len(moves)+1}: {exc}; stderr={exc.stderr!r}') from exc
            try:
                point, depth, nodes, score, reason, status, ms = read_move(reply)
            except (OSError, ValueError) as exc:
                raise RuntimeError(f'{game.name}, ply {len(moves)+1}: {exc}; stderr={proc.stderr!r}') from exc
            if not legal(cells, point):
                raise RuntimeError(f'{game.name}, ply {len(moves)+1}: illegal move {point}')
            moves.append(point)
            cells[point] = color
            record_move(len(moves), point, 'ai', ms, depth, nodes, score, reason, status)
            depths[side].append(depth)
            if status == 'move':
                elapsed[side].append(ms)
            if won(cells, point):
                winner, termination = color, 'five'
                break
    if args.replay:
        export = game / 'replayed.txt'
        subprocess.run([str(args.replay), str(record_path), '--position', str(export)],
                       check=True, stdout=subprocess.DEVNULL, timeout=10)
        expected = ''.join(f'{x} {y}\n' for x, y in moves)
        if export.read_text(encoding='ascii') != expected:
            raise RuntimeError(f'{game.name}: replay differs from match')
        export.unlink()
    history.unlink(missing_ok=True)
    reply.unlink(missing_ok=True)
    return dict(opening=name, family=family, new_color=new_color, winner=winner,
                outcome='draw' if not winner else 'new' if winner == new_color else 'old',
                termination=termination, plies=len(moves), moves=moves,
                record=record_path.relative_to(args.directory).as_posix(),
                depths=depths, elapsed_ms=elapsed)


def summarize(games):
    outcomes = {key: sum(g['outcome'] == key for g in games) for key in ('new', 'old', 'draw')}
    families, pairs = {}, {}
    for game in games:
        pairs.setdefault(game['opening'], []).append(game)
    for pair in pairs.values():
        if len(pair) == 2:
            points = {'new': 1, 'old': 0, 'draw': 0.5}
            families.setdefault(pair[0]['family'], []).append(statistics.mean(points[g['outcome']] for g in pair))
    estimate = interval = None
    paired = [v for f in families.values() for v in f]
    if paired:
        estimate = statistics.mean(paired)
    # 同一三手开局派生的变体作为一组重采样。
    if len(families) >= 10:
        groups = [(sum(v), len(v)) for v in families.values()]
        rng = random.Random(20261118)
        samples = []
        for _ in range(10000):
            draw = rng.choices(groups, k=len(groups))
            samples.append(sum(x for x, _ in draw) / sum(n for _, n in draw))
        samples.sort()
        interval = [samples[250], samples[9749]]
    sequences = {tuple(map(tuple, g['moves'])) for g in games}

    def means(field):
        result = {}
        for side in ('old', 'new'):
            values = [n for game in games for n in game[field][side]]
            result[side] = statistics.mean(values) if values else None
        return result

    return dict(outcomes=outcomes, pairs=len(paired), families=len(families), score_rate=estimate,
                family_bootstrap_95=interval, duplicates=len(games)-len(sequences),
                timeouts=sum(g['termination'] == 'timeout' for g in games),
                mean_depth=means('depths'), mean_elapsed_ms=means('elapsed_ms'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('old', type=Path)
    parser.add_argument('new', type=Path)
    parser.add_argument('directory', type=Path, help='new output directory')
    parser.add_argument('budget_ms', nargs='?', type=int, default=100)
    parser.add_argument('--openings', type=Path, default=Path(__file__).with_name('openings.txt'))
    parser.add_argument('--limit', type=int, help='run the first N opening pairs')
    parser.add_argument('--jobs', type=int, default=1)
    parser.add_argument('--timeout-seconds', type=float)
    parser.add_argument('--replay', type=Path, help='verify each saved game with gomoku_replay')
    args = parser.parse_args()
    if min(args.budget_ms, args.jobs) < 1 or (args.limit is not None and args.limit < 1):
        parser.error('budget, jobs and limit must be positive')
    if args.timeout_seconds is None:
        args.timeout_seconds = max(10.0, 3 * args.budget_ms / 1000)
    if not math.isfinite(args.timeout_seconds) or args.timeout_seconds <= 0:
        parser.error('timeout must be finite and positive')
    for name in ('old', 'new', 'replay'):
        path = getattr(args, name)
        if path is not None:
            if not path.is_file():
                parser.error(f'{name} executable not found: {path}')
            setattr(args, name, path.resolve())
    args.directory = args.directory.resolve()
    try:
        openings = load_openings(args.openings)[:args.limit]
        args.directory.mkdir(parents=True, exist_ok=False)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    info = dict(budget_ms=args.budget_ms, jobs=args.jobs, timeout_seconds=args.timeout_seconds,
                opening_count=len(openings), opening_sha256=hashlib.sha256(args.openings.read_bytes()).hexdigest(),
                engines={side: dict(path=str(getattr(args, side)),
                                    sha256=hashlib.sha256(getattr(args, side).read_bytes()).hexdigest())
                         for side in ('old', 'new')})
    (args.directory / 'config.json').write_text(json.dumps(info, indent=2), encoding='utf-8')
    start = time.monotonic()
    games = []
    # 同一局的局面文件私有；黑白顺序交替，避免固定先后顺序。
    with (args.directory / 'games.jsonl').open('x', encoding='utf-8') as journal, ThreadPoolExecutor(args.jobs) as pool:
        futures = [pool.submit(play, args, i, opening, color)
                   for i, opening in enumerate(openings, 1)
                   for color in ((1, 2) if i % 2 else (2, 1))]
        try:
            for future in futures:
                game = future.result()
                games.append(game)
                journal.write(json.dumps(game) + '\n')
                journal.flush()
                print(f'{len(games)}/{len(futures)} {game["opening"]} new={game["new_color"]}: '
                      f'{game["outcome"]} ({game["termination"]}, {game["plies"]} plies)', flush=True)
        except BaseException:
            for future in futures:
                future.cancel()
            raise
    summary = summarize(games)
    info = dict(**info, elapsed_seconds=round(time.monotonic() - start, 2), summary=summary)
    (args.directory / 'results.json').write_text(json.dumps(dict(**info, games=games), indent=2), encoding='utf-8')
    print(json.dumps(info, ensure_ascii=True), flush=True)


if __name__ == '__main__':
    main()
