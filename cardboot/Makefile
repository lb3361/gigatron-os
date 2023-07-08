SHELL=/bin/sh
GIGAROM=../../gigatron-rom
COMPILEGCL=${GIGAROM}/Core/compilegcl.py

PGMS=CardBoot.gt1 CardBoot_v2.gt1

all:${PGMS}

%.gt1: %.gcl
	${COMPILEGCL} -s ${GIGAROM}/interface.json $<

clean: FORCE
	-rm 2>/dev/null ${PGMS}

FORCE:

.PHONY: FORCE

