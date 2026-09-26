/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * PROJECT PORT (Wokwi): replaces the library's ARM_CM3 port.c (together with
 * include/portmacro.h). Linked instead of the library's copy; see
 * lib/freertos_port_patch/library.json. docs/dev-log.md records the probes
 * behind every decision below.
 *
 * Why a different port: Wokwi's Cortex-M3 model deviates from ARMv7-M:
 *   - an exception pended by a store to ICSR returns to that store, so the
 *     stock PendSV-based context switch loops forever;
 *   - BASEPRI is not implemented and PRIMASK/FAULTMASK do not block SysTick,
 *     so no CPU mask can keep SysTick out of a critical section;
 *   - cpsie sets PRIMASK/FAULTMASK instead of clearing them.
 * What does work, and is used here: SVC and SysTick exceptions (correct
 * return addresses, including returning onto a task's PSP), MSR to
 * PRIMASK/FAULTMASK, gating SysTick with SysTick->CTRL.TICKINT, COUNTFLAG, and
 * the DWT cycle counter.
 *
 * Design:
 *   - PendSV is never used. A yield switches context synchronously inside the
 *     SVC handler (`svc 1`); a tick switches context synchronously inside the
 *     SysTick handler.
 *   - A critical section gates SysTick at its source (TICKINT = 0). On leaving
 *     the outermost one, the SysTick wraps that fell inside it are computed
 *     from the DWT cycles elapsed and SysTick's position (VAL) at entry, and
 *     caught up with xTaskIncrementTick() BEFORE SysTick is re-enabled, so the
 *     catch-up cannot race the tick handler. A switch it asks for is then done
 *     with `svc 1`.
 *   - Every SysTick-gated window reads SysTick->CTRL once before ungating, and
 *     the tick handler reads it on entry, so COUNTFLAG only ever reports wraps
 *     that nobody has accounted for.
 *   - Assumption: SysTick is the only interrupt that calls the kernel, so the
 *     "from ISR" interrupt mask is a no-op.
 *
 * Known residual error: a SysTick wrap that lands in the few instructions
 * between gating and reading VAL at critical-section entry, or between the
 * final cycle-counter read and ungating at exit, can make the tick count
 * lose or gain one tick. It never corrupts kernel state.
 *
 * On real hardware this port is also correct, but the stock port is simpler
 * and should be preferred there.
 */

#include "FreeRTOS.h"
#include "task.h"

#ifndef configSYSTICK_CLOCK_HZ
	#define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
	/* Ensure the SysTick is clocked at the same frequency as the core. */
	#define portNVIC_SYSTICK_CLK_BIT	( 1UL << 2UL )
#else
	#define portNVIC_SYSTICK_CLK_BIT	( 0 )
#endif

#ifndef configKERNEL_INTERRUPT_PRIORITY
	#define configKERNEL_INTERRUPT_PRIORITY 255
#endif

/* SysTick registers and bits. */
#define portNVIC_SYSTICK_CTRL_REG			( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG			( * ( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG	( * ( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SYSTICK_INT_BIT			( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT			( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT		( 1UL << 16UL )

/* SysTick->CTRL values: counting with the tick interrupt gated / enabled. A
write never clears COUNTFLAG (only a read does). */
#define portSYSTICK_CTRL_GATED	( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_ENABLE_BIT )
#define portSYSTICK_CTRL_ON		( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT )

/* Priorities of PendSV/SysTick, as the stock port sets them (Wokwi ignores
exception priorities, but real hardware should keep SysTick lowest). */
#define portNVIC_SYSPRI2_REG				( * ( ( volatile uint32_t * ) 0xe000ed20 ) )
#define portNVIC_PENDSV_PRI					( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI				( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/* DWT cycle counter, used to measure how long SysTick was gated. */
#define portDEMCR_REG						( * ( ( volatile uint32_t * ) 0xe000edfc ) )
#define portDEMCR_TRCENA_BIT				( 1UL << 24UL )
#define portDWT_CTRL_REG					( * ( ( volatile uint32_t * ) 0xe0001000 ) )
#define portDWT_CYCCNTENA_BIT				( 1UL << 0UL )
#define portDWT_CYCCNT_REG					( * ( ( volatile uint32_t * ) 0xe0001004 ) )

/* Constants required to set up the initial stack. */
#define portINITIAL_XPSR					( 0x01000000UL )

/* For strict compliance with the Cortex-M spec the task start address should
have bit-0 clear, as it is loaded into the PC on exit from an ISR. */
#define portSTART_ADDRESS_MASK				( ( StackType_t ) 0xfffffffeUL )

#ifdef configTASK_RETURN_ADDRESS
	#define portTASK_RETURN_ADDRESS	configTASK_RETURN_ADDRESS
#else
	#define portTASK_RETURN_ADDRESS	prvTaskExitError
#endif

void vPortSetupTimerInterrupt( void );
void SVC_Handler( void ) __attribute__ (( naked ));
void SysTick_Handler( void ) __attribute__ (( naked ));
void vPortTickFromTask( void );
void vPortTickNotFromTask( void );
void vPortSwitchFromSvc( void );
static void prvPortStartFirstTask( void ) __attribute__ (( naked ));
static void prvTaskExitError( void );

#if( configUSE_TICK_HOOK == 1 )
	extern void vApplicationTickHook( void );
#endif
/*-----------------------------------------------------------*/

/* Nesting depth of task-level critical sections. Starts non-zero so that
kernel calls made before the scheduler starts leave SysTick gated until the
first task runs; set to 0 in xPortStartScheduler(). */
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

/* SysTick VAL and DWT cycle count when the outermost critical section began. */
static uint32_t ulCriticalEntryVal = 0;
static uint32_t ulCriticalEntryCycles = 0;

/* Set when a switch was requested but could not be done at that moment
(inside a critical section, or from an interrupt); performed at the next
critical-section exit or tick. */
static volatile uint32_t ulYieldPending = 0;
/*-----------------------------------------------------------*/

StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
	/* Simulate the stack frame as it would be created by a context switch
	interrupt. */
	pxTopOfStack--; /* Offset added to account for the way the MCU uses the stack on entry/exit of interrupts. */
	*pxTopOfStack = portINITIAL_XPSR;	/* xPSR */
	pxTopOfStack--;
	*pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK;	/* PC */
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS;	/* LR */
	pxTopOfStack -= 5;	/* R12, R3, R2 and R1. */
	*pxTopOfStack = ( StackType_t ) pvParameters;	/* R0 */
	pxTopOfStack -= 8;	/* R11, R10, R9, R8, R7, R6, R5 and R4. */

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
volatile uint32_t ulDummy = 0UL;

	/* A function that implements a task must not exit or attempt to return to
	its caller as there is nothing to return to.  If a task wants to exit it
	should instead call vTaskDelete( NULL ). */
	configASSERT( uxCriticalNesting == ~0UL );
	portDISABLE_INTERRUPTS();
	while( ulDummy == 0 )
	{
		/* ulDummy is volatile so the compiler cannot assume this never returns. */
	}
}
/*-----------------------------------------------------------*/

void vPortGateTick( void )
{
	portNVIC_SYSTICK_CTRL_REG = portSYSTICK_CTRL_GATED;
	__asm volatile( "dsb" ::: "memory" );
	__asm volatile( "isb" );
}
/*-----------------------------------------------------------*/

void vPortUngateTick( void )
{
	portNVIC_SYSTICK_CTRL_REG = portSYSTICK_CTRL_ON;
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
	/* Gate first: from here on SysTick cannot run, so nothing below can be
	interrupted by the tick. */
	vPortGateTick();

	if( uxCriticalNesting == 0 )
	{
		ulCriticalEntryVal = portNVIC_SYSTICK_CURRENT_VALUE_REG;
		ulCriticalEntryCycles = portDWT_CYCCNT_REG;
	}
	uxCriticalNesting++;
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
	configASSERT( uxCriticalNesting );
	uxCriticalNesting--;

	if( uxCriticalNesting == 0 )
	{
	uint32_t ulNowCycles, ulPeriod, ulMissed;
	BaseType_t xSwitchRequired = pdFALSE;

		/* Still gated. Clear COUNTFLAG: the wraps in this window are counted
		exactly below, so the flag must not be counted again later. */
		ulNowCycles = portDWT_CYCCNT_REG;
		( void ) portNVIC_SYSTICK_CTRL_REG;

		/* SysTick wraps inside the window: cycles since the last wrap before
		entry (LOAD - VAL at entry), plus the cycles elapsed, in whole periods. */
		ulPeriod = portNVIC_SYSTICK_LOAD_REG + 1UL;
		ulMissed = ( ( ulPeriod - 1UL - ulCriticalEntryVal ) + ( ulNowCycles - ulCriticalEntryCycles ) ) / ulPeriod;

		/* Catch up while SysTick is still gated, so the tick handler cannot
		run at the same time. */
		while( ulMissed > 0UL )
		{
			if( xTaskIncrementTick() != pdFALSE )
			{
				xSwitchRequired = pdTRUE;
			}
			ulMissed--;
		}

		vPortUngateTick();

		if( ( xSwitchRequired != pdFALSE ) || ( ulYieldPending != 0UL ) )
		{
			ulYieldPending = 0UL;
			__asm volatile( "svc 1" ::: "memory" );
		}
	}
}
/*-----------------------------------------------------------*/

