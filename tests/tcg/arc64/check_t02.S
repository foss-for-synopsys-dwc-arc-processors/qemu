	.include "macros.inc"
	start

	test_name PREDICATE_1
	mov	r2,2
	lsr.f	r2,r2
	mov.nc	r2,1
	mov.cs	r2,-1	# Should not execute
	check_r2	0x01

	test_name PREDICATE_2
	mov	r2,2
	xor.f	0,r2,r2
	mov_s.ne	r2,-1	# Should not execute
	check_r2	2

	test_name PREDICATE_3
	mov	r2,3
	xor.f	0,r2,r2
	sub_s.ne	r2,r2,r2	# Should not execute
	check_r2	3

	end
