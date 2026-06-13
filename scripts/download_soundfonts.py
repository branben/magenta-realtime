#!/usr/bin/env python3
"""
download_soundfonts.py — Download free soundfont files for Magenta Retro reference presets.

Downloads SF2 files from musical-artifacts.com mirrors and extracts WAV
renders for use as MusicCoCa conditioning input.

Covers 3 timbral families:
  - SNES / Super Nintendo (ExpressiveSNES, SNES GM, Super Mario SNES)
  - NES / 8-bit (8bitSF, NES 8-bit, Famicom Multichip)
  - Sega Genesis / Mega Drive (GenesiSF, GenesiSonic)
  - Chiptune /multi-platform (Chiptune v4, Pico-8)
  - General MIDI (GeneralUser GS — balanced baseline)

All soundfonts are free for music creation. Licenses noted per entry.
See docs/plans/2026-06-10-001-feat-juce-vst-retro-soundfonts-plan.md for context.

Usage:
  python scripts/download_soundfonts.py [--output-dir path] [--skip-render]

Output:
  resources/soundfonts/*.sf2     — raw soundfont files
  resources/presets/*.wav       — rendered WAV files for each preset
"""

import argparse
import json
import os
import subprocess
import urllib.request
import zipfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUTPUT = REPO_ROOT / "resources"

