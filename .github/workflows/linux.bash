#!/bin/bash
set -exo pipefail

export PATH="/usr/lib/ccache:/usr/lib64/ccache:/opt/rh/devtoolset-11/root/usr/bin:$PATH"
export CCACHE_DIR=/icinga2/ccache
export CTEST_OUTPUT_ON_FAILURE=1
CMAKE_OPTS=''

case "$DISTRO" in
  amazonlinux:2)
    amazon-linux-extras install -y epel
    yum install -y bison ccache cmake3 gcc-c++ flex ninja-build \
      {libedit,mariadb,ncurses,openssl,postgresql,systemd}-devel

    yum install -y bzip2 tar wget
    wget https://boostorg.jfrog.io/artifactory/main/release/1.69.0/source/boost_1_69_0.tar.bz2
    tar -xjf boost_1_69_0.tar.bz2

    (
      cd boost_1_69_0
      ./bootstrap.sh --with-libraries=context,coroutine,date_time,filesystem,iostreams,program_options,regex,system,test,thread
      ./b2
    )

    ln -vs /usr/bin/cmake3 /usr/local/bin/cmake
    ln -vs /usr/bin/ninja-build /usr/local/bin/ninja
    CMAKE_OPTS='-DBOOST_INCLUDEDIR=/boost_1_69_0 -DBOOST_LIBRARYDIR=/boost_1_69_0/stage/lib'
    export LD_LIBRARY_PATH=/boost_1_69_0/stage/lib
    ;;

  amazonlinux:20*)
    dnf install -y bison cmake flex gcc-c++ ninja-build \
      {boost,libedit,mariadb1\*,ncurses,openssl,postgresql,systemd}-devel
    ;;

  centos:*)
    yum install -y centos-release-scl epel-release
    yum install -y bison ccache cmake3 devtoolset-11-gcc-c++ flex ninja-build \
      {boost169,libedit,mariadb,ncurses,openssl,postgresql,systemd}-devel

    ln -vs /usr/bin/cmake3 /usr/local/bin/cmake
    ln -vs /usr/bin/ccache /usr/lib64/ccache/g++
    CMAKE_OPTS='-DBOOST_INCLUDEDIR=/usr/include/boost169 -DBOOST_LIBRARYDIR=/usr/lib64/boost169'
    ;;

  debian:*|ubuntu:*)
    apt-get update
    DEBIAN_FRONTEND=noninteractive apt-get install --no-install-{recommends,suggests} -y bison \
      ccache cmake flex g++ lib{boost-all,edit,mariadb,ncurses,pq,ssl,systemd}-dev ninja-build tzdata
    ;;

  fedora:*)
    dnf install -y bison ccache cmake flex gcc-c++ ninja-build \
      {boost,libedit,mariadb,ncurses,openssl,postgresql,systemd}-devel
    ;;

  opensuse/*)
    zypper in -y bison ccache cmake flex gcc-c++ ninja {lib{edit,mariadb,openssl},ncurses,postgresql,systemd}-devel \
      libboost_{context,coroutine,filesystem,iostreams,program_options,regex,system,test,thread}-devel
    ;;

  rockylinux:*)
    dnf install -y 'dnf-command(config-manager)' epel-release

    case "$DISTRO" in
      *:8)
        dnf config-manager --enable powertools
        ;;
      *)
        dnf config-manager --enable crb
        ;;
    esac

    dnf install -y bison ccache cmake gcc-c++ flex ninja-build \
      {boost,libedit,mariadb,ncurses,openssl,postgresql,systemd}-devel
    ;;
esac

mkdir /icinga2/build
cd /icinga2/build

cmake \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DICINGA2_UNITY_BUILD=ON \
  -DUSE_SYSTEMD=ON \
  -DICINGA2_USER=$(id -un) \
  -DICINGA2_GROUP=$(id -gn) \
  $CMAKE_OPTS ..

ninja

ninja test
ninja install
icinga2 daemon -C
