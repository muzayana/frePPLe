#
# Process this file with automake to produce Makefile.in
#

EXTRA_DIST = build.bat frepple.sln README.txt model.vcxproj utils.vcxproj main.vcxproj solver.vcxproj dll.vcxproj model.vcxproj.filters utils.vcxproj.filters main.vcxproj.filters solver.vcxproj.filters dll.vcxproj.filters 

# Additional in the Enterprise Edition.
EXTRA_DIST += module_webserver.vcxproj module_webserver.vcxproj.filters module_inventoryplanning.vcxproj module_inventoryplanning.vcxproj.filters module_forecast.vcxproj module_forecast.vcxproj.filters
if LICENSE
EXTRA_DIST += createkey.vcxproj createkey.vcxproj.filters createlicense.vcxproj createlicense.vcxproj.filters
endif

clean-local:
	-rm -rf model utils main dll solver module_webserver module_forecast module_inventoryplanning createlicense createkey *.user *.opt *.ncb *.suo *.log *.sdf ipch
