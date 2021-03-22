from __future__ import annotations

import matplotlib.pyplot as plt # type: ignore
from .touch import find_all_touchpads, Touchpad, TouchpadUpdate


class TouchpadLog:
    def __init__(self, touchpad: Touchpad) -> None:
        self.ns: list[tuple[float, int]] = []
        self.xs: dict[int, list[tuple[float, float]]] = {}
        self.ys: dict[int, list[tuple[float, float]]] = {}
        self.zs: dict[int, list[tuple[float, float]]] = {}

        touchpad.listener(self.on_update)

    def plot(self) -> None:
        if len(self.ns) == 0 or max([d[1] for d in self.ns]) <= 1:
            return

        fig, ax = plt.subplots(4, 1, sharex=True)
        plt.sca(ax[0])
        plt.plot([d[0] - self.ns[0][0] for d in self.ns],
                 [d[1] for d in self.ns], '-')
        plt.sca(ax[1])
        for i in self.xs:
            plt.plot([d[0] - self.ns[0][0] for d in self.xs[i]],
                     [d[1] for d in self.xs[i]], '-', label=str(i))
        plt.sca(ax[2])
        for i in self.ys:
            plt.plot([d[0] - self.ns[0][0] for d in self.ys[i]],
                     [d[1] for d in self.ys[i]], '-', label=str(i))
        plt.sca(ax[3])
        for i in self.zs:
            plt.plot([d[0] - self.ns[0][0] for d in self.zs[i]],
                     [d[1] for d in self.zs[i]], '-', label=str(i))

        plt.legend()
        plt.show(block=False)
        plt.savefig('tmp.png')
        input("Done? ")
        plt.close(fig)

    def on_update(self, update: TouchpadUpdate) -> None:
        if update.n_touches == 0:
            self.plot()
            self.ns = []
            self.xs = {}
            self.ys = {}
            self.zs = {}
        else:
            self.ns += [(update.t, update.n_touches)]
            for i, x, y, z in update.touches:
                if i not in self.xs:
                    self.xs[i] = []
                if i not in self.ys:
                    self.ys[i] = []
                if i not in self.zs:
                    self.zs[i] = []

                self.xs[i] += [(update.t, x)]
                self.ys[i] += [(update.t, y)]
                self.zs[i] += [(update.t, z)]

