
#include "arpc.h"
#include "tame_event.h"

namespace tame_rpc {

  inline void
  call (ptr<aclnt> c, rpc_bundle_t b, event<clnt_stat>::ref ev)
  {
    aclnt_cb cb = ev;
    c->call (b.proc (), b.arg (), b.res (), cb);
  }

  inline callbase *
  rcall (ptr<aclnt> c, rpc_bundle_t b, event<clnt_stat>::ref ev)
  {
    aclnt_cb cb = ev;
    return c->call (b.proc (), b.arg (), b.res (), cb);
  }

};
