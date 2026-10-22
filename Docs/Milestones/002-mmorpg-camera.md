# Hito 002 — Cámara híbrida MMORPG

Fecha de revisión: 22 de octubre de 2026. Versión propuesta: **v0.2.0**, sin etiqueta creada.

## Objetivo y estado

Incorporar una cámara híbrida con cursor libre, órbita independiente del marco de movimiento, control relativo a la cámara y zoom suave. «MMORPG» describe el estilo de control; no implica multijugador ni infraestructura MMO.

El autor comunica el hito completado. Esta sesión confirma fuentes C++, presencia de assets y diferencias Git, pero no ha compilado ni ejecutado Play. Las pruebas siguientes son criterios de aceptación pendientes de registrar para esta revisión, no resultados aprobados.

## Comportamiento y decisiones técnicas

La implementación sigue concentrada en `AKrerusCharacter`, en `Source/Krerus/Public/KrerusCharacter.h` y `Source/Krerus/Private/KrerusCharacter.cpp`.

| Función | Implementación comprobada |
| --- | --- |
| Cursor libre | `PawnClientRestart` inicializa cursor visible y `FInputModeGameAndUI`; `Look` retorna si no hay modo de cámara activo |
| Órbita con clic izquierdo | `StartFreeCamera`/`StopFreeCamera` cambian `bIsFreeCameraActive`; habilitan Look sin activar el marco relativo de Move |
| Control con clic derecho | `StartCameraControl`/`StopCameraControl` cambian `bIsCameraControlActive`; Move usa el yaw de `GetControlRotation`; al soltar reinicia la referencia libre |
| Captura de ratón | `UpdateMouseCapture` oculta cursor y usa `GameOnly` mientras cualquiera de los modos está activo; vuelve a `GameAndUI` cuando ambos se desactivan |
| Zoom | `ZoomCamera` modifica la distancia objetivo con `Clamp`; Tick interpola `TargetArmLength` con `FInterpTo` |
| Límites verticales | `PawnClientRestart` fija `ViewPitchMin`/`ViewPitchMax` en el `PlayerCameraManager`, si existe |
| Suavizado de posición | Spring Arm con `bEnableCameraLag = true` y `CameraLagSpeed = 12`; lag de rotación desactivado para conservar respuesta directa al ratón |
| Colisiones | Cámara conectada al socket del Spring Arm; se delega en el mecanismo de colisión del componente, sin solver propio |

La correspondencia izquierda/derecha/rueda es la comunicada por el autor. La inspección de cadenas del Mapping Context encuentra `LeftMouseButton`, `RightMouseButton`, `MouseWheelAxis` y referencias a las nuevas acciones, pero no certifica sus asociaciones ni modificadores. Revisar el Mapping Context y las propiedades del Blueprint en el editor.

### Parámetros C++ por defecto

| Parámetro | Valor |
| --- | --- |
| Longitud inicial del Spring Arm | 1400 cm |
| Distancia mínima / máxima objetivo | 300 / 1800 cm |
| Paso de zoom por unidad de entrada | 150 cm |
| Velocidad de interpolación del zoom | 8 |
| Pitch mínimo / máximo | −75° / +45° |
| CameraLag de posición | 12 |

`BeginPlay` toma la longitud efectiva del Spring Arm como objetivo inicial; el Clamp se aplica al recibir zoom. Los límites de zoom limitan el objetivo, no garantizan una distancia física mínima ante obstáculos. `FInterpTo` expresa suavizado, no una duración fija. Los defaults editables pueden estar sobrescritos en Blueprint.

## Diferencias y límites de lo comprobado

- El README anterior se centraba en 001 y su documento describía un `Look` sin condición de modo. El código actual sí exige cámara libre o control activo; se actualiza esa descripción.
- `CameraFollowSpeed = 3` y `CameraFollowDelay = 0.25` están declarados pero no se usan en el código revisado. No hay seguimiento con retardo personalizado acreditado.
- La clase no asigna explícitamente `bDoCollisionTest`, canal ni tamaño de prueba del Spring Arm. Las colisiones comunicadas requieren comprobar configuración efectiva y obstáculos en Play; no se presentan como prueba ejecutada.
- El control relativo cambia el marco de movimiento; `bUseControllerRotationYaw` sigue desactivado y el personaje se orienta al desplazamiento. No se acredita un sistema adicional de strafe o alineación permanente al yaw de cámara.
- Los handlers de los modos y sprint enlazan Started/Completed, sin ruta explícita Canceled. Incluir cancelación, pérdida de foco y botones simultáneos en las regresiones.
- El salto sigue pendiente por falta de animación, aunque existen acción y enlaces C++.
- La conversación recuperada solo ofrece mensajes de progreso y referencias de contenido sin su texto expandido; no permite reconstruir más detalles de diseño que los aportados en la petición actual.

## Pruebas y evidencias

Comprobado en esta sesión: `.uproject` indica 5.8; fuentes y dependencias incluyen Enhanced Input; existen AB/Blend Space, mapa y acciones de cámara; revisión de diferencias Git. `git lfs fsck --pointers` devuelve OK para los punteros existentes; no valida por sí solo los nuevos assets sin seguimiento ni la subida de objetos al remoto. No se ha ejecutado compilación, Play ni revisado el vídeo local completo.

