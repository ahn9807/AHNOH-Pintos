# OS HW Report
## 20175097 안준호 20175104 오정석

# HW 2
## Implementing Alarm Clock

## Implementing Priority Scheduling


### Requirement 1: implementing priority scheduling
#### Problem Definition
현재 pintos는 priority에 대한 고려를 하지 않고 ready queue에 순서대로 저장하고 사용한다. 이 경우 스레드가 우선순위에 상관없이 실행되어 비효율적이다. 따라서 스레드의 우선순위에 따라 실행 순서가 결정되도록 ready queue에 대기시켜줄 필요성이 있다.

#### Algorithm Design
ready queue에 스레드를 삽입할 때 우선순위가 정렬되어 삽입되도록 수정한다. ready queue에서 현재 cpu를 점유하고 있는 스레드의 우선순위보다 높은 스레드가 존재한다면 cpu를 양보해야 한다. ready queue에 스레드가 저장되는 것은 스레드가 unblock될 때와 yield될 때이다. 따라서 unblock에서 리스트의 뒤로 스레드를 삽입하는 부분을 list_insert_ordered로 변경해 정렬되어 삽입되도록 한다.

#### Implementation
먼저, 스레드의 우선순위를 비교하는 함수를 만든다.

    bool thread_order_priority(const struct list_elem* a,const struct list_elem* b, void *aux UNUSED) {
        return list_entry(a, struct thread, elem)->priority > list_entry(b, struct thread, elem)->priority;
    }
    
이 함수를 이용해 unblock 또는 yield되어 ready queue에 들어갈 때마다 정렬되어 들어가도록 한다.

    void thread_unblock (struct thread *t) 
    {
        enum intr_level old_level;  
        ASSERT (is_thread (t));
        old_level = intr_disable ();
        ASSERT (t->status == THREAD_BLOCKED);
        list_insert_ordered (&ready_list, &t->elem, thread_order_priority, 0);
        t->status = THREAD_READY;
        intr_set_level (old_level);
    }

    void thread_yield (void) 
    {
        struct thread *cur = thread_current ();
        enum intr_level old_level;
  
        ASSERT (!intr_context ());
        old_level = intr_disable ();
        if (cur != idle_thread) 
            list_insert_ordered (&ready_list, &cur->elem, thread_order_priority, 0);
        cur->status = THREAD_READY;
        schedule ();
        intr_set_level (old_level);
    }

현재 cpu를 점유중인 스레드와 priority를 비교해 더 높으 스레드가 대기중이면 양보하도록 하는 thread_test_priority를 만든다. 이 함수는 thread_create와 thread_set_priority 함수 내부에서 호출되도록 한다.

    void thread_test_priority(void) {
        enum intr_level old_level;
        old_level = intr_disable();
        struct thread* cur = thread_current();

        if(!list_empty(&ready_list) && cur->priority  < list_entry(list_front(&ready_list), struct thread, elem)->priority) {
            thread_yield();
        }
        intr_set_level(old_level);
    }


### Requirement2 : implementing priority donation
#### Problem Definition
운영체제에서는 스레드 간 공유되는 데이터를 읽고 쓸 때 충돌을 방지하기 위해 lock이 존재한다. 따라서 데이터에 접근해 쓰는 권한은 lock을 가지고 있는 스레드에 제한된다. 하지만 우선순위가 낮은 스레드(L)가 이미 lock을 가져간 상황에서 우선순위가 높은 스레드(H)가 lock을 요청하는 경우, H는 priority scheduling의 경우와 다르게 나중에 실행될 필요가 있다. 이를 위해 H는 일시적으로 자신의 높은 우선순위를 L에게 양보해 먼저 실행되도록 해야 한다.

