sys_exit:       equ             60
sys_write:      equ             1
sout:           equ             1
serr:           equ             2
error_exit_code equ             1
                section         .text
                global          _start

buf_size:       equ             8192
_start:
                xor             ebx, ebx
                sub             rsp, buf_size
                mov             rsi, rsp
                mov             r8,  1              ; flag

read_again:
                xor             eax, eax            ; sys_read
                xor             edi, edi
                mov             rdx, buf_size
                syscall
                mov             r15, rax
                test            r15, r15
                jz              quit
                js              read_error
                xor             rcx, rcx
check_char:
                xor             r12, r12
                mov             r12b, byte [rsi + rcx]
                cmp             r12, 0x20
                je              whitespace
                
                sub             r12, 9
                cmp             r12, 4      ; 13 - 9
                ja              letter                
whitespace:
                mov             r8, 1
                inc             rcx
                cmp             rcx, r15
                je              read_again
                jmp             check_char
letter:
                add             rbx, r8
                xor             r8, r8
                inc             rcx
                cmp             rcx, r15
                je              read_again
                jmp             check_char
quit:
                mov             rax, rbx
                call            print_int

                mov             rax, sys_exit
                xor             rdi, rdi
                syscall

; rax -- number to print
print_int:
                mov             rsi, rsp
                mov             rbx, 10

                dec             rsi
                mov             byte [rsi], 0x0a

next_char:
                xor             rdx, rdx
                div             rbx
                add             dl, '0'
                dec             rsi
                mov             [rsi], dl
                test            rax, rax
                jnz             next_char

                mov             rax, sys_write
                mov             rdi, sout
                mov             rdx, rsp
                sub             rdx, rsi
                syscall

                ret

read_error:
                mov             rax, sys_write
                mov             rdi, serr
                mov             rsi, read_error_msg
                mov             rdx, read_error_len
                syscall

                mov             rax, sys_exit
                mov             rdi, error_exit_code
                syscall

                section         .rodata

read_error_msg: db              "read failure", 0x0a
read_error_len: equ             $ - read_error_msg
