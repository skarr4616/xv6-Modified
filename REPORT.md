# OSN Assignment 4 Report
# Specification 1:

## strace system call
- Added `sys_trace()` in `sysproc.c` that reads the syscall arguments sets the mask field of the process struct.

- Modified `fork()` to copy mask from parent to child

- Modified `syscall()` to store the arguments in registers `r0` to `r6` and to print syscall and its arguments if the syscall bit is set in the mask.

- Created user program strace in `user/strace.c` that calls the trace system call to set the mask of the process and run the function passed as argument.

## sigalarm and sigreturn

- Added `sys_sigalarm()` in `sysproc.c` that reads the syscall arguments and sets the number of ticks and the address of the alarm handler.

- Modified `trap.c` to increase the `curr_ticks` of the running process on timer interrupts and if `curr_ticks` is greater than ticks, it saves the current trapframe of the process and switches context to the alarm handler.

- Added `sys_sigreturn()` in `sysproc.c` that restores the original trapframe and switches context to the previously running process.

# Specification 2:
## Scheduling schemes

Round Robinis the default scheduler of xv6-RISC. We have implemented four other scheduluing schemes namely

- FCFS : First Come, First Serve
- PBS : Priority Based Scheduling
- LBS : Lottery Based Scheduling
- MLFQ : Multi Level Feedback Queue

The user can specify the scheduler that they want by providing an extra argument while running `make qemu`. For example, to compile with the FCFS scheduler, give the following argument:

```
make qemu SCHEDULER=<scheme>
```
where scheme can be `FCFS`, `RR`, `LBS`, `PBS`, `MLFQ`

### Universal changes

- `struct proc`:  variables `start_time`, `total_run_time` and `end_time` have been added to keep track of the time of creation, total running duration and completion respectively for a process.

- `clockintr()`:  was modified with the new `update_timer()` function to track the process's timer variables.

- `allocproc()`:  for the purpose of initialising the new timer variables

- `kerneltrap()`: so that the process can/cannot be prempted with the inbuilt timer interrupts according to the scheduler type

- `usertrap()`: so that the process can/cannot be prempted with the inbuilt timer interrupts according to the scheduler type


### FCFS Scheme:

- FCFS Scheduling scheme selects the process with the lowest start time.

- Appropriate flags have been added to prevent preemption of a process in FCFS.

- Compilation flags were added accordingly to run the FCFS scheduler scheme.

```
#ifdef FCFS

    struct proc *p;
    struct proc *process_to_serve = 0;

    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state != RUNNABLE)
      {
        release(&p->lock);
        continue;
      }

      if (!process_to_serve)
      {
        process_to_serve = p;
        continue;
      }
      else if (p->start_time < process_to_serve->start_time)
      {
        release(&process_to_serve->lock);
        process_to_serve = p;
        continue;
      }

      release(&p->lock);
    }

    if (process_to_serve)
    {
      process_to_serve->state = RUNNING;

      c->proc = process_to_serve;
      swtch(&c->context, &process_to_serve->context);

      c->proc = 0;
      release(&process_to_serve->lock);
    }

#else
```

- The scheduler selects the process with the earliest start time and allocates the CPU's resources to that particular process. 

### PBS Scheme:

Modified:
- `struct proc` : new variables `static_p`, `n_scheduled`, `last_run_time` and `last_sleep_time` were added
<!-- - `proc.c`: new system call `set_priortiy` was implemented -->

- `get_dynamic_priority()` is a new function which is called in the PBS scheduling scheme to get the dynamic priority of the process in consideration.

```
int get_dynamic_priority(struct proc *p)
{
  int niceness = 5;
  int last_exec_time = p->last_sleep_time + p->last_run_time;

  if (last_exec_time != 0)
    niceness = (p->last_sleep_time * 10) / last_exec_time;

  int dynamic_p = p->static_p - niceness + 5;

  if (100 < dynamic_p)
  {
    dynamic_p = 100;
  }

  if (0 > dynamic_p)
  {
    dynamic_p = 0;
  }

  return dynamic_p;
}
```