void vPortYield( void )
{
	if( uxCriticalNesting == 0 )
	{
		/* Switch now, synchronously, in the SVC handler. */
		__asm volatile( "svc 1" ::: "memory" );
	}
	else
	{
		/* Inside a critical section: switch when it ends. */
		ulYieldPending = 1UL;
	}
}
/*-----------------------------------------------------------*/

void vPortYieldFromISR( void )
{
	ulYieldPending = 1UL;
}
/*-----------------------------------------------------------*/

/* Called from SysTick_Handler when the tick interrupted a task; that task's
context is already saved and pxCurrentTCB may be changed. */
void vPortTickFromTask( void )
{
	if( ( xTaskIncrementTick() != pdFALSE ) || ( ulYieldPending != 0UL ) )
	{
		ulYieldPending = 0UL;
		vTaskSwitchContext();
	}
}
/*-----------------------------------------------------------*/

/* Called from SysTick_Handler when no task was interrupted: before the
scheduler starts (only the application tick hook runs, which keeps the HAL
tick going), or if the tick nested inside another handler (the switch is
deferred). */
void vPortTickNotFromTask( void )
{
	if( xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED )
	{
		#if( configUSE_TICK_HOOK == 1 )
		{
			vApplicationTickHook();
		}
		#endif
	}
	else if( xTaskIncrementTick() != pdFALSE )
	{
		ulYieldPending = 1UL;
	}
}
/*-----------------------------------------------------------*/

/* Called from SVC_Handler (svc 1) with SysTick gated and the yielding task's
context saved. */
void vPortSwitchFromSvc( void )
{
	ulYieldPending = 0UL;
	vTaskSwitchContext();

	/* The switch takes far less than one tick, so COUNTFLAG (one bit) is
	enough to catch a wrap that fell inside it. Reading it also clears it. */
	if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0UL )
	{
		if( xTaskIncrementTick() != pdFALSE )
		{
			vTaskSwitchContext();
		}
	}

	vPortUngateTick();
}
/*-----------------------------------------------------------*/

/* svc 0: start the first task. svc 1: synchronous yield from a task. The
frame's stack is chosen with branches, not an IT block (Wokwi executes a
conditional MRS even when its condition is false). */
void SVC_Handler( void )
{
	__asm volatile
	(
	"	tst lr, #4						\n"
	"	bne 1f							\n"
	"	mrs r0, msp						\n"
	"	b 2f							\n"
	"1:	mrs r0, psp						\n"
	"2:	ldr r1, [r0, #24]				\n" /* Stacked PC. */
	"	ldrb r1, [r1, #-2]				\n" /* The svc immediate. */
	"	cmp r1, #1						\n"
	"	beq 4f							\n"
	"	cmp r1, #0						\n"
	"	beq 3f							\n"
	"	bx lr							\n"
	"3:									\n" /* svc 0: start the first task. */
	"	ldr r3, =pxCurrentTCB			\n"
	"	ldr r1, [r3]					\n"
	"	ldr r0, [r1]					\n" /* First item in the TCB is the top of stack. */
	"	ldmia r0!, {r4-r11}				\n"
	"	msr psp, r0						\n"
	"	isb								\n"
	"	ldr r1, =0xe000e010				\n"
	"	mov r0, %0						\n" /* SysTick on: ticks start now. */
	"	str r0, [r1]					\n"
	"	orr lr, lr, #13					\n" /* Return to thread mode on PSP. */
	"	bx lr							\n"
	"4:									\n" /* svc 1: yield, only from a task on PSP. */
	"	mvn r1, #2						\n"
	"	cmp lr, r1						\n"
	"	bne 5f							\n"
	"	ldr r1, =0xe000e010				\n"
	"	mov r0, %1						\n" /* Gate SysTick for the switch. */
	"	str r0, [r1]					\n"
	"	mrs r0, psp						\n"
	"	isb								\n"
	"	ldr r3, =pxCurrentTCB			\n"
	"	ldr r2, [r3]					\n"
	"	stmdb r0!, {r4-r11}				\n"
	"	str r0, [r2]					\n"
	"	stmdb sp!, {r3, r14}			\n"
	"	bl vPortSwitchFromSvc			\n" /* Switches and ungates SysTick. */
	"	ldmia sp!, {r3, r14}			\n"
	"	ldr r1, [r3]					\n"
	"	ldr r0, [r1]					\n"
	"	ldmia r0!, {r4-r11}				\n"
	"	msr psp, r0						\n"
	"	isb								\n"
	"5:	bx lr							\n"
	"	.ltorg							\n"
	:: "i" ( portSYSTICK_CTRL_ON ), "i" ( portSYSTICK_CTRL_GATED )
	);
}
/*-----------------------------------------------------------*/