#### Algorithm Design
1. Multiple Donation
우선순위에 따라 L, M, H 스레드가 존재한다고 가정한다. 한 스레드가 두개 이상의 lock을 가지게 되면, 각 lock마다 donation이 발생 가능하다. 즉, 한 스레드에 양보되는 우선순위게 여러개 발생하게 된다. 이를 위해 양보되는 우선순의들을 저장하는 자료구조를 스레드 구조체 내에 새로 만들고, 양보되는 우선순위의 크기들을 비교해 현재의 우선순위를 결정한다. 예를 들어, 스레드 L이 lock A와 B를 보유하고 있고, M과 H가 각각 A와 B를 요청하는 경우, L은 H의 priority를 갖게 된다. 이 priority는 스레드 L이 작업을 마치고 lock B를 포기함에 따라 M의 priority로 강등되고, 최종적으로 lock A마저 포기하게 되면 자신의 priority로 돌아와야 한다.
2. Nested Donation
우선순위에 따라 L, M, H 스레드가 존재한다고 가정한다. 스레드 L이 처음에 lock A를 요청해 보유하고 있다. M이 lock B를 요청하고, 작업하는 과정에서 lock A를 요청한다. 이후 H가 lock B를 요청한면 Nested Donation의 경우에 해당한다. 시간 순서대로 보면, 처음 L이 lock A를 가지고 있고, M이 lock B를 가지고 있는 상황에서 lock A를 요청하고, H가 lock B를 요청한 상황이다. 이 경우, M이 lock A를 요청하는 상황에서 첫번째 priority donation이 발생한다. 이에 따라 M의 priority가 L에게 양보된다. 이후 H가 lock B를 요청할 때 두번째 priority donation이 발생한다. 이때 L은 H의 priority를 가지게 된다. L이 lock A를 반환하게 되면 L의 priority가 M으로 양보된다. M이 lock B를 반환하게 되면 다시 L이 자신의 priority를 회수하게 되고, H가 실행되게 된다.

#### Implementation
우선순위를 저장할 struct thread 내 자료구조 선언

    struct thread{
      …
      int init_priority;                  // 스레드의 원래 priority 저장
      struct lock *wait_on_lock;          // 스레드가 대기중인 lock의 자료구조 저장
      struct list donations;              // 스레드에 양보된 priority값들 저장
      …
    }

자신의 priority를 양보하는 함수 donate priority를 구현. 이 함수는 lock을 기다리는 스레드가 있을 경우 재귀적으로 호출됨.

    void donate_priority(struct lock *lock){
        ASSERT(lock != NULL);
        ASSERT(lock->holder != NULL);

        if (thread_current ()->priority > lock->holder->priority){
            lock->holder->priority = thread_current()->priority;
            if(lock->holder->wait_on_lock != NULL){
                donate_priority(lock->holder->wait_on_lock);
            }
        }
    }

lock_aquire에서 mlfqs가 아니고 lock의 holder가 존재할 때 priority donation이 일어나도록 설정

    lock_acquire (struct lock *lock)
    {
      ASSERT (lock != NULL);
      ASSERT (!intr_context ());
      ASSERT (!lock_held_by_current_thread (lock));
  
      if(thread_mlfqs == false) {
        struct thread *curr_thread = thread_current();
        if(lock->holder != NULL){
          donate_priority(lock);
        }
        curr_thread->wait_on_lock = lock;
        sema_down (&lock->semaphore);
        lock->holder = thread_current ();
        curr_thread->wait_on_lock = NULL;
        list_push_back(&curr_thread->donations, &lock->elem);
      } else {
        sema_down(&lock->semaphore);
        lock->holder = thread_current();
      }
    }

lock_release에서 lock 스레드 대기열에서 release와 동시에 제거하고 priority를 초기화하도록 설정한다. priority를 초기화하는 함수를 만든다.

    int refresh_priority(void){
        int initial_priority = thread_current()->init_priority;
        struct list *list = &thread_current()->donations;
        if(list_empty(list)){
            return initial_priority;
        }

        struct list_elem *e;
        for (e = list_begin (list); e != list_end (list); e = list_next (e)){
            struct semaphore *sema = &list_entry(e, struct lock, elem)->semaphore;
            if (!list_empty (&sema->waiters))
            {
                int temp_priority = list_entry (list_begin (&sema->waiters),
                                          struct thread, elem)->priority;
                if (temp_priority > initial_priority)
                    initial_priority = temp_priority;
            }
        }
        return initial_priority;
    }
 
 thread_set_priority와 thread_get_priority를 구현해 donation이 일어날 때마다 priority들을 비교해 현재의 priority를 갱신한다.
 
    void thread_set_priority (int new_priority) 
    {
        if(thread_mlfqs == false) {
            struct thread *curr_thread = thread_current();
            int priority = curr_thread->priority;
            if(curr_thread->priority==curr_thread->init_priority || new_priority > priority){
                curr_thread->priority = new_priority;
            }
            curr_thread->init_priority = new_priority;
            if(priority > curr_thread->priority){
                thread_test_priority();
            }
        }
        else{
            thread_current ()->priority = new_priority;
            thread_test_priority();
        }
    }
    int thread_get_priority (void) 
    {
        return thread_current ()->priority;
    }



## Implementing Advanced Scheduler
### Problem Definition
우선순위 알고리즘에서 더욱 발전된, 다단계 피드백 큐 스케쥴링 알고리즘을 구현해야 한다. 다단계 피드백 큐란, 프로세스 (pintos 에서는 thread)들이 CPU 버스트 성격에 따라서 우선순위를 바꾸는 것이다. 어떤 프로세스가 CPU시간을 너무 많이 사용하면, 그 프로세스의 우선순위는 낮은 우선순위의 큐로 이동된다. 따라서 우리는 동적으로 우선순위가 각각의 스레드마다 배정될 수 있게 하여, CPU사용량에 따라 우선순위가 수정되고 그를 통해서 스레드를 스케쥴링 하는 것을 구현해야 한다. 
### Algorithm Design
1. Nice value
스레드의 우선순위는 동적으로 결정되지만, 스레드 마다 어느정도로 다른 스레드에 자신의 우선순위를 양보할 것인지에 대한 정도를 지정할 수 있다. 그 값을 Nice value라고, 하는데 nice 값이 0 이면 아무런 일도 하지 않지만, 양수면 다른 스레드보다 우선순위가 빨리 떨어지고, 음수면 다른 스레드보다 우선순위가 천천히 떨어지게 된다. 즉 nice value로 다른 스레드들에 대한 상대적인 위치를 지정할 수 있다. 
2. Priority의 계산
핀토스에는 0부터 63까지 64개의 우선순위가 지정되어 있다. niec 값이 양수이면 priority가 떨어져야 하며, 최근 CPU를 많이 차지할 수록 우선순위가 떨어져야 한다. 이를 Ageing이라 하기도 한다. 따라서 다음 공식처럼 주어진다면, priority를 계산 할 수 있다. 
$$priority=PRI\_MAX-(recent\_cpu/4)-(nice*2)$$
윗 공식에서 알 수 있다시피, 최근 cpu를 많이 사용할 수록 우선순위가 낮아지기 때문에 기아(starvation)또한 예방 할 수 있다. 
3. CPU 사용량의 계산
CPU사용량을 바로직전 사용한 CPU의 총량으로 해도 되지만, 최근에 사용한 CPU의 점유율 일 수록 더욱 많은 가중치를 줌으로써, CPU의 점유율을 표현할 수 있다. 핀토스에서는 채점의 용의함을 위해서, recent_cpu의 계산은 4틱당 한번씩 하도록 규정하고 있다.
$$recent\_cpu=(2*load\_avg)/(2*load\_avg + 1)*recent\_cpu + nice$$
4. Load Average 의 계산
Load Average또한 ready queue에서 동작을 기다리는 큐들의 평균으로 구현 할 수 있다. Load Average의 평균또한 최근에 사용한 값일 수록 더욱 많은 가중치를 부여받게 된다. 핀토스에서는 채점의 용의함을 위해서, load average의 계산은 타이머의 주기마다 한번씩 실행하도록 규정하고 있다. 
$$load\_avg=(59/60)*load\_avg + (1/60)*ready\_threads$$
5. 실수의 계산
운영체제에서는 속도상의 문제와 구현의 복잡성때문에 실수의 계산은 커널상에서 제공하지 않는다. 따라서 실수의 계산은 직접 구현해야만 한다. 그러나 pintos에서는 실수의 계산에 대한 가이드라인을 제공하고 있기에, 그에따라 만들면 별 문제 없이 처리 할 수 있다.
### Implementation
