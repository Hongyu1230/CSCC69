make
./ext2_mkdir twolevel.img /level1/level2/level3
./ext2_mkdir twolevel.img /level1/level2/level3/level4
./ext2_mkdir twolevel.img /level1/level2/level3/level4/level5
./ext2_cp twolevel.img tester /level1/level2/level3/level4/level5
./ext2_cp twolevel.img tester /level1/level2/level3/level4
./ext2_cp twolevel.img tester /level1/level2/level3
./ext2_cp twolevel.img tester /level1/level2
./ext2_cp twolevel.img tester /level1
./ext2_ln twolevel.img /level1/level2/level3/level4/level5/tester /link
./ext2_ln twolevel.img /level1/level2/level3/level4/tester /link2
./ext2_ln twolevel.img /level1/level2/level3/tester /link3
./ext2_ln twolevel.img /level1/level2/tester /link4
./ext2_ln twolevel.img /level1/tester /link5
./ext2_rm_bonus twolevel.img -r /level1/level2/level3/level4
