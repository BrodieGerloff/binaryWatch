
# Project Overview

This project is a simple binary watch built as a learning project to de-rust myself on embedded systems and electronics.

The goals for this project:
1. Create a binary watch that I will use myself; namely functioning consistently and low power consumption so I don't need to frequently change the batteries.
2. Write all code without using any libraries, and implement everything myself so I fully understand how everything works.

# Technologies Used

- MCU: ATtiny85 10MHz 
- Eclipse for Embedded
- KiCad

# Techniques Used

- Power consumption is minimized by using an idle state where the LEDs are off and the system is in sleep mode while the time in not advancing. 
- LEDs are charlieplexed using four pins. The ATtiny85 only has 6 GPIO ports, one of which is the RESET pin and ideally not used.

# Using the Watch / Watch states

The watch is controlled by one button and has four states.

### Idle 
All LEDS are off. Pressing/holding the button will switch to Display Time mode.

### Display Time 
Displays the current time for three seconds. Pressing the button again will reset the three seconds timer. Holding for three seconds will switch to Hour Set mode.

### Hour Set 
The hour LEDs will stay on, and the minutes will stay off. Pressing the button will advance one hour. Holding for three seconds will switch to Minute Set mode.

### Minute Set 
The minute LEDs will stay on, and the hours will stay off. Pressing the button will advance one minute. Holding for three seconds will switch to Display Time mode.

# Power Consumption

- Idle / 0 LEDs is too low to be measured by my multimeter
- 5 LEDs will hold at 0.1 uA 
- 9 LEDs will hold at 0.2 uA

A 2032 battery has a capacity of 225 mAh, so a worst case of constant 0.2 uA leaves an expected 1,125,000 hours (128 years). 

Realistically the internal clocks will slow down when the battery drops to 2.7V, so even in a worst case this should last about 10 years.

# To-do

- Redesign board to be smaller; adding a case and wristband makes it too large for what I would like.
- 3D print a case for the watch.
- Power testing. My current tools can't read nA, so I can't properly estimate how long it will last.

# Misc notes

- The clock is not perfect due to the fact 10 MHz / 1024 prescaler = 976.5625Hz. The best case I could find is 49.7664 seconds fast per day; this is mostly accounted for by adding 50 seconds to the length of a day (SECONDS_IN_DAY_CORRECTED).
