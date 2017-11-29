I finished Part 1, slicm.
* first, I hoisted all qualified instructions and used several maps and sets to record infomation.
* second, based on the information recorded in the first step, for each load instruction, I did:
** Get Entry Block
** Alloca flag
** Split basic block and create redoBB
** Clone instructions
** Add flag after store
* last, I fix SSA issue with two steps:
** iterated by instruction in redoBB, 
** find all users of hoisted instruction

I didn't finish Part 2, intelligent slicm.

