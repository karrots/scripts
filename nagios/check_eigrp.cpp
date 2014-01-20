#include <stdio.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <string>
#include <unistd.h>
#include <iostream>
using namespace std;

static void usage(string name)
{
    cerr << "Usage: " << name << " -H <hostipaddress> -c <community> -p <neighbors> -a <EIGRP AS number>\n "
              << "Options:\n"
              << "\t-h, \tShow this help message;\n"
              << "\t-H, \tSpecify the hostname of router;\n"
	      << "\t-c, \tSpecify the SNMP community of router;\n"
	      << "\t-a, \tSpecify the EIGRP AS number of router;\n"
	      << "\t-p, \tSpecify the neighbors count of router."
              << endl;
}

struct globalArgs_t {
	const char *HOSTNAME;	//Hostname of monitoring router;
	char *COMMUNITY;	//SNMP Community;
	const char *NEIGHBORS;	//Neighbors count;
	string AS;				//AS number of monitoring router.
} globalArgs;

static const char *optString = "H:c:p:a:h";

int main(int argc, char *argv[])
{

//Nagios plugin exit status:
	int OK=0, WARNING=1, CRITICAL=2, UNKNOWN=3, USAGE=5;
	int ERROR=OK;

	globalArgs.HOSTNAME=NULL;
	globalArgs.COMMUNITY=NULL;
	globalArgs.NEIGHBORS=0;
	globalArgs.AS="";

	int opt = getopt(argc, argv, optString);
	while(opt!=-1){
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
				break;
		}
		opt = getopt(argc, argv, optString);
	}

	if (globalArgs.HOSTNAME==NULL || globalArgs.COMMUNITY==NULL ||  globalArgs.NEIGHBORS=="0" || globalArgs.AS==""){
		ERROR=USAGE;
	} else {

//Ok, starting the SNMP session:		
		
		struct snmp_session session, *ss;
		struct snmp_pdu *pdu;
		struct snmp_pdu *response;
		
		oid anOID[MAX_OID_LEN];
		size_t anOID_len = MAX_OID_LEN;
		
		struct variable_list *vars;
		int status;
		
		init_snmp("check_eigrp");
		
		snmp_sess_init( &session );
		session.peername = strdup(globalArgs.HOSTNAME);
		
		session.version = SNMP_VERSION_2c;
		
		session.community = (u_char*) globalArgs.COMMUNITY;

		session.community_len = strlen(globalArgs.COMMUNITY);

		ss = snmp_open(&session);
		
		if (!ss) {
    			snmp_perror("ack");
				snmp_log(LOG_ERR, "Some error occured in SNMP session establishment.\n");
       			exit(UNKNOWN);
   		}

		string PEERCOUNTOID="1.3.6.1.4.1.9.9.449.1.2.1.1.2.0."+globalArgs.AS;

		pdu = snmp_pdu_create(SNMP_MSG_GET);
		
		read_objid(PEERCOUNTOID.c_str(), anOID, &anOID_len);
		get_node(PEERCOUNTOID.c_str(), anOID, &anOID_len);
		
		snmp_add_null_var(pdu, anOID, anOID_len);

		status = snmp_synch_response(ss, pdu, &response);

		netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, 1);

		if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
			for (vars = response->variables; vars; vars = vars->next_variable) {
				if (vars->val.integer != 0) {

					long long int PEERCOUNT = *(vars->val.integer);
//Nagios check
					string PEERCOUNTstr=to_string(PEERCOUNT);
					if (PEERCOUNTstr != globalArgs.NEIGHBORS){
						ERROR=CRITICAL;
						fprintf(stderr, "Current neighbors counts is %d but schould be %s.\n", PEERCOUNT, globalArgs.NEIGHBORS);
					} else {
						ERROR=OK;
						fprintf(stderr, "Neighbor count is %ld.\n", PEERCOUNT);
					}
//End of Nagios check
				} else {
					ERROR=UNKNOWN;
				}
      		}
		} else {
			if (status == STAT_SUCCESS) {
						ERROR=UNKNOWN;
						fprintf(stderr, "Error in packet\nReason: %s\n",
               			snmp_errstring(response->errstat));
     			} else {
						ERROR=UNKNOWN;
						snmp_sess_perror("ERROR: ", ss);
			}
		}
		if (response) {
     			snmp_free_pdu(response);
				snmp_close(ss);
		}
	}
	if (ERROR==USAGE){
		usage(argv[0]);
	} else {	
		return ERROR;
	}
}
