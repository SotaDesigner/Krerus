# Hito 005 — Autoataque y ciclo de vida

Revisión: 27 de octubre de 2026. Unreal Engine 5.8.
Estado: terminado y validado en PIE según el autor; fuentes revisadas y build de editor validada en este cierre.

## Estado real de Git

Base del cierre: 3480d8d. Solo estaba pendiente KrerusCombatComponent.cpp: mover la suscripción OnDeath del objetivo después de todas las validaciones de StartAutoAttack. Se conserva en 94e8fed, sin otros cambios funcionales.

Ya estaban guardados:
- 6e8e2b1: implementación de combate, incluida la limpieza por muerte y EndPlay presente en HEAD.
- 89fd077: recuperación de CombatComponent en BeginPlay del Character antes de Super, además de cambios de combate, controller y Blueprint. Se recupera un componente existente mediante FindComponentByClass; no se crea otro.
- 3480d8d: reinicio del temporizador al reanudar y ajuste del controller.

No se vuelven a atribuir estos cambios al commit de cierre. No había otros archivos modificados ni assets pendientes al inicio. No se reescribe historial, no se crean etiquetas ni se hace push. Las reglas LFS se conservan.

## Arquitectura y flujo comprobados por código

Las rutas siguientes son relativas a Source/Krerus.

| Elemento | Responsabilidad |
| --- | --- |
| KrerusCombatComponent.h/.cpp | Objetivo, estados, temporizador, validación, daño y delegates |
| KrerusCharacter.cpp | Subobjeto de combate, recuperación de referencia, reproducción de Montage, impacto y fin de animación |
| KrerusCharacterInput.cpp | Enlace de AutoAttackAction al controller |
| KrerusPlayerController.cpp | Selección y alternancia del autoataque |
| KrerusAttributeComponent | Vida, AttackPower y señal OnDeath |
| KrerusEnemy | Estado de cadáver y ciclo de muerte del enemigo |

El componente de combate no usa Tick. CombatTarget es TObjectPtr; SelectedNPC del controller es una referencia débil.

1. AutoAttackAction llama a OnAutoAttack y ToggleAutoAttack. Si está activo, se detiene. Si no, solo acepta una selección convertible a AKrerusEnemy, asigna objetivo e intenta iniciar. La tecla T aparece en el mensaje de diagnóstico; el mapping efectivo de Blueprint no se ha inspeccionado.
2. StartAutoAttack rechaza objetivo inválido, doble inicio, propietario inválido, jugador muerto/sin atributos, objetivo muerto/sin atributos y cadáver. Solo después enlaza OnDeath del objetivo.
3. Activa bIsAutoAttacking, programa un temporizador repetitivo y emite OnAutoAttackStarted. El primer ciclo espera el intervalo; no hay golpe inmediato al iniciar. GetAttackInterval limita el intervalo a 0,1 s como mínimo.
4. PerformAutoAttack retorna si está pausado. Valida propietario, objetivo y vida; ante invalidez detiene el combate. Fuera de alcance o arco frontal omite ese ciclo sin desactivarlo. Si ya hay Montage activo, también omite el ciclo.
5. Antes de reproducir abre una oportunidad de impacto: bAttackImpactConsumed=false. Character reproduce el Montage, ajustando su duración al 90 % de un intervalo limitado allí a un mínimo de 1 s.
6. OnAutoAttackImpact delega en ApplyAutoAttackImpact. Solo acepta Montage activo y una oportunidad sin consumir; la consume antes de validar estado, vida, alcance y orientación. El daño es BaseAttackDamage + AttackPower × AttackPowerCoefficient. Defaults C++: 10 + AttackPower × 0,5; alcance horizontal 200 uu; intervalo 2 s. Arco: producto escalar 2D >= 0.
7. ApplyDamage actualiza atributos y puede emitir muerte; después se emite OnAutoAttackHit. La interrupción del Montage cancela la oportunidad pendiente.

Los defaults no certifican los valores efectivos de Blueprint. La conexión del notify de animación a OnAutoAttackImpact requiere comprobación en editor.

## Pausa y reanudación

bIsAutoAttacking expresa combate habilitado; bAutoAttackPaused impide que PerformAutoAttack lance otro ciclo. Pausar no llama a PauseTimer ni limpia el temporizador: este sigue invocando el callback, que retorna pronto. Asignar de nuevo el mismo estado no produce trabajo.

Reanudar un combate activo limpia el temporizador, intenta PerformAutoAttack inmediatamente y vuelve a programarlo únicamente si el combate sigue activo. Así evita conservar el vencimiento anterior. Un Montage todavía activo puede hacer que ese intento inmediato se omita.