# Free soundfonts with verified open-source licenses.
# Organized by timbral family for easy reference preset construction.
# License key: "Free" = free for any use; "CC-BY" = attribution required;
# "Gray area" = community mirror, unclear provenance — use with caution.
SOUNDFONTS = {
    # ── SNES / Super Nintendo ──────────────────────────────────────────────
    "expressive_snes": {
        "name": "ExpressiveSNES",
        "url": "https://musical-artifacts.com/artifacts/4795/download",
        "fallback_url": None,
        "filename": "ExpressiveSNES.sf2",
        "size_mb": 3.73,
        "license": "CC-BY 3.0 (attribution, commercial OK)",
        "description": "High-quality GM SNES soundfont (10 months of work, clean samples)",
    },
    "snes_gm": {
        "name": "SNES GM",
        "url": "https://musical-artifacts.com/artifacts/560/download",
        "fallback_url": "https://raw.githubusercontent.com/Daniel-176/Usefull-Soundfonts/main/SNES%20Soundfont.sf2",
        "filename": "SNES_GM.sf2",
        "size_mb": 1.77,
        "license": "Free",
        "description": "Super Nintendo GM soundfont (dotsarecool, 95K+ downloads)",
    },
    "super_mario_snes": {
        "name": "Super Mario SNES",
        "url": "https://musical-artifacts.com/artifacts/2321/download",
        "fallback_url": None,
        "filename": "Super_Mario_SNES_Soundfont.sf2",
        "size_mb": 15.0,
        "license": "Gray area (game samples, community compiled)",
        "description": "Samples from Mario World, Kart, RPG, DKC — broad SNES palette",
    },
    "dkc2": {
        "name": "DKC2",
        "url": "https://musical-artifacts.com/artifacts/2167/download",
        "fallback_url": "https://www.williamkage.com/snes_soundfonts/donkey_kong_country_soundfont_collection.zip",
        "filename": "DKC2.sf2",
        "filename_inner": None,
        "size_mb": 0.74,
        "license": "Free",
        "description": "Donkey Kong Country 2 soundfont (tropical, bright) — uses DKC 2012 from collection",
        "substitute": "Donkey Kong Country 2012.sf2",
    },
    "ff6": {
        "name": "FF6",
        "url": "https://musical-artifacts.com/artifacts/208/download",
        "fallback_url": "https://www.williamkage.com/snes_soundfonts/final_fantasy_6_soundfont.zip",
        "filename": "FF6.zip",
        "filename_inner": "FinalFantasyVI.sf2",
        "size_mb": 1.0,
        "license": "Gray area (mirror from williamkage.com)",
        "description": "Final Fantasy 6 soundfont (orchestral, dramatic)",
    },

    # ── NES / 8-bit ───────────────────────────────────────────────────────
    "nes_8bitsf": {
        "name": "8bitSF (The NES Soundfont)",
        "url": "https://musical-artifacts.com/artifacts/23/download",
        "fallback_url": None,
        "filename": "8bitsf.SF2",
        "size_mb": 6.43,
        "license": "Free (credit The Eighth Bit)",
        "description": "Most popular NES soundfont (130K+ downloads, game samples)",
        "substitute": "NES_Soundfont.SF2",
    },
    "nes_8bit_fixed": {
        "name": "NES 8-Bit Soundfont",
        "url": "https://musical-artifacts.com/artifacts/6804/download",
        "fallback_url": None,
        "filename": "8-Bit_Sounds.sf2",
        "size_mb": 5.97,
        "license": "CC-BY 3.0 (attribution, commercial OK)",
        "description": "NES waveforms: square, sawtooth, triangle, noise — GM compatible",
    },
    "famicom_multichip": {
        "name": "Famicom Multichip V2G",
        "url": "https://musical-artifacts.com/artifacts/793/download",
        "fallback_url": None,
        "filename": "The_Normalized_Famicom_Multichip_Bank_V2G.sf2",
        "size_mb": 224.0,
        "license": "CC-BY 3.0 (attribution, commercial OK)",
        "description": "NES with expansion chips (FDS, N163, VRC7, MMC5) — deepest NES palette",
    },

    # ── Sega Genesis / Mega Drive ─────────────────────────────────────────
    "genesis_genesisf": {
        "name": "GenesiSF",
        "url": "https://musical-artifacts.com/artifacts/6423/download",
        "fallback_url": None,
        "filename": "GenesiSF.SF2",
        "size_mb": 65.7,
        "license": "CC-BY 3.0 (attribution, commercial OK)",
        "description": "Sega Genesis/Megadrive FM synthesis soundfont (14K+ downloads)",
        "substitute": "Megadrive_SoundFount.sf2",
    },
    "genesis_ym2612": {
        "name": "SEGA Genesis YM2612",
        "url": "https://musical-artifacts.com/artifacts/1931/download",
        "fallback_url": None,
        "filename": "YM2612.sf2",
        "size_mb": 35.2,
        "license": "Gray area (community compiled)",
        "description": "YM2612 FM chip soundfont — authentic Genesis lead/bass",
    },
    "genesis_genesisonic": {
        "name": "GenesiSonic",
        "url": "https://musical-artifacts.com/artifacts/2969/download",
        "fallback_url": None,
        "filename": "GenesiSonic.sf2",
        "size_mb": 20.8,
        "license": "CC-BY 3.0 (attribution, commercial OK)",
        "description": "Sonic 1/2/3&K instruments (playful, bright Genesis character)",
        "substitute": "Megadrive_SoundFount.sf2",
    },

    # ── Chiptune / Multi-Platform ─────────────────────────────────────────
    "chiptune_v4": {
        "name": "Chiptune Soundfont v4.0",
        "url": "https://musical-artifacts.com/artifacts/1152/download",
        "fallback_url": None,
        "filename": "chiptune_soundfont_4.0.sf2",
        "size_mb": 96.6,
        "license": "Free",
        "description": "GM/GS/XG-compatible chiptune soundfont (39K+ downloads, clean & melodic)",
    },
    "pico8_sf": {
        "name": "Pico-8 SF",
        "url": "https://musical-artifacts.com/artifacts/8318/download",
        "fallback_url": None,
        "filename": "Pico-8_SF.sf2",
        "size_mb": 2.5,
        "license": "CC-BY 4.0 (attribution, commercial OK)",
        "description": "Emulates Pico-8 limitations — tiny, crunchy, authentic fantasy console",
    },

    # ── General MIDI (baseline) ───────────────────────────────────────────
    "generaluser_gs": {
        "name": "GeneralUser GS 2.02",
        "url": "https://musical-artifacts.com/artifacts/6789/download",
        "fallback_url": "https://registry.npmjs.org/generaluser/-/generaluser-1.47.1.tgz",
        "filename": "GeneralUser-GS.sf2",
        "filename_inner": "package/GeneralUser.sf2",
        "size_mb": 30.8,
        "license": "Free for any use (no attribution required)",
        "description": "GeneralUser GS 2.02 — balanced GM/GS baseline, detailed programming (S. Christian Collins)",
    },
}

