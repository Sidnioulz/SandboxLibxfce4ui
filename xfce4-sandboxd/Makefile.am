
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfce4-sandboxd\" \
	-DDATADIR=\"$(datadir)/xfce4\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	xfce4-sandboxd

xfce4_sandboxd_SOURCES = \
	main.c

xfce4_sandboxd_CFLAGS = \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(GTK2_CFLAGS) \
	$(GIO_UNIX_CFLAGS) \
	$(PLATFORM_CFLAGS)

xfce4_sandboxd_LDADD = \
	$(top_builddir)/libxfce4ui/libxfce4ui-$(LIBXFCE4UI_VERSION_API).la \
	$(LIBXFCE4UTIL_LIBS) \
	$(GIO_UNIX_LIBS) \
	$(GTK2_LIBS)

xfce4_sandboxd_DEPENDENCIES = \
	$(top_builddir)/libxfce4ui/libxfce4ui-$(LIBXFCE4UI_VERSION_API).la

if MAINTAINER_MODE

endif
