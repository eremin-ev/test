/*
 * See the "How to create a Linux system that runs a single
 * application?" StackExchange/SuperUser question [1], and
 * specifically Ciro Santilli's answer [2].
 *
 * [1] https://superuser.com/questions/320529/how-to-create-a-linux-system-that-runs-a-single-application/
 * [2] https://superuser.com/questions/320529/how-to-create-a-linux-system-that-runs-a-single-application/991733#991733
 */
.global _start
_start:
    /*
     * See Standard file descriptor defines
     *
     * #define	STDIN_FILENO	0	// Standard input
     * #define	STDOUT_FILENO	1	// Standard output
     * #define	STDERR_FILENO	2	// Standard error output
     *
     * in /usr/include/unistd.h
     */
    movq $1, %rax
    /*
     * See syscall number defines, and specifically
     *
     *    #define __NR_write 1
     *
     * in /usr/include/asm/unistd.h
     */
    movq $1, %rdi
    movq $message, %rsi
    movq $message_len, %rdx
    syscall
    jmp .
    message: .ascii "Hello, world!\n"
    .equ message_len, . - message
