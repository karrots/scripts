#include <stdio.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <unistd.h>
#include <stdlib.h>

//Nagios plugin exit status:
enum EXITCODE { 
	OK,
	WARNING,
	CRITICAL,
	UNKNOWN
} exitcode;

void usage(char* name)
{
	printf("Usage: %s -H <hostipaddress> -c <community> -p <neighbors> -a <EIGRP AS number>\n", name);
	printf("Options:\n\t-h, \tShow this help message;\n");
	printf("\t-H, \tSpecify the hostname of router;\n");
	printf("\t-c, \tSpecify the SNMP community of router;\n");
	printf("\t-a, \tSpecify the EIGRP AS number of router;\n");
	printf("\t-p, \tSpecify the neighbors count of router.\n");
	exit(UNKNOWN);
}
// Structure for command-line arguments
struct globalArgs_t {
	const char 	*HOSTNAME;	//Hostname of monitoring router;
	char		*COMMUNITY;	//SNMP Community;
	const char 	*NEIGHBORS;	//Neighbors count;
	const char 	*AS;		//AS number of monitoring router.
} globalArgs;

const char *optString = "H:c:p:a:h";

void* snmpopen( char* community, const char* hostname){
	
	struct snmp_session session;
		
	init_snmp("check_eigrp");
		
	snmp_sess_init( &session );
	
	session.peername = strdup(hostname);
	session.version = SNMP_VERSION_2c;
	session.community = (u_char*) community;
	session.community_len = strlen(community);
	session.retries = 3;
	session.timeout = 3000;

	return snmp_open(&session);
}

void* snmpget (void *snmpsession, char *oidvalue, char *buffer, size_t buffersize){

	netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, 1);

	struct snmp_session *ss;
	oid anOID[MAX_OID_LEN];
	size_t anOID_len = MAX_OID_LEN;
	struct variable_list *vars;
	struct snmp_pdu *pdu;
	struct snmp_pdu *response;	

	ss = snmpsession;

	//Ok, starting the SNMP session:		
	if (ss) {

		pdu = snmp_pdu_create(SNMP_MSG_GET);

		//OK, get the SNMP vlaue from router
		read_objid(oidvalue, anOID, &anOID_len);
		snmp_add_null_var(pdu, anOID, anOID_len);

		if (snmp_synch_response(ss, pdu, &response) == STAT_SUCCESS) {
			if (response->errstat == SNMP_ERR_NOERROR) {
				vars = response->variables;
				//print_value(vars->name, vars->name_length, vars);				
				//printf("YAHOO %zu and %zu\n", vars->name_length, buffersize);
				if (snprint_value(buffer, buffersize, vars->name, vars->name_length, vars) == -1) {
					exitcode=UNKNOWN;
					fprintf(stderr, "UNKNOWN: May be this router has not EIGRP protocol?\n");
					snmp_close(ss);
					exit(exitcode);
				}
			} else {
				exitcode=UNKNOWN;
				fprintf(stderr, "Error in packet\nReason: %s\n",
				snmp_errstring(response->errstat));
				snmp_close(ss);
				exit(exitcode);
			}
		} else {
			exitcode=UNKNOWN;
			snmp_sess_perror("ERROR", ss);
			snmp_close(ss);
			exit(exitcode);
		}
	} else {
		exitcode=UNKNOWN;
		snmp_perror("ERROR");
		snmp_log(LOG_ERR, "Some error occured in SNMP session establishment.\n");
		exit(exitcode);
	}
	if (response) {
		snmp_free_pdu(response);
	}
}

