sudo mkdir /var/www/c
sudo mkdir /var/www/c/f
sudo mkdir /var/www/c/r
./makeplist.sh
gcc -o plistStarter plistStarter.c
sudo cp plist /var/www/c/
sudo cp plistStarter /var/www/c/
sudo cp plistClean.sh /var/www/c/
sudo cp plistStarter.sh /var/www/c/
sudo cp watchplist.sh /var/www/c/
./makemainh.sh
cd /var/www/c/
./plistClean.sh
./watchplist.sh
sudo service apache2 restart
