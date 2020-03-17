Tiny Shell
=====

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
  
In the cloned folder run

  make

We can also compile it by firstly `cd /build` then `cmake ..` then `make`.

## To run the shell

After building the shell, run

    ./shell

or if we compiled using `cmake` in the `build` dir as in last section, we can run the shell by `cd build` followed by `./shell`.

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
