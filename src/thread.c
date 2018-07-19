#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "common.h"
#include "tdm.h"

#define ITEMS_PER_ALLOC 128

static CQ_ITEM *cqi_freelist;
static CQ_ITEM *cqi_freelist_work;
static pthread_mutex_t cqi_freelist_lock;
static pthread_mutex_t cqi_freelist_work_lock;
static pthread_mutex_t *item_locks;
static LIBEVENT_THREAD *threads;
static LIBEVENT_THREAD *threads_work;
static int last_thread = -1;//用于记录上一次插入的线程索引值
static int thread_nums = -1;

static int init_count = 0;
static pthread_mutex_t init_lock;
static pthread_cond_t init_cond;

static CQ_ITEM *cqi_new_base(CQ_ITEM **list, pthread_mutex_t *lock)
{
    CQ_ITEM *item = NULL;

    pthread_mutex_lock(lock);
    if (*list) {
        item = *list;
        *list = item->next;
    }
    pthread_mutex_unlock(lock);

    if (NULL == item) {
        int i;
		item = malloc(sizeof(CQ_ITEM) * ITEMS_PER_ALLOC);
        if (NULL == item) {
            return NULL;
        }

		for (i = 2; i < ITEMS_PER_ALLOC; i++)
            item[i - 1].next = &item[i];

        pthread_mutex_lock(lock);
        item[ITEMS_PER_ALLOC - 1].next = *list;
        *list = &item[1];
        pthread_mutex_unlock(lock);
    }

    return item;
}

static CQ_ITEM *cqi_new_work(void)
{
	return cqi_new_base(cqi_freelist_work, &cqi_freelist_work_lock);
}

static CQ_ITEM *cqi_new(void)
{
	return cqi_new_base(cqi_freelist, &cqi_freelist_lock);
}

static void cqi_free_base(CQ_ITEM *item, CQ_ITEM **list, pthread_mutex_t *lock)
{
	pthread_mutex_lock(lock);
	item->next = *list;
	*list = item;
	pthread_mutex_unlock(lock);
}

static void cqi_free(CQ_ITEM *item) {
	cqi_free_base(item, &cqi_freelist, &cqi_freelist_lock);
}

static void cqi_free_work(CQ_ITEM *item) {
	cqi_free_base(item, &cqi_freelist_work, &cqi_freelist_work_lock);
}

static void cq_push(CQ *cq, CQ_ITEM *item) {
    item->next = NULL;

    pthread_mutex_lock(&cq->lock);
    if (NULL == cq->tail)
        cq->head = item;
    else
        cq->tail->next = item;
    cq->tail = item;
    pthread_mutex_unlock(&cq->lock);
}

static CQ_ITEM *cq_pop(CQ *cq) {
    CQ_ITEM *item;

    pthread_mutex_lock(&cq->lock);
    item = cq->head;
    if (NULL != item) {
        cq->head = item->next;
        if (NULL == cq->head)
            cq->tail = NULL;
    }
    pthread_mutex_unlock(&cq->lock);

    return item;
}

static void cq_init(CQ *cq) {
    pthread_mutex_init(&cq->lock, NULL);
    cq->head = NULL;
    cq->tail = NULL;
}


static void thread_libevent_process(int fd, short which, void *arg) {
    LIBEVENT_THREAD *me = arg;
    CQ_ITEM *item;
    char buf[1];
    conn *c;

    if (read(fd, buf, 1) != 1) {
       	fprintf(stderr, "Can't read from libevent pipe\n");
        return;
    }

    switch (buf[0]) {
    case 'c':
        item = cq_pop(me->new_conn_queue);

        if (NULL == item) {
            break;
        }

		switch (item->mode) {
			case queue_new_conn:
				c = conn_new(item->sfd, item->init_state, item->event_flags, item->read_buffer_size, me->base);
				if (c == NULL) {
					fprintf(stderr, "Can't listen for events on fd %d\n", item->sfd);
					close(item->sfd);
				}else{
					c->thread = me;
				}
				break;
		}

		cqi_free(item);
		break;
	}
}

