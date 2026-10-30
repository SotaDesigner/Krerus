# Krerus — base reutilizable de habilidades de combate GAS

Fecha: 30 de octubre de 2026. Estado: diseño para una migración incremental; no se ha cambiado aún la jerarquía de habilidades.

## Objetivo y primera entrega

Crear nuevas habilidades dirigidas e instantáneas mediante configuración de una base común, sin copiar `UKrerusStrikeAbility::ActivateAbility`. Golpe demoledor será la primera migración y conservará 15 de daño físico base, coste de 15 Ira, alcance de 600 unidades y Fractura después del daño si el enemigo sobrevive. El autoataque conservará su habilidad propia: su ciclo de montajes, pausas y tareas tiene una duración y unas responsabilidades diferentes.

La primera base atenderá ataques instantáneos contra un enemigo. Proyectiles, canalizaciones, áreas, curación y selección de aliados se incorporarán como flujos específicos que reutilicen la construcción y aplicación de efectos, sin añadir ramas para todas las clases en una única función.

## Responsabilidades

| Elemento | Responsabilidad |
| --- | --- |
| `UKrerusTargetedCombatAbility` (nuevo) | Validar emisor/objetivo, alcance y tags; construir specs; confirmar coste/cooldown; aplicar daño y efectos; terminar la habilidad por todas las salidas. |
| `UKrerusStrikeAbility` (compatibilidad) | Mantener la clase padre que usan los Blueprints existentes; resolver inicialmente `Character->GetStrikeTarget()`. Al terminar la migración no contendrá un flujo de combate duplicado. |
| Blueprint de cada habilidad | Configurar daño, alcance, tipo de daño y clases de efectos. Usar los campos GAS heredados para coste y cooldown. |
| Gameplay Effects | Coste del recurso, duración, acumulaciones, refresco, inmunidades, requisitos de aplicación y cues. |
| `UKrerusDamageExecution` | Capturar defensas del objetivo y calcular mitigación. Una sola implementación de la fórmula. |
| `AKrerusCharacter` y RPC de ranura | Recibir la intención del cliente y encontrar el spec concedido por el servidor. No aceptar daño, coste o clases de efectos del cliente. |
| `UKrerusClassDataAsset` | Seleccionar habilidades iniciales de cada clase; no duplicar sus estadísticas de combate. |

Se mantiene `InstancedPerActor` y `ServerOnly` en la primera migración. La predicción requerirá un diseño posterior de target data y prediction keys, además de pruebas con latencia; no se activa incidentalmente al extraer una base.

## Configuración propuesta

| Campo | Tipo/valor inicial | Decisión |
| --- | --- | --- |
| `DamageEffectClass` | `TSubclassOf<UGameplayEffect>` | Conservar nombre y tipo actuales para facilitar compatibilidad. |
| `BaseAbilityDamage` | `float`, 15 | Conservar el valor serializado; rechazar valores negativos o no finitos. Escalado por nivel mediante un campo nuevo opcional `FScalableFloat` en una fase posterior. |
| `AbilityRange` | `float`, 600 | Distancia 3D, como Strike actual; finita y no negativa. No cambiar silenciosamente a distancia horizontal. |
| `DamageTypeTag` | `FGameplayTag`, `Damage.Type.Physical` | El GE spec lleva el tag dinámico. La ejecución interpreta explícitamente los tipos admitidos. |
| `AdditionalEffectClass` | Clase GE actual | Mantener temporalmente el campo existente para Fractura. |
| `AdditionalEffects` | Array de `FKrerusCombatEffectEntry` | Nuevas habilidades pueden tener varios efectos sin copiar código. No aplicar dos veces una clase configurada también en el campo de compatibilidad. |
| `RequiredTargetTags`, `BlockedTargetTags` | `FGameplayTagContainer` | Requisitos del objetivo antes de gastar recursos; conservar requisitos GAS nativos para el emisor. |
| `bRequireLineOfSight` | `bool`, false para Strike | Política explícita; definir canal, excepciones del emisor y objetivo, y probar obstáculos antes de usarla. |
| Coste y cooldown | Campos GAS heredados | Usar `CostGameplayEffectClass` y `CooldownGameplayEffectClass`; evitar una segunda resta manual. Cada GE declara si consume Ira, maná u otro atributo. |

`FKrerusCombatEffectEntry` contiene `EffectClass`, destinatario (`Target` o `Source`), condición (`OnSuccessfulDamageApplication` o `OnTargetSurvivedDamage`) y requisitos/bloqueos de tags del destinatario. Para un efecto que se deba aplicar siempre al activar, diseñar una condición separada; no reutilizar «golpe exitoso» con semántica ambigua. La primera migración de Fractura equivale a `Target + OnTargetSurvivedDamage`.

«Aplicación exitosa» significa que GAS aceptó el spec; no implica daño positivo ni ausencia de absorción. Si una futura habilidad requiere daño efectivo positivo, usar la variación real de Health o un resultado de ejecución explícito, no el handle como sustituto. El snapshot del objetivo y sus punteros deben seguir siendo válidos después de aplicar daño: la muerte puede ejecutar callbacks de forma síncrona.

## Flujo de activación

