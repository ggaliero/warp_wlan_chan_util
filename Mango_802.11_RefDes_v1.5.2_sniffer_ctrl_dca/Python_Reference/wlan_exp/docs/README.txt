WLAN Exp Documentation

HTML documentation for the WLAN Experiments framework is generated directly from 
the source code documentation using Sphinx.

Requirements:
  - Sphinx v1.2.3 (and any dependencies)
  - make

How to build:
  - For a WLAN Exp installation in: 
        <installation directory>\Mango_802.11_RefDes_v<version>\Python_Reference
    where <version> is the version of the reference design, e.g. 1.5.2, and the 
    <installation directory> is where the 802.11 reference design was unzipped.
    This directory will be referred to below as <WLAN_EXP_BASE_DIR>.

  - In a terminal that supports make (such as a Cygwin Terminal), run:
        cd <WLAN_EXP_BASE_DIR>/wlan_exp/docs
        PYTHONPATH=<WLAN_EXP_BASE_DIR> make html
    
  - Use your preferred web browser to open the documentation in:
        <WLAN_EXP_BASE_DIR>/wlan_exp/docs/build/html/index.html

