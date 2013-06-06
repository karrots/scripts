#!/usr/bin/perl -w
use strict;
use warnings;
use SNMP;
use Socket;
use lib "/usr/lib64/nagios/plugins";
use utils qw(%ERRORS);

&SNMP::addMibDirs('/usr/share/mibs/netsnmp/');
&SNMP::loadModules("ALL");
&SNMP::initMib();
# VARIABLES
my $comm = 'sdco_it';
my $sver = '2c';
my $oid = 'sysName';
my $ins = '';
my $crit = '0';
my $dest = '0';
my $sess;
my $errnum = 0;
my %snmpparms;
my $arg;
my $vb;
my $var;

while ( $arg = shift @ARGV ) {
        if ( $arg eq '-H') {
                $dest = shift @ARGV;
        }
        elsif ( $arg eq '-c' ) {
                $comm = shift @ARGV;
        }
        elsif ( $arg eq '-o' ) {
                $oid = shift @ARGV;
        }
        elsif ( $arg eq '-i' ) {
                $ins = shift @ARGV;
        }
                elsif ( $arg eq '-crit' ) {
                $crit = shift @ARGV;
        }
        else {
        print STDOUT "ERROR. BAD ARGUMENTS.\n";
        exit $ERRORS{'UNKNOWN'};
        }
}

# MAIN
$snmpparms{Community} = $comm;
$snmpparms{Version} = $sver;
$snmpparms{UseSprintValue} = '1';
$snmpparms{DestHost} = inet_ntoa(inet_aton($dest));
$sess = SNMP::Session->new(%snmpparms);

$vb = SNMP::Varbind->new([$oid,$ins]);
$var = $sess->get($vb);

if ($sess->{ErrorNum}) {
        print "ERROR \"$sess->{ErrorStr}\" querying $dest for $oid.$ins.\n";
        exit $ERRORS{'CRITICAL'};
}
else {
        if ($var eq $crit) {
                        print "$var \n";
                        exit $ERRORS{'CRITICAL'};
                }
                else {
                        print "$var \n";
                        exit $ERRORS{'OK'};
                }
}

