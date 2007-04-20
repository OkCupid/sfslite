
// -*-c++-*-

#include "tame_rpcserver.h"
#include "parseopt.h"

namespace tame {

  server_t::server_t (int fd, int v) : _verbosity (v)
  {
    tcp_nodelay (fd);
    _x = axprt_stream::alloc (fd);
  }

  void
  server_t::run()
  {
    _srv = asrv::alloc (_x, get_prog (), wrap (this, &server_t::_dispatch));
  }

  void
  server_t::_dispatch (svccb *sbp)
  {
    if (sbp) {
      dispatch (sbp);
    } else {
      if (_verbosity >= VERB_MED)
	warn << "EOF on socket received; deleting server...\n";
      delete this;
    }
  }

  void
  server_factory_t::new_connection (int lfd)
  {
    sockaddr_in sin;
    socklen_t sinlen = sizeof (sin);
    bzero (&sin, sinlen);
    int newfd = accept (lfd, reinterpret_cast<sockaddr *> (&sin), &sinlen);
    if (newfd >= 0) {
      if (_verbosity >= VERB_MED)
	warn ("accepting connection from %s\n", inet_ntoa (sin.sin_addr));
      server_t *srv = alloc_server (newfd, _verbosity);
      srv->run();
    } else if (errno != EAGAIN) {
      if (_verbosity >= VERB_LOW)
	warn ("accept failure: %m\n");
    }
  }

  void
  server_factory_t::run (const str &s, cbb done)
  {
    int port;
    if (!convertint (s, &port)) {
      warn << "Could not convert string to port: " << s << "\n";
      SIGNAL (done, false);
    } else if (port <= 0 || port > int (USHRT_MAX)) {
      warn << "Invalid port specified: " << port << "\n";
      SIGNAL (done, false);
    } else {
      run (u_int (port), done);
    }
  }

  TAMED void
  server_factory_t::run_T (u_int port, cbb done)
  {
    VARS {
      int fd;
      bool ret (false);
      bool go (true);
      coordgroup_t<bool> cg (__FILE__, __LINE__);
    }
    
    fd = inetsocket (SOCK_STREAM, port);
    if (fd <= 0) {
      warn << "cannot allocate TCP port: " << port << "\n";
    } else {
      ret = true;
      
      sigcb (SIGINT, @[cg,false]());
      sigcb (SIGTERM, @[cg,false]());
      
      close_on_exec (fd);
      listen (fd, 200);
      
      while (go) {

	fdcb (fd, selread, @[cg,true]());
	WAIT(cg, go);
	fdcb (fd, selread, NULL);

	if (go) 
	  new_connection (fd);
      }
    }
    SIGNAL (done, ret);
  }
  
};
