# Hito 001 — Movimiento C++ y locomoción

Fecha de revisión: 22 de octubre de 2026.

## Objetivo y estado

Construir una base de control de personaje en Unreal Engine 5.8 para el portfolio de Krerus. La implementación está recogida en el commit existente `640f69d` (`feat: Hito 001 - Character movement and locomotion`). El autor comunica este hito completado en su alcance de movimiento y locomoción. Esta revisión documental no ejecuta una nueva prueba visual en Play; su registro sigue pendiente antes de etiquetar la entrega.

El salto y la animación de salto **NO están completados** y quedan fuera del alcance del Hito 001. No se han eliminado los enlaces C++ existentes a `ACharacter::Jump` y `StopJumping` ni el asset `IA_Jump`.

## Implementación

### Movimiento y dos modos de control

`Source/Krerus/Private/KrerusCharacter.cpp` implementa `Move` sobre una entrada 2D de Enhanced Input. Sus ejes generan las direcciones adelante y derecha usando únicamente yaw.

- **Sin mantener clic derecho:** al comenzar a moverse se guarda `GetActorRotation()` en `FreeMovementRotation`. Esa referencia se conserva durante el movimiento, mientras `bOrientRotationToMovement` orienta el personaje hacia su desplazamiento. No se recalcula el marco de movimiento en cada giro del personaje.
- **Manteniendo clic derecho:** `StartCameraControl` activa el movimiento relativo a `Controller->GetControlRotation()`. Al soltarlo, `StopCameraControl` desactiva ese modo y reinicia `bWasMoving` para capturar una nueva referencia.
- `StopMovementInput`, enlazado al evento Completed, reinicia la referencia al terminar la entrada.

`PawnClientRestart` añade el Mapping Context al subsistema local. Los controles previstos son WASD, ratón, clic derecho mantenido y Shift izquierdo. En el asset `IMC_Krerus` se han encontrado referencias a las acciones correspondientes, Mouse2D, RightMouseButton y LeftShift; verificar en el editor los modificadores y mappings efectivos de WASD.

### Cámara

`CameraBoom` es un Spring Arm con longitud C++ por defecto de 1400 cm y rotación del controlador. `FollowCamera` se conecta a su socket y no aplica por su cuenta la rotación del Pawn. Tras el Hito 002, `Look` solo envía yaw/pitch cuando está activo el modo de cámara libre o el de control relativo. La descripción anterior de Look sin condición de modo ha quedado obsoleta. Ver [Hito 002](002-mmorpg-camera.md) para órbita, captura del cursor, zoom, límites, CameraLag y colisiones. Los valores de Blueprint pueden sobrescribir los valores C++.

### Sprint y desaceleración

`WalkSpeed` vale 300 cm/s por defecto. `StartSprint` cancela la desaceleración y establece `MaxWalkSpeed = WalkSpeed * 2`, es decir, 600 cm/s. `StopSprint` activa `bIsDecelerating`.

Durante Tick, `FMath::FInterpTo` aproxima `MaxWalkSpeed` a 300 con velocidad de interpolación 5.0; con una diferencia de hasta 1.0 se fija exactamente el valor final. Se suaviza el límite de velocidad al soltar sprint; no es una curva de frenado personalizada ni una duración fija de transición.

### Animación Idle/Walk/Run

Assets existentes: `Content/Core/Character/AB_KrerusCharacter.uasset` y `BS_WalkRun.uasset`. La inspección estática del binario encuentra un Blend Space 1D, el parámetro `Speed`, referencias a animaciones Walk/Run y uso de `GetVelocity` en el Animation Blueprint.

El alcance previsto es Idle/Walk/Run en función de la velocidad. La lectura estática no acredita la muestra de Idle, los umbrales exactos ni la calidad visual de las transiciones. Confirmar en el editor el reposo a velocidad cero, Walk en torno a 300 y Run en torno a 600, así como que el Blueprint del personaje utiliza este Animation Blueprint.