```
#ifdef PBS

    struct proc *p;
    struct proc *process_to_serve = 0;
    uint min_p = 101;

    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        uint dynamic_p = get_dynamic_priority(p);

        if (!process_to_serve)
        {
          process_to_serve = p;
          continue;
        }
        else if (dynamic_p < min_p)
        {
          release(&process_to_serve->lock);
          process_to_serve = p;
          min_p = dynamic_p;
          continue;
        }
        else if (dynamic_p == min_p && p->n_scheduled < process_to_serve->n_scheduled)
        {
          release(&process_to_serve->lock);
          process_to_serve = p;
          min_p = dynamic_p;
          continue;
        }
        else if (dynamic_p == min_p && p->n_scheduled == process_to_serve->n_scheduled && p->start_time < process_to_serve->start_time)
        {
          release(&process_to_serve->lock);
          process_to_serve = p;
          min_p = dynamic_p;
          continue;
        }
      }
      release(&p->lock);
    }

    if (process_to_serve)
    {
      process_to_serve->state = RUNNING;

      process_to_serve->n_scheduled += 1;
      process_to_serve->last_run_time = 0;
      process_to_serve->last_sleep_time = 0;

      c->proc = process_to_serve;
      swtch(&c->context, &process_to_serve->context);

      c->proc = 0;
      release(&process_to_serve->lock);
    }

#else
```

- This scheduling scheme selects the process with the lowest dynamic priority. In case two or more processes have the same priority, we
use the number of times the process has been scheduled to break the tie. If the tie remains, use the start-time of the process to break the tie(processes with lower start times should be scheduled further).

- `set_priority` system call was added which takes 2 arguments - the new static priority and the process's PID, modifies the static priority of the process with that particular PID and returns the old static priority. Returns -1 if the process doesn't exist. 

- Preempts the CPU to run the scheuler if the new dynamic priority is more than the old dynamic priority.

```
int set_priority(int new_static_p, int pid)
{
  struct proc *p;

  if (new_static_p < 0 || new_static_p > 100)
  {
    printf("static priority %d is not in range [0, 100]\n", new_static_p);
    return -1;
  }

  int old_static_p = -1;
  int found = 0;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->pid == pid)
    {
      found = 1;

      old_static_p = p->static_p;
      int old_dynamic_p = get_dynamic_priority(p);

      p->static_p = new_static_p;
      int new_dynamic_p = get_dynamic_priority(p);

      release(&p->lock);

      if (old_dynamic_p < new_dynamic_p)
        yield();

      break;
    }
    release(&p->lock);
  }

  if (!found)
  {
    printf("process with pid = %d not found\n", pid);
  }

  return old_static_p;
}
```

### LBS Scheme

Modified:
- `struct proc` : new variables `tickets` was added

```
#ifdef LBS

    struct proc *p;
    int total_tickets = 0;

    for (p = proc; p < &proc[NPROC]; p++)
    {
      if (p->state != RUNNABLE)
        continue;
      total_tickets = total_tickets + p->tickets;
    }

    if (total_tickets == 0)
    {
      continue;
    }

    int golden_ticket = random_in_range(total_tickets);
    int curr_ticket = 0;

    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        if (curr_ticket + p->tickets >= golden_ticket)
        {
          p->state = RUNNING;
          c->proc = p;
          swtch(&c->context, &p->context);
          c->proc = 0;
          release(&p->lock);
          break;
        }
        else
        {
          curr_ticket = curr_ticket + p->tickets;
        }
      }
      release(&p->lock);
    }
#else
```

- We generate a random number between 1 and the total number of tickets of the processes. The process whose cumulative ticket sum is greater than or equal to this random number is scheduled on the CPU.


- `sysproc.c`: new system call `sys_settickets` was implemented which takes one argument - number of tickets, and changes the tickets of the calling process.

```
uint64
sys_settickets(void)
{
  argint(0, &(myproc()->tickets));
  return 0;  
}
```

### MLFQ Scheme

Modified:
- `struct proc` : new variables `q_level`, `queue_time`, `drop_queue` and a struct `Queue` were added

