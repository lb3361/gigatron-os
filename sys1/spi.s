
def scope():

    # these functions overwrite random bytes in page JUNKPAGE
    JUNKPAGE = 0x81
    FFPAGE = 0x82
    FFPAGE_Init = False
    
    # void spi_send(const char *buffer, int len)
    # void spi_recv(char *buffer, int len)

    def code0(): # helper
        nohop()
        label('.helper')
        LDW(R8);STW(R20);LDI(255);ST(R20+1)         # R20 is minus count to end of block
        LDW(R9);_BGT('.spi2')
        _BNE('.spi1')                               # a) len is zero
        POP();RET()
        label('.spi1')
        ADDW(R20);_BRA('.spi4')                     # b) len larger than 0x8000
        label('.spi2')
        ADDW(R20);_BLE('.spi5')
        label('.spi4')                              # c) len larger than -R20
        STW(R9)                                     #    R9 is the remaining len
        LDI(0);SUBW(R20);STW(R20)                   #    R20 is the current block len
        BRA('.spi6')
        label('.spi5')                              # d) len smaller than -R20
        LDW(R9);STW(R20)                            #    R20 is the current block len
        LDI(0);STW(R9)                              #    R9 is zero since this is the last
        label('.spi6')
        RET()

    def code1():
        label('spi_send');
        PUSH()
        LDW(R9);STW(R21)
        label('.sendloop')
        _CALLJ('.helper')
        _LDI('SYS_SpiExchangeBytes_v4_134');STW('sysFn')
        # sysArgs[0]      Page index start, for both send/receive (in, changed)
        # sysArgs[1]      Memory page for send data (in)
        # sysArgs[2]      Page index stop (in)
        # sysArgs[3]      Memory page for receive data (in)
        LDW(R8);STW('sysArgs0')
        ADDW(R20);STW(R8);ST('sysArgs2')
        LDI(JUNKPAGE);ST('sysArgs3')
        SYS(134)
        _BRA('.sendloop')

    def code2():
        label('spi_recv');
        PUSH()
        LDW(R9);STW(R21)
        label('.recvloop')
        _CALLJ('.helper')
        if FFPAGE_Init:
            LDWI('SYS_SetMemory_v2_54');STW('sysFn')    # prep sys
            # sysArgs[0]      Len
            # sysArgs[1]      Val
            # sysArgs[2,3]    Buff
            LD(R20);ST('sysArgs0')
            LDI(255);ST('sysArgs1')
            LD(R8);ST('sysArgs2')
            LDI(JUNKPAGE);ST('sysArgs3')
            SYS(54)
        _LDI('SYS_SpiExchangeBytes_v4_134');STW('sysFn')
        # sysArgs[0]      Page index start, for both send/receive (in, changed)
        # sysArgs[1]      Memory page for send data (in)
        # sysArgs[2]      Page index stop (in)
        # sysArgs[3]      Memory page for receive data (in)
        LDW(R8);STW('sysArgs0')
        ADDW(R20);STW(R8);ST('sysArgs2')
        LD('sysArgs1');ST('sysArgs3')
        LDI(FFPAGE);ST('sysArgs1')
        SYS(134)
        _BRA('.recvloop')

    module(name='spi.s',
           code=[('EXPORT', 'spi_send'),
                 ('EXPORT', 'spi_recv'),
                 ('CODE', 'spi_helper', code0),
                 ('CODE', 'spi_send', code1),
                 ('CODE', 'spi_recv', code2)] )

scope()

# Local Variables:
# mode: python
# indent-tabs-mode: ()
# End:
