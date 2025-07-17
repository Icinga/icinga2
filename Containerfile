# Icinga 2 Docker image | (c) 2025 Icinga GmbH | GPLv2+

FROM debian:bookworm-slim AS build-base

# Install all the necessary build dependencies for building Icinga 2 and the plugins.
#
# This stage includes the build dependencies for the plugins as well, so that they can share the same base
# image, since Docker builds common stages only once [^1] even if they are used in multiple build stages.
# This eliminates the need to have a separate base image for the plugins, that basically has kind of the
# same dependencies as the Icinga 2 build stage (ok, not exactly the same, but some of them are shared).
#
# [^1]: https://docs.docker.com/build/building/best-practices/#create-reusable-stages
RUN apt-get update && \
    apt-get install -y --no-install-recommends --no-install-suggests \
        autoconf \
        automake \
        bison \
        ccache \
        cmake \
        flex \
        g++ \
        git \
        libboost1.74-dev \
        libboost-context1.74-dev \
        libboost-coroutine1.74-dev \
        libboost-date-time1.74-dev \
        libboost-filesystem1.74-dev \
        libboost-iostreams1.74-dev \
        libboost-program-options1.74-dev \
        libboost-regex1.74-dev \
        libboost-system1.74-dev \
        libboost-thread1.74-dev \
        libboost-test1.74-dev \
        libedit-dev \
        libmariadb-dev \
        libpq-dev \
        libssl-dev \
        libsystemd-dev \
        make && \
    rm -rf /var/lib/apt/lists/*

# Set the default working directory for subsequent commands of the next stages.
WORKDIR /icinga2-build

FROM build-base AS build-plugins

# Install all the plugins that are not included in the monitoring-plugins package.
ADD https://github.com/lausser/check_mssql_health.git#747af4c3c261790341da164b58d84db9c7fa5480 /check_mssql_health
ADD https://github.com/lausser/check_nwc_health.git#a5295475c9bbd6df9fe7432347f7c5aba16b49df /check_nwc_health
ADD https://github.com/bucardo/check_postgres.git#58de936fdfe4073413340cbd9061aa69099f1680 /check_postgres
ADD https://github.com/matteocorti/check_ssl_cert.git#341b5813108fb2367ada81e866da989ea4fb29e7 /check_ssl_cert

WORKDIR /check_mssql_health
RUN mkdir bin && \
    autoconf && \
    autoreconf && \
    ./configure "--build=$(uname -m)-unknown-linux-gnu" --libexecdir=/usr/lib/nagios/plugins && \
    make && \
    make install DESTDIR="$(pwd)/bin"

WORKDIR /check_nwc_health
RUN mkdir bin && \
    autoreconf && \
    ./configure "--build=$(uname -m)-unknown-linux-gnu" --libexecdir=/usr/lib/nagios/plugins && \
    make && \
    make install DESTDIR="$(pwd)/bin"

WORKDIR /check_postgres
RUN mkdir bin && \
    perl Makefile.PL INSTALLSITESCRIPT=/usr/lib/nagios/plugins && \
    make && \
    make install DESTDIR="$(pwd)/bin" && \
    # This is necessary because of this build error: cannot copy to non-directory: /var/lib/docker/.../merged/usr/local/man
    rm -rf bin/usr/local/man

FROM build-base AS build-icinga2

# To access the automated build arguments in the Dockerfile originated from the Docker BuildKit [^1],
# we need to declare them here as build arguments. This is necessary because we want to use unique IDs
# for the mount cache below for each platform to avoid conflicts between multi arch builds. Otherwise,
# the build targets will invalidate the cache one another, leading to strange build errors.
#
# [^1]: https://docs.docker.com/reference/dockerfile/#automatic-platform-args-in-the-global-scope
ARG TARGETPLATFORM

# Icinga 2 build arguments.
#
# These arguments are used to configure the build of Icinga 2 and can be overridden
# by the user when building the image. All of them have a default value suitable for our official image.
ARG CMAKE_BUILD_TYPE=RelWithDebInfo
ARG ICINGA2_UNITY_BUILD=ON
ARG ICINGA2_BUILD_TESTING=ON

# The number of jobs to run in parallel when building Icinga 2.
# By default, it is set to the number of available CPU cores on the build machine.
ARG MAKE_JOBS=auto

# Create the directory where the final Icinga 2 files will be installed.
#
# This directory will be used as the destination for the `make install` command below and will be
# copied to the final image. Other than that, this directory will not be used for anything else.
RUN mkdir /icinga2-install

# Mount the source code as a bind mount instead of copying it, so that we can use the cache effectively.
# Additionally, add the ccache and CMake build directories as cache mounts to speed up rebuilds.
RUN --mount=type=bind,source=.,target=/icinga2,readonly \
    --mount=type=cache,id=ccache-${TARGETPLATFORM},target=/root/.ccache \
    --mount=type=cache,id=icinga2-build-${TARGETPLATFORM},target=/icinga2-build \
    PATH="/usr/lib/ccache:$PATH" \
    cmake -S /icinga2 -B /icinga2-build \
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
        # Podman supports forwarding notifications from containers to systemd, so build Icinga 2 with systemd support.
        -DUSE_SYSTEMD=ON \
        -DBUILD_TESTING=${ICINGA2_BUILD_TESTING} \
        -DICINGA2_UNITY_BUILD=${ICINGA2_UNITY_BUILD} \
        # The command group name below is required for the prepare-dirs script to work, as it expects
        # the command group name, which by default is `icingacmd` to exist on the system. Since we
        # don't create the `icingacmd` command group in this image, we need to override it with icinga.
        -DICINGA2_COMMAND_GROUP=icinga \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_INSTALL_SYSCONFDIR=/data/etc \
        -DCMAKE_INSTALL_LOCALSTATEDIR=/data/var \
        -DICINGA2_SYSCONFIGFILE=/etc/sysconfig/icinga2 \
        -DICINGA2_RUNDIR=/run \
        -DICINGA2_WITH_COMPAT=OFF \
        -DICINGA2_WITH_LIVESTATUS=OFF && \
    make -j$([ "$MAKE_JOBS" = auto ] && nproc || echo "$MAKE_JOBS") && \
    CTEST_OUTPUT_ON_FAILURE=1 make test && \
    make install DESTDIR=/icinga2-install

RUN rm -rf /icinga2-install/etc/icinga2/features-enabled/mainlog.conf \
        /icinga2-install/usr/share/doc/icinga2/markdown && \
    strip -g /icinga2-install/usr/lib/*/icinga2/sbin/icinga2 && \
    strip -g /icinga2-install/usr/lib/nagios/plugins/check_nscp_api

# Prepare the final image with the necessary configuration files and runtime dependencies.
FROM debian:bookworm-slim AS icinga2

# The real UID of the Icinga user to be used in the final image.
ARG ICINGA_USER_ID=5665

# Install the necessary runtime dependencies for the Icinga 2 binary and the monitoring-plugins.
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends --no-install-suggests \
        bc \
        ca-certificates \
        curl \
        dumb-init \
        file \
        libboost1.74-dev \
        libboost-context1.74-dev \
        libboost-coroutine1.74-dev \
        libboost-date-time1.74-dev \
        libboost-filesystem1.74-dev \
        libboost-iostreams1.74-dev \
        libboost-program-options1.74-dev \
        libboost-regex1.74-dev \
        libboost-system1.74-dev \
        libboost-thread1.74-dev \
        libboost-test1.74-dev \
        libcap2-bin \
        libedit2 \
        libldap-common \
        libmariadb3 \
        libmoosex-role-timer-perl \
        libpq5 \
        libssl3 \
        libsystemd0 \
        mailutils \
        monitoring-plugins \
        msmtp \
        msmtp-mta \
        openssh-client \
        openssl && \
    # Official Debian images automatically run `apt-get clean` after every install, so we don't need to do it here.
    rm -rf /var/lib/apt/lists/*

# Create the icinga user and group with a specific UID as recommended by Docker best practices.
# The user has a home directory at /var/lib/icinga2, and if configured, that directory will also
# be used to store the ".msmtprc" file created by the entrypoint script.
RUN adduser \
    --system \
    --group \
    --home /var/lib/icinga2 \
    --disabled-login \
    --no-create-home \
    --uid ${ICINGA_USER_ID} icinga

COPY --from=build-plugins /check_mssql_health/bin/ /
COPY --from=build-plugins /check_nwc_health/bin/ /
COPY --from=build-plugins /check_postgres/bin/ /
COPY --from=build-plugins /check_ssl_cert/check_ssl_cert /usr/lib/nagios/plugins/check_ssl_cert

COPY --from=build-icinga2 /icinga2-install/ /

# Create a corresponding symlink in the root filesystem for all Icinga 2 directories in /data.
# This is necessary because we want to maintain the compatibility with containers built with the
# legacy Dockerfile, which expects the Icinga 2 directories to be in the root directory.
RUN for dir in /etc/icinga2 /var/cache/icinga2 /var/lib/icinga2 /var/log/icinga2 /var/spool/icinga2; do \
    ln -vs "/data$dir" "$dir"; \
done

# The below prepare-dirs script will not fix any permissions issues for the actuall /var/lib/icinga2 or
# /etc/icinga2 directories, so we need to set the correct ownership for the /data directory recursively.
RUN chown -R icinga:icinga /data

# Run the prepare-dirs script to create non-existing directories and set the correct permissions for them.
# It's invoked in the same way as in the systemd unit file in a Debian package, so this will ensure that
# all the necessary directories are created with the correct permissions and ownership.
RUN /usr/lib/icinga2/prepare-dirs /etc/sysconfig/icinga2

# Well, since the /data directory is intended to be used as a volume, we should also declare it as such.
# This will allow users to mount their own directories or even specific files to the /data directory
# without any issues. We've already filled the /data directory with the necessary configuration files,
# so users can simply mount their own files or directories if they want to override the default ones and
# they will be able to do so without any issues.
VOLUME ["/data"]

COPY --chmod=0755 tools/container/entrypoint.sh /usr/local/bin/entrypoint.sh
ENTRYPOINT ["/usr/bin/dumb-init", "-c", "--", "/usr/local/bin/entrypoint.sh"]

EXPOSE 5665
USER icinga

CMD ["icinga2", "daemon"]
