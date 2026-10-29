# Krerus: estado final de la limpieza de combate

29 de octubre de 2026. Proyecto real: `E:/15.MiVideoJuego/Krerus`, Unreal Engine 5.8.4. Este documento describe el estado tras la segunda pasada; sustituye el informe anterior para evitar describir consumidores que ya fueron retirados.

## Resultado

La muerte común y la inicialización pertenecen al ASC personalizado. Los valores de atributos y recursos pertenecen exclusivamente al AttributeSet y cambian mediante GAS. Autoataque y Strike son habilidades autoritativas. Enemy conserva presentación, estado replicado de muerte y retirada del cadáver. CombatStateComponent conserva las relaciones simétricas de combate.

Se eliminaron los eventos antiguos sin consumidores: OnDeath, OnAttributesChanged, OnDerivedAttributesChanged, OnHealthChanged y OnResourceChanged del componente. También se retiró OnAscHealthChanged del ASC, que duplicaba las notificaciones GAS sin consumidores. OnASCDeath se mantiene y lo consume Enemy. La función OnHealthChanged de AutoAttackAbility permanece: escucha directamente a GAS y detiene una habilidad cuando muere el atacante o su objetivo; no es el delegado antiguo.

## Segunda pasada: cambios y razones

| Archivo relativo al proyecto | Cambio |
| --- | --- |
| Source/Krerus/Public/KrerusAttributeComponent.h | Retirados los dos delegados restantes, InitializeGAS, callbacks de ciclo de vida y estado de suscripción. |
| Source/Krerus/Private/KrerusAttributeComponent.cpp | Eliminados BeginPlay/EndPlay propios, UnbindGAS, InitializeGAS y OnGASChanged. Ya no existen emisiones ni ocho suscripciones por instancia. |
| Source/Krerus/Private/KrerusCharacter.cpp | Retiradas las llamadas y recuperación del componente que solo servían para conectar esos eventos. |
| Source/Krerus/Private/KrerusNPC.cpp | Retirada la llamada a InitializeGAS del componente. El NPC sigue inicializando su ASC. |
| Source/Krerus/Public/KrerusResourceTypes.h | Retirados CurrentValue y MaxValue; la estructura conserva Type. Los máximos reales están en GAS y la configuración vigente está en InitialAttributes del actor. |
| Source/Krerus/Public/KrerusAttributeTypes.h | Eliminada FKrerusAttributeScaling, sin usos. Los coeficientes vigentes están en FKrerusAttributeInitialization. |
| Content/Dev/Maps/L_DevSandbox.umap | Retirada una entrada de delegado colgante en BP_EnemyRage_C_1 procedente del debug eliminado. |
| Docs/FlujoCombate.md y Docs/InformeLimpiezaCombate.md | Actualizados al estado final, sin atribuir consumidores activos a los mensajes retirados. |

No se eliminaron archivos completos ni se añadieron reemplazos de los eventos. No se ha hecho commit ni se han revertido los cambios locales del usuario.

## Usos revisados antes de borrar

Se contrastaron el código C++, los grafos inspeccionados y los nombres ASCII/UTF-16 en Content, Config y Scripts. Tras retirar el debug de BP_EnemyRage, no quedaban referencias a OnHealthChanged del componente, pero sí un nombre OnResourceChanged en el mapa. Se inspeccionó el mapa con Unreal: sus componentes no tenían consumidores válidos; el enemigo de ira conservaba una entrada que Unreal mostraba como `[<Unbound>]`, procedente de la función de debug borrada. Se limpió únicamente esa entrada y se guardó el mapa con copia previa.

El puente no ofrece una operación sobre delegados de instancias colocadas en mapas; esa limpieza se realizó con Python dentro de Unreal. El primer guardado encontró el mapa bloqueado por la sesión auxiliar de auditoría. Se verificó que sus bytes seguían idénticos a la copia anterior; el segundo guardado, con esa sesión ya cerrada, fue correcto. La búsqueda posterior no encontró los nombres eliminados en Content, Config o Scripts. No se borró una suscripción desconocida basándose solo en la búsqueda C++.

Antes de eliminar los campos de recurso y la estructura de escalado, se verificó que no existían referencias serializadas a sus nombres ni usos C++ fuera de sus declaraciones. No se dejaron pines de eventos ni estructuras sospechosos en los Blueprints validados.

## Qué queda en AttributeComponent y por qué

El componente sigue exponiendo consultas de vida, recursos y estadísticas; daño/curación, consumo/restauración de recursos y adición/retirada de bonificaciones. Estas operaciones consultan o aplican efectos sobre GAS. No mantiene un segundo almacén, no inicializa atributos, no observa cambios y no decide la muerte.

Se conserva esa API funcional para preservar las operaciones de recursos y bonificaciones solicitadas en el alcance original. Las pruebas de ejecución la ejercitan directamente. No se afirma que existan consumidores Blueprint jugables de sus eventos: esos consumidores y los propios eventos ya no existen. Eliminar el componente entero implicaría retirar esas operaciones o trasladarlas a otra API, una refactorización distinta de borrar notificaciones sobrantes.

## Cambios anteriores conservados

- InitializeAttributes crea InitialStatsEffect y DerivedStatsEffect correctamente, en lugar de crear dos efectos iniciales.
- Se retiró la suposición de que cualquier efecto activo equivale a estadísticas inicializadas. Los reintentos del efecto derivado no reinician primarios o recursos ya inicializados.
- ASC usa el Avatar para parar autoataque, limpiar relaciones y cancelar habilidades en autoridad. Enemy recibe OnASCDeath; no hace otra limpieza de relaciones ni repite InitAbilityActorInfo.
- Se retiraron configuración antigua del componente sin lecturas y el miembro PreviousHealth que solo necesitaba ser local.
- En BP_EnemyRage se eliminaron diez nodos de debug: eventos de vida/recurso, asignación, acceso al componente, formato/conversión e impresiones. Se preservaron los eventos del padre. Snapshot: BP_EnemyRage_20261029T145301f.uasset.

## Validación final

Todas las comprobaciones siguientes se ejecutaron con el código final de la segunda pasada:

| Verificación | Resultado |
| --- | --- |
| KrerusEditor Win64 Development, DLL normal | Compilación correcta, salida 0. |
| Combate individual | 23 de 23 comprobaciones correctas. |
| Servidor y cliente PIE | 13 de 13 correctas. |
| Recursos e interrupciones | 16 de 16 correctas. |
| Ausencia de eventos/campos, selección, muerte y cadáver | 9 de 9 correctas. |
| Siete Blueprints afectados | Cero pines sospechosos, cero errores y cero advertencias de compilación en cada uno. |

Las 61 comprobaciones cubren modificadores, derivados, límites, Strike/coste, daño remoto, cadencia, duplicados, pausa, parada, cambio de objetivo, alcance, orientación, muerte, relaciones, ausencia de miembros antiguos, retirada del cadáver y limpieza de selección. Las muestras de cadencia individual y de red mantuvieron intervalos aproximados de 2,000 a 2,016 segundos. No hubo excepción ni timeout en los resultados finales.

Se analizaron y compilaron con el puente Character, PlayerState, NPC, EnemyBase, EnemyMana, EnemyRage y Strike. Se conservaron los snapshots de compilación protegida en Saved/CodexBlueprintBridge/PreCompile. En esta segunda pasada no se editaron grafos Blueprint ni se guardaron esos assets; el cambio de Content corresponde al mapa y al BP_EnemyRage ya modificado en el paso anterior.

Evidencia: `C:/Users/Jose/Documents/ChatGPT/MiVideoJuego/LimpiezaCombate/segunda_pasada/`: build.log, validacion.json, eventos.json, blueprints.json, binding_mapa.json, cambios.diff y copias anteriores en antes/. La copia del mapa anterior es L_DevSandbox.antes.umap. Los registros y scripts de la auditoría y pasos anteriores están en la carpeta superior LimpiezaCombate.

## Límites

Las pruebas se ejecutaron sin renderizado; queda pendiente la revisión visual con GPU de animaciones y presentación. La prueba de red usa mundos PIE bajo el mismo proceso, sin certificar latencia real o máquinas físicas separadas. Se conservan los avisos previos de DefaultSlot duplicado y GameplayCueNotifyPaths. No se ha añadido una política de resurrección, IA ofensiva o regeneración. El cadáver sigue ocultándose y desactivando colisiones sin destruir el actor.

El proyecto tenía cambios locales anteriores y espacios finales en código ajeno a esta pasada. El diff de esta intervención se conserva por separado y no pretende atribuirse toda la diferencia frente a Git.
