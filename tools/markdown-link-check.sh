#!/bin/bash

set -e

PATH="$(pwd)/node_modules/.bin:${PATH}"

errors=0

while read -r file; do
  if ! markdown-link-check --quiet "$file"; then
    (( errors++ )) || true
  fi
done < <(find -name \*.md)

if [[ $errors -gt 0 ]]; then
  echo "Found ${errors} files with errors!"
  exit 1
fi
