# Sound

## VAG

The VAG files are used to store music and voice lines. They contain raw ADPCM samples in the format expected by the PS2's SPUs.

## 989snd

The games use 989snd, an audio library from Sony. Sound effects are stored in sound banks, in the format the library expects. It appears to use a version of the library in which the MIDI functionality has been removed.

The same library was used in Jak & Daxter, and has [already been reverse engineered](https://github.com/open-goal/jak-project/tree/master/game/sound/989snd) by [Ziemas](https://github.com/Ziemas) as part of the [OpenGOAL project](https://opengoal.dev/).

Wrench doesn't currently support 989snd sound banks.
