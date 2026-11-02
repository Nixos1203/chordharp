# ChordHarp — by MIDISOUDRINK

Instrument VST3 / AU inspiré du concept de l'Omnichord : **une touche = un accord**, et une **plaque de slide** qui joue les notes de cet accord en arpège quand on glisse dessus.

## Télécharger

Onglet **Releases** (à droite sur la page du repo) → prends `ChordHarp-macOS.zip` ou `ChordHarp-Windows.zip`, puis suis `INSTALLATION.txt` qui est dans le zip.

Chaque modification poussée sur `main` recompile automatiquement le plugin (onglet **Actions**) et publie une nouvelle release.

## Jouer

| Où | Ce que ça fait |
|---|---|
| Boutons d'accords (souris) | Majeur / mineur / 7e sur les 12 notes, rangées façon Omnichord |
| Pad SLIDE (souris) | Une seule surface, comme la strumplate : **haut/bas** = les notes de l'accord sur 4 octaves, **gauche/droite** = le timbre (doux → brillant) |
| PRESETS | 20 sons en 5 univers : Classique, Cinéma, Trance, Rétro, Ambient |
| C2–B2 (FL Studio) | Accords majeurs |
| C3–B3 | Accords mineurs |
| C4–B4 | Accords de 7e |
| C5–C6 | Les 13 cases de la plaque |
| Mod wheel (CC1) | Balaye la plaque — pratique pour enregistrer le slide en automation |
| CC74 | Timbre de la plaque (= axe gauche/droite du pad) |
| HOLD | L'accord continue de sonner après avoir relâché la touche |

## Réglages

- **Son du slide** : 10 moteurs (Omni 1/2 analogiques, harpe / guitare / banjo en cordes pincées, vibraphone, piano électrique, cristal, synth pluck, boîte à musique)
- **Couche** : un 2e son superposé au slide (octave -1 à +2, niveau réglable)
- **Son de l'accord** : Analog, Orgue, Cordes, Accordéon, Chœur, Piano électrique
- **Attaque** : du pluck sec aux montées lentes
- **Effets du slide** : filtre résonant + LFO, drive, delay ping-pong calé sur le tempo du projet
- **Global** : chorus, reverb (niveau + taille), volume
- La fenêtre se redimensionne depuis le coin en bas à droite
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
