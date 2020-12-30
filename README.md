# orb-basic

One Robot Band - working title. 

A simple wireless network based on ESP32s and IL9341 TFT touchscreens, for building a spatially distributed robot drum kit.


# What it does

Large gallery room. A snare in one corner, a bass drum in another, hi hat in the third and cymbal in the fourth.
As the band play, you create the mix you want by positioning yourself in between the instruments and turning your head.

A live sequencer sends notes to each instrument. The instruments share a mesh network so do not have to be within distance of a single wifi AP. Furthermore they do not need the internet, so can be run in remote places off the grid so long as the instruments stay close together. 

Lack of wires makes for fewer tripping hazards, and mobility. An instrument could be on tracks, moving up and down a room. A group of instruments could be carried by a group of people into an open field, or up a mountain, or to the seaside. An instrument could actually play by driving through a set of chimes, or jumping along a row of drums.

Melodic content is not forbidden, but not the focus of this first iteration either. I am suggesting the use of a MIDI-like note message protocol. 

I'm imagining fairly sophisticated 3D movements of drum beaters to achieve more than simple strikes - eg a rhythmically brushed technique - however, I anticipate this will be controlled by defining a play style in the instrument, and then issuing simple note commands. So instruments may be considered fairly opaque, in the same way a human player is, rather than having all parameters of the instrument exposed to be sequenced. 

The sequencer will set a style (a "patch" maybe) which could be something like "lazy brushed" and allow the machine to interpret that. This keeps the implementation in the unit, and the interface clean and hopefully makes for a more easily hacked drum kit.

Run workshops around building instruments, changing them in real time, while a sequence plods along. This could be code driven where we change the programming of an instrument and upload a new version, or more physical, where we move the instrument to hit different things (with different things). In both cases, the network continues and the instrument just rejoins the mesh when it feels ready.
