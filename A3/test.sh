make
./ext2_cp deleteddirectory.img tester /
./ext2_cp deletedfile.img tester /
./ext2_cp emptydisk.img tester /
./ext2_cp hardlink.img tester /
./ext2_cp hardlink.img tester /level1
./ext2_cp largefile.img tester /
./ext2_cp onedirectory.img tester /
./ext2_cp onedirectory.img tester /level1
./ext2_cp onefile.img tester /
./ext2_cp twolevel.img tester /
./ext2_cp twolevel.img tester /level1
./ext2_cp twolevel.img tester /level1/level2
./ext2_mkdir deleteddirectory.img /newdir
./ext2_mkdir deletedfile.img /newdir
./ext2_mkdir emptydisk.img /newdir
./ext2_mkdir hardlink.img /newdir
./ext2_mkdir hardlink.img /level1/newdir
./ext2_mkdir largefile.img /newdir/
./ext2_mkdir onedirectory.img /newdir/
./ext2_mkdir onedirectory.img /level1/newdir
./ext2_mkdir onefile.img /newdir
./ext2_mkdir twolevel.img /newdir
./ext2_mkdir twolevel.img /level1/newdir
./ext2_mkdir twolevel.img /level1/level2/newdir
./ext2_mkdir twolevel.img /level1/level2/newdir/newdir2
