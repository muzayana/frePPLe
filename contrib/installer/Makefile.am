#
# Process this file with automake to produce Makefile.in
#

EXTRA_DIST = frepple.nsi README.txt frepple.bmp frepplectl.py setup.py freppleservice.py freppleserver.py parameters.ini finish.ini finish.bmp licensefile.ini

clean-local:
	-rm -rf dist build *.pyc

# Create the 32-bit installer.
# We assume the frePPLe binaries have already been compiled for 32-bits.
contrib:
	"/c/Program Files (x86)/NSIS/makensis.exe" /DPYTHON='c:\develop\python34' frepple.nsi

# Create the 64-bit installer.
# We assume the frePPLe binaries have already been compiled for 64-bits.
contrib64:
	"/c/Program Files (x86)/NSIS/makensis.exe" /DPYTHON='c:\develop\python34_64bit' /D64BIT frepple.nsi

py2exe:
	setup.py
