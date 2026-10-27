# LinkedIn — Hito 005 de Krerus

Borrador. No publicado.

Nuevo avance en Krerus, mi proyecto personal de desarrollo en Unreal Engine y C++: he completado y validado en PIE el Hito 005, centrado en una base de autoataque y en la gestión de su ciclo de vida.

El trabajo conecta selección de enemigos, temporizadores, animaciones de ataque y aplicación de daño sobre el sistema modular de atributos. También contempla validaciones de alcance y orientación, cambio de objetivo y detención del combate cuando muere el jugador o el enemigo.

Uno de los aprendizajes más útiles ha sido distinguir entre un autoataque habilitado, una pausa y un impacto pendiente. Gestionar esos estados, reiniciar la cadencia al reanudar y limpiar las suscripciones a eventos en EndPlay ayuda a razonar sobre los casos que aparecen entre un ataque y el siguiente.

También he trabajado en la recuperación de una referencia de componente que podía quedar nula tras Hot Reload, y en registrar los eventos de muerte solo después de validar el objetivo.

El cierre incluye una build incremental de editor satisfactoria y documentación que separa la revisión de código de las pruebas en PIE. Krerus sigue siendo un proyecto en desarrollo: este hito consolida una base local de combate para seguir aprendiendo y construyendo sobre ella.

#UnrealEngine #CPP #GameDevelopment #IndieDev #Krerus
