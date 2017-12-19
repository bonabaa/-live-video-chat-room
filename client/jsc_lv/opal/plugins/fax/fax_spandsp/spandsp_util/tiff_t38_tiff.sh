#!/bin/sh

PORTA=5000
PORTB=5001

./spandsp_util -m tiff_to_t38 -n fax.tif -T ${PORTA} -t 127.0.0.1:${PORTB} &
./spandsp_util -m t38_to_tiff -n test.tif -T ${PORTB} -t 127.0.0.1:${PORTA} 
