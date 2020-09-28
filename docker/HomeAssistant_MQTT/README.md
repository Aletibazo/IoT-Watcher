# Home Assistant + MQTT server on docker using docker-compose
This is the basic folders structure to deploy Home Assistant and Mosquitto containters configured with TLS support.

After cloning this repo in a folder we're calling 'root_dir':

### Prepare
First step is generating certificates and keys to enable TLS support. To do this, go to certs folder and run make-keys.sh as root. 
NOTE: Don't forget to change IP and certificate attributes.

```
# cd root_dir/certs
# ./make_keys.sh
```

This will generate all certs needed and create certs folders under homeassistant and mosquitto, copying necessary certs to each folder.

### Deploy

Navigate to root_dir and run docker-compose:

```
# cd root_dir
# docker-compose -f docker-compose.yml up -d
```

Access Home Assistant on your_server_IP:80 and MQTT on your_server_IP:8883

In order to access MQTT you have to configure your client with ca.crt, client.crt and client.key on main certs folder.

