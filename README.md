# ns1nanosynth QuattroFonti

## Introduction

The QuattroFonti is a software/hardware configuration of the soundmachines ns1nanosynth providing extra capabilities via the nanosynth's built-in Arduino computer. QuattroFonti adds four additional LFOs and pseudo-sequencers which dramatically increase the creative expressibility of the nanosynth.

QuattroFonti is an open source project and I envision many modifications of the system. The current version contains a few extra buttons and LEDs attached to the nanosynth but we would like to make simplified versions of QuattroFonti which might operate without special hardware.

## What you need

* 1 soundmachines ns1nanosynth
* 1 breadboard
* 2 breadboard mountable pushbuttons
* 1 RGB LED
* 1 yellow LED
* 4 680 ohm resistors
* QuattroFonti software
* QuattroFonti schematic
* these directions

### Other prerequisites

We assume that you have the Arduino IDE installed on your computer and are able to install new software on the nanosynth's Arduino. The QuattroFonti's software preserves the basic MIDI functionality of the nanosynth and thus you must configure the Arduino IDE to use the special midi-enabled board configuration via the boards manager. see https://github.com/rkistner/arcore

## Instructions 

1. Wire up your nanosynth with the additional components according to the diagram. Note the connections to the digipots - each digipot must be connected to both 5V and 0V reference voltages.

2. Update the Arduino's software with the QuattroFonti software.

## How to use

QuattroFonti uses the three built-in buttons on the nanosynth, plus two extra buttons to control the software on the computer. The two LEDs provide feedback to understand what the software is doing.

### Voltage Sources

QuattroFonti provides four customizable voltage sources. Each one can operate as a pseudo-sequencer or LFO. Each source also can be set into LINKED GATE mode where it provides 5V pulses synced with the previous source's sequence.

All four sources are constantly emitting voltages at all times. However one source is currently "selected". The selected source determines the color of the RGB led:

* Red - Source A
* Green - Source B
* Blue - Source C
* Purple - Source D

The SELECT button determines which source is currently selected. 

The other four buttons modify the currently selected source's settings.

The MODE button cycles between the available modes for the selected source.

#### Modes

Three modes are available:

* PSEUDO-SEQUENCER
* LFO
* LINKED GATE*

In PSEUDO-SEQUENCER mode, the source emits a repeating random sequence of voltages at a steady tempo. 

LFO mode offers triangle and rising/falling saw wave shapes at much slower rates than the analog LFOs provided by the nanosynth. 

LINKED GATE matches the tempo of the previous source and emits synced 5V pulses ideal for connecting to the EG or VCA gate inputs.

> LINKED GATE mode can only be enabled if the PREVIOUS source is set to PSEUDO-SEQUENCER. 
> 
> For example, when Source A (Red) is operating as a PSEUDO-SEQUENCER, Source B (Green) can be switched to LINKED GATE mode, in which case it is emitting gate pulses in sync with Source A's tempo. Every time Source A emits a new voltage, Source B will go high.
> 
> If Source A (Red) is operating as an LFO, Source B (Green) cannot be set in LINKED GATE mode.

All four sources are set as PSEUDO-SEQUENCERS by default.

##### Mode-specific Cotnrols

Buttons 1, 2 and 3 change the settings of the current source, depending on the current's source's mode.
 
###### PSEUDO-SEQUENCER

* Button 1: New random sequence
* Button 2: Slow down
* Button 3: Speed up

###### LFO

* Button 1: Change LFO shape (triangle, rising saw, falling saw)
* Button 2: Slow down
* Button 3: Speed up

###### LINKED GATE:
* Button 1: [currently unimplemented] <!-- cycle between gate modes - ALWAYS, RANDOM LOW, RANDOM HIGH -->
* Button 2: Decrease gate pulse size
* Button 3: Increase gate pulse size

## Future Development

An obvious tweak would be to modify this to work without the supplementary hardware. The onboard voltage sources C1 and C2 could be used to select a source and control the speeds of each. I look forward to seeing what others can do with this project as a foundation.
