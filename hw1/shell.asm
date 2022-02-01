
.data
	promopt: 	.asciiz "\nEnter program number:\n 1-BubbleSort.asm\n 2-ShowPrimes.asm\n 3-Factorize.asm\n 4-Exit\n "    
	errorMess: 	.asciiz "Wrong input number\n"
	exitMess:	.asciiz	"\nProgram is finished\n"
	

.text

	main:
		addi	$t0, $zero, 4
		loop:   
			la	$a0, promopt
			li	$v0, 4
			syscall
			li 	$v0, 5
			syscall
			beq 	$v0, $t0, exit  # ret $v0 == 4, "exit" command has given, exiting.
			ble 	$v0, $zero, printError # ret $v0 == -1, program does not found, listening.
			bgt 	$v0, $t0, printError			
			move 	$a0, $v0
			li 	$v0, 18      # create process from that program.
			syscall
			
			j loop

		printError:  
			la 	$a0, errorMess
			li 	$v0, 4
			syscall
			jal loop

		exit:	
			li 	$v0, 4
			la 	$a0, exitMess
			syscall
			li 	$a0, 0
			li 	$v0, 17
		    	syscall
