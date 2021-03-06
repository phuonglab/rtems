/**
 *  @file  rtems/score/scheduler.h
 *
 *  @brief Constants and Structures Associated with the Scheduler
 *
 *  This include file contains all the constants and structures associated
 *  with the scheduler.
 */

/*
 *  Copyright (C) 2010 Gedare Bloom.
 *  Copyright (C) 2011 On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_SCHEDULER_H
#define _RTEMS_SCORE_SCHEDULER_H

#include <rtems/score/thread.h>
#if defined(__RTEMS_HAVE_SYS_CPUSET_H__) && defined(RTEMS_SMP)
  #include <sys/cpuset.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct Per_CPU_Control;

/**
 *  @defgroup ScoreScheduler Scheduler Handler
 *
 *  @ingroup Score
 *
 *  This handler encapsulates functionality related to managing sets of threads
 *  that are ready for execution.
 */
/**@{*/

typedef struct Scheduler_Control Scheduler_Control;

#if defined(RTEMS_SMP)
  typedef bool Scheduler_Void_or_bool;

  #define SCHEDULER_RETURN_VOID_OR_BOOL return false
#else
  typedef void Scheduler_Void_or_bool;

  #define SCHEDULER_RETURN_VOID_OR_BOOL return
#endif

/**
 * @brief The scheduler operations.
 */
typedef struct {
  /** @see _Scheduler_Handler_initialization() */
  void ( *initialize )( const Scheduler_Control * );

  /** @see _Scheduler_Schedule() */
  void ( *schedule )( const Scheduler_Control *, Thread_Control *);

  /** @see _Scheduler_Yield() */
  Scheduler_Void_or_bool ( *yield )(
    const Scheduler_Control *,
    Thread_Control *,
    Scheduler_Node *
  );

  /** @see _Scheduler_Block() */
  void ( *block )(
    const Scheduler_Control *,
    Thread_Control *,
    Scheduler_Node *
  );

  /** @see _Scheduler_Unblock() */
  Scheduler_Void_or_bool ( *unblock )(
    const Scheduler_Control *,
    Thread_Control *,
    Scheduler_Node *
  );

  /** @see _Scheduler_Update_priority() */
  void ( *update_priority )(
    const Scheduler_Control *,
    Thread_Control *,
    Scheduler_Node *
  );

  /** @see _Scheduler_Map_priority() */
  Priority_Control ( *map_priority )(
    const Scheduler_Control *,
    Priority_Control
  );

  /** @see _Scheduler_Unmap_priority() */
  Priority_Control ( *unmap_priority )(
    const Scheduler_Control *,
    Priority_Control
  );

#if defined(RTEMS_SMP)
  /**
   * @brief Ask for help operation.
   *
   * @param[in] scheduler The scheduler instance to ask for help.
   * @param[in] the_thread The thread needing help.
   * @param[in] node The scheduler node.
   *
   * @retval true Ask for help was successful.
   * @retval false Otherwise.
   */
  bool ( *ask_for_help )(
    const Scheduler_Control *scheduler,
    Thread_Control          *the_thread,
    Scheduler_Node          *node
  );

  /**
   * @brief Reconsider help operation.
   *
   * @param[in] scheduler The scheduler instance to reconsider the help
   *   request.
   * @param[in] the_thread The thread reconsidering a help request.
   * @param[in] node The scheduler node.
   */
  void ( *reconsider_help_request )(
    const Scheduler_Control *scheduler,
    Thread_Control          *the_thread,
    Scheduler_Node          *node
  );

  /**
   * @brief Withdraw node operation.
   *
   * @param[in] scheduler The scheduler instance to withdraw the node.
   * @param[in] the_thread The thread using the node.
   * @param[in] node The scheduler node to withdraw.
   * @param[in] next_state The next thread scheduler state in case the node is
   *   scheduled.
   */
  void ( *withdraw_node )(
    const Scheduler_Control *scheduler,
    Thread_Control          *the_thread,
    Scheduler_Node          *node,
    Thread_Scheduler_state   next_state
  );
#endif

  /** @see _Scheduler_Node_initialize() */
  void ( *node_initialize )(
    const Scheduler_Control *,
    Scheduler_Node *,
    Thread_Control *,
    Priority_Control
  );

  /** @see _Scheduler_Node_destroy() */
  void ( *node_destroy )( const Scheduler_Control *, Scheduler_Node * );

  /** @see _Scheduler_Release_job() */
  void ( *release_job ) (
    const Scheduler_Control *,
    Thread_Control *,
    Priority_Node *,
    uint64_t,
    Thread_queue_Context *
  );

  /** @see _Scheduler_Cancel_job() */
  void ( *cancel_job ) (
    const Scheduler_Control *,
    Thread_Control *,
    Priority_Node *,
    Thread_queue_Context *
  );

  /** @see _Scheduler_Tick() */
  void ( *tick )( const Scheduler_Control *, Thread_Control * );

  /** @see _Scheduler_Start_idle() */
  void ( *start_idle )(
    const Scheduler_Control *,
    Thread_Control *,
    struct Per_CPU_Control *
  );

#if defined(__RTEMS_HAVE_SYS_CPUSET_H__) && defined(RTEMS_SMP)
  /** @see _Scheduler_Get_affinity() */
  bool ( *get_affinity )(
    const Scheduler_Control *,
    Thread_Control *,
    size_t,
    cpu_set_t *
  );
  
  /** @see _Scheduler_Set_affinity() */
  bool ( *set_affinity )(
    const Scheduler_Control *,
    Thread_Control *,
    size_t,
    const cpu_set_t *
  );
#endif
} Scheduler_Operations;

