# Replicación de combate y animaciones de Krerus

Fecha: 28/10/2026. Proyecto real: `E:/15.MiVideoJuego/Krerus`. Motor: Unreal Engine 5.8.4. Se trabaja sobre el estado que ya tenía el usuario, no sobre una copia antigua ni sobre HEAD.

## Problema y resultado del cambio

Antes, StartAutoAttack creaba un temporizador en la máquina que recibía la entrada. PerformAutoAttack reproducía el Montage únicamente allí. ApplyAutoAttackImpact exigía autoridad: por tanto, el cliente remoto veía su ataque pero no podía aplicar el GE en servidor; tampoco distribuía su montaje a los demás. El Listen Server sí recorría la rama de daño.

Ahora el Character poseído recibe solicitudes Server RPC con objetivo explícito. Solo el servidor programa la cadencia, valida impactos y aplica el GE. Un único multicast reproduce cada montaje aceptado en servidor y clientes; no se reproduce previamente desde el input. El estado de combate se confirma al propietario mediante replicación del componente.

Strike también consultaba SelectedNPC del PlayerController del servidor, aunque la selección era local. Ahora transmite un objetivo explícito al Character y activa en servidor una habilidad Strike ya concedida, conservando su daño, coste y CommitAbility.

## Cambios por archivo

Todas las rutas de código siguientes son relativas a `Source/Krerus/`.

| Archivo | Cambio concreto |
| --- | --- |
| Public/KrerusCharacter.h | Declara ServerStartAutoAttack(Target), ServerStopAutoAttack, ServerSetAutoAttackPaused, ServerActivateStrike(Target) y MulticastAutoAttackMontage; añade objetivo transitorio de Strike y getter. |
| Private/KrerusCharacter.cpp | Implementa RPC con guardas de autoridad; distribuye inicio/parada del Montage; busca una habilidad concedida derivada de Strike y la activa por Handle; fuerza evaluación de animación del servidor fuera de pantalla. Conserva el notify y la recuperación del CombatComponent existente. |
| Private/KrerusCharacterInput.cpp | ActivateStrike recoge la selección local y envía el objetivo por RPC en vez de pedir una activación sin TargetData. |
| Public/KrerusCombatComponent.h | Declara estado replicado al propietario, OnRep de ataque, replicación de pausa y objetivo y reloj NextAttackTime para impedir eludir cadencia al pausar. |
| Private/KrerusCombatComponent.cpp | Activa replicación del componente; cliente solicita, servidor ejecuta. Valida vida del atacante, vida GAS/cadáver del enemigo, mundo, rango y frente. Solo servidor crea temporizador y consume daño. Multicast sustituye la reproducción local. Stop/pausa invalidan impacto y paran montaje. Reanudar respeta NextAttackTime. OnRep comunica inicio/parada al propietario. |
| Public/KrerusPlayerController.h | Expone SetSelectedNPC como BlueprintCallable para selección desde UI/Blueprint y pruebas diferidas; no replica selección visual a otros jugadores. |
| Private/KrerusPlayerController.cpp | El mensaje de inicio pasa a «solicitud enviada», porque enviar no equivale a aceptación del servidor. Conserva selección, cancelación y ToggleAutoAttack. |
| Private/KrerusStrikeAbility.cpp | Exige autoridad, consulta el objetivo explícito del Character, comprueba atacante vivo, mundo, vida GAS y distancia finita antes de CommitAbility. Conserva GE de daño y coste existentes. |
| Public/KrerusEnemy.h | Añade expiración persistente replicada del cadáver, OnRep, multicast de reacción y protección de presentación de muerte. Corrige comentarios obsoletos sobre vida tradicional. |
| Private/KrerusEnemy.cpp | Solo autoridad convierte bajadas de Health GAS en reacción/muerte. Reacción usa multicast no fiable; muerte usa bIsDead persistente. Aplica colisión también en clientes; evita repetir BP_OnEnemyDeath. Expiración usa bCorpseExpired y OnRep para ocultar/desactivar colisiones y limpiar selección local. CorpseDuration <= 0 expira inmediatamente. |