1. El cliente envía ranura y referencia de objetivo. El servidor valida la ranura y busca la habilidad que él mismo concedió.
2. Resolver el objetivo desde el contexto de esa activación. No consultar la selección local del servidor ni conservar un puntero sin limpiar entre activaciones. La extracción inicial seguirá siendo síncrona; antes de añadir tareas latentes habrá que sustituir `StrikeTarget` temporal por target data/contexto propio de la activación.
3. Validar autoridad; Avatar, controlador y ASC; mismo mundo; emisor y objetivo vivos; objetivo permitido; valores finitos; alcance y requisitos de tags. Las restricciones dinámicas seguirán residiendo también en GAS.
4. Construir los specs obligatorios con `SourceASC->MakeEffectContext()` y `SourceASC->MakeOutgoingSpec()`. Añadir el emisor como SourceObject y usar el nivel de habilidad real. Mantener `Data.Damage` como **FGameplayTag** y el convenio negativo actual hasta migrar explícitamente todos sus consumidores.
5. Ejecutar `CommitAbility` una sola vez después de validar y construir los specs. Este es el punto de gasto del recurso y aplicación del cooldown.
6. Aplicar el spec de daño al ASC del objetivo. El tag de tipo de daño debe viajar en ese mismo spec, y no proceder de datos libres del cliente.
7. Revalidar objetivo/ASC tras callbacks. Aplicar efectos adicionales en el orden configurado y con sus condiciones. Fractura sigue al daño, de modo que el primer golpe contra 50 de armadura hace 10 y el siguiente con 42,5 hace aproximadamente 10,5263.
8. Registrar la relación de combate si procede; emitir cues/eventos apropiados. Ejecutar `EndAbility` exactamente una vez por cada salida lógica. Evitar logs de éxito por impacto en nivel Warning.

No prometer una transacción atómica de varios Gameplay Effects: GAS puede rechazar una aplicación después del commit por inmunidad o requisitos. La política inicial será consumir el coste cuando la ejecución se haya comprometido y no devolverlo automáticamente. Fallos de configuración detectables antes del commit deben cancelar sin coste. Documentar y probar esa distinción; los fallos posteriores necesitan un resultado claro, sin aplicar Fractura si falló el daño.

## Tipos de daño y extensiones

Actualmente solo está implementada la mitigación por armadura. Registrar `Damage.Type.Physical` con un tag nativo y darle compatibilidad al daño antiguo sin tag durante la migración. Rechazar una combinación ambigua de varios tipos.

No presentar daño mágico como funcional por añadir un tag. Antes de habilitar `Damage.Type.Magical` se necesita decidir atributo de resistencia, captura y fórmula, y escribir la prueba correspondiente. Paladín, Mago u otras clases pueden compartir la base y configurar costes/efectos, pero sus mecánicas particulares no se inventan durante esta extracción.

## Migración sin perder valores de los assets

1. Conservar el resultado de la prueba multijugador actual como línea base y corregir regresiones antes de extraer lógica.
2. Crear snapshots de `GA_KrerusStrike` y `GA_KrerusDemolishingStrike` y registrar sus valores actuales, padres, coste, daño y Fractura. Inspeccionar qué otros Blueprints heredan de Strike.
3. Extraer el flujo a la nueva base; mantener `UKrerusStrikeAbility` y sus nombres de propiedades. Cambiar la propiedad de una UPROPERTY a una superclase puede afectar serialización y referencias: comprobar los valores reales después de reiniciar el editor; no asumir que compilar C++ garantiza la migración del asset.
4. Compilar C++, reiniciar y compilar Blueprints con diagnóstico. Verificar que no hay pines obsoletos ni valores de configuración restablecidos. Crear redirects o una migración explícita solo si es necesaria y comprobarlos.
5. Repetir la prueba de Guerrero. Añadir una segunda habilidad de prueba con distinta configuración, coste y condiciones para demostrar reutilización real. No crear habilidades de producción incompletas de otras clases solo como ejemplo.
6. Retirar el campo adicional singular únicamente tras migrar sus consumidores y probar equivalencia. Mantener el autoataque independiente y reutilizar helpers de specs solo cuando eviten duplicación concreta.

## Criterios para aceptar la extracción

- Prueba de Guerrero anterior y posterior equivalente: tres impactos, +15 Ira, coste 15, daño 10, Fractura 42,5, refresco sin acumulación y restauración a 50 en servidor/cliente.
- Rechazos sin gasto: objetivo inválido, muerto, otro mundo, sin ASC, fuera de alcance, tags bloqueados y recursos insuficientes.
- Validación de valores no finitos o negativos, ranura no concedida y objetivo destruido por callbacks.
- Dos efectos adicionales en orden definido; bloqueo por tags; efecto sobre emisor; ningún duplicado del campo de compatibilidad.
- Nuevos casos sin duplicar ActivateAbility y con costes GAS aplicados una sola vez.
- Pruebas posteriores en procesos separados, servidor dedicado y latencia/pérdida de paquetes antes de afirmar cobertura de red de producción.

## Referencias de la plataforma

La propuesta usa el ciclo de `UGameplayAbility` y el commit de costes/cooldowns documentados por [Epic: Gameplay Abilities](https://dev.epicgames.com/documentation/unreal-engine/using-gameplay-abilities-in-unreal-engine). Los buffs/debuffs, sus specs, duración y políticas de acumulación permanecen en [Gameplay Effects](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-effects-for-the-gameplay-ability-system-in-unreal-engine); el refresco usa [RefreshOnSuccessfulApplication](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/EGameplayEffectStackingDurationP-). La estructura concreta de clases y la secuencia de migración son decisiones de diseño para Krerus.
