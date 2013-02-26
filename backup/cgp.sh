#!/bin/sh
echo "####################################  BEGIN ###################################" > /root/cgpbackup.log
date >> /root/cgpbackup.log
echo "1. Mount share directory for backup." >> /root/cgpbackup.log
var1=`cat /etc/mtab | grep "/mnt"`
if [ -n "$var1" ]
 then
  date >> /root/cgpbackup.log
  echo " /mnt already mounted as:" >> /root/cgpbackup.log
  echo " "  $var1 >> /root/cgpbackup.log
  while [ -n "$var1" ]; do
   umount -flv /mnt >> /root/cgpbackup.log 2>&1
   var1=`cat /etc/mtab | grep "/mnt"`
  done
fi
#rm -rRvf /mnt/* >> /root/cgpbackup.log 2>&1
mount -t cifs //192.168.10.185/backup /mnt --verbose  -o username=backup@domain.local,password=***,rw >>/root/cgpbackup.log 2>&1
echo "2. Backup." >> /root/cgpbackup.log
date >> /root/cgpbackup.log
if [ -d /mnt/cgp/backup ]
 then
  rsync -avpEg --delete-after /var/CommuniGate/ /mnt/cgp/backup >/root/cgprsync.log 2>&1
  echo "I RSYNC !!!!" >>/root/cgpbackup.log
  date >> /root/cgpbackup.log
  umount -flv /mnt >>/root/cgpbackup.log 2>&1
#  rm -rRvf /mnt/* >>/root/cgpbackup.log 2>&1
  echo " Backup Complete."  >> /root/cgpbackup.log
  date >> /root/cgpbackup.log
 else
  echo " Backup Failed. Folder '/mnt/backup' not exist." >> /root/cgpbackup.log
  date >> /root/cgpbackup.log
fi
echo "###################################  END  #####################################" >> /root/cgpbackup.log
