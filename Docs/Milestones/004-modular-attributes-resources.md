# Hito 004 — Sistema modular de atributos y recursos

Fecha de revisión: 26 de octubre de 2026. Unreal Engine: **5.8**, según `Krerus.uproject`.

## Objetivo y estado

Construir una base reutilizable de vida, recursos y atributos para jugador, NPC y enemigos. Sucede al [Hito 003 — Selección e interacción](003-npc-selection-interaction.md); la separación C++ previa permanece como antecedente técnico, no como hito independiente.

Implementación contrastada con las fuentes reales. El autor comunica pruebas Blueprint satisfactorias de vida, ira, modificadores y derivados, detalladas abajo. Esta revisión no repite Play ni certifica conexiones visuales mediante inspección de gráficos. Se intentó compilar `KrerusEditor Win64 Development`, pero UnrealBuildTool terminó con `OtherCompilationError` al no poder eliminar `UnrealEditor-Krerus-0002.dll`, cargada por el editor. El fallo es previo a la compilación de fuentes; no acredita ni descarta errores C++. Es necesario cerrar el editor y repetir la compilación y las pruebas antes de distribuir una build.

## Arquitectura y decisiones técnicas

| Archivo | Responsabilidad comprobada |
| --- | --- |
| `Public/KrerusAttributeComponent.h` / `Private/KrerusAttributeComponent.cpp` | Estado, operaciones, recálculos y delegates Blueprint |
| `Public/KrerusAttributeTypes.h` | Primarios, derivados, modificadores y coeficientes de escalado |
| `Public/KrerusResourceTypes.h` | Tipo y estado del recurso |
| `KrerusCharacter.h/.cpp` y `KrerusNPC.h/.cpp` | Propiedad `TObjectPtr` y creación del subobjeto `AttributeComponent` |

Las rutas C++ son relativas a `Source/Krerus`. `AKrerusEnemy` hereda el componente de `AKrerusNPC`; no crea una segunda copia. La lógica se concentra en un `UActorComponent` expuesto a Blueprint, evitando duplicarla por tipo de actor. El componente conserva Tick habilitado, pero su override solo llama a Super: no existe regeneración ni actualización periódica de atributos.

### Vida y muerte

`BaseMaxHealth` vale 100 por defecto; `MaxHealth` y `CurrentHealth` representan el estado efectivo. `ApplyDamage` y `Heal` rechazan cantidades no finitas o no positivas y actores muertos; limitan el resultado al intervalo permitido. `OnHealthChanged` avisa cuando cambia la vida y `OnDeath` al pasar de vivo a cero mediante daño. Repetir daño sobre un muerto no vuelve a emitir muerte.

`BeginPlay` limita la base a un mínimo de 1, recalcula atributos, pone la vida al máximo e inicializa el recurso. Por tanto, el valor editable de vida actual no determina la vida inicial efectiva. Esta inicialización sí llena la vida; los recálculos posteriores no curan ni resucitan. La asignación final de vida inicial no emite un aviso adicional: los consumidores deben consultar los getters al inicializarse.

### Recursos

`EKrerusResourceType` ofrece `None`, `Mana`, `Energy` y `Rage`. `FKrerusResource` contiene tipo, máximo y actual; el máximo por defecto es 100 y se limita a un mínimo de cero al inicializar. Maná y energía comienzan llenos; ira y None, a cero. None conserva el máximo configurado pero deshabilita consumo y recuperación.

`TryConsumeResource` devuelve false ante cantidades inválidas, None o saldo insuficiente; si acepta, descuenta, limita y emite `OnResourceChanged`. `RestoreResource` limita la recuperación al máximo y avisa solo si cambia el saldo. Durante este cierre se añade el rechazo de cantidades no finitas o no positivas en recuperación, ausente en el código recibido. Los getters exponen tipo, actual y máximo. Estas operaciones no exigen estar vivo: no se implementa aquí una política de recursos ligada a muerte.

### Primarios y modificadores independientes

`FKrerusPrimaryAttributes` contiene `Strength`, `Agility`, `Intellect` y `Stamina`, todos con base predeterminada 10. `BaseAttributes` conserva la configuración y `FinalAttributes` el resultado, limitado a cero como mínimo.

Cada llamada a `AddAttributeModifier` genera un nuevo `FGuid`, lo asigna a la copia del modificador, la agrega a `ActiveModifiers` y recalcula. El identificador recibido en la entrada se sustituye. `RemoveAttributeModifier` retira únicamente la entrada identificada; devuelve false si el GUID es inválido o no existe. `AttributeBonuses` agrega los bonos activos sin alterar las bases. No hay duración, prioridad, porcentajes ni persistencia de modificadores.

### Derivados, escalado y vida máxima

`FKrerusDerivedAttributes` contiene `AttackPower`, `SpellPower`, `Armor` y `CriticalChance`. `FKrerusAttributeScaling` permite ajustar los coeficientes en defaults/Blueprint.

