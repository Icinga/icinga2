#!/bin/sh
cd $(dirname -- $0)

if [ -z "$1" ]; then
  echo "Syntax: $0 <build-dir>"
  exit 1
fi

BUILDDIR="$1"

mkdir -p $BUILDDIR/htdocs

if ! which pandoc; then
  echo "Please install pandoc to build the documentation files." > $BUILDDIR/htdocs/index.html
  exit 0
fi

if ! which sphinx-build; then
  echo "Please install sphinx-build to build the documentation files." > $BUILDDIR/htdocs/index.html
  exit 0
fi

BUILDDIR="$1"

echo "Build dir: $BUILDDIR"

rm -f index.rst
cat > index.rst <<RST
Icinga 2
========

.. toctree::
RST

for chapter in $(seq 1 100); do
  files=$chapter*.md
  count=0
  for file in $files; do
    if [ -f $file ]; then
      count=$(expr $count + 1)
    fi
  done
  if [ $count = 0 ]; then
    break
  fi
  echo "    chapter-$chapter" >> index.rst
  for file in $files; do
    cat $file
    echo
  done | sed 's/<a id=".*"><\/a>//' | pandoc -f markdown_phpextra -t rst > chapter-$chapter.rst
done

sphinx-build -b html -d $BUILDDIR/doctrees . $BUILDDIR/htdocs
