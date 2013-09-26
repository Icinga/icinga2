#!/bin/sh
cd -- `dirname $0`
cat <<HTML
<!DOCTYPE html>
<html>
<xmp theme="united" style="display:none;">
HTML

for file in *.md; do
	cat $file
	echo
done

cat <<HTML
</xmp>

<script src="http://strapdownjs.com/v/0.2/strapdown.js"></script>
</html>
HTML
