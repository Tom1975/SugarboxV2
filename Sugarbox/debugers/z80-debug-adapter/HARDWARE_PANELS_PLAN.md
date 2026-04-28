# Plan — Fenêtres de debug hardware (VS Code extension)

Date : 2026-04-27  
Branche : `feature/gdb_support`

---

## Contexte

Chaque composant hardware de l'Amstrad CPC dispose d'une représentation dans VS Code sous forme de **WebviewPanel indépendant** (onglets librement déplaçables).  
Le modèle existant est `MemoryViewPanel.ts`.

Les données remontent via un `customRequest` vers `Z80DebugSession.ts` qui appelle le `DebugServer` C++ via TCP.

---

## Architecture générale

### Côté TypeScript

```
src/
  HardwarePanel.ts      ← classe de base abstraite (refresh, CSS, lifecycle)
  CrtcAsicPanel.ts      ← panneau unique CRTC/ASIC : bascule selon machine (CPC std → CRTC, CPC+ → ASIC)
  GateArrayPanel.ts
  PpiPanel.ts
  PsgPanel.ts
  FdcPanel.ts
  TapePanel.ts
  main.ts               ← enregistrement commandes + appel refresh sur StoppedEvent
  Z80DebugSession.ts    ← nouveaux customRequests délégués au DebugServer
```

### Côté C++ (`DebugServer.cpp`)

Nouveaux `cmd` JSON à implémenter :

| cmd              | Retour                                            |
|------------------|---------------------------------------------------|
| `getCrtcState`   | registres R0–R17, compteurs internes, type CRTC   |
| `getGateArrayState` | palette 16 couleurs + border, mode, int counter, ROM flags |
| `getAsicState`   | sprites, palette 32 couleurs 12-bit, DMA×3, lock  |
| `getPpiState`    | valeurs ports A/B/C, control register             |
| `getPsgState`    | registres 0–15 bruts + décodé                     |
| `getFdcState`    | MSR, ST0–3, état drives 0 et 1                    |
| `getTapeState`   | chemin, moteur, position, longueur, blocs         |

### Accès C++ depuis `DebugServer`

```cpp
emulation_->GetEngine()->GetCRTC()       // CRTC*
emulation_->GetEngine()->GetGateArray()  // via Motherboard AmstradCore (AmstradCore::GateArray*)
emulation_->GetEngine()->GetPSG()        // Ay8912* (registres + décodage)
emulation_->GetEngine()->GetPPI()        // PPI8255*
emulation_->GetEngine()->GetFDC()        // FDC*
emulation_->GetEngine()->GetTape()       // CTape*
emulation_->GetEngine()->GetAsic()       // Asic* (CPC+ seulement)
```

### Cycle refresh

1. `DebugAdapterTracker.onDidSendMessage` reçoit un `StoppedEvent`
2. → appelle `HardwarePanel.refreshAll()` sur tous les panneaux ouverts
3. Chaque panneau envoie un `customRequest` approprié
4. Bouton "↺ Refresh" dans chaque panneau pour refresh manuel

### Mise en évidence des changements

À chaque refresh, les valeurs modifiées depuis le refresh précédent sont marquées  
(fond légèrement coloré avec `--vscode-diffEditor-insertedTextBackground`).

---

## Composants — détail

---

### 1. CRTC / ASIC (panneau unique)

**Fichier :** `CrtcAsicPanel.ts`  
**Commande :** `z80debug.showCrtcPanel`  
**Titre panneau :** `CRTC [type]` (CPC standard) ou `ASIC (CPC+)` (CPC+)

Le panneau bascule automatiquement selon la machine détectée au démarrage de la session.  
Le `DebugServer` indique le type de machine dans la réponse `getCrtcState` (champ `machineType`).

---

#### Mode CPC standard — données CRTC (`CRTC`)

| Champ C++                   | Description                     | Taille |
|-----------------------------|---------------------------------|--------|
| `registers_list_[0..17]`    | Registres R0–R17 écrits         | 8-bit  |
| `registers_mask_[0..17]`    | Masques de bits utiles          | 8-bit  |
| `status_register_`          | Registre de statut              | 8-bit  |
| `adddress_register_`        | Registre d'adresse sélectionné  | 8-bit  |
| `hcc_`                      | Horizontal character counter    | 8-bit  |
| `vlc_`                      | Vertical line counter           | 8-bit  |
| `vcc_`                      | Vertical character counter      | 8-bit  |
| `vertical_adjust_counter_`  | VA counter                      | 8-bit  |
| `ma_`                       | Memory Address                  | 16-bit |
| `horinzontal_pulse_`        | Pulse H counter                 | 8-bit  |
| `scanline_vbl_`             | Scanline VBL                    | 8-bit  |
| `type_crtc_`                | Type CRTC (0/1/2/3/4)          | enum   |

