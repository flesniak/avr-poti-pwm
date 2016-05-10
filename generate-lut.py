#!/usr/bin/env python3

current_target=0.6
shunt_ohm=1.8/4
vref=1.1
adc_max=1023
lut_size=64

sense_target=adc_max*shunt_ohm*current_target/vref
sense_target_int=round(sense_target)
print("#define SENSE_TARGET {}".format(sense_target_int))

lut = []

print("uint16_t brightness_lut[64] PROGMEM = {")
for div in range(lut_size-1,0,-1):
  lut.append(round(sense_target*div/(lut_size-1)))
  if len(lut)==10 or div==0:
    print(','.join(str(v).rjust(3) for v in lut)+',') # "0x{:02x}".format(v)
    lut = []
print("};")
