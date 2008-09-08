

enum rftp_status_t {
     RTFTP_OK = 0,
     RTFTP_NOENT = 1,
     RTFTP_CORRUPT = 2		 
};

%#define RTFTP_HASHSZ 20

typedef opaque rtftp_hash_t[RTFTP_HASHSZ];

typedef string rtftp_id_t<>;
typedef string rtftp_dat_t<>;

struct rtftp_file_t {
       rtftp_id_t id;
       rtftp_dat_t dat;
       rtftp_hash_t hsh;
};

struct rtftp_put_arg_t {
       rtftp_file_t file;
};

struct rtftp_get_arg_t {
       rtftp_file_t file;
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
	RTFTP_CHECK(rtftp_file_id_t) = 1;

	rtftp_status_t
	RTFTP_PUT(rtftp_put_arg_t) = 2;

	rtftp_get_res_t
	RTFTP_GET(rtftp_get_arg_t) = 3;

	} = 1;
} = 5401;	  

};

%#define RTFTP_TCP_PORT 5401
%#define RTFTP_UDP_PORT 5402
