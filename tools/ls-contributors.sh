#!/bin/bash
# Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+

set -eo pipefail

GIT_REMOTE="$1"
COMMIT_RANGE="$2"
MILESTONE="$3"

if [ "$GIT_REMOTE" = '' ] || [ "$COMMIT_RANGE" = '' ] || [ "$MILESTONE" = '' ]; then
	cat <<EOF >&2
Usage: ${0} GIT_REMOTE COMMIT_RANGE MILESTONE
EOF

	exit 1
fi

PULL_REFS="$(git branch -a |\
	cut -c 3- |\
	grep -Fe "remotes/${GIT_REMOTE}/pull/" |\
	wc -l)"

if [ "$PULL_REFS" -eq 0 ]; then
	cat <<EOF >&2
You don't have mirrored any pull requests locally. Run:

git config --add remote.${GIT_REMOTE}.fetch '+refs/pull/*/head:refs/remotes/origin/pull/*'
git fetch ${GIT_REMOTE}
EOF

	exit 1
else
	cat <<EOF >&2
Make sure to have done: git fetch ${GIT_REMOTE}

EOF
fi

PULLS="$(git log --format=%H "$COMMIT_RANGE" |\
	xargs -n 1 git describe --all --contains |\
	grep -Fe "remotes/${GIT_REMOTE}/pull/" |\
	grep -vFe '~' |\
	cut -d / -f 4 |\
	sort -nu)"

USERS="$((for PULL in $PULLS; do
		curl -fsSLH 'Accept: application/vnd.github.v3+json' "https://api.github.com/repos/Icinga/icinga2/pulls/$PULL" |\
			python -c 'import json, sys; print(json.load(sys.stdin)["user"]["login"])'
	done) |\
		sort -u)"

for USR in $USERS; do
	cat <<EOF
[${USR}](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3A${USR}+milestone%3A${MILESTONE}),
EOF
done
