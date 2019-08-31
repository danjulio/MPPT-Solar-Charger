; Code-space storage for ADC Reference voltage (mV)
;
; Designed to be accessed through the variable, adcVRefMv,
; defined in adc.c.  This allows a calibrated value to be loaded
; by a manufacturing test programmer.
;
; The default value is 1650 mV (specified internal reference voltage).
;   1650 = 672h
;
; The address is the last 2-bytes in code-space before the bootloader
; signature byte and lock byte
;
CSEG AT 01FFCh
ADC_REF_VAL_HIGH: DB 06h
ADC_REF_VAL_LOW:  DB 72h
END
