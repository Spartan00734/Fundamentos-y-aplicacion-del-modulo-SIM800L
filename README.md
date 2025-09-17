# README - Fundamentos y aplicación del módulo SIM800L

**Equipo:** 4  
**Integrantes:** Pedro Iván Palomino Viera, Luis Antonio Torres Padrón  

---

## 1. Objetivo General
Diseñar e implementar una solución embebida que combine comunicaciones GSM (mensaje SMS y llamada telefónica) a través del módulo SIM800L y control remoto de un LED mediante MQTT. Se demostrará la integración de redes celulares e IoT para aplicaciones donde no se dispone de WiFi o Ethernet.

---

## 2. Objetivos Específicos
1. Investigar y usar comandos AT básicos del SIM800L para enviar SMS y gestionar llamadas.  
2. Configurar el hardware y la alimentación del SIM800L asegurando estabilidad de voltaje y señal.  
3. Implementar un cliente MQTT en el microcontrolador que reciba órdenes ON/OFF para controlar un LED.  
4. Integrar pruebas GSM con la lógica MQTT para demostrar versatilidad en entornos sin WiFi.  
5. Demostrar de forma individual el control remoto de un LED mediante mensajes MQTT, documentando el tópico, payload y resultados obtenidos.  

---

## 3. Competencias
- Aplicar principios de diseño de sistemas embebidos con dispositivos de radiofrecuencia.  
- Configurar módulos GSM y gestionar comandos AT para SMS, llamadas y GPRS.  
- Integrar protocolos IoT (MQTT) en entornos de red celular.  
- Documentar prácticas técnicas y resolver problemas de conectividad en escenarios reales.  

---

## 4. Tabla de Contenidos
1. Objetivo General  
2. Objetivos Específicos  
3. Competencias  
4. Tabla de Contenidos  
5. Descripción  
6. Requisitos  
7. Instalación y Configuración  
8. Conexiones de Hardware  
9. Parámetros Técnicos del SIM800L  
10. Uso y Ejemplos de Código  
11. Resultados de Prueba  
12. Consideraciones Éticas y de Seguridad  
13. Solución de Problemas  
14. Contribuciones  
15. Referencias  

---

## 5. Descripción
Esta práctica integra un módulo GSM SIM800L con un ESP32 para enviar SMS, realizar llamadas y controlar un LED a través de mensajes MQTT sobre GPRS. El sistema demuestra cómo combinar comunicaciones celulares tradicionales con protocolos IoT.  

---

## 6. Requisitos

### Hardware necesario
- ESP32 DevKit  
- Módulo SIM800L con antena  
- Fuente de alimentación estable 4.0–4.2 V (2 A)  
- LED y resistencia (220–330 Ω)  
- Protoboard y cables Dupont  

### Software requerido
- Arduino IDE actualizado  
- Monitor serie  
- Cliente MQTT (ej. MQTT Explorer o mosquitto_pub)  

### Conocimientos previos imprescindibles
- Programación en C/C++ para Arduino  
- Conceptos de redes celulares (GSM/GPRS) y protocolos de comunicación  
- Uso de la plataforma Arduino IDE  

---

## 7. Instalación y Configuración
1. Clonar o copiar el código de la práctica.  
2. Abrir el proyecto en Arduino IDE.  
3. Configurar el número de teléfono, APN, broker y tópico en el código.  
4. Subir el sketch al ESP32 con el SIM800L conectado.  
5. Abrir el Monitor Serie para verificar conexión y pruebas.  

---

## 8. Conexiones de Hardware

| Señal SIM800L | Pin ESP32       | Función                                    |
|---------------|-----------------|--------------------------------------------|
| VCC           | Fuente 4.0–4.2 V| Alimentación estable                       |
| GND           | GND             | Tierra                                     |
| TX            | GPIO16 (RX2)    | UART TX → RX ESP32                         |
| RX            | GPIO17 (TX2)    | UART RX → TX ESP32 (nivel 2.8–3.0 V)       |
| LED           | GPIO2           | LED interno o externo con resistencia      |

---

## 9. Parámetros Técnicos del SIM800L

| Parámetro              | Valor típico     | Unidad  |
|------------------------|-----------------|---------|
| Voltaje de operación   | 3.8–4.2         | V       |
| Consumo en transmisión | 2               | A (pico)|
| Baudrate               | 9600            | bps     |
| Frecuencia GSM         | 850/900/1800/1900 | MHz   |

---

## 10. Uso y Ejemplos de Código
El código implementa tres funciones principales:
1. Envía un SMS al número configurado.  
2. Realiza una llamada de voz y cuelga automáticamente.  
3. Establece una conexión GPRS y se suscribe a un tópico MQTT para controlar un LED con los mensajes ON/OFF.  

---

## 11. Resultados de Prueba
Durante las pruebas, el sistema envió correctamente un SMS al número configurado, realizó una llamada con colgado automático, y logró conectarse al broker público **test.mosquitto.org**, respondiendo a los mensajes **ON/OFF** para encender y apagar el LED.  

---

## 12. Consideraciones Éticas y de Seguridad
- No exponer públicamente credenciales APN o números de teléfono.  
- Usar brokers públicos solo para fines académicos, no para aplicaciones críticas.  
- Respetar la privacidad y no abusar del envío de SMS o llamadas.  

---

## 13. Solución de Problemas
- **Problema:** El SIM800L se reinicia al transmitir.  
  **Solución:** Revisar fuente de alimentación (mínimo 2 A).  

- **Problema:** No se registra en red GSM.  
  **Solución:** Revisar antena y cobertura.  

- **Problema:** No conecta al broker MQTT.  
  **Solución:** Verificar APN, broker y parámetros de red.  

---

## 14. Contribuciones
1. Realizar fork del repositorio.  
   Link del repositorio: [Fundamentos-y-aplicacion-del-modulo-SIM800L](https://github.com/Spartan00734/Fundamentos-y-aplicacion-del-modulo-SIM800L.git)  
2. Crear una rama `feature/mi-mejora`.  
3. Hacer commits documentados.  
4. Abrir un Pull Request describiendo cambios y pruebas.  

---

## 15. Referencias
- SIMCom Wireless Solutions. (2016). *SIM800L Hardware Design V1.00* [Datasheet].  
- SIMCom Wireless Solutions. (2016). *SIM800 Series AT Commands Manual V2.11*.  
- Espressif Systems. (2021). *ESP32 Technical Reference Manual (Rev. 3)*.  
- OASIS. (2015). *MQTT Version 3.1.1. OASIS Standard*.  
- IoT Bhai. (2023). *Interfacing SIM800L GSM Module with ESP32* [YouTube].  
- Kumar, R., & Priya, S. (2024). *Remote IoT Sensor Node Using SIM800L and ESP32*.  