int main(int argc, char *argv[])
{
//Inicialization of command-line arguments
	globalArgs.HOSTNAME=NULL;
	globalArgs.COMMUNITY=NULL;
	globalArgs.NEIGHBORS=0;
	globalArgs.AS="";

//Command-line arguments parsing
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
			case 'h':
				usage(argv[0]);
				break;
			default:
				usage(argv[0]);
				break;
		}
	}

	if (globalArgs.HOSTNAME==NULL || globalArgs.COMMUNITY==NULL ||  globalArgs.NEIGHBORS=="0" || globalArgs.AS==""){
		usage(argv[0]);
	} else {
//Some integers for counts
		int i, k, l;
//Create OID from concatenate EIGRP AS number:
		char peercountoid[37];
//Create buffer for SNMP output value (peercount). 65535 is a maximum namber + \0
		char peercount[6];
//Create OID for peers IP-addresses
		char peeripoid[100];
//Create buffer for SNMP output value (peerip).
		char peerip[18];
//Buffers and mutex for IOS version check
		char iosver[16];
		char buffer[3];
		int mutex=0;

		strcpy(peercountoid, "1.3.6.1.4.1.9.9.449.1.2.1.1.2.0.");
		strcat(peercountoid, globalArgs.AS);
//Open SNMP session
		void* session = snmpopen(globalArgs.COMMUNITY, globalArgs.HOSTNAME);
/*
	Get the number of current EIGRP peers.
*/
//And get value
		snmpget(session, peercountoid, peercount, sizeof(peercount));
//Start of Nagios Check
		if (strcmp(peercount, "0") == 0 ){
			exitcode=CRITICAL;
			fprintf(stderr, "CRITICAL: This router has no EIGRP neighbors.\n");
		} else if (strcmp(peercount, globalArgs.NEIGHBORS) != 0){
			exitcode=WARNING;
			fprintf(stderr, "WARNING: Current neighbors counts is %s but schould be %s:\n", peercount, globalArgs.NEIGHBORS);
		} else {
			exitcode=OK;
			fprintf(stderr, "OK: Neighbors count is %s:\n", peercount);
		}
/*
	Get the IOS version
*/
		snmpget(session, "1.3.6.1.2.1.16.19.2.0", iosver, sizeof(iosver));
		buffer[2]='\0';
		for (i=0;i<=1;++i)
			buffer[i] = iosver[i+1];
		if (strcmp(buffer, "15") == 0) {
			memset(buffer, 0, 3);
			buffer[0] = iosver[4];
			buffer[1] = '\0';
			if (atoi(buffer) >= 3)
				mutex = 1;
		}

/*
	Get the list of current EIGRP peers.
*/		
		if (exitcode == WARNING || exitcode == OK){
			strcpy(peeripoid, "1.3.6.1.4.1.9.9.449.1.4.1.1.3.0.");
			strcat(peeripoid, globalArgs.AS);
			strcat(peeripoid, ".0");

			for (i = 0; i<=atoi(peercount)-1; i++) {
				k = 1;
				l = i;
				while (i/10)
					++k;
				while (k--){
					peeripoid[33+strlen(globalArgs.AS)+k] = (char)(((int)'0')+l%10);
					l = l/10;
				}
				snmpget(session, peeripoid, peerip, sizeof(peerip));
/*
	Print the list of current EIGRP peers.
*/
				if ( mutex == 1)
					printf("\t%d: %s\n", i+1, peerip);
				else {
					printf("\t%d: ", i+1);

					for (k=0;k<=1;++k)
						buffer[k] = peerip[k+1];
					printf("%d", (int)strtol(buffer, NULL, 16));
					printf(".");

					for (k=0;k<=1;++k)
						buffer[k] = peerip[k+4];
					printf("%d", (int)strtol(buffer, NULL, 16));
					printf(".");

					for (k=0;k<=1;++k)
						buffer[k] = peerip[k+7];
					printf("%d", (int)strtol(buffer, NULL, 16));
					printf(".");
					for (k=0;k<=1;++k)
						buffer[k] = peerip[k+10];
					printf("%d", (int)strtol(buffer, NULL, 16));
					printf("\n");
				}
			}
		}
//End of Nagios Check
		if (session)
			snmp_close(session);
	}
	return exitcode;
}
