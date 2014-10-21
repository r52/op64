.code
fpu_cmp_64 proc
    movsd xmm0, QWORD PTR[rcx]
    movsd xmm1, QWORD PTR[rdx]
    comisd xmm0, xmm1
	ret
fpu_cmp_64 endp
end

