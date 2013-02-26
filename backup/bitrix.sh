#!/bin/sh
echo " "
echo " "
echo "################################  BEGIN  ###################################"
date
echo "1. Create bitrix database dump."
rm -rvRf /home/bitrix/www/bitrix/backup/*.gz 
rm -rvRf /home/bitrix/www/bitrix/backup/*.sql
filedump=`eval date +%Y.%m.%d`".sql"
filebackup=`eval date +%Y.%m.%d`".tar.gz"
backupfolder=backup
#backupfolder=`eval date +%d.%m.%Y`
for list in `mysql -e "use bitrix; show tables;" | tail -n +2` ; do
    mysqldump  bitrix $list | sed '/^\/\*/d' >> /home/bitrix/www/bitrix/backup/$filedump
done
echo " DUMP COMPLETE." 
echo "2. Mount share directory for backup."
var1=`cat /etc/mtab | grep "/mnt"`
if [ -n "$var1" ]
 then
  echo "  /mnt already mounted as:"
  echo $var1
  while [ -n "$var1" ]; do
   umount -fvl /mnt
   var1=`cat /etc/mtab | grep "/mnt"`
  done
fi
rm -rvRf /mnt/*
mount -t cifs //192.168.1.1/backup /mnt --verbose -o username=backup@domain.local,password=***,rw
echo " folder contains:"
ls -la /mnt/www
if [ -d /mnt/www ]
 then
  echo "2. Backup site."
  cd /home/bitrix/www
    if [ -d /mnt/www/backup ]
	then
	    echo "Folder for backup exist."
	else
	    echo "Folder for backup not exist, create"
	    mkdir /mnt/www/$backupfolder
    fi
  tar -cvpzf /mnt/www/$backupfolder/$filebackup * .??*
  echo " folder contains:"
  ls /mnt/www
  umount -fvl /mnt
  rm -rvRf /mnt/*
  echo " Backup complete."
 else
  echo "2. Smbmount ERROR."
  umount -fvl /mnt
  rm -rvRf /mnt/*
fi
echo "#################################  END  ####################################"