/**
 * @brief Scheduler context.
 *
 * The scheduler context of a particular scheduler implementation must place
 * this structure at the begin of its context structure.
 */
typedef struct Scheduler_Context {
  /**
   * @brief Lock to protect this scheduler instance.
   */
  ISR_LOCK_MEMBER( Lock )

#if defined(RTEMS_SMP)
  /**
   * @brief Count of processors owned by this scheduler instance.
   */
  uint32_t processor_count;
#endif
} Scheduler_Context;

/**
 * @brief Scheduler control.
 */
struct Scheduler_Control {
  /**
   * @brief Reference to a statically allocated scheduler context.
   */
  Scheduler_Context *context;

  /**
   * @brief The scheduler operations.
   */
  Scheduler_Operations Operations;

  /**
   * @brief The maximum priority value of this scheduler.
   *
   * It defines the lowest (least important) thread priority for this
   * scheduler.  For example the idle threads have this priority.
   */
  Priority_Control maximum_priority;

  /**
   * @brief The scheduler name.
   */
  uint32_t name;
};

/**
 * @brief Registered schedulers.
 *
 * Application provided via <rtems/confdefs.h>.
 *
 * @see _Scheduler_Count.
 */
extern const Scheduler_Control _Scheduler_Table[];

/**
 * @brief Count of registered schedulers.
 *
 * Application provided via <rtems/confdefs.h> on SMP configurations.
 *
 * It is very important that this is a compile-time constant on uni-processor
 * configurations (in this case RTEMS_SMP is not defined) so that the compiler
 * can optimize the some loops away
 *
 * @see _Scheduler_Table.
 */
#if defined(RTEMS_SMP)
  extern const size_t _Scheduler_Count;
#else
  #define _Scheduler_Count ( (size_t) 1 )
#endif

#if defined(RTEMS_SMP)
  /**
   * @brief The scheduler assignment default attributes.
   */
  #define SCHEDULER_ASSIGN_DEFAULT UINT32_C(0x0)

  /**
   * @brief The presence of this processor is optional.
   */
  #define SCHEDULER_ASSIGN_PROCESSOR_OPTIONAL SCHEDULER_ASSIGN_DEFAULT

  /**
   * @brief The presence of this processor is mandatory.
   */
  #define SCHEDULER_ASSIGN_PROCESSOR_MANDATORY UINT32_C(0x1)

  /**
   * @brief Scheduler assignment.
   */
  typedef struct {
    /**
     * @brief The scheduler for this processor.
     */
    const Scheduler_Control *scheduler;

    /**
     * @brief The scheduler assignment attributes.
     *
     * Use @ref SCHEDULER_ASSIGN_DEFAULT to select default attributes.
     *
     * The presence of a processor can be
     * - @ref SCHEDULER_ASSIGN_PROCESSOR_OPTIONAL, or
     * - @ref SCHEDULER_ASSIGN_PROCESSOR_MANDATORY.
     */
    uint32_t attributes;
  } Scheduler_Assignment;

  /**
   * @brief The scheduler assignments.
   *
   * The length of this array must be equal to the maximum processors.
   *
   * Application provided via <rtems/confdefs.h>.
   *
   * @see _Scheduler_Table and rtems_configuration_get_maximum_processors().
   */
  extern const Scheduler_Assignment _Scheduler_Assignments[];
#endif

/**
 * @brief Returns the thread priority.
 *
 * @param[in] scheduler Unused.
 * @param[in] priority The thread priority.
 *
 * @return priority The thread priority.
 */
Priority_Control _Scheduler_default_Map_priority(
  const Scheduler_Control *scheduler,
  Priority_Control         priority
);

#define _Scheduler_default_Unmap_priority _Scheduler_default_Map_priority

#if defined(RTEMS_SMP)
  /**
   * @brief Does nothing.
   *
   * @param[in] scheduler Unused.
   * @param[in] the_thread Unused.
   * @param[in] node Unused.
   *
   * @retval false Always.
   */
  bool _Scheduler_default_Ask_for_help(
    const Scheduler_Control *scheduler,
    Thread_Control          *the_thread,
    Scheduler_Node          *node
  );

  /**
   * @brief Does nothing.
   *
   * @param[in] scheduler Unused.
   * @param[in] the_thread Unused.
   * @param[in] node Unused.
   */
  void _Scheduler_default_Reconsider_help_request(
    const Scheduler_Control *scheduler,
    Thread_Control          *the_thread,
    Scheduler_Node          *node
  );

  /**
   * @brief Does nothing.
   *
   * @param[in] scheduler Unused.
   * @param[in] the_thread Unused.
   * @param[in] node Unused.
   * @param[in] next_state Unused.
   */
  void _Scheduler_default_Withdraw_node(
    const Scheduler_Control *scheduler,
    Thread_Control          *the_thread,
    Scheduler_Node          *node,
    Thread_Scheduler_state   next_state
  );

  #define SCHEDULER_OPERATION_DEFAULT_ASK_FOR_HELP \
    _Scheduler_default_Ask_for_help, \
    _Scheduler_default_Reconsider_help_request, \
    _Scheduler_default_Withdraw_node,
