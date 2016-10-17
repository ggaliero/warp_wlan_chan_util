import os
import glob

from setuptools import setup, find_packages

#Excellent guide on packaging python:
# https://hynek.me/articles/sharing-your-labor-of-love-pypi-quick-and-dirty/

def read(*paths):
    """Build a file path from *paths* and return the contents."""
    with open(os.path.join(*paths), 'r') as f:
        return f.read()

setup(
    name='mango-wlan-exp',
    version='1.3.0',
    description='Experiments framework for Mango 802.11 Reference Design',
    long_description=(read('README')),
    url='http://warpproject.org/w/802.11/',
    license='WARP - http://warpproject.org/license',
    author='Mango Communications, Inc.',
    author_email='info@mangocomm.com',
    package_dir = {'wlan_exp': '.'}, #setup.py lives at same level as wlan_exp modules
    packages=['wlan_exp', 'wlan_exp.warpnet', 'wlan_exp.log'],
    include_package_data=True,
    zip_safe=False,
    classifiers=[
        'Private :: Do Not Upload', #Use bogus classifier to block accidental PyPI uploads for now
    ],
)
