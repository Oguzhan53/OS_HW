/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.
   Execute MIPS syscalls in bare mode, when running on MIPS systems.
   Copyright (c) 1990-2010, James R. Larus.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the James R. Larus nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef _WIN32
#include <io.h>
#endif

#include "spim.h"
#include "string-stream.h"
#include "inst.h"
#include "reg.h"
#include "mem.h"
#include "sym-tbl.h"
#include "syscall.h"
/*
* I added this file because of the "read_assembly_file","starting_address" and
* "run_program" functions.
*/
#include "spim-utils.h"


#ifdef _WIN32
/* Windows has an handler that is invoked when an invalid argument is passed to a system
   call. https://msdn.microsoft.com/en-us/library/a9yf33zb(v=vs.110).aspx

   All good, except that the handler tries to invoke Watson and then kill spim with an exception.

   Override the handler to just report an error.
*/

#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>

void myInvalidParameterHandler(const wchar_t* expression,
                               const wchar_t* function,
                               const wchar_t* file,
                               unsigned int line,
                               uintptr_t pReserved)
{
    if (function != NULL)
    {
        run_error ("Bad parameter to system call: %s\n", function);
    }
    else
    {
        run_error ("Bad parameter to system call\n");
    }
}

static _invalid_parameter_handler oldHandler;

void windowsParameterHandlingControl(int flag )
{
    static _invalid_parameter_handler oldHandler;
    static _invalid_parameter_handler newHandler = myInvalidParameterHandler;

    if (flag == 0)
    {
        oldHandler = _set_invalid_parameter_handler(newHandler);
        _CrtSetReportMode(_CRT_ASSERT, 0); // Disable the message box for assertions.
    }
    else
    {
        newHandler = _set_invalid_parameter_handler(oldHandler);
        _CrtSetReportMode(_CRT_ASSERT, 1);  // Enable the message box for assertions.
    }
}
#endif




