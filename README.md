# 📜 SKSE "OBody Preset Distribution Config Assistant NG"

It is a simple but powerful SKSE DLL that processes OBody preset distribution configuration files from INI, paired with SPID to manage presets to the Master OBody_presetDistributionConfig.json automatically without direct intervention. It was created with the objective of automating the application of preset rules for the OBody mod in Skyrim Special Edition, the purpose of this is to facilitate modders in delivering an OBody for their characters, and also adding a default preset which users can easily change from their game.

---

# What does it do?

Each time Skyrim is launched after data loaded, the DLL will scan the Data folder for OBody_PD_*.ini files, parse rules like key = plugin|presets|count, and apply them to OBody_presetDistributionConfig.json in SKSE/Plugins.

The first action is to read existing JSON, preserve order of plugins and presets using manual parsing, then add new ones from INI rules without duplicates.

For rules with count >0 or x (unlimited), add presets to sections like "npc", "raceFemale", etc. Update INI count to 0 after application. Skip if count=0.

If no existing JSON, create default structure. Update JSON selectively, preserving pretty-print format and non-modified sections.

Log all processed files, rules applied/skipped, and summary in OBody_preset_Distribution_Config_Assistant-NG.log in Documents/My Games/Skyrim Special Edition/SKSE.

Console message on success: "OBody Assistant: Processed X files, applied Y rules, skipped Z. JSON updated."

That's all for now.

## INI Rules Examples

OBody_preset_Distribution_Config_Assistant-NG

**Example of code designs:** It's very similar to SPID but shorter and simpler.

```
; npcFormID = xx0001|Preset,...|x or 1              FormID
; npc = EditorID|Preset,...|x or 1                  EditorID name like 000Rabbit_NPC or Serana
; factionFemale = Faction|Preset,...|x or 1         Faction name like "ImperialFaction" or "KhajiitFaction"
; factionMale = Faction|Preset,...|x or 1
; npcPluginFemale = Plugin.esp|Preset,...|x or 1        The name of the esp, who has a defined body
; npcPluginMale = Plugin.esp|Preset,...|x or 1
; raceFemale = Race|Preset,...|x or 1               Work with "NordRace", "OrcRace", "ArgonianRace", "HighElfRace", "WoodElfRace", "DarkElfRace", "BretonRace", "ImperialRace", "KhajiitRace", "RedguardRace", "ElderRace"  or  Works with custom races too
; raceMale = Race|Preset,...|x or 1
```

More information about the JSON to which these adjustments are applied in the end is here:
[https://www.nexusmods.com/skyrimspecialedition/articles/4756](https://www.nexusmods.com/skyrimspecialedition/articles/4756)

# Acknowledgements

Special thanks to the SKSE community and CommonLibSSE developers for the foundation. This plugin is based on SKSE templates and my custom parsing logic for INI and JSON. Thanks for the tools that make modding possible.

# CommonLibSSE NG

Because this uses [CommonLibSSE NG](https://github.com/CharmedBaryon/CommonLibSSE-NG), it supports Skyrim SE, AE, GOG, and VR.

[CommonLibSSE NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) is a fork of the popular [powerof3 fork](https://github.com/powerof3/CommonLibSSE) of the _original_ `CommonLibSSE` library created by [Ryan McKenzie](https://github.com/Ryan-rsm-McKenzie) in [2018](https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/commit/224773c424bdb8e36c761810cdff0fcfefda5f4a).

# Requirements

- [SKSE - Skyrim Script Extender](https://skse.silverlock.org/)
- [OBody Next Generation](https://www.nexusmods.com/skyrimspecialedition/mods/77016)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