/* The tick. Reading CTRL first clears COUNTFLAG, so afterwards the flag only
reports wraps this handler did not see. If a task was interrupted (thread mode
on PSP), its context is saved and the tick may switch to another task. */
void SysTick_Handler( void )
{
	__asm volatile
	(
	"	ldr r0, =0xe000e010				\n"
	"	ldr r0, [r0]					\n" /* Read CTRL: clears COUNTFLAG. */
	"	mvn r1, #2						\n"
	"	cmp lr, r1						\n" /* Interrupted a task? */
	"	bne 1f							\n"
	"	mrs r0, psp						\n"
	"	isb								\n"
	"	ldr r3, =pxCurrentTCB			\n"
	"	ldr r2, [r3]					\n"
	"	stmdb r0!, {r4-r11}				\n"
	"	str r0, [r2]					\n"
	"	stmdb sp!, {r3, r14}			\n"
	"	bl vPortTickFromTask			\n"
	"	ldmia sp!, {r3, r14}			\n"
	"	ldr r1, [r3]					\n"
	"	ldr r0, [r1]					\n"
	"	ldmia r0!, {r4-r11}				\n"
	"	msr psp, r0						\n"
	"	isb								\n"
	"	bx r14							\n"
	"1:	b vPortTickNotFromTask			\n"
	"	.ltorg							\n"
	);
}
/*-----------------------------------------------------------*/

static void prvPortStartFirstTask( void )
{
	__asm volatile(
					" ldr r0, =0xE000ED08 	\n" /* Use the NVIC offset register to locate the stack. */
					" ldr r0, [r0] 			\n"
					" ldr r0, [r0] 			\n"
					" msr msp, r0			\n" /* Set the msp back to the start of the stack. */
					" cpsie i				\n" /* Globally enable interrupts. */
					" cpsie f				\n"
					" dsb					\n"
					" isb					\n"
					/* Wokwi workaround: its cpsie sets PRIMASK/FAULTMASK instead of
					clearing them, which blocks the svc below. MSR clears them; on
					real hardware this is a no-op. */
					" movs r0, #0			\n"
					" msr primask, r0		\n"
					" msr faultmask, r0		\n"
					" svc 0					\n" /* System call to start first task. */
					" nop					\n"
					" .ltorg				\n"
				);
}
/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
	/* Lowest priority for PendSV/SysTick, as in the stock port. */
	portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;

	/* The DWT cycle counter measures how long SysTick stays gated. */
	portDEMCR_REG |= portDEMCR_TRCENA_BIT;
	portDWT_CTRL_REG |= portDWT_CYCCNTENA_BIT;

	/* SysTick counts from here, gated until the first task starts. */
	vPortSetupTimerInterrupt();

	uxCriticalNesting = 0;

	prvPortStartFirstTask();

	/* Should never get here. vTaskSwitchContext() is referenced so link-time
	optimisation does not remove it. */
	vTaskSwitchContext();
	prvTaskExitError();

	return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* Not implemented in ports where there is nothing to return to. */
	configASSERT( uxCriticalNesting == 1000UL );
}
/*-----------------------------------------------------------*/

__attribute__(( weak )) void vPortSetupTimerInterrupt( void )
{
	portNVIC_SYSTICK_CTRL_REG = 0UL;
	portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
	portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
	portNVIC_SYSTICK_CTRL_REG = portSYSTICK_CTRL_GATED;
}
/*-----------------------------------------------------------*/
