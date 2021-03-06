Tiny Shell
=====

![C/C++ CI](https://github.com/L1AN0/tinyshell/workflows/C/C++%20CI/badge.svg)

## Additional features

1. In my shell one can execute several commands at once (commands are sparated by `;`)

For example:

```
echo 123 ; echo 5 | base64 ; echo lalal5 | base64 | base64 -d
```

2. If any of the commands in a command with pipe fails to run, then the all the processes created by the command line will be terminated if we run the shell with `-k` flag. For example

```
./shell -k
```

This is the default behaviour of `fish` shell. If `-k` is not passed then the command will still be run and added to the job control (we may use ctrl-c to terminate it if it is in foreground). This is the default behavior of `bash` shell and `dash` shell etc.

## Files

Header files are in `/include`; source files are in `/src`

## To compile the shell
  
Firstly `mkdir -p build && cd build` then `cmake ..` then `make`.

## To run the shell

After building the shell, run

    ./shell

`./shell -h` will print the usage.

## To bring a program foreground:

    tsh> fg <jobid>

for example (I did not remove the & at the end since I found that bash does not remove it either)

    tsh> jobs
    0: sleep 20 &
    1: sleep 30 &
    tsh > fg 0

There is also

    tsh> bg <jobid>

## Others

`cd`, pipe, background & foreground (`jobs`, `fg`, `bg`), `exit` are implemented. errors are handled the job control for commands with pipe is also well implemented.

## TODO

- [ ] Full unit test and mock test.
- [ ] Simple job queue implementation.
- [ ] Autocompletion.