static void work_func(int fd, short which, void *arg)
{
#if 0
	Log_Data *p = NULL;
	p = (Log_Data *)arg;
	int num = 0;

	if(p)
	{
		num = atoll(p->SN) % SOCKET_NUM;

		if((!strlen(p->Buff)))
			LogMsg(MSG_ERROR, "The SN is %s, the Buff is %s, the len is %d\n", p->SN, p->Buff, p->len);
		else
		{
			if(num >= 0 && num < SOCKET_NUM)
				Send_Logs_Data(p->Buff, strlen(p->Buff), &Data_Fd.Logs_Fd[num], &Data_Fd.Logs_mutex[num]);
		}
	}
#endif
    char buf[1];

    if (read(fd, buf, 1) != 1) {
       	fprintf(stderr, "Can't read from libevent pipe\n");
        return;
    }

	return ;
}

static void setup_thread(LIBEVENT_THREAD *me)
{
	me->base = event_init();
	
	if (! me->base) {
        fprintf(stderr, "Can't allocate event base\n");
        exit(1);
    }

	event_set(&me->notify_event, me->notify_receive_fd,
              EV_READ | EV_PERSIST, me->handler, me);
    event_base_set(me->base, &me->notify_event);

    if (event_add(&me->notify_event, 0) == -1) {
        fprintf(stderr, "Can't monitor libevent notify pipe\n");
        exit(1);
    }

    me->new_conn_queue = malloc(sizeof(struct conn_queue));
    if (me->new_conn_queue == NULL) {
        perror("Failed to allocate memory for connection queue");
        exit(EXIT_FAILURE);
    }
    cq_init(me->new_conn_queue);
}

static void create_worker(void *(*func)(void *), void *arg) {
    pthread_attr_t  attr;
    int             ret;

    pthread_attr_init(&attr);

    if ((ret = pthread_create(&((LIBEVENT_THREAD*)arg)->thread_id, &attr, func, arg)) != 0) {
        fprintf(stderr, "Can't create thread: %s\n",
                strerror(ret));
        exit(1);
    }
}

static void register_thread_initialized(void) {
    pthread_mutex_lock(&init_lock);
    init_count++;
    pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);
}

static void *worker_libevent(void *arg) {
    LIBEVENT_THREAD *me = arg;

    register_thread_initialized();

    event_base_loop(me->base, 0);

    event_base_free(me->base);

    return NULL;
}

static void wait_for_thread_registration(int nthreads) {
    while (init_count < nthreads) {
        pthread_cond_wait(&init_cond, &init_lock);
    }
}

static void thread_init(LIBEVENT_THREAD **me, int nthreads, tdm_event_handler_pt handler)
{
	int         i;
	LIBEVENT_THREAD *tmp = NULL;

	init_count = 0;
	
	tmp = calloc(nthreads, sizeof(LIBEVENT_THREAD));
	if (!tmp) {
        perror("Can't allocate thread descriptors");
        exit(1);
    }

	for (i = 0; i < nthreads; i++) {
		int fds[2];
        if (pipe(fds)) {
            perror("Can't create notify pipe");
            exit(1);
        }

		tmp[i].notify_receive_fd = fds[0];
		tmp[i].notify_send_fd = fds[1];
		tmp[i].handler = handler;
		setup_thread(&tmp[i]);
	}
	*me = tmp;

	for (i = 0; i < nthreads; i++) {
		create_worker(worker_libevent, &tmp[i]);
	}

	pthread_mutex_lock(&init_lock);
    wait_for_thread_registration(nthreads);
    pthread_mutex_unlock(&init_lock);
}

void thread_init_pool()
{
	pthread_mutex_init(&cqi_freelist_lock, NULL);
	pthread_mutex_init(&cqi_freelist_work_lock, NULL);
	cqi_freelist = NULL;
	cqi_freelist_work = NULL;

	pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);

	thread_init(&threads, RT_POOL_NUM, thread_libevent_process);
	thread_init(&threads_work, POOL_NUM, work_func);
}

void dispatch_conn_new(int sfd, enum conn_states init_state, int event_flags, int read_buffer_size) 
{
    CQ_ITEM *item = cqi_new();
    char buf[1];
    if (item == NULL) {
        close(sfd);
        fprintf(stderr, "Failed to allocate memory for connection object\n");
        return ;
    }   

    int tid = (last_thread + 1) % RT_POOL_NUM;

    LIBEVENT_THREAD *thread = threads + tid;

    last_thread = tid;

    item->sfd = sfd;
    item->init_state = init_state;
    item->event_flags = event_flags;
    item->read_buffer_size = read_buffer_size;
    item->mode = queue_new_conn;

    cq_push(thread->new_conn_queue, item);
	buf[0] = 'c';
    if (write(thread->notify_send_fd, buf, 1) != 1) {
        perror("Writing to thread notify pipe");
    }
}
