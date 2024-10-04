# bool hasGoodCpuTimer()
# System V calling convention
    .global hasGoodCpuTimer

    .text
hasGoodCpuTimer:
    push %rbx

    # test for cpuid leaf >= 15h
    xor %eax, %eax
    cpuid
    cmp $15h, %eax
    jl no

    # test for rdtsc
    mov $1, %eax
    cpuid
    test $16, %edx
    jz no

    # test for cpuid 15h, eax != 0 && ebx != 0 && ecx != 0
    mov $15h, %eax
    cpuid
    test %eax, %eax
    jz no
    test %ebx, %ebx
    jz no
    test %ecx, %ecx
    jz no

yes:
    mov $1, %eax
    pop %rbx
    ret

no:
    xor %eax, %eax
    pop %rbx
    ret