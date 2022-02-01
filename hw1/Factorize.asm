.data
	promopt:	.asciiz	"Enter number :"
	message1:	.asciiz	"Factor of "
	message2:	.asciiz	" is "
	message3:	.asciiz	", "
	exitMess:	.asciiz	"\nProgram is finished\n"
	errorMess:	.asciiz	"Invalid Input\n"

.text
	main:
		li 	$v0, 4
		la 	$a0, promopt
		syscall
		li 	$v0, 5
		syscall,
		move 	$t0, $v0 

	
		ble	$t0, $zero, printError
		addi 	$t1, $zero, 0
		li 	$v0, 4
		la 	$a0, message1
		syscall
		li	$v0, 1
		move	$a0, $t0
		syscall
		li 	$v0, 4
		la 	$a0, message2
		syscall

	
		loop:
			addi	$t1, $t1, 1
			bgt	$t1, $t0, exit
			div	$t0, $t1
			mfhi	$t2
			beqz	$t2, print
			j loop
		
		print:
			li	$v0, 1
			move	$a0, $t1
			syscall
			li 	$v0, 4
			la 	$a0, message3
			syscall
			jal	loop
	
	
		exit :
			li 	$v0, 4
			la 	$a0, exitMess
			syscall
			li 	$v0,10
			syscall
	
		printError:
			li 	$v0, 4
			la 	$a0, errorMess
			syscall
			jal exit
