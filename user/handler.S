.text
.global __ex_handler
.type __ex_handler, @function
__ex_handler:
popl target
popl %eax
popf
add $0x4,target
jmp *target
/* use global var target to store the target pc */
.size __ex_handler, .-__ex_handler