# Guía de replicación para ampliar Krerus

Fecha: 28/10/2026. Aplicación: Unreal Engine 5.8.4, Listen Server, C++ + Blueprint + GAS.

## 1. Decidir quién conoce y quién decide cada dato

El cliente propietario recoge entrada y selección. El servidor decide si la acción existe, cuándo ocurre, qué alcanza y cuánto daño causa. Cada máquina representa los resultados recibidos. Un Listen Server reúne servidor y jugador local en el mismo proceso: su presentación debe ejecutarse una sola vez.

| Elemento nuevo | Autoridad y transporte | Presentación / persistencia |
| --- | --- | --- |
| Ataque básico | Server RPC en Character poseído con objetivo explícito; cadencia del servidor | Multicast del montaje corto; GE aplicado por notify del servidor |
| Habilidad GAS | Concesión del servidor; activación GAS o RPC con intención; validar TargetData | AbilityTask y replicación de montage del ASC, o un único circuito visual propio |
| Impacto | El servidor valida alcance, vida y resultado | GameplayCue o evento cosmético con posición y normal |
| Niagara breve | El servidor confirma el acontecimiento | Cue o multicast generalmente no fiable; un spawn por instancia |
| Niagara persistente | Efecto/tag/estado replicado con duración y origen | OnRep/Cue de duración crea y retira el componente; funciona al entrar tarde |
| Sonido de golpe | Resultado confirmado por servidor | Audio local en cada receptor; atenuación espacial; deduplicar predicción |
| Muerte/cadáver | Health GAS y decisión del servidor | Estado replicado; OnRep restaura colisión y apariencia; servidor controla expiración |
| Proyectil con gameplay | Spawn y daño del servidor, actor replicado | Movimiento replicado o simulación cosmética reconciliada |

Replicar un SkeletalMesh o un actor no replica automáticamente sus llamadas Montage_Play, Niagara ni audio. Tampoco convierte un AnimNotify en un mensaje de red.

## 2. Server RPC: entrada con objetivo explícito

Usar un actor que pertenezca a la conexión: Character poseído o PlayerController. No enviar un Server RPC desde un enemigo que el jugador no posee. En Krerus, `ServerStartAutoAttack(Target)` inicia, `ServerStopAutoAttack()` detiene y `ServerSetAutoAttackPaused(bool)` gestiona pausa.

El cliente envía una referencia al actor objetivo, nunca el daño final, un GE arbitrario ni una clase de habilidad elegida libremente. El servidor comprueba validez, mismo mundo, estado vivo, alcance y condiciones del atacante. El motor restringe quién puede enviar el RPC por la propiedad de red del actor; la implementación también comprueba HasAuthority.

Volver a validar en el instante del impacto: entre solicitud y notify el objetivo puede moverse, morir o desaparecer. Añadir equipo/facción, línea de visión, arma, tags de bloqueo y recursos cuando esas reglas existan en el diseño. Actualmente el autoataque conserva distancia horizontal y semiplano frontal; no tiene prueba de obstáculos.

Usar Reliable para órdenes discretas que no deben perderse. No enviarlo cada Tick. Una implementación de producción expuesta a spam debe además limitar solicitudes por conexión; la cadencia de daño no sustituye un límite de tráfico.

## 3. Añadir un montaje de ataque al circuito actual

1. Preparar Skeleton y Slot compatible en el AnimBlueprint. Evitar dos nodos DefaultSlot con el mismo nombre en rutas conflictivas.
2. Asignar el Montage en los defaults del Character correspondiente. Verificar que también carga en servidor.
3. Colocar el notify de impacto y hacer que termine en `OnAutoAttackImpact`. Configurar el notify para dispararse en servidor dedicado, no filtrarlo por LOD/chance y comprobar qué ocurre al saltar secciones.
4. El temporizador autoritativo llama al multicast. Este reproduce en servidor, propietario y observadores. No reproducir además en el input ni desde OnRep del estado del combate.
5. Únicamente el notify del servidor puede aplicar el GE. Los notifies de clientes pueden crear presentación, pero no daño.
6. Invalidar el golpe al detener, pausar, interrumpir o cambiar objetivo. No permitir que la animación del objetivo anterior dañe al nuevo.
7. Verificar cadencia y duración: si un montaje sigue activo, el circuito actual omite el siguiente intento. Mantener una única aplicación de daño por ataque.

El componente conserva `bAttackImpactConsumed`: varios notifies de impacto dentro del mismo ataque solo permiten un GE. Para combos o ataques de varios impactos, sustituirlo conscientemente por identificadores de ataque y de ventana, más un conjunto de objetivos alcanzados por ventana. No retirar la protección sin reemplazarla.

La malla del servidor debe avanzar animación aunque no esté visible. El Character configura AlwaysTickPoseAndRefreshBones y desactiva optimización de frecuencia en autoridad. Es una solución fiable para conservar notifies, con coste de CPU: medirlo antes de escalar a muchos actores. Un futuro sistema puede separar el reloj de impactos del render, pero no debe aplicar daño tanto por reloj como por notify.

## 4. Habilidades GAS desde el principio

Conceder habilidades solo en servidor. Inicializar ASC con PlayerState como Owner y Character como Avatar, tanto en PossessedBy como en OnRep_PlayerState. Conservar las notificaciones GAMEPLAYATTRIBUTE_REPNOTIFY y revisar el modo de replicación del ASC.

