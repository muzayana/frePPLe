#
# Process this file with automake to produce Makefile.in
#

nobasedir = $(datadir)/frepple
nobase_DATA = frepple.xsd frepple_core.xsd

pkgsysconfdir = $(sysconfdir)/frepple
if LICENSE
# Full installation of all files
pkgsysconf_DATA = license.xml license_community_edition.xml init.xml
install-data-hook:
	cd "$(DESTDIR)$(datadir)/frepple" && $(LN_S) "$(DESTDIR)$(pkgsysconfdir)/license.xml" license.xml
else
# Install only secure components
pkgsysconf_DATA = license.xml init.xml
install-data-hook:
	rm "$(DESTDIR)$(pkgsysconfdir)/license.xml"
	$(INSTALL_DATA) license_community_edition.xml "$(DESTDIR)$(pkgsysconfdir)/license.xml"
	cd "$(DESTDIR)$(datadir)/frepple" && $(LN_S) "$(pkgsysconfdir)/license.xml" license.xml
endif

EXTRA_DIST = $(nobase_DATA) $(pkgsysconf_DATA)

clean-local:
	-rm -rf *.tds *.exe *.lib *.pdb *.dll *.so *.so.* *.exp *.tds *.BAK *.ilk mod_*.xsd *.out output*.* frepple *.log *.sqlite createkey* createlicense*
