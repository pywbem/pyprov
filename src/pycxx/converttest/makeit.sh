BUILDDIR=../../../mybuild
SRCDIR=../..

g++ -I$SRCDIR/pycxx -I$SRCDIR/ifc/pyprovider -I/usr/include/python2.4 -DPEGASUS_PLATFORM_LINUX_IX86_GNU -o mytest mytest.cpp $BUILDDIR/src/pycxx/.libs/libpegpycxx.a $BUILDDIR/src/ifc/pyprovider/.libs/libpegpyprovider.a -lpegcommon -lpython2.4
