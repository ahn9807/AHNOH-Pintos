# OS HW Report
## 20175097 안준호 20175104 오정석

# HW 2
## Implementing Alarm Clock

## Implementing Priority Scheduling
현재 pintos는 priority에 대한 고려를 하지 않고 ready queue에 순서대로 저장하고 사용한다. 이를 해결하기 위해 thread끼리의 priority를 비교하는 함수를 만든다.

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
