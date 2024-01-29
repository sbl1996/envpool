from setuptools import setup, find_packages

__version__ = "0.0.1"

INSTALL_REQUIRES = [
  "setuptools",
  "wheel",
  "numpy",
  "dm-env",
  "gym>=0.26",
  "gymnasium>=0.26,!=0.27.0",
  "optree>=0.6.0",
  "packaging",
]

setup(
    name="envpool2",
    version=__version__,
    packages=find_packages(include='envpool2*'),
    long_description="",
    install_requires=INSTALL_REQUIRES,
    python_requires=">=3.7",
    include_package_data=True,
)