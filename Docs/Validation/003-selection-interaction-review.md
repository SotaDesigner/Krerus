# Hito 003 — Registro de revisión documental

Fecha: 23 de octubre de 2026. Proyecto inspeccionado: `E:/15.MiVideoJuego/Krerus`. Documento: [selección e interacción](../Milestones/003-npc-selection-interaction.md).

## Método y alcance

Lectura de las fuentes reales, README e hitos 001/002; contraste con el registro anterior de extracción C++. Inspección de cadenas ASCII/Unicode de assets, sin deserializar sus gráficos. Consulta de logs locales ya existentes. `Krerus.uproject` declara EngineAssociation 5.8. No se usa el texto no expandido de respuestas de la conversación como evidencia de implementación.

`unreal_health` devolvió Unreal no accesible (conexión rechazada); no se pudieron consultar gráficos, defaults ni Asset Registry con el puente. No se lanzó compilación, Play ni scripts de Unreal. No se afirma una nueva prueba funcional. La consulta Git tampoco pudo cambiar al directorio por restricción de acceso; no se informa un HEAD/estado actual inferido de los documentos históricos.

## Contraste de nombres y comportamiento

- `SelectedNPC` es débil y de tipo `AKrerusNPC`; `SetSelectedNPC`, `ClearSelectedEnemy` y `SelectEnemyAtScreenPosition` son los nombres reales.
- Clic: posición al pulsar, recorrido acumulado de Look, umbral 5, selección al soltar sin arrastre y traza Visibility compleja.
- Interacción: Started, validez, Dist2D, alcance del NPC, interfaz Execute_Interact; default 300, igualdad permitida.
- Assets NPC/enemigo contienen el padre nativo correspondiente, `AggroRing`, evento `SetSelectionRingVisible` y `SetVisibility`. Las conexiones y defaults no se han certificado.
- GameMode contiene referencias a `PlayerControllerClass` y `KrerusPlayerController`; los logs corroboran instancia nativa en sesiones previas.
- Mapping Context contiene `IA_ClearTarget`, `IA_Interact`, `LeftMouseButton`, `RightMouseButton` y F en lectura Unicode. No se identificó Escape en las cadenas. Es obligatorio verificar la asociación efectiva de Escape y F, sin confundir nombres presentes con mappings aprobados.
- Ambos materiales existen con la escritura `AggronRing`. No se deducen colores, conexiones ni valores.
- Las incidencias de Custom Event y Camera se documentan como diagnóstico/resolución comunicados por el autor, contrastados con el contrato C++ y datos estáticos disponibles; su cierre visual sigue pendiente.

## Logs históricos observados

Tiempos siguientes copiados literalmente del log, sin convertirlos a hora local. Los nombres y números de línea permiten localizar la evidencia; Saved no forma parte de una entrega documental portable y puede rotar. Se conservan aquí solo fragmentos pertinentes, no logs completos.

| Archivo bajo `Saved/Logs` | Línea | Tiempo registrado | Mensaje |
| --- | --- | --- | --- |
| `Krerus.log` | 2448 | 2026.10.23-19.20.48:820 | `KRERUS: Controller actual: KrerusPlayerController_0` |
| `Krerus.log` | 2454 | 2026.10.23-19.20.48:940 | `KRERUS: Nuevo objetivo: BP_EnemyBase_C_2` |
| `Krerus.log` | 2462 | 2026.10.23-19.20.50:022 | `KRERUS: Nuevo objetivo: BP_EnemyBase_C_1` |
| `Krerus.log` | 2470 | 2026.10.23-19.20.51:210 | `KRERUS: Nuevo objetivo: BP_KrerusNPC_C_1` |
| `Krerus.log` | 2309 | 2026.10.23-19.16.10:834 | `KRERUS: Arrastre de cámara detectado` |
| `Krerus-backup-2026.10.23-18.41.52.log` | 4224 | 2026.10.23-18.03.17:982 | `KRERUS: Objetivo cancelado` |
| `Krerus.log` | 2657 | 2026.10.23-19.27.37:595 | `KRERUS: Interaccion con NPC: BP_KrerusNPC_C_1` |
| `Krerus.log` | 2812 | 2026.10.23-19.35.29:333 | `KRERUS: NPC demasiado lejos. Distancia: 1639 / Maxima: 300` |
| `Krerus.log` | 2821 | 2026.10.23-19.35.37:547 | `KRERUS: NPC demasiado lejos. Distancia: 371 / Maxima: 300` |
| `Krerus.log` | 2824 | 2026.10.23-19.35.42:211 | `KRERUS: Interaccion con NPC: BP_EnemyBase_C_2` |

