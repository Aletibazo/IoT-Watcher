# TFM

Sistema de monitorizaciónde código abierto para infraestructuras de TI incorporando sensórica basada en IoT

## Motivación

En los centros de procesamiento de datos habitualmente existen soluciones para monitorización ofrecidas por el fabricante, que integran todos los equipos y sensores que conforman la solución en un único sistema. En el CPD del CITIC se tienen todos los sensores integrados en un sistema APC que permite gestionar umbrales de cada tipo de sensor y configurar alertas.

Estas soluciones propietarias funcionan muy bien, pero resulta complicado añadir elementos externos a estos sistemas, ya que tienen APIs cerradas y resulta complicado conseguir la documentación adecuada. En el caso concreto del CITIC se desea poder integrar la monitorización de los servidores alojados en los armarios del CPD junto con la monitorización de parámetros ambientales de la sala, centralizando así todos los elementos que conforman el CPD y que son gestionados por el administrador.

## Descripción

Con este proyecto se quiere desarrollar un sistema abierto multiplataforma que permita añadir distintos tipos de sensores y máquinas. Con este objetivo secundario en mente, se realizarán desarrollos sobre plataformas hardware abiertas para la parte de sensórica y se utilizarán, siempre que sea posible, sistemas operativos, librerías, programas externos, etc. open-source con la meta de tener un sistema lo más abierto posible.

El servidor de monitorización estará basado en Zabbix, una herramienta para monitorización ampliamente usada en el mundo de sistemas. En la primera fase se añadirán los servidores a este sistema utilizando los agentes Zabbix para Windows, Linux y VMware, y se comprobará su correcto funcionamiento tomando datos y cotejándolos con los propios servidores. El servidor de monitorización forma la base del proyecto, y una vez desplegado y probado se pasará a desarrollar el apartado de sensórica.

Se utilizarán sensores compatibles con placas Arduino open-source y open-hardware, teniendo siempre en mente que los sensores utilizados en este proyecto puedan ser sustituidos por otros con características similares. En el caso particular de este proyecto, se utilizarán sensores de temperatura, humedad, presencia y consumo eléctrico. En primer lugar, se desarrollará el programa en Arduino que lea datos de los sensores y los envíe a una Raspberry Pi que se encargará de centralizar los datos de todos los Arduinos en la infraestructura y enviarlos al servidor Zabbix. Luego se colocará cada sensor en su lugar correspondiente en el CPD, para lo que se tendrán en cuenta cuestiones como autonomía, colocación en los racks, etc.

Se utilizará alguna una aplicación open-source con el protocolo MQTT para la transmisión de datos entre los Arduinos y la Raspberry Pi. Con el auge del IoT han surgido múltiples proyectos open-source para controlar los elementos de domótica en hogares, por lo que se empleará uno de estos proyectos para recoger todos los datos de los sensores y se añadirán las funcionalidades necesarias para enviar los datos al servidor Zabbix configurado. Para conseguir esta comunicación entre Raspberry y Zabbix se desarrollará un agente específico.

Se tendrá en cuenta la posibilidad de implementar un sistema de predicción basado en modelos de Machine Learning que lance alertas de posibles fallos en la infraestructura. Se pretende que el sistema sea capaz de alertar de fallos que se puedan producir en todos los elementos del CPD -desde avisos por fin de vida de discos hasta posibles fallos en el sistema de refrigeración- basándose en los datos históricos y en los datos en tiempo real.