**Affichage CPC standard :**
- Badge "Type CRTC 0..4" + titre dynamique
- Tableau R0–R17 : nom 6845, valeur hex, valeur binaire (avec masque), description courte
- Bloc compteurs internes : Hcc, Vlc, Vcc, VA, MA, Status, Addr Reg, H.Pulse, Scan VBL

---

#### Mode CPC+ — données ASIC (`Asic`)

- 16 sprites : position X/Y, magnification, priorité, palette associée
- Palette étendue 32 couleurs 12-bit (4 bits R/G/B)
- 3 canaux DMA : adresse liste, longueur, prescaler, pause, loop count
- Raster interrupt
- Split screen
- Status du lock ASIC (verrouillé/déverrouillé)

**Affichage CPC+ :**
- Onglets internes : Sprites | Palette | DMA | Misc
- Sprites : tableau (X, Y, mag, couleur, swatch)
- Palette : grille 32 swatches 12-bit
- DMA : 3 lignes avec adresse/longueur/prescaler/état
- Misc : lock status, raster, split

---

**À préciser :**
- Afficher R0–R17 ou seulement R0–R13 (actifs sur CRTC type 0) ?
- Noms des registres selon spec 6845 (R0=Horizontal Total, R1=H Displayed…) ?
- Accès ASIC depuis C++ : via `Asic*` ou lecture mémoire directe ?
- Les 16 registres sprites : adresses exactes dans le bloc ASIC ?

---

### 2. Gate Array

**Fichier :** `GateArrayPanel.ts`  
**Commande :** `z80debug.showGateArrayPanel`  
**Titre panneau :** `Gate Array`

**Données côté C++ :**

*CPC standard* (AmstradCore `GateArray`) :

| Champ C++       | Description                      |
|-----------------|----------------------------------|
| `inkr_[5]`      | 5 registres écrits (pen sélect + 4 derniers INK) |
| `inksel_`       | Pen sélectionné (0–15 + border)  |
| `inkre_`        | Dernier INK écrit                |
| `mode_`         | Mode vidéo (0/1/2/3)             |
| `border_`       | Couleur border                   |
| `hromen_`       | Lower ROM activée                |
| `lromen_`       | Upper ROM activée                |

*CPC+ (CPCCoreEmu `GateArray`, partiel — ASIC prend le relais)* :

| Champ C++            | Description                         |
|----------------------|-------------------------------------|
| `ink_list_[16]`      | Palette 16 couleurs actuelles       |
| `sprite_ink_list_[16]` | Palette sprites                   |
| `screen_mode_`       | Mode vidéo                          |
| `interrupt_counter_` | Compteur interruption (6 bits)      |
| `interrupt_raised_`  | Interruption en attente             |
| `pen_r_`             | PEN sélectionné                     |

**Affichage :**
- Mode vidéo (badge 0/1/2/3)
- Grille de 16 + 1 swatches de couleur (INK 0–15 + border) avec numéro CPC hardware
- Chaque swatch : numéro INK, valeur hex, couleur RGB visuellement rendue
- Flags : Lower ROM, Upper ROM, IRQ counter

**À préciser :**
- Afficher les couleurs en valeur hardware CPC (0–26) ou RGB ? Les deux ?
- Table de correspondance couleur hardware → RGB à coder côté TS
- Sur CPC+, faut-il afficher la palette ici ou seulement dans ASIC Panel ?

---

### 3. PPI (8255)

**Fichier :** `PpiPanel.ts`
**Commande :** `z80debug.showPpiPanel`  
**Titre panneau :** `PPI 8255`

**Données côté C++ (`PPI8255` + `PPI`) :**

| Champ             | Description                                 |
|-------------------|---------------------------------------------|
| Port A            | Données PSG (bus bidirectionnel)            |
| Port B            | Entrées : VSYNC, EXP, /BUSY, LK1–4, /RD DATA |
| Port C (high)     | BDIR, BC1, /WR DATA, MOTOR (AY control)    |
| Port C (low)      | Numéro ligne clavier sélectionnée           |
| Control register  | Mode I/O des ports                          |

**Affichage :**
- 4 lignes : Port A, Port B, Port C, Control
- Chaque port : valeur hex + binaire annoté (bit par bit)
- Décodage BDIR/BC1 → état AY (INACTIVE/READ/WRITE/LATCH_ADDR)
- Numéro ligne clavier active
- Indicateur moteur cassette

