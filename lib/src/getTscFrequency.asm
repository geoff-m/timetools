# uint64_t getTscFrequency()
# System V calling convention
    .global getTscFrequency

    .text
getTscFrequency:
    push %rbp
    mov %rsp, %rbp
    sub $, %rsp

    mov $15h, %eax
    cpuid
    mov %eax, -4(%rsp)
    mov %ebx, -8(%rsp)
    mov %ecx, -16(%rsp)



    add $24, %rsp
    ret