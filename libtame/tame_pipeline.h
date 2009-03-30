
// -*-c++-*-
/* $Id: tame.h 2077 2006-07-07 18:24:23Z max $ */

#ifndef _LIBTAME_PIPELINE_H_
#define _LIBTAME_PIPELINE_H_

#include "async.h"
#include "tame.h"

//
// A class to automate pipelined operations.
//
namespace tame {

  //-----------------------------------------------------------------------

  class pipeliner_t {
  public:
    pipeliner_t (size_t w);
    virtual ~pipeliner_t () {}
    
    void run (evv_t done, CLOSURE);
    void cancel () { _cancelled = true; }
    
  protected:
    virtual void pipeline_op (size_t i, evv_t done, CLOSURE) = 0;
    virtual bool keep_going (size_t i) const = 0;
    
    size_t _wsz;
    rendezvous_t<> _rv;
    bool _cancelled;
    
  private:
    void wait_n (size_t n, evv_t done, CLOSURE);
    void launch (size_t i, evv_t done, CLOSURE);
  };
  
  //-----------------------------------------------------------------------

  typedef callback<void, size_t, cbb, ptr<closure_t> >::ref pipeline_op_t;
  void do_pipeline (size_t w, size_t n, pipeline_op_t op, evv_t done, CLOSURE);

  //-----------------------------------------------------------------------

  class pipeliner2_t {

  public:
    pipeliner2_t (size_t w, const char *f, int l);
    void wait (evv_t ev);
    void flush (evv_t ev);
    
    template<typename T1> ref<_event<T1> >
    pl_mkevent (T1 &t1) { _n_sent++; return mkevent (_rv, t1); }

    template<typename T1, typename T2> ref<_event<T1, T2> >
    pl_mkevent (T1 &t1, T2 &t2) { _n_sent++; return mkevent (_rv, t1, t2); }

    template<typename T1, typename T2, typename T3>
    ref<_event<T1, T2, T3> >
    pl_mkevent (T1 &t1, T2 &t2, T3 &t3) 
    { _n_sent++; return mkevent (_rv, t1, t2, t3); }

  private:
    void wait (evv_t ev, size_t w, CLOSURE);

    size_t _wsz;
    size_t _n_sent, _n_recv;
    rendezvous_t<> _rv;
  };

  //-----------------------------------------------------------------------

};


#endif /* _LIBTAME_PIPELINE_H_ */