```
struct Queue {
  int head, tail;
  int size;
  struct proc *q[NPROC];
};
```

- `proc.c`: new functions `push_proc`, `pop_proc` and `rem_proc` were written.

```
#ifdef MLFQ

    struct proc *p;
    struct proc *process_to_serve = 0;

    for (int l = 0; l < 5; l++)
    {
      while (mlfq[l].size)
      {
        p = mlfq[l].q[mlfq[l].head];
        pop_proc(l);

        acquire(&p->lock);
        if (p->state == RUNNABLE)
        {
          process_to_serve = p;
          break;
        }
        release(&p->lock);
      }

      if (process_to_serve)
        break;
    }

    if (process_to_serve)
    {
      process_to_serve->state = RUNNING;
      process_to_serve->n_scheduled += 1;
      process_to_serve->last_run_time = 0;
      process_to_serve->last_sleep_time = 0;

      c->proc = process_to_serve;
      swtch(&c->context, &process_to_serve->context);

      c->proc = 0;

      if (process_to_serve->state == RUNNABLE)
      {
        if (process_to_serve->drop_queue)
        {
          process_to_serve->q_level += 1;
          process_to_serve->drop_queue = 0;
        }

        push_proc(process_to_serve->q_level, process_to_serve);
      }

      release(&process_to_serve->lock);
    }
#endif
```

- We find the non-empty queue with the top-most priority and schedule the process at the head of that queue. 

- Aging was implemented in the `update_timer()` function. The aging time has been kept as 30 ticks for every process.

```
else if (p->state == RUNNABLE)
    {
      p->queue_time++;

#ifdef MLFQ
      if (p->q_level > 0 && p->queue_time >= 30)
      {
        int l = p->q_level;
        rem_proc(l, p);
        push_proc(l - 1, p);
      }
#endif
    }
```

- `drop_queue` flag is set in trap.c , kerneltrap and usertrap function in case given process exceeds the time slice for that priority queue.

- If a new process is pushed into a queue of higher priority than the current process is preempted.


```
#ifdef MLFQ
  if (which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
  {
    struct proc* p = myproc();
    uint quanta = 1 << p->q_level;

    if (p->last_run_time >= quanta)
    {
      if (p->q_level < 4)
      {
        p->drop_queue = 1;
      }
      yield();
    }
    else
    {
      for (int q = 0; q < myproc()->q_level; ++q)
      {
        if (mlfq[q].size)
        {
          yield();
        }
      }
    }
  }
#endif
```

- When a process's status is changed to RUNNABLE, it is pushed into its respective queue in accordance to the flags set

## Performance Analysis


| Scheduling Scheme | Average run time | Average wait time | Number of harts |
| :-: | :-: | :-: | :-: |
| Round Robin | 17 | 118 | 3 |
| FCFS | 44 | 53| 3 |
| PBS | 21 | 100 | 3 |
| LBS | 13 | 121 | 3| 
| MLFQ | 18 | 172 | 1 |


# Specification 3:

- Modified `riscv.h` to define the `PTE_RSW` bit.
- Modified `kmem` struct to include a reference counter array `refc[]` that stores the number of virtual addresses a physical page is mapped to. This array is initialized to 0 in `freerange()` function

- `kfree` is modified to decrement the reference counter of a page and free it if the page's reference counter becomes zero.

- `kalloc()` is modified to initialize the reference counter of the allocated page to 1. 

- Added a `inc_refc()` function that increases the reference counter of a given page address. 

- Modified `uvmcopy()` function to unset the `PTE_W` bit and set the `PTE_RSW` bit. This function maps the physical page of the old virtual address to the new virtual address instead of allocating a new physical page.

- Implemented a function `cowfault()` which is called on page fault errors. This function allocates a new physical page with the same content to the virtual address of the process triggering the page fault and also unmaps it from the old physical page. The new physical page will have `PTE_W` bit set and the `PTE_RSW` bit unset.

- Modified `copyout()` function to check for `cowfaults`

- Modified `trap.c` to call `cowfault()` on page fault errors (r_scause = 15)