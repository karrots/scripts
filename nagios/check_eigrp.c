#include <stdio.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <unistd.h>
#include <signal.h>

const char *VERSION = "0.85";

/*Nagios plugin exit status:*/
enum EXITCODE { 
	OK,
	WARNING,
	CRITICAL,
	UNKNOWN
} exitcode;

/*Structure for command-line arguments*/
struct globalArgs_t {
	const char 	*HOSTNAME;	/*Hostname of monitoring router;*/
	char		*COMMUNITY;	/*SNMP Community;*/
	const char 	*NEIGHBORS;	/*Neighbors count;*/
	const char 	*AS;		/*AS number of monitoring router;*/
	int		noList;		/*Get or not list of neighbors (disabled by default).*/
	int		timeOut;	/*Set timeout for plugin, default is 3 seconds.*/
} globalArgs;

const char *optString = "H:c:p:a:t:lhv";

/*Usage function, for printing help*/
void usage(char* name)
{
	printf("Usage: %s -H <hostipaddress> -c <community> -p <neighbors> -a <EIGRP AS number>\n", name);
	printf("Options:\n\t-h, \tShow this help message;\n");
	printf("\t-v, \tPrint the version of plugin;\n");
	printf("\t-H, \tSpecify the hostname of router;\n");
	printf("\t-c, \tSpecify the SNMP community of router;\n");
	printf("\t-a, \tSpecify the EIGRP AS number of router;\n");
	printf("\t-p, \tSpecify the neighbors count of router;\n");
	printf("\t-t, \tSpecify the timeout of plugin, default is 3 sec, max 60 sec;\n");
	printf("\t-l, \tSpecify this key if you need to get a \n\t\tlist of neighbors (disabled by default).\n\n");
	exit(UNKNOWN);
}
/*Print the version of plugin*/
void version()
{
	printf("check_eigrp (Nagios Plugin) %s\nCopyright (C) 2014 Tiunov Igor\n", VERSION);
	printf("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
	printf("Written by Tiunov Igor <igortiunov@gmail.com>\n");
	exit(OK);
}
/*This function open SNMP session to router*/
void* snmpopen( char* community, const char* hostname, int timeout){
	
	struct snmp_session session, *session_p;
		
	init_snmp("check_eigrp");
		
	snmp_sess_init( &session );
	
	session.peername = strdup(hostname);
	session.version = SNMP_VERSION_2c;
	session.community = (u_char*) community;
	session.community_len = strlen(community);
/*
	One attempt plus session.retries, therefore
	GLOBAL timeout = session.timeout*(1+session.retries)
	Change this values if you need other timeout options:
*/
	session.retries = 2;
	session.timeout = timeout*1000000/(session.retries+1);

	session_p = snmp_open(&session);

	if (session_p){
		return session_p;
	} else {
		/*Send stderr to stdout for nagios error handling*/
		dup2(1, 2);
		snmp_perror("UNKNOWN");
		snmp_log(LOG_ERR, "Some error occured in SNMP session establishment.\n");
		exit(UNKNOWN);
	}
}
/*Here we get some information from device*/
void snmpget (void *snmpsession, char *oidvalue, char *buffer, size_t buffersize){

	netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, 1);

	oid anOID[MAX_OID_LEN];
	size_t anOID_len = MAX_OID_LEN;
	struct variable_list *vars;
/*
	PDU is a snmp protocol data unit.
*/
	struct snmp_pdu *pdu;
	struct snmp_pdu *response;	

	/*If SNMP session is started, create pdu for OID and get the value		*/
	pdu = snmp_pdu_create(SNMP_MSG_GET);

	/*OK, get the SNMP vlaue from router*/
	read_objid(oidvalue, anOID, &anOID_len);
	snmp_add_null_var(pdu, anOID, anOID_len);

	if (snmp_synch_response(snmpsession, pdu, &response) == STAT_SUCCESS) {
		if (response->errstat == SNMP_ERR_NOERROR) {
			vars = response->variables;
			if (snprint_value(buffer, buffersize, vars->name, vars->name_length, vars) == -1) {
				printf("UNKNOWN: May be this router has not EIGRP protocol?\n");
				snmp_free_pdu(response);
				snmp_close(snmpsession);
				exit(UNKNOWN);
			}
			snmp_free_pdu(response);
		} else {
			printf("UNKNOWN: Error in packet\nReason: %s\n", snmp_errstring(response->errstat));
			snmp_free_pdu(response);
			snmp_close(snmpsession);
			exit(UNKNOWN);
		}
	} else {
		/*Send stderr to stdout for nagios error handling*/
		dup2(1, 2);
		snmp_sess_perror("UNKNOWN", snmpsession);
		snmp_close(snmpsession);
		exit(UNKNOWN);
	}
}
/*
	This finction hangle alarm signal (SIGALRM)
	if some problem occurs in UNIX socket, etc.
*/
void alarmHandler()
{
	snmp_close_sessions();
	printf("UNKNOWN: Plugin timeout exceeded for %d seconds.\n", globalArgs.timeOut);
	exit(UNKNOWN);
}