## Validación para cerrar y grabar la demo

Esta revisión verifica fuentes, presencia y referencias de assets y estado Git. No se ha compilado ni ejecutado Play en esta sesión. La sesión anterior conserva una prueba de avance con W, pero no acredita toda la versión actual.

- [ ] Compilar KrerusEditor y abrir L_DevSandbox sin errores.
- [ ] Probar WASD y diagonales; soltar y volver a iniciar movimiento.
- [ ] Sin clic derecho, comprobar movimiento independiente de la cámara y giro del personaje.
- [ ] Mantener y soltar clic derecho mientras se mueve; comprobar ambos marcos de referencia.
- [ ] Comprobar cámara y que IA_Look se active en los momentos previstos.
- [ ] Verificar 300/600 y retorno gradual a 300 al soltar Shift; repetir sprint durante la desaceleración.
- [ ] Comprobar Idle/Walk/Run y transiciones al arrancar y detenerse.
- [ ] Grabar un clip breve de estos controles; dejar el salto fuera de la demostración.

## Revisión Git y publicación — registro previo al Hito 002

Estado inicial: árbol limpio en `main`; HEAD `640f69d`; un commit por delante de la referencia local `origin/main`. No se ha hecho fetch, por lo que esto no confirma el estado remoto actual. No había tags locales.

`.gitignore` excluye Binaries, Intermediate, Saved, DerivedDataCache, .vs, soluciones generadas y temporales. Se añaden exclusiones para caché Rider, salidas de compilación y configuración local con secretos. No se excluyen Source, Config, Content ni el archivo .uproject. No se encontraron archivos ya versionados que coincidieran con las exclusiones iniciales.

`.gitattributes` configura Git LFS para .uasset/.umap y otros formatos de producción. Los 21 assets/mapas versionados aparecen en LFS; `git lfs fsck --pointers` pasó. Los mayores archivos actuales rondan 3 MiB; los PNG de referencias, hasta unos 2.5 MiB, están en Git normal. No se necesita migrar el historial por tamaño para esta entrega.

Se encontró un `SecurityToken` de Android File Server en `Config/DefaultEngine.ini`, introducido en un commit anterior. Se retira del archivo actual sin reproducir su valor. **Eliminarlo del archivo no lo borra del historial**: antes de publicar, invalidar o regenerar ese token si sigue en uso. Cualquier limpieza del historial requiere decidirla y autorizarla por separado. El escaneo de patrones en Source/Config no encontró otras coincidencias; no constituye una auditoría exhaustiva de secretos o metadatos binarios.

No se han alterado mecánicas, assets ni commits existentes. No se ha creado commit, tag ni push.

Propuesta, sujeta a consentimiento y a completar la comprobación jugable:

- Commit: `docs: document milestone 001 movement and portfolio readiness`.
- Archivos: README.md, este documento, .gitignore y Config/DefaultEngine.ini.
- Tag anotado sobre el nuevo commit: `milestone-001-character-movement`.
- Mensaje de tag: `Hito 001: movimiento C++ y locomoción; salto pendiente`.

Antes del push autorizado, revisar el diff final, actualizar la información del remoto y confirmar los derechos de distribución de los assets. No se propone reescribir el commit funcional existente ni migrar archivos ya versionados.

## Continuidad tras el Hito 002

El registro Git anterior describe la revisión previa de 001; no representa el árbol limpio actual. Las propuestas de commit/tag de esa sección quedan como antecedente y son sustituidas por la propuesta `v0.2.0` del [Hito 002](002-mmorpg-camera.md), que incluye el estado actual y los cambios funcionales aún sin commit.

Se conserva la prueba histórica de avance con W como evidencia limitada; no sustituye la regresión completa. El salto continúa NO completado por falta de animación.

El próximo Hito 003 separará `KrerusCharacter` en responsabilidades/clases de PlayerController, cámara, movimiento e input. Se validará cada extracción con compilación, referencias de Blueprint y pruebas de regresión de los dos primeros hitos.