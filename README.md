# CockEngine

Cheat interne pour Counter-Strike 2 avec un aimbot basé sur CreateMove et un overlay ESP.

## Fonctionnalités

### Aimbot
- **Visée sur les os** — lit la matrice osseuse directement depuis `CSkeletonInstance` pour un ciblage précis
- **Smooth configurable** — activable/désactivable avec valeur ajustable
- **Limite FOV** — ne cible que dans le champ de vision défini
- **Cercle FOV** — affichage optionnel du rayon de visée à l'écran
- **Team check** — filtrage d'équipe activable
- **Sélection d'os** — Head, Neck, Chest

### Triggerbot
- **Tir automatique** — tire automatiquement quand un ennemi est visé
- **Key bind** — touche configurable
- **Team check** — filtrage d'équipe activable

### ESP
- **Box** — styles standard, coins et rempli
- **Barre de vie** — affichage coloré de la santé
- **Nom & Distance** — identification du joueur et distance
- **Couleurs personnalisables** — chaque élément a des couleurs ajustables

### Visual
- **No Flash** — supprime l'effet de flashbang
- **FOV Changer** — modifie le champ de vision
- **No Fog** — supprime le brouillard

### Misc
- **Bhop** — saut automatique en maintenant espace (via global button state)
- **Watermark** — overlay FPS, ping et nom du cheat
- **Système de config** — sauvegarde/chargement des paramètres dans un fichier `.cfg`

## Build

### Prérequis
- Visual Studio 2022 Build Tools (MSVC v143)
- Windows SDK 10.0

### Cheat DLL
```
build_dll.bat
```

### Injector
```
build_injector.bat
```

## Utilisation

1. Compilez la DLL et l'injecteur avec les fichiers batch fournis
2. Lancez CS2
3. Exécutez `injector.exe` et sélectionnez `cs2_internal.dll`
4. Appuyez sur **INSERT** pour ouvrir le menu

### Touches
| Touche | Action |
|--------|--------|
| INSERT | Ouvrir/fermer le menu |
| END | Décharger le cheat |

## Structure du Projet

```
src/
├── main.cpp              # Point d'entrée, hook Present, DllMain
├── core/
│   ├── includes.h        # Headers communs
│   ├── settings.h        # Tous les settings consolidés
│   ├── config.h          # Sauvegarde/chargement config
│   ├── vector.h          # Vector3, ViewMatrix, WorldToScreen
│   ├── entity.h          # Lecture entités, os, équipes
│   ├── game_offsets.h    # Offsets schema & variables globales
│   ├── cs2_runtime.h     # Init SchemaSystem & résolution offsets
│   ├── schema_system.h   # Interface vtable SchemaSystem
│   └── pattern_scan.h    # Moteur de signature scanning
├── features/
│   ├── aimbot.h          # Paramètres aimbot, DrawFOV, KeyName
│   ├── createmove.h      # Hook CreateMove + logique aimbot + bhop
│   ├── esp.h             # Rendu ESP (boîtes, vie, noms)
│   ├── triggerbot.h      # Logique triggerbot
│   ├── visuals.h         # NoFlash, FOV Changer, NoFog
│   └── menu.h            # UI rendering, ApplyStyle, KeyBinder
└── libs/
    ├── imgui/            # Dear ImGui (v1.90)
    └── kiero/            # Kiero D3D hook + MinHook
```
