# 📜 SKSE "OBody Preset Distribution Config Assistant NG"

Es un DLL SKSE simple pero potente que procesa archivos de configuración de distribución de presets OBody desde INI, pareados a los SPID para gestionar los presets al JSON Master OBody_presetDistributionConfig.json de forma automática sin intervención directa. Fue creado con el objetivo de automatizar la aplicación de reglas de presets para el mod OBody en Skyrim Special Edition, el fin de esto es facilitar a los modders el poder entregar un OBody de sus personajes, y además agregar un preset por default el cual fácilmente los usuarios podrán cambiar si gustan desde su juego.

---

# What does it do?

Each time Skyrim is launched after data loaded, the DLL will scan the Data folder for OBody_PD_*.ini files, parse rules like key = plugin|presets|count, and apply them to OBody_presetDistributionConfig.json in SKSE/Plugins.

The first action is to read existing JSON, preserve order of plugins and presets using manual parsing, then add new ones from INI rules without duplicates.

For rules with count >0 or x (unlimited), add presets to sections like "npc", "raceFemale", etc. Update INI count to 0 after application. Skip if count=0.

If no existing JSON, create default structure. Update JSON selectively, preserving pretty-print format and non-modified sections.

Log all processed files, rules applied/skipped, and summary in OBody_preset_Distribution_Config_Assistant-NG.log in Documents/My Games/Skyrim Special Edition/SKSE.

Console message on success: "OBody Assistant: Processed X files, applied Y rules, skipped Z. JSON updated."

That's all for now.

That's all for now.

# Acknowledgements

Special thanks to the SKSE community and CommonLibSSE developers for the foundation. This plugin is based on SKSE templates and my custom parsing logic for INI and JSON. Thanks for the tools that make modding possible.

# CommonLibSSE NG

Because this uses [CommonLibSSE NG](https://github.com/CharmedBaryon/CommonLibSSE-NG), it supports Skyrim SE, AE, GOG, and VR.

[CommonLibSSE NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) is a fork of the popular [powerof3 fork](https://github.com/powerof3/CommonLibSSE) of the _original_ `CommonLibSSE` library created by [Ryan McKenzie](https://github.com/Ryan-rsm-McKenzie) in [2018](https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/commit/224773c424bdb8e36c761810cdff0fcfefda5f4a).

# Requirements

- [SKSE - Skyrim Script Extender](https://skse.silverlock.org/)
- [OBody Next Generation](https://www.nexusmods.com/skyrimspecialedition/mods/77016)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
