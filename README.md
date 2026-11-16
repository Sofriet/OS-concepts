# Operating System Concepts: course projects

Three assignments from the Operating Systems course at Radboud University (2022), by Sofie Vos and Philipp Küppers. Each folder contains our code and the report we handed in (PDF).

| Folder | Topic | Code |
| --- | --- | --- |
| [assignment1](assignment1) | Processes: a Unix shell | `shell.cpp` |
| [assignment2](assignment2) | Synchronization: a thread-safe buffer and log | `main.cpp` |
| [assignment3](assignment3) | Memory: a cache-friendly image filter | `main.cpp` |

## Building

You need Linux or macOS and a C++17 compiler. The code was tested with GCC 13 and Clang 18 on Linux. From the repository root:

```sh
g++ -std=c++17 -o shell assignment1/shell.cpp
g++ -std=c++17 -pthread -o buffer assignment2/main.cpp
g++ -std=c++17 -O2 -o memory assignment3/main.cpp
```

Replace `g++` with `clang++` to build with Clang.

## Assignment 1: a Unix shell

A small shell built on `fork`, `execvp`, `pipe`, `dup2` and `waitpid`. Start it with `./shell`. The prompt shows the current directory, and the shell supports:

- programs with arguments: `ls -l`
- pipes of any length: `cat names.txt | grep -v a | sort -r`
- input redirection on the first command and output redirection on the last: `sort < names.txt > sorted.txt`
- background jobs with a trailing `&`: `sleep 10 &`
- the built-ins `cd <dir>` and `exit`

Arguments are split on spaces only, so there is no quoting, globbing (`*`) or variable expansion. When input doesn't come from a terminal, the prompt is hidden, so you can also pipe commands in:

```sh
printf 'ls | wc -l\nexit\n' | ./shell
```

Report: [OperatingSystemsProject-1.pdf](assignment1/OperatingSystemsProject-1.pdf)

## Assignment 2: synchronization

A buffer that producer threads add to and consumer threads remove from, optionally with a maximum size (`set_bound`), plus a log that every operation writes to and any thread can read.

- `buffer` and `bound` are protected by one mutex, `m_buf`, so changing the bound can't interleave with adding or removing.
- The log is a readers-writers problem. Readers lock `m_rw_log`, a `std::shared_mutex`, in shared mode, so several can read at once; writers lock it exclusively. `m_log` is a turnstile: a writer holds it while it waits, so new readers queue behind the writer and can't starve it.

`./buffer` runs `run_test1()`: 50 threads add to the buffer and 50 remove from it, then the log is printed, ending with `The size of the log is: 100`. `run_test2()` to `run_test5()` test one reader against 50 writers, one writer against 50 readers, 50 of each, and a bounded buffer. To run one of those, call it from `main()` instead. Run one test at a time, because the tests share a global `threads` vector.

To check for data races and lock-order problems, build with ThreadSanitizer. It should print no warnings:

```sh
g++ -std=c++17 -g -fsanitize=thread -pthread -o buffer-tsan assignment2/main.cpp
./buffer-tsan > /dev/null
```

Report: [OperatingSystemsProject-2.pdf](assignment2/OperatingSystemsProject-2.pdf)

## Assignment 3: memory

`./memory` applies a 3×3 averaging filter to a 16384 × 16384 image twice and prints the user time, page faults, peak memory and context switches. It needs about 2 GiB of RAM and runs for roughly 10 seconds.

Swapping the two loops so the inner loop walks through memory in order cut the user time from about 28 s to 8 s on our machine. The report also summarizes Poul-Henning Kamp's article "You're Doing It Wrong" and explains why a B-heap doesn't expand the first generation on a new page.

Report: [OperatingSystemsProject-3.pdf](assignment3/OperatingSystemsProject-3.pdf)


## Feedback from the graders (2022)

Line numbers refer to the code as it was handed in.

**Assignment 1**

- After `dup2(x, y)`, call `close(x)` to avoid waiting forever in some situations. *(fixed)*
- The `waitpid` loop is inside the loop that starts the commands; it should come after it (line 245.5). *(fixed)*

**Assignment 2**

- The reasoning in "Deadlocks and starvation" should be sharper.
- The shared data of the critical sections isn't mentioned.
- Why not swap lines 70 and 71?

**Assignment 3**

- The summary would be stronger if it were framed in terms of complexity analysis counting the wrong abstraction.
- Soft page faults happen when pages are in virtual memory but not in physical memory. The memory allocator uses a copy-on-write zero page for faster allocation, as is done with the arrays. The soft page faults are related to the size of the arrays.
