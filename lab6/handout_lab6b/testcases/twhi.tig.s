.text
.globl tigermain
.type tigermain, @function
tigermain:
pushl %ebp
movl %esp,%ebp
subl $8,%esp
L6:
movl %edi,-8(%ebp)
movl $10,%ecx
movl %ecx,%edi
L4:
movl $0,%ecx
cmpl %ecx,%edi
jge L3
L0:
movl -8(%ebp),%edi
jmp L5
L3:
pushl %edi
call printi
movl $1,%ecx
movl %edi,%edx
subl %ecx,%edx
movl %edx,%edi
movl $2,%ecx
cmpl %ecx,%edi
je L1
L2:
jmp L4
L1:
jmp L0
L5:

leave
ret