Estos mensajes no prueban las teclas concretas, el límite exacto 300, la apariencia del aro ni la corrección visual de la cámara. Los valores del log de distancia están redondeados a cero decimales.

## Archivos contrastados y SHA-256

Huellas del contenido observado, para identificar esta revisión aunque se modifique después. No equivalen a una validación de compilación ni a commits Git.

| Archivo relativo al proyecto | SHA-256 |
| --- | --- |
| `Source/Krerus/Public/KrerusNPC.h` | `c8ddd4f2d16456b7a7fc213cedd11818c27853eb591df5e030e81477620f760f` |
| `Source/Krerus/Private/KrerusNPC.cpp` | `73b433be0882d09b0053ad9bd2475aac6f57ff364b8dfaa830732c24b87f333b` |
| `Source/Krerus/Public/KrerusEnemy.h` | `d3d77796637a05f43d75eb8e4a0c558342efe313cfd6329cddc1a47aa57af8f4` |
| `Source/Krerus/Private/KrerusEnemy.cpp` | `ee1a7d8ff936350b9a9111f550e9e3d8d77c50f72d81444e855309aa5b6afbf5` |
| `Source/Krerus/Public/KrerusInteractable.h` | `8f00107d84f02e8bdb515bfd78c9d063e89cf06ecab5ba48d6d16fa1c98be1ec` |
| `Source/Krerus/Public/KrerusPlayerController.h` | `fcb45a61e626e1d92672a0bd4a09067602fc39eeb627690da694b1f534b74641` |
| `Source/Krerus/Private/KrerusPlayerController.cpp` | `1fa5af1b7e03c316414832c86da78009ec5ddb97e0e03dfaa04edaff58b4f980` |
| `Source/Krerus/Private/KrerusCharacterInput.cpp` | `758fde1758023db10fd15150890f078b56eda4fb4749092b89111dccc2603e6d` |
| `Source/Krerus/Private/KrerusCharacter.cpp` | `18566e08e1f11cf6b5768f58990d90ccc2225e73cba8936b0d37fd986c5a62d7` |
| `Content/Core/NPC/BP_KrerusNPC.uasset` | `c604aa7d32764b3717f8b0d936905660ff4d191c44fed75adfc9a3272f98e28e` |
| `Content/Core/NPC/Enemy/BP_EnemyBase.uasset` | `51f54e6567fe3cef7b7a986b81584555c1dc302890148914f425f518125c5f3e` |
| `Content/Core/GameModes/BP_KrerusGameMode.uasset` | `e141ed1431bebe6da9ca785b956d0487baa687401f3d26e5c802db05299d0789` |
| `Content/Core/Input/IMC_Krerus.uasset` | `5f0faf4bd3f8dd73d9e0c2a2b0fb7c687ec47fa7ccfab26e1b5eca813a97521f` |
| `Content/Krerus/VFX/Enemies/M_NPC_AggronRing.uasset` | `63185e4f8c927b9190bb96b12b847d1874dbd3884fd79f26ebd11cae25f69fd9` |
| `Content/Krerus/VFX/Enemies/M_EnemyAggronRing.uasset` | `1bae1f9ac38d8fddb116bc3177f3df239cc51cadac6a71446c20f544a75dce28` |

## Evidencias anteriores y pendientes

Los JSON `003-runtime.json`, `003-blueprints.json` y `003-preservation.json` permanecen intactos: son del registro de extracción C++ del 22 de octubre. No usar sus 17 aprobados para cerrar el Hito 003 de selección/interacción. La lista de aceptación vigente está en el documento del hito. Falta registrar compilación actual, mappings, conexiones/defaults Blueprint, casos límite y demostración renderizada. No se inicia el Hito 004.
