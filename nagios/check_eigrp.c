#include <stdio.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <unistd.h>

//Nagios plugin exit status:
enum exitcode { OK, WARNING, CRITICAL, UNKNOWN };

static void usage(char* name)
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

static const char *optString = "H:c:p:a:h";

int main(int argc, char *argv[])
{
	int ERROR=OK;

//Inicialization of command-line arguments
	globalArgs.HOSTNAME=NULL;
	globalArgs.COMMUNITY=NULL;
	globalArgs.NEIGHBORS=0;
	globalArgs.AS="";

//Command-line arguments parsing
	int opt;
	while(-1 != (opt = getopt(argc, argv, optString))){
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
		}
	}

	if (globalArgs.HOSTNAME==NULL || globalArgs.COMMUNITY==NULL ||  globalArgs.NEIGHBORS=="0" || globalArgs.AS==""){
		usage(argv[0]);
	} else {

		netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, 1);
//Create OID from concatenate EIGRP AS number:
		char PEERCOUNTOID[38];
		strcpy(PEERCOUNTOID, "1.3.6.1.4.1.9.9.449.1.2.1.1.2.0.");
		strcat(PEERCOUNTOID, globalArgs.AS);

//Ok, starting the SNMP session:		
				
		struct snmp_session session, *ss;
		struct snmp_pdu *pdu;
		struct snmp_pdu *response;
		
		oid anOID[MAX_OID_LEN];
		size_t anOID_len = MAX_OID_LEN;
		
		struct variable_list *vars;
		
		init_snmp("check_eigrp");
		
		snmp_sess_init( &session );
		session.peername = strdup(globalArgs.HOSTNAME);
		
		session.version = SNMP_VERSION_2c;
		
		session.community = (u_char*) globalArgs.COMMUNITY;

		session.community_len = strlen(globalArgs.COMMUNITY);

		ss = snmp_open(&session);
		
		if (ss) {

			pdu = snmp_pdu_create(SNMP_MSG_GET);

//OK, get the current peer count from router
			read_objid(PEERCOUNTOID, anOID, &anOID_len);

			snmp_add_null_var(pdu, anOID, anOID_len);

			if (snmp_synch_response(ss, pdu, &response) == STAT_SUCCESS) {
				if (response->errstat == SNMP_ERR_NOERROR) {
					vars = response->variables;
//Copy the current peercount to array:				
					char PEERCOUNT[3];
					if (snprint_value(PEERCOUNT, sizeof(PEERCOUNT), vars->name, vars->name_length, vars) != -1) {
//Nagios check
						if (strcmp(PEERCOUNT, "0") == 0 ){
							ERROR=CRITICAL;
							fprintf(stderr, "CRITICAL: This router has no EIGRP neighbors.\n");
						} else if (strcmp(PEERCOUNT, globalArgs.NEIGHBORS) != 0){
							ERROR=WARNING;
							fprintf(stderr, "WARNING: Current neighbors counts is %s but schould be %s.\n", PEERCOUNT, globalArgs.NEIGHBORS);
						} else {
							ERROR=OK;
							fprintf(stderr, "OK: Neighbors count is %s.\n", PEERCOUNT);
						}
//End of Nagios check
					} else {
						ERROR=UNKNOWN;
						fprintf(stderr, "UNKNOWN: May be this router has not EIGRP protocol?\n");
					}
				} else {
					ERROR=UNKNOWN;
					fprintf(stderr, "Error in packet\nReason: %s\n",
					snmp_errstring(response->errstat));
				}
			} else {
				ERROR=UNKNOWN;
				snmp_sess_perror("ERROR: ", ss);
			}
		} else {
			snmp_perror("ERROR");
			snmp_log(LOG_ERR, "Some error occured in SNMP session establishment.\n");
			exit(UNKNOWN);
		}
		if (response) {
			snmp_free_pdu(response);
			snmp_close(ss);
		}
	}
	return ERROR;
}
