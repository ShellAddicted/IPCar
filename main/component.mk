#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name).a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#

COMPONENT_EMBED_TXTFILES := webUI/index.min.html webUI/css/stylesheet.min.css webUI/js/ChartJS/Chart.min.js webUI/js/core.min.js
# -D_GLIBCXX_USE_C99 required for std::to_string()
CPPFLAGS += -D_GLIBCXX_USE_C99