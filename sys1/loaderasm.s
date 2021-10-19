
def scope():

    # int _exec_pgm(void *ramptr)
    # -- Copy page zero variables from 

    def code0():
        label('_exec_pgm')
        PUSH()

        # insert trampoline and vLR into page 0 mirror
        _BMOV('.trampoline', 0x80f8, 8, 1)

        # insert desired vLR and vPC into page 0 mirror
        LDWI(0x8000+vPC);STW(R9)
        LDWI(0x8000+vPC+1);STW(R10)
        LD(R8+1);POKE(R10)
        LD(R8);SUBI(2);POKE(R9)
        
        # copy user variables from page 0 mirror to actual page 0
        # starting from this point we cannot use registers anymore
        if not 'has_SYS_CopyMemoryExt' in rominfo:
            error('This program needs SYS_CopyMemoryExt')
        if not 'has_SYS_CopyMemory' in rominfo:
            error('This program needs SYS_CopyMemory')
        else:
            info = rominfo['has_SYS_CopyMemory']
            addr = int(str(info['addr']),0)
            cycs = int(str(info['cycs']),0)
            _LDI(addr);STW('sysFn')
            LDI(0x30);STW('sysArgs0');LDWI(0x8030);STW('sysArgs2')
            LDI(0x100-0x30);SYS(cycs)

        # set sysArgs0, sysFn, vSP, vLR, and call the trampoline
        LDI(vPC);STW('sysArgs0') 
        LDWI(0x8000+vPC);DEEK();STW('sysArgs4')
        LDWI('SYS_ExpanderControl_v4_40');STW('sysFn')
        LDI(0);ST(vSP)
        LDI(0x7c)
        _CALLJ(0x00f8)
        HALT()

        # this is the trampoline at 0xf8
        label('.trampoline')
        SYS(40)
        LDW('sysArgs4')
        STW(vLR)           # Set vLR and vPC to ramptr-2
        DOKE('sysArgs0')   # See https://forum.gigatron.io/viewtopic.php?p=29#p29


    module(name='loaderasm.s',
           code=[('EXPORT', '_exec_pgm'),
                 ('CODE', '_exec_pgm', code0) ] )
    
scope()

# Local Variables:
# mode: python
# indent-tabs-mode: ()
# End:
