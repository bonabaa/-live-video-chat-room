#!/bin/sh

PORTA=5000
PORTB=5001

IP=10.0.2.7

./spandsp_util -v -m tiff_to_fax -n fax.tif -F ${PORTA} -f ${IP}:${PORTB} &
./spandsp_util -v -m fax_to_tiff -n test.tif -F ${PORTB} -f ${IP}:${PORTA} 
