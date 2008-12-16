// -*-c++-*-

#ifndef __LIBASYNC__SFS_BUNDLE_H__
#define __LIBASYNC__SFS_BUNDLE_H__

namespace sfs {

  struct nil_t {};
  extern nil_t g_nil;
  
  template<class C1, class C2, class C3 = nil_t, class C4 = nil_t>
  class bundle_t {
  public:
    bundle_t (const C1 &v1, const C2 &v2, const C3 &v3 = g_nil, 
	      const C4 &v4 = g_nil)
      : _v1 (v1), _v2 (v2), _v3 (v3), _v4 (v4) {}
    
    const C1 &obj1 () const { return _v1; }
    const C2 &obj2 () const { return _v2; }
    const C3 &obj3 () const { return _v3; }
    const C4 &obj4 () const { return _v4; }
    
    C1 &obj1 () { return _v1; }
    C2 &obj2 () { return _v2; }
    C3 &obj3 () { return _v3; }
    C4 &obj4 () { return _v4; }
    
  private:
    C1 _v1;
    C2 _v2;
    C3 _v3;
    C4 _v4;
  };

};


#endif /* __LIBASYNC__SFS_BUNDLE_H__ */
