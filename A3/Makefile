all: cp ln ls mkdir rm rmb
	
cp : ext2_cp.c
	gcc -Wall -g -o ext2_cp $^ -lm
	
ln : ext2_ln.c
	gcc -Wall -g -o ext2_ln $^ -lm
	
ls : ext2_ls.c
	gcc -Wall -g -o ext2_ls $^
	
mkdir : ext2_mkdir.c
	gcc -Wall -g -o ext2_mkdir $^ -lm
	
rm : ext2_rm.c
	gcc -Wall -g -o ext2_rm $^ -lm
	
rmb : ext2_rm_bonus.c
	gcc -Wall -g -o ext2_rm_bonus $^ -lm

%.o : %.c ext2.h
	gcc -Wall -g -c $<

clean : 
	rm -f *.o readimage ext2_cp ext2_ln ext2_ls ext2_mkdir ext2_rm ext2_rm_bonus *~
