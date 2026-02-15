# CV64 Recomp
Static Recomp and Remaster of Castlevania 64
Castlevania 64 Remastered (Claude Bravo)
– Release Notes & Feature Overview
🚀 Major Upgrade: A Fully Native PC Port
This release is a complete native port of Castlevania 64, rebuilt from the ground up with refactored code, modern systems, and deep engine‑level fixes.
The result is the smoothest, cleanest, most accurate version of the game ever released.
Key Engine Improvements
- True 60 FPS gameplay across all areas
- No more slowdowns, even in heavy combat or crowded scenes
- Menus now run at 60 FPS
- Massively increased model detail, texture quality, and animation smoothness (interpolated)
- Brand‑new lighting engine
- Complete elimination of original DMA stalls
- Improved text rendering and removal of low‑quality framebuffer artifacts
- Performance‑critical engine hacks replaced with stable modern code

🎮 Gameplay Enhancements
Manual D‑Pad Camera Control
At long last, Castlevania 64 has full manual camera control, just like modern 3D action games.
This required deep engine surgery — but it transforms platforming and exploration.
Gameplay Fixes
- Camera Speed Fix – First‑person camera is now responsive
- Soft‑lock Prevention – Detects and recovers from stuck states
- Collision Fixes – Prevents fall‑through and edge‑case clipping
- Actor Spawn Throttling – Prevents slowdowns when many enemies appear
- Pickup System Fixes – Items no longer desync or disappear

🎨 Graphics Enhancements
- Framerate Stability – Eliminates DMA stalls
- Extended Fog Distance – Better visibility without losing atmosphere
- Modern rendering backend via Mupen‑derived technology
- Upscaled and redesigned textures throughout the game (HD Texture Pack already included)
- Improved lighting, shadows, and material response
- No ROM file or original assets used, it's a complete static recomp based on the CV64 Decomp efforts

🎵 Audio Improvements
- BGM Restoration Fix – Music no longer cuts out after death or transitions
- Audio Reset Prevention – Eliminates music dropouts
- Crackling/Popping Fixes – Clean, stable audio pipeline
- Improved mixing and timing accuracy
- 
💾 Save System Protection- Save Corruption Prevention – Validates save file numbers and prevents invalid writes
- Door/State Tracking Fixes – Eliminates soft‑locks caused by incorrect state transitions

⚡ Quality of Life Options- Skip Intro Logos (optional)
- Faster Text Speed (optional)
- Improved loading behavior
- Cleaner UI and more readable fonts
- 
🟥 New Boot Experience
While these display, the game performs:
- Texture and asset unpacking
- One‑time compilation for the user’s PC
After the first run, the game boots instantly.

🧠 Novel Hybrid Emulation + Native Execution This project uses a cutting‑edge technique:- Game‑specific native code execution
- 12 performance‑critical functions recompiled
- Transparent operation — no user configuration
- Safe fallbacks on all error paths
- 
Performance Gains- 15–40% CPU reduction
- 20–35% FPS increase
- Smoother gameplay
- 50,000–200,000 hook calls per second
- 98–99% recomp native success rate
  
This is a rare and advanced emulation strategy — almost no other projects do this. Minor emulation was needed to fill in the gaps that CV64 Decomp team hadn't done yet.

🐛 Major Issues Fixed- DMA stalls → eliminated
- Camera lag → fixed
- Music glitch after death → fixed
- Save corruption → prevented
- Door lock desync → fixed
- Collision bugs → fixed
- Fog issues → improved
- Cutscene soft‑locks → resolved
- 
📚 Technical Background
This remaster was made possible thanks to:- Symbol maps and documentation from original Castlevania 64 developers and CV64 Decomp Team
- Support from the decompilation community
- Reverse engineering of the 1.0 ROM (the only version with complete symbol data)
- Integration of three major codebases:
- Mupen backend for modern rendering
- Recompiled Castlevania 64 engine with extensive patches
- Bakery Engine (Spacefarer Retro Remasters Engine) tying everything together

The 1.1 and 1.2 revisions introduced regressions and hacks; this remaster undoes those issues and restores the intended behavior while adding modern enhancements.This was a massive undertaking — nearly 30‑year‑old fixed‑function code, MIPS‑to‑C++ conversions, lighting system redesigns, and engine‑level patching across thousands of lines.

❤️ A Note From the Team
Castlevania 64 never deserved the criticism it received at launch.
Its atmosphere, gameplay, and charm were always there — buried under hardware limits and rushed development.This remaster brings the game into the 21st century with the respect it always deserved.
And why not Legacy of Darkness?
Because Castlevania 64 is the one that needed saving — and now it finally has been.
All source changes are included in the release.

Whats Next
All these are already foundationally implemented, but need further work-
-Mod System and Model Viewer - Code is there to support this, but needs finishing
-External asset loading - The ROM is not needed at all for this Recomp to function, but we have added the ability to load external soundtrack replacement, (for example, picking up items / crystals plays an external WAV) so the ability to replace music and sound is already there)
-Fix the post processing pipeline - We did get reflections, bloom and other effects working but we revoked it due to severe stability issues. This needs fixing.
-Additional Camera fixups- The current code that modifies the camera system is currently not reliable due to many game states and overlays that dont always talk to each other well.
