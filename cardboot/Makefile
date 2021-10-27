SHELL=/bin/sh
GIGAROM=../../gigatron-rom
COMPILEGCL=${GIGAROM}/Core/compilegcl.py

PGMS=CardBoot.gt1

all:${PGMS}

CardBoot.gt1: CardBoot.gcl
	${COMPILEGCL} -s ${GIGAROM}/interface.json CardBoot.gcl

clean: FORCE
	-rm 2>/dev/null CardBoot.gt1

patch: ${PGMS}
	cp CardBoot.gcl CardBoot.gt1 ${GIGAROM}/Apps/CardBoot/


FORCE:

.PHONY: FORCE

