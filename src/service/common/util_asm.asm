;.386
.code
get_cur_cycles proc
	rdtsc
	ret
get_cur_cycles endp
end