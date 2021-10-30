
def scope():

    # WORD _ld_word(const BYTE *ptr);
    # DWORD _ld_dword(const BYTE *ptr);
    # -- Unaligned/page crossing word and dword loads


    def code0():
        nohop()
        label('_ld_word')
        LD(R8);INC(vAC);_BEQ('.ld1')
        LDW(R8);DEEK();RET()
        label('.ld1')
        LDW(R8);PEEK();ST(R9)
        LDI(1);ADDW(R8);PEEK();ST(R9+1)
        LDW(R9);RET()

    def code1():
        label('_ld_dword')
        PUSH();_CALLJ('_ld_word');STW(LAC)
        LDI(2);ADDW(R8);STW(R8);_CALLJ('_ld_word');STW(LAC+2)
        tryhop(2);POP();RET()

    module(name='ffasm.s',
           code=[('EXPORT', '_ld_word'),
                 ('CODE', '_ld_word', code0),
                 ('EXPORT', '_ld_dword'),
                 ('CODE', '_ld_dword', code1) ] )

scope()

# Local Variables:
# mode: python
# indent-tabs-mode: ()
# End:
