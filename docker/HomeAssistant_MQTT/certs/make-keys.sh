#!/bin/bash

# Script must be run as root
if [[ $(id -u) -ne 0 ]]
	then echo "Please, run script as root"
	exit 1
fi

# IP should be server's hostname or IP
# Don't forget to fill C, ST, L, O fields
IP="10.56.5.120"
SUBJECT_CA="/C=ES/ST=Prov/L=Loc/O=Org/OU=CA/CN=$IP"
SUBJECT_SERVER="/C=ES/ST=Prov/L=Loc/O=Org/OU=Server/CN=$IP"
SUBJECT_CLIENT="/C=ES/ST=Prov/L=Loc/O=Org/OU=Client/CN=$IP"

# Generate CA
echo "$SUBJECT_CA"
openssl req -x509 -nodes -sha256 -newkey rsa:2048 -subj "$SUBJECT_CA"  -days 365 -keyout ca.key -out ca.crt

# Generate server cert & key
echo "$SUBJECT_SERVER"
openssl req -nodes -sha256 -new -subj "$SUBJECT_SERVER" -keyout server.key -out server.csr
openssl x509 -req -sha256 -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 365

# Generate client cert & key
echo "$SUBJECT_CLIENT"
openssl req -new -nodes -sha256 -subj "$SUBJECT_CLIENT" -out client.csr -keyout client.key
openssl x509 -req -sha256 -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 365

# Create Mosquitto and Home Assistant certs folders and copy files
mkdir ../mosquitto/certs
cp ca.crt server.crt server.key ../mosquitto/certs/
mkdir ../homeassistant/certs
cp ca.crt client.crt client.key ../homeassistant/certs/