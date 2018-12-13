#!/bin/sh

# finish header
echo "Content-Type: text/html"
echo

# then produce content
echo "<html><head>"
echo "<title>a script</title>"
echo "</head><body>"
echo "<p>[<a href="/">home</a>]</p>"
echo "<h2>This page is a script</h2>"
echo "<hr>"
echo "<p>date&nbsp;:"
date
echo "<hr>"
echo "<p>process list &nbsp;:"
echo "<pre>"
ps axf | sed -e 's/</\&lt;/g'
echo "</pre>"
echo "<hr>"
echo "</body></html>"
