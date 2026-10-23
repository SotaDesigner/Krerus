# Hito 003 — Sistema de selección e interacción con NPC y enemigos

Fecha de revisión: 23 de octubre de 2026. Unreal Engine: **5.8**, según `Krerus.uproject`.

## Objetivo y estado

Permitir seleccionar un NPC o enemigo con clic izquierdo, distinguir el clic del arrastre orbital, cancelar el objetivo con Escape, mostrar su aro de selección e interactuar con F dentro de su alcance horizontal. El Hito 003 es esta entrega jugable. La separación previa de responsabilidades C++ es contexto técnico, **no un hito independiente**.

El código implementa el flujo de selección e interacción. Los logs locales del 23 de octubre registran objetivos NPC/enemigos, arrastre, cancelación, interacción y rechazo por distancia. Esta revisión documental contrasta fuentes, archivos de assets y logs existentes; no realiza una nueva compilación ni ejecuta Play. El puente Blueprint no está disponible. Por tanto, las asociaciones exactas del Mapping Context, conexiones visuales, valores efectivos por instancia y resolución visual de las incidencias requieren confirmación en el editor. No se presenta la lista de aceptación como ya aprobada.

La interacción actual termina en un mensaje de log. No implementa diálogo, tienda, daño, combate, IA, atributos ni red. El Hito 004 de atributos queda fuera y no se inicia. El salto animado de 001 continúa pendiente.

## Arquitectura y decisiones técnicas

### Jerarquía C++ y Blueprints

```text
ACharacter
  AKrerusNPC : IKrerusInteractable
    AKrerusEnemy

AKrerusNPC   → /Game/Core/NPC/BP_KrerusNPC
AKrerusEnemy → /Game/Core/NPC/Enemy/BP_EnemyBase
```

Las dos últimas líneas expresan las clases padre nativas referenciadas en los assets. La herencia común está en C++; no debe describirse `BP_EnemyBase` como hijo de `BP_KrerusNPC`. La copia en `Content/Core/Enemy/BP_EnemyBase.uasset` contiene referencias a la ubicación `Core/NPC/Enemy`; es compatible con un traslado/redirección, pero su tipo efectivo queda pendiente de consultar en Asset Registry. No eliminarla ni tratarla como otro enemigo funcional sin inspección.

| Clase/archivo real | Responsabilidad comprobada |
| --- | --- |
| `Source/Krerus/Public/KrerusNPC.h` y `Private/KrerusNPC.cpp` | Base seleccionable, interfaz de interacción, alcance por NPC y evento visual |
| `Public/KrerusEnemy.h` y `Private/KrerusEnemy.cpp` | Hereda todo lo anterior de `AKrerusNPC`; no añade combate ni override de interacción |
| `Public/KrerusInteractable.h` y `Private/KrerusInteractable.cpp` | Declaración de `UKrerusInteractable` / `IKrerusInteractable`; el `.cpp` solo incluye la cabecera |
| `Public/KrerusPlayerController.h` y `Private/KrerusPlayerController.cpp` | Propietario de selección, estado de clic/arrastre, traza y notificación del aro |
| `Public/KrerusCharacter.h` y `Private/KrerusCharacterInput.cpp` | Propiedades de acciones, enlaces Enhanced Input, adaptación del ratón, limpiar objetivo y comprobar/interactuar |
| `Private/KrerusCharacter.cpp` / `UKrerusCameraComponent` | Coordinación del personaje y cámara; conserva zoom y límites de 002 |

`AKrerusNPC` mantiene Tick habilitado, aunque sus métodos de ciclo de vida solo llaman a Super. `AKrerusEnemy` deshabilita Tick en su constructor. Esto describe los defaults C++, sin afirmar los valores finales de Blueprint.

### Propiedad de la selección

`AKrerusPlayerController::SelectedNPC` es `TWeakObjectPtr<AKrerusNPC>`, con `VisibleInstanceOnly`, `BlueprintReadOnly` y categoría `Targeting`. Guarda una sola referencia débil: puede seleccionar tanto la base NPC como su derivada enemigo, sin mantener vivo al actor. `GetSelectedNPC()` devuelve `SelectedNPC.Get()`; las rutas de uso comprueban validez.

Se conservan los nombres reales `SelectEnemyAtScreenPosition`, `ClearSelectedEnemy` y el parámetro `NewEnemy` de `SetSelectedNPC`, aunque el tipo aceptado es `AKrerusNPC`. No existe una propiedad `SelectedEnemy` en las fuentes revisadas.

`SetSelectedNPC(NewEnemy)`:

1. Retorna si la referencia actual y la nueva coinciden.
2. Si el objetivo anterior sigue válido, llama a `SetSelectionRingVisible(false)`.
3. Asigna la referencia nueva.
4. Si es válida, llama a `SetSelectionRingVisible(true)` y registra `KRERUS: Nuevo objetivo: ...`.

`ClearSelectedEnemy()` registra la cancelación cuando procede y llama a `SetSelectedNPC(nullptr)`. Seleccionar de nuevo el mismo actor no vuelve a disparar el evento. Una referencia destruida deja de ser válida, pero no hay callback adicional de destrucción ni actualización de HUD implementados.

### Requisito de PlayerController

La selección exige un `AKrerusPlayerController`: el Character hace Cast en las rutas de clic, Look, limpiar e interactuar. La ruta estática `ApplyMouseCapture` conserva la política del cursor con un PlayerController genérico, pero no le añade selección. Los logs actuales registran `KrerusPlayerController_0`; el asset `BP_KrerusGameMode` contiene `PlayerControllerClass` y la referencia `KrerusPlayerController`. Esto supera el estado histórico de la refactorización, cuando el GameMode aún usaba el controlador genérico. Verificar también los overrides del mapa en World Settings al reproducir.

## Selección por clic izquierdo y convivencia con la cámara

### Flujo de entrada

1. `CameraFreeAction` enlaza `Started` con `StartFreeCamera`. Antes de ocultar el cursor, llama a `BeginLeftMouseClick()`.
2. El controlador activa `bLeftMousePressed`, reinicia arrastre/distancia e intenta guardar el cursor mediante `GetMousePosition` en `LeftClickScreenPosition`.
3. El Character activa `bIsFreeCameraActive` y actualiza la captura. `Look`, enlazado a `LookAction / Triggered`, entrega su vector 2D a `RegisterLeftMouseMovement` mientras ese modo está activo, y aplica yaw/pitch.
4. `CameraFreeAction / Completed` llama a `StopFreeCamera`: desactiva el modo, actualiza captura y después llama a `EndLeftMouseClick()`.
5. Al soltar, solo si había pulsación y no se detectó arrastre, se traza desde la **posición guardada al pulsar**, no desde la posición final del cursor.

El clic no depende de un tiempo máximo de pulsación. `LeftMouseDragDistance` acumula `MouseDelta.Size()` de cada entrada Look. Con `>= LeftMouseDragThreshold` marca arrastre y no selecciona al soltar. El default es **5.0**, en unidades del vector de entrada recibido: no hay una conversión comprobada a píxeles físicos. Es recorrido acumulado, no distancia neta entre inicio y fin. El umbral no es UPROPERTY ni parámetro editable por Blueprint en el código actual.

La cámara puede recibir movimiento antes de alcanzar el umbral; este decide si se selecciona al soltar, no retrasa la órbita. Si `GetMousePosition` falla, no hay retorno de error: queda la posición anterior (inicialmente cero). Es un caso pendiente de prueba, no una corrección realizada en esta revisión.

### Traza y reglas de selección

`SelectEnemyAtScreenPosition` usa `GetHitResultAtScreenPosition(ScreenPosition, ECC_Visibility, true, Hit)`: consulta el canal **Visibility**, con traza compleja habilitada. Solo acepta el actor golpeado si `Cast<AKrerusNPC>` tiene éxito.

- NPC y enemigos comparten la ruta, gracias a su herencia.
- No hay filtro adicional por facción, hostilidad, vida o interfaz genérica.
- No se limita la distancia de selección; el alcance de 300 se aplica después, al interactuar.
- Un clic sin hit o sobre un actor ajeno a `AKrerusNPC` conserva la selección anterior.
- Un obstáculo que responda a Visibility puede impedir alcanzar al actor deseado. Revisar las respuestas de colisión efectivas; no se infiere una configuración completa a partir de las cadenas del asset.

### Escape y controles

| Entrada prevista | Acción/handler real | Resultado |
| --- | --- | --- |
| Izquierdo, pulsar/soltar sin arrastre | `CameraFreeAction` → `StartFreeCamera` / `StopFreeCamera` | Seleccionar desde la posición inicial |
| Izquierdo + movimiento | `LookAction` → `Look` → `RegisterLeftMouseMovement` | Orbitar y, desde el umbral, impedir selección al soltar |
| Derecho + movimiento | `CameraControlAction` → `StartCameraControl` / `StopCameraControl` | Conservar control relativo de 002 |
| Escape | `ClearTargetAction / Started` → `ClearTarget` → `ClearSelectedEnemy` | Limpiar referencia y ocultar aro anterior |
| F | `InteractAction / Started` → `Interact` | Intentar interacción del objetivo válido y en alcance |

Existen `IA_ClearTarget`, `IA_Interact` e `IMC_Krerus`; el Mapping Context contiene referencias a ambas acciones y a los botones del ratón. La inspección de cadenas encuentra F, pero **no confirma Escape** ni permite certificar la asociación acción/tecla. Escape y F son los controles solicitados/comunicados por el autor; confirmar sus mappings efectivos y asignaciones `ClearTargetAction` / `InteractAction` en `BP_KrerusCharacter`. El C++ no codifica esas teclas directamente. En PIE, comprobar que Escape llegue al juego y no solo finalice la sesión por el atajo del editor.

No hay enlace `Canceled` específico para estas rutas de cámara. Probar pérdida de foco, captura del ratón y pulsación simultánea de botones antes de declarar cerrada la regresión.

## Aro visual mediante material y evento Blueprint

`AKrerusNPC` declara `UFUNCTION(BlueprintImplementableEvent, Category = "Targeting") void SetSelectionRingVisible(bool bVisible)`. C++ decide qué actor mostrar y cuándo; Blueprint implementa la presentación. No hay implementación nativa del aro en `KrerusNPC.cpp`.

Los assets revisados contienen `AggroRing`, `SetSelectionRingVisible`, `SetVisibility` y `K2Node_Event`. Las referencias de material son:

| Blueprint | Material referenciado |
| --- | --- |
| `/Game/Core/NPC/BP_KrerusNPC` | `/Game/Krerus/VFX/Enemies/M_NPC_AggronRing` |
| `/Game/Core/NPC/Enemy/BP_EnemyBase` | `/Game/Krerus/VFX/Enemies/M_EnemyAggronRing` |

Se respeta la escritura real **AggronRing**. Los nombres `AggroRing` no acreditan un sistema de aggro/IA. Los materiales contienen expresiones de coordenadas, longitud, resta, valor absoluto, saturación, tiempo y seno; esta lectura no reconstruye sus conexiones ni autoriza a afirmar colores, radios, pulsación, blend mode o parámetros concretos.

El contrato visual que debe verificarse en cada BP es el evento heredado `SetSelectionRingVisible` conectado a `SetVisibility` de `AggroRing`, usando `bVisible` como nuevo estado; oculto al comenzar, visible solo en el seleccionado. Al ser dos BP con padres nativos distintos, comprobar ambos gráficos. La existencia de los nombres y nodos no demuestra por sí sola sus enlaces, visibilidad inicial o resultado renderizado.

## Interacción con interfaz y alcance configurable

`UKrerusInteractable` se declara con `UINTERFACE(MinimalAPI, Blueprintable)`. `IKrerusInteractable` expone `Interact(AActor* Interactor)` como `BlueprintNativeEvent`, `BlueprintCallable`, categoría `Interaction`. `AKrerusNPC` implementa `Interact_Implementation`; un BP puede personalizar el evento. El despacho correcto usado es `IKrerusInteractable::Execute_Interact(SelectedNPC, this)`, que permite la implementación Blueprint.

`AKrerusCharacter::Interact()` realiza estas comprobaciones en orden:

1. Obtener un `AKrerusPlayerController`; si falla, retornar.
2. Obtener `GetSelectedNPC()` y comprobar `IsValid`; si falla, retornar.
3. Calcular `FVector::Dist2D(GetActorLocation(), SelectedNPC->GetActorLocation())`.
4. Si supera `GetInteractionDistance()`, registrar `KRERUS: NPC demasiado lejos. Distancia: ... / Maxima: ...` y retornar.
5. Comprobar `ImplementsInterface(UKrerusInteractable::StaticClass())` y ejecutar la interfaz, pasando al Character como Interactor.

`InteractionDistance` vive en `AKrerusNPC`: `float = 300.0f`, `EditAnywhere`, `BlueprintReadOnly`, `meta = (ClampMin = "0.0")`, categoría escrita literalmente **Interacion**. El getter `GetInteractionDistance() const` devuelve ese valor. El alcance puede ajustarse en cada NPC desde el editor, también en la clase BP; 300 uu equivale a 300 cm bajo la escala habitual del proyecto. ClampMin es metadato del editor, no un clamp explícito dentro de la función.

La distancia ignora Z y mide entre orígenes de actores, sin descontar cápsulas/radios. Como la exclusión usa `>`, el valor exactamente igual al alcance está permitido. No se mide distancia al aro, al punto del hit ni a la cámara. No hay comprobación adicional de línea de visión al pulsar F, ni movimiento automático hacia el objetivo. La selección se conserva tras interactuar o quedar fuera de alcance.

La implementación nativa registra `KRERUS: Interaccion con NPC: <nombre>`. No usa el argumento Interactor para otra acción. `AKrerusEnemy` hereda esta implementación, por lo que un log de interacción con enemigo sigue diciendo NPC. La interfaz está preparada para extensión, pero el selector actual solo admite `AKrerusNPC`, no cualquier actor que implemente la interfaz.

## Problemas resueltos y diagnóstico

### Eventos copiados como Custom Event

Incidencia comunicada por el autor: al copiar la lógica visual de un Blueprint, el evento pudo quedar como **Custom Event**, sin implementar el evento heredado de C++. Compartir un nombre visible no basta para recibir la llamada `SetSelectionRingVisible`.

La resolución de diseño consiste en crear/implementar el evento heredado desde la clase padre y conectar su parámetro `bVisible` al cambio de visibilidad del aro, conservando la lógica útil copiada. Las cadenas actuales de ambos BP contienen `K2Node_Event`, `SetSelectionRingVisible` y referencias a `Public/KrerusNPC.h`, coherentes con esa solución. Sin el editor no se certifican los enlaces ni el estado compilado. Confirmar que C++ enciende/apaga el aro en ambos BP y que no depende de un Custom Event desconectado. Esta revisión no ha editado ni recompilado assets.

### Cámara comprimida por colisión del Skeletal Mesh con Camera

Incidencia y solución comunicadas por el autor: el Skeletal Mesh respondía bloqueando el canal **Camera**, provocando la retracción del Spring Arm y una cámara demasiado cercana pese a una longitud deseada mayor. Ajustar la respuesta Camera del mesh implicado a **Ignore**, conservando la comprobación del Spring Arm frente a los obstáculos del escenario, permite evitar esa compresión accidental. No atribuir la solución a cambiar el límite mínimo del zoom ni a desactivar globalmente la colisión de cámara.

El código actual en `AKrerusCharacter::Tick` muestra `Camara - Deseada: ... | Real: ...` y `SpringArm Collision: SI/NO`, usando `TargetArmLength`, distancia efectiva e `IsCollisionFixApplied()`. Permite distinguir el objetivo del brazo y la retracción por colisión. El ajuste del canal no está programado en los constructores C++ revisados; es configuración de assets. Los BP contienen datos de colisión, pero esta revisión no certifica qué componente/canal recibió el override. Queda por comprobar el mesh afectado en el editor y grabar cámara libre de compresión junto a NPC/enemigos y retracción correcta contra paredes.

## Pruebas y evidencias

El [registro de revisión](../Validation/003-selection-interaction-review.md) identifica archivos, huellas y fragmentos de logs. Son evidencias históricas locales, no una ejecución nueva ni una batería completa aprobada.

| Caso | Evidencia disponible | Límite |
| --- | --- | --- |
| Controlador nativo activo | `Controller actual: KrerusPlayerController_0` | Sesiones registradas, no todos los mapas |
| Selección de enemigo y NPC | `Nuevo objetivo: BP_EnemyBase_C_2`, `_C_1` y `BP_KrerusNPC_C_1` | No acredita el aro renderizado |
| Arrastre | `Arrastre de cámara detectado` | No demuestra por sí solo ausencia de selección al soltar |
| Limpieza | `Objetivo cancelado` en log de respaldo | No identifica la tecla pulsada |
| Interacción con NPC/enemigo | `Interaccion con NPC` para ambos tipos | No acredita acciones de juego adicionales |
| Fuera de alcance | Distancias 1639 y 371, máxima 300 | No prueba el límite exacto ni overrides por instancia |

### Lista de aceptación para cerrar y grabar la demo

- [ ] Compilar KrerusEditor con UE 5.8 y abrir `L_DevSandbox`; comprobar GameMode, controlador y Pawn efectivos.
- [ ] Compilar/revisar `BP_KrerusCharacter`, `BP_KrerusGameMode`, `BP_KrerusNPC` y `BP_EnemyBase` sin errores; registrar avisos.
- [ ] Revisar en `IMC_Krerus` izquierdo/derecho, Escape y F y sus acciones asignadas al Character.
- [ ] Clic en NPC y después enemigo: cambia la referencia y solo el último aro está visible; repetir sobre el mismo actor.
- [ ] Clic en suelo, actor ajeno o vacío: conserva selección. Comprobar oclusión por Visibility y respuestas del NPC.
- [ ] Soltar sin movimiento, por debajo de 5, justo al umbral y por encima: solo los casos sin arrastre seleccionan; probar pequeños movimientos acumulados en sentidos opuestos.
- [ ] Orbitar sobre un objetivo, soltar y confirmar que el arrastre no cambia selección; probar ambos botones y ambos órdenes de liberación.
- [ ] Escape borra referencia y aro; repetir sin objetivo. Asegurar recepción en el juego durante PIE.
- [ ] F sin objetivo no falla; con NPC y enemigo válidos ejecuta la interfaz una vez por pulsación Started, según los triggers efectivos.
- [ ] Comprobar menor/igual/mayor que 300 uu; registrar valores sin depender del redondeo del log.
- [ ] Cambiar alcance en una instancia y contrastarlo con otra; comprobar que se usa el valor del objetivo seleccionado.
- [ ] Separar solo en Z y confirmar Dist2D; probar orígenes/cápsulas y ausencia de requisito de línea de visión al interactuar.
- [ ] Destruir el actor seleccionado y pulsar F/Escape; confirmar que no se accede a una referencia inválida.
- [ ] Revisar eventos heredados y visibilidad inicial de ambos aros, material y ausencia de interferencia del aro en las trazas.
- [ ] Verificar Camera = Ignore en el Skeletal Mesh implicado y colisión del Spring Arm contra paredes; registrar distancia deseada/real.
- [ ] Repetir regresiones 001/002: WASD/diagonales, ambos marcos, sprint 300/600 y retorno, Idle/Walk/Run, cursor, zoom 300–1800, pitch −75/+45, lag y obstáculos.
- [ ] Probar pérdida de foco/cancelación, fallo de captura de posición y cambio de Pawn; registrar estado de botones/cursor/selección.
- [ ] Adjuntar clip y resultados con mapa, valores efectivos y versión compilada; mantener salto y atributos fuera del cierre.

## Contexto de la separación C++

Movimiento e input están en archivos de implementación de la misma clase `AKrerusCharacter`; no son dos componentes nuevos. `UKrerusCameraComponent` gestiona zoom/límites y el PlayerController cursor/selección. El documento [contexto técnico de arquitectura](003-cpp-architecture.md) conserva el registro anterior del 22 de octubre, con sus pruebas de 17 casos sobre movimiento/cámara y limitaciones de renderizado.

Los JSON `003-runtime.json`, `003-blueprints.json`, `003-preservation.json` y scripts `validate_hito003.py` / `runtime_hito003.py` pertenecen a **esa extracción previa**, pese a conservar el prefijo 003. No certifican selección, interfaz, alcance, nuevos BP ni el estado actual de GameMode. Se conservan por trazabilidad y no se ejecutan ni renombran para aparentar cobertura de este hito.

## Git, alcance documental y continuidad

Esta revisión solo prepara/actualiza Markdown: README, continuidad de 001/002, contexto de arquitectura, este hito y su registro de evidencias. No cambia lógica C++, assets, Config, scripts funcionales, índice, commits ni tags. La consulta Git desde esta sesión no pudo acceder al directorio; no se atribuye un HEAD, rama o estado de seguimiento actuales a partir de registros antiguos.

No se propone una etiqueta nueva basándose solo en esta revisión. Pendiente: registrar compilación y aceptación visual/funcional actual, verificar mappings y overrides, y adjuntar demostración. La refactorización no ocupa un hito separado. El Hito 004 de atributos no se inicia ni se desarrolla en esta entrega.