void backupAndRun(char * filename)
{

    /*
    * The part where the data in mem.h and reg.h file is backed up before creating a new process
    */

    //reg.h data
    reg_word buff_R[R_LENGTH];
    memcpy(buff_R, R, R_LENGTH* sizeof(reg_word));
    reg_word buff_HI = HI;
    reg_word buff_LO = LO;
    mem_addr buff_PC = PC;
    mem_addr buff_nPC=nPC;
    reg_word buff_CCR[4][32], buff_CPR[4][32];
    memcpy(buff_CCR, CCR, 4 * 32 * sizeof(reg_word));
    memcpy(buff_CPR, CPR, 4 * 32 * sizeof(reg_word));

    //mem.h data
    instruction **buff_text_seg= text_seg;
    text_seg = NULL;
    bool buff_text_modified=  text_modified;
    mem_addr buff_text_top = text_top;

    mem_word *buff_data_seg = data_seg;
    data_seg = NULL;
    bool buff_data_modified= data_modified;
    short *buff_data_seg_h = data_seg_h;
    data_seg_h = NULL;

    BYTE_TYPE *buff_data_seg_b = data_seg_b;
    data_seg_b = NULL;
    mem_addr buff_data_top =data_top;
    mem_addr buff_gp_midpoint = gp_midpoint;

    mem_word *buff_stack_seg = stack_seg;
    stack_seg = NULL;
    short *buff_stack_seg_h = stack_seg_h;
    stack_seg_h = NULL;
    BYTE_TYPE *buff_stack_seg_b =stack_seg_b;
    stack_seg_b = NULL;
    mem_addr buff_stack_bot = stack_bot;

    instruction **buff_k_text_seg = k_text_seg;
    k_text_seg = NULL;
    mem_addr buff_k_text_top = k_text_top;

    mem_word *buff_k_data_seg = k_data_seg;
    k_data_seg = NULL;
    short *buff_k_data_seg_h = data_seg_h;
    data_seg_h = NULL;
    BYTE_TYPE *buff_k_data_seg_b = k_data_seg_b;
    k_data_seg_b = NULL;
    mem_addr buff_k_data_top = k_data_top;


    /*
    * Running program
    */
    // This part taken from spim.cpp
    initialize_world(exception_file_name, false);
    read_assembly_file(filename);
    static mem_addr addr;
    bool continuable = false;

    addr = starting_address ();

    if (addr != 0)
    {
        char *undefs = undefined_symbol_string ();
        if (undefs != NULL)
        {
            write_output (message_out, "The following symbols are undefined:\n");
            write_output (message_out, undefs);
            write_output (message_out, "\n");
            free (undefs);
        }

        if (run_program (addr, DEFAULT_RUN_STEPS, false, false, &continuable))
            write_output (message_out, "Breakpoint encountered at 0x%08x\n", PC);
    }


    /*
    * The part where backed up files are restored.
    */
    memcpy(R, buff_R, R_LENGTH*sizeof(reg_word));
    HI = buff_HI;
    LO = buff_LO;
    PC = buff_PC;
    nPC = buff_nPC;
    memcpy(CCR, buff_CCR, 128 * sizeof(reg_word));
    memcpy(CPR, buff_CPR, 128 * sizeof(reg_word));

    text_seg = buff_text_seg;
    text_modified = buff_text_modified;
    text_top = buff_text_top;
    data_seg = buff_data_seg;
    data_modified = buff_data_modified;
    data_seg_h = buff_data_seg_h;
    data_seg_b = buff_data_seg_b;
    data_top = buff_data_top;
    gp_midpoint = buff_gp_midpoint;
    stack_seg = buff_stack_seg;
    stack_seg_h = buff_stack_seg_h;
    stack_seg_b = buff_stack_seg_b;
    stack_bot = buff_stack_bot;
    k_text_seg = buff_k_text_seg;
    k_text_top = buff_k_text_top;
    k_data_seg = buff_k_data_seg;
    k_data_seg_h = buff_k_data_seg_h;
    k_data_seg_b = buff_k_data_seg_b;
    k_data_top = buff_k_data_top;



    R[REG_V0] = 1;






}
/* Decides which syscall to execute or simulate.  Returns zero upon
   exit syscall and non-zero to continue execution. */