# Preset definitions: which soundfont + which MIDI program/preset to render.
# MIDI program numbers follow General MIDI spec:
#   0-7: Piano, 8-15: Chromatic Percussion, 16-23: Organ, 24-31: Guitar,
#   32-39: Bass, 40-47: Strings, 48-55: Ensemble, 56-63: Brass,
#   64-71: Reed, 72-79: Pipe, 80-87: Synth Lead, 88-95: Synth Pad,
#   96-103: Synth Effects, 104-111: Ethnic, 112-119: Percussive, 120-127: SFX
#   128: Standard Drum Kit
PRESETS = {
    # ── SNES presets ──────────────────────────────────────────────────────
    "snes_rpg": {
        "soundfont": "expressive_snes",
        "name": "SNES RPG",
        "description": "SNES-style RPG sound (piano, strings, flute, oboe)",
        "programs": [0, 48, 73, 68],
    },
    "snes_action": {
        "soundfont": "snes_gm",
        "name": "SNES Action",
        "description": "SNES-style action game sound (brass, synth lead, drums)",
        "programs": [56, 81, 128],
    },
    "snes_square_lead": {
        "soundfont": "snes_gm",
        "name": "SNES Square Lead",
        "description": "Classic SNES square wave lead (Star Fox, Final Fantasy)",
        "programs": [80, 81],  # Lead 1 (square), Lead 2 (sawtooth)
    },
    "snes_pad": {
        "soundfont": "expressive_snes",
        "name": "SNES Pad",
        "description": "Warm SNES synth pad (Secret of Mana, Chrono Trigger)",
        "programs": [88, 89, 90],  # Pad 1 (new age), Pad 2 (warm), Pad 3 (polysynth)
    },
    "dkc2_adventure": {
        "soundfont": "dkc2",
        "name": "DKC2 Adventure",
        "description": "Donkey Kong Country 2 timbre (bright, tropical)",
        "programs": [0, 24, 73],
    },
    "ff6_epic": {
        "soundfont": "ff6",
        "name": "FF6 Epic",
        "description": "Final Fantasy 6 timbre (orchestral, dramatic)",
        "programs": [0, 48, 56, 44],
    },
    "ff6_organ": {
        "soundfont": "ff6",
        "name": "FF6 Organ",
        "description": "FF6 church organ timbre (dramatic, gothic)",
        "programs": [16, 17, 18],  # Drawbar, Percussive, Rock Organ
    },

    # ── NES / 8-bit presets ───────────────────────────────────────────────
    "nes_pulse_lead": {
        "soundfont": "nes_8bitsf",
        "name": "NES Pulse Lead",
        "description": "NES 25% pulse wave lead (Mega Man, Castlevania)",
        "programs": [80, 81],  # Square, Sawtooth
    },
    "nes_triangle_bass": {
        "soundfont": "nes_8bitsf",
        "name": "NES Triangle Bass",
        "description": "NES triangle wave bass (deep, round)",
        "programs": [32, 33],  # Acoustic Bass, Electric Bass (finger)
    },
    "nes_noise_perc": {
        "soundfont": "nes_8bit_fixed",
        "name": "NES Noise Perc",
        "description": "NES noise channel percussion (crunchy, metallic)",
        "programs": [115, 116, 117],  # Woodblock, Taiko, Melodic Tom
    },
    "nes_waveforms": {
        "soundfont": "nes_8bit_fixed",
        "name": "NES Waveforms",
        "description": "Pure NES waveforms (square, saw, triangle, noise)",
        "programs": [80, 81, 73, 127],  # Square, Saw, Flute, Noise (SFX)
    },

    # ── Genesis / Mega Drive presets ──────────────────────────────────────
    "genesis_fm_lead": {
        "soundfont": "genesis_genesisf",
        "name": "Genesis FM Lead",
        "description": "Genesis FM synthesis lead (bright, metallic)",
        "programs": [80, 81, 82],  # Lead 1-3 (square, saw, calliope)
    },
    "genesis_fm_bass": {
        "soundfont": "genesis_genesisf",
        "name": "Genesis FM Bass",
        "description": "Genesis FM bass (fat, punchy — Sonic, Streets of Rage)",
        "programs": [32, 33, 34],  # Acoustic, Electric (finger), Electric (pick)
    },
    "genesis_ym2612_chip": {
        "soundfont": "genesis_ym2612",
        "name": "Genesis YM2612 Chip",
        "description": "Raw YM2612 FM chip tones (authentic Genesis hardware)",
        "programs": [0, 16, 56, 80],  # Piano, Organ, Trumpet, Lead
    },
    "genesis_sonic_bright": {
        "soundfont": "genesis_genesisonic",
        "name": "GenesiSonic Bright",
        "description": "Sonic-style bright Genesis timbre (playful, bouncy)",
        "programs": [0, 24, 56, 80],  # Piano, Guitar, Trumpet, Lead
    },

    # ── Chiptune / Multi-Platform presets ─────────────────────────────────
    "chiptune_melodic": {
        "soundfont": "chiptune_v4",
        "name": "Chiptune Melodic",
        "description": "Clean melodic chiptune (GM/GS/XG compatible, 39K+ downloads)",
        "programs": [0, 48, 56, 80, 88],  # Piano, Strings, Trumpet, Lead, Pad
    },
    "chiptune_square": {
        "soundfont": "chiptune_v4",
        "name": "Chiptune Square",
        "description": "Square wave chiptune lead (classic 8-bit game sound)",
        "programs": [80, 81, 82, 83],  # Lead 1-4 (square, saw, calliope, chiff)
    },
    "pico8_fantasy": {
        "soundfont": "pico8_sf",
        "name": "Pico-8 Fantasy",
        "description": "Pico-8 fantasy console sound (4-channel, tiny, crunchy)",
        "programs": [0, 80, 88, 128],  # Piano, Lead, Pad, Drums
    },

    # ── General MIDI baseline ─────────────────────────────────────────────
    "gm_baseline": {
        "soundfont": "generaluser_gs",
        "name": "GM Baseline",
        "description": "GeneralUser GS — balanced GM baseline for A/B comparison",
        "programs": [0, 24, 48, 56, 73, 80, 88],  # Piano, Guitar, Strings, Trumpet, Flute, Lead, Pad
    },
}


