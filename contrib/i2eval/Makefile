i2tcl.so: i2tcl.cpp i2tcl.hpp
	swig -c++ -o i2tcl_wrap.cpp i2tcl.hpp
	g++ -g -std=c++11 -I/usr/include/tcl8.5 -shared -fpic -Iicinga2/lib -Iicinga2/build -Iicinga2/build/lib -L/opt/icinga2/lib/icinga2 -lbase -lconfig -Wl,-rpath=/opt/icinga2/lib/icinga2 -o i2tcl.so i2tcl_wrap.cpp i2tcl.cpp