int
do_syscall ()
{
#ifdef _WIN32
    windowsParameterHandlingControl(0);
#endif

    /* Syscalls for the source-language version of SPIM.  These are easier to
       use than the real syscall and are portable to non-MIPS operating
       systems. */

    switch (R[REG_V0])
    {
    case PRINT_INT_SYSCALL:
        write_output (console_out, "%d", R[REG_A0]);
        break;

    case PRINT_FLOAT_SYSCALL:
    {
        float val = FPR_S (REG_FA0);

        write_output (console_out, "%.8f", val);
        break;
    }

    case PRINT_DOUBLE_SYSCALL:
        write_output (console_out, "%.18g", FPR[REG_FA0 / 2]);
        break;

    case PRINT_STRING_SYSCALL:
        write_output (console_out, "%s", mem_reference (R[REG_A0]));
        break;

    case READ_INT_SYSCALL:
    {
        static char str [256];

        read_input (str, 256);
        R[REG_RES] = atol (str);
        break;
    }

    case READ_FLOAT_SYSCALL:
    {
        static char str [256];

        read_input (str, 256);
        FPR_S (REG_FRES) = (float) atof (str);
        break;
    }

    case READ_DOUBLE_SYSCALL:
    {
        static char str [256];

        read_input (str, 256);
        FPR [REG_FRES] = atof (str);
        break;
    }

    case READ_STRING_SYSCALL:
    {
        read_input ( (char *) mem_reference (R[REG_A0]), R[REG_A1]);
        data_modified = true;
        break;
    }

    case SBRK_SYSCALL:
    {
        mem_addr x = data_top;
        expand_data (R[REG_A0]);
        R[REG_RES] = x;
        data_modified = true;
        break;
    }

    case PRINT_CHARACTER_SYSCALL:
        write_output (console_out, "%c", R[REG_A0]);
        break;

    case READ_CHARACTER_SYSCALL:
    {
        static char str [2];

        read_input (str, 2);
        if (*str == '\0') *str = '\n';      /* makes xspim = spim */
        R[REG_RES] = (long) str[0];
        break;
    }

    case EXIT_SYSCALL:
        spim_return_value = 0;
        return (0);

    case EXIT2_SYSCALL:
        spim_return_value = R[REG_A0];	/* value passed to spim's exit() call */
        return (0);

    case OPEN_SYSCALL:
    {
#ifdef _WIN32
        R[REG_RES] = _open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#else
        R[REG_RES] = open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#endif
        break;
    }

    case READ_SYSCALL:
    {
        /* Test if address is valid */
        (void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
        R[REG_RES] = _read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
        R[REG_RES] = read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
        data_modified = true;
        break;
    }

    case WRITE_SYSCALL:
    {
        /* Test if address is valid */
        (void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
        R[REG_RES] = _write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
        R[REG_RES] = write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
        break;
    }

    case CLOSE_SYSCALL:
    {
#ifdef _WIN32
        R[REG_RES] = _close(R[REG_A0]);
#else
        R[REG_RES] = close(R[REG_A0]);
#endif
        break;
    }
    /*
    * The case I created for the syscall I added
    */
    case EXECUTE_FILE_SYSCALL:
    {

        if(R[REG_A0]==1)
        {
            backupAndRun("BubbleSort.asm");

        }
        else if(R[REG_A0]==2)
        {
            backupAndRun("ShowPrimes.asm");

        }
        else if(R[REG_A0]==3)
        {
            backupAndRun("Factorize.asm");

        }
        else if(R[REG_A0]==4)  //Exit
        {
            R[REG_V0] = 0;
        }
        else // Unexpected input
        {
            R[REG_V0] = -1;
        }

        break;
    }

    default:
        run_error ("Unknown system call: %d\n", R[REG_V0]);
        break;
    }

#ifdef _WIN32
    windowsParameterHandlingControl(1);
#endif
    return (1);
}






void
handle_exception ()
{
    if (!quiet && CP0_ExCode != ExcCode_Int)
        error ("Exception occurred at PC=0x%08x\n", CP0_EPC);

    exception_occurred = 0;
    PC = EXCEPTION_ADDR;

    switch (CP0_ExCode)
    {
    case ExcCode_Int:
        break;

    case ExcCode_AdEL:
        if (!quiet)
            error ("  Unaligned address in inst/data fetch: 0x%08x\n", CP0_BadVAddr);
        break;

    case ExcCode_AdES:
        if (!quiet)
            error ("  Unaligned address in store: 0x%08x\n", CP0_BadVAddr);
        break;

    case ExcCode_IBE:
        if (!quiet)
            error ("  Bad address in text read: 0x%08x\n", CP0_BadVAddr);
        break;

    case ExcCode_DBE:
        if (!quiet)
            error ("  Bad address in data/stack read: 0x%08x\n", CP0_BadVAddr);
        break;

    case ExcCode_Sys:
        if (!quiet)
            error ("  Error in syscall\n");
        break;

    case ExcCode_Bp:
        exception_occurred = 0;
        return;

    case ExcCode_RI:
        if (!quiet)
            error ("  Reserved instruction execution\n");
        break;

    case ExcCode_CpU:
        if (!quiet)
            error ("  Coprocessor unuable\n");
        break;

    case ExcCode_Ov:
        if (!quiet)
            error ("  Arithmetic overflow\n");
        break;

    case ExcCode_Tr:
        if (!quiet)
            error ("  Trap\n");
        break;

    case ExcCode_FPE:
        if (!quiet)
            error ("  Floating point\n");
        break;

    default:
        if (!quiet)
            error ("Unknown exception: %d\n", CP0_ExCode);
        break;
    }
}