Inspeccionados sin modificar: KrerusPlayerController.h/.cpp antes del cambio, KrerusPlayerState y su ASC, KrerusAttributeSet con atributos replicados y REPNOTIFY_Always, KrerusNPC, declaraciones de Strike y circuito de entrada. El AttributeComponent del atacante continúa aportando AttackPower y estado de vida: no se ha hecho una migración completa del jugador a Health GAS.

El enemigo conserva físicamente su componente heredado por compatibilidad. El inicio y la ejecución del autoataque ya no consultan su vida tradicional ni se suscriben a su OnDeath. La fórmula sigue siendo BaseAttackDamage + AttackPower * AttackPowerCoefficient; el GE usa Data.Damage negativo como antes.

No se han editado ni guardado assets Blueprint, GameplayEffects, montajes, niveles ni notifies. Los cambios previos del usuario en esos assets se conservan.

## Flujo de red exacto

1. El propietario selecciona un enemigo y pulsa T. ToggleAutoAttack mantiene la API del CombatComponent.
2. En cliente, StartAutoAttack envía `Character.ServerStartAutoAttack(CombatTarget)`. Stop y pausa usan RPC equivalentes. La propiedad del Pawn limita el RPC a su conexión; cada implementación también exige autoridad.
3. El servidor obtiene su CombatComponent, valida objetivo y atacante, fija objetivo y crea un único temporizador. Repetir Start con el mismo combate activo no crea otro.
4. El temporizador comprueba pausa, vida GAS, rango horizontal y arco frontal, y evita solapar montajes. El reloj NextAttackTime impide acelerar al reanudar o cambiar objetivo.
5. El servidor emite MulticastAutoAttackMontage. Se ejecuta una vez en servidor y una vez en cada receptor relevante, incluido el propietario. OnRep del estado de combate no reproduce el Montage.
6. AttackImpact entra por OnAutoAttackImpact. En clientes se descarta antes de consumir gameplay. En servidor exige montage activo y golpe pendiente; consume una sola vez y vuelve a validar antes de construir/aplicar el GE desde ASC atacante al ASC enemigo.
7. GAS modifica Health. Solo el servidor emite la reacción o fija bIsDead. La muerte tiene prioridad sobre reacción. Cada máquina aplica la presentación de muerte de forma idempotente.
8. El temporizador del servidor fija bCorpseExpired. El estado replica la retirada y las colisiones; no depende de que el otro jugador haya recibido un multicast anterior.

Strike: entrada local → ServerActivateStrike(Target) → buscar Handle de una Strike concedida → TryActivateAbility → validación y CommitAbility → GE. El campo StrikeTarget dura solo esa activación síncrona; consultar la guía antes de añadir AbilityTasks o esperas.

## Anim Notify revisado

El montaje AM_AutoAttack usa DefaultSlot y contiene el notify de nombre AttackImpact, situado aproximadamente a 0,631613 s del clip original. La inspección mediante AnimationLibrary confirma TriggerChance=1, NoFiltering y bTriggerOnDedicatedServer=True. No se ha reconfigurado ni duplicado el notify. La validación de daño en PIE demuestra que el circuito de impacto llega al GE en ese escenario; no implica haber revisado cada nodo de todos los AnimBlueprints.

## Pruebas y evidencias

Los resultados finales se registran en `replicacion_pie.json`, `replicacion_pie.log` y `replicacion_build_final.log` del espacio de trabajo. La prueba utiliza dos mundos PIE de un mismo proceso, Listen Server y cliente conectado por loopback, sin render (NullRHI), sin audio y sin guardar assets.

Prueba multijugador principal completada sin excepción ni timeout: doce comprobaciones satisfactorias en el JSON.

