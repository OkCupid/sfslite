
typedef string ex2_str_t<255>;

struct ex2_struct_t {
	ex2_str_t 	s;
	unsigned 	u;
};

program EX2_PROG {
	version EX2_VERS {
		void
		EX2_NULL(void) = 0;
	
		int
		EX2_RANDOM(void) = 1;

		ex2_str_t
		EX2_REVERSE(ex2_str_t) = 2;

		ex2_struct_t
		EX2_STRUCT(void) = 3;
	} = 1;
} = 31313;
