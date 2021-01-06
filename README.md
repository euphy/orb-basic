# One Robot Band 

Working title!

A simple wireless network based on ESP32s and IL9341 TFT touchscreens, for building a spatially distributed robot drum kit.


## What it does

Large gallery room. A snare in one corner, a bass drum in another, hi hat in the third and cymbal in the fourth.
As the band play, you create the mix you want by positioning yourself in between the instruments and turning your head.

A live sequencer sends notes to each instrument. The instruments share a mesh network so do not have to be within distance of a single wifi AP. Furthermore they do not need the internet, so can be run in remote places off the grid so long as the instruments stay close together. 

Lack of wires makes for fewer tripping hazards, and mobility. An instrument could be on tracks, moving up and down a room. A group of instruments could be carried by a group of people into an open field, or up a mountain, or to the seaside. An instrument could actually play by driving through a set of chimes, or jumping along a row of drums.

Melodic content is not forbidden, but not the focus of this first iteration either. I am suggesting the use of a MIDI-like note message protocol. 

I'm imagining slightly sophisticated 3D movements of drum beaters to achieve more than simple strikes - eg a rhythmically brushed technique - however, I anticipate this will be controlled by defining a play style in the instrument, and then issuing simple note commands. So instruments may be considered fairly opaque, in the same way a human player is, rather than having all parameters of the instrument exposed to be sequenced. 

The sequencer will set a style (a "patch" maybe) which could be something like "lazy brushed" and allow the machine to interpret that. This keeps the implementation in the unit, and the interface clean and hopefully makes for a more easily hacked drum kit.

Run workshops around building instruments, changing them in real time, while a sequence plods along. This could be code driven where we change the programming of an instrument and upload a new version, or more physical, where we move the instrument to hit different things (with different things). In both cases, the network continues and the instrument just rejoins the mesh when it feels ready.

## Design


### Instruments:

Instruments are standalone machines with a simple interface. They will almost always have a latency as the physical mechanism works. This means that while they can be instructed in real time (ie using direct input) the different latencies of different instruments will make it hard to play in real time.

1.  STROKE DURATION, that is how long it takes to perform the strike, ending with the strike itself. This could be the downward swing of the beater. The sequencer needs to know the maximum stroke time to understand how much warning it must give the instrument that a strike is required. The instrument needs to know this to be able to initiate the motion so that the strike happens at the required time.
2.  STRIKE DURATION, time required to actually do the strike, including pulling the beater back from the surface so the drum can sound. This might be 0 but is the minimum time required to make a noise.
3.  RESET duration, that is how long after a strike that a new stroke can be started. Is likely to be a mechanical resetting of the beater into a position ready to stroke again.

Instruments will report these three things to the sequencer. 

    {
        "name": "left hand snare",
        "id": 1223456988,               (station id in the mesh)
        "role": ["snaredrum", "bassdrum"], (used to target OTA updates)
        "stroke-duration": "integer",   (microseconds)
        "strike-duration": "integer",   (microseconds)
        "reset-duration": "integer"     (microseconds)
    }

Newly joined instruments will start executing instructions when they've been synchronised with the mesh time.


### Sequencer:

The sequencer will send instructions with absolute times, but only as far ahead as it needs, and based on the current time.
The sequencer will use 1 (stroke duration) to decide how far it needs to look ahead.
The sequencer will use 2 and 3 to decide how soon the next stroke can be initiated.

Expect there to be only one sequencer but don't block the idea of multiples.

Instructions will be something like: 

    {
        "command": ["strike", "noteon", "noteoff", "reset"],   (what to actually do)
        "velocity": "integer",                        (how hard to do it)
        "time": "integer"                             (When the strike should happen - 0 means immediately or ASAP)
    }

Instruments can buffer at least one of these instructions but likely to be able to do more than one at a time - perhaps the sequencer could send a bulk packet.

The sequencer will send a ```{"command": "reset", "time": 0}``` when it is stopped (seq:stop or seq:pause). Instruments will clear their buffers when they receive that "reset" and so buffered notes don't play after seq:stop or seq:pause. 

Seq:start after seq:pause will pick up where the play head is at the time of the pause. 
Seq:start after seq:stop will pick up from the beginning of the sequence. 

From an instrument's perspective, there's no difference between a pause and a stop since they both clear the buffer and wait for further instruction.
(Maybe do a time sync now too?)

If, during play, a note is placed in the sequencer ahead of the current position, but inside the look ahead range, it might still be sent out to the instrument. If the instrument doesn't have time to deliver the strike (because of the long stroke duration) then it'll just ignore it.


### Time keeping

The sequencer keeps the canonical musical timebase - seqTime. It can be queried for the current seqTime and instruments will have to do this and make themselves satisfied that they are on-beat before they join in. Where there is no sequencer (either running standalone, or just sequencer not started), an instrument can get time from it's neighbours (use raw meshTime perhaps?). Running standalone doesn't even need timebase though, since commands will be "time": 0, which means immediate.

The sequencer has an internal clock (probably just the ESP32's internal clock).

