# Krerus — validación multijugador del Guerrero

30 de octubre de 2026. Unreal Engine 5.8.4. Base del repositorio: `c5dbbe52d2be706fdcb45e6ef7143b506ecb6a70`.

## Resultado

La ejecución del 30/10/2026, de 03:32:47 a 03:33:19 (Europe/Madrid), terminó con **16 comprobaciones superadas y 0 fallos**. Se usó PIE con dos jugadores, servidor anfitrión y cliente, en un proceso. Las peticiones de ataque se emitieron desde el Pawn propietario del cliente a través de los RPC existentes; no se llamó directamente a sus implementaciones de servidor.

| Comprobación | Resultado |
| --- | --- |
| Pawn de servidor con autoridad y Pawn local de cliente sin autoridad | Correcto |
| Enemigo de servidor y réplica de cliente distintos | Correcto |
| Tres impactos reales de montaje generan 15 Ira | Servidor 15, cliente 15 |
| Daño de autoataque aplicado una vez por impacto | 30 total; 3 × 10 con 50 armadura |
| Ira, vida del enemigo y armadura tras autoataque | Iguales entre servidor y cliente |
| Primer Golpe demoledor | 15 Ira consumidos y 10 de daño |
| Fractura | Armadura 50 → 42,5, replicada al cliente |
| Segundo golpe, aproximadamente 5 segundos después | Coste 15; daño conforme a armadura 42,5; armadura permanece 42,5 |
| Tiempo tras el primer golpe | Sigue en 42,5 pasados 11 segundos desde el primero |
| Expiración tras reaplicar | Ambas instancias vuelven a 50 después de la duración renovada |
| Lanzamiento sin Ira | Rechazado sin daño ni debuff |
| Lanzamiento fuera de alcance | Rechazado sin daño ni coste |

El reporte automático desglosa las 16 aserciones en `Docs/Validation/WarriorMultiplayer_PASS.md`. Los tiempos tienen la resolución de tick de PIE; la prueba comprueba persistencia a los 11 segundos del primer golpe y expiración a los 10,5 segundos de observar el segundo, no precisión de cronómetro submilisegundo.

## Fallo encontrado y corrección

La primera ejecución falló: servidor y cliente permanecían en 5 Ira y 9990 de vida del enemigo después del primer impacto. En el log se veían nuevos swings cada pocos segundos y finalizaciones interrumpidas de montajes, sin nuevos impactos. El editor en segundo plano estaba limitado a aproximadamente 3 FPS. Esa evidencia se conserva en `Docs/Validation/WarriorMultiplayer_before_fix.md`.

Un montaje anterior podía terminar su blend-out después de comenzar el siguiente. Su callback de finalización invalidaba el impacto pendiente del nuevo swing. `AKrerusCharacter` ahora incrementa una generación antes de `Montage_Play` y enlaza un callback débil que solo acepta la generación actual. Así se ignoran finalizaciones atrasadas sin retirar la invalidación cuando se interrumpe el montaje vigente.

La misma prueba y las mismas condiciones de baja frecuencia de tick pasaron tras la corrección. Compilación `KrerusEditor Win64 Development`: `Result: Succeeded`, salida 0. `git diff --check` también pasa.

## Cómo repetirla

1. Compilar KrerusEditor y abrir una sesión nueva de PIE en `L_DevSandbox`, modo Listen Server, dos jugadores, un proceso. No usar una partida en curso: la prueba modifica la fixture temporal.
2. Ejecutar `Krerus.Test.WarriorMultiplayer` desde la consola de Unreal/Output Log. También se puede arrancar el editor con `-ExecCmds="Krerus.Test.WarriorMultiplayer"` y comenzar PIE antes de 120 segundos.
3. Esperar el mensaje `LogKrerusValidation: COMPLETE: PASS (0 failures)` o leer el fallo. El reporte se escribe en `Saved/Validation/WarriorMultiplayer.md` y reemplaza el resultado de la ejecución anterior.
4. Detener PIE al acabar. La prueba no guarda assets ni mapas; todos sus cambios de fixture desaparecen al cerrar los mundos PIE.

El código está en `Source/Krerus/Private/Tests/WarriorMultiplayerValidation.cpp`, bajo `#if WITH_EDITOR`. No existe en el juego empaquetado. Solo se activa mediante el comando explícito y no se ejecuta automáticamente al abrir el proyecto normalmente. Si se interrumpe PIE, informa del fallo; si no encuentra los actores, vence a los 120 segundos. No iniciar dos pruebas a la vez.

## Alcance y límites

- La fixture coloca el Pawn remoto frente al enemigo y fija en servidor armadura 50, vida máxima/actual 10000 y Ira inicial 0. El objetivo con más vida evita que muera durante la secuencia. Los impactos proceden de los montajes reales: la prueba no inyecta eventos de impacto ni aplica el GE de daño directamente.
- Después de comprobar los tres impactos, la fixture fija Ira en 30 para poder realizar dos golpes y comprobar el refresco. Esto no pretende demostrar que esos 30 puntos proceden del autoataque.
- El cliente pide activaciones mediante los RPC de producción; no se valida aquí el clic de selección, Enhanced Input, cámara, HUD ni predicción visual.
- Las lecturas de ASC de ambos mundos ocurren dentro del proceso de prueba para comparar réplicas; esto no sustituye pruebas de serialización entre procesos independientes.
- No se han probado servidor dedicado, latencia artificial, pérdida de paquetes, múltiples clientes simultáneos, enemigos muertos, inmunidad ni destrucción de objetivo durante callbacks. Las aserciones de autoridad comprueban roles y ejecución por RPC válida; no son una auditoría exhaustiva contra clientes maliciosos.
- Continúa el aviso de `DefaultSlot` duplicado de la animación; no se han editado assets para resolverlo.
- Se restauró la configuración PIE guardada anterior a la prueba y el editor se cerró normalmente.

## Continuación

El diseño para extraer la base común está en `Docs/DisenoHabilidadesCombateGAS.md`. La jerarquía de habilidades y sus assets todavía no se han migrado. La prueba proporciona la regresión que se repetirá al hacerlo. No se ha creado ni reescrito ningún commit durante esta continuación.
