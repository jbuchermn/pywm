import numpy as np

"""
Python is great
"""
try:
    from .touchpad import TouchpadUpdate
except Exception:
    from touchpad import TouchpadUpdate


_coherence_bound = .02


class SanitizeBogusIds:
    def __init__(self, touchpad):
        self._last_update = None
        self._listeners = []

        self._next_id = 1
        self._id_map = {}

        touchpad.listener(self.on_update)

    def listener(self, l):
        self._listeners += [l]

    def _sanitize(self, old, new):
        if len(old.touches) == 0 or len(new.touches) == 0:
            return

        coherence_matrix = np.zeros(shape=(len(old.touches), len(new.touches)))
        for i in range(len(old.touches)):
            for j in range(len(new.touches)):
                coherence = (old.touches[i][1] - new.touches[j][1])**2 + \
                    (old.touches[i][2] - new.touches[j][2])**2

                coherence_matrix[i, j] = coherence

        """
        Decide on correspondences
        """
        coherence_matrix[coherence_matrix > _coherence_bound] = 2.

        for i in range(len(old.touches)):
            m = np.argmin(coherence_matrix[i, :])
            for j in range(len(new.touches)):
                if j != m:
                    coherence_matrix[i, j] = 2.

        for j in range(len(new.touches)):
            m = np.argmin(coherence_matrix[:, j])
            for i in range(len(old.touches)):
                if i != m:
                    coherence_matrix[i, j] = 2.

        """
        Implement replacements
        """
        for j in range(len(new.touches)):
            if min(coherence_matrix[:, j]) <= 1.:
                old_corresp = np.argmin(coherence_matrix[:, j])
                old_id = old.touches[old_corresp][0]
                new_id = new.touches[j][0]

                if old_id != new_id:
                    i = self._id_map[old_id]
                    del self._id_map[old_id]
                    self._id_map[new_id] = i

        """
        Implement new tracks
        """
        for j in range(len(new.touches)):
            if min(coherence_matrix[:, j]) > 1.:
                self._id_map[new.touches[j][0]] = self._next_id
                self._next_id += 1

    def on_update(self, update):
        if self._last_update is None or self._last_update.n_touches == 0:
            for i, _, _, _ in update.touches:
                self._id_map[i] = self._next_id
                self._next_id += 1
        else:
            if set([d[0] for d in self._last_update.touches]) \
                    != set([d[0] for d in update.touches]):
                self._sanitize(self._last_update, update)

        san_update = TouchpadUpdate(update.n_touches, [])
        for i in range(len(update.touches)):
            san_update.touches += [(self._id_map[update.touches[i][0]],
                                    *update.touches[i][1:])]

        for l in self._listeners:
            l(san_update)

        self._last_update = update
        if update.n_touches == 0:
            self._id_map.clear()



