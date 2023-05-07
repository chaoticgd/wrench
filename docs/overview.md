# Wrench Overview

This document describes each component of Wrench and how it fits into the system as a whole.

## Data Flow

```
                          ____                                     
 /--\                    |    \___                                 
( () ) ----------------> |  /------. ---\                          
 \--/    wrenchbuild     | /       /    |                          
           unpack        |/_______/     |                          
 Stock                 /    Game        |                    /--\  
ISO File              /  Asset Bank     +-----------------> ( () ) 
                     /    ____          |    wrencbuild      \--/  
                    |    |    \___      |       pack               
                    |    |  /------. ---/                   Modded 
                    |    | /       /                       ISO File
                    |    |/_______/                                
                    |        Mod                                   
                    |    Asset Bank                                
                    |      ^    |                                  
                    |      |    v                                  
                    \--> wrencheditor                              
```

## Programs

### wrenchlauncher

Provides a GUI for unpacking ISO files, managing mods, and packing new builds of the game.

### wrenchbuild

The Wrench Build Tool. This combies the asset system and the engine code in order to implement an asset pipeline for the game. The launcher and editor calls this tool to unpack ISO files and to pack asset banks.

### wrencheditor

The level editor GUI. This is currently the least developed part of Wrench.

### asset2json

Converts [Wrench Text Format](wrench_text_format.md) files to JSON.

### memcard

Memory card editor. Currently only works properly with Deadlocked.

### unpackbin

Unpacks the custom executable file format the game uses, and outputs a standard ELF file. This is normally handled by `wrenchbuild` automatically so this tool is mostly useful for debugging.

### vif

Parses VIF command lists.

## Libraries

### assetmgr

Implements the asset system as described in [asset_system.md](asset_system.md).

### core

Provides basic infrastructure for the other components.

### engine

Most of the code for packing and unpacking the more complicated engine data structures lives here. For example model packing/unpacking, LZ compression, collision, occlusion and more.

### gui

GUI code shared between the launcher and the editor.

### iso

Code for packing and unpacking ISO files.

### toolwads

Code for packing resources to be distributed alongside wrench.

### wtf

Code for handling [Wrench Text Format](wrench_text_format.md) files.
