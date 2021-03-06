import os
import glob
import shutil
from setuptools import setup

res = os.system("meson build && ninja -C build")
if res != 0:
    print("Fatal: Error building PyWM")
    exit(1)

so = None
for f in glob.glob('build/_pywm.*.so'):
    so = f

if so is not None:
    shutil.copy(so, 'pywm/_pywm.so')
else:
    print("Fatal: Could not find shared library")
    exit(1)

setup(name='pywm',
      version='0.0.9',
      description='wlroots-based Wayland compositor with Python frontend',
      url="https://github.com/jbuchermn/pywm",
      author='Jonas Bucher',
      author_email='j.bucher.mn@gmail.com',
      package_data={'pywm': ['_pywm.so', 'py.typed'], 'pywm.touchpad': ['py.typed']},
      packages=['pywm', 'pywm.touchpad'],
      install_requires=['evdev', 'imageio', 'pycairo', 'numpy'])
