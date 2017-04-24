# MIPSim
A C++ MIPS simulator for ECEN-4593, Computer Organization

# run it!
```
make
./run_sim <file to run> <i size> <d size> <block size> [--write_through/-wt]
```

You can run with an i/d size of 0 to disable those caches, but all arguments except the `-wt` flag are REQUIRED.
