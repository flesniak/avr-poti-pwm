#!/usr/bin/env python3

from math import exp, log, log2, ceil

current_target=0.6
shunt_ohm=1.8/4
vref=1.1
adc_max=1023
lut_size=256

dead_values_low=-21 # cuts the lowest 20 values
dead_values_high=5 # repeats highest value 5 times
# make relative lut size bigger to cut the dead values afterwards and larger for duplicate values
lut_size_relative=lut_size-dead_values_low-dead_values_high

sense_target=adc_max*shunt_ohm*current_target/vref
sense_target_int=round(sense_target)
print("#include <avr/pgmspace.h>\n")
print("#define SENSE_TARGET {}".format(sense_target_int))
print("#define LUT_SIZE {}".format(lut_size))
print("#define ADC_SHIFT_BITS {}\n".format(ceil(log2(adc_max+1))-ceil(log2(lut_size))))

# exponential growth
# 0 at 0, sense_target at lut_size-1
def exp_lut(lut_size):
  lut = []
  b = log(sense_target+1)/(lut_size-1)
  for div in range(0,lut_size):
    lut.append(round(exp(b*div)-1))
  return lut

# parabolic growth
# 0 at 0, sense_target at lut_size-1, 1 at 1
def para_lut(lut_size):
  lut = []
  a = (sense_target/(lut_size-1)-1)/(lut_size-2)
  b = 1-a
  for x in range(0,lut_size):
    lut.append(round(a*x*x+b*x))
  return lut

# linear growth
def linear_lut(lut_size):
  lut = []
  for div in range(0,lut_size):
    lut.append(round(sense_target*div/(lut_size-1)))
  return lut

def dead_values_fixup(lut, dead_low, dead_high):
  lut = lut[(-1*dead_low if dead_low<0 else 0):(len(lut)+dead_high if dead_high<0 else len(lut))]
  if dead_low>0:
    lut = [lut[0]]*dead_low + lut
  if dead_high>0:
    lut = lut + [lut[len(lut)-1]]*dead_high
  return lut

#lut = linear_lut(lut_size_relative)
lut = exp_lut(lut_size_relative)
#lut = para_lut(lut_size_relative)

lut = dead_values_fixup(lut, dead_values_low, dead_values_high)

print("const uint8_t brightness_lut[{}] PROGMEM = {{".format(lut_size))
for index, chunk in enumerate([lut[i:i+10] for i in range(0, len(lut), 10)]):
  print("  "+",".join(str(v).rjust(3) for v in chunk)+("," if index < len(lut)/10-1 else ""))
print("};")
