#
# Process this file with automake to produce Makefile.in
#

nobasedir = $(datadir)/frepple
nobase_DATA = frepple.xsd frepple_core.xsd init.xml

if LICENSE
# Full installation of all files
nobase_DATA += license.xml license_community_edition.xml
else
# Install only secure components
install-data-local:
	echo "Installing configuration files in $(DESTDIR)$(datadir)/frepple"; \
	$(INSTALL_DATA) license_community_edition.xml $(DESTDIR)$(datadir)/frepple/license.xml
	for p in $(nobase_DATA); do \
	  $(INSTALL_DATA) $$p "$(DESTDIR)$(datadir)/frepple/$$p"; \
	done
endif

EXTRA_DIST = $(nobase_DATA)

clean-local:
	-rm -rf *.tds *.exe *.lib *.pdb *.dll *.so *.so.* *.exp *.tds *.BAK *.ilk mod_*.xsd *.out output*.* frepple *.log *.sqlite createkey* createlicense*
