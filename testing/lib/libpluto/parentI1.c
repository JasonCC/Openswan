#define LEAK_DETECTIVE
#define AGGRESSIVE 1
#define XAUTH 
#define MODECFG 
#define DEBUG 1
#define PRINT_SA_DEBUG 1
#define USE_KEYRR 1

#include "constants.h"
#include "oswalloc.h"
#include "whack.h"
#include "../../programs/pluto/rcv_whack.h"

#include "../../programs/pluto/connections.c"

#include "whackmsgtestlib.c"
#include "seam_timer.c"
#include "seam_pending.c"
#include "seam_ikev1.c"
#include "seam_crypt.c"
#include "seam_kernel.c"
#include "seam_rnd.c"
#include "seam_log.c"
#include "seam_xauth.c"
#include "seam_initiate.c"
#include "seam_alg.c"
#include "seam_x509.c"
#include "seam_spdbstruct.c"
#include "seam_demux.c"

u_int8_t reply_buffer[MAX_OUTPUT_UDP_SIZE];
bool nat_traversal_support_non_ike = FALSE;
bool nat_traversal_support_port_floating = FALSE;

unsigned char tc2_gi[] = {
 0xff, 0xbc, 0x6a, 0x92,  0xa6, 0xb9, 0x55, 0x9b,  
 0x05, 0xfa, 0x96, 0xa7,  0xa4, 0x35, 0x07, 0xb4,  
 0xc1, 0xe1, 0xc0, 0x86,  0x1a, 0x58, 0x71, 0xd9,  
 0xba, 0x73, 0xa1, 0x63,  0x11, 0x37, 0x88, 0xc0,  
 0xde, 0xbb, 0x39, 0x79,  0xe7, 0xff, 0x0c, 0x52,  
 0xb4, 0xce, 0x60, 0x50,  0xeb, 0x05, 0x36, 0x9e,  
 0xa4, 0x30, 0x0d, 0x2b,  0xff, 0x3b, 0x1b, 0x29,  
 0x9f, 0x3b, 0x80, 0x2c,  0xcb, 0x13, 0x31, 0x8c,  
 0x2a, 0xb9, 0xe3, 0xb5,  0x62, 0x7c, 0xb4, 0xb3,  
 0x5e, 0xb9, 0x39, 0x98,  0x20, 0x76, 0xb5, 0x7c,  
 0x05, 0x0d, 0x7b, 0x35,  0xc3, 0xc5, 0xc7, 0xcc,  
 0x8c, 0x0f, 0xea, 0xb7,  0xb6, 0x4a, 0x7d, 0x7b,  
 0x6b, 0x8f, 0x6b, 0x4d,  0xab, 0xf4, 0xac, 0x40,  
 0x6d, 0xd2, 0x01, 0x26,  0xb9, 0x0a, 0x98, 0xac,  
 0x76, 0x6e, 0xfa, 0x37,  0xa7, 0x89, 0x0c, 0x43,  
 0x94, 0xff, 0x9a, 0x77,  0x61, 0x5b, 0x58, 0xf5,  
 0x2d, 0x65, 0x1b, 0xbf,  0xa5, 0x8d, 0x2a, 0x54,  
 0x9a, 0xf8, 0xb0, 0x1a,  0xa4, 0xbc, 0xa3, 0xd7,  
 0x62, 0x42, 0x66, 0x63,  0xb1, 0x55, 0xd4, 0xeb,  
 0xda, 0x9f, 0x60, 0xa6,  0xa1, 0x35, 0x73, 0xe6,  
 0xa8, 0x88, 0x13, 0x5c,  0xdc, 0x67, 0x3d, 0xd4,  
 0x83, 0x02, 0x99, 0x03,  0xf3, 0xa9, 0x0e, 0xca,  
 0x23, 0xe1, 0xec, 0x1e,  0x27, 0x03, 0x31, 0xb2,  
 0xd0, 0x50, 0xf4, 0xf7,  0x58, 0xf4, 0x99, 0x27,  
};
unsigned int tc2_gi_len = sizeof(tc2_gi);

unsigned char tc2_ni[] = {
 0xb5, 0xce, 0x84, 0x19,  0x09, 0x5c, 0x6e, 0x2b,  
 0x6b, 0x62, 0xd3, 0x05,  0x53, 0x05, 0xb3, 0xc4,  
};
unsigned int tc2_ni_len = sizeof(tc2_ni);

unsigned char tc2_secret[] = {
 0x17, 0x9b, 0xb3, 0x22,  0xa6, 0x77, 0x6f, 0xbc,  
 0x01, 0x4e, 0x41, 0x03,  0xf0, 0xf6, 0x2e, 0x93,  
 0xfb, 0x07, 0xd0, 0x93,  0x84, 0x57, 0xe4, 0x54,  
 0x1e, 0x64, 0x46, 0xa9,  0x34, 0x37, 0xc0, 0x9d,  
};
unsigned int tc2_secret_len = sizeof(tc2_secret);

main(int argc, char *argv[])
{
    int   len;
    char *infile;
    char *conn_name;
    int  lineno=0;
    struct connection *c1;
    struct state *st;
    struct pluto_crypto_req r;
    struct pcr_kenonce *kn = &r.pcr_d.kn;

    EF_PROTECT_FREE=1;
    EF_FREE_WIPES  =1;

    progname = argv[0];
    leak_detective = 1;
    memset(&r, 0, sizeof(r));
    pcr_init(&r);

    if(argc != 3) {
	fprintf(stderr, "Usage: %s <whackrecord> <conn-name>\n", progname);
	exit(10);
    }
    /* argv[1] == "-r" */

    tool_init_log();
    init_pluto_vendorid();
    
    infile = argv[1];
    conn_name = argv[2];

    readwhackmsg(infile);

    send_packet_setup_pcap("parentI1.pcap");
 
    c1 = con_by_name(conn_name, TRUE);

    show_one_connection(c1);

    c1->extra_debugging = DBG_EMITTING|DBG_CONTROLMORE;
    ipsecdoi_initiate(/* whack-sock=stdout */1
		      , c1
		      , c1->policy
		      , 0
		      , FALSE
		      , pcim_demand_crypto);

    /* find st involved */
    st = state_with_serialno(1);

    
    /* now fill in the KE values from a constant.. not calculated */
    clonetowirechunk(&kn->thespace, kn->space, &kn->secret, tc2_secret,tc2_secret_len);
    clonetowirechunk(&kn->thespace, kn->space, &kn->n,   tc2_ni, tc2_ni_len);
    clonetowirechunk(&kn->thespace, kn->space, &kn->gi,  tc2_gi, tc2_gi_len);
    
    run_continuation(&r);

    /* clean up so that we can see any leaks */
    delete_state(st);

    report_leaks();

    tool_close_log();
    exit(0);
}


/*
 * Local Variables:
 * c-style: pluto
 * c-basic-offset: 4
 * compile-command: "make TEST=parentI1 one"
 * End:
 */
