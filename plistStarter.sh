./plistStarter
if [ $? -eq 255 ]
then
 sudo ./plist </dev/null &>/dev/null &
else
 echo "plist service already up"
fi