| Comprobación | Evidencia y alcance |
| --- | --- |
| Inicio desde cliente | La solicitud sale mediante temporizador nativo diferido de ToggleAutoAttack; el servidor entra en ataque y el propietario recibe el estado. |
| Montage del cliente visto por ambos | 45 muestras del circuito; se observa Montage activo simultáneamente en el Pawn de servidor y el de cliente. Es una comprobación de estado de animación, no de imagen renderizada. |
| Daño remoto GAS | Siete aplicaciones de 15 puntos: 100 → 85 → 70 → 55 → 40 → 25 → 10 → 0. Los impactos válidos se separan por la cadencia y las pausas/paradas. |
| Pausa/reanudación | Pausa aplicada directamente en servidor; no hay impactos durante la ventana de pausa y se reanuda después. Esta prueba no certifica el RPC de pausa originado desde cliente. |
| Stop desde cliente | StopAutoAttack diferido desde el componente del cliente; el servidor detiene combate. |
| Fuera de rango | Tras mover al Pawn en autoridad a 1200 unidades y replicar la posición, un nuevo inicio desde cliente queda rechazado en servidor. |
| Muerte por cliente | IsCorpse verdadero en ambos mundos; el log de servidor registra muerte al agotar Health GAS. |
| Expiración del cadáver | A los 32 s posteriores a la muerte se comprueba colisión global desactivada en servidor y cliente; el temporizador de cadáver es de servidor. No es una verificación visual de ocultación. |
| Ataque del anfitrión | Inicio aceptado, Montage activo en ambos mundos y muerte del segundo enemigo observada por ambos. |

Prueba GAS adicional completada sin excepción ni timeout, con cinco comprobaciones satisfactorias (`replicacion_gas_pie.json`):

- Strike activado desde el cliente mediante su tecla One del contexto real de entrada: vida del enemigo 100 → 85 y maná del atacante 100 → 80. El servidor registra una aplicación de Strike y su resultado.
- Autoataque posterior: vida 85 → 70.
- Con ese impacto ya consumido, dos llamadas adicionales de impacto en servidor y una en cliente mantienen la vida en 70; no se aplica daño extra.
- Stop enviado desde cliente deja la vida en 70 durante la ventana posterior de tres segundos.

Estas pruebas suman 17 comprobaciones satisfactorias. El RPC de pausa desde cliente, la entrada por clic real y la representación con render no se consideran validados por esta automatización.

`git diff --check -- Source/Krerus` pasó. Las diez fuentes aplicadas se compararon byte a byte con la propuesta y se generaron hashes y un parche respecto al estado previo. Tras la compilación solo se ajustaron dos comentarios del componente, sin cambiar lógica.

La compilación normal encontró una DLL de Hot Reload bloqueada por el editor del usuario. Se compiló KrerusEditor / Win64 / Development con ModuleWithSuffix=Krerus,4822; resultado Succeeded. No se cerró el editor del usuario. El módulo alternativo permite verificar C++ y lanzar una instancia nueva; no equivale a una compilación Shipping ni a verificar todos los estados serializados de un Blueprint tras Hot Reload.

La primera automatización detectó una recursión causada por la ejecución Python de RPC como llamadas locales en contexto editor. Se añadieron guardas HasAuthority en las implementaciones y la prueba se cambió a temporizadores nativos diferidos para que Start/Stop salgan realmente del cliente por red. También se corrigieron nombres de API Python y propiedades reflejadas del script de prueba. El log del primer intento se conserva por separado. Los intentos interrumpidos no cuentan como pruebas satisfactorias.

## Límites y comprobaciones manuales

- NullRHI permite comprobar estado de Montage, red y notifies, pero no certifica poses, mezcla de slots, audio o Niagara en pantalla.
- Hay advertencias existentes de DefaultSlot duplicado en AB_KrerusCharacter. No se modificó el grafo sin poder inspeccionarlo por el puente, que estaba inaccesible. Revisar visualmente el AnimGraph.
- Sin predicción de ataque: el propietario espera la confirmación del servidor. Multicast de ataque corto no reconstruye un golpe ya iniciado antes de entrar tarde o recuperar relevancia; se recibirá el siguiente. No hay sincronización exacta de fase frente a latencia.
- bIsDead y expiración sí son estado persistente, pero un nuevo receptor de un cadáver aún visible reproduce la muerte desde el principio. No se ha añadido reconstrucción de pose final ni hora de muerte. No se implementa respawn.
- No se añadieron reglas de facción ni línea de visión a través de obstáculos: se conserva el alcance y frente originales. No se implementó limitación de tráfico por conexión; sí se limita la cadencia del daño.
- BP_OnEnemyDeath debe reservarse para presentación o separar la lógica autoritativa: revisar recompensas/loot si se añaden en ese evento.
- Falta verificar procesos separados, servidor dedicado real, entrada tardía real, pérdida de paquetes, latencia artificial, desconexión durante un golpe, varias armas y esqueletos, y habilidades asíncronas.

