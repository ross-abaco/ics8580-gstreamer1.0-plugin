# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgst8580src.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgst8580src_la_SOURCES = gst8580src.c gst8580src.h gst8580capture.c gst8580capture.h 

# compiler and linker flags used to compile this plugin, set in configure.ac
libgst8580src_la_CFLAGS = $(GST_CFLAGS)
libgst8580src_la_LIBADD = $(GST_LIBS)
libgst8580src_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgst8580src_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gst8580src.h
