si
si
si
si
si
si
si
si
si
si
si
si
si
si
b start.c:58
c
p main
p/x $mepc
b userinit
c
b kernel/proc.c:433
c
p cpus[$tp]->proc->name
delete
b kernel/exec.c:90
c
n
p cpus[$tp]->proc->name