#include "coroutine.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>

#define STACK_SIZE (1024 * 1024)	//协程调度栈大小 1MB
#define DEFAULT_COROUNTINE 16		// 默认协程容量

struct coroutine;

struct schedule {	//调度器的结构
	char stack[STACK_SIZE];	//调用栈，用来记录协程调用关系的栈
	ucontext_t main;		//协程的上下文
	int nco;	//当前协程存在的数量
	int cap;	//协程的容量
	int running;	//正在运行的协程“id”
	struct coroutine **co;	//协程控制块
};

struct coroutine {
	coroutine_func func;	//执行函数
	void *ud;		//函数参数
	ucontext_t ctx;		//协程上下文
	struct schedule *sch;	//调度器指针
	ptrdiff_t cap;
	ptrdiff_t size;
	int status;		//协程的状态
	char *stack;	//协程的运行栈
};

struct coroutine *	//初始化创建协程
_co_new(struct schedule *S, coroutine_func func, void *ud) {
	struct coroutine * co = malloc(sizeof(*co));
	co->func = func;
	co->ud = ud;
	co->sch = S;
	co->cap = 0;
	co->size = 0;
	co->status = COROUTINE_READY;
	co->stack = NULL;
	return co;
}

void  	//释放协程占有的资源
_co_delete(struct coroutine *co) {
	free(co->stack);
	free(co);
}

struct schedule *	//初始化创建“协程调度器”
coroutine_open(void) {
	struct schedule * S = malloc(sizeof(*S));
	S->nco = 0;
	S->cap = DEFAULT_COROUNTINE;
	S->running = -1;
	S->co = malloc(sizeof(struct coroutine *) * S->cap);
	memset(S->co, 0, sizeof(struct coroutine *) * S->cap);
	return S;
}

void		//关闭协程调度器
coroutine_close(struct schedule * S) {
	int i;
	for(i=0; i<S->cap; i++){
		struct coroutine * co = S->co[i];
		if(co) {
			_co_delete(co);
		}
	}
	free(S->co);
	S->co = NULL;
	free(S);
}

//创建协程，返回协程“id”
int coroutine_new(struct schedule *S, coroutine_func func, void *ud) {
	struct coroutine *co = _co_new(S, func, ud);
	if(S->nco >= S->cap) {
		int id = S->cap;
		S->cap *= 2;
		S->co = realloc(S->co, S->cap * 2 * sizeof(struct coroutine *));
		memset(S->co + S->cap, 0, sizeof(struct coroutine *) * 2);
		S->co[S->cap] = co;
		S->cap *= 2;
		++S->nco;
		return id;
	} else {
		int i;
		for(i=0; i<S->cap; i++) {
			int id = (i+S->nco) % S->cap;
			if(S->co[id] == NULL) {
				S->co[id] = co;
				++S->nco;
				return id;
			}
		}
	}
	assert(0);
	return -1;
}

//协程的执行函数
static void 
mainfunc(struct schedule *S) {
	int id = S->running;
	struct coroutine *C = S->co[id];
	C->func(S,C->ud);
	_co_delete(C);
	S->co[id] = NULL;
	--S->nco;
	S->running = -1;
}

//协程的调用
void coroutine_resume(struct schedule * S, int id) {
	assert(S->running == -1);
	assert(id >= 0 && id < S->cap);
	struct coroutine *C = S->co[id];
	if(C == NULL)
		return ;
	int status = C->status;
	switch(status){
		case COROUTINE_READY:	//当被调用的协程就绪时
			getcontext(&C->ctx);
			C->ctx.uc_stack.ss_sp = S->stack;
			C->ctx.uc_stack.ss_size = STACK_SIZE;
			C->ctx.uc_link = &S->main;
			S->running = id;
			C->status = COROUTINE_RUNNING;
			makecontext(&C->ctx, (void (*)(void))mainfunc, 1, S);
			swapcontext(&S->main, &C->ctx);		//切换上下文，调度器切换到被调度的协程
			break;
		case COROUTINE_SUSPEND:		//当被调用的协程阻塞时
			memcpy(S->stack + STACK_SIZE - C->size, C->stack, C->size);	//？？？
			S->running = id;
			C->status = COROUTINE_RUNNING;	//？？？
			swapcontext(&S->main, &C->ctx);
			break;
		default:
			assert(0);
	}
}

static void		//保存临时的栈变量用于交换
_save_stack(struct coroutine *C, char *top) {
	char dummy = 0;
	assert(top - &dummy <= STACK_SIZE);
	if(C->cap < top - &dummy) {
		free(C->stack);
		C->cap = top - &dummy;
		C->stack = malloc(C->cap);
	}
	C->size = top - &dummy;
	memcpy(C->stack, &dummy, C->size);
}

void	//挂起协程
coroutine_yield(struct schedule * S) {
	int id = S->running;
	assert(id >= 0);
	struct coroutine * C = S->co[id];
	assert((char *)&C > S->stack);
	_save_stack(C, S->stack + STACK_SIZE);
	C->status = COROUTINE_SUSPEND;
	S->running = -1;
	swapcontext(&C->ctx, &S->main);
}

int		//协程的状态
coroutine_status(struct schedule * S, int id) {
	assert(id >= 0 && id < S->cap);
	if(S->co[id] == NULL) {
		return COROUTINE_DEAD;
	}
	return S->co[id]->status;
}

//当前执行的协程“id”
int coroutine_running(struct schedule * S) {
	return S->running;
}