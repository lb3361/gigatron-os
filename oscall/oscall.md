

# OSCall routine.



## OS call sequence

To invoke a GTOS system call, one needs to sets its
arguments in `sysArgs[0-7]`, load the syscall number in `vAC`
and call the `OSCall` routine with the vCPU instruction `CALLI`.
This subroutine checks that the OS is loaded in bank 3
and transfers control to its entry point. On return it restores
the currently active bank and returns its result code in `vAC`.

## Error codes

Wher the system call runs successfully, the `OSCall` subroutine returns
with a positive value in `vAC` or with one of the following error codes:
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

## Filenames

Absolute filenames follow the pattern `[[drive:]/]path/to/dir/filename.ext`.

The two recognized drives `SD0` and `SD1` correspond to FAT formatted 
SD cards connected to SPI ports 0 and 1 respectively.

Each path component can be at least 26 characters long. Legal characters
are the printable ASCII characters (including space) except the special
characters `:`, `/`, `\`, `*`, `<`, `>`, `|`, `?`, and DEL which are
forbidden in FAT LFN file names. Files whose LFN filenames contains 
non ASCII characters may only be accessed through their short filename.

There is a very limited notion of current drive and current directory which
respectively are the drive and directory that were accessed by the last OS_EXEC 
command which therefore corresponds to location of the currently executing program.
If no drive is specified, the current drive is inserted. If, in addition, the path
does not start with a slash `/`, the current directory is inserted.

The total filename length should not exceed 255 characters

## System calls

### OS_EXEC (syscall 0x01)

Executes a GT1 file.

Arguments:
```
  sysArgs[0-1] : pointer to a GT1 file name
```

Error ENOENT may be reported if the file is not found.
Otherwise, all open file descriptors are closed, the drive and directory containing 
the GT1 file becomes the current drive and current directory and the GT1 file is loaded
in memory, possibly overwriting the current program. If the load
fails, the system hangs with an error message (hopefully). Otherwise
the vCPU control is transfered to the GT1 execution address.

### OS_OPEN (syscall 0x02)

Opens a file descriptor.

Arguments:
```
   sysArgs[0-1] : file descriptor number (3,4, or 5)
   sysArgs[2-3] : pointer to file name
   sysArgs[4-5] : opening flags
```
Calling this function with a valid file descriptor number and 
a null file name pointer closes the corresponding descriptor.
Calling this function with a zero file descriptor and 
a null file name closes all open file descriptors.

Opening flags:
```
   OS_OPEN_RDONLY (0x01)    open existing file for reading
   OS_OPEN_WRONLY (0x02)    open for writing, replacing any existing file
   OS_OPEN_RDWR   (0x03)    open existing file for reading and writing
   OS_OPEN_APPEND (0x0A)    open existing file for appending
```



### OS_READ (syscall 0x03)

### OS_WRITE (syscall 0x04)

### OS_LSEEK (syscall 0x05)

### OS_OPENDIR (syscall 0x06)

### OS_READDIR (syscall 0x07)





## OSCall mplementations

In order to be able to safely switch banks, the `OSCall` subroutine 
must be placed in the low 32K of the address space. 

## GCL implementation

The following code is a canonical GCL implementation of `OSCall`.

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
Entry point `OSCall` takes arguments in `sysArgs0-7` as explained above.
Entry point `_oscall` takes up to four word arguments in R8-R11 like
a regular GLCC routine and copies them into ` vAC` and `sysArgs[0-5]` 
before jumping into `OSCall`.  This alternate entry point is used
to define convenient C macros in file `oscall.h`.

