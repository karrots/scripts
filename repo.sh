#!/bin/bash
/bin/mount /usr/share/repo

#/bin/ls -1 /usr/share/repo/6.4/x86_64/rdk/| /bin/awk -F'-' '{for (i=NF;i>=NF-2 AND i>0 ;i=i-1)gsub($i,""); print}'|/bin/sed 's/..$//g'| uniq > /root/repofiles/packages.txt

newfolder=`eval date +"%d.%m.%Y"`
/bin/rm -vfr /usr/share/repo/6.4/x86_64/`/bin/ls -t /usr/share/repo/6.4/x86_64/|/usr/bin/tail -1`
/bin/mkdir -v /usr/share/repo/6.4/x86_64/$newfolder
/usr/bin/yum clean all

/usr/bin/yum list all |/bin/grep -v '^ '|/usr/bin/tail -n +5|/bin/awk '{print $1}' > /root/repofiles/packages.txt

for package in `cat /root/repofiles/packages.txt`; do
    /usr/bin/yumdownloader $package --destdir /usr/share/repo/6.4/x86_64/$newfolder --resolve
done

/bin/rm -fr /root/repo/*
/usr/bin/createrepo /usr/share/repo/6.4/x86_64/$newfolder -o /root/repo/ -g /root/repofiles/group.xml
/bin/rm -f /usr/share/repo/6.4/x86_64/rdk
cd /usr/share/repo/6.4/x86_64/; /bin/ln -s $newfolder rdk
/bin/cp -vfr /root/repo/repodata /usr/share/repo/6.4/x86_64/rdk/repodata

exit 0
