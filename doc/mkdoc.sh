#!/bin/sh
cd -- `dirname $0`

while read badword; do
	if grep -i "$badword" *.md; then
		echo "Documentation contains banned word."
		exit 1
	fi
done < BLACKLIST

cat <<HTML
<!DOCTYPE html>
<html>
<head>
<link rel="stylesheet" type="text/css" href="toc.css" media="all" />
<link rel="stylesheet" type="text/css" href="toc-print.css" media="print" />
<xmp theme="cerulean" style="display:none;">
HTML

for file in *.md; do
	cat $file
	echo; echo
done

cat <<HTML
</xmp>

<script src="strapdownjs/strapdown.js"></script>
<script src="jquery.min.js"></script>
<script src="jquery.toc.min.js"></script>
<script>
\$('div#content').prepend('<div id="toc"></div>');
\$('#toc').toc({
    'selectors': 'h1,h2,h3,h4,h5,h6'
});
</script>
<link rel="stylesheet" type="text/css" href="toc-margin.css" media="screen" />
</head>
<body>
<div id="content" />
<script>
var indices = [];

function addIndex() {
  // jQuery will give all the HNs in document order
  jQuery('h1,h2,h3,h4,h5,h6').each(function(i,e) {
      var hIndex = parseInt(this.nodeName.substring(1)) - 1;

      // just found a levelUp event
      if (indices.length - 1 > hIndex) {
        indices= indices.slice(0, hIndex + 1 );
      }

      // just found a levelDown event
      if (indices[hIndex] == undefined) {
         indices[hIndex] = 0;
      }

      // count + 1 at current level
      indices[hIndex]++;

      // display the full position in the hierarchy
      jQuery(this).prepend(indices.join(".") + " ");

  });
}

jQuery(document).ready(function() {
  addIndex();
});
</script>
</body>
</html>
HTML
