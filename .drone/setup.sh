export ICINGA_BUILD_PROJECT=icinga2
export ICINGA_BUILD_TYPE=snapshot
export UPSTREAM_GIT_URL=file:///drone/src/.git
export ICINGA_BUILD_UPSTREAM_BRANCH="drone-build/$DRONE_BUILD_NUMBER"
export RPM_BUILD_NCPUS=1
export DEB_BUILD_OPTIONS="parallel=$RPM_BUILD_NCPUS"

sudo chown -R "$(whoami)." .
git fetch
git checkout -B "$ICINGA_BUILD_UPSTREAM_BRANCH"
