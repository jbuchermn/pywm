import subprocess
import glob
import shutil
from setuptools import setup

proc = subprocess.Popen(["meson", "build"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
stdout, stderr = proc.communicate()
if proc.returncode != 0:
    raise Exception("Fatal: Error executing 'meson build': \n%r" % stderr)


proc1 = subprocess.Popen(["ninja", "-C", "build"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
stdout, stderr = proc1.communicate()
if proc1.returncode != 0:
    raise Exception("Fatal: Error executing 'ninja -C build': \n%r" % stderr)

so = None
for f in glob.glob('build/_pywm.*.so'):
    so = f

if so is not None:
    shutil.copy(so, 'pywm/_pywm.so')
else:
    raise Exception("Fatal: Could not find shared library")

setup(name='pywm',
      version='0.1.0',
      description='wlroots-based Wayland compositor with Python frontend',
      url="https://github.com/jbuchermn/pywm",
      author='Jonas Bucher',
      author_email='j.bucher.mn@gmail.com',
      package_data={'pywm': ['_pywm.so', 'py.typed'], 'pywm.touchpad': ['py.typed']},
      packages=['pywm', 'pywm.touchpad'],
      install_requires=['evdev', 'imageio', 'pycairo', 'numpy'])
