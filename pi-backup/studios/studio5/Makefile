all : leader follewer
.PHONY : all
leader : leader.c shared_memory.h
	gcc leader.c -o leader -lrt
follewer : follewer.c shared_memory.h
	gcc follewer.c -o follewer -lrt
