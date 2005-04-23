

struct foo_t {
	string x<>;
	unsigned xx;
};

struct bar_t {
	hyper y<>;
};

struct baz_t {
	foo_t foos<4>;
	bar_t bar;
};

program FOO_PROG {
	version FOO_VERS {

		void
		FOO_NULL(void) = 0;
	
		unsigned
		FOO_FUNC (foo_t) = 1;

		foo_t
		FOO_BAR (bar_t) = 2;

		bar_t
		FOO_BAZ (baz_t) = 3;
	} = 1;
} = 100;
