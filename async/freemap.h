

// -*- c++ -*-

struct node_t {
  node_t (u_int32_t id);
  ~node_t () {}

  bool getbit (u_int i) const;
  void setbit (u_int i, bool b);
  bool is_empty () const;
  int topbit () const;

  enum { n_bits = (sizeof (_bits) * 8) };

  u_int32_t _id;
private:
  u_int32_t _bits;
};

class freemap_t {
public:
  freemap_t (size_t c);
  ~freemap_t ();

  int alloc ();
  void dealloc (u_int i);

private:
  void grow ();
  void clobber_root ();
  void swap (size_t i, size_t j);
  void push_down ();

  u_int32_t ind2key (size_t i) const;
  int check_index (int i) { return i < _cursor ? i : -1; }
  int left (int i) { return check_index (2 * i + 1); }
  int right (int i) { return check_index (2 * i + 2); }
  int parent (int i) { return check_index ((i -1 ) / 2); }

  size_t _capacity;
  size_t _cursor;
  node_t *_nodes;
};
