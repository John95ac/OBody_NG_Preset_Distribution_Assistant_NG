# 📜 SKSE "OBody Preset Distribution Config Assistant NG"

It is a simple but powerful SKSE DLL that processes OBody preset distribution configuration files from INI, paired with SPID to manage presets to the Master OBody_presetDistributionConfig.json automatically without direct intervention. It was created with the objective of automating the application of preset rules for the OBody mod in Skyrim Special Edition, the purpose of this is to facilitate modders in delivering an OBody for their characters, and also adding a default preset which users can easily change from their game.

---

# What does it do?

Upon data loaded, scans Data for OBody_PD_*.ini files, parses rules (Key = Keyselec|PresetA,PresetB,...|Mode), applies to OBody_presetDistributionConfig.json preserving order.

Modes:

```
- " x " : Unlimited – adds presets to JSON every execution (does not change INI); if already exist, no changes.
- " 1 " : Once – adds presets, updates INI to |0 (applies only once, then deactivates).
- " 0 " : No apply – skips rule and logs "Skipped"; INI deactivated, no JSON changes.
- " - " : Removes specific preset – searches with "!" (e.g., "!CCPoundcakeNaked2" removes "CCPoundcakeNaked2"), updates INI to |0.
- " x- " : Removes specific preset every execution – (does not change INI); if already removed, no changes.
- " * " : Removes complete entry from key (e.g., npc or race) – eliminates plugin (e.g., "YurianaWench.esp" and presets), updates INI to |0.
- " x* " : Removes complete entry from key every execution – (does not change INI); if already removed, no changes.
- Any number >=2 or invalid element: Treated as "1" – applies once, updates INI to |0.
```

If JSON cannot be read (e.g., due to execution path discrepancy), process stops for stability to avoid CTD or problems.

Logs actions and summary in OBody_preset_Distribution_Config_Assistant-NG.log.

## INI Rules Examples

```
;OBody_preset_Distribution_Config_Assistant-NG

;Example of code designs: it's very similar to SPID but shorter and simpler.

; npcFormID = xx0001|Preset,...|x , 1 , 0 , - , *             FormID
; npc = EditorID|Preset,...|x , 1 , 0 , - , *                    EditorID name like 000Rabbit_NPC or Serana
; factionFemale = Faction|Preset,...|x , 1 , 0 , - , *           Faction name like "ImperialFaction" or "KhajiitFaction"
; factionMale = Faction|Preset,...|x , 1 , 0 , - , *
; npcPluginFemale = Plugin.esp|Preset,...|x , 1 , 0 , - , *       The name of the esp, who has a defined body
; npcPluginMale = Plugin.esp|Preset,...|x , 1 , 0 , - , *
; raceFemale = Race|Preset,...|x , 1 , 0 , - , *                Work with "NordRace", "OrcRace", "ArgonianRace", "HighElfRace", "WoodElfRace", "DarkElfRace", "BretonRace", "ImperialRace", "KhajiitRace", "RedguardRace", "ElderRace"  or  Works with custom races too
; raceMale = Race|Preset,...|x , 1 , 0 , - , *

; More information about the JSON that these adjustments are applied to in the end is here.
; https://www.nexusmods.com/skyrimspecialedition/articles/4756
```

# Acknowledgements

Special thanks to the SKSE community and CommonLibSSE developers for the foundation. This plugin is based on SKSE templates and my custom parsing logic for INI and JSON. Thanks for the tools that make modding possible.

# CommonLibSSE NG

Because this uses [CommonLibSSE NG](https://github.com/CharmedBaryon/CommonLibSSE-NG), it supports Skyrim SE, AE, GOG, and VR.

[CommonLibSSE NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) is a fork of the popular [powerof3 fork](https://github.com/powerof3/CommonLibSSE) of the _original_ `CommonLibSSE` library created by [Ryan McKenzie](https://github.com/Ryan-rsm-McKenzie) in [2018](https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/commit/224773c424bdb8e36c761810cdff0fcfefda5f4a).

# Requirements

- [SKSE - Skyrim Script Extender](https://skse.silverlock.org/)
- [OBody Next Generation](https://www.nexusmods.com/skyrimspecialedition/mods/77016)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
