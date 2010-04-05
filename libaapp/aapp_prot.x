

enum ip_vers_t { IP_V4 = 4 };

union x_ip_addr_t switch (ip_vers_t vers) {
case IP_V4:
	unsigned v4;
default:
	void;
};

struct x_host_addr_t {
       x_ip_addr_t ip_addr;
       unsigned port;
};

struct aapp_newcon_t {
       x_host_addr_t addr;
};

enum aapp_status_t {
     AAPP_OK = 0,
     AAPP_BAD_FD = 1,
     AAPP_ERR = 2
};

typedef string logline_t<>;

namespace RPC {

program AAPP_SERVER_PROG {
	version APP_SERVER_VERS {

		void
		AAPP_SERVER_NULL(void) = 0;

		aapp_status_t
		AAPP_SERVER_NEWCON(aapp_newcon_t) = 1;

	} = 1;
} = 5402;

program LOGGER_PROG {
	version LOGGER_VERS {
		void 
		LOGGER_NULL(void) = 0;

		bool
		LOGGER_LOG(logline_t) = 1;

		bool
		LOGGER_TURN(void) = 2;
	} = 1;
} = 5403;

};