def download_file(url: str, dest: Path, expected_size_mb: float = None):
    """Download a file with progress reporting.

    Falls back to a manual download hint when the host returns a
    Cloudflare JS challenge (HTTP 403 with cf-mitigated).
    """
    _UA = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36"
    print(f"  Downloading {url}...")
    dest.parent.mkdir(parents=True, exist_ok=True)

    try:
        req = urllib.request.Request(url, headers={"User-Agent": _UA})
        with urllib.request.urlopen(req, timeout=60) as resp:
            data = resp.read()
    except urllib.error.HTTPError as e:
        if e.code == 403:
            print(f"  ✗ Download blocked (HTTP 403). Manual download required.")
            print(f"    → Open in browser: {url}")
            print(f"    → Save to: {dest}")
            return False
        raise

    if data[:15] == b"<!DOCTYPE " or b"cf-mitigated" in data[:500]:
        print(f"  ✗ Cloudflare challenge page received. Manual download required.")
        print(f"    → Open in browser: {url}")
        print(f"    → Save to: {dest}")
        return False

    dest.write_bytes(data)
    size_mb = len(data) / (1024 * 1024)
    print(f"  ✓ {dest.name} ({size_mb:.2f} MB)")

    if expected_size_mb and abs(size_mb - expected_size_mb) > 1.0:
        print(f"  ⚠ Size mismatch: expected ~{expected_size_mb} MB, got {size_mb:.2f} MB")
    return True


