FROM debian:jessie
RUN apt-get update
RUN apt-get install -y clang cmake make ccache flex bison libboost-all-dev libssl-dev git-core cgdb subversion
RUN cd /root && mkdir gdb_printers && cd gdb_printers && svn co svn://gcc.gnu.org/svn/gcc/trunk/libstdc++-v3/python
RUN cd /root && git clone https://github.com/ruediger/Boost-Pretty-Printer
RUN mkdir /root/build
ADD build /root/build
ENV HOME /root
