# MIDI Clock Converter

4HP eurorack module that provides two trigger outputs with an adjustable rhythm based on the incoming MIDI timing clock.

![Description.](/docs/description.svg)

MIDI clock (24 PPQN) is read from TRS type A input. MIDI start message resets all clocks.

Adjust output clock rate using the clock divider encoder. Turn it clockwise to decrease the rate and counterclockwise to increase it. Press and hold while turning the knob to set the second clock divider.

Clock outputs provide about 3 milliseconds triggers.

# DIY

## Hardware

See [BOM](https://ivofrolov.github.io/midiclk/ibom.html) and board layout.

Schematic and PCB files are in the [hardware](/hardware) KiCad project folder, which also contains [panel design](/hardware/Panel.FCStd) FreeCad file.

## Firmware

Project relies on [platformio](https://platformio.org/) build system. Though it consists of [single c file](/src/main.c) and can be built using any AVR compiler.

In release mode reset should be disabled (RSTDISBL fuse bit should be programmed) due to lack of available pins.

# More ideas

  * [ ] offset control (half of the note duration)
  * [ ] triplet/dotted note switch
  * [ ] MIDI type A/B switch
  * [ ] gate length control (trigger or note duration)
  * [ ] customizable dividers

# References

  * [ATtiny USI UART](http://www.technoblogy.com/show?RPY=)
  * [Audio Jack Footprints](https://github.com/clacktronics/AudioJacks)