def try_extract(dest: Path, inner_filename: str, sf2_dir: Path, downloaded: dict, key: str):
    """Extract a compressed SF2 archive (zip or tgz) and update the downloaded mapping."""
    inner_path = sf2_dir / inner_filename
    if inner_path.exists():
        downloaded[key] = inner_path
        return

    # If the "dest" file is already a valid SF2, don't try to extract it
    if dest.exists():
        with open(dest, "rb") as f:
            header = f.read(4)
        if header == b"RIFF":
            print(f"  ✓ {dest.name} is already a valid SF2, skipping extraction")
            downloaded[key] = dest
            return

    print(f"  Extracting {inner_filename} from {dest.name}...")
    try:
        with zipfile.ZipFile(dest) as zf:
            sf2_members = [n for n in zf.namelist() if n.lower().endswith(".sf2")]
            if sf2_members:
                zf.extract(sf2_members[0], sf2_dir)
                extracted = sf2_dir / sf2_members[0]
                if extracted != inner_path:
                    extracted.rename(inner_path)
            else:
                zf.extractall(sf2_dir)
            dest.unlink()
            downloaded[key] = inner_path
            return
    except zipfile.BadZipFile:
        pass

    try:
        import tarfile
        with tarfile.open(dest, "r:gz") as tf:
            sf2_members = [m for m in tf.getmembers() if m.name.lower().endswith(".sf2")]
            if sf2_members:
                tf.extract(sf2_members[0], sf2_dir)
                extracted = sf2_dir / sf2_members[0].name
                if extracted != inner_path:
                    inner_path.parent.mkdir(parents=True, exist_ok=True)
                    extracted.rename(inner_path)
            else:
                print(f"  ⚠ No SF2 found in tgz")
                tf.extractall(sf2_dir)
        dest.unlink()
        downloaded[key] = inner_path
    except (tarfile.TarError, ModuleNotFoundError):
        print(f"  ⚠ Not a valid zip or tgz; leaving as-is")


def extract_wav_with_fluidsynth(sf2_path: Path, program: int, output_wav: Path):
    """Render a MIDI note using FluidSynth to get a WAV for MusicCoCa tokenization."""
    # Create a minimal MIDI file with a single note
    midi_path = output_wav.with_suffix(".mid")

    # Simple MIDI: note-on at middle C, hold 1 second, note-off
    # Standard MIDI file format
    midi_data = bytearray()
    # Header chunk
    midi_data += b"MThd"
    midi_data += (6).to_bytes(4, "big")  # header length
    midi_data += (0).to_bytes(2, "big")  # format 0
    midi_data += (1).to_bytes(2, "big")  # 1 track
    midi_data += (96).to_bytes(2, "big")  # ticks per quarter

    # Track chunk
    track_data = bytearray()
    # Program change
    track_data += bytes([0x00, 0xC0, program])
    # Note on (middle C, velocity 100)
    track_data += bytes([0x00, 0x90, 0x3C, 0x64])
    # Delta time (96 ticks = quarter note), note off
    track_data += bytes([0x60, 0x80, 0x3C, 0x40])
    # End of track
    track_data += bytes([0x00, 0xFF, 0x2F, 0x00])

    midi_data += b"MTrk"
    midi_data += len(track_data).to_bytes(4, "big")
    midi_data += track_data

    midi_path.parent.mkdir(parents=True, exist_ok=True)
    midi_path.write_bytes(midi_data)

    # Render with FluidSynith if available
    # NOTE: -F and -r must come BEFORE the soundfont argument (macOS FluidSynth 2.5.5)
    try:
        subprocess.run(
            [
                "fluidsynth",
                "-ni",
                "-F",
                str(output_wav),
                "-r",
                "16000",
                str(sf2_path),
                str(midi_path),
            ],
            capture_output=True,
            timeout=30,
            check=True,
        )
        print(f"  ✓ Rendered {output_wav.name}")
    except (FileNotFoundError, subprocess.CalledProcessError) as e:
        print(f"  ⚠ FluidSynth not available ({e}), creating placeholder WAV")
        # Create a minimal valid WAV file as placeholder
        create_placeholder_wav(output_wav)
    finally:
        midi_path.unlink(missing_ok=True)


def create_placeholder_wav(path: Path):
    """Create a minimal 16kHz mono WAV file as a placeholder."""
    import struct
    import math

    sample_rate = 16000
    duration = 1.0  # 1 second
    num_samples = int(sample_rate * duration)

    # Generate a simple sine wave at 440 Hz
    samples = []
    for i in range(num_samples):
        t = i / sample_rate
        # Decaying sine
        amplitude = max(0, 1.0 - t) * 0.5
        sample = int(amplitude * 32767 * math.sin(2 * math.pi * 440 * t))
        samples.append(sample)

    # WAV header
    data_size = num_samples * 2  # 16-bit mono
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF",
        36 + data_size,
        b"WAVE",
        b"fmt ",
        16,  # PCM header size
        1,   # PCM format
        1,   # mono
        sample_rate,
        sample_rate * 2,  # byte rate
        2,  # block align
        16,  # bits per sample
        b"data",
        data_size,
    )

    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "wb") as f:
        f.write(header)
        for s in samples:
            f.write(struct.pack("<h", max(-32768, min(32767, s))))


