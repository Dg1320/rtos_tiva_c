; Memory Protection Unit
; DeZean Gardner

;-----------------------------------------------------------------------------
; Hardware Target
;-----------------------------------------------------------------------------

; Target Platform: EK-TM4C123GXL
; Target uC:       TM4C123GH6PM
; System Clock:    40 MHz

;-----------------------------------------------------------------------------
; Device includes, defines, and assembler directives
;-----------------------------------------------------------------------------

	.def userMode
	.def kernelMode
    .def setPSP
    .def setMSP
    .def setASP
    .def getPSP
    .def getMSP
    .def getXPSR
    .def getAPSR
    .def getEPSR
    .def getIPSR
	.def runFirstTask
	.def appearAsRanTask
	.def saveR4toR11
	.def taskSwitch
	.def getSVCallNumber
	.def returnR0
	.def returnR3
	.def returnR12
	.def gobacktoPSP
	.def appearAsRanTaskAgain
;-----------------------------------------------------------------------------
; Register values and large immediate values
;-----------------------------------------------------------------------------

GOTO_PSP_AFTER       .field  0xFFFFFFFD				; exception value to tell to go to psp after
													; see data sheet pg. 111

.thumb
.text

setPSP:									; set the process stack pointer
			 MSR PSP, R0				; have to use main stack register to set the ASP (active stack pointer
			 							; since this is a special register
			 ISB 						; Instruction Synchronisation Barrier
			 							; must follow immideately after MSR command
			 BX   LR					; branch and link back to previous location
setMSP:									; set the process stack pointer
			 MSR MSP, R0				; have to use main stack register to set the ASP (active stack pointer
			 							; since this is a special register
			 ISB 						; Instruction Synchronisation Barrier
			 							; must follow immideately after MSR command
			 BX   LR					; branch and link back to previous location

userMode: 	 MRS R0 , CONTROL 			; move the control registister into General purpose register
			 ORR R0 , R0 , #1			; set the USERMODE bit in fied 0b0001
			 MSR CONTROL , R0			; move value set back into control register
			 ISB						; Instruction Synchronisation Barrier
			 BX  LR

kernelMode:  MRS R0 , CONTROL 			; move the control registister into General purpose register
			 BIC R0 , R0 , #1			; set the KERNELMODE bit in fied 0b0001
			 MSR CONTROL , R0			; move value set back into control register
			 ISB						; Instruction Synchronisation Barrier
			 BX  LR

setASP: 	 MRS R0 , CONTROL 			; move the control registister into General purpose register
			 ORR R0 , R0 , #2			; set the Active Stack Pointer bit in fied 0b0010
			 MSR CONTROL , R0			; move value set back into control register
			 ISB						; Instruction Synchronisation Barrier
			 BX  LR						; branch and link back to previous location


getPSP: 	 MRS R0 , PSP 				; move the PSP registister into General purpose register
			 ISB						; Instruction Synchronisation Barrier
			 BX  LR						; branch and link back to previous location

getMSP: 	 MRS R0 , MSP 				; move the MSP registister into General purpose register
			 ISB						; Instruction Synchronisation Barrier
			 BX  LR

getXPSR: 	 MRS R0 , XPSR 				; move the xPSR registister into General purpose register
			 ISB						; Instruction Synchronisation Barrier
			 BX  LR

getAPSR: 	 MRS R0 , APSR 				; move the APSR registister into General purpose register
			 ISB						; Instruction Synchronisation Barrier
			 BX  LR

getEPSR: 	 MRS R0 , EPSR 				; move the EPSR registister into General purpose register
			 ISB						; Instruction Synchronisation Barrier
			 BX  LR

getIPSR: 	 MRS R0 , IPSR 				; move the IPSR registister into General purpose register
			 ISB						; Instruction Synchronisation Barrier
			 BX  LR


appearAsRanTask:

		    MOV R1, LR						; temp LR
											; set these values to make it appear as the return values when
											; the hardware pops the registers from the stack

			MRS R2, XPSR					; LOAD XPSR
			ORR R2, R2, #16777216			; set the thumb bit
			STR R2,  [R0, #-4]			    ; XPSR

			STR R12,  [R0, #-8]				; PC

			LDR LR,  GOTO_PSP_AFTER			; exeption allowing to break from the exception
											; since values r0-r3, r12, lr, pc, xPSR
											; are all set in place in memory, it appears as the values
											; when the other 8 registers are popped by the hardware
											; all the remaining values is what is refered to in the memory
			STR R4,  [R0, #-12]    			; LR
			STR R12, [R0, #-16]    			; R12
			STR R3,  [R0, #-20]    			; R3
 			STR R2,  [R0, #-24]   			; R2
 			STR R1,  [R0, #-28]   			; R1
			STR R0,  [R0, #-32]    	    	; R0

		    SUB R0, R0, #32					; move the stack pointer to point to where

											; temp values are set to see how the values stack up when pushed
			MOV R4, #4						; UPDATE R4
			MOV R5, #5						; UPDATE R5
			MOV R6, #6						; UPDATE R6
			MOV R7, #7						; UPDATE R7
			MOV R8, #8						; UPDATE R8
			MOV R9, #9						; UPDATE R9
			MOV R10, #10					; UPDATE R10
			MOV R11, #11					; UPDATE R11

			STR R4,  [R0, #-4]    			; LR
			STR R5,  [R0, #-8]    			; R12
			STR R6,  [R0, #-12]    			; R3
 			STR R7,  [R0, #-16]   			; R2
 			STR R8,  [R0, #-20]   			; R1
			STR R9,  [R0, #-24]    	    	; R0
			STR R10, [R0, #-28]    			; LR
			STR R11, [R0, #-32]    			; R12
			STR LR,  [R0, #-36]    			; R3

			SUB R0, R0, #36					; 9 REGISTERS X 4 BYTES EACH

			MSR PSP, R0						;NEW PSP
			ISB

			MOV LR, R1	 					;LR

			BX LR							;go back to where left off in program

runFirstTask:

											;pop all those values from the top of the stack

			LDR LR,  [R0]					; LR exception stored in memory
			LDR R11, [R0,#4]
			LDR R10, [R0,#8]
			LDR R9,  [R0,#12]
			LDR R8,  [R0,#16]
			LDR R7,  [R0,#20]
			LDR R6,  [R0,#24]
			LDR R5,  [R0,#28]
			LDR R4,  [R0,#32]

			ADD R0, R0, #36					; 9 REGISTERS X 4 BYTES EACH

			MSR PSP, R0						; new psp set to last popped value
			ISB


		    BX  LR


saveR4toR11:

		    MOV R1, LR						; temp LR
											; set these values to make it appear as the return values when
											; the hardware pops the registers from the stack

		    LDR LR,  GOTO_PSP_AFTER
			STR R4,  [R0, #-4]    			; R4
			STR R5,  [R0, #-8]    			; R5
			STR R6,  [R0, #-12]    			; R6
 			STR R7,  [R0, #-16]   			; R7
 			STR R8,  [R0, #-20]   			; R8
			STR R9,  [R0, #-24]    	    	; R9
			STR R10, [R0, #-28]    			; R10
			STR R11, [R0, #-32]    			; R11
			STR LR,  [R0, #-36]    			; LR

											;push all values to the stack

			SUB R0, R0, #36					; 9 REGISTERS X 4 BYTES EACH

			MSR PSP, R0						;NEW PSP
			ISB

			MOV LR, R1	 					;LR

			BX LR							;go back to where left off in program							;go back to where left off in program


getSVCallNumber:						; SERVICE CALL NUMBER IS ENCODED INTO THE INSTRUCTION
			MRS R0 , PSP 				; move the PSP registister into R0
    		LDR     R0, [R0, #24]       ; Load the return address	aka PC of previous insruction
    		LDRB    R1, [R0, #-2]       ; Get the SVC number			Load Register with byte offset of 2
    		MOV 	R0, R1				; make sure to get the return value
			BX LR

taskSwitch:
			MOV SP, R0						;pop all those values from the top of the stack

			LDR LR,  [R0]
			LDR R11, [R0,#4]
			LDR R10, [R0,#8]
			LDR R9,  [R0,#12]
			LDR R8,  [R0,#16]
			LDR R7,  [R0,#20]
			LDR R6,  [R0,#24]
			LDR R5,  [R0,#28]
			LDR R4,  [R0,#32]

			ADD R0, R0, #36					; 9 REGISTERS X 4 BYTES EACH

			MSR PSP, R0						; new psp set to last popped value
			ISB

			LDR LR,  GOTO_PSP_AFTER			; exeption allowing to break from the exception
											; since values r0-r3, r12, lr, pc, xPSR
											; are all set in place in memory, it appears as the values
											; when the other 8 registers are popped by the hardware
											; all the remaining values is what is refered to in the memory
			 	 							; Instruction Synchronisation Barrier
		    BX  LR

returnR0:
			BX LR							; just return whats already in R0
returnR3:
			MOV R0, R3
			BX LR							; just return whats already in R0

returnR12:
			MOV R0, R12
			BX LR							; just return whats already in R0

gobacktoPSP:
			LDR LR,  GOTO_PSP_AFTER
			BX LR


appearAsRanTaskAgain:

		    MOV R1, LR						; temp LR
											; set these values to make it appear as the return values when
											; the hardware pops the registers from the stack
			STR R3,  [R0, #-8]				; PC

			MOV R2, #0						; xPSR
			ORR R2, R2, #16777216			; set the thumb bit
			STR R2,  [R0, #-4]			    ; XPSR


			LDR LR,  GOTO_PSP_AFTER			; exeption allowing to break from the exception
											; since values r0-r3, r12, lr, pc, xPSR
											; are all set in place in memory, it appears as the values
											; when the other 8 registers are popped by the hardware
											; all the remaining values is what is refered to in the memory
			STR R4,  [R0, #-12]    			; LR
			STR R12, [R0, #-16]    			; R12
			STR R3,  [R0, #-20]    			; R3
 			STR R2,  [R0, #-24]   			; R2
 			STR R1,  [R0, #-28]   			; R1
			STR R0,  [R0, #-32]    	    	; R0

		    SUB R0, R0, #32					; move the stack pointer to point to where

											; temp values are set to see how the values stack up when pushed
			MOV R4, #4						; UPDATE R4
			MOV R5, #5						; UPDATE R5
			MOV R6, #6						; UPDATE R6
			MOV R7, #7						; UPDATE R7
			MOV R8, #8						; UPDATE R8
			MOV R9, #9						; UPDATE R9
			MOV R10, #10					; UPDATE R10
			MOV R11, #11					; UPDATE R11

			STR R4,  [R0, #-4]    			; LR
			STR R5,  [R0, #-8]    			; R12
			STR R6,  [R0, #-12]    			; R3
 			STR R7,  [R0, #-16]   			; R2
 			STR R8,  [R0, #-20]   			; R1
			STR R9,  [R0, #-24]    	    	; R0
			STR R10, [R0, #-28]    			; LR
			STR R11, [R0, #-32]    			; R12
			STR LR,  [R0, #-36]    			; R3

			SUB R0, R0, #36					; 9 REGISTERS X 4 BYTES EACH

			MOV LR, R1	 					;LR

			BX LR							;go back to where left off in program
