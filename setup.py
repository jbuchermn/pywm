import os
import glob
import shutil
from setuptools import setup

os.system("meson build && ninja -C build")

so = None
for f in glob.glob('build/_pywm.*.so'):
    so = f

if so is None:
    print("Could not find shared library")
    exit(1)

shutil.copy(so, 'pywm/_pywm.so')

setup(name='pywm',
      version='0.0.1',
      description='wlroots-based Wayland compositor with Python frontend',
      url="https://github.com/jbuchermn/pywm",
      author='Jonas Bucher',
      author_email='j.bucher.mn@gmail.com',
      package_data={'pywm': ['_pywm.so']},
      packages=['pywm', 'pywm.touchpad'],
      install_requires=['evdev', 'imageio', 'pycairo'])
