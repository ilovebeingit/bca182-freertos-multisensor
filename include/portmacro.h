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
 * PROJECT PORT (Wokwi): replaces the library's ARM_CM3 portmacro.h. This
 * directory (include/) comes before the library's port directory on the
 * kernel's include path, so tasks.c, queue.c and event_groups.c compile
 * against this file. Only the scheduling and critical-section macros differ
 * from the original; see lib/freertos_port_patch/src/port.c and
 * docs/dev-log.md for why:
 *   - portYIELD() no longer pends PendSV with a store to ICSR (Wokwi returns
 *     to that store and loops). It calls vPortYield(), which switches context
 *     synchronously in the SVC handler, or defers the switch to the end of
 *     the current critical section.
 *   - Critical sections no longer raise BASEPRI (not implemented in Wokwi).
 *     They gate the SysTick interrupt at its source (SysTick->CTRL.TICKINT)
 *     and recover the ticks that fell inside the window when they end.
 */

#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Type definitions (unchanged). */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	uint32_t
#define portBASE_TYPE	long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#if( configUSE_16_BIT_TICKS == 1 )
	typedef uint16_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffff
#else
	typedef uint32_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffffffffUL

	/* 32-bit tick type on a 32-bit architecture, so reads of the tick count do
	not need to be guarded with a critical section. */
	#define portTICK_TYPE_IS_ATOMIC 1
#endif
/*-----------------------------------------------------------*/

/* Architecture specifics (unchanged). */
#define portSTACK_GROWTH			( -1 )
#define portTICK_PERIOD_MS			( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT			8
/*-----------------------------------------------------------*/

/* Scheduler utilities (PROJECT PORT). A yield from a task switches context
synchronously through `svc 1`, or is deferred to the end of the current
critical section. From an interrupt it only records that a switch is wanted;
the tick handler or the next critical-section exit performs it. */
extern void vPortYield( void );
extern void vPortYieldFromISR( void );
#define portYIELD()									vPortYield()
#define portEND_SWITCHING_ISR( xSwitchRequired )	if( ( xSwitchRequired ) != pdFALSE ) vPortYieldFromISR()
#define portYIELD_FROM_ISR( x )						portEND_SWITCHING_ISR( x )

/* Only so the library's own (unlinked) ARM_CM3 port.c still compiles in its
archive; nothing linked into the firmware pends PendSV. */
#define portNVIC_INT_CTRL_REG		( * ( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT		( 1UL << 28UL )
/*-----------------------------------------------------------*/

/* Critical section management (PROJECT PORT). SysTick is the only interrupt
that uses the kernel, and it cannot preempt itself, so the "from ISR" mask is
a no-op. Task-level critical sections gate SysTick at its source. */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
extern void vPortGateTick( void );
extern void vPortUngateTick( void );
#define portSET_INTERRUPT_MASK_FROM_ISR()		( 0UL )
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )	( ( void ) ( x ) )
#define portDISABLE_INTERRUPTS()				vPortGateTick()
#define portENABLE_INTERRUPTS()					vPortUngateTick()
#define portENTER_CRITICAL()					vPortEnterCritical()
#define portEXIT_CRITICAL()						vPortExitCritical()
/*-----------------------------------------------------------*/

/* Task function macros (unchanged). */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )
/*-----------------------------------------------------------*/

/* Tickless idle is not supported by this port (configUSE_TICKLESS_IDLE 0),
and the clz-based task selection is not used (FreeRTOSConfig.h sets
configUSE_PORT_OPTIMISED_TASK_SELECTION to 0). */
#if( configUSE_TICKLESS_IDLE == 1 )
	#error The Wokwi project port does not support tickless idle.
#endif
#if( configUSE_PORT_OPTIMISED_TASK_SELECTION == 1 )
	#error The Wokwi project port expects configUSE_PORT_OPTIMISED_TASK_SELECTION 0.
#endif
/*-----------------------------------------------------------*/

/* portNOP() is not required by this port. */
#define portNOP()

#define portINLINE	__inline

#ifndef portFORCE_INLINE
	#define portFORCE_INLINE inline __attribute__(( always_inline))
#endif

portFORCE_INLINE static BaseType_t xPortIsInsideInterrupt( void )
{
uint32_t ulCurrentInterrupt;
BaseType_t xReturn;

	/* Obtain the number of the currently executing interrupt. */
	__asm volatile( "mrs %0, ipsr" : "=r"( ulCurrentInterrupt ) :: "memory" );

	if( ulCurrentInterrupt == 0 )
	{
		xReturn = pdFALSE;
	}
	else
	{
		xReturn = pdTRUE;
	}

	return xReturn;
}

#define portMEMORY_BARRIER() __asm volatile( "" ::: "memory" )

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */
