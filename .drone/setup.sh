mkdir -p ccache
sudo chown -R "$(whoami)." ccache
ln -vs "$(pwd)/ccache" ~/.ccache
ccache -z

export ICINGA_BUILD_PROJECT=icinga2
export ICINGA_BUILD_TYPE=snapshot
export UPSTREAM_GIT_URL="$DRONE_REMOTE_URL"
export RPM_BUILD_NCPUS=1
export DEB_BUILD_OPTIONS="parallel=$RPM_BUILD_NCPUS"

if [ -n "$DRONE_TAG" ]; then
	export ICINGA_BUILD_UPSTREAM_BRANCH="$DRONE_TAG"
fi

if [ -n "$DRONE_BRANCH" ]; then
	export ICINGA_BUILD_UPSTREAM_BRANCH="$DRONE_BRANCH"
fi
