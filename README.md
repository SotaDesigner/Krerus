# Krerus

Proyecto personal de portfolio desarrollado en **Unreal Engine 5.8** con C++ y Enhanced Input. Construcción incremental de una base jugable de personaje y cámara híbrida de estilo MMORPG, con decisiones y criterios de validación documentados por hito.

## Estado del proyecto

| Hito | Entrega | Estado |
| --- | --- | --- |
| [001 — Movimiento y locomoción](Docs/Milestones/001-character-movement.md) | WASD, dos marcos de movimiento, sprint y Blend Space Idle/Walk/Run | Completado en el alcance comunicado; salto pendiente |
| [002 — Cámara MMORPG](Docs/Milestones/002-mmorpg-camera.md) | Cursor libre, órbita, control relativo, zoom y Spring Arm | Completado según validación del autor; revisión estática realizada |
| [003 — Selección e interacción con NPC y enemigos](Docs/Milestones/003-npc-selection-interaction.md) | Clic/arrastre, objetivo único, Escape, aro y F con alcance horizontal por NPC | Implementado en fuentes y con logs previos; aceptación actual en editor pendiente |

**El salto NO está completado: falta su animación.** Los enlaces existentes a `Jump`/`StopJumping` no constituyen una entrega terminada.

La revisión del 23 de octubre de 2026 contrasta el Hito 003 con fuentes, assets y logs guardados; no ejecuta una nueva compilación ni Play. La extracción C++ previa conserva sus pruebas históricas como contexto técnico, sin constituir un hito. Quedan pendientes los mappings efectivos, conexiones Blueprint y aceptación visual/funcional actual. Los valores de Blueprint pueden sobrescribir los defaults C++.

## Controles y comportamiento

| Entrada prevista | Comportamiento |
| --- | --- |
| WASD | Movimiento direccional; sin clic derecho conserva la orientación de referencia capturada al iniciar el desplazamiento |
| Shift izquierdo | Sprint: 300 → 600 cm/s por defecto; al soltar, retorno gradual del límite a 300 |
| Ratón sin botones | Cursor visible y libre; no gira la cámara |
| Clic izquierdo sin arrastre | Seleccionar NPC/enemigo al soltar desde la posición inicial; conserva objetivo al pulsar suelo/vacío |
| Clic izquierdo mantenido + ratón | Cámara orbital; desde el umbral de arrastre no selecciona al soltar |
| Clic derecho mantenido + ratón | Control de cámara y movimiento relativo a su yaw |
| Rueda | Zoom interpolado, con distancia objetivo limitada a 300–1800 cm por defecto |
| Escape | Limpiar objetivo y aro; confirmar mapping efectivo en editor |
| F | Interactuar con objetivo dentro de su alcance horizontal: 300 uu por defecto, configurable por NPC |

Ambos modos de cámara ocultan el cursor mientras están activos y lo restauran al soltar los botones. El pitch se limita a −75°/+45°. El Spring Arm utiliza CameraLag de posición (12), sin lag de rotación, y proporciona el mecanismo de colisión de cámara; comprobar `Do Collision Test` y el resultado en el Blueprint/escenario. Los botones y asignaciones efectivas deben verificarse en `IMC_Krerus` y `BP_KrerusCharacter`.

La locomoción utiliza `AB_KrerusCharacter` y `BS_WalkRun` para Idle/Walk/Run. La configuración visual y las transiciones requieren validación en Unreal.

## Abrir y probar

1. Instalar Unreal Engine 5.8 y las herramientas C++ compatibles con esa instalación.
2. Clonar con Git LFS instalado y ejecutar `git lfs pull`.
3. Generar los archivos desde `Krerus.uproject` y compilar `KrerusEditor` en Development Editor para Windows.
4. Abrir `/Game/Dev/Maps/L_DevSandbox`, ejecutar Play y seguir las listas de comprobación de los hitos 001, 002 y 003. Verificar también GameMode, PlayerController y acciones efectivas del mapa.

## Estructura

```text
Source/Krerus/                  Código C++ del módulo
  Public/KrerusCharacter.h             Declaraciones y parámetros compatibles con Blueprint
  Public/KrerusCameraComponent.h       Sistema de zoom y límites verticales
  Public/KrerusPlayerController.h      Cursor, clic/arrastre y selección única
  Public/KrerusNPC.h                   Base de NPC, alcance y evento del aro
  Public/KrerusEnemy.h                 Enemigo derivado de NPC
  Public/KrerusInteractable.h          Interfaz de interacción
  Private/KrerusCharacter.cpp          Construcción y coordinación por frame
  Private/KrerusCharacterMovement.cpp  Movimiento direccional y sprint
  Private/KrerusCharacterInput.cpp     Enhanced Input y handlers
Content/Core/Character/        Personaje, Animation Blueprint y Blend Space
Content/Core/Input/            Acciones y Mapping Context de Enhanced Input
Content/Core/NPC/              BP_KrerusNPC y Enemy/BP_EnemyBase
Content/Krerus/VFX/Enemies/     Materiales de los aros
Content/Core/GameModes/        GameMode del proyecto
Content/Dev/Maps/              Escenario de desarrollo
Config/                       Configuración del proyecto
Docs/Milestones/               Objetivos, decisiones, pruebas y próximos pasos
```

## Hito 003 — Selección e interacción

`AKrerusNPC` implementa `IKrerusInteractable` y sirve de base para `AKrerusEnemy`. `AKrerusPlayerController` conserva `SelectedNPC` como referencia débil y diferencia clic/arrastre. El Character enlaza las acciones y calcula `FVector::Dist2D` antes de ejecutar la interfaz; el alcance pertenece al NPC. C++ notifica `SetSelectionRingVisible(bool)` y cada Blueprint resuelve su aro mediante material y visibilidad. La interacción nativa actual registra un mensaje, sin diálogo ni combate.

Ver [arquitectura, flujo, incidencias y pruebas del Hito 003](Docs/Milestones/003-npc-selection-interaction.md). La [separación C++ previa](Docs/Milestones/003-cpp-architecture.md) se mantiene como antecedente técnico, sin numeración de hito. El Hito 004 de atributos no se inicia en esta entrega.

## Portfolio y versión propuesta

La propuesta histórica de 002 fue **v0.2.0**; esta revisión no verifica etiquetas actuales ni propone una nueva para 003. El [Hito 002](Docs/Milestones/002-mmorpg-camera.md) recoge la revisión Git, las limitaciones y la propuesta de commits. `.gitignore` excluye las salidas habituales de Unreal e IDE; `.gitattributes` configura LFS para `.uasset` y `.umap`.

Este repositorio presenta una base de control y locomoción. No acredita sistemas de red/MMO, combate ni un juego completo. Antes de publicar una demo, completar las pruebas y comprobar los permisos de distribución de modelos, animaciones y referencias. La revisión previa dejó registrada una incidencia con un token de Android File Server en el historial; consultar el Hito 001 antes de publicar.
