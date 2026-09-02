#!/usr/bin/env bash

# Helper script for quickly building Icinga 2 in a container.
#
# CAUTION: This modifies the system by automatically installing required dependencies and should
# only be used on test systems that will be thrown away afterwards, like in a CI environment.
#
# The behavior can be tweaked by setting the following environment variables:
#
#  - ICINGA2_SOURCE_DIR: Path where to find the Icinga 2 source tree.
#  - ICINGA2_BUILD_DIR: Path where to place the CMake build directory.
#  - CCACHE_DIR: Path where to keep a ccache cache directory.
#  - DISTRO: Container image name, this is used to automatically detect build dependencies and compile flags.
#  - ICINGA2_CI_BUILD_JOBS_RUNNER_${CI_RUNNER_ID} (dynamic name): number of parallel build jobs, see below for details.

set -exo pipefail

: "${ICINGA2_SOURCE_DIR:=/icinga2}"
: "${ICINGA2_BUILD_DIR:=${ICINGA2_SOURCE_DIR}/build}"
: "${CCACHE_DIR:=${ICINGA2_SOURCE_DIR}/ccache}"

export PATH="/usr/lib/ccache/bin:/usr/lib/ccache:/usr/lib64/ccache:$PATH"
export CTEST_OUTPUT_ON_FAILURE=1
export CCACHE_DIR

CMAKE_OPTS=()

case "$DISTRO" in
  alpine:*)
    # Packages inspired by the Alpine package, just
    # - LibreSSL instead of OpenSSL 3 and
    # - no MariaDB or libpq as they depend on OpenSSL.
    # https://gitlab.alpinelinux.org/alpine/aports/-/blob/master/community/icinga2/APKBUILD
    apk add bison boost-dev ccache cmake flex g++ libedit-dev libressl-dev ninja-build tzdata
    ln -vs /usr/lib/ninja-build/bin/ninja /usr/local/bin/ninja
    ;;

  amazonlinux:2)
    amazon-linux-extras install -y epel
    yum install -y bison ccache cmake3 gcc-c++ flex ninja-build system-rpm-config \
      {libedit,mariadb,ncurses,openssl,postgresql,systemd}-devel

    yum install -y bzip2 tar wget
    wget https://archives.boost.io/release/1.69.0/source/boost_1_69_0.tar.bz2
    tar -xjf boost_1_69_0.tar.bz2

    (
      cd boost_1_69_0
      ./bootstrap.sh --with-libraries=context,coroutine,date_time,filesystem,iostreams,program_options,regex,system,test,thread
      ./b2
    )

    ln -vs /usr/bin/cmake3 /usr/local/bin/cmake
    ln -vs /usr/bin/ninja-build /usr/local/bin/ninja
    CMAKE_OPTS+=(-DBOOST_{INCLUDEDIR=/boost_1_69_0,LIBRARYDIR=/boost_1_69_0/stage/lib})
    export LD_LIBRARY_PATH=/boost_1_69_0/stage/lib
    ;;

  amazonlinux:20*)
    dnf install -y amazon-rpm-config bison cmake flex gcc-c++ ninja-build \
      {boost,libedit,mariadb-connector-c,ncurses,openssl,postgresql,systemd}-devel
    ;;

  debian:*|ubuntu:*)
    apt-get update
    DEBIAN_FRONTEND=noninteractive apt-get install --no-install-{recommends,suggests} -y \
      bison ccache cmake dpkg-dev flex g++ ninja-build tzdata \
      lib{boost-all,edit,mariadb,ncurses,pq,ssl,systemd}-dev
    ;;

  fedora:*)
    dnf install -y bison ccache cmake flex gcc-c++ ninja-build redhat-rpm-config \
      {boost,libedit,mariadb,ncurses,openssl,postgresql,systemd}-devel
    ;;

  *suse*)
    zypper in -y --allow-downgrade bison ccache cmake flex gcc-c++ ninja rpm-config-SUSE \
      {lib{edit,mariadb,openssl},ncurses,postgresql,systemd}-devel \
      libboost_{context,coroutine,filesystem,iostreams,program_options,regex,system,test,thread}-devel
    ;;

  *rockylinux:*)
    dnf install -y 'dnf-command(config-manager)' epel-release

    case "$DISTRO" in
      *:8)
        dnf config-manager --enable powertools
        ;;
      *)
        dnf config-manager --enable crb
        ;;
    esac

    dnf install -y bison ccache cmake gcc-c++ flex ninja-build redhat-rpm-config \
      {boost,bzip2,libedit,mariadb,ncurses,openssl,postgresql,systemd,xz,libzstd}-devel
    ;;
esac

case "$DISTRO" in
  alpine:*)
    CMAKE_OPTS+=(-DUSE_SYSTEMD=OFF -DICINGA2_WITH_MYSQL=OFF -DICINGA2_WITH_PGSQL=OFF)
    ;;
  debian:*|ubuntu:*)
    CMAKE_OPTS+=(-DICINGA2_LTO_BUILD=ON)
    source <(dpkg-buildflags --export=sh)
    ;;
  *)
    CMAKE_OPTS+=(-DCMAKE_{C,CXX}_FLAGS="$(rpm -E '%{optflags} %{?march_flag}')")
    export LDFLAGS="$(rpm -E '%{?build_ldflags}')"
    ;;
esac

mkdir -p "$ICINGA2_BUILD_DIR"
cd "$ICINGA2_BUILD_DIR"

cmake \
  -GNinja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DICINGA2_UNITY_BUILD=ON \
  -DUSE_SYSTEMD=ON \
  -DICINGA2_USER=$(id -un) \
  -DICINGA2_GROUP=$(id -gn) \
  "${CMAKE_OPTS[@]}" \
  "$ICINGA2_SOURCE_DIR"

NINJA_OPTS=(-v)

# Allow to customize the number of parallel build jobs to scale the jobs to the available resources provided by
# different GitLab runners. Setting `ICINGA2_CI_BUILD_JOBS_RUNNER_23=4` and `ICINGA2_CI_BUILD_JOBS_RUNNER_42=8` in the
# CI variables of the project causes the runners with ID 23 and 42 to use `-j 4` and `-j 8` respectively.
# If not set, ninja uses its default.
if [[ -v CI_RUNNER_ID ]]; then
  jobs_var_name="ICINGA2_CI_BUILD_JOBS_RUNNER_${CI_RUNNER_ID}"
  if [[ -v "$jobs_var_name" ]]; then
    NINJA_OPTS+=(-j "${!jobs_var_name}")
  fi
fi

ninja "${NINJA_OPTS[@]}"

ninja test
ninja install
icinga2 daemon -C
