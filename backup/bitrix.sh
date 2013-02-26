#!/bin/sh
echo " " > /root/wwwbackup.log
echo " " >> /root/wwwbackup.log
echo "################################  BEGIN  ###################################" >> /root/wwwbackup.log
date >> /root/wwwbackup.log
echo "1. Create bitrix database dump." >> /root/wwwbackup.log
rm -rvRf /home/bitrix/www/bitrix/backup/*.gz >> /root/wwwbackup.log 2>&1
rm -rvRf /home/bitrix/www/bitrix/backup/*.sql >> /root/wwwbackup.log 2>&1
filedump=`eval date +%Y.%m.%d`".sql"
filebackup=`eval date +%Y.%m.%d`".tar.gz"
backupfolder=backup
#backupfolder=`eval date +%d.%m.%Y`
for list in `mysql -e "use bitrix; show tables;" | sed '/^Tables_in/d'` ; do
    mysqldump  bitrix $list | sed '/^\/\*/d' >> /home/bitrix/www/bitrix/backup/$filedump
done
echo " DUMP COMPLETE." >> /root/wwwbackup.log
echo "2. Mount share directory for backup." >> /root/wwwbackup.log
var1=`cat /etc/mtab | grep "/mnt"`
if [ -n "$var1" ]
 then
  echo "  /mnt already mounted as:" >> /root/wwwbackup.log
  echo $var1 >> /root/wwwbackup.log
  while [ -n "$var1" ]; do
   umount -fvl /mnt >> /root/wwwbackup.log 2>&1
   var1=`cat /etc/mtab | grep "/mnt"`
  done
fi
rm -rvRf /mnt/* >> /root/wwwbackup.log 2>&1
mount -t cifs //192.168.1.1/backup /mnt --verbose -o username=backup@domain.local,password=***,rw >> /root/wwwbackup.log 2>&1
echo " folder contains:" >> /root/wwwbackup.log
ls -la /mnt/www >> /root/wwwbackup.log 2>&1
if [ -d /mnt/www ]
 then
  echo "2. Backup site." >> /root/wwwbackup.log
  cd /home/bitrix/www
    if [ -d /mnt/www/backup ]
	then
	    echo "Folder for backup exist." >> /root/wwwbackup.log
	else
	    echo "Folder for backup not exist, create" >> /root/wwwbackup.log
	    mkdir /mnt/www/$backupfolder >> /root/wwwbackup.log 2>$1
    fi
  tar -cvpzf /mnt/www/$backupfolder/$filebackup * .??* >> /root/wwwbackup.log 2>&1
  echo " folder contains:" >> /root/wwwbackup.log 2>&1
  ls /mnt/www >> /root/wwwbackup.log
  umount -fvl /mnt >>/root/wwwbackup.log 2>&1
  rm -rvRf /mnt/* >>/root/wwwbackup.log 2>&1
  echo " Backup complete." >> /root/wwwbackup.log
 else
  echo "2. Smbmount ERROR." >>/root/wwwbackup.log
  umount -fvl /mnt >>/root/wwwbackup.log 2>&1
  rm -rvRf /mnt/* >>/root/wwwbackup.log 2>&1
fi
echo "#################################  END  ####################################" >> /root/wwwbackup.log
