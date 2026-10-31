/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
   */

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
extern struct list ready_list;
extern bool compare_pr(const struct list_elem *a, const struct list_elem *b, void *aux);
/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
   decrement it.

   - up or "V": increment the value (and wake up one waiting
   thread, if any). */

void
sema_init (struct semaphore *sema, unsigned value) {
	ASSERT (sema != NULL);

	sema->value = value;
	list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. This is
   sema_down function. */
void
sema_down (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);
	ASSERT (!intr_context ());

	old_level = intr_disable ();
	while (sema->value == 0) {
		/*우선순위 정렬*/
		list_insert_ordered(&sema->waiters, &thread_current ()->elem,compare_pr,NULL);
		thread_block ();
	}
	sema->value--;
	intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) {
	enum intr_level old_level;
	bool success;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (sema->value > 0)
	{
		sema->value--;
		success = true;
	}
	else
		success = false;
	intr_set_level (old_level);

	return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (!list_empty (&sema->waiters)){
	        list_sort(&sema->waiters,compare_pr,NULL);
		thread_unblock (list_entry (list_pop_front (&sema->waiters),struct thread, elem));}
	sema->value++;
        if(!list_empty(&ready_list)){ 
		struct thread *r = list_entry(list_front(&ready_list),struct thread,elem);
	if (r->priority > thread_current()->priority)
	{
		thread_yield();
	}

       }       intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) {
	struct semaphore sema[2];
	int i;

	printf ("Testing semaphores...");
	sema_init (&sema[0], 0);
	sema_init (&sema[1], 0);
	thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
	for (i = 0; i < 10; i++)
	{
		sema_up (&sema[0]);
		sema_down (&sema[1]);
	}
	printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) {
	struct semaphore *sema = sema_;
	int i;

	for (i = 0; i < 10; i++)
	{
		sema_down (&sema[0]);
		sema_up (&sema[1]);
	}
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock.락은 한 번에 최대 한 스레드만 가질 수 있음 재귀가 아니라서, 이미 락을 쥔 스레드가 같은 락을 또 acquire하면 에러 락은 초기값 1인 세마포어의 특수한 형태 차이는 두 가지.
   세마포어는 값이 1보다 클 수 있지만, 락은 한 스레드만 소유 가능
   세마포어는 주인이 없어서 A가 down하고 B가 up해도 되지만, 락은 잡은 스레드가 직접 풀어야 함 */
void
lock_init (struct lock *lock) {
	ASSERT (lock != NULL);

	lock->holder = NULL;
	sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep.락을 얻을 때까지 필요하면 잠들어서 기다림 단, 이미 내가 쥔 락은 안 돼.
   잠들 수 있어서 인터럽트 핸들러 안에서 호출 금지.인터럽트가 꺼진 상태로 호출해도 되는데, 잠들어야 하면 인터럽트가 다시 켜져 */
static bool wait_pr(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED) ;
void
lock_acquire (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (!lock_held_by_current_thread (lock));
        struct thread *t = thread_current();
        if (thread_mlfqs == false &&lock->holder != NULL){
	t->wait_on_lock = lock;
	list_insert_ordered(&lock->holder->donation,&t->donation_elem,wait_pr,NULL);
	if(t->priority > lock->holder->priority)
        lock->holder->priority = t->priority;
       /*내 우선순위가 홀더의 우선순위보다 높으면*/ 
	/*내가 기다리는 사람->그사람이 기다리는 사람->따라 올라가면서 전원에게 내 우선순위를 기부하는 것
	지금 그사람에게 기부 그사람의  wait_on_lock을 보고 다음 사람으로 이동 멈추는 조건 다음 기다리는 락이 없을때 깊이가 8이 넘었을때 최대 8번이동*/
	int i = 0;
	struct thread *curr_holder = lock->holder;
	struct thread *temp_holder = NULL;//내다음사람의 리스트 잠시담을변수
        while ((curr_holder->wait_on_lock != NULL) && i < 8)
       //다음 사람으로 이동
      { temp_holder = curr_holder->wait_on_lock->holder;
	
	/*현재 우선순위가 다음 우선순위보다 크면 다음 우선순위한테 현재 우선순위 기부*/
        if(curr_holder->priority > temp_holder->priority)
	{temp_holder->priority = curr_holder->priority;
	}
	curr_holder = temp_holder;
        i++;
} 
}       sema_down (&lock->semaphore);
        t->wait_on_lock = NULL;
	lock->holder = thread_current ();
	
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler.락을 얻으면 true, 못 얻으면 false를 반환.
잠들지 않아서 인터럽트 핸들러 안에서도 호출 가능. (기다리지 않고 바로 결과만 알려줌) */
bool
lock_try_acquire (struct lock *lock) {
	bool success;

	ASSERT (lock != NULL);
	ASSERT (!lock_held_by_current_thread (lock));

	success = sema_try_down (&lock->semaphore);
	if (success)
		lock->holder = thread_current ();
	return success;
}

/* Releases LOCK, which must be owned by the current thread.
   This is lock_release function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. 현재 스레드가 소유한 락만 풀수 있음 인터럽트 핸들러는 애초에 락을 얻을 수 없으니까 핸들러 안에서 푸는것도 의미없음 이락때문에 기부한 애들만 명단에서 빼고 남은 명단+원래 우선순위로 다시계산 */
void
lock_release (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (lock_held_by_current_thread (lock));
	struct thread *t = thread_current();
	struct list_elem *d;//elem 담을 임시 변수
	struct thread *tmp;//스레드 담을 임시변수
	/*기부명단 처음부터 끝까지 훑으면서*/
	if (thread_mlfqs == false){
        for (d = list_begin(&t->donation); d !=list_end(&t->donation);){
	/*스레드랑 wait_on_lock에있는 스레드랑 같으면 기부 요소 명단에서 삭제*/
        tmp = list_entry(d,struct thread,donation_elem);
        if (tmp->wait_on_lock == lock)
	{ d = list_remove(d);}
	else
	d = list_next(d);
}       t->priority = t->init_priority;
        /*기존 기부명단에 남은 스레드들 +우선순위 순서대로 재정렬하기*/
        if (!list_empty(&t->donation))
	{struct thread *dnt = list_entry(list_front(&t->donation),struct thread,donation_elem);
		if (dnt->priority > t->priority)
		{ t->priority = dnt->priority; }
	}
	}	lock->holder = NULL;
	sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) {
	ASSERT (lock != NULL);

	return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem {
	struct list_elem elem;              /* List element. */
	struct semaphore semaphore;         /* This semaphore. */
};
static bool sema_pr(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED) 
{ 
	struct semaphore_elem *ma = list_entry(a, struct semaphore_elem, elem);
	struct semaphore_elem *mb = list_entry(b, struct semaphore_elem, elem);
	
	struct thread *sa = list_entry(list_begin(&ma->semaphore.waiters), struct thread, elem);
	struct thread *sb = list_entry(list_begin(&mb->semaphore.waiters), struct thread, elem);
	return sa->priority > sb->priority; 
	// a가 b보다 작으면 참 반환 아니면 거짓 반환
}

static bool wait_pr(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED) 
{ 
	struct thread *ma = list_entry(a, struct thread, donation_elem);
	struct thread *mb = list_entry(b, struct thread, donation_elem);
	
	return ma->priority > mb->priority; 
	// a가 b보다 크면 참 반환 아니면 거짓 반환
}

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it.  */
void
cond_init (struct condition *cond) {
	ASSERT (cond != NULL);

	list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. 락을 원자적으로 풀고 신호를 기다렸다가 신호를 받으면 락을 다시 잡고 리턴 호출전에 락을 쥐고있어야함*/
void
cond_wait (struct condition *cond, struct lock *lock) {
	struct semaphore_elem waiter;

	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	sema_init (&waiter.semaphore, 0);
	list_push_back (&cond->waiters, &waiter.elem);
	lock_release (lock);
	sema_down (&waiter.semaphore);
	lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler cond에서 기다리는 스레드가 있으면 그중 하나를 깨움 호출 전에 락을 쥐고있어야함 인터럽트 핸들러에서는 락을 못쓰니까 호출할 일 없음. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	if (!list_empty (&cond->waiters)){
	        list_sort(&cond->waiters,sema_pr,NULL);
		sema_up (&list_entry (list_pop_front (&cond->waiters),
					struct semaphore_elem, elem)->semaphore);}
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);

	while (!list_empty (&cond->waiters))
		cond_signal (cond, lock);
}
