
def scope():

    if 'has_SYS_CopyMemory' in rominfo:
        cm_info = rominfo['has_SYS_CopyMemory']
        cm_addr = int(str(cm_info['addr']),0)
        cm_cycs = int(str(cm_info['cycs']),0)
    if 'has_SYS_CopyMemoryExt' in rominfo:
        cme_info = rominfo['has_SYS_CopyMemoryExt']
        cme_addr = int(str(cme_info['addr']),0)
        cme_cycs = int(str(cme_info['cycs']),0)
    if not (cme_info and cm_info):
        error('ROM needs to support SYS_CopyMemory and SYS_CopyMemoryExt')


    # int _exec_pgm(void *ramptr)
    # -- Execute program at address ramptr.
    #    Execution happens after copying the zero page mirror into page zero
    #    and resetting the ctrl bits to their default value. 

    def code0():
        label('_exec_pgm')
        PUSH()

        # insert desired vLR/vPC into page 0 stack
        LDWI(0x80fe);STW(R9);
        LDW(R8);_BNE('.dokepc')
        LDI('sysArgs7')
        label('.dokepc')
        DOKE(R9)
        
        # copy user variables from page 0 mirror to actual page 0
        # starting from this point we cannot use registers anymore
        _LDI(cm_addr);STW('sysFn')
        LDI(0x30);STW('sysArgs0')
        LDWI(0x8030);STW('sysArgs2')
        LDI(0x100-0x30);SYS(cm_cycs)

        # Prepare trampoline, init stack, jump
        LDWI(0xfab4);STW('sysArgs4')
        LDWI(0xff63);STW('sysArgs6')
        LDWI('SYS_ExpanderControl_v4_40');STW('sysFn')
        LDI(0xfe);STW(vSP)
        LDI(0x7c);CALLI('sysArgs4')

    module(name='exec_pgm.s',
           code=[('EXPORT', '_exec_pgm'),
                 ('CODE', '_exec_pgm', code0) ])
        
    # BYTE *load_gt1_addr;
    # UINT load_gt1_len;
    # UINT load_gt1_stream(register const BYTE *p, register UINT n)
    def code1():
        label('load_gt1_stream')              # R8: p  R9: n
        PUSH()
        label('load_gt1_addr', pc()+1)
        LDWI(0);STW(R10)                      # R10: addr
        label('load_gt1_len', pc()+1)
        LDWI(0);STW(R11)                      # R11: len
        # if (n == 0)
        #   return len > 0;
        LDW(R9);_BNE('.lgs0')
        LDW(R11);tryhop(2);POP();RET()
        # Prevent cross page copies
        label('.lgs0')
        LDW(R8);ADDW(R9);SUBI(1);XORW(R8)
        LD(vACH);_BEQ('.lgs1')
        LDW(R8);ORI(255);ADDI(1);SUBW(R8);STW(R9)
        #  if ((int)addr < 0)
        #    _memcpyext(0x70, addr, p, n);
        label('.lgs1')
        LDW(R10);_BGE('.lgs2')
        _LDI(cme_addr);STW('sysFn')
        LDW(R10);STW('sysArgs0')
        LDW(R8);STW('sysArgs2')
        LD(R9);ST(R12);LDI(0x70);ST(R12+1)
        LDW(R12);SYS(cme_cycs)
        _BRA('.lgs4')
        # else if ((int)addr - 255 <= 0)
        #    memcpy(addr + 0x8000u, p, n);
        # else
        #    memcpy(addr, p, n);
        label('.lgs2')
        _LDI(cm_addr);STW('sysFn')
        LDW(R8);STW('sysArgs2')
        LDW(R10);STW('sysArgs0')
        LD(vACH);_BNE('.lgs3')
        LDI(0x80);ST('sysArgs1')
        label('.lgs3')
        LD(R9);SYS(cm_cycs)
        #  load_gt1_addr += n;
        #  load_gt1_len -= n;
        #  return n;
        label('.lgs4')
        _LDI('load_gt1_addr');STW(R22)
        LDW(R10);ADDW(R9);DOKE(R22)
        _LDI('load_gt1_len');STW(R22)
        LDW(R11);SUBW(R9);DOKE(R22)
        LDW(R9);tryhop(2);POP();RET()

    module(name='loadgt1stream.s',
           code=[('EXPORT', 'load_gt1_stream'),
                 ('EXPORT', 'load_gt1_addr'),
                 ('EXPORT', 'load_gt1_len'),
                 ('CODE', 'load_gt1_stream', code1) ] )


    # int _exec_rom(void *romptr)
    # -- Execute rom program

    def code2():
        label("_exec_rom")
        # copy trampoline
        _MOVM('.trampoline', 0xf0, 16, 1)
        # prepare syscalls
        LDW(R8);STW('sysArgs0')
        LDWI('SYS_ExpanderControl_v4_40');STW('sysFn')
        # jump and never come back
        LDI(0);STW(vSP)
        LDI(0x7c);CALLI(0xf0)
        label('.trampoline')
        SYS(40)
        LDWI('SYS_Exec_88');STW('sysFn');
        LDWI(0x200);STW(vLR)
        SYS(88)

    module(name='romasm.s',
           code=[('EXPORT', '_exec_rom'),
                 ('CODE', '_exec_rom', code2)] )

scope()

# Local Variables:
# mode: python
# indent-tabs-mode: ()
# End:
