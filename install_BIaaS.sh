sudo mkdir /var/www/c
sudo mkdir /var/www/c/f
sudo mkdir /var/www/c/r
makeplist.sh
gcc -o plistStarter plistStarter.c
sudo cp plist /var/www/c/
sudo cp plistStarter /var/www/c/
sudo cp plistClean.sh /var/www/c/
sudo cp plistStarter.sh /var/www/c/
sudo cp watchPlist.sh /var/www/c/
makemain.sh
cd /var/www/c/
./plistClean.sh
./watchPlist.sh
