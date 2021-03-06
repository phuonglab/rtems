/**
 *  @file
 *
 *  @brief SMP Support
 *  @ingroup Score
 */

/*
 *  COPYRIGHT (c) 1989-2011.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/score/smpimpl.h>
#include <rtems/score/assert.h>
#include <rtems/score/schedulerimpl.h>
#include <rtems/score/threadimpl.h>
#include <rtems/config.h>

#if CPU_USE_DEFERRED_FP_SWITCH == TRUE
  #error "deferred FP switch not implemented for SMP"
#endif

Processor_mask _SMP_Online_processors;

uint32_t _SMP_Processor_count;

static void _SMP_Start_processors( uint32_t cpu_count )
{
  uint32_t cpu_index_self = _SMP_Get_current_processor();
  uint32_t cpu_index;

  for ( cpu_index = 0 ; cpu_index < cpu_count; ++cpu_index ) {
    const Scheduler_Assignment *assignment =
      _Scheduler_Get_assignment( cpu_index );
    Per_CPU_Control *cpu = _Per_CPU_Get_by_index( cpu_index );
    bool started;

    if ( cpu_index != cpu_index_self ) {
      if ( _Scheduler_Should_start_processor( assignment ) ) {
        started = _CPU_SMP_Start_processor( cpu_index );

        if ( !started && _Scheduler_Is_mandatory_processor( assignment ) ) {
          _SMP_Fatal( SMP_FATAL_START_OF_MANDATORY_PROCESSOR_FAILED );
        }
      } else {
        started = false;
      }
    } else {
      started = true;

      cpu->boot = true;

      if ( !_Scheduler_Should_start_processor( assignment ) ) {
        _SMP_Fatal( SMP_FATAL_BOOT_PROCESSOR_NOT_ASSIGNED_TO_SCHEDULER );
      }
    }

    cpu->online = started;

    if ( started ) {
      Scheduler_Context *context =
        _Scheduler_Get_context( assignment->scheduler );

      ++context->processor_count;
      cpu->scheduler_context = context;

      _Processor_mask_Set( _SMP_Online_processors, cpu_index );
    }
  }
}

void _SMP_Handler_initialize( void )
{
  uint32_t cpu_max = rtems_configuration_get_maximum_processors();
  uint32_t cpu_count;
  uint32_t cpu_index;

  for ( cpu_index = 0 ; cpu_index < cpu_max; ++cpu_index ) {
    Per_CPU_Control *cpu = _Per_CPU_Get_by_index( cpu_index );

    _ISR_lock_Initialize( &cpu->Watchdog.Lock, "Watchdog" );
    _SMP_ticket_lock_Initialize( &cpu->Lock );
    _SMP_lock_Stats_initialize( &cpu->Lock_stats, "Per-CPU" );
    _Chain_Initialize_empty( &cpu->Threads_in_need_for_help );
  }

  /*
   * Discover and initialize the secondary cores in an SMP system.
   */

  cpu_count = _CPU_SMP_Initialize();
  cpu_count = cpu_count < cpu_max ? cpu_count : cpu_max;
  _SMP_Processor_count = cpu_count;

  for ( cpu_index = cpu_count ; cpu_index < cpu_max; ++cpu_index ) {
    const Scheduler_Assignment *assignment =
      _Scheduler_Get_assignment( cpu_index );

    if ( _Scheduler_Is_mandatory_processor( assignment ) ) {
      _SMP_Fatal( SMP_FATAL_MANDATORY_PROCESSOR_NOT_PRESENT );
    }
  }

  _SMP_Start_processors( cpu_count );

  _CPU_SMP_Finalize_initialization( cpu_count );
}

