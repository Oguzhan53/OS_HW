.data
	message:	.asciiz " prime\n"
	errorMess:	.asciiz	"Invalid Input\n"
	upperBound:	.word	2
	exitMess:	.asciiz	"Program is finished\n"
	promopt:	.asciiz	"Enter upper bound :"

.text
	
	main:
		li 	$v0, 4
		la 	$a0, promopt
		syscall
		li 	$v0, 5
		syscall,
		move 	$s1, $v0 

		addi 	$s1, $s1, 1
		addi 	$t6, $zero, 2
		ble	$s1, $t6, printError
		addi 	$t1, $zero, 2 # prime number counter 
		addi 	$t2, $zero, 2 #  dividing number counter
		addi	$t7, $zero, 1
		jal 	print
	
	
	
		loop1:
			addi	$t1, $t1, 1 #loop1 counter
			beq 	$t1, $s1, exit
			addi 	$t2, $zero, 2
			sub	$t5, $t1, $t7
			loop2:
				div 	$t1, $t2
				mfhi 	$t4
				beqz	$t4, loop1
				beq	$t5, $t2, print
				addi 	$t2, $t2, 1 # loop2 counter
				j loop2
			j loop1		
	
		
		print:
			li	$v0, 1
			move	$a0, $t1
			syscall
			li 	$v0, 4
			la 	$a0, message
			syscall
			jr 	$ra
	
	
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
	
