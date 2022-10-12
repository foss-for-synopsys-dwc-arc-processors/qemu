.include "macros.inc"

	start

	test_name STLD_1
	st	-32,[0x10000]
	ld 	r2,[0x10000]
	check_r2	-32

	test_name STLD_2
	mov	r3, 0x10000
	mov	r2, 1
	st.as	-31,[r3,1]
	ld_s.as	r2,[r3,r2]
	check_r2	-31

	end
