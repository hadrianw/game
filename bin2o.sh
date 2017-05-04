#!/bin/sh

filename="$1"
name=$(echo "$1" | sed "s/[^A-Za-z0-9]/_/g")
obj="$2"

echo \
"	.section .rodata
	.global ${name}
	.type ${name}, @object
	.global ${name}_size
${name}:
	.incbin \"${filename}\"
1:
${name}_size:
	.int 1b - ${name}
	.section .note.GNU-stack,\"\",%progbits
" | gcc -x assembler -c - -o "$obj"
