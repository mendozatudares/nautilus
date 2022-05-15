	// handle a task call as if it does not know
	// about callee-save registers
	// void *nk_task_trampoline(void * (*task)(void *), void*)
.global nk_task_trampoline
nk_task_trampoline:
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15 
	mov %rdi, %rax  // function
	mov %rsi, %rdi  // argument
	subq $256, %rsp // give function some breathing room (HACK)
	callq *%rax
	addq $256, %rsp // unroll breathing room (HACK)
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	retq
