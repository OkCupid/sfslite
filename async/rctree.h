
// -*-c++-*-

#ifndef _ASYNC_RCTREE_H_INCLUDED_
#define _ASYNC_RCTREE_H_INCLUDED_ 1

#include "itree.h"


/*
 * rctree_t
 *
 *  This class allows a itree.h-like interface to reference-counted objects,
 *  so that as long as the object is in the list, its refcount is at least
 *  1.
 *
 */

template<class T>
struct rctree_entry_t {
public:
  rctree_entry_t () : _node (NULL) {}
  ~rctree_entry_t () { assert (!_node); }
    
  void put_in_list (void *v)
  {
    /* don't double-insert this object */
    assert (!_node); 
    _node = v;
  }
  void *v_node () { return _node; }
  
  void remove_from_list ()
  {
    assert (_node);
    _node = NULL;
  }

private:
  void *_node;
};

template<class K, class V, rctree_entry_t<T> T::*field>
class rctree_node_t {
public:
  rctree_node_t (const K &k, ptr<V> v) : 
    _key (k), _elm (v) { (e->*field).put_in_list (this); }

  const K _key;
  itree_entry<rctree_node_t<K,V,field> > _lnk;
  ptr<V> elem () { return _elm; }
private:
  ptr<V> _elm;
};

template<class K, class V, rctree_entry_t<V> T::*field, class C = compare<K> >
class rctree_t {
private:
  rctree_t (const rctree_t<K, V, field, C> &r);
  rctree_t &operator = (const rctree_t<K, V, field, C> &r);


public:

  rctree_t () {}
  ~rctree_t () { _tree.deleteall (); }
  
  typedef rctree_node_t<K, V, field> mynode_t;
  typedef itree<K, mynode_t, &mynode_t::_key, &mynode_t::_lnk> mytree_t;

private:
  
  static mynode_t *get_node (ptr<T> e) 
  { return static_cast<mynode_t *> ( (e->*field).v_node () ); }
  static const mynode_t *get_node (ptr<const T> e) 
  { return static_cast<const mynode_t *> ( (e->*field).v_node () ); }
  
  int search_adapter_cb (callback<int, ptr<T> >::ref fn, const T *el)
  {
    return (*fn) (el->elem ());
  }

public:

  ptr<T> root ()
  {
    ptr<T> ret; 
    if (_lst.root) ret = _lst.root->elem (); 
    return ret;
  }
  
#define WALKFN(typ,dir)                                               \
  static ptr<typ T> up (ptr<typ T> e)                                 \
  {                                                                   \
    ptr<typ T> ret;                                                   \
    typ mynode_t *p, *n;                                              \
    if (e && (p = get_node (e)) && (n = mytree_t::dir(p)))            \
      ret = n->elem ();                                               \
    return ret;                                                       \
  }

  WALKFN(,up)
  WALKFN(const,up)
  WALKFN(,right)
  WALKFN(const,right)
  WALKFN(,left)
  WALKFN(const,left)
#undef WALKFN

  static ptr<T> up (ptr<T> e) 
  {
    ptr<T> ret;
    mynode_t *p, *n;
    if (e && (p = get_node (e)) && (n = mytree_t::up (p)))
      ret = n->elem ();
    return ret;
  }

  static ptr<const T> right (ptr<const T> e) 
  {
    ptr<const T> ret;
    const mynode_t *p, *n;
    if (e && (p = get_node (e)) && (n = mytree_t::right (p)))
      ret = n->elem ();
    return ret;
  }
  
  static ptr<T> right (ptr<T> e) 
  {
    ptr<T> ret;
    mynode_t *p, *n;
    if (e && (p = get_node (e)) && (n = mytree_t::right (p)))
      ret = n->elem ();
    return ret;
  }

  void insert (const K &k, ptr<T> n) { _tree.insert (New mynode_t (k, n)); }
  
  void remove (ptr<T> p) {
    mynode_t *n = get_node (p);
    _tree.remove (n);
    (n->*field).remove_from_list ();
    delete n;
  }
  
  ptr<T> search (typename callback<int, ptr<T> >::ref fn) const 
  {
    ptr<T> ret;
    mynode_t *n = _tree.search (wrap (search_adapter_cb, fn));
    if (n) {
      ret = n->elem ();
    }
    return ret;
  }
  
  void deleteall () { _tree.deleteall (); }


private:
  mylist_t _lst;
};


#endif /* _ASYNC_RCTREE_H_INCLUDED_ */