En las fuentes revisadas no hay llamadas externas a SetAutoAttackPaused: la integración puede estar en Blueprint y no se certifica aquí. ApplyAutoAttackImpact no consulta la pausa: esta API por sí sola no garantiza cancelar el impacto de una animación ya iniciada. CancelPendingAutoAttackImpact existe para invalidarlo; la interrupción del Montage lo utiliza. No se afirma que cualquier pausa visual lo invoque.

## Cambio de objetivo, muerte y EndPlay

- SetCombatTarget conserva el estado si recibe el mismo objetivo. Con otro objetivo llama a StopAutoAttack antes de sustituir la referencia.
- Cambiar SelectedNPC en el controller limpia el objetivo de combate. Seleccionar otro enemigo no reinicia automáticamente el ataque; ToggleAutoAttack debe iniciarlo.
- BeginPlay del componente se suscribe a OnDeath del propietario cuando personaje y atributos son válidos. OnOwnerDeath y OnCombatTargetDeath llaman a StopAutoAttack.
- StopAutoAttack retira el delegate del objetivo válido, desactiva combate, consume el impacto pendiente, restablece pausa y limpia el temporizador si existe mundo. Emite OnAutoAttackStopped solo si antes estaba activo.
- StopAutoAttack no borra CombatTarget ni detiene expresamente el Montage; el impacto pendiente queda invalidado. El controller y SetCombatTarget gestionan el cambio de referencia.
- EndPlay detiene primero el autoataque, retira la suscripción a la muerte del propietario y llama a Super. Las comprobaciones de validez durante ciclos e impactos complementan las señales de muerte.

## Validación y límites de la evidencia

| Evidencia | Resultado |
| --- | --- |
| Comunicación del autor | Hito 005 y limpieza de ciclo de vida terminados y validados en PIE |
| Revisión de fuentes y diff | Flujos anteriores y único cambio pendiente confirmados; no se certifican gráficos Blueprint |
| Build de cierre | KrerusEditor Win64 Development: Result: Succeeded, salida 0 |
| Alcance de build | Ejecución incremental: tres acciones de enlace/metadatos, sin recompilación C++ completa en esta ejecución |
| git diff --check | Sin errores en el cambio funcional |
| git lfs status | Sin assets pendientes al inicio; reglas LFS intactas |
| PIE ejecutado por Codex en este cierre | No ejecutado |

Comando utilizado:

    E:/3.Epic/UE_5.8/Engine/Build/BatchFiles/Build.bat KrerusEditor Win64 Development -Project=E:/15.MiVideoJuego/Krerus/Krerus.uproject -WaitMutex -NoHotReloadFromIDE

No había proceso UnrealEditor detectado en la comprobación previa. No se abrió ni cerró el editor. UBT limpió sus salidas antiguas de hot reload como parte de la build normal. El resultado confirma construcción incremental y enlace; no equivale a una reconstrucción limpia ni a una prueba funcional.

La confirmación del autor es global: no se recibió una matriz detallada de cada caso ni se deducen resultados individuales. La build de cierre no demuestra por sí sola comportamiento visual, notifies, mappings ni ausencia de fallos en ejecución.

### Lista para futuras regresiones PIE (no ejecutada aquí)

- Iniciar/parar y repetir inicio; comprobar cadencia y un único daño por animación.
- Rechazar objetivo nulo, NPC no enemigo, cadáver y jugador muerto.
- Salir/entrar en alcance y arco; comprobar omisión y recuperación de ciclos.
- Pausar y reanudar varias veces; comprobar el temporizador y el impacto pendiente durante una animación.
- Cambiar objetivo y limpiar selección; confirmar que no continúa atacando al anterior.
- Muerte del objetivo y del propietario; salida de PIE/EndPlay durante ataque activo y pausado.
- Interrupción de Montage, ausencia de Montage y notify repetido.
- Repetir movimiento, cámara, selección y atributos como regresión.

## Alcance

Base local de autoataque con animación y daño sobre atributos. No acredita replicación/MMO, IA avanzada, habilidades, mitigación de armadura, críticos aplicados ni un juego completo. Sin comprobación de línea de visión ni alcance vertical en estas funciones. No hay endurecimiento numérico general de todos los parámetros de combate. El salto animado sigue pendiente según la documentación anterior.

El Hito 004 permanece como informe histórico del sistema previo al combate. Este hito añade su integración. Texto de portfolio: ../Portfolio/005-linkedin.md.
