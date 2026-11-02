# ChordHarp — by MIDISOUDRINK

Instrument VST3 / AU inspiré du concept de l'Omnichord : **une touche = un accord**, et une **plaque de slide** qui joue les notes de cet accord en arpège quand on glisse dessus.

## Télécharger

Onglet **Releases** (à droite sur la page du repo) → prends `ChordHarp-macOS.zip` ou `ChordHarp-Windows.zip`, puis suis `INSTALLATION.txt` qui est dans le zip.

Chaque modification poussée sur `main` recompile automatiquement le plugin (onglet **Actions**) et publie une nouvelle release.

## Jouer

| Où | Ce que ça fait |
|---|---|
| Boutons d'accords (souris) | Majeur / mineur / 7e sur les 12 notes, rangées façon Omnichord |
| Plaque SLIDE (souris) | Glisse de haut en bas : joue les notes de l'accord en cascade |
| C2–B2 (FL Studio) | Accords majeurs |
| C3–B3 | Accords mineurs |
| C4–B4 | Accords de 7e |
| C5–C6 | Les 13 cases de la plaque |
| Mod wheel (CC1) | Balaye la plaque — pratique pour enregistrer le slide en automation |
| HOLD | L'accord continue de sonner après avoir relâché la touche |

## Réglages

- **Accord** : volume de la nappe tenue pendant que l'accord est enfoncé
- **Basse** : volume de la basse auto (fondamentale)
- **Slide** : volume de la plaque
- **Durée** : longueur des notes de la plaque
- **Timbre** : doux (triangle) → brillant (carré)
- **Vibrato**, **Reverb**, **Volume**

## Compiler soi-même (optionnel)

```bash
cmake -B build            # télécharge JUCE automatiquement
cmake --build build --config Release
```

Le plugin est dans `build/ChordHarp_artefacts/Release/`.

Code : C++ / [JUCE](https://juce.com) 8. Sons entièrement synthétisés (aucun sample).
