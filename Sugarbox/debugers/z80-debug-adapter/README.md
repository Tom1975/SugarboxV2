# Z80 Debug Adapter for VS Code

Extension de débogage VS Code pour le développement Amstrad CPC, utilisant l'émulateur **SugarboxV2** comme backend via le protocole DAP (Debug Adapter Protocol).

## Prérequis

- [VS Code](https://code.visualstudio.com/) 1.85+
- [SugarboxV2](https://github.com/...) compilé (binaire `Sugarbox`)
- [RASM](http://www.rasm.assemble.tf/) (assembleur Z80 recommandé)
- Node.js 18+ (pour compiler l'extension)

## Installation

### Build depuis les sources

```bash
cd Sugarbox/debugers/z80-debug-adapter
npm install
npm run compile
```

Packager et installer l'extension (`@vscode/vsce` est inclus dans les devDependencies) :

```bash
npm run package                              # génère z80-debug-0.0.1.vsix
code --install-extension z80-debug-0.0.1.vsix
```

## Setup d'un nouveau projet

### 1. Copier les templates

```bash
cp -r Sugarbox/debugers/z80-debug-adapter/templates/.vscode  <votre-projet>/.vscode
```

Quatre fichiers sont copiés : `launch.json`, `tasks.json`, `settings.json`, `extensions.json`.

### 2. Configurer les réglages du projet

Éditez `.vscode/settings.json` :

```json
{
  "z80debug.entryPoint": "src/main.asm",   // votre fichier source principal
  "z80debug.buildName":  "mygame",         // nom de base des sorties build/

  "terminal.integrated.env.linux": {
    "SUGARBOX": "/home/vous/tools/Sugarbox"
  }
}
```

> **Astuce :** La variable `SUGARBOX` peut aussi être définie une fois pour toutes
> dans vos settings utilisateur VS Code (`Ctrl+Shift+P` → *Open User Settings (JSON)*)
> plutôt que dans chaque projet. Les settings projet `.vscode/settings.json`
> peuvent alors omettre la section `terminal.integrated.env.*`.

### 3. Installer l'extension de syntaxe Z80

VS Code proposera automatiquement d'installer les extensions listées dans
`.vscode/extensions.json`. Acceptez l'installation de **ASM Code Lens**
(`maziac.asm-code-lens`) pour avoir la coloration syntaxique Z80.

Ou installez-la manuellement :

```bash
code --install-extension maziac.asm-code-lens
```

## Configuration

Créez un fichier `.vscode/launch.json` dans votre projet :

### Mode Launch (recommandé)

Démarre l'émulateur automatiquement, charge le média, et attache le debugger.

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "z80",
      "request": "launch",
      "name": "Amstrad CPC - Debug",
      "emulator": "/chemin/vers/Sugarbox",
      "disk": "${workspaceFolder}/build/mygame.dsk",
      "symbolFile": "${workspaceFolder}/build/mygame.rasm",
      "port": 1234
    }
  ]
}
```

### Mode Attach

Attache le debugger à un émulateur déjà lancé.

```bash
# Lancer Sugarbox manuellement avec le serveur de debug :
./Sugarbox --debug --debug_server 1234
```

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "z80",
      "request": "attach",
      "name": "Amstrad CPC - Attach",
      "port": 1234,
      "symbolFile": "${workspaceFolder}/build/mygame.rasm"
    }
  ]
}
```

### Propriétés de configuration

#### Mode `launch`

| Propriété | Type | Défaut | Description |
|-----------|------|--------|-------------|
| `emulator` | string | *(requis)* | Chemin vers le binaire Sugarbox |
| `disk` | string | — | Fichier `.dsk` à insérer dans le lecteur A |
| `tape` | string | — | Fichier `.cdt` ou `.wav` à insérer |
| `snapshot` | string | — | Fichier `.sna` à charger |
| `port` | number | `1234` | Port TCP du serveur de debug |
| `symbolFile` | string | — | Fichier symboles RASM (`.rasm`) |
| `hideEmulator` | boolean | `false` | Cacher la fenêtre de l'émulateur |

#### Mode `attach`

| Propriété | Type | Défaut | Description |
|-----------|------|--------|-------------|
| `port` | number | `1234` | Port TCP du serveur de debug |
| `symbolFile` | string | — | Fichier symboles RASM (`.rasm`) |

## Workflow de développement

### 1. Compiler votre projet avec RASM

Exemple de `tasks.json` :

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Assembler (RASM)",
      "type": "shell",
      "command": "rasm",
      "args": [
        "${workspaceFolder}/src/main.asm",
        "-o", "${workspaceFolder}/build/mygame",
        "-s",                                      
        "-sq", "${workspaceFolder}/build/mygame",  
        "-d", "${workspaceFolder}/build/mygame.dsk"
      ],
      "group": { "kind": "build", "isDefault": true },
      "problemMatcher": []
    }
  ]
}
```

Configurez `preLaunchTask` dans `launch.json` pour assembler automatiquement avant chaque session de debug :

```json
{
  "type": "z80",
  "request": "launch",
  "name": "Amstrad CPC - Debug",
  "preLaunchTask": "Assembler (RASM)",
  "emulator": "/chemin/vers/Sugarbox",
  "disk": "${workspaceFolder}/build/mygame.dsk",
  "symbolFile": "${workspaceFolder}/build/mygame.rasm"
}
```

### 2. Lancer le debug

Appuyez sur **F5**. VS Code va :
1. Exécuter la tâche d'assemblage (`preLaunchTask`)
2. Démarrer Sugarbox avec le disque chargé
3. Attacher le debugger — l'émulateur s'arrête à `PC=0x0000`

### 3. Naviguer dans le code

L'extension ouvre automatiquement une **vue de désassemblage** correspondant à l'adresse courante du PC. Si un fichier symboles RASM est fourni, les labels sont intercalés dans le désassemblage.

## Fonctionnalités

### Contrôle de l'exécution

| Action | Raccourci VS Code |
|--------|-------------------|
| Continuer | F5 |
| Pause | F6 |
| Step Over | F10 |
| Step Into | F11 |
| Step Out | Shift+F11 |
| Restart | Ctrl+Shift+F5 |
| Stop | Shift+F5 |

**Step Over** gère intelligemment les instructions `CALL`, `RST`, `DJNZ`, et les instructions de bloc (`LDIR`, `LDDR`, etc.) : le debugger ne rentre pas dedans.

**Step Out** lit l'adresse de retour sur la pile et pose un breakpoint temporaire dessus.

### Breakpoints

Deux types de breakpoints sont supportés :

- **Breakpoints sur le désassemblage** : cliquez dans la marge de la vue désassemblage. Si la ligne cliquée est un label, le breakpoint est posé sur l'instruction suivante.
- **Instruction breakpoints** : depuis la vue "Disassembly" de VS Code (ouverte via *Open Disassembly View* dans le menu contextuel d'un registre).

Les deux types coexistent et sont fusionnés avant d'être envoyés à l'émulateur.

### Registres

Le panneau **Variables > Registers** affiche tous les registres Z80 :

```
AF   0x1234    HL'  0x0000
AF'  0x0000    SP   0xFF00
BC   0x4000    PC   0x5A00
BC'  0x0000    IX   0x0000
DE   0x8000    IY   0x0000
DE'  0x0000
```

- Les registres **16 bits** (BC, DE, HL, SP, PC, IX, IY, BC', DE', HL') proposent un lien *Open Memory View* ou *Open Disassembly View* à leur adresse.
- Double-cliquer sur la valeur d'un registre permet de **l'éditer**.

### Pile (Stack)

Le panneau **Variables > Stack** affiche les 16 premiers mots sur la pile, avec leur adresse.

### Mémoire

Clic droit sur un registre 16 bits → *Open Memory View* pour inspecter et éditer la mémoire Z80 à cette adresse.

Le `writeMemoryRequest` invalide automatiquement le cache de désassemblage de la page modifiée.

### Console de debug (Evaluate)

La console de debug de VS Code accepte des expressions :

| Expression | Description |
|------------|-------------|
| `af`, `bc`, `hl`, `sp`, `pc`, ... | Valeur d'un registre |
| `read:0x4000` | Lecture en mémoire mappée |
| `ram:0x1234` | Lecture RAM (bank calculée depuis l'adresse) |
| `ram[2]:0x0000` | Lecture RAM bank 2, offset 0x0000 |
| `rom:0x0000` | Lecture ROM paginée courante |
| `rom[1]:0x0000` | Lecture ROM bank 1 |
| `cart[0]:0x0000` | Lecture cartouche bank 0 |
| `write:0x4000=0xFF` | Écriture en mémoire |

### Symboles RASM

Le fichier symboles (option `symbolFile`) est un fichier super-symbol RASM (`.rasm`) généré avec l'option `-sq` de RASM. Il contient les labels avec leur adresse et leur bank.

Les labels sont automatiquement intercalés dans le désassemblage virtuel :

```
GAME_LOOP:
0x5A00  LD A,(0x5C00)
0x5A03  CP #FF
0x5A05  JR Z,GAME_OVER

