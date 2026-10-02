# OpenSigns

Open-source DIY pitch caller for baseball. A belt-clipped M5StickC Plus2 sends a recorded pitch call over Bluetooth to the catcher's earpiece.

Status: v0.2. It works in real games, and it has real problems, listed below. Full build guide coming soon.

- `firmware/` PlatformIO project for the M5StickC Plus2
- `audio/source/` original recorded pitch calls (fastball, curveball, slider, changeup)
- `tools/wav_to_header.py` turns a WAV recording into the audio file the firmware plays

## Known issues in v0.2

These come from using it in men's league games. Each one lists what happens, why, and what you can do about it today.

### 1. Range gets shaky at full mound distance

**What happens:** the connection holds up close but gets unreliable around the full 60 feet 6 inches from mound to plate.

**Why:** the firmware never sets the Bluetooth transmit power, so the chip runs at its default level. The device sits on a belt, so the wearer's body is between it and the catcher.

**For now:** wear the device on the front of your belt, facing the plate. Test range before the game at full distance.

**Next version:** set transmit power to maximum in firmware and field-test the result.

### 2. Volume is low

**What happens:** the call is hard to hear in the earpiece, especially with crowd and field noise.

**Why:** the recordings are quiet. The loudest point in each clip is around -19 to -21 dB, about a tenth of full volume. They were also recorded as 8-bit, 16 kHz mono, which adds hiss.

**For now:** re-record your calls louder and closer to the mic, then run them through `tools/wav_to_header.py`.

**Next version:** record in 16-bit and have the script normalize every clip to full volume automatically.

### 3. Battery won't last a full game

**What happens:** the sending device needs a battery pack or outlet between innings.

**Why:** the screen stays on at full brightness the whole game, and the Bluetooth audio stream runs nonstop, sending silence between calls.

**For now:** bring a small USB battery pack and charge between innings.

**Next version:** dim or turn off the screen between calls, and only wake the audio stream when sending.

### 4. Other devices steal the earpiece

**What happens:** if a phone or other device the earpiece has paired with has Bluetooth on, the earpiece may connect to it instead.

**Why:** the firmware finds the earpiece by its name, which is hardcoded to one model (`V80013`). Earpieces connect to one device at a time and reconnect to whatever they last paired with.

**For now:** turn off Bluetooth on every other device the earpiece has paired with before the game. To use a different earpiece, change `TARGET_NAME` in `firmware/src/main.cpp`.

**Next version:** a pairing mode on the device, so it pairs with any earpiece and remembers it.

### Also missing

- Only four pitches, and no way to call location.
- The Bluetooth link is not encrypted.
- Nothing warns you when the connection drops.

## Is it legal?

It depends on your league. One-way communication is the key point: the device only talks to the catcher, never back. Check your league's rules before using it in a game.
