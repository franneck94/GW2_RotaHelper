# Open Issues

- Number of Auto Attacks instead of list of auto attacks
- Some builds (daredevil, spectre) show false skills (like aa from mesmer GS)
- The skill cast detection logic needs to be improved
- Addon ICON not loading

# Ideas

Currently: If Skill cast detected we go to next.
Better: Only go to next skill if a new one is casted.
If its the same skill thats also fine.

# TODO

# Reverse Engineering Guide for GW2 (DX11, Windows)

## 1. Set Up Your Environment

- **Debugger:** Install x64dbg (recommended for Windows, supports DX11 games)
- **Disassembler/Decompiler:** Install Ghidra (free), IDA Free, or Radare2
- **Hex Editor:** Install HxD
- **.dat File Tools:**
  - GW2Browser or QuickBMS (with GW2 scripts) for browsing/extracting `.dat` files
- **Prepare the Game Executable:**
  - Locate `Gw2-64.exe` in your GW2 install directory
  - Copy it to a working folder for analysis
- **Optional:** Install DirectX SDK for DX11 symbols

---

## 2. Identify the Target

- **Define your goal:**
  - What function or struct do you want to find? (e.g., player position update, inventory struct)
- **Gather clues:**
  - In-game actions (e.g., moving, opening inventory)
  - Game logs or error messages
  - Community forums or documentation
- **List keywords:**
  - Function names, variable names, or strings related to your target

---

## 3. Static Analysis

- **Open the executable in your disassembler (Ghidra, IDA, etc.):**
  - Load `Gw2-64.exe` and let the tool analyze it.
- **Search for relevant strings:**
  - Use the “Strings” window to find text related to your target (e.g., “inventory”, “player”, “update”).
- **Find references:**
  - Locate where these strings are used in code.
  - Check cross-references (XREFs) to see which functions use them.
- **Analyze function names and signatures:**
  - Look for functions with meaningful names (if symbols are present).
  - If symbols are stripped, look for patterns in code (e.g., function prologues, common DX11 calls).

---

## 4. Dynamic Analysis

- **Run the game in a debugger (x64dbg):**
  - Attach x64dbg to the running `Gw2-64.exe` process.
- **Set breakpoints:**
  - Place breakpoints on suspected functions or code regions.
  - Use hardware breakpoints for memory access if needed.
- **Trigger in-game actions:**
  - Perform the action related to your target (e.g., move character, open inventory).
- **Observe execution:**
  - When a breakpoint hits, note the address and call stack.
  - Step through instructions to understand the flow.
- **Inspect memory:**
  - Use the debugger to view memory regions and registers.
  - Look for struct layouts and data changes.

---

> **Tip:** Use both static and dynamic analysis for best results. Static analysis helps you find possible targets, while dynamic analysis confirms them in real time.
