#pragma

//协程的四种不同的状态
#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

struct schedule;    //调度器结构体

//协程执行函数
typedef void (*coroutine_func)(struct schedule *, void *ud);

//调度器的创建初始化和关闭
struct schedule * coroutine_open(void);
void coroutine_close(struct schedule *);

//协程创建到让出cpu挂起的整过程
int coroutine_new(struct schedule *, coroutine_func, void *ud);
void coroutine_resume(struct schedule *, int fd);
int coroutine_status(struct schedule *, int fd);
int coroutine_running(struct schedule *);
void coroutine_yield(struct schedule *);