| Resultado | Fórmula antes de limitar | Coeficientes por defecto |
| --- | --- | --- |
| AttackPower | Fuerza × StrengthToAttackPower + Agilidad × AgilityToAttackPower + bono directo | 1 y 0 |
| SpellPower | Intelecto × IntellectToSpellPower + bono directo | 1 |
| Armor | Aguante × StaminaToArmor + bono directo | 0 |
| CriticalChance | Agilidad × AgilityToCriticalChance + bono directo | 0 |
| MaxHealth | BaseMaxHealth + Aguante × StaminaToHealth | 10 |

Ataque, poder mágico y armadura tienen mínimo cero; crítico se limita a 0–100 y vida máxima a un mínimo de 1. El crítico es un valor calculado, todavía sin tirada ni efecto en daño. Los bonos directos de derivados se suman además de los aportados por primarios; no hay campo de bono directo a MaxHealth.

El flujo es: sumar modificadores → calcular primarios finales → derivados → vida máxima → notificar primarios. `OnAttributesChanged` y `OnDerivedAttributesChanged` comparan valores anteriores y actuales y solo emiten si hay diferencias reales. Un bono directo a AttackPower puede emitir derivados sin cambiar primarios. `OnHealthChanged` también avisa cuando cambia el máximo, aunque el actual permanezca igual.

Al aumentar el máximo se conserva la vida actual. Al reducirlo se aplica Clamp; un muerto permanece en cero. Esto evita que equipar o retirar un futuro bono de Aguante se convierta por sí mismo en una curación.

## Validación y evidencias

### Pruebas Blueprint comunicadas por el autor

| Caso | Resultado comunicado |
| --- | --- |
| Daño, curación y muerte | Cambios de vida y eventos comprobados; sin curación de muertos |
| Ira | Recuperación y consumo comprobados |
| Dos bonos de Fuerza, +5 y +3 | Acumulación y retirada selectiva mediante sus identificadores |
| Derivados | Recálculo al modificar primarios |
| Aguante con vida parcial | 150/200 → 150/250 → 150/200 |
| Aguante con personaje muerto | 0/200 → 0/250 → 0/200 |
| Bono directo a AttackPower | Modifica el derivado sin exigir un cambio primario |

Son resultados aportados en la solicitud de cierre, no pruebas automatizadas ejecutadas en esta revisión. Los valores 200/250 pertenecen a la configuración de las pruebas; no se deducen valores efectivos de todos los Blueprints a partir de los defaults C++.

### Revisión de cierre

- Confirmados tipos, fórmulas, herencia, eventos, límites y diferencias mediante lectura de fuentes y diff.
- Corrección puntual en `RestoreResource` para rechazar negativos, cero, NaN e infinitos antes de modificar estado.
- Compilación intentada y bloqueada por DLL de hot reload en uso; no se ejecutó Play de nuevo.
- El log local existente contiene dos entradas `LogAutomationTest: Error: Condition failed` al inicio del editor. Sin evidencia que las vincule al componente, no se presentan como resultados de pruebas del Hito 004 ni como una ejecución limpia.

Para reproducir tras cerrar el editor: compilar `KrerusEditor` en Development Editor para Windows, abrir `/Game/Dev/Maps/L_DevSandbox` y repetir los casos de la tabla. Comprobar además consumo insuficiente, None, cantidades negativas/cero/no finitas, saturación al recuperar, reducción del máximo por debajo de la vida actual, GUID inexistente y ausencia de eventos al no cambiar valores. Repetir movimiento, cámara, selección e interacción de 001–003 como regresión. La corrección de recuperación requiere una nueva prueba ejecutable.

## Alcance y límites

No se implementan combate, mitigación por armadura, aplicación de críticos, habilidades, IA avanzada, equipamiento real, regeneración automática ni replicación. El componente es una base para integrar esos sistemas más adelante; no es Gameplay Ability System ni demuestra multijugador. El salto animado de 001 sigue pendiente.

La validación de cantidades finitas corresponde a daño, curación, consumo y recuperación. No existe validación general de finitud para las bases, coeficientes o campos de modificadores; deben configurarse con valores válidos. No se presenta el sistema como endurecido frente a cualquier configuración numérica.

## Git y continuidad

Base revisada: `d8ec70c` (`feat: complete NPC selection and interaction milestone 003`). Convención conservada: prefijo de tipo, mensaje en inglés y número de milestone. Sin reescritura histórica, tags ni push.

El autor confirma como parte de 004 los cambios en `BP_KrerusCharacter`, `BP_KrerusNPC`, `BP_EnemyBase`, los nuevos `BP_EnemyMana` y `BP_EnemyRage`, y `L_DevSandbox`. Se incluyen junto a las fuentes. El vídeo ajeno al cierre `1.Videos/h3.mp4` queda sin añadir. La confirmación de pertenencia no sustituye una inspección de sus gráficos o valores efectivos.

Se separan el commit de implementación y assets del commit de informe, README y publicación de portfolio. La refactorización previa no recibe otro número. El siguiente trabajo funcional queda por decidir; no se declara iniciado un Hito 005.
