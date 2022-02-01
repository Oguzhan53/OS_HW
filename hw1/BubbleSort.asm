.data 
	arr:		.space 	40
	promopt:	.asciiz	"Enter array size(max 10): "
	promopt2:	.asciiz "\nEnter number :"
	errorMess:	.asciiz "\nInvalid array size\n" 
	exitMess:	.asciiz	"\nProgram is finished\n"
	str :		.asciiz	" - "
.text
	main:
		addi	$t0, $zero, 10
		li 	$v0, 4
		la 	$a0, promopt
		syscall
		li 	$v0, 5
		syscall,
		bgt	$v0, $t0, printError
		blt	$v0, $zero,printError  
		move 	$t0, $v0  # array size
		addi	$t3, $zero, 4
		mul	$t3, $t3, $t0
		addi	$t1, $zero, 0
		loop0:
			bge	$t1, $t3, bubbleSort
			li 	$v0, 4
			la 	$a0, promopt2
			syscall
			li 	$v0, 5
			syscall,
			move	$t4, $v0
			sw	$t4, arr($t1)
			addi	$t1, $t1, 4
			j 	loop0
			
		
		
		bubbleSort:
		
			addi	$t1, $zero, -4 # loop1 counter
			loop1:
				addi	$t1, $t1, 4
				bge 	$t1, $t3, printArr
				addi	$t4, $zero, -4 # loop2 conter
				sub 	$t5, $t3, $t1
				sub 	$t5, $t5, 4
				loop2:
					addi	$t4, $t4, 4
					bge	$t4, $t5, loop1
					lw	$s0, arr($t4)
					addi	$t6, $t4, 4
					lw	$s1, arr($t6)
					bgt	$s0, $s1, swap
					j loop2
				j loop1
			
		swap:
			sw	$s0, arr($t6)
			sw	$s1, arr($t4)
			jal loop2
	
		printArr:
			beq 		$t2, $t3, exit
			li 		$v0, 1
			lw 		$a0, arr($t2)
			syscall
			li 		$v0, 4
			la		$a0, str
			syscall
			addi 		$t2, $t2, 4
			j printArr

		printError:
			li 	$v0, 4
			la 	$a0, errorMess
			syscall
			jal exit

		exit :
			li 	$v0, 4
			la 	$a0, exitMess
			syscall
			li 	$v0,10
			syscall