GAME_OVER:
0x5A07  HALT
```

## Architecture technique

L'extension est composée de trois couches :

```
VS Code (DAP client)
    ↕  DAP over stdio
Z80DebugSession.ts  (debug adapter)
    ↕  JSON/TCP port 1234
DebugServer.cpp  (SugarboxV2)
    ↕  appels directs
Emulation.cpp / Machine.cpp / Z80
```

### Lancement de l'émulateur

En mode `launch`, l'adapter :
1. Génère un script CSL temporaire dans `$TMPDIR` si un média est fourni
2. Lance Sugarbox : `Sugarbox --debug --debug_server 1234 [--csl /tmp/sugarbox_xxx.csl] [--hide]`
3. Attend que le port TCP soit disponible (retry toutes les 250 ms, timeout 10 s)
4. Se connecte et envoie `InitializedEvent`

À la déconnexion, le processus Sugarbox est tué automatiquement.

## Limitations connues

- **Source breakpoints sur fichiers `.asm` réels** non supportés (les breakpoints fonctionnent uniquement sur le désassemblage virtuel). Le support nécessite un fichier de listing RASM avec la correspondance ligne → adresse.
- Un seul thread Z80 est exposé au debugger.
- Le timeout de réponse de l'émulateur est fixé à 1 s par commande.
