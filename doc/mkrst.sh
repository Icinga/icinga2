#!/bin/sh
cd $(dirname -- $0)

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
