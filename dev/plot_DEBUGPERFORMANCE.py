import matplotlib.pyplot as plt  # type: ignore
import sys
import re


print("Loading data...")
pattern = re.compile(r".*DEBUGPERFORMANCE\[(.*)\]: ([0-9]*.[0-9]*)")

data: list[tuple[str, int, float, float]] = []
with open(sys.argv[1], 'r') as inp:
    idx_render = 0
    for l in inp:
        res = pattern.match(l)
        if res is not None:
            kind, ts = res.group(1), float(res.group(2))
            data += [(kind, idx_render, ts, ts)]

            if kind == "render":
                for i in range(1, len(data)):
                    if i > 1 and data[-i][0] == "render":
                        break
                    data[-i] = data[-i][0], data[-i][1], data[-i][2] - ts, data[-i][3]
                data += [("render2", idx_render + 1, ts, ts)]
                idx_render += 1


data = [d for d in data if d[2] <= 0.]

if sys.argv[2] == "plot":
    visual = {
        'render': 'g.-',
        'render2': 'g.-',
        'damage': 'b.',
        'schedule_frame': 'b+',
        'skip_frame': 'y.',
        'present_frame': 'ro',
        'py_start': 'r+',
        'py_finish': 'r.',
    }

    print("Plotting...")
    plt.figure()
    for kind in visual:
        plt.plot([t[1] for t in data if t[0] == kind], [t[2] for t in data if t[0] == kind], visual[kind], alpha=.5)

    plt.show()
elif sys.argv[2] == "show":
    idx = int(sys.argv[3])
    print([d for d in data if d[1] == idx])
