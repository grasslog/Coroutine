#include "coroutine.h"
#include <stdio.h>

struct args {
	int n;
};

static void		//测试使用的协程执行函数
foo(struct schedule * S, void *ud) {
	struct args * args = ud;
	int start = args->n;
	int i;
	for(i=0; i<5; i++){
		printf("coroutine %d : %d\n", coroutine_running(S), start + i);
		coroutine_yield(S);
	}
}

static void		//关于两个打印协程的测试用例
test(struct schedule * S) {
	struct args args1 = { 0 };
	struct args args2 = { 100 };

	int co1 = coroutine_new(S, foo, &args1);
	int co2 = coroutine_new(S, foo, &args2);
	printf("main start\n");
	while(coroutine_status(S,co1) && coroutine_status(S,co2)) {
		coroutine_resume(S,co1);
		coroutine_resume(S,co2);
	}
	printf("main end\n");
}

int main() {
	struct schedule * S = coroutine_open();
	test(S);
	coroutine_close(S);

	return 0;
}