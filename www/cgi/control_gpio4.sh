# finish header
echo "Content-Type: text/html"
echo

# then produce content
echo "<html><head>"
echo "<title>Un script</title>"
echo "</head><body>"
echo "<h1>Dynamic Part : </h1>"

echo "<hr>"

echo "<p>QUERYSTRING = ["
echo $QUERY_STRING
echo "]"

echo "<br/>"

result=$(echo $QUERY_STRING | cut -d'=' -f2)
result=$(echo $result | cut -d'&' -f1)
#echo $result

if [ "$result" = "off" ]
then
	echo 0 > /sys/class/gpio/gpio4/value
	echo "gpio4 led turned off at"
	date
else
	echo 1 > /sys/class/gpio/gpio4/value
	echo "gpio4 led turned on at"
	date
fi

echo "<br/>"
echo "<a href="/">home</a>" 

echo "<hr>"
echo "</body></html>"

