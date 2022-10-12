.include "macros.inc"

	start

	test_name STLD_10000
	st	-32,[0x10000]
	ld 	r2,[0x10000]
	check_r2	-32

	test_name STLD_AS
	movl	r3, 0x10000
	st.as	-30,[r3,1]
	ld.as 	r2,[r3,1]
	check_r2	-30

	test_name LD_S_AS
	movl	r3, 0x10000
	movl	r2, 1
	st.as	-31,[r3,1]
	ld_s.as	r2,[r3,r2]
	check_r2	-31

	end
