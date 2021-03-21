import matplotlib.pyplot as plt
import time

from .touch import find_all_touchpads, Touchpad
from .gestures import Gestures, GestureListener, LowpassGesture


class GesturesLog:
    def __init__(self, gestures):
        self.ts = []
        self.values = []
        self.lp_ts = []
        self.lp_values = []

        gestures.listener(self.on_update)

    def plot(self):
        if len(self.ts) == 0:
            return

        fig, ax = plt.subplots(len(self.values[0]), 1, sharex=True)
        for i, k in enumerate(self.values[0].keys()):
            ax[i].set_title(k)
            plt.sca(ax[i])
            plt.plot([t - self.ts[0] for t in self.ts],
                     [d[k] for d in self.values], '-o')
            plt.plot([t - self.ts[0] for t in self.lp_ts],
                     [d[k] for d in self.lp_values], '-o')

        plt.show(block=False)
        plt.savefig('tmp.png')
        input("Done? ")
        plt.close(fig)

    def on_gesture_lp_update(self, values):
        self.lp_ts += [time.time()]
        self.lp_values += [values]

    def on_gesture_update(self, values):
        self.ts += [time.time()]
        self.values += [values]

    def on_gesture_terminate(self):
        self.plot()
        self.ts = []
        self.values = []
        self.lp_ts = []
        self.lp_values = []

    def on_update(self, gesture):
        gesture.listener(GestureListener(
            self.on_gesture_update,
            None))
        LowpassGesture(gesture).listener(GestureListener(
            self.on_gesture_lp_update,
            self.on_gesture_terminate))

