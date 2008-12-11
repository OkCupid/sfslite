
// -*-c++-*-
#ifndef _LIBAAPP_AAPP_H_
#define _LIBAAPP_AAPP_H_

#include "async.h"
#include "arpc.h"
#include "aapp_prot.h"

namespace sfs {

  //-----------------------------------------------------------------------

  typedef callback<void, ptr<axprt_stream> >::ref cbx_t;

  //-----------------------------------------------------------------------

  typedef u_int32_t port_t;
  typedef u_int32_t ip4_addr_t;

  //-----------------------------------------------------------------------

  class acceptor_t {
  public:
    acceptor_t (bool verbose = true, size_t ps = axprt::defps);
    virtual ~acceptor_t () {}
    virtual bool init () = 0;
    void run (cbx_t cb);

  protected:
    void accept_impl (int fd, const str &a);
    virtual void run_impl () = 0;

    bool _verbose;
    size_t _packet_sz;

  private:
    cbx_t::ptr _cb;
  };

  //-----------------------------------------------------------------------

  class accept_acceptor_t : public acceptor_t {
  public:
    accept_acceptor_t (bool verbose, size_t ps);
    ~accept_acceptor_t ();

    enum { qlen = 200 };
  protected:
    void run_impl ();
    void accept ();

    int _fd;
  };

  //-----------------------------------------------------------------------

  class net_acceptor_t : public accept_acceptor_t {
  public:

    // NB: addr and port are in HOST order
    net_acceptor_t (port_t port, ip4_addr_t addr = INADDR_ANY,
		    bool verbose = true, size_t ps = axprt::defps);

    ~net_acceptor_t ();
    bool init ();

  protected:
    void inaddr_from_env ();
    str addr_s () const;
  private:
    port_t _port;
    ip4_addr_t _addr;
  };

  //-----------------------------------------------------------------------

  class slave_acceptor_t : public acceptor_t {
  public:
    slave_acceptor_t (int fd = 0, bool verbose = true, 
		      size_t ps = axprt::defps);
    bool init ();
    void run (cbx_t cb);
  protected:
    void dispatch (svccb *sbp);
    aapp_status_t newcon (const aapp_newcon_t *arg);
    void run_impl ();
  private:
    ptr<axprt_unix> _x;
    ptr<asrv> _srv;
    int _fd;
  };

  //-----------------------------------------------------------------------

  namespace x_host_addr {
   
    void c2x (const sockaddr_in &in, x_host_addr_t *out);
    void x2c (const x_host_addr_t &in, sockaddr_in *out);
    str x2s (const x_host_addr_t &in);

  };


  //-----------------------------------------------------------------------

};

#endif /* _LIBAAPP_AAPP_H_ */