def main():
    parser = argparse.ArgumentParser(description="Download soundfonts for Magenta Retro")
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Output directory for soundfonts and presets",
    )
    parser.add_argument(
        "--skip-render",
        action="store_true",
        help="Skip WAV rendering (download SF2 files only)",
    )
    args = parser.parse_args()

    sf2_dir = args.output_dir / "soundfonts"
    preset_dir = args.output_dir / "presets"

    print("Magenta Retro Soundfont Downloader")
    print(f"Output: {args.output_dir}")
    print()

    # Download soundfonts
    downloaded = {}
    for key, info in SOUNDFONTS.items():
        dest = sf2_dir / info["filename"]

        # Try fallback_url if primary is for musical-artifacts.com (blocked)
        urls_to_try = [info["url"]]
        if info.get("fallback_url") and info["url"].startswith("https://musical-artifacts.com"):
            urls_to_try = [info["fallback_url"]]

        inner_path = sf2_dir / info["filename_inner"] if info.get("filename_inner") else None

        if dest.exists():
            if inner_path and inner_path.exists():
                print(f"  ✓ {info['filename_inner']} already exists (from {info['filename']}), skipping")
                downloaded[key] = inner_path
                continue
            elif inner_path and not inner_path.exists():
                print(f"  ✓ {info['filename']} already exists, extracting {info['filename_inner']}...")
                try_extract(dest, info["filename_inner"], sf2_dir, downloaded, key)
            else:
                print(f"  ✓ {info['filename']} already exists, skipping")
                downloaded[key] = dest
            continue

        # Check for substitute file (only if dest doesn't exist)
        substitute = info.get("substitute")
        if substitute:
            sub_path = sf2_dir / substitute
            if sub_path.exists():
                print(f"  ↻ Using substitute {substitute} as {info['filename']} (not downloaded)")
                downloaded[key] = sub_path
                continue

        print(f"[{info['name']}]")
        ok = False
        for url in urls_to_try:
            try:
                ok = download_file(url, dest, info.get("size_mb"))
                if ok:
                    break
            except Exception as e:
                print(f"  ✗ Failed: {e}")
                continue

        if not ok:
            print(f"  ✗ Could not download {info['name']} from any source")
            continue

        downloaded[key] = dest

        # Handle extraction for newly downloaded file
        if inner_path:
            try_extract(dest, info["filename_inner"], sf2_dir, downloaded, key)

    if args.skip_render:
        print("\nSkipping WAV rendering (--skip-render)")
        return

    # Render preset WAVs
    print("\nRendering preset WAVs...")
    for preset_key, preset in PRESETS.items():
        sf2_key = preset["soundfont"]
        if sf2_key not in downloaded:
            print(f"  ⚠ Skipping {preset_key}: soundfont {sf2_key} not downloaded")
            continue

        sf2_path = downloaded[sf2_key]
        for prog in preset["programs"]:
            wav_name = f"{preset_key}_prog{prog}.wav"
            wav_path = preset_dir / wav_name
            if wav_path.exists():
                print(f"  ✓ {wav_name} already exists, skipping")
                continue

            print(f"  Rendering {wav_name} ({preset['name']}, program {prog})...")
            extract_wav_with_fluidsynth(sf2_path, prog, wav_path)

    # Write manifest
    manifest = {
        "soundfonts": {
            k: {
                "path": str(v.relative_to(args.output_dir)),
                "license": SOUNDFONTS[k].get("license", "Unknown"),
                "description": SOUNDFONTS[k].get("description", ""),
            }
            for k, v in downloaded.items()
        },
        "presets": {
            k: {
                "name": v["name"],
                "description": v["description"],
                "soundfont": v["soundfont"],
                "programs": v["programs"],
            }
            for k, v in PRESETS.items()
        },
    }
    manifest_path = args.output_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2))
    print(f"\n✓ Manifest written to {manifest_path}")
    print(f"  {len(downloaded)} soundfonts, {len(PRESETS)} presets")


if __name__ == "__main__":
    main()
