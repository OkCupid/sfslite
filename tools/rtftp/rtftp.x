

enum rftp_status_t {
     RTFTP_OK = 0,
     RTFTP_NOENT = 1,
     RTFTP_CORRUPT = 2,
     RTFTP_EOF = 3,
     RTFTP_EEXISTS = 4
};

%#define RTFTP_HASHSZ 20
%#define CHUNKSZ 0x2000

typedef opaque rtftp_hash_t[RTFTP_HASHSZ];

typedef string rtftp_id_t<>;

struct rtftp_header_t {
       rtftp_id_t id;
       rtftp_hash_t hsh;
       unsigned size;
};

struct rtftp_footer_t {
       unsigned xfer_id;
       unsigned size;
       rtftp_hash_t hash;
};

struct rtftp_put_arg_t {
       rtftp_header_t file;
};

struct rtftp_get_arg_t {
       rtftp_id_t file;
};

struct rtftp_datchnk_t {
       unsigned hyper xfer_id;
       unsigned chnk_id;
       opaque dat<CHNKSZ>;
};

union rtftp_chnk_t switch (rtftp_status_t eof) {
case RTFTOP_EOF:
       rtftp_footer_t footer;
case RTFTP_OK:
       rtftp_datchnk_t dat;
default:
       void;
};

union rtftp_get_res_t switch (rtftp_status_t status) {
case RTFTP_OK:
        unsigned hyper xfer_id;
default:
	void;
};

struct rtftp_getchnk_arg_t {
       unsigned hyper file_id;
       unsigned chunk_id;
};

union rtftp_put_res_t switch (rtftp_status_t status) {
case RTFTP_OK:
       unsigned hyper file_id;
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

	rtftp_put_res_t
	RTFTP_PUT(rtftp_id_t) = 2;

	rtftp_get_res_t
	RTFTP_GET(rtftp_get_arg_t) = 3;

	rtftp_chnk_t 
	RTFTP_GETCHNK(rtftp_getchnk_arg_t) = 4;

	rtftp_status_t
	RTFTP_PUTCHNK(rtftp_chnk_t) = 5;

	} = 1;
} = 5401;	  

};

%#define RTFTP_TCP_PORT 5401
%#define RTFTP_UDP_PORT 5402

/* Maximum packet size is 1MB */

%#define MAX_PACKET_SZ 0x100000