Checklist manual de aceptación:

- [ ] Reiniciar el editor con los cambios C++ y abrir L_DevSandbox.
- [ ] PIE con Listen Server y dos jugadores; seleccionar por clic y pulsar T desde cada ventana.
- [ ] Ver un único ataque por jugador en ambas ventanas, sin saltos de slot ni sonido duplicado.
- [ ] Revisar 15 puntos por impacto con los defaults actuales, sin dos aplicaciones del GE para un mismo ataque.
- [ ] Parar, pausar/reanudar, cambiar objetivo y cancelar antes del notify; comprobar que no queda daño pendiente.
- [ ] Probar sin objetivo, fuera de rango, detrás, muerto y destruido.
- [ ] Strike desde cliente con selección propia y coste aplicado una sola vez.
- [ ] Matar con cada jugador, comprobar reacción/muerte y retirada del cadáver en ambas ventanas.
- [ ] Repetir con latencia/pérdidas y con un jugador que entra durante muerte y después de la expiración.

## Git, preservación y entrega

El repositorio ya tenía modificaciones en Enemy, Strike, GameplayEffects, Blueprint enemigo, documentación, eliminaciones de GASLab y un vídeo no seguido. Se guardaron copias previas de los diez archivos de código afectados en `ReplicacionBackup/Source/Krerus` del espacio de trabajo. El parche de esta tarea se calcula contra esas copias, no contra HEAD, para separar los cambios previos.

No se crea un commit parcial que deje fuera Enemy/Strike, ni un commit que incorpore sus cambios previos sin distinguirlos. Los cambios quedan aplicados para revisión conjunta. Mensaje sugerido al integrar el conjunto revisado: `fix(combat): authorize attacks on server and synchronize combat presentation`.

Los dos informes se entregan como `Docs/01_InformeCambiosReplicacion.md` y `Docs/02_GuiaReplicacion.md` y se conserva una copia en ReplicacionPreparada. La guía complementaria explica cómo añadir habilidades, montajes, notifies, Niagara, sonidos e impactos sin duplicaciones y qué ampliar para entrada tardía y predicción.

Evidencias y reproducción, en `C:/Users/Jose/Documents/ChatGPT/MiVideoJuego/`:

- `replicacion_build_final.log`: compilación satisfactoria del módulo 4822.
- `replicacion_pie.py`, `replicacion_pie.json`, `replicacion_pie.log`: prueba principal de dos jugadores.
- `replicacion_gas_pie.py`, `replicacion_gas_pie.json`, `replicacion_gas_pie.log`: Strike, coste y deduplicación.
- `ReplicacionPreparada/manifest.json`: hashes del antes y después de los diez archivos.
- `ReplicacionPreparada/cambios-replicacion.patch`: delta exclusivo de esta tarea respecto a la copia previa.
- `ReplicacionBackup/Source/Krerus/`: copia de recuperación de las fuentes afectadas.

Para repetir las pruebas, lanzar una instancia separada de UnrealEditor con el proyecto, `-nullrhi -unattended -nosound -nop4`, el `-ExecutePythonScript=` correspondiente y una ruta `-abslog=`. Requieren la configuración PIE Listen Server / dos jugadores / un proceso que ya tenía guardada el proyecto. Los scripts colocan personajes y atacan únicamente en los mundos temporales PIE; no guardan los niveles ni assets. Ejecutar las dos pruebas de forma secuencial para no competir por el puerto PIE.
