#!/bin/bash
set -exo pipefail

export PATH="/usr/lib/ccache/bin:/usr/lib/ccache:/usr/lib64/ccache:$PATH"
export CCACHE_DIR=/icinga2/ccache
export CTEST_OUTPUT_ON_FAILURE=1
CMAKE_OPTS=()
SCL_ENABLE_GCC=()
PROTOBUF_INCLUDE_DIR=""
# -Wstringop-overflow is notorious for false positives and has been a problem for years.
# See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88443
# -Wtemplate-id-cdtor leaks from using the generated headers. We should reenable this once
# we're considering moving to C++20 and/or the -ti.hpp files are generated differently.
WARN_FLAGS="-Wall -Wextra -Wno-template-id-cdtor -Wno-stringop-overflow"

case "$DISTRO" in
  alpine:*)
    # Packages inspired by the Alpine package, just
    # - LibreSSL instead of OpenSSL 3 and
    # - no MariaDB or libpq as they depend on OpenSSL.
    # https://gitlab.alpinelinux.org/alpine/aports/-/blob/master/community/icinga2/APKBUILD
    apk add bison boost-dev ccache cmake flex g++ libedit-dev libressl-dev ninja-build tzdata protobuf-dev
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
      ./b2 define=BOOST_COROUTINES_NO_DEPRECATION_WARNING
    )

    ln -vs /usr/bin/cmake3 /usr/local/bin/cmake
    ln -vs /usr/bin/ninja-build /usr/local/bin/ninja
    CMAKE_OPTS+=(-DBOOST_{INCLUDEDIR=/boost_1_69_0,LIBRARYDIR=/boost_1_69_0/stage/lib})
    export LD_LIBRARY_PATH=/boost_1_69_0/stage/lib
    ;;

  amazonlinux:20*)
    dnf install -y amazon-rpm-config bison cmake flex gcc-c++ ninja-build \
      {boost,libedit,mariadb-connector-c,ncurses,openssl,postgresql,systemd,protobuf-lite}-devel
    ;;

  debian:*|ubuntu:*)
    apt-get update
    DEBIAN_FRONTEND=noninteractive apt-get install --no-install-{recommends,suggests} -y \
      bison ccache cmake dpkg-dev flex g++ ninja-build tzdata protobuf-compiler \
      lib{boost-all,edit,mariadb,ncurses,pq,ssl,systemd,protobuf}-dev
    ;;

  fedora:*)
    dnf install -y bison ccache cmake flex gcc-c++ ninja-build redhat-rpm-config \
      {boost,libedit,mariadb,ncurses,openssl,postgresql,systemd,protobuf-lite}-devel
    ;;

  *suse*)
    zypper in -y bison ccache cmake flex gcc-c++ ninja rpm-config-SUSE \
      {lib{edit,mariadb,openssl},ncurses,postgresql,systemd,protobuf}-devel \
      libboost_{context,coroutine,filesystem,iostreams,program_options,regex,system,test,thread}-devel
    ;;

  *rockylinux:*)
    dnf install -y 'dnf-command(config-manager)' epel-release

    case "$DISTRO" in
      *:8)
        dnf config-manager --enable powertools
        # Our Protobuf package on RHEL 8 is built with GCC 13, and since the ABI is not compatible with GCC 8,
        # we need to enable the SCL repository and install the GCC 13 packages to be able to link against it.
        SCL_ENABLE_GCC=(scl enable gcc-toolset-13 --)
        dnf install -y gcc-toolset-13-gcc-c++ gcc-toolset-13-annobin-plugin-gcc
        ;;
      *)
        dnf config-manager --enable crb
        ;;
    esac

    dnf install -y bison ccache cmake gcc-c++ flex ninja-build redhat-rpm-config \
      {boost,bzip2,libedit,mariadb,ncurses,openssl,postgresql,systemd,xz,libzstd}-devel

    # Rocky Linux 8 and 9 don't have a recent enough Protobuf compiler for OTel, so we need to add
    # our repository to install the pre-built Protobuf devel package.
    case "$DISTRO" in
      *:[8-9])
        rpm --import https://packages.icinga.com/icinga.key
        cat > /etc/yum.repos.d/icinga-build-deps.repo <<'EOF'
[icinga-build-deps]
name=Icinga Build Dependencies
baseurl=https://packages.icinga.com/build-dependencies/rhel/$releasever/release
enabled=1
gpgcheck=1
gpgkey=https://packages.icinga.com/icinga.key
EOF
        dnf install -y icinga-protobuf
        # And of course, make sure to add our custom Protobuf includes to the compiler include path.
        PROTOBUF_INCLUDE_DIR="-isystem $(rpm -E '%{_includedir}')/icinga-protobuf"
        # Tell CMake where to find our own Protobuf CMake config files.
        CMAKE_OPTS+=(-DCMAKE_PREFIX_PATH="$(rpm -E '%{_libdir}')/icinga-protobuf/cmake")
        ;;
      *)
        dnf install -y protobuf-lite-devel
    esac
    ;;
esac

case "$DISTRO" in
  alpine:*)
    CMAKE_OPTS+=(
      -DUSE_SYSTEMD=OFF
      -DICINGA2_WITH_MYSQL=OFF
      -DICINGA2_WITH_PGSQL=OFF
      -DCMAKE_{C,CXX}_FLAGS="${WARN_FLAGS}"
    )
    ;;
  debian:*|ubuntu:*)
    CMAKE_OPTS+=(-DICINGA2_LTO_BUILD=ON)
    source <(dpkg-buildflags --export=sh)
    export CFLAGS="${CFLAGS} ${WARN_FLAGS}"
    export CXXFLAGS="${CXXFLAGS} ${WARN_FLAGS}"

    # The default Protobuf compiler is too old for OTel, so we need to turn it off on Debian 11 and Ubuntu 22.04.
    case "$DISTRO" in
      debian:11|ubuntu:22.04)
        CMAKE_OPTS+=(-DICINGA2_WITH_OPENTELEMETRY=OFF)
        ;;
    esac
    ;;
  *)
    # Turn off with OTel on Amazon Linux 2 as the default Protobuf compiler is way too old.
    if [ "$DISTRO" = "amazonlinux:2" ]; then
      CMAKE_OPTS+=(-DICINGA2_WITH_OPENTELEMETRY=OFF)
    fi
    CMAKE_OPTS+=(-DCMAKE_{C,CXX}_FLAGS="$(rpm -E '%{optflags} %{?march_flag}') ${WARN_FLAGS} ${PROTOBUF_INCLUDE_DIR}")
    export LDFLAGS="$(rpm -E '%{?build_ldflags}')"
    ;;
esac

mkdir /icinga2/build
cd /icinga2/build

"${SCL_ENABLE_GCC[@]}" cmake \
  -GNinja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DICINGA2_UNITY_BUILD=ON \
  -DUSE_SYSTEMD=ON \
  -DICINGA2_USER=$(id -un) \
  -DICINGA2_GROUP=$(id -gn) \
  "${CMAKE_OPTS[@]}" ..

ninja -v

ninja test
ninja install
icinga2 daemon -C
