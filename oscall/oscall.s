def scope():
    
    if args.cpu < 5:
        fatal(f"oscall requires cpu>=5")
    if not 'has_SYS_CopyMemoryExt' in rominfo:
        fatal(f"oscall requires a more recent rom")


    def code0():
        nohop()

        # C entry point
        #   int _oscall(int oscall_number, ... )
        #   returns error code, -256 if OS not found.

        label('_oscall') 
        LDW(R9);STW('sysArgs0')
        LDW(R10);STW('sysArgs2')
        LDW(R11);STW('sysArgs4')
        LDW(R8)
        
        # ASM entry point
        #   vAC:         oscall number
        #   sysArgs0..5: oscall arguments
        # Returns:
        #   vAC:         error code, -256 if OS not found.

        label('OScall')
        PUSH()
        ALLOC(-4);STLW(2)
        LDWI(0x1f8);PEEK();STLW(0) 
        LDWI('SYS_ExpanderControl_v4_40');STW('sysFn')
        LD(0xfc);SYS(40)
        LDWI(0x5447);STW(vLR);LDWI(0x8000);DEEK();XORW(vLR);_BNE('.abrt') # GT
        LDWI(0x534f);STW(vLR);LDWI(0x8002);DEEK();XORW(vLR);_BEQ('.abrt') # OS
        CALLI(0x8004);STLW(2) # must leave SYS_ExpanderControl in sysFn
        label('.ret')
        LDLW(0);SYS(40);
        LDLW(2);ALLOC(4);POP();RET()
        label('.abrt')
        LDWI(-256);STLW(2);_BRA('.ret')

    module(name='OScall.s',
           code=[('EXPORT', 'OScall'),
                 ('EXPORT', '_oscall'),
                 ('CODE', 'oscall', code0),
                 ('PLACE', 'oscall', 0x0200, 0x7fff) ])

scope()

# Local Variables:
# mode: python
# indent-tabs-mode: ()
# End:
