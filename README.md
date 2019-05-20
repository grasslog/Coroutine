# Coroutine
**Coroutines and concurrency are largely orthogonal. Coroutines are a general control structure whereby flow control is cooperatively passed between two different routines without returning.**

**The 'yield' statement in Python is a good example. It creates a coroutine. When the 'yield ' is encountered the current state of the function is saved and control is returned to the calling function. The calling function can then transfer execution back to the yielding function and its state will be restored to the point where the 'yield' was encountered and execution will continue.**
