# SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

ARG DEBIAN_VERSION="trixie"
ARG PLUGINS_VERSION=${DEBIAN_VERSION}

# Alias the container to plugins as the ARG substitution is not allowed in the RUN bindmount on Docker.
FROM ghcr.io/icinga/icinga2-plugins-container:${PLUGINS_VERSION} AS plugins

FROM debian:${DEBIAN_VERSION}-slim AS build-base

# Install all the necessary build dependencies for building Icinga 2 and the plugins.
#
# This stage includes the build dependencies for the plugins as well, so that they can share the same base
# image, since Docker builds common stages only once [^1] even if they are used in multiple build stages.
# This eliminates the need to have a separate base image for the plugins, that basically has kind of the
# same dependencies as the Icinga 2 build stage (ok, not exactly the same, but some of them are shared).
#
# [^1]: https://docs.docker.com/build/building/best-practices/#create-reusable-stages
RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y --no-install-recommends --no-install-suggests \
        autoconf \
        automake \
        bison \
        ccache \
        cmake \
        flex \
        g++ \
        git \
        libboost1.83-dev \
        libboost-context1.83-dev \
        libboost-coroutine1.83-dev \
        libboost-date-time1.83-dev \
        libboost-filesystem1.83-dev \
        libboost-iostreams1.83-dev \
        libboost-program-options1.83-dev \
        libboost-regex1.83-dev \
        libboost-system1.83-dev \
        libboost-thread1.83-dev \
        libboost-test1.83-dev \
        libedit-dev \
        libmariadb-dev \
        libpq-dev \
        libssl-dev \
        libsystemd-dev \
        libprotobuf-dev \
        protobuf-compiler \
        make && \
    rm -rf /var/lib/apt/lists/*

# Set the default working directory for subsequent commands of the next stages.
WORKDIR /icinga2-build

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
    cmake -S /icinga2 -B /icinga2-build \
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
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
    if [ "${ICINGA2_BUILD_TESTING}" = ON ]; then CTEST_OUTPUT_ON_FAILURE=1 make test; fi && \
    make install DESTDIR=/icinga2-install

RUN rm -rf /icinga2-install/etc/icinga2/features-enabled/mainlog.conf \
        /icinga2-install/usr/share/doc/icinga2/markdown && \
    strip -g /icinga2-install/usr/lib/*/icinga2/sbin/icinga2 && \
    strip -g /icinga2-install/usr/lib/nagios/plugins/check_nscp_api

# Prepare the final image with the necessary configuration files and runtime dependencies.
FROM debian:${DEBIAN_VERSION}-slim AS icinga2

# The real UID of the Icinga user to be used in the final image.
ARG ICINGA_USER_ID=5665

COPY --from=plugins /usr/lib/nagios/plugins/* /usr/lib/nagios/plugins/
COPY --from=plugins /plugins.cdx.json /usr/share/sbom/

# Install the necessary runtime dependencies for the Icinga 2 binary and the monitoring-plugins.
RUN --mount=from=plugins,target=/plugins \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get upgrade -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends --no-install-suggests \
        bc \
        ca-certificates \
        curl \
        dumb-init \
        file \
        libboost-context1.83.0 \
        libboost-coroutine1.83.0 \
        libboost-date-time1.83.0 \
        libboost-filesystem1.83.0 \
        libboost-iostreams1.83.0 \
        libboost-program-options1.83.0 \
        libboost-regex1.83.0 \
        libboost-system1.83.0 \
        libboost-thread1.83.0 \
        libcap2-bin \
        libedit2 \
        libldap-common \
        libmariadb3 \
        libmoosex-role-timer-perl \
        libpq5 \
        libprotobuf-lite32t64 \
        libssl3 \
        libsystemd0 \
        mailutils \
        monitoring-plugins \
        msmtp \
        msmtp-mta \
        openssh-client \
        openssl \
        $(cat /plugins/debian-packages) && \
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
