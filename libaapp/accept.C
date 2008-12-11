
#include "aapp.h"
#include "aapp_prot.h"

namespace sfs {

  //-----------------------------------------------------------------------

  namespace x_host_addr {

    void 
    c2x (const sockaddr_in &in, x_host_addr_t *out)
    {
      out->port = in.sin_port;
      out->ip_addr.set_vers (IP_V4);
      *out->ip_addr.v4 = in.sin_addr.s_addr;
    }

    //--------------------------------------------------------------------

    void 
    x2c (const x_host_addr_t &in, sockaddr_in *out)
    {
      out->sin_port = in.port;
      if (in.ip_addr.vers == IP_V4) {
	out->sin_addr.s_addr = *in.ip_addr.v4;
      }
    }

    //--------------------------------------------------------------------

    str 
    x2s (const x_host_addr_t &in)
    {
      const char *ip;
      if (in.ip_addr.vers == IP_V4) {
	struct in_addr ia;
	ia.s_addr = *in.ip_addr.v4;
	ip = inet_ntoa (ia);
      } else {
	ip = "<n/a>";
      }
      port_t p = ntohl (in.port);
      strbuf b ("%s:%u", ip, p);
      return b;
    }

    //--------------------------------------------------------------------

  };

  //-----------------------------------------------------------------------

  static str
  arg2str (const aapp_newcon_t &arg)
  {
    const x_host_addr_t &addr = arg.addr;
    return x_host_addr::x2s (addr);
  }

  //-----------------------------------------------------------------------

  acceptor_t::acceptor_t (bool v, size_t ps)
    : _verbose (v),
      _packet_sz (ps) {}

  //-----------------------------------------------------------------------

  void
  acceptor_t::run (cbx_t cb)
  {
    _cb = cb;
    run_impl ();
  }

  //-----------------------------------------------------------------------

  void
  acceptor_t::accept_impl (int fd, const str &s)
  {
    if (_verbose) {
      warn ("accepting connection from %s\n", s.cstr ());
    }
    tcp_nodelay (fd);
    ref<axprt_stream> x = axprt_stream::alloc (fd, _packet_sz);
    (*_cb) (x);
  }

  //-----------------------------------------------------------------------

  net_acceptor_t::net_acceptor_t (port_t port, ip4_addr_t addr, 
				  bool v, size_t sz)
    : accept_acceptor_t (v, sz),
      _port (port), 
      _addr (addr)
  {
    inaddr_from_env ();
  }

  //-----------------------------------------------------------------------

  net_acceptor_t::~net_acceptor_t () {}

  //-----------------------------------------------------------------------

  accept_acceptor_t::accept_acceptor_t (bool v, size_t ps) 
    : acceptor_t (v, ps),
      _fd (-1) {}

  //-----------------------------------------------------------------------

  accept_acceptor_t::~accept_acceptor_t ()
  {
    if (_fd >= 0) {
      close (_fd);
      _fd = -1;
    }
  }

  //-----------------------------------------------------------------------

  str
  net_acceptor_t::addr_s () const
  {
    strbuf b;
    struct in_addr ia;
    ia.s_addr = htonl (_addr);
    const char *c = inet_ntoa (ia);
    b << c << ":" << _port;
    return b;
  }

  //-----------------------------------------------------------------------

  void
  net_acceptor_t::inaddr_from_env ()
  {
    if (_addr == INADDR_ANY) {
      struct in_addr ia;
      const char *c = getenv ("SFS_SERVER_ADDR");
      if (c && inet_aton (c, &ia) > 0) {
	warn << "binding to SFS_SERVER_ADDR=" << c << "\n";
	_addr = ntohl (ia.s_addr);
      }
    }
  }

  //-----------------------------------------------------------------------

  bool
  net_acceptor_t::init () 
  {
    bool ret = true;
    _fd = inetsocket (SOCK_STREAM, _port, _addr);
    if (_fd < 0) {
      str s = addr_s ();
      warn ("failed to bind to %s: %m\n", s.cstr ());
      ret = false;
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  accept_acceptor_t::run_impl ()
  {
    listen (_fd, qlen);
    fdcb (_fd, selread, wrap (this, &net_acceptor_t::accept));
  }

  //-----------------------------------------------------------------------

  void
  accept_acceptor_t::accept ()
  {
    sockaddr_in sin;
    socklen_t sinlen = sizeof (sin);
    bzero (&sin, sinlen);

    int nfd = ::accept (_fd, reinterpret_cast<sockaddr *> (&sin), &sinlen);
    if (nfd < 0 && errno != EAGAIN) {
      warn ("accept failure: %m\n");
    } else {
      strbuf addr ("%s:%u", inet_ntoa (sin.sin_addr), ntohs (sin.sin_port));
      str s = addr;
      accept_impl (nfd, s);
    }
  }

  //-----------------------------------------------------------------------

  slave_acceptor_t::slave_acceptor_t (int fd, bool v, size_t ps)
    : acceptor_t (v, ps),
      _fd (fd) {}

  //-----------------------------------------------------------------------

  bool
  slave_acceptor_t::init ()
  {
    bool ret = true;
    if (!isunixsocket (_fd)) {
      warn ("non-unixsocket given (fd=%d)\n", _fd);
      ret = false;
    } else {
      _x = axprt_unix::alloc (_fd);
    }
    return ret;
  }

  //-----------------------------------------------------------------------

  void
  slave_acceptor_t::run_impl ()
  {
    _srv = asrv::alloc (_x, aapp_server_prog_1, 
			wrap (this, &slave_acceptor_t::dispatch));
  }

  //-----------------------------------------------------------------------

  aapp_status_t
  slave_acceptor_t::newcon (const aapp_newcon_t *arg)
  {
    int fd = _x->recvfd ();
    aapp_status_t res;
    if (fd < 0) {
      res = AAPP_BAD_FD;
      warn ("acceptor got bad fd (%d)\n", fd);
    } else {
      accept_impl (fd, arg2str (*arg));
      res = AAPP_OK;
    }
    return res;
  }

  //-----------------------------------------------------------------------

  void
  slave_acceptor_t::dispatch (svccb *sbp)
  {
    if (!sbp) {
      // shouldn't really happen
      warn << "acceptor shutdown on EOF\n";
    } else {
      switch (sbp->proc ()) {
      case AAPP_SERVER_NEWCON:
	{
	  RPC::aapp_server_prog_1::aapp_server_newcon_srv_t<svccb> srv (sbp);
	  aapp_status_t st = newcon (srv.getarg ());
	  srv.reply (st);
	}
	break;
      default:
	sbp->reject (PROC_UNAVAIL);
	break;
      }
    }
  }

  //-----------------------------------------------------------------------

};
