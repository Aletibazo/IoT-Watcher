# IoT-Watcher

Open source monitoring system for IT infrastructures using IoT sensors.

Related paper: https://doi.org/10.1007/978-3-031-48590-9_15

## Motivation

In Data Centers there are usually monitoring solutions offered by the manufacturer, which integrate all the equipment and sensors that make up the solution in a single system. In the CITIC data centre, all the sensors are integrated into an APC system that allows the thresholds of each type of sensor to be managed and alerts to be configured.

These proprietary solutions work very well, but it is difficult to add external elements to these systems, since they have closed APIs and it is difficult to obtain the appropriate documentation. In the specific case of the CITIC, it is desired to be able to integrate the monitoring of the servers housed in the CPD cabinets together with the monitoring of the environmental parameters of the room, thus centralising all the elements that make up the CPD and that are managed by the administrator.

## Description

This project aims to develop an open, multi-platform system that allows for the addition of different types of sensors and machines. With this secondary objective in mind, developments will be made on open hardware platforms for the sensor part and, whenever possible, open-source operating systems, libraries, external programs, etc. will be used with the goal of having a system that is as open and compatible as possible.

The monitoring server will be based on Zabbix, a widely used monitoring tool in the systems world. In the first phase, servers will be added to this system using the Zabbix agents for Windows, Linux and VMware, and their correct operation will be checked by taking data and comparing it with the servers themselves. The monitoring server forms the basis of the project, and once it is deployed and tested, the sensor section will be developed.

Sensors compatible with open-source and open-hardware Arduino boards will be used, always keeping in mind that the sensors used in this project can be replaced by others with similar characteristics. In the particular case of this project, temperature, humidity, presence and electrical consumption sensors will be used. First, the Arduino program will be developed to read data from the sensors and send it to a Raspberry Pi that will be responsible for centralizing the data from all the Arduinos in the infrastructure and sending it to the Zabbix server. Then each sensor will be placed in its corresponding place in the DC, for which issues such as autonomy, placement in racks, etc. will be taken into account.

An open-source application with the MQTT protocol will be used for data transmission between the Arduinos and the Raspberry Pi. With the rise of the IoT, multiple open-source projects have emerged to control home automation elements, so one of these projects will be used to collect all the data from the sensors and the necessary functionalities will be added to send the data to the configured Zabbix server. To achieve this communication between Raspberry and Zabbix, a specific agent will be developed.

The possibility of implementing a prediction system based on Machine Learning models that launches alerts of possible failures in the infrastructure will be considered. The system is intended to be able to alert of failures that may occur in all elements of the DC - from end-of-life warnings for disks to possible failures in the cooling system - based on historical data and real-time data.
