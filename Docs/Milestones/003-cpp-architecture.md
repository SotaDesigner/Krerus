# Contexto técnico — Separación de responsabilidades C++

> Reclasificación documental: 23 de octubre de 2026. La refactorización **no es un hito**. El Hito 003 vigente es [Sistema de selección e interacción con NPC y enemigos](003-npc-selection-interaction.md). Se conserva este nombre de archivo para no romper enlaces anteriores y mantener las evidencias técnicas.
>
> Todo el registro siguiente describe la extracción del **22 de octubre**, no el estado funcional actual. En particular, la sección de compatibilidad que indica un GameMode con `Engine.PlayerController` es histórica: los logs posteriores registran `KrerusPlayerController_0` y el asset actual referencia `KrerusPlayerController`. Los 17 casos y JSON con prefijo 003 solo cubren la extracción de movimiento/cámara, no selección/interacción. Las instrucciones de scripts no se han vuelto a ejecutar para esta revisión.

Fecha: 22 de octubre de 2026. Unreal Engine comprobado: **5.8.4**. Estado: extracciones implementadas y compiladas; pruebas automatizadas aprobadas; cierre visual pendiente. Salto animado pendiente, fuera de este hito.

## Objetivo y estilo

Separar responsabilidades sin cambiar el comportamiento de 001/002. Se mantiene el estilo existente: nombres directos, llaves en línea independiente, tabulación y funciones pequeñas. No se incorporan interfaces, gestores globales ni otro sistema de movimiento.

## Arquitectura y decisiones

| Archivo/clase | Responsabilidad |
| --- | --- |
| `KrerusCharacter.cpp` | Construcción de componentes, BeginPlay y coordinación de Tick |
| `KrerusCharacterMovement.cpp` | Move, referencia direccional libre, sprint y retorno gradual del límite de velocidad |
| `KrerusCharacterInput.cpp` | Enlaces de Enhanced Input, Mapping Context, Look y adaptación de los modos de cámara |
| `UKrerusCameraComponent` | Distancia objetivo, zoom interpolado y límites de pitch |
| `AKrerusPlayerController` | Política del cursor y GameOnly/GameAndUI |

Movimiento e input son archivos de implementación de **la misma clase AKrerusCharacter**, no dos componentes nuevos. El coste de separar sus estados en más objetos no aporta valor aquí. `UCharacterMovementComponent` conserva física, orientación y velocidad.

Los parámetros de cámara permanecen en el Character, junto a sus propiedades reflejadas, para que los valores serializados de Blueprint sigan siendo efectivos. El componente recibe esos valores al actualizarse; no introduce una segunda copia editable. Solo la distancia objetivo, que no era UPROPERTY, pasa al componente. `CameraBoom` y `FollowCamera` mantienen sus nombres, tipos y jerarquía. El nuevo subobjeto es `CameraSystem`.

El componente no usa un Tick independiente: el Character llama a UpdateSprint y después a UpdateCamera en el mismo orden que antes. Conserva 300/600 cm/s, interpolación de retorno 5 y tolerancia 1, zoom 300–1800 cm, paso 150, interpolación 8 y pitch −75/+45. CameraLag y colisiones siguen en el Spring Arm.

### Compatibilidad del PlayerController

El GameMode real usa todavía `Engine.PlayerController`. No se modifica su asset ni se cambia su Pawn. El Character delega la captura en `AKrerusPlayerController::ApplyMouseCapture`, que aplica la misma política sobre el controlador actual. Si el GameMode selecciona la nueva clase nativa, utiliza su método de instancia UpdateMouseCapture. Así la extracción funciona ya, conservando configuraciones existentes.

La selección de `KrerusPlayerController` como Player Controller Class es una migración posterior de configuración, no un requisito para esta entrega. No se afirma haber probado esa instancia nativa en Play. Los flags de modos y sus handlers siguen en el Character como adaptación compatible; trasladar también ese estado exige una segunda migración de Blueprint.

No se renombran ni trasladan las acciones y el Mapping Context. Se mantienen exactamente los enlaces Started/Triggered/Completed existentes. Cancelación, pérdida de foco y limpieza al cambiar de Pawn se dejan para un cambio funcional posterior; no se mezclan con la extracción.

## Secuencia y compilación

1. Revisar Git, README, hito 002 y fuentes reales. Guardar copias de los archivos previos, incluidos los cambios sin commit del autor.
2. Compilar la base: correcto después de cerrar una instancia bloqueada de Unreal, con autorización del autor para finalizar su proceso tras guardar.
3. Extraer cámara: compilación KrerusEditor Win64 Development correcta.
4. Extraer política de cursor y modos: compilación correcta.
5. Separar movimiento/input: el primer enlace falló porque el makefile de Unreal no incluía los dos archivos nuevos. Regenerar la lista con `-gather` resolvió los símbolos pendientes; compilación correcta.
6. Cargar y compilar BP_KrerusCharacter, AB_KrerusCharacter y BP_KrerusGameMode mediante Unreal en modo de comprobación, sin guardar assets. Los tres quedaron BS_UP_TO_DATE; el commandlet terminó con 0 errores y 0 avisos.

El puente Blueprint no estaba disponible. La comprobación alternativa usó el Python de Unreal; no editó ni guardó assets. No se introdujeron cambios en structs ni Data Tables.

## Evidencias

- [Play automatizado](../Validation/003-runtime.json): 17 comprobaciones aprobadas mediante Enhanced Input, con claves forzadas y Look inyectado.
- [Blueprints y valores efectivos](../Validation/003-blueprints.json): las ocho referencias de input están asignadas; hay 10 mappings; Spring Arm con colisiones activas, CameraLag 12 y sin lag rotacional.
- [Preservación](../Validation/003-preservation.json): diez funciones conservan su contenido salvo espacios, incluidas Move y SetupPlayerInputComponent; las declaraciones de propiedades Blueprint existentes se conservan; los tres assets comparados mantienen exactamente sus bytes.

Play aprobó cursor libre, W, sprint a 600 y retorno a 300, órbita izquierda, rechazo de Look sin botones, ambos botones y sus dos órdenes de liberación, zoom hacia ambos límites, límite de pitch comprobado a −75, movimiento relativo a yaw 90 con derecho y mantenimiento del marco libre después de soltarlo y girar cámara.

La primera ejecución del comprobador falló por una llamada Python no expuesta (`get_character_movement`). Se corrigió el comprobador para leer `character_movement`; no hizo falta cambiar el juego. Las ejecuciones posteriores aprobaron las comprobaciones registradas.

### Alcance pendiente de la validación

Las pruebas de Play usaron `-nullrhi`: verifican comportamiento y valores, no imagen. No certifican la apariencia de Idle/Walk/Run, ausencia de clipping, suavidad perceptiva, recuperación ante obstáculos, límite superior de pitch ni todas las tasas de frames. Tampoco cubren foco/cancelación o una instancia nativa del nuevo PlayerController.

- [ ] Abrir L_DevSandbox y revisar visualmente WASD/diagonales, ambos marcos de movimiento y Blend Space Idle/Walk/Run.
- [ ] Verificar zoom y seguimiento con renderizado, CameraLag y retracción/recuperación ante paredes.
- [ ] Probar ambos extremos de pitch, cambios de foco, cancelación y posesión/reinicio de Pawn.
- [ ] Si se cambia Player Controller Class a KrerusPlayerController, volver a compilar GameMode y repetir Play.
- [ ] Registrar clip y resultados antes de declarar el cierre visual del hito.

## Repetir las comprobaciones

Compilar KrerusEditor Win64 Development con el proyecto cerrado. Usar `-gather` si Unreal conserva una lista antigua de fuentes.

Para la comprobación de assets: ejecutar UnrealEditor-Cmd con el .uproject, `-run=pythonscript`, `-script=<ruta absoluta a Scripts/Validation/validate_hito003.py>`, `-EnablePlugins=PythonScriptPlugin -unattended -nullrhi`.

Para Play: ejecutar UnrealEditor con el .uproject, `-ExecutePythonScript=<ruta absoluta a Scripts/Validation/runtime_hito003.py>`, `-EnablePlugins=PythonScriptPlugin -unattended -nullrhi`. Usar una instancia dedicada y el proyecto guardado: el comprobador abre el mapa, fuerza entradas, termina Play y cierra esa instancia. Los resultados se escriben en `Saved/Tests/Hito003`. Los scripts no guardan assets.

## Git y portfolio

Base: rama main, HEAD 640f69d, un commit por delante de la referencia local origin/main. Sin fetch: no se afirma el estado actual del servidor. Había cambios del autor en C++, Blueprint, Mapping Context, acciones, Config y .gitignore; se conservan. README y Docs estaban sin seguimiento y se continúa su trabajo previo.

Los cambios 003 se añaden sobre ese árbol, sin restaurar archivos desde HEAD ni tocar el índice. Las copias previas se conservan fuera del repositorio en la carpeta de trabajo del chat. No se crean commits, tags ni push. La propuesta v0.2.0 del hito 002 sigue como antecedente y no representa una etiqueta nueva de esta refactorización.

Commit propuesto, después de revisar el diff respecto a los cambios 002: `refactor: separate camera, cursor and character input responsibilities`. Incluir fuentes nuevas, documentación y scripts de comprobación; no seleccionar automáticamente el vídeo ni mezclar la retirada previa de configuración sensible. El cierre visual debe registrarse antes de publicar la entrega del hito.


## Registro histórico de commits — 22 de octubre de 2026

El registro previo indica dos commits locales: la cámara en `5315396` y una extracción posterior de responsabilidades. Es información histórica del 22 de octubre, no una consulta Git actual. No acredita un hito funcional separado ni valida la selección e interacción añadidas después. No se verifica aquí el estado del remoto ni de las etiquetas.
