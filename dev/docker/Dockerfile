FROM debian:jessie
RUN apt-get update
RUN apt-get install -y clang cmake make ccache flex bison libboost-all-dev libssl-dev git-core cgdb
RUN mkdir /root/build
ADD build /root/build
ENV HOME /root