**À préciser :**
- Les valeurs des ports A/B/C sont-elles accessibles directement ou faut-il lire la mémoire I/O ?
- Le `PPI8255` AmstradCore n'expose que des bus lines. Les vraies valeurs sont-elles dans `PPI` (CPCCoreEmu) ?

---

### 4. PSG / AY-3-8912

**Fichier :** `PsgPanel.ts`  
**Commande :** `z80debug.showPsgPanel`  
**Titre panneau :** `PSG (AY-3-8912)`

**Données côté C++ (`Ay8912` via `GetPSG()`) :**

| Registre | Champ C++                 | Description                       |
|----------|---------------------------|-----------------------------------|
| R0–R1    | `channel_a_freq_`         | Période canal A (12 bits)         |
| R2–R3    | `channel_b_freq_`         | Période canal B                   |
| R4–R5    | `channel_c_freq_`         | Période canal C                   |
| R6       | `noise_frequency_`        | Période bruit (5 bits)            |
| R7       | `mixer_control_register_` | Enable tone/noise A/B/C + I/O     |
| R8       | `channel_a_volume_`       | Volume A (+ env flag)             |
| R9       | `channel_b_volume_`       | Volume B                          |
| R10      | `channel_c_volume_`       | Volume C                          |
| R11–R12  | `volume_enveloppe_frequency_` | Période enveloppe             |
| R13      | `volume_enveloppe_shape_` | Forme enveloppe (CONT/ATT/ALT/HOLD) |
| R14      | `register_14_`            | Port A (non utilisé CPC)          |
| R15      | `external_data_register_b_` | Port B (matrice clavier)        |
| —        | `register_[16]`           | Registres bruts                   |

**Affichage :**
- Registres bruts R0–R15 (hex) + décodage sur la droite
- Canal A / B / C : fréquence en Hz calculée, volume (0–15), enable tone, enable noise, envelope
- Bruit : fréquence Hz
- Enveloppe : fréquence Hz, forme avec icône graphique (8 formes standard)
- Mixer : tableau 6 cases (Tone A/B/C, Noise A/B/C)

**À préciser :**
- Formule Hz : `f = 1 000 000 / (16 × période)` (CPC à 1 MHz AY) ?
- Forme enveloppe : afficher les 8 formes valides sous forme de mini-waveform ?

---

### 5. FDC (µPD765)

**Fichier :** `FdcPanel.ts`  
**Commande :** `z80debug.showFdcPanel`  
**Titre panneau :** `FDC (µPD765)`

**Données côté C++ (`FDC`) :**

| Méthode / champ              | Description                          |
|------------------------------|--------------------------------------|
| `GetStatus0()` – `GetStatus3()` | ST0–ST3 (8 bits chacun)           |
| `main_status_`               | Main Status Register (MSR)           |
| `GetCurrentTrack(0/1)`       | Piste courante drive A/B             |
| `GetCurrentSector(0/1)`      | Secteur courant                      |
| `GetCurrentSide(0/1)`        | Face courante                        |
| `IsMotorOn()`                | Moteur drive A                       |
| `IsDiskPresent(0/1)`         | Disque inséré                        |
| `IsDiskWriteProtected(0/1)`  | Protection écriture                  |
| `GetDiskPath(0/1)`           | Chemin fichier DSK                   |
| `GetCurrentDrive()`          | Drive actif (0/1)                    |

**Affichage :**
- MSR décrypté bit par bit (FDC RQM, DIO, NDM, CB, D3B…D0B)
- ST0–ST3 décryptés (EC, SE, IC, HD, US1, US0…)
- Drive A et Drive B côte à côte :
  - Icône disquette + chemin (basename)
  - Indicateur présence + protection
  - Track / Sector / Side courants
  - Voyant moteur (animé ?)
- Indicateur drive actif

**À préciser :**
- Faut-il afficher les drives A et B toujours, ou seulement les drives présents ?
- Afficher le nom du fichier DSK ou le chemin complet ?
- Besoin d'un bouton "Éjecter" / "Charger" depuis le panneau ?

---

### 6. Cassette

**Fichier :** `TapePanel.ts`  
**Commande :** `z80debug.showTapePanel`  
**Titre panneau :** `Cassette`

**Données côté C++ (`CTape`) :**