int main(int argc, char *argv[])
{
/*Default value of command-line arguments*/
	globalArgs.HOSTNAME=NULL;
	globalArgs.COMMUNITY=NULL;
	globalArgs.NEIGHBORS=NULL;
	globalArgs.AS=NULL;
	globalArgs.timeOut=3;
	globalArgs.noList=0;

/*Command-line arguments parsing*/
	int opt;
	while((opt = getopt(argc, argv, optString)) != -1){
		switch( opt ) {
			case 'H':
				globalArgs.HOSTNAME = optarg;
				break;
			case 'c':
				globalArgs.COMMUNITY = optarg;
				break;
			case 'p':
				globalArgs.NEIGHBORS = optarg;
				break;
			case 'a':
				globalArgs.AS = optarg;
				break;
			case 't':
				if ((globalArgs.timeOut = atoi(optarg)) <= 0)
					globalArgs.timeOut = 3;
				else if (globalArgs.timeOut > 60)
					globalArgs.timeOut = 60;
				break;
			case 'l':
				globalArgs.noList = 1;
				break;
			case 'v':
				version();
				break;
			case 'h':
				usage(argv[0]);
				break;
			default:
				usage(argv[0]);
				break;
		}
	}

	if (globalArgs.HOSTNAME==NULL || globalArgs.COMMUNITY==NULL ||  globalArgs.NEIGHBORS==NULL || globalArgs.AS==NULL){
		usage(argv[0]);
	} else {
/*Set the alarm timer if some problem occurs in UNIX socket, etc.*/
		struct sigaction alarmAct;
		memset(&alarmAct, 0, sizeof(alarmAct));
		alarmAct.sa_handler = alarmHandler;
		sigaction(SIGALRM, &alarmAct, 0);

		alarm(globalArgs.timeOut+1);
/*Create SNMP session*/
		void* session = snmpopen(globalArgs.COMMUNITY, globalArgs.HOSTNAME, globalArgs.timeOut);
/*Create buffer for snmp OID*/
		char snmpOID[100];
		memset(snmpOID, 0, 100);
/*Create buffer for SNMP output value (peercount). ~4G is a maximum namber + '\0' */
		char peercount[12];
		size_t sizeOfBuffer = sizeof(peercount);
		memset(peercount, 0, sizeOfBuffer);

		strcpy(snmpOID, "1.3.6.1.4.1.9.9.449.1.2.1.1.2.0.");
		strcat(snmpOID, globalArgs.AS);
/*
	Get the number of current EIGRP peers.
*/
		snmpget(session, snmpOID, peercount, sizeOfBuffer);
/*Start of Nagios Check*/
		if (strcmp(peercount, "0") == 0 ){
			exitcode=CRITICAL;
			printf("CRITICAL: This router has no EIGRP neighbors. |\n");
		} else if (strcmp(peercount, globalArgs.NEIGHBORS) != 0){
			exitcode=WARNING;
			printf("WARNING: Current neighbors counts is %s but schould be %s |\n", peercount, globalArgs.NEIGHBORS);
		} else {
			exitcode=OK;
			printf("OK: Neighbors count is %s |\n", peercount);
		}
/*End of Nagios Check*/
/*
	Get the list of current EIGRP peers.
*/		
		if ((exitcode == WARNING || exitcode == OK) && globalArgs.noList == 1){
/*Some integers for counts*/
			int i, peerNum;
/*Create buffer for SNMP output value (midlBuff).*/
			char midlBuff[18];
			sizeOfBuffer = sizeof(midlBuff);
			memset(midlBuff, 0, sizeOfBuffer);
/*Buffers and mutex for IOS version check*/
			char* iosver = midlBuff;
			char buffer[3];
			int mutex=0;
/*
	Get and check the IOS version for IP address converting
*/
			snmpget(session, "1.3.6.1.2.1.16.19.2.0", iosver, sizeOfBuffer);
			strncpy(buffer, iosver+1, 2);
			buffer[2]='\0';
/*If the major version of IOS is 15 then check minor version*/
			if (strcmp(buffer, "15") == 0) {
				memset(buffer, 0, 2);
				snprintf(buffer, 2, "%c", iosver[4]);
/*If minor version is 3 or higher then change mutex*/
				if (atoi(buffer) >= 3)
					mutex = 1;
			}
/*
	Get IP addresses
*/
			memset(snmpOID, 0, 100);
			strcpy(snmpOID, "1.3.6.1.4.1.9.9.449.1.4.1.1.3.0.");
			strcat(snmpOID, globalArgs.AS);
			strcat(snmpOID, ".");

			peerNum = atoi(peercount);
			char* peerip;

			for (i = 0; i<peerNum; i++) {
				memset(midlBuff, 0, 18);
				peerip = midlBuff;

				snprintf(snmpOID+33+strlen(globalArgs.AS), 12, "%d", i);
				snmpget(session, snmpOID, peerip, sizeOfBuffer);
/*
	Print the list of current EIGRP peers.
*/
				printf("\t%d: ", i+1);
				if (mutex == 1)
					printf("%.*s", strlen(peerip)-2,peerip+1);
				else {
					int l;
					while ((peerip = strtok(peerip, "\" ")) != NULL){
						sscanf(peerip,"%x",&l);
						if (mutex < 3)
							printf("%d.", l);
						else
							printf("%d", l);
						peerip = NULL;
						mutex++;
					}
					mutex=0;
				}
				printf("\n");
			}
		}
		if (session)
			snmp_close(session);
	}
	return exitcode;
}
