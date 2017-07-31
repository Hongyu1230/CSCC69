I am unsure if the symlink is supposed to be a relative path or not, currently mine points to the abs path given by the source, 
however when mounting this obviously won't work because the mounted folder is not the root folder, this can be changed if we do a relative path and traverse it

e.g ext2_ln <disk image> -s /abc/cde/a /fgh/link2 would make it so we make link 2 with a relative path like ../abc/cde/a

However I decided against it because this is how a symbolic link would be on a OS

this thread never got answered by a TA, so I don't know what I should do, so I just left it as a abs path(which points to the actual system's abs path, not the disk if you mount it)

https://piazza.com/class/j26ijjr2cqa2if?cid=379


For the bonus, I made it really hacky way by using system function to call rm and rm_bonus for recurse, this sometimes the program makes extra system calls despite the removal already being done, this only happens on the school PC supposedly, however should not effect the result as the file/directory is already removed and the error returned back is usually just something like not found.