| Méthode / champ          | Description                            |
|--------------------------|----------------------------------------|
| `GetTapePath()`          | Chemin du fichier tape (.cdt/.wav)     |
| `IsTapeInserted()`       | Cassette présente                      |
| `GetMotor()`             | Moteur en marche                       |
| `GetCounter()`           | Compteur position (secondes)           |
| `LengthOfTape()`         | Longueur totale (secondes)             |
| `GetNbBlocks()`          | Nombre de blocs                        |
| `GetBlockPosition(n)`    | Position bloc n (en unités internes)   |
| `GetTextBlock(n)`        | Nom/description bloc n                 |

**Affichage :**
- Nom du fichier + indicateur "inséré"
- Barre de progression : position / longueur (style cassette)
- Compteur affiché en mm:ss
- Voyant moteur
- Liste des blocs : numéro, nom, position en mm:ss, indicateur "courant"

**À préciser :**
- L'unité de `GetCounter()` / `LengthOfTape()` est en secondes ou en échantillons ?
- `GetBlockPosition` retourne quoi exactement comme unité ?
- Faut-il un bouton "Rembobiner" / "Aller au bloc N" depuis le panneau ?

---

## Extensions hardware futures

Pour les extensions matérielles à venir (Playcity, Digiblaster, MX4, etc.), prévoir :

- Un mécanisme d'enregistrement côté C++ : `DebugServer` expose un `cmd = "getExtHardwareState"` avec un champ `component` (string)
- Côté TS : classe `ExtHardwarePanel` générique — affiche un tableau de registres bruts + noms configurables
- L'extension VS Code reçoit une liste des composants disponibles via `cmd = "listHardwareComponents"` au démarrage
- Chaque composant custom renvoie : `{ name, registers: [{name, value, bits}] }`

---

## Ergonomie VS Code

- Chaque panneau : titre visible dans l'onglet, icône (chip SVG générique)
- Toutes les commandes dans la palette : `Z80 Debug: Show CRTC Panel`, etc.
- Raccourcis clavier optionnels (à définir)
- Les panneaux restent ouverts entre les sessions de debug (option `retainContextWhenHidden: true`)
- Auto-refresh sur chaque `StoppedEvent` — peut être désactivé via setting `z80debug.hardwarePanelsAutoRefresh`
- Highlight diff : les valeurs changées depuis le dernier refresh ont un fond de couleur (`--vscode-diffEditor-insertedTextBackground`)

---

## Ordre d'implémentation suggéré

| Priorité | Composant     | Complexité | Dépendances C++              |
|----------|---------------|------------|------------------------------|
| 1        | Architecture base (`HardwarePanel.ts`) | Faible | MemoryViewPanel (modèle) |
| 2        | CRTC (mode CPC std) | Faible | `GetCRTC()` — tout accessible |
| 3        | Gate Array    | Moyenne    | `GetGateArray()` AmstradCore + table couleurs CPC |
| 4        | PSG           | Faible     | `GetPSG()` — `register_[16]` direct |
| 5        | FDC           | Faible     | `GetFDC()` — tout accessible |
| 6        | PPI           | Moyenne    | À clarifier accès valeurs ports |
| 7        | Cassette      | Faible     | `GetTape()` — tout accessible |
| 8        | ASIC (mode CPC+, dans `CrtcAsicPanel`) | Élevée | Accès registres ASIC à préciser |
| 9        | Extensions    | Élevée     | Architecture extensible C++  |

---

## Points ouverts (à affiner avant de coder)

1. **CRTC** : R0–R17 ou seulement les actifs ? Noms des registres selon spec 6845 ?
2. **CRTC/ASIC** : comment le DebugServer détecte-t-il CPC+ vs CPC standard ? (champ `machineType` dans la réponse)
3. **ASIC** : accès sprites via `Asic*` — quels champs C++ exacts ? Adresses dans le bloc ASIC ?
4. **Gate Array** : affichage couleurs en valeur hardware (0–26) ou RGB ou les deux ?
5. **Gate Array CPC+** : palette 16 couleurs toujours dans GateArrayPanel (la palette ASIC 32 couleurs est dans CrtcAsicPanel) ?
6. **PPI** : les valeurs des ports A/B/C sont dans `PPI8255` (AmstradCore) ou `PPI` (CPCCoreEmu) ?
7. **PSG** : confirmer la formule Hz (1 MHz / 16 / période) ; formes enveloppe graphiquement ?
8. **Cassette** : unités de `GetCounter()` et `GetBlockPosition()` ?
9. **FDC** : drives A/B toujours visibles ou conditionnels ? Boutons d'action (éjecter/charger) ?
10. **Extensions** : format du JSON pour les composants custom ?
11. **Refresh auto** : en mode "run" (pas stoppé), faut-il un polling périodique pour certains panneaux (ex. FDC pendant un chargement) ?