#else
  #define SCHEDULER_OPERATION_DEFAULT_ASK_FOR_HELP
#endif

/**
 * @brief Does nothing.
 *
 * @param[in] scheduler Unused.
 * @param[in] the_thread Unused.
 */
void _Scheduler_default_Schedule(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread
);

/**
 * @brief Performs the scheduler base node initialization.
 *
 * @param[in] scheduler Unused.
 * @param[in] node The node to initialize.
 * @param[in] the_thread Unused.
 * @param[in] priority The thread priority.
 */
void _Scheduler_default_Node_initialize(
  const Scheduler_Control *scheduler,
  Scheduler_Node          *node,
  Thread_Control          *the_thread,
  Priority_Control         priority
);

/**
 * @brief Does nothing.
 *
 * @param[in] scheduler Unused.
 * @param[in] node Unused.
 */
void _Scheduler_default_Node_destroy(
  const Scheduler_Control *scheduler,
  Scheduler_Node          *node
);

/**
 * @brief Does nothing.
 *
 * @param[in] scheduler Unused.
 * @param[in] the_thread Unused.
 * @param[in] priority_node Unused.
 * @param[in] deadline Unused.
 * @param[in] queue_context Unused.
 *
 * @retval NULL Always.
 */
void _Scheduler_default_Release_job(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  Priority_Node           *priority_node,
  uint64_t                 deadline,
  Thread_queue_Context    *queue_context
);

/**
 * @brief Does nothing.
 *
 * @param[in] scheduler Unused.
 * @param[in] the_thread Unused.
 * @param[in] priority_node Unused.
 * @param[in] queue_context Unused.
 *
 * @retval NULL Always.
 */
void _Scheduler_default_Cancel_job(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  Priority_Node           *priority_node,
  Thread_queue_Context    *queue_context
);

/**
 * @brief Performs tick operations depending on the CPU budget algorithm for
 * each executing thread.
 *
 * This routine is invoked as part of processing each clock tick.
 *
 * @param[in] scheduler The scheduler.
 * @param[in] executing An executing thread.
 */
void _Scheduler_default_Tick(
  const Scheduler_Control *scheduler,
  Thread_Control          *executing
);

/**
 * @brief Starts an idle thread.
 *
 * @param[in] scheduler The scheduler.
 * @param[in] the_thread An idle thread.
 * @param[in] cpu This parameter is unused.
 */
void _Scheduler_default_Start_idle(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  struct Per_CPU_Control  *cpu
);

#if defined(__RTEMS_HAVE_SYS_CPUSET_H__) && defined(RTEMS_SMP)
  /**
   * @brief Get affinity for the default scheduler.
   *
   * @param[in] scheduler The scheduler instance.
   * @param[in] thread The associated thread.
   * @param[in] cpusetsize The size of the cpuset.
   * @param[out] cpuset Affinity set containing all CPUs.
   *
   * @retval 0 Successfully got cpuset
   * @retval -1 The cpusetsize is invalid for the system
   */
  bool _Scheduler_default_Get_affinity(
    const Scheduler_Control *scheduler,
    Thread_Control          *thread,
    size_t                   cpusetsize,
    cpu_set_t               *cpuset
  );

  /** 
   * @brief Set affinity for the default scheduler.
   *
   * @param[in] scheduler The scheduler instance.
   * @param[in] thread The associated thread.
   * @param[in] cpusetsize The size of the cpuset.
   * @param[in] cpuset Affinity new affinity set.
   *
   * @retval 0 Successful
   *
   *  This method always returns successful and does not save
   *  the cpuset.
   */
  bool _Scheduler_default_Set_affinity(
    const Scheduler_Control *scheduler,
    Thread_Control          *thread,
    size_t                   cpusetsize,
    const cpu_set_t         *cpuset
  );

  #define SCHEDULER_OPERATION_DEFAULT_GET_SET_AFFINITY \
    , _Scheduler_default_Get_affinity \
    , _Scheduler_default_Set_affinity
#else
  #define SCHEDULER_OPERATION_DEFAULT_GET_SET_AFFINITY
#endif

/**
 * @brief This defines the lowest (least important) thread priority of the
 * first scheduler instance.
 */
#define PRIORITY_MAXIMUM ( _Scheduler_Table[ 0 ].maximum_priority )

/**@}*/

#ifdef __cplusplus
}
#endif

#endif
/* end of include file */
