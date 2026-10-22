# Krerus

Proyecto personal de portfolio desarrollado en **Unreal Engine 5.8** con C++ y Enhanced Input. Construcción incremental de una base jugable de personaje y cámara híbrida de estilo MMORPG, con decisiones y criterios de validación documentados por hito.

## Estado del proyecto

| Hito | Entrega | Estado |
| --- | --- | --- |
| [001 — Movimiento y locomoción](Docs/Milestones/001-character-movement.md) | WASD, dos marcos de movimiento, sprint y Blend Space Idle/Walk/Run | Completado en el alcance comunicado; salto pendiente |
| [002 — Cámara MMORPG](Docs/Milestones/002-mmorpg-camera.md) | Cursor libre, órbita, control relativo, zoom y Spring Arm | Completado según validación del autor; revisión estática realizada |
| 003 — Separación de responsabilidades | PlayerController, cámara, movimiento e input | Próximo hito; sin implementar |

**El salto NO está completado: falta su animación.** Los enlaces existentes a `Jump`/`StopJumping` no constituyen una entrega terminada.

La revisión del 22 de octubre de 2026 confirma la implementación C++ y la presencia de assets. No incluye una nueva compilación ni pruebas Play; las comprobaciones pendientes están descritas en los hitos. Los valores de Blueprint pueden sobrescribir los valores C++.

## Controles y comportamiento

| Entrada prevista | Comportamiento |
| --- | --- |
| WASD | Movimiento direccional; sin clic derecho conserva la orientación de referencia capturada al iniciar el desplazamiento |
| Shift izquierdo | Sprint: 300 → 600 cm/s por defecto; al soltar, retorno gradual del límite a 300 |
| Ratón sin botones | Cursor visible y libre; no gira la cámara |
| Clic izquierdo mantenido + ratón | Cámara orbital sin activar el movimiento relativo a la cámara |
| Clic derecho mantenido + ratón | Control de cámara y movimiento relativo a su yaw |
| Rueda | Zoom interpolado, con distancia objetivo limitada a 300–1800 cm por defecto |

Ambos modos de cámara ocultan el cursor mientras están activos y lo restauran al soltar los botones. El pitch se limita a −75°/+45°. El Spring Arm utiliza CameraLag de posición (12), sin lag de rotación, y proporciona el mecanismo de colisión de cámara; comprobar `Do Collision Test` y el resultado en el Blueprint/escenario. Los botones y asignaciones efectivas deben verificarse en `IMC_Krerus` y `BP_KrerusCharacter`.

La locomoción utiliza `AB_KrerusCharacter` y `BS_WalkRun` para Idle/Walk/Run. La configuración visual y las transiciones requieren validación en Unreal.

## Abrir y probar

1. Instalar Unreal Engine 5.8 y las herramientas C++ compatibles con esa instalación.
2. Clonar con Git LFS instalado y ejecutar `git lfs pull`.
3. Generar los archivos desde `Krerus.uproject` y compilar `KrerusEditor` en Development Editor para Windows.
4. Abrir `/Game/Dev/Maps/L_DevSandbox`, ejecutar Play y seguir las listas de comprobación de ambos hitos.

## Estructura

```text
Source/Krerus/                  Código C++ del módulo
  Public/KrerusCharacter.h     Declaraciones y parámetros
  Private/KrerusCharacter.cpp  Movimiento, cámara e input actuales
Content/Core/Character/        Personaje, Animation Blueprint y Blend Space
Content/Core/Input/            Acciones y Mapping Context de Enhanced Input
Content/Core/GameModes/        GameMode del proyecto
Content/Dev/Maps/              Escenario de desarrollo
Config/                       Configuración del proyecto
Docs/Milestones/               Objetivos, decisiones, pruebas y próximos pasos
```

## Próximo paso: Hito 003

Refactorizar `AKrerusCharacter` en responsabilidades/clases separadas para **PlayerController, cámara, movimiento e input**. Mantener el comportamiento de los hitos 001/002 y validar cada extracción con compilación y pruebas en Play antes de continuar. La distribución concreta de clases/componentes se decidirá durante ese hito.

## Portfolio y versión propuesta

La entrega candidata es **v0.2.0**; la etiqueta aún no está creada. El [Hito 002](Docs/Milestones/002-mmorpg-camera.md) recoge la revisión Git, las limitaciones y la propuesta de commits. `.gitignore` excluye las salidas habituales de Unreal e IDE; `.gitattributes` configura LFS para `.uasset` y `.umap`.

Este repositorio presenta una base de control y locomoción. No acredita sistemas de red/MMO, combate ni un juego completo. Antes de publicar una demo, completar las pruebas y comprobar los permisos de distribución de modelos, animaciones y referencias. La revisión previa dejó registrada una incidencia con un token de Android File Server en el historial; consultar el Hito 001 antes de publicar.
