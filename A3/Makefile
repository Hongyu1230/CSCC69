all: readimage cp ln ls mkdir rm

readimage : readimage.o
	gcc -Wall -g -o readimage $^
	
cp : ext2_cp.o
	gcc -Wall -g -o ext2_cp $^ -lm
	
ln : ext2_ln.o
	gcc -Wall -g -o ext2_ln $^ -lm
	
ls : ext2_ls.o
	gcc -Wall -g -o ext2_ls $^
	
mkdir : ext2_mkdir.o
	gcc -Wall -g -o ext2_mkdir $^ -lm
	
rm : ext2_rm.o
	gcc -Wall -g -o ext2_rm $^ -lm

%.o : %.c ext2.h
	gcc -Wall -g -c $<

clean : 
	rm -f *.o readimage ext2_cp ext2_ln ext2_ls ext2_mkdir ext2_rm *~
