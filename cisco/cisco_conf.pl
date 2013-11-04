#!/usr/bin/perl -w
use warnings;
use strict;
use SNMP;
use Socket;
&SNMP::addMibDirs('/usr/share/snmp/mibs/');
&SNMP::loadModules("ALL");
&SNMP::initMib();
# VARIABLES
my @devices = ( '192.168.1.1',
		'192.168.1.2',
                '192.168.1.3',
                '192.168.1.4',
                '192.168.1.5');

my $comm = 'community';
my $sver = '2c';
my $instance = int(rand(898)) + 101;
my $dest;
my $sess;
my $errnum = 0;
my %snmpparms;
$snmpparms{Community} = $comm;
$snmpparms{Version} = $sver;
$snmpparms{UseSprintValue} = '1';
# MAIN
foreach $dest (@devices) {
        $snmpparms{DestHost} = inet_ntoa(inet_aton($dest));
        $sess = SNMP::Session->new(%snmpparms);

        my $hostname;
        $hostname = new SNMP::Varbind(['sysName','0']);
        $hostname = $sess->get($hostname);

        if ($sess->{ErrorNum}) {
                open (LOG, ">> /root/logs/cisco_config.log");
                print LOG "Got $sess->{ErrorStr} querying $dest.\n";
                close LOG;
                $errnum = 1;
        }
        else {
                COPY_CONF ('ccCopyEntryRowStatus', 5);
                COPY_CONF ('ccCopyProtocol', 1);
                COPY_CONF ('ccCopySourceFileType', 4);
                COPY_CONF ('ccCopyDestFileType', 1);
                COPY_CONF ('ccCopyServerAddress', '192.168.1.200');
                COPY_CONF ('ccCopyFileName', "conf/00.CURRENT/$hostname.txt");
                COPY_CONF ('ccCopyEntryRowStatus', 1);
                sleep (10);
                COPY_CONF ('ccCopyEntryRowStatus', 6);
        }
}

sub COPY_CONF {
        my($vb,$var);
        $vb = SNMP::Varbind->new([$_[0], $instance, $_[1]]);
        $var = $sess->set($vb);
}

exit $errnum;
