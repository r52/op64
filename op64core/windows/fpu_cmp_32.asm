.code
fpu_cmp_32 proc
    movss xmm0, DWORD PTR[rcx]
    movss xmm1, DWORD PTR[rdx]
    comiss xmm0, xmm1
	ret
fpu_cmp_32 endp
end

