from __future__ import annotations
from typing import Optional

class DamageTracked:
    def __init__(self, parent: Optional[DamageTracked]=None) -> None:
        self._damage_parent = parent
        self._damage_children: list[DamageTracked] = []

        if self._damage_parent is not None:
            self._damage_parent._damage_children += [self]

        self._damaged = False

    def damage_finish(self) -> None:
        if self._damage_parent is not None:
            self._damage_parent._damage_children.remove(self)

    def damage(self, propagate: bool=True) -> None:
        self._damaged = True
        if propagate:
            for c in self._damage_children:
                c.damage(propagate=True)

    def is_damaged(self) -> bool:
        res = self._damaged
        self._damaged = False
        return res
