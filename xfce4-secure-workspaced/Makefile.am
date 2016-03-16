
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfce4-secure-workspaced\" \
	-DDATADIR=\"$(datadir)/xfce4\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	xfce4-secure-workspaced

xfce4_secure_workspaced_SOURCES = \
	main.c

xfce4_secure_workspaced_CFLAGS = \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(GTK2_CFLAGS) \
	$(GIO_UNIX_CFLAGS) \
	$(PLATFORM_CFLAGS)

xfce4_secure_workspaced_LDADD = \
	$(top_builddir)/libxfce4ui/libxfce4ui-$(LIBXFCE4UI_VERSION_API).la \
	$(LIBXFCE4UTIL_LIBS) \
	$(GIO_UNIX_LIBS) \
	$(GTK2_LIBS)

xfce4_secure_workspaced_DEPENDENCIES = \
	$(top_builddir)/libxfce4ui/libxfce4ui-$(LIBXFCE4UI_VERSION_API).la

if MAINTAINER_MODE

endif
