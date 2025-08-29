Hello

# <WizardDungeon>

This is a Top-down action prototype with spells, and enemy AI.

<img width="1323" height="733" alt="image" src="https://github.com/user-attachments/assets/634855df-9c86-4631-8015-44cd1aa8f93d" />

---

## Table of Contents

- [About](#about)  
- [Requirements](#requirements)
- [Controls](#Controls (Keyboard Only))
- [Getting Started (Clone & Setup)](#getting-started-clone--setup)

---

## About

`WizardDungeon` is an Unreal Engine (Top-Down) prototype that demonstrates:
- Player spells (instant shoot, charge-throw arc, AOE), projectile logic and damage
- Enemy AI with behavior tree tasks/services

---

## Requirements

- **Unreal Engine:** `UE 5.6` 
- **Git** (installed)  
- **Git LFS** (required for `.uasset/.umap` and large media) — https://git-lfs.github.com/  

---

## Controls (Keyboard Only)

- WASD for movement
- Keypad for direction change and shooting
- 1 to switch to the first spell (Also default setting)
- 2 to switch to the second spell
- E for Melee attack

## Getting Started (Clone & Setup)

You can clone via CLI or use GitHub Desktop.

### CLI (recommended)
```bash
# clone repo (use SSH or HTTPS as configured)
git clone git@github.com:yourusername/yourrepo.git
cd yourrepo

# install git-lfs locally if not already
git lfs install

# fetch LFS objects
git lfs pull

# open the project in Unreal:
# - double-click <CPP_TopDown>.uproject
# or from CLI:
# on Windows
start <CPP_TopDown>.uproject
# on macOS
open <CPP_TopDown>.uproject
