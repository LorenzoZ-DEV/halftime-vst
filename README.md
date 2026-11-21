# Halftime

Plugin VST3 (JUCE) che rallenta l'audio a metà velocità, sincronizzato al tempo dell'host. Implementazione originale.

## Compilare

Serve CMake >= 3.22 e un compilatore C++17 (su Windows Visual Studio 2022). JUCE viene scaricato da CMake.

```
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
```

Solo core DSP e test, senza JUCE: `cmake -S . -B build -DHALFTIME_PLUGIN=OFF`.

Il bundle è in `build/Halftime_artefacts/Release/VST3/Halftime.vst3` (anche come artifact della GitHub Action).

## Installare e provare in FL Studio

1. Copia l'intera cartella `Halftime.vst3` in `C:\Program Files\Common Files\VST3`.
2. FL Studio: Options > Manage plugins > Find plugins (Rescan).
3. Carica "Halftime" su un mixer insert e avvia la riproduzione.

## Parametri

- **Cycle**: 1/4, 1/2, 1, 2 battute (in 4/4).
- **Mix**: dry/wet da 0 a 1 (default 1).

## Comportamento e latenza

A ogni inizio ciclo il buffer riparte: la prima metà del ciclo viene suonata a velocità 0.5 (un'ottava sotto) per tutto il ciclo.
Il plugin non dichiara latenza all'host: il ritardo è parte dell'effetto e cresce fino a mezzo ciclo, non è compensabile.
Al cambio di ciclo c'è un crossfade di 5 ms contro i click.

## Limiti noti

- Battute sempre da 4 movimenti (la time signature dell'host è ignorata); BPM limitato a 30..300.
- A trasporto fermo o senza PPQ dall'host usa un contatore interno (BPM dell'host se disponibile, altrimenti 120).
- Dopo un salto del trasporto a metà ciclo la parte di buffer non ancora registrata contiene audio del ciclo precedente.
- Solo stereo; nessuna interfaccia grafica dedicata (editor generico di JUCE).
