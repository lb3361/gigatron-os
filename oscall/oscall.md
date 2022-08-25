

# OSCall routine.



## Calling sequence

The following sequence is used to invoke a GTOS system call.

- Set arguments in sysArgs0-7
- Set syscall number in vAC
- CALLI OSCall
- On return, vAC is positive for success and <0 if an error occurs.


## Error codes

```
0xff00:  ENOSYS-256     Operating system not found
0xff03:  EINVAL-256     Invalid argument
0xff04:  ENOENT-256     File not found
0xff05:  ENOTDIR-256    Not a directory
0xff07:  ENOMEM-256     Out of memory
0xff08:  EIO-256        I/O error
0xff09:  EPERM-256      Permission denied
0xff0a:  ENOTSUP-256    Not supported
```



## GCL implementation of OSCall


```
gcl0x

*=$8ac                                         { must be in low 32K       }

[def _OScall=*                                 { call with \OScall!       }
  push
  4-- %2= $1f8 peek %0=                        { save vAC and ctrlBits    }
  \SYS_ExpanderControl_v4_40 _sysFn= $fc 40!!  { select bank 3            }
  [ $5447 _vLR= $8000 deek _vLR^ if=0          { test $8000 for "GT"      }
    $534f _vLR= $8002 deek _vLR^ if=0          { test $8002 for "OS"      }
    $8004! %2=                                 { call $8004 save result   }
   else
    -256 %2= ]                                 { else error code -256     }
  %0 40!! %2 4++                               { restore ctrlBits, result }
  pop ret
]
```

### GLCC implementation

File  `oscall.s` contains vCPU code with two entry points. 

- Entry point `OSCall` takes arguments in `sysArgs0-7` as explained above.

- Entry point `_oscall` takes up to three word arguments in R8-R10 like
  a regular GLCC routine and copies them into `sysArgs0-5` 
  before jumping into `OSCall`.  This is used by file `oscall.h` which
  defines useful macros for invoking the various syscalls.

