#!/usr/bin/env python3

from math import exp, log, log2, ceil

current_target=0.6
shunt_ohm=1.8/4
vref=1.1
adc_max=1023
lut_size=256

# for low_values_fixup, DEPRECATED
# current sensing only works for sense_targets > 21 (duty cycle 30 = ~50mA)
# all lower sense_targets will result in the same current
min_measurable_current=0.05 # current below this value will be set to pwm without current measure
max_direct_duty_cycle=59 # duty cycle at which min_measurable_current flows
min_direct_duty_cycle=15 # lowest duty cycle that produces desirable results, this one will be at lut[0]

sense_target=adc_max*shunt_ohm*current_target/vref
sense_target_int=round(sense_target)
print("#include <avr/pgmspace.h>\n")
print("#define SENSE_TARGET {}".format(sense_target_int))
print("#define LUT_SIZE {}".format(lut_size))
print("#define ADC_SHIFT_BITS {}".format(ceil(log2(adc_max+1))-ceil(log2(lut_size))))

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

def low_values_fixup(lut):
  global sense_target # modify global value for algorithm
  limit = next(x for x in lut if x > sense_target*min_measurable_current/current_target) # first entry that is above limit
  limit_index = lut.index(limit) # index of first one not below limit
  print("limit {} index {} limit {}".format(limit, limit_index, sense_target*min_measurable_current/current_target))
  del lut[0:limit_index]
  sense_target = max_direct_duty_cycle-min_direct_duty_cycle
  newlut = linear_lut(limit_index)
  lut[:0] = [x+min_direct_duty_cycle for x in newlut]
  return limit_index

#lut = linear_lut(lut_size)
lut = exp_lut(lut_size)
#lut = para_lut(lut_size)
#print("#define DIRECT_PWM_LIMIT {}\n".format(low_values_fixup(lut)))
print("#define DIRECT_PWM_LIMIT 0\n") # disables low value direct pwm
print("const uint8_t brightness_lut[{}] PROGMEM = {{".format(lut_size))
for index, chunk in enumerate([lut[i:i+10] for i in range(0, len(lut), 10)]):
  print("  "+",".join(str(v).rjust(3) for v in chunk)+("," if index < lut_size/10-1 else "")) # "0x{:02x}".format(v)
print("};")
