import matplotlib.pyplot as plt  # type: ignore
import sys
import re


print("Loading data...")
pattern = re.compile(r".*DEBUGPERFORMANCE\[(.*)\(([0-9]*)\)\]: ([0-9]*.[0-9]*)")

output = int(sys.argv[2])

data: list[tuple[str, float, float, float, int]] = []
with open(sys.argv[1], 'r') as inp:
    last_render = -1.
    idx_render = 0
    for l in inp:
        res = pattern.match(l)
        if res is not None:
            kind, o, ts = str(res.group(1)), int(res.group(2)), float(res.group(3))
            if o != 0 and o != output and kind != "py_start" and kind != "py_finish":
                continue
            if last_render > 0:
                data += [(kind, idx_render, ts - last_render, ts, idx_render)]

            if kind == "render":
                last_render = ts
                idx_render += 1
                data += [("render2", idx_render, 0, ts, idx_render)]


zero_ts = data[0][3]
cur_idx = -1.
cur_ts = 0.
for i in range(len(data)):
    if data[i][1] != cur_idx:
        cur_idx, cur_ts = data[i][1], data[i][3] - zero_ts
    data[i] = data[i][0], cur_ts, data[i][2], data[i][3] - zero_ts, data[i][4]



if sys.argv[3] == "plot":
    visual = {
        'render': 'g.-',
        'render2': 'g.-',
        'damage': 'b.',
        'schedule_frame': 'b+',
        'skip_frame': 'gx',
        'present_frame': 'ro',
        'py_start': 'r+',
        'py_finish': 'r.',
        'callback_start': 'y+',
        'callback_finish': 'y.',
    }

    print("Plotting...")
    plt.figure()

    for d in data:
        if d[0] == 'enter_constant_damage':
            plt.axvline(d[1], color='gray')
        elif d[0] == 'exit_constant_damage':
            plt.axvline(d[1], color='gray', linestyle='--')

    plt.plot([t[1] for t in data], [t[4] for t in data], 'k.')
    for kind in visual:
        plt.plot([t[1] for t in data if t[0] == kind], [1000.*t[2] for t in data if t[0] == kind], visual[kind], alpha=.5)

    plt.show()
elif sys.argv[3] == "show":
    idx = int(sys.argv[4])
    for d in data:
        if d[4] == idx:
            print(d)
