#!/usr/bin/env bash

# This script is used to build a Docker image for Icinga 2.

# Action defines the type of build to perform. It can be one of:
#
# - load: Build the image for the building platform and load it into the local Docker engine image storage.
# - push: Build the image for all specified platforms and push them to the Docker registry (icinga/icinga2:<TAG>).
# - cache: Build the image for all specified platforms using the default Buildx's exporter (cacheonly) and will just
#   create a cache for each platform. There will be no images that you can use but any subsequent build will use the
#   cache and speed up the build process.
#
# If no action is provided, it defaults to "load", and any specified platforms will be ignored.
ACTION="${1:-load}"
# Tag defines the tag to use for the built image. If not provided, it defaults to "test".
# The resulting image will be tagged as "icinga/icinga2:<TAG>".
TAG="${2:-test}"
# Platform defines the platforms to build the image for. It defaults to "linux/amd64,linux/arm64/v8".
PLATFORM="${3:-linux/amd64,linux/arm64/v8}"
# ROOT_DIR is the root directory of the Icinga 2 source code repository and where the Containerfile is located.
# It's also used to determine the build context for the Docker build, so that every file referenced in the
# Containerfile can be found relative to this directory by Docker.
ROOT_DIR="$(dirname "$(realpath "$0")")/../.."

if ! command -v docker &> /dev/null; then
    echo "Docker is not installed. Please install Docker to build the image." >&2
    exit 1
fi

if ! command -v docker buildx &> /dev/null; then
    echo "Docker Buildx is not installed. Please install Docker Buildx to build the image." >&2
    exit 1
fi

# Set up the buildx command with the specified platform.
BuildX=(docker buildx build --platform "${PLATFORM}")
BUILD_ARGS=(-t "icinga/icinga2:${TAG}" -f "${ROOT_DIR}"/Containerfile "${ROOT_DIR}")

# Check the action and perform the appropriate build command.
case "$ACTION" in
    push)
        "${BuildX[@]}" --push "${BUILD_ARGS[@]}"
        ;;
    load)
        docker buildx build --load "${BUILD_ARGS[@]}"
        ;;
    cache)
        "${BuildX[@]}" "${BUILD_ARGS[@]}"
        ;;
    *)
        echo "Unknown action: ${ACTION}. Valid actions are: push, load, cache." >&2
        exit 1
esac
