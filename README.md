# Operating System Concepts projects 2022 (at Radboud)
with Phillip Küppers
## Assignment 1
Created a simple shell in C++
<br>Feedback:
After dup2(x,y) a close(x) should be executed to avoid infinite wait in certain situations;
waitpid loop is inside the main loop? should be on line 245.5
## Assignment 2
Created an implementation of a buffer of which  threads might be writing to it or reading from it
<br>Feedback:
- Reasoning in 'Deadlocks and starvation' should be sharper
- Shared data of critical sections not mentioned
- Why not swap lines 70 and 71
## Assignment 3
Optimized given shell code and summarized article "You're doing it wrong" by Kamp and explain the B-Heap design choice for non-expanding elements.
<br>Feedback:
- Summary would be stronger if it was framed in terms of complexity analysis counting the wrong abstraction;
- Soft page faults are when pages are in virtual memory but not in physical memory. 
 The memory allocator uses a COW zero page for faster allocation, such as is done with the arrays. 
 The soft page faults are related to the size of the arrays.
