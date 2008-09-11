

enum rtftp_status_t {
     RTFTP_OK = 0,
     RTFTP_NOENT = 1,
     RTFTP_CORRUPT = 2,
     RTFTP_EOF = 3,
     RTFTP_EEXISTS = 4,
     RTFTP_EFS = 5
};

%#define RTFTP_HASHSZ 20
%#define CHUNKSZ 0x2000

typedef opaque rtftp_hash_t[RTFTP_HASHSZ];

typedef string rtftp_id_t<>;
typedef opaque rtftp_data_t<>;

struct rtftp_file_t {
       rtftp_id_t name;
       rtftp_data_t data;
       rtftp_hash_t hash;
};

union rtftp_get_res_t switch (rtftp_status_t status) {
case RTFTP_OK:
       rtftp_file_t file;
default:
       void;
};

namespace RPC {

program RTFTP_PROGRAM { 
	version RTFTP_VERS {

	void
	RTFTP_NULL (void) = 0;

	rtftp_status_t
	RTFTP_CHECK(rtftp_id_t) = 1;

	rtftp_status_t
	RTFTP_PUT(rtftp_file_t) = 2;

	rtftp_get_res_t
	RTFTP_GET(rtftp_id_t) = 3;

	} = 1;
} = 5401;	  

};

%#define RTFTP_TCP_PORT 5401
%#define RTFTP_UDP_PORT 5402

%#define MAX_PACKET_SIZE 0x1000000

