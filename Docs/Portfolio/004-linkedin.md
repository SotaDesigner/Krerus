# LinkedIn — Hito 004 de Krerus

He completado el Hito 004 de Krerus, mi proyecto personal en Unreal Engine 5.8 con C++: un sistema modular de atributos y recursos.

La base es un componente compartido por jugador, NPC y enemigos que gestiona vida, maná, energía o ira, atributos primarios y valores derivados. Los modificadores tienen un identificador propio, lo que permite acumular varios bonos y retirar uno sin afectar a los demás.

Una de las decisiones que más me ha hecho pensar ha sido cómo cambia la vida al modificar Aguante. Aumentar la vida máxima no debe curar al personaje; reducirla debe limitar la vida actual, y ninguno de esos cambios debe resucitar a un personaje muerto.

Lo he comprobado con pruebas en Blueprint: daño, curación y muerte; consumo y recuperación de ira; acumulación y retirada selectiva de bonos; recálculo de derivados y cambios de vida máxima. También he separado los eventos para notificar cambios de estado sin acoplar el cálculo a su futura presentación.

El aprendizaje principal ha sido definir reglas claras para cada cambio y comprobar sus efectos en cadena. Esta base todavía no implementa combate, habilidades ni equipamiento, pero deja preparado el sistema de atributos sobre el que podrán apoyarse.

Sigo avanzando en Krerus, documentando las decisiones y probando cada sistema paso a paso.

#UnrealEngine #CPP #GameDevelopment #Portfolio #Krerus
