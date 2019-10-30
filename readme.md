# proc_topstat

`proc_topstat` is a linux kernel module that provides simple system stats in `/proc/topstat`.

> _i dont really know what i'm doing, so, yeah..._

```
$> cat /proc/topstat 
cpu 2085949 109734
cpu 2086513 108671
cpu 2086203 110817
cpu 2086167 127600
mem 8026148 2993832 1033648
```

## fields

```
cpu <total> <used>
```

- _line number_ implies cpu number.
- `<total>` total _jiffies_ elapsed.
- `<used>` amount of `<total>` in use by programs, that cannot be used by other programs.

```
mem <total> <used> <cached>
```

- `<total>` total kilobytes of memory.
- `<used>` amount of `<total>` in use by programs, that can not be used by other programs.
- `<cached>` amount of `<used>` in use for caches, that can be freed by user.
