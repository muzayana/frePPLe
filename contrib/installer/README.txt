
This is a script for creating a windows installer for frePPLe.
The following steps are required:

1) Install NSIS v3.0 or higher (Nullsoft Scriptable Install System)
   This is a free package to create installers.
   Further details on http://nsis.sourceforge.net/

2) Activate the following plugins by copying them from the
   <NSIS>\plugins\x86-ansi folder to <NSIS>\plugins\:
     - AccessControl
     - InstallOptions

3) Compile the executables with Microsoft C++ compiler.
   You'll need to compile before creating the installer.

4) Install Python 3
   Adjust the path appropriately, if required.

5) Install the following Python extensions.
   First, install the normal dependencies:
     pip3 install -r requirements
   The installer uses 2 additional packages
      - py2exe for Python 3, >= 0.9.2.2
      - pywin32
   The installer uses py2exe to create a directory containing the Python
   language (with its libraries and extensions) and the frePPLe user
   interface.
   As the standalone web server we use WSGIServer that is provided by the
   CherryPy project. It is a bit more scalable and robust than the Django
   development server.

6) Download the PostgreSQL binaries for 64-bit windows from:
     http://www.enterprisedb.com/products-services-training/pgbindownload
   Unzip the zip-file in the folder pgsql before running the installer.

CONSIDERING ALL THE ABOVE, BUILDING THE INSTALLER ISN'T FOR BEGINNERS.
IT REQUIRES PROPER UNDERSTANDING OF ALL COMPONENTS AND THE BUILD PROCESS...
