#!/bin/bash

# for 512 MB
#sudo mkdir /media/blueDBM/1
#sudo bonnie++ -s 80:1024 -n 20 -x 1 -r 8 -b -z 1 -u root -d /media/blueDBM/1 &
#sudo mkdir /media/blueDBM/2
#sudo bonnie++ -s 80:1024 -n 20 -x 1 -r 8 -b -z 1 -u root -d /media/blueDBM/2 &
#sudo mkdir /media/blueDBM/3
#sudo bonnie++ -s 80:1024 -n 20 -x 1 -r 8 -b -z 1 -u root -d /media/blueDBM/3 &

# for 2 GB
sudo mkdir /media/blueDBM/1
sudo bonnie++ -s 320:1024 -n 40 -x 1 -r 8 -z 1 -u root -d /media/blueDBM/1 &
sudo mkdir /media/blueDBM/2
sudo bonnie++ -s 320:1024 -n 40 -x 1 -r 8 -z 1 -u root -d /media/blueDBM/2 &
sudo mkdir /media/blueDBM/3
sudo bonnie++ -s 320:1024 -n 40 -x 1 -r 8 -z 1 -u root -d /media/blueDBM/3 &

# for 2 GB
#sudo mkdir /media/blueDBM/1
#sudo bonnie++ -s 450 -n 40 -x 1 -r 4 -z 1 -u root -d /media/blueDBM/1 &
#sudo mkdir /media/blueDBM/2
#sudo bonnie++ -s 450 -n 40 -x 1 -r 4 -z 1 -u root -d /media/blueDBM/2 &
#sudo mkdir /media/blueDBM/3
#sudo bonnie++ -s 450 -n 40 -x 1 -r 4 -z 1 -u root -d /media/blueDBM/3 &


# for 2 GB
#sudo mkdir /media/blueDBM/1
#sudo bonnie++ -s 240:1024 -n 40 -x 1 -r 8 -z 1 -u root -d /media/blueDBM/1 &
#sudo mkdir /media/blueDBM/2
#sudo bonnie++ -s 240:1024 -n 40 -x 1 -r 8 -z 1 -u root -d /media/blueDBM/2 &
#sudo mkdir /media/blueDBM/3
#sudo bonnie++ -s 240:1024 -n 40 -x 1 -r 8 -z 1 -u root -d /media/blueDBM/3 &
#sudo mkdir /media/blueDBM/4
#sudo bonnie++ -s 240:1024 -n 40 -x 1 -r 8 -z 1 -u root -d /media/blueDBM/4 &
#sudo mkdir /media/blueDBM/5
#sudo bonnie++ -s 160:1024 -n 40 -x 2 -r 8 -b -z 1 -u root -d /media/blueDBM/5 &
#sudo mkdir /media/blueDBM/6
#sudo bonnie++ -s 160:1024 -n 40 -x 2 -r 8 -b -z 1 -u root -d /media/blueDBM/6 &

for job in `jobs -p`
do
	echo $job
   	wait $job
done
