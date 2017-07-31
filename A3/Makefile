functions : ext2_ls.o ext2_cp.o ext2_mkdir.o ext2_ln.o ext2_rm.o ext2_rm_bonus.o
	gcc -Wall -g -o functions $^ -lm

%.o : %.c ext2.h
	gcc -Wall -g -c $<

clean : 
	rm -f *.o functions *~
