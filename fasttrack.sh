rm c:\kafka\sensors\*.part
rm c:\kafka\sensors\sensors.out
python c:\kafka\sensors\fasttrack4-load.py $@ -f c:\kafka\sensors\iotdata\sensors.out -o c:\kafka\sensors\iotdata\sensors
