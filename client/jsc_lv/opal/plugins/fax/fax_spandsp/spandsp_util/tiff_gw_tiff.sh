#!/bin/sh

PORTA=5000
PORTB=5001
PORTC=5002
PORTD=5003

./spandsp_util -m tiff_to_fax -n fax.tif  -F ${PORTA} -f 127.0.0.1:${PORTB} &

./spandsp_util -V 0 -v -m fax_to_t38              -F ${PORTB} -f 127.0.0.1:${PORTA} \
                                          -T ${PORTC} -t 127.0.0.1:${PORTD} &

./spandsp_util -V 0 -m t38_to_tiff -n test.tif -T ${PORTD} -t 127.0.0.1:${PORTC} &
