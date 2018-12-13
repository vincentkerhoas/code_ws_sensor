# finish header
echo "Content-Type: text/html"
echo

# then produce content
echo "<html><head>"
echo "<title>Un script</title>"
echo "</head><body>"
echo "<h3>Dynamic Part : </h3>"

echo "<p>QUERYSTRING = ["
echo $QUERY_STRING
echo "]"

echo "<br/>"

result=$(echo $QUERY_STRING | cut -d'=' -f2)

if [ "$result" = "false" ]
then
echo 0 > /sys/class/gpio/gpio4/value
echo "gpio4 led turned off at"
date
else
echo 1 > /sys/class/gpio/gpio4/value
echo "gpio4 led turned on at"
date
fi

echo "</body></html>"