Elegir política conscientemente:

- ServerOnly para acciones cuya respuesta puede esperar confirmación. Strike continúa en esta política.
- LocalPredicted si hace falta respuesta inmediata. Usar el flujo de activación, prediction keys, costes/cooldowns y TargetData de GAS, con validación final del servidor.

En la versión actual Strike recibe un objetivo explícito a través del Character y lo consulta durante una activación síncrona. `StrikeTarget` se borra al volver TryActivateAbility. Si se añade una espera, un proyectil o un AbilityTask, copiar y validar el objetivo dentro de la instancia de la habilidad antes de esperar, o migrarlo a GameplayEvent/TargetData. No mantener la dependencia de ese campo transitorio después de ActivateAbility.

Aplicar daño con GameplayEffect desde el ASC del atacante al del objetivo. El servidor calcula SetByCaller y valida coste/cooldown antes de confirmar. No modificar simultáneamente Health GAS y la vida tradicional del enemigo. No añadir un segundo ApplyGameplayEffect en Blueprint al conservar el de C++.

Para montajes de habilidades, preferir el circuito del ASC con AbilityTask PlayMontageAndWait cuando se integre la habilidad completa. No mezclarlo con el multicast del autoataque para el mismo montaje: escoger un propietario de la reproducción.

## 5. Predicción sin doble reproducción

El autoataque de este cambio no predice montajes: espera al servidor. Eso evita la reproducción local más multicast y añade la latencia de confirmación.

Si más adelante se predice una animación, identificar la solicitud con una prediction key o secuencia. El propietario reconoce la confirmación y corrige posición/tiempo sin volver a iniciar el mismo montaje; los otros clientes sí lo reproducen. Ante rechazo, cancelar la animación y efectos predichos y restaurar recursos mediante GAS. No omitir siempre al propietario: también necesita confirmaciones de acciones que no llegó a predecir.

La predicción visual nunca autoriza daño. El servidor no acepta un mensaje «mi notify ha golpeado» como prueba suficiente de impacto.

## 6. Niagara, sonidos e impactos

Para una chispa, polvo o sonido breve, transmitir un evento cosmético confirmado, preferiblemente GameplayCue si pertenece a GAS. Enviar parámetros suficientes: actor/origen, ubicación, normal, superficie y un identificador de impacto si puede predecirse. No replicar miles de partículas individuales.

Elegir una sola ruta por efecto: notify visual local, Cue o multicast. Si el sonido ya está en el notify que se ejecuta en todas las mallas, añadir el mismo sonido al multicast lo duplica. Si el efecto solo debe salir cuando el golpe acierta, no dispararlo incondicionalmente desde la animación: usar el resultado confirmado del servidor.

Para fuego, aura o canalización, replicar estado/duración. Crear el componente local de forma idempotente, actualizarlo si ya existe y retirarlo al finalizar el estado. No confiar en que un multicast antiguo llegue a alguien que todavía no estaba conectado o no tenía el actor relevante.

## 7. Muertes, cadáveres y jugadores que entran tarde

La muerte es estado persistente. En Krerus `bIsDead` decide la presentación de muerte y `bCorpseExpired` restaura la retirada del cadáver. El servidor ejecuta el temporizador. Colisión y visibilidad deben aplicarse también en los clientes. Una recompensa o loot se concede solo en servidor; `BP_OnEnemyDeath` debe contener presentación o separar explícitamente su rama de autoridad.

Limitación actual: quien recibe por primera vez un enemigo ya muerto pero cuyo cadáver no ha expirado reproduce la muerte desde el inicio. Tampoco se reconstruye la fase de un autoataque corto que comenzó antes de que el actor fuera relevante. El siguiente ataque se recibe normalmente. La retirada del cadáver sí es estado persistente, no un multicast perdido.

Para una sincronización temporal completa, replicar una estructura con fase, secuencia, montaje/sección, velocidad y hora de inicio del servidor. Compararla con el reloj sincronizado del GameState. OnRep reproduce desde la posición calculada o instala directamente la pose de cadáver si ya terminó. Guardar la duración/expiración autoritativa y evitar volver a emitir sonidos, recompensas o notifies de daño al reconstruir. Probar entrada tardía durante ataque, muerte y después de expirar.

## 8. Checklist antes de dar por terminada una ampliación

- [ ] Listen Server y cliente remoto pueden iniciar la misma acción.
- [ ] Ambos observan la acción ajena y el propietario la ve una sola vez.
- [ ] El servidor rechaza objetivo nulo, destruido, muerto, lejano o inválido por las reglas del juego.
- [ ] Coste, cooldown y daño se aplican una sola vez, también con spam o notifies duplicados.
- [ ] Cambiar objetivo, parar, pausar, morir y desconectarse invalidan impactos pendientes.
- [ ] El servidor sin render sigue generando los impactos previstos.
- [ ] No hay daño de clientes ni dos fuentes de verdad para Health del enemigo.
- [ ] Los efectos persistentes se reconstruyen al entrar tarde o recuperar relevancia.
- [ ] Hay pruebas con latencia, pérdida de paquetes y procesos separados.
- [ ] Audio, Niagara, slots, pose final y desaparición se comprueban visualmente con render real.
- [ ] Compilar y revisar logs no se presenta como sustituto de esas pruebas visuales.