void _SMP_Request_start_multitasking( void )
{
  Per_CPU_Control *self_cpu = _Per_CPU_Get();
  uint32_t cpu_count = _SMP_Get_processor_count();
  uint32_t cpu_index;

  _Per_CPU_State_change( self_cpu, PER_CPU_STATE_READY_TO_START_MULTITASKING );

  for ( cpu_index = 0 ; cpu_index < cpu_count ; ++cpu_index ) {
    Per_CPU_Control *cpu = _Per_CPU_Get_by_index( cpu_index );

    if ( _Per_CPU_Is_processor_online( cpu ) ) {
      _Per_CPU_State_change( cpu, PER_CPU_STATE_REQUEST_START_MULTITASKING );
    }
  }
}

bool _SMP_Should_start_processor( uint32_t cpu_index )
{
  const Scheduler_Assignment *assignment =
    _Scheduler_Get_assignment( cpu_index );

  return _Scheduler_Should_start_processor( assignment );
}

void _SMP_Start_multitasking_on_secondary_processor( void )
{
  Per_CPU_Control *self_cpu = _Per_CPU_Get();
  uint32_t cpu_index_self = _Per_CPU_Get_index( self_cpu );

  if ( cpu_index_self >= rtems_configuration_get_maximum_processors() ) {
    _SMP_Fatal( SMP_FATAL_MULTITASKING_START_ON_INVALID_PROCESSOR );
  }

  if ( !_SMP_Should_start_processor( cpu_index_self ) ) {
    _SMP_Fatal( SMP_FATAL_MULTITASKING_START_ON_UNASSIGNED_PROCESSOR );
  }

  _Per_CPU_State_change( self_cpu, PER_CPU_STATE_READY_TO_START_MULTITASKING );

  _Thread_Start_multitasking();
}

void _SMP_Request_shutdown( void )
{
  ISR_Level level;

  _ISR_Local_disable( level );
  (void) level;

  _Per_CPU_State_change( _Per_CPU_Get(), PER_CPU_STATE_SHUTDOWN );
}

void _SMP_Send_message( uint32_t cpu_index, unsigned long message )
{
  Per_CPU_Control *cpu = _Per_CPU_Get_by_index( cpu_index );

  _Atomic_Fetch_or_ulong( &cpu->message, message, ATOMIC_ORDER_RELEASE );

  _CPU_SMP_Send_interrupt( cpu_index );
}

void _SMP_Send_message_broadcast( unsigned long message )
{
  uint32_t cpu_count = _SMP_Get_processor_count();
  uint32_t cpu_index_self = _SMP_Get_current_processor();
  uint32_t cpu_index;

  _Assert( _Debug_Is_thread_dispatching_allowed() );

  for ( cpu_index = 0 ; cpu_index < cpu_count ; ++cpu_index ) {
    if (
      cpu_index != cpu_index_self
        && _Processor_mask_Is_set( _SMP_Online_processors, cpu_index )
    ) {
      _SMP_Send_message( cpu_index, message );
    }
  }
}

void _SMP_Send_message_multicast(
    const size_t setsize,
    const cpu_set_t *cpus,
    unsigned long message
)
{
  uint32_t cpu_count = _SMP_Get_processor_count();
  uint32_t cpu_index;

  for ( cpu_index = 0 ; cpu_index < cpu_count ; ++cpu_index ) {
    if ( CPU_ISSET_S( cpu_index, setsize, cpus ) ) {
      _SMP_Send_message( cpu_index, message );
    }
  }
}

bool _SMP_Before_multitasking_action_broadcast(
  SMP_Action_handler  handler,
  void               *arg
)
{
  bool done = true;
  uint32_t cpu_count = _SMP_Get_processor_count();
  uint32_t cpu_index;

  for ( cpu_index = 0 ; done && cpu_index < cpu_count ; ++cpu_index ) {
    Per_CPU_Control *cpu = _Per_CPU_Get_by_index( cpu_index );

    if (
      !_Per_CPU_Is_boot_processor( cpu )
        && _Per_CPU_Is_processor_online( cpu )
    ) {
      done = _SMP_Before_multitasking_action( cpu, handler, arg );
    }
  }

  return done;
}

SMP_Test_message_handler _SMP_Test_message_handler;