- [ ] Compilar `KrerusEditor`, abrir `L_DevSandbox` y comprobar ausencia de errores.
- [ ] Sin botones: cursor visible y desplazable; mover ratón no cambia yaw/pitch.
- [ ] Mantener izquierdo: orbitar en reposo y andando; conservar el marco de movimiento libre; soltar restaura cursor.
- [ ] Mantener derecho: girar cámara y verificar WASD relativo al yaw; soltar en movimiento y volver a entrar sin saltos de referencia.
- [ ] Pulsar ambos botones, soltarlos en ambos órdenes; el cursor vuelve solo cuando ninguno está activo.
- [ ] Probar rueda en ambos sentidos hasta 300/1800, transición suave y estabilidad a distintas tasas de frames.
- [ ] Comprobar pitch −75°/+45° y ausencia de inversión o giro vertical completo.
- [ ] Comprobar CameraLag de posición sin lag rotacional, al arrancar, parar y girar.
- [ ] Acercar cámara a paredes/esquinas, comprobar retracción y recuperación del Spring Arm y ausencia de clipping persistente.
- [ ] Perder/recuperar foco durante cada modo y sprint; comprobar que no quedan estados bloqueados.
- [ ] Repetir regresiones de 001: WASD/diagonales, 300/600, retorno gradual e Idle/Walk/Run.
- [ ] Registrar resultado, configuración y clip de demostración; mantener salto fuera del cierre.

## Revisión Git y preparación de portfolio

Estado inicial de esta revisión: rama `main`, HEAD `640f69d`, un commit por delante de la referencia local `origin/main`, sin tags locales. No se ha consultado el remoto ni hecho fetch. Índice sin cambios preparados.

Cambios previos conservados: `.gitignore`, `Config/DefaultEngine.ini`, Blueprint del personaje, Mapping Context y fuentes de `KrerusCharacter`; eliminación de `IA_CameraControl`. Nuevos sin seguimiento: README, Docs, `IA_CameraFreeControl`, `IA_CameraLockControl`, `IA_CameraZoom` y la carpeta `1.Videos`. No asumir un renombrado de la acción antigua sin revisar las referencias en Unreal.

`.gitignore` ya cubre Binaries, Intermediate, Saved, DerivedDataCache, .vs, soluciones/archivos IDE y temporales, además de salidas de plugins y configuración local. `git ls-files -ci --exclude-standard` no devuelve archivos versionados ignorados. Se conserva el archivo y sus cambios previos; no se necesita otra exclusión para esta entrega. Source, Config, Content y `.uproject` deben seguir versionados. No se borran generados ni se modifica el índice.

LFS está configurado para `.uasset`/`.umap`. Las tres acciones nuevas aún deben incorporarse mediante el filtro LFS. El vídeo local de unos 18.7 MB queda fuera de la selección propuesta; decidir su inclusión aparte, sin migrar historial ni añadirlo automáticamente.

La revisión previa de 001 registró retirada de un token de Android File Server de la copia actual de Config. Se conserva ese cambio sin reproducir el valor. Su retirada no sanea el historial; verificar invalidación si sigue vigente antes de publicar. Esta revisión no certifica una auditoría completa de secretos.

### Propuesta de commits y etiqueta

Tras revisar y validar los cambios funcionales existentes:

1. Commit funcional: `feat: complete milestone 002 hybrid MMORPG camera`. Seleccionar explícitamente ambos archivos C++, `BP_KrerusCharacter`, `IMC_Krerus`, las tres acciones nuevas y la eliminación de la acción antigua. Confirmar referencias de assets y resultados Play antes de cerrar.
2. Commit documental/preparación: `docs: document milestones 001-002 and prepare v0.2.0 portfolio`. Seleccionar README, ambos documentos de hitos y, tras revisar sus cambios previos, `.gitignore` y `Config/DefaultEngine.ini`.
3. Etiqueta anotada **`v0.2.0`** sobre el commit documental posterior al funcional. Mensaje propuesto: `Krerus v0.2.0: character movement and hybrid MMORPG camera; jump pending`.

No usar una selección global que incluya el vídeo sin revisión. No se ha creado ningún commit o tag, hecho push ni publicado. La etiqueta se propone para una entrega que incluya las fuentes y assets, no solo documentación. Push/publicación requieren confirmación explícita.

## Próximo Hito 003 — Refactorización

Separar responsabilidades/clases de PlayerController, cámara, movimiento e input actualmente concentradas en `KrerusCharacter`. Definir propietarios de estado y referencias de assets antes de mover código. Realizar extracciones pequeñas: compilar, comprobar asignaciones/serialización de Blueprint y repetir las pruebas 001/002 después de cada cambio. Mantener controles y parámetros como referencia de regresión; no combinar la refactorización con nuevas mecánicas. Revisar campos sin uso y gestión de cancelación con validación explícita.

## Propuesta breve para LinkedIn

Segundo hito de Krerus: cámara híbrida de estilo MMORPG en Unreal Engine 5.8 con C++ y Enhanced Input. Cursor libre, órbita con clic izquierdo, control relativo con clic derecho y zoom suave con límites, junto con CameraLag de posición y colisiones mediante Spring Arm. El siguiente paso será separar las responsabilidades de personaje, cámara e input, validando cada cambio. Documento los avances y sus límites como parte de mi portfolio. #UnrealEngine #CPP #GameDev #Portfolio

Borrador para usar tras confirmar las pruebas de cámara y colisiones; no publicado.


## Continuidad del Hito 003

El 22 de octubre de 2026 se implementaron extracciones incrementales de cámara, cursor y archivos de movimiento/input. KrerusEditor compila y se registraron 17 comprobaciones aprobadas en Play sin renderizado. La inspección visual sigue pendiente; no sustituye las pruebas anteriores ni completa el salto. Ver [arquitectura, compatibilidad y evidencias](003-cpp-architecture.md).
