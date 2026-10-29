# Krerus: flujo real de combate

Revisión: 29 de octubre de 2026, Unreal Engine 5.8.4. Describe el código aplicado en `Source/Krerus`, no una arquitectura propuesta.

```mermaid
flowchart TD
    Player[Jugador: KrerusCharacter] --> PS[PlayerState: ASC personalizado + AttributeSet]
    NPC[NPC / Enemy] --> NASC[ASC personalizado + AttributeSet en NPC]
    PS --> ActorInfo[Owner = PlayerState / Avatar = Character]
    NASC --> NPCInfo[Owner = Avatar = NPC]
    ActorInfo --> Init[Servidor: InitializeAttributes con InitialAttributes del actor]
    NPCInfo --> Init
    Init --> Initial[InitialStatsEffect instantáneo: primarios, energía e ira]
    Initial --> Derived[DerivedStatsEffect persistente: estadísticas derivadas y escalado]
    Derived --> Ready[ASC inicializado una vez; no se reinicia al revincular el Pawn]
    Input[Entrada local] --> Select[PlayerController: selección de NPC y anillo Blueprint]
    Select --> Auto[Alternar autoataque]
    Select --> Strike[Tecla Strike]
    Auto --> RPC[RPC de Character: objetivo explícito validado por servidor]
    Strike --> SRPC[ServerActivateStrike: habilidad concedida y objetivo explícito]
    RPC --> Combat[CombatComponent: intención, objetivo, pausa y estado]
    Combat --> AA[AutoAttackAbility: GAS ServerOnly]
    AA --> Validate[Validar vida, mundo, objetivo, alcance 2D y orientación]
    Validate --> Wait[WaitDelay: cadencia sin recuperar retrasos con ráfagas]
    Wait --> Montage[Multicast: montage en servidor y clientes]
    Montage --> Notify[AnimNotify AttackImpact en AnimBlueprint]
    Notify --> Impact[Servidor: evento GAS de impacto; máximo un impacto pendiente]
    Impact --> Recheck[Revalidar objetivo, vida, alcance, orientación y pausa]
    Recheck --> AutoDamage[GE_KrerusAutoAttackDamage: Data.Damage negativo]
    SRPC --> SA[StrikeAbility: GAS ServerOnly]
    SA --> SValidate[Validar vida, mundo, objetivo y distancia 3D]
    SValidate --> Commit[CommitAbility: coste GAS configurado]
    Commit --> StrikeDamage[GE_KrerusStrikeDamage]
    AutoDamage --> TargetASC[ASC del objetivo aplica GameplayEffect]
    StrikeDamage --> TargetASC
    TargetASC --> Attributes[AttributeSet: Health y límites de atributos / recursos]
    Attributes --> Alive{Health mayor que cero?}
    Alive -->|Sí| Relation[CombatStateComponent: relación simétrica entre actores]
    Alive -->|Daño no mortal| Hit[Enemy: reacción de impacto por multicast]
    Alive -->|Transición a cero| Death[ASC: HandleDeath solo en autoridad]
    Death --> Stop[Parar autoataque, limpiar relaciones y cancelar habilidades]
    Stop --> Event[OnASCDeath]
    Event --> EnemyDeath[Enemy: bIsDead, animación y BP_OnEnemyDeath]
    EnemyDeath --> Timer[Servidor: temporizador CorpseDuration]
    Timer --> Expire[bCorpseExpired replicado]
    Expire --> Hide[Ocultar actor y desactivar colisión; no destruir]
    Hide --> Clear[Cada controlador local retira su selección si apunta al cadáver]
    Attributes -.-> View[AttributeComponent: consultas GAS sin eventos propios]
    View --> ExtraEffects[Operaciones de recursos y bonificaciones: GameplayEffects]
    ExtraEffects --> OwnASC[ASC del actor propietario]
    OwnASC --> Attributes
    PS -.-> Rep[GAS: atributos replicados; efectos Mixed en jugador]
    NASC -.-> RepNPC[GAS: atributos y efectos Full en NPC]
    Combat -.-> OwnerRep[Objetivo, autoataque y pausa: solo propietario]
    Relation -.-> StateRep[bIsInCombat replicado]
    EnemyDeath -.-> DeadRep[bIsDead replicado: OnRep aplica presentación]
```

## Responsabilidades

| Elemento | Responsabilidad |
| --- | --- |
| ASC personalizado | Inicialización autoritativa única y transición común de muerte. Usa Avatar, no el PlayerState, para buscar componentes de combate. |
| AttributeSet | Almacén GAS único, límites y notificaciones de replicación de los 16 atributos. |
| GameplayEffects | Inicialización, derivados, bonificaciones, variaciones de recursos, daño y coste. |
| AutoAttackAbility | Esperas GAS, impacto único, cancelación y cadencia. Conserva el límite temporal entre activaciones. |
| StrikeAbility | Validación y ejecución autoritativa de Strike y su coste. El asset existente se llama GE_KrerusStrike aunque sea una habilidad. |
| CombatComponent | Configuración e intención del autoataque; no aplica daño por su cuenta. |
| CombatStateComponent | Relaciones simétricas, estado de combate y limpieza también en EndPlay. |
| Enemy | Reacción visual, estado persistente de muerte, animación y duración/retirada del cadáver. |
| AttributeComponent | Consultas y operaciones Blueprint sobre GAS; sin eventos, suscripciones, inicialización ni lógica de muerte. |
| PlayerController | Selección local y anillo; el servidor recibe el objetivo explícito para cada acción. |

## Interrupciones y red

Parar, pausar, cambiar de objetivo, interrumpir el montage o morir invalida el impacto pendiente. La habilidad observa también Health del objetivo para detenerse cuando este muere. Los notifies del cliente no aplican daño. Salir de alcance u orientación bloquea el impacto; volver a una posición válida permite continuar.

El servidor concede autoataque y la habilidad inicial en PlayerState. Character vincula el ASC desde PossessedBy y OnRep_PlayerState. NPC configura su propio Owner/Avatar; Enemy reutiliza ese vínculo. La vida GAS decide la muerte; bIsDead replica su presentación y cubre el orden de llegada de los paquetes. La notificación nativa OnASCDeath no es un RPC.

Se retiraron los eventos de vida/recurso del componente y sus consumidores de debug. Una futura interfaz debe consultar el valor inicial y escuchar directamente los delegados GAS de los atributos que necesita. El código actual no crea otra capa de notificación. FKrerusResource solo selecciona Type; valores y máximos proceden del AttributeSet y la configuración de InitialAttributes.

No se ha añadido IA ofensiva para NPC, regeneración, resurrección, recompensas ni destrucción del cadáver: el flujo refleja las funcionalidades existentes.